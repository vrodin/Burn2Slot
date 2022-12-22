#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include "app.h"
#include "util.h"

int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	extern u64 *fake_heap_end;
	*fake_heap_end = 0;

	initGraphics();
	if (!fatInitDefault()) {
		iprintf ("fatinitDefault failed!\n");
	}

	keysSetRepeat(25,5);
	sysSetBusOwners(BUS_OWNER_ARM9,BUS_OWNER_ARM9);
	while(1) {
		flashRepro_GBA();	
        		//BootGbaARM9(1);	
	}

	return 0;
}
