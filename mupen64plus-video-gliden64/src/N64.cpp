#include "N64.h"

u8 *HEADER;
u8 *DMEM;
u8 *IMEM;
u64 TMEM[512];
u8 *RDRAM;

u32 RDRAMSize;

bool ConfigOpen = false;
