#include "thread.h"
#include <ucontext.h>
#include <deque>
#include <iostream>
#include <map>
#include "interrupt.h"
using namespace std;

#define DEBUG(x) cerr << x << endl

// Thread TCB structure
struct Thread {
    unsigned int id;
    ucontext_t* pucontext;
    char* stack;
    bool isFinished;
};

// Mutex lock structure
struct Mutex {
    Thread* owner;
    deque<Thread*>* qBlocked;
};

static bool init = false;                       // is this library instance initialized?
static int tid = 0;                             // threadID allocator
static Thread* pthreadCurrent;                  // refers to the running(will run, exactly) user thread
static ucontext_t* pscheduler;                  // pscheduler refers to the while-loop scheduler in thread_init()
static deque<Thread*> qReady;                   // queue for ready threads
static map<unsigned int, Mutex*> mLock;         // mutex lock table
static map<unsigned int, deque<Thread*>*> mCV;  // conditional variable table

///////
// func
///////

// Execute current thread in context `func` with parameter `arg`.
static void start(thread_startfunc_t func, void* arg) {
    // allow interruption, exec func, and disaable interruption again
    interrupt_enable();
    func(arg);
    interrupt_disable();
    // mark this thread as finished
    pthreadCurrent->isFinished = true;
    swapcontext(pthreadCurrent->pucontext, pscheduler);
}

// Recycle any resources associated to current thread.
static void deleteCurrentThread() {
    delete pthreadCurrent->stack;
    pthreadCurrent->pucontext->uc_stack.ss_sp = NULL;
    pthreadCurrent->pucontext->uc_stack.ss_size = 0;
    pthreadCurrent->pucontext->uc_stack.ss_flags = 0;
    pthreadCurrent->pucontext->uc_link = NULL;
    delete pthreadCurrent->pucontext;
    delete pthreadCurrent;
    pthreadCurrent = NULL;
}

/////////////////
// thread library
/////////////////

int thread_libinit(thread_startfunc_t func, void* arg) {
    // if already initialized - exit
    if (init == true) {
        // cerr << "- Multiple thread_libinit() call." << endl;
        return -1;
    }

    init = true;  // this instance has been initialized

    // create init thread
    if (thread_create(func, arg) != 0) {
        // cerr << "- FAILED creating init thread." << endl;
        return -1;
    }

    Thread* pthreadInit = qReady.front();
    qReady.pop_front();
    pthreadCurrent = pthreadInit;  // initial thread created, make it current thread (will run)

    try {
        pscheduler = new ucontext_t;
    } catch (std::bad_alloc err) {  // in case tons of threads created
        delete pscheduler;
        return -1;
    }

    getcontext(pscheduler);  // initialize pscheduler by copying current context

    interrupt_disable();
    swapcontext(pscheduler, pthreadInit->pucontext);  // save current context (this scheduler) into pscheduler, then switch to pucontext

    while (qReady.empty() == false) {
        if (pthreadCurrent->isFinished == true) {
            // recycle current thread context
            deleteCurrentThread();
        }
        // if returned to this scheduler, switch to next thread
        Thread* pthreadNext = qReady.front();
        qReady.pop_front();
        pthreadCurrent = pthreadNext;
        swapcontext(pscheduler, pthreadCurrent->pucontext);  // return to the running thread
    }

    if (pthreadCurrent != NULL) {
        // recycle current(last) thread context
        deleteCurrentThread();
    }

    // theoretically should do this
    // interrupt_enable();

    // required output
    cout << "Thread library exiting.\n";
    exit(0);
}

int thread_create(thread_startfunc_t func, void* arg) {
    if (init == false) {
        // cerr << "- Must call thread_libinit() before thread_create()." << endl;
        return -1;
    }

    Thread* pthread;

    interrupt_disable();

    try {
        // make a new thread and its context
        pthread = new Thread;
        pthread->pucontext = new ucontext_t;
        getcontext(pthread->pucontext);  // "Initialize a context structure by copying the current thread's context."
        // "Direct the new thread to use a different stack."
        // "Your thread library should allocate STACK_SIZE bytes for each thread's stack."
        pthread->stack = new char[STACK_SIZE];
        pthread->pucontext->uc_stack.ss_sp = pthread->stack;
        pthread->pucontext->uc_stack.ss_size = STACK_SIZE;
        pthread->pucontext->uc_stack.ss_flags = 0;
        pthread->pucontext->uc_link = NULL;
        // Direct the new thread to start by calling start(arg1, arg2).
        makecontext(pthread->pucontext, (void (*)())start, 2, func, arg);

        // allocate a ThreadID
        pthread->id = tid;
        tid++;
        pthread->isFinished = false;
        qReady.push_back(pthread);  // append the new thread into ready queue
    } catch (std::bad_alloc err) {
        delete pthread->pucontext;
        delete pthread->stack;
        delete pthread;

        interrupt_enable();
        return -1;
    }

    interrupt_enable();
    return 0;
}

int thread_yield(void) {
    if (init == false) {
        // cerr << "- Must call thread_libinit() before thread_yield()." << endl;
        return -1;
    }

    interrupt_disable();
    qReady.push_back(pthreadCurrent);
    swapcontext(pthreadCurrent->pucontext, pscheduler);  // return to the scheduler
    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    if (init == false) {
        // cerr << "- Must call thread_libinit() before thread_lock()." << endl;
        return -1;
    }

    interrupt_disable();

    auto iter = mLock.find(lock);
    if (iter == mLock.end()) {
        // lock doesn't exist yet. this shall be a new lock
        Mutex* mutex = NULL;
        try {
            mutex = new Mutex;
            mutex->owner = pthreadCurrent;
            mutex->qBlocked = new deque<Thread*>;
        } catch (std::bad_alloc err) {
            delete mutex->qBlocked;
            delete mutex;
            interrupt_enable();
            return -1;
        }
        mLock.insert(std::make_pair(lock, mutex));

        interrupt_enable();
        return 0;  // normal return
    } else {
        Mutex* mutex = iter->second;
        if (mutex->owner == NULL) {
            // a previous unlocked lock being locked again
            mutex->owner = pthreadCurrent;

            interrupt_enable();
            return 0;
        } else {
            if (mutex->owner->id == pthreadCurrent->id) {
                // DANGEROUS: locking a mutex locked by itself, leads to DEADLOCK
                // "trying to acquire a lock by a thread that already has the lock IS an error."
                // cerr << "- Lock # " << lock << " has already locked. Waiting a lock held by the thread itself could cause DEADLOCK." << endl;

                interrupt_enable();
                return -1;  // error
            } else {
                // waiting a lock
                mutex->qBlocked->push_back(pthreadCurrent);  // current thread is waiting for this lock
                swapcontext(pthreadCurrent->pucontext, pscheduler);

                interrupt_enable();
                return 0;  // normal return
            }
        }
    }
    return 0;
}

int thread_unlock(unsigned int lock) {
    if (init == false) {
        // cerr << "- Must call thread_libinit() before thread_unlock()." << endl;
        return -1;
    }

    interrupt_disable();

    auto iter = mLock.find(lock);
    if (iter == mLock.end()) {
        // lock not found
        // cerr << "- Lock # " << lock << " does NOT EXIST." << endl;

        interrupt_enable();
        return -1;  // error
    } else {
        Mutex* mutex = iter->second;
        if (mutex->owner == NULL) {
            // lock held by nobody. Theoretically this will not happen
            // cerr << "- Lock # " << lock << " is not hold by any thread." << endl;

            interrupt_enable();
            return -1;  // error
        } else if (mutex->owner->id != pthreadCurrent->id) {
            // current thread does not own this lock: trying to unlock other's lock
            // cerr << "- Lock #" << lock << " is not hold by this thread, thus cannot be unlocked by it." << endl;

            interrupt_enable();
            return -1;  // error
        } else {
            // expected situation: lock held by itself
            if (mutex->qBlocked->empty() == false) {
                // has waiting thread
                mutex->owner = mutex->qBlocked->front();
                mutex->qBlocked->pop_front();
                qReady.push_back(mutex->owner);
            } else {
                // has no waiting thread
                mutex->owner = NULL;
            }
            interrupt_enable();
            return 0;  // normal return
        }
    }
    // will never be here
    return 0;
}

