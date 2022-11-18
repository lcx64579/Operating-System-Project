#include <fstream>
#include <iostream>
#include <string>

#include "mm2types.h"
#include "pagetable.h"

#define OUTFILE_FILENAME "output-part2"

int ADDR_LENGTH = 8;
int ADDR_PAGE_OFFSET_BIT = 7;
int MAX_FRAMES = 8;
int MAX_PAGES = 32;

Address8 translateAddress(Address8 addrVirtual, int pageShift, PageTable* pt) {
    Address8 page = addrVirtual >> pageShift;
    Address8 offset = page << pageShift ^ addrVirtual;
    Address8 frame = pt->map(page);
    Address8 addrPhysical = frame << pageShift | offset;
    return addrPhysical;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        ERROR_RETURN;
    }

    std::string filename = argv[1];
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