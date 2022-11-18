#include <fstream>
#include <iostream>
#include <string>

// #define DEBUGGING

#ifdef DEBUGGING
#define DEBUG(x) std::cerr << ">>>>> " << #x << ": " << x << std::endl
#define DEBUG16(x) std::cerr << ">>>>> " << #x << ": 0x" << std::hex << x << std::endl
#else
#define DEBUG(x)
#define DEBUG16(x)
#endif
#define ERROR_RETURN return 0

#define OUTFILE_FILENAME "output-part1"
#define ADDR_LENGTH 8
#define ADDR_PAGE_OFFSET_BIT 7
#define MAX_FRAMES 8
#define MAX_PAGES 32

typedef unsigned long Address8;
typedef Address8 PageTable[MAX_PAGES];

PageTable pt = {2, 4, 1, 7, 3, 5, 6};

Address8 translateAddress(Address8 addrVirtual, int pageShift, PageTable pt) {
    Address8 offset = addrVirtual >> pageShift << pageShift ^ addrVirtual;
    Address8 addrPhysical = (pt[addrVirtual >> pageShift] << pageShift) + offset;
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