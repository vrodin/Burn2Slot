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
	u8  busType;
	u8  adressSeqType;
	u16 flashid;
	u16 manufactorID;
	u32 size;
} CartInfo;

bool flashRepro_GBA();
void check();
