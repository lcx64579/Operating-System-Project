#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include "thread.h"

// Parameters for the whole program.
struct Parameters {
    int maxRequests = 0;  // max number of requests qRequest can hold
    int numThreads = 0;   // number of living requester
    std::string threadName[100];
};

// A disk request, include:
//  - which thread (by requesterID) sent this request
//  - which track the thread requested to access
struct Request {
    int requesterID;
    int track;
};

// globals
Parameters param;                                // SHALL NOT BE MODIFIED ANYWHERE - NO MUTEX VARIABLE FOR THIS TO AVOID POTENTIAL PROBLEMS
std::deque<Request*> qRequest;                   // request queue. To sort() on a queue, it must be a deque to use .begin() and .end() methods.
int nowtrack = 0;                                // current location of the disk tracker
unsigned int mutexQRequest;                      // mutex variable for access to qRequest
unsigned int cvQRequestFull, cvQRequestNotFull;  // condition variables for qRequest
bool* hasJobInQueue;                             // if hasJobInQueue[requesterID] no more requests, because:
                                                 // "Each request is synchronous; a requester thread must wait until the servicing thread finishes handling its last request before issuing its next request."

// #define ABS(x) ((x)>0 ? (x) : -(x))      // would rather use std::abs() in <algorithm>

bool requestSorter(Request* a, Request* b) {
    return std::abs(a->track - nowtrack) < std::abs(b->track - nowtrack);
}

// MUST run this function with mutex promise to qRequest
void sendRequest(Request* req) {
    using namespace std;
    // pack a request
    int requester = req->requesterID;
    int track = req->track;
    cout << "requester " << requester << " track " << track << endl;
    qRequest.push_back(req);
    // sort qRequest by absolute distance to nowtrack
    std::sort(qRequest.begin(), qRequest.end(), requestSorter);
    // indicate has a job in queue, no more requests can be sent
    hasJobInQueue[requester] = true;
}

// MUST run this function with mutex promise to qRequest
void serveRequest() {
    using namespace std;
    // fetch a request
    Request* req = qRequest.front();
    // serve the request
    int requester = req->requesterID;
    int track = req->track;
    cout << "service requester " << requester << " track " << track << endl;
    // mark as job removed, allow more requests from this file
    hasJobInQueue[requester] = false;
    // remove the request from the request queue
    qRequest.pop_front();
    delete req;
    nowtrack = track;
}

void threadRequester(void* argRequesterID) {
    // parse arg
    long requesterID = (long)argRequesterID;
    std::string filename = param.threadName[requesterID];
    std::cerr << "- thread of requesterID: " << requesterID << "  filename: " << filename << std::endl;

    // read file
    std::ifstream file;
    file.open(filename, std::ios::in);
    while (!file.eof()) {
        int requestTrack;
        file >> requestTrack;
        if (file.eof()) {
            break;
        }

        // pack up a new request
        Request* req = new Request;
        req->requesterID = requesterID;
        req->track = requestTrack;

        thread_lock(mutexQRequest);
        std::cerr << "- request " << requesterID << " lock mutex" << std::endl;

        while (qRequest.size() >= param.maxRequests || hasJobInQueue[requesterID] == true) {
            // queue full; or current file has a pending request
            std::cerr << "- request " << requesterID << " wait and unlock mutex" << std::endl;
            thread_wait(mutexQRequest, cvQRequestNotFull);
            std::cerr << "- request " << requesterID << " awake and lock mutex" << std::endl;
        }
        sendRequest(req);
        std::cerr << "- request " << requesterID << " signal" << std::endl;
        thread_broadcast(mutexQRequest, cvQRequestFull);  // inform server (may not be served though, because queue might not be full)
    }
    file.close();

    // this requester died, decline param.numThreads for qRequest
    while (hasJobInQueue[requesterID] == true) {
        thread_wait(mutexQRequest, cvQRequestNotFull);
    }
    param.numThreads--;  // THIS IS NOT SAFE
    if (param.numThreads < param.maxRequests) {
        // do this because:
        // "When fewer than max_disk_queue(param.maxRequests) requester threads are alive, the largest number of requests in the queue is equal to the number of living requester threads (param.numThreads)."
        param.maxRequests--;                              // THIS IS NOT SAFE
        thread_broadcast(mutexQRequest, cvQRequestFull);  // inform server that maybe another request can be served
    }

    std::cerr << "- request " << requesterID << " unlock mutex for rest jobs" << std::endl;
    thread_unlock(mutexQRequest);

    // exit this thread
    std::cerr << "- exiting request " << requesterID << std::endl;
}

// accept no arg
void threadServer(void* arg) {
    std::cerr << "- server lock mutex" << std::endl;
    thread_lock(mutexQRequest);  // lock it anyway, because thread_wait() is going to unlock it
    while (param.maxRequests > 0) {
        while (qRequest.size() < param.maxRequests) {
            // qRequest is not full yet (if no more new request, param.maxRequests decline itself so no need to worry)
            std::cerr << "- server wait and unlock mutex" << std::endl;
            thread_wait(mutexQRequest, cvQRequestFull);
            std::cerr << "- server awake and lock mutex" << std::endl;
        }
        if (qRequest.empty() == false) {
            // do this judgement because no service for situation if param.maxRequests is 0. this happened at last
            serveRequest();
        }
        std::cerr << "- server signal" << std::endl;
        thread_broadcast(mutexQRequest, cvQRequestNotFull);  // broadcast instead of signal because all threads shall wake up now
    }
    std::cerr << "- server unlock mutex" << std::endl;
    thread_unlock(mutexQRequest);

    // exit this thread
}

void threadMain(void* arg) {
    // uncomment this line to enable thread preemption
    // start_preemptions(false, true, 1);
    std::cerr << "- Max Requests: " << param.maxRequests << std::endl;

    // create requesters
    for (long i = 0; i < param.numThreads; i++) {  // using long because long -> void* shall match
        if (thread_create((thread_startfunc_t)threadRequester, (void*)i)) {
            std::cerr << "- thread_create FAILED for request " << i << std::endl;
            exit(1);
        }
    }

    // create a server
    if (thread_create((thread_startfunc_t)threadServer, NULL)) {
        std::cerr << "- thread_create FAILED for server." << std::endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    // for DEBUG. comment this to see all debug output
    freopen("/dev/null", "w", stderr);

    // arguments parser
    if (argc < 2) {  // illegal argument number
        std::cerr << "- Invalid Arguments." << std::endl;
        return 0;
    }
    param.maxRequests = std::atoi(argv[1]);
    // if arg 1 is invalid
    if (param.maxRequests < 1 || param.maxRequests > argc - 2) {
        std::cerr << "- Invalid Arguments." << std::endl;
        return 0;
    }
    // args of input filenames
    for (int i = 2; i < argc; i++) {
        param.threadName[i - 2] = argv[i];
    }

    // initialize globals
    nowtrack = 0;
    param.numThreads = argc - 2;
    cvQRequestNotFull = 0x00000003;  // these are identifier numbers
    cvQRequestFull = 0x00000002;
    mutexQRequest = 0x00000001;
    hasJobInQueue = new bool[param.numThreads];  // using new because don't know how many threads
    memset(hasJobInQueue, 0, sizeof(hasJobInQueue));

    // create main thread
    if (thread_libinit((thread_startfunc_t)threadMain, NULL)) {
        std::cout << "- thread_libinit in main() FAILED." << std::endl;
        exit(1);
    }

    return 0;
}
