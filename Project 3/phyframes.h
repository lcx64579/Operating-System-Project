#ifndef phyframes_h_
#define phyframes_h_

#include "mm2types.h"
#include "pagetable.h"

struct ReverseMappingTableEntry {
    Address8 page;
    int counter;  // Counter for LRU page swapping algorithm usage.
};

class PageTable;  // to resolve circuit dependency

/**
 * NOTICE: Must align a PageTable before using.
 */
class PhyFrames {
   private:
    ReverseMappingTableEntry _ft[ASSUMED_MAX_FRAMES];
    PageTable* _pt;
    int _freeFramePointer;
    int _globalTimer;

   public:
    PhyFrames();
    int alignPageTable(PageTable* pt);
    Address8 reverse(Address8 frameNumber);
    bool hasFreeFrameSpace();
    Address8 allocateKnownFreeFrame(Address8 reversePage);
    Address8 leastRecentlyUsedFrame();
    int swap(Address8 swappedFrameNumber, Address8 reversePage);
    int accessFrame(Address8 frameNumber);
};

#endif
