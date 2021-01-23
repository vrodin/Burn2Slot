#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include "app.h"
void GBAMode()
{
  sysSetBusOwners(BUS_OWNER_ARM7,BUS_OWNER_ARM7);
  if(PersonalData->gbaScreen)
    REG_POWERCNT=1;
  else
    REG_POWERCNT=(POWER_SWAP_LCDS|1)&0xffff;
  fifoSendValue32(FIFO_USER_08,1);
  while(true) swiWaitForVBlank();	
    
}

int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	extern u64 *fake_heap_end;
	*fake_heap_end = 0;

	// Subscreen as a console
	videoSetModeSub(MODE_0_2D);
	vramSetBankH(VRAM_H_SUB_BG);
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
	if (!fatInitDefault()) {
		iprintf ("fatinitDefault failed!\n");
	}

	keysSetRepeat(25,5);
	sysSetBusOwners(BUS_OWNER_ARM9,BUS_OWNER_ARM9);
	while(1) {
		if(flashRepro_GBA())	
        		GBAMode();	
	}

	return 0;
}
