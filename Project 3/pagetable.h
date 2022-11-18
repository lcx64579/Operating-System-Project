#ifndef pagetable_h_
#define pagetable_h_

#include "mm2types.h"
#include "phyframes.h"

struct PTE {
    Address8 frame;  // 0 for kernel
    bool valid;      // true if in memory
};

class PhyFrames;  // to resolve circuit dependency

/**
 * NOTICE: Must align a PhyFrames before using.
 */
class PageTable {
   private:
    PTE _pt[ASSUMED_MAX_PAGES];
    PhyFrames* _ft;

   public:
    PageTable();
    int alignPhyFrames(PhyFrames* ft);
    Address8 map(Address8 pageNumber);
};

#endif
