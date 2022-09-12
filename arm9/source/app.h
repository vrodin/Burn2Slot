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

extern void printTop(const char* str, ...);

typedef struct{
	union {
		u8 busType;
		u8 AMDType;
	};
	union {
		u8 AMDAddr;
		u8 AddrType;
	};	
	union {
		u8 flashType;
		u8 AMDData;
	};	
	u16 FID;
	u16 MID;
	u32 size;	
} CartI;

bool flashRepro_GBA();
void check();
