#ifndef _APP_H_
#define _APP_H_


#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <unistd.h>
#include "nds.h"
#include "fat.h"
#include "file_browse.h"

#define WAIT_STATUS_READY [](u16 a){return (a | 0xFF7F) != 0xFFFF;}
#define WAIT_NON_FFFF [](u16 a){return a!=0xFFFF;}
#define WAIT_FOR_FFFF [](u16 a){return a==0xFFFF;}
#define WAIT_NON_0 [](u16 a){return a!=0;}
#define WAIT_FOR_0 [](u16 a){return a==0;}

const u32 TIMEOUT_5S = 0xA00000;
const u32 TIMEOUT_8S = 0xF00000;

typedef struct{
	u8  busType;
	union {
		u8  intelType;
		u8  adressSeqType;
	};
	
	u16 flashid;
	u16 manufactorID;
	u32 size;
} CartInfo;

bool flashRepro_GBA();

#endif//_APP_H_
