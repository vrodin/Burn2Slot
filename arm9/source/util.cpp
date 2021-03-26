#include "util.h"

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

void BootGbaARM9(u32 useBottomScreen)//, cBMP15& border)
{	
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
	
	//if(border.valid()){
	//	memcpy((u16*)BG_BMP_RAM(0), border.buffer(), SCREEN_WIDTH*SCREEN_HEIGHT*2);
	//	memcpy((u16*)BG_BMP_RAM(8), border.buffer(), SCREEN_WIDTH*SCREEN_HEIGHT*2);
	//}else{
		memset((void*)BG_BMP_RAM(0), 0, 0x18000);
		memset((void*)BG_BMP_RAM(8), 0, 0x18000);
	//}
	
	//POWCNT = 8003h / 0003h
	if(useBottomScreen) {
        REG_POWERCNT =  3;
    }else {
        REG_POWERCNT = 0x8003;
    }
	
	REG_EXMEMCNT |= 0x8080;     // ARM7 has access to GBA cart
	REG_EXMEMCNT &= ~0x4000;    // set Async Main Memory mode
	
	// halt indefinitly, since irqs are disabled
	REG_IME = 0;
	swiWaitForIRQ();
	
}


