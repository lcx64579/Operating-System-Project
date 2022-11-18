#include <iostream>
#include "thread.h"
using namespace std;

#define RAISE(str) \
    cout << "ERROR: " << str << endl

typedef unsigned int Mutex;    // mutex lock
typedef unsigned int CondVar;  // conditional variable

int num = 1;
Mutex lockNum = 0x1;
Mutex lock1 = 0x3;  // will be locked by worker1
Mutex lock2 = 0x2;  // never used
CondVar cvNum = 0x1;
CondVar cv2 = 0x2;  // never used

/////////////
// THREADS //
/////////////

void threadWorker1(void* arg) {
    thread_lock(lockNum);
    thread_lock(lock1);
    if (thread_lock(lockNum) == 0)
        RAISE("Lock being locked again by the same thread.");
    if (thread_unlock(lock2) == 0)
        RAISE("Unlocked a never-locked lock.");
    if (thread_wait(lock2, cv2) == 0)
        RAISE("Waited a never-locked lock.");
    if (thread_signal(lockNum, cvNum) != 0)
        RAISE("Signal without wait failed. This should succeed.");
    if (thread_broadcast(lockNum, cvNum) != 0)
        RAISE("Broadcast without wait failed. This should succeed.");
    while (num != 1) {
        thread_wait(lockNum, cvNum);
    }
    num = 2;
    cout << "Worker1: " << num << endl;
    thread_broadcast(lockNum, cvNum);
    thread_unlock(lockNum);
    thread_yield();

    thread_lock(lockNum);
    while (num != 1) {
        thread_wait(lockNum, cvNum);
    }
    num = 2;
    cout << "Worker1: " << num << endl;
    thread_broadcast(lockNum, cvNum);
    thread_unlock(lockNum);
    thread_yield();
}

void threadWorker2(void* args) {
    thread_lock(lockNum);
    while (num != 2) {
        thread_wait(lockNum, cvNum);
    }
    num = 1;
    cout << "Worker2: " << num << endl;
    thread_broadcast(lockNum, cvNum);
    thread_unlock(lockNum);
    thread_yield();

    thread_lock(lockNum);
    while (num != 2) {
        thread_wait(lockNum, cvNum);
    }
    if (thread_unlock(lock1) == 0)
        RAISE("Unlocked a lock held by another thread.");
    if (thread_wait(lock1, cv2) == 0)
        RAISE("Waited a lock held by another thread.");
    num = 1;
    cout << "Worker2: " << num << endl;
    thread_broadcast(lockNum, cvNum);
    thread_unlock(lockNum);
    thread_yield();
}

// A duplicated initializing thread. Run this and receive an error. Supposes never to be called.
void threadMainAgain(void* arg) {
    RAISE("Multiple thread_init() execution.");
}

// Initialized main thread. DOES NOT ACCEPT ANY ARGUMENTS.
void threadMain(void* arg) {
    thread_create((thread_startfunc_t)threadWorker1, (void*)NULL);
    thread_create((thread_startfunc_t)threadWorker2, (void*)NULL);

    int result = thread_libinit((thread_startfunc_t)threadMainAgain, (void*)NULL);
    if (result == 0)
        RAISE("Multiple thread_init() call succeed - which should not happen.");
}

/**
 * @brief Test cases for thread lib `thread.h`.
 *
 * @return int
 */
int main() {
    // thread lib exploit without initialization
    if (thread_lock(lockNum) == 0)
        RAISE("Function thread_lock() exploit without initialization.");
    if (thread_unlock(lockNum) == 0)
        RAISE("Function thread_lock() exploit without initialization.");
    if (thread_wait(lockNum, cvNum) == 0)
        RAISE("Function thread_lock() exploit without initialization.");
    if (thread_signal(lockNum, cvNum) == 0)
        RAISE("Function thread_lock() exploit without initialization.");
    if (thread_broadcast(lockNum, cvNum) == 0)
        RAISE("Function thread_lock() exploit without initialization.");
    if (thread_yield() == 0)
        RAISE("Function thread_lock() exploit without initialization.");

    thread_libinit((thread_startfunc_t)threadMain, (void*)NULL);

    return 0;
}