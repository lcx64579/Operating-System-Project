#include <fstream>
#include <iostream>
#include <string>

#include "mm2types.h"
#include "pagetable.h"

#define OUTFILE_FILENAME "output-part3"

int ADDR_LENGTH = 8;
int ADDR_PAGE_OFFSET_BIT = 7;
int MAX_FRAMES = 8;
int MAX_PAGES = 32;

int fastLog(int x) {
    int counter = 0;
    for (; x >> 1 != 0; x >>= 1, counter++)
        ;
    return counter;
}

Address8 translateAddress(Address8 addrVirtual, int pageShift, PageTable* pt) {
    Address8 page = addrVirtual >> pageShift;
    Address8 offset = page << pageShift ^ addrVirtual;
    Address8 frame = pt->map(page);
    Address8 addrPhysical = frame << pageShift | offset;
    return addrPhysical;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        ERROR_RETURN;
    }

    int sizeOfPage = std::atoi(argv[1]);
    ADDR_PAGE_OFFSET_BIT = fastLog(sizeOfPage);
    DEBUG(ADDR_PAGE_OFFSET_BIT);
    int sizeOfVirtualMemory = std::atoi(argv[2]);
    MAX_PAGES = sizeOfVirtualMemory / sizeOfPage;
    int sizeOfPhysicalMemory = std::atoi(argv[3]);
    MAX_FRAMES = sizeOfPhysicalMemory / sizeOfPage;
    std::string filename = argv[4];
    std::ifstream filein;
    std::ofstream fileout;

    filein.open(filename, std::ios::binary | std::ios::in);
    fileout.open(OUTFILE_FILENAME, std::ios::binary | std::ios::out);

    if (!filein || !fileout) {
        ERROR_RETURN;
    }

    PageTable* pt = new PageTable();
    PhyFrames* ft = new PhyFrames();
    pt->alignPhyFrames(ft);
    ft->alignPageTable(pt);
    while (true) {
        Address8 addrVirtual;
        filein.read((char*)&addrVirtual, ADDR_LENGTH);
        if (filein.eof())
            break;
        DEBUG16(addrVirtual);
        Address8 addrPhysical = translateAddress(addrVirtual, ADDR_PAGE_OFFSET_BIT, pt);
        DEBUG16(addrPhysical);
        fileout.write((char*)&addrPhysical, ADDR_LENGTH);
    }
    filein.close();
    fileout.close();

    return 0;
}