#include <nds.h>
#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <unistd.h>
#include "fat.h"
#include "file_browse.h"

#define WAIT_NON_FFFF [](u16 a){return a!=0xFFFF;}

extern void printTop(const char* str, ...);

typedef struct{
	u8 flashType;
	u8 busType;
	u8 AddrType;
	u8 DataType;
	u16 FID;
	u16 MID;
	u32 size;
} CartI;

bool flashRepro_GBA();
void check();
