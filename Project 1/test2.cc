#include <iostream>
#include "thread.h"
using namespace std;

#define RAISE(str) \
    cout << "ERROR: " << str << endl

typedef unsigned int Mutex;    // mutex lock
typedef unsigned int CondVar;  // conditional variable

int num = 1;
int count = 0;
int maxcount = 8;
Mutex lockCount = 0x1;
Mutex lock2 = 0x2;
Mutex lock3 = 0x3;
CondVar cv1 = 0x1;
CondVar cv3 = 0x3;

//////////
// TOOL //
//////////

void check() {
    if (thread_lock(lockCount) == 0)
        RAISE("Lock being locked again by the same thread.");
    if (thread_unlock(lock2) == 0)
        RAISE("Unlocking other's lock.");
    if (thread_unlock(lock3) == 0)
        RAISE("Unlocking other's lock.");
    if (thread_wait(lock3, cv3) == 0)
        RAISE("Waiting a never-locked lock.");
    if (thread_wait(lock2, cv3) == 0)
        RAISE("Waiting other's lock.");
    if (thread_wait(lock3, cv1) == 0)
        RAISE("Waiting a never-locked lock.");
    if (thread_wait(lock2, cv1) == 0)
        RAISE("Waiting other's lock.");
    if (thread_signal(lockCount, cv3) != 0)
        RAISE("Signal without wait failed. This should succeed.");
    if (thread_broadcast(lockCount, cv3) != 0)
        RAISE("Broadcast without wait failed. This should succeed.");
    if (thread_signal(lock3, cv1) != 0)
        RAISE("Signal without wait failed. This should succeed.");
    if (thread_broadcast(lock3, cv1) != 0)
        RAISE("Broadcast without wait failed. This should succeed.");
    if (thread_signal(lock2, cv1) != 0)
        RAISE("Signal without wait failed. This should succeed.");
    if (thread_broadcast(lock2, cv1) != 0)
        RAISE("Broadcast without wait failed. This should succeed.");
}

void output() {
    cout << "num: " << num << " count: " << count << endl;
}

/////////////
// THREADS //
/////////////

void threadWorker1(void* arg) {
    while (count < maxcount) {
        thread_lock(lockCount);
        check();
        while (num != 1)
            thread_wait(lockCount, cv1);
        count++;
        output();
        num = 2;
        thread_broadcast(lockCount, cv1);
        thread_unlock(lockCount);
        thread_yield();
    }
}

void threadWorker2(void* arg) {
    while (count < maxcount) {
        thread_lock(lockCount);
        while (num != 2) {
            check();
            thread_wait(lockCount, cv1);
        }
        count++;
        output();
        num = 3;
        thread_broadcast(lockCount, cv1);
        thread_unlock(lockCount);
        thread_yield();
    }
}

void threadWorker3(void* arg) {
    while (count < maxcount) {
        thread_lock(lockCount);
        while (num != 3)
            thread_wait(lockCount, cv1);
        check();
        count++;
        output();
        num = 1;
        thread_broadcast(lockCount, cv1);
        thread_unlock(lockCount);
        thread_yield();
    }
}

void threadWorker4(void* a) {
    thread_lock(lock2);
    thread_lock(lockCount);

    while (count < maxcount - 1) {
        thread_wait(lockCount, cv1);
    }
    output();
    thread_broadcast(lockCount, cv1);
    thread_unlock(lockCount);
    thread_unlock(lock2);
    thread_yield();
}

void master(void* arg) {
    //    start_preemptions(false,true,1);
    thread_create((thread_startfunc_t)threadWorker1, (void*)NULL);
    thread_create((thread_startfunc_t)threadWorker2, (void*)NULL);
    thread_create((thread_startfunc_t)threadWorker3, (void*)NULL);
    thread_create((thread_startfunc_t)threadWorker4, (void*)NULL);
}

int main() {
    thread_libinit((thread_startfunc_t)master, (void*)10);
}