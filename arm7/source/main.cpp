#include <nds.h>

#include <nds/bios.h>
#include <nds/arm7/touch.h>

void BootGbaARM7(u32 useBottomScreen)
{
    u8 powcnt;
	
	if(useBottomScreen) {
		powcnt = 5;
	}
	else {
		powcnt = 9;
	}	
	
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz | SPI_CONTINUOUS;
	REG_SPIDATA = 0;
	SerialWaitBusy();
	
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
	REG_SPIDATA = powcnt;
	SerialWaitBusy();
	
	
	REG_IME = 0;
	REG_IE = 0;
	while(REG_VCOUNT != 200);  // Wait for VBlank
	// enter GBA mode
	asm volatile (
		"mov r2,#0x40\n"
		"swi 0x1f0000\n"
	:
	:
	: "r2"
	);
	
}

void VcountHandler() 
{
	inputGetAndSend();
}

volatile bool exitflag = false;

void powerButtonCB() 
{
	exitflag = true;
}

int main()
{
	readUserSettings();

	irqInit();
	fifoInit();
	SetYtrigger(80);
	installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT);   
	setPowerButtonCB(powerButtonCB);   

	while(!exitflag)
	{
		//swiWaitForVBlank();
		
		if(fifoCheckValue32(FIFO_USER_01)){
			u32 screen = fifoGetValue32(FIFO_USER_01);
			irqDisable(IRQ_ALL);
			BootGbaARM7(screen);
		}
		
		swiIntrWait(1,IRQ_FIFO_NOT_EMPTY);
	}
}


