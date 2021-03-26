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
