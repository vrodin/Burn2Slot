#include <nds.h>
#include <string.h>

static void menuValue32Handler(u32 value,void* data)
{
  switch(value)
  {
    case 1:
      {
        u32 ii=0;
        if(PersonalData->gbaScreen)
          ii=(1*PM_BACKLIGHT_BOTTOM)|PM_SOUND_AMP;
        else
          ii=(1*PM_BACKLIGHT_TOP)|PM_SOUND_AMP;
        writePowerManagement(PM_CONTROL_REG,ii);
        swiChangeSoundBias(0,0x400);
        swiSwitchToGBAMode();
      }
      break;
    default:
      break;
  }
}

int main()
{
  // read User Settings from firmware
  readUserSettings();

  irqInit();
  fifoInit();

  // Start the RTC tracking IRQ
  initClockIRQ();

  fifoSetValue32Handler(FIFO_USER_08,menuValue32Handler,0);

  installSystemFIFO();

  irqSet(IRQ_VBLANK,inputGetAndSend);

  irqEnable(IRQ_VBLANK);

  for(;;)swiWaitForVBlank();
  
  return 0;
}