int thread_unlock_with_interrupt_disabled(unsigned int lock) {
    if (init == false) {
        return -1;
    }

    auto iter = mLock.find(lock);
    if (iter == mLock.end()) {
        // lock not found
        return -1;  // error
    } else {
        Mutex* mutex = iter->second;
        if (mutex->owner == NULL) {
            // lock held by nobody. Theoretically this will not happen
            return -1;  // error
        } else if (mutex->owner->id != pthreadCurrent->id) {
            // current thread does not own this lock: trying to unlock other's lock
            return -1;  // error
        } else {
            // expected situation: lock held by itself
            if (mutex->qBlocked->empty() == false) {
                // has waiting thread
                mutex->owner = mutex->qBlocked->front();
                mutex->qBlocked->pop_front();
                qReady.push_back(mutex->owner);
            } else {
                // has no waiting thread
                mutex->owner = NULL;
            }
            return 0;  // normal return
        }
    }
    // will never be here
    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    if (init == false) {
        // cerr << "- Must call thread_libinit() before thread_wait()." << endl;
        return -1;
    }

    interrupt_disable();

    // unlock the lock at first. This will disable interrupt and enable again.
    if (thread_unlock_with_interrupt_disabled(lock) != 0) {
        // cerr << "- FAILED to unlock lock # " << lock << " while waiting for Conditional Variable # " << cond << "." << endl;
        interrupt_enable();
        return -1;  // error
    } else {
        // lock was unlocked. wait cv
        auto iter = mCV.find(cond);
        if (iter == mCV.end()) {
            // cv doesn't exist yet. this shall be a new cv
            // allocate its waiting-thread queue
            deque<Thread*>* qthreadWaiting = NULL;
            try {
                qthreadWaiting = new deque<Thread*>;
            } catch (std::bad_alloc err) {
                delete qthreadWaiting;

                interrupt_enable();
                return -1;
            }
            qthreadWaiting->push_back(pthreadCurrent);
            mCV.insert(std::make_pair(cond, qthreadWaiting));

            // DO NOT RETURN AT THIS PLACE - I SPENT 5+ HOURS ON THIS BUG
        } else {
            // a previous waited cv being waited again
            iter->second->push_back(pthreadCurrent);
        }
        // swapcontext(pscheduler, pthreadCurrent->pucontext);
        swapcontext(pthreadCurrent->pucontext, pscheduler);

        interrupt_enable();

        // lock the lock at last
        if (thread_lock(lock) != 0) {
            // cerr << "- FAILED to lock lock # " << lock << " while waiting for Conditional Variable # " << cond << "." << endl;
            return -1;
        } else {
            return 0;
        }
    }
}

int thread_signal(unsigned int lock, unsigned int cond) {
    if (init == false) {
        // cerr << "- Must call thread_libinit() before thread_signal()." << endl;
        return -1;
    }

    interrupt_disable();

    auto iter = mCV.find(cond);
    if (iter == mCV.end()) {
        // cv not found
        // cerr << "- CV # " << cond << " is not hold by any thread." << endl;

        interrupt_enable();
        return 0;  // Not an error: "signaling without holding the lock (this is explicitly NOT an error in Mesa monitors)"
    } else {
        deque<Thread*>* qthreadWaiting = iter->second;
        if (qthreadWaiting->empty() == false) {
            // cv held by this thread
            Thread* pthread = qthreadWaiting->front();
            qthreadWaiting->pop_front();
            qReady.push_back(pthread);

            interrupt_enable();
            return 0;  // normal return
        } else {
            // signaling a cv with nobody waiting - this is normal behavior
            interrupt_enable();
            return 0;  // normal return
        }
    }

    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    if (init == false) {
        return -1;
    }

    interrupt_disable();

    auto iter = mCV.find(cond);
    if (iter == mCV.end()) {
        // cv not found
        interrupt_enable();
        return 0;  // Not an error: signaling without holding the lock (this is explicitly NOT an error in Mesa monitors)
    } else {
        // cv found
        deque<Thread*>* qthreadWaiting = iter->second;
        while (qthreadWaiting->empty() == false) {
            Thread* pthread = qthreadWaiting->front();
            qthreadWaiting->pop_front();
            qReady.push_back(pthread);
        }
    }

    interrupt_enable();
    return 0;
}