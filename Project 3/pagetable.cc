#include "pagetable.h"

PageTable::PageTable() {
    for (int i = 0; i < MAX_PAGES; i++) {
        _pt[i].frame = 0;
        _pt[i].valid = false;
    }
}

int PageTable::alignPhyFrames(PhyFrames* ft) {
    if (!ft) {
        return -1;
    }
    _ft = ft;

    return 0;
}

Address8 PageTable::map(Address8 virtualPageNumber) {
    if (_pt[virtualPageNumber].valid == false) {
        if (_ft->hasFreeFrameSpace()) {
            DEBUG("FREE SPACE");
            Address8 frameNumber = _ft->allocateKnownFreeFrame(virtualPageNumber);
            _pt[virtualPageNumber].frame = frameNumber;
            _pt[virtualPageNumber].valid = true;
            DEBUG(frameNumber);
            _ft->accessFrame(frameNumber);
        } else {
            DEBUG("SWAP");
            Address8 frameNumber = _ft->leastRecentlyUsedFrame();
            if (frameNumber == -1) {
                DEBUG("ERROR: FAILED TO LOCATE LRU FRAME");
            }

            Address8 oldPageNumber = _ft->reverse(frameNumber);
            _pt[oldPageNumber].valid = false;
            _ft->swap(frameNumber, virtualPageNumber);
            _pt[virtualPageNumber].frame = frameNumber;
            _pt[virtualPageNumber].valid = true;
            DEBUG(frameNumber);
            _ft->accessFrame(frameNumber);
        }
    } else {
        DEBUG("VALID");
        DEBUG(_pt[virtualPageNumber].frame);
        _ft->accessFrame(_pt[virtualPageNumber].frame);
    }

    return _pt[virtualPageNumber].frame;
}