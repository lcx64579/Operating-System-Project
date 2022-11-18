#include "phyframes.h"

PhyFrames::PhyFrames() {
    _freeFramePointer = 1;  // frame 0 is for kernel
    _globalTimer = 1;
    for (int i = 0; i < MAX_FRAMES; i++) {
        _ft[i].page = 0;
        _ft[i].counter = 0;
    }
}

int PhyFrames::alignPageTable(PageTable* pt) {
    if (!pt) {
        return -1;
    }
    _pt = pt;

    return 0;
}

Address8 PhyFrames::reverse(Address8 frameNumber) {
    return _ft[frameNumber].page;
}

bool PhyFrames::hasFreeFrameSpace() {
    return _freeFramePointer < MAX_FRAMES;
}

Address8 PhyFrames::allocateKnownFreeFrame(Address8 reversePage) {
    ReverseMappingTableEntry fte;
    fte.page = reversePage;
    fte.counter = _globalTimer;
    int frame = _freeFramePointer;
    _ft[frame] = fte;
    _freeFramePointer++;

    return frame;
}

Address8 PhyFrames::leastRecentlyUsedFrame() {
    int lruFrame = 1, lruCounter = _ft[1].counter;  // frame 0 is for kernel
    for (int i = 2; i < MAX_FRAMES; i++) {
        if (_ft[i].counter < lruCounter) {
            lruCounter = _ft[i].counter;
            lruFrame = i;
        }
    }

    return lruFrame;
}

int PhyFrames::swap(Address8 swappedFrameNumber, Address8 reversePage) {
    _ft[swappedFrameNumber].page = reversePage;
    _ft[swappedFrameNumber].counter = _globalTimer;

    return 0;
}

int PhyFrames::accessFrame(Address8 frameNumber) {
    _ft[frameNumber].counter = _globalTimer;
    _globalTimer++;

    return 0;
}