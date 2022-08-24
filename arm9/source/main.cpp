#include <nds.h>
#include <stdio.h>
#include <cstdarg>//va_list
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>
#include <string>
#include <unistd.h>

#include "app.h"


PrintConsole topScreen;
PrintConsole bottomScreen;

void initGraphics(){
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);
	consoleInit(&topScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
	consoleSelect(&bottomScreen);
}


void printTop(const char* str, ...){
	REG_IME	= 0;
	va_list args;
	va_start(args, str);
	consoleSelect(&topScreen);
	vprintf(str, args);
	consoleSelect(&bottomScreen);
	va_end(args);
	REG_IME = 1;
}


void BootGbaARM9(u32 useBottomScreen){	
	//start arm7 sequence
	fifoSendValue32(FIFO_USER_01, useBottomScreen);
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000, VRAM_C_SUB_BG_0x06200000, VRAM_D_LCD);
	REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_WRAP_OFF;
	REG_BG3PA = 1 << 8;
	REG_BG3PB = 0;
	REG_BG3PC = 0;
	REG_BG3PD = 1 << 8;
	REG_BG3VOFS = 0;
	REG_BG3HOFS = 0;
	memset((void*)BG_BMP_RAM(0), 0, 0x18000);
	memset((void*)BG_BMP_RAM(8), 0, 0x18000);
    REG_POWERCNT =  3;
	REG_EXMEMCNT |= 0x8080;     // ARM7 has access to GBA cart
	REG_EXMEMCNT &= ~0x4000;    // set Async Main Memory mode
	swiWaitForIRQ();
}



int main(){
	initGraphics();
	check();
	keysSetRepeat(25,5);
	sysSetBusOwners(BUS_OWNER_ARM9,BUS_OWNER_ARM9);
	while(1) {
		if(flashRepro_GBA())BootGbaARM9(1);	
	}
	return 0;
}
