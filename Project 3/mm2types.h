#ifndef mm2types_h_
#define mm2types_h_

#include <iostream>

#define DEBUGGING

#ifdef DEBUGGING
#define DEBUG(x) std::cerr << ">>>>> " << #x << ": " << x << std::endl
#define DEBUG16(x) std::cerr << ">>>>> " << #x << ": 0x" << std::hex << x << std::endl
#else
#define DEBUG(x)
#define DEBUG16(x)
#endif
#define ERROR_RETURN return 0

extern int ADDR_LENGTH;
extern int ADDR_PAGE_OFFSET_BIT;
extern int MAX_FRAMES;
extern int MAX_PAGES;

#define ASSUMED_MAX_PAGES  2 << 5
#define ASSUMED_MAX_FRAMES  2 << 5

typedef unsigned long Address8;

#endif
