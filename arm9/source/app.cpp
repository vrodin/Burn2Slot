#include "app.h"

u16 flashid = 0;
u16 cartSize = 0;
u16 busType = 0;
u16 romType = 0;
unsigned long fileSize = 0;
FILE* fd;
u8* sdBuffer = (u8*)malloc(sizeof(u8) * 512);

u16 swapBits(u16 n) {
	u16 p1 = 0;
	u16 p2 = 1;
	u16 bit1 = (n >> p1) & 1;
	u16 bit2 = (n >> p2) & 1;
	u16 x = (bit1 ^ bit2);
	x = (x << p1) | (x << p2);
	u16 result = n ^ x;
	return result;
}

u16 read_word_rom(unsigned long address) {
	return GBAROM[address];
}
u16 read_swapped_word_rom(unsigned long address) {
	return swapBits(read_word_rom(address));
}

void write_word_rom(unsigned long address, u16 byte) {
	GBA_BUS[address] = byte;
	swiDelay(10);
}
void write_swapped_word_rom(unsigned long address, u16 byte) {
	write_word_rom(address, swapBits(byte));
}

void ( *( write_word() ) )(unsigned long address, u16 byte) { 
	switch(busType) {
		case 0: return write_word_rom; 
		case 1: return write_swapped_word_rom; 
	}
	
} 

u16 ( *( read_word() ) )(unsigned long address) { 
	switch(busType) {
		case 0: return read_word_rom; 
		case 1: return read_swapped_word_rom; 
	}
	
} 

void resetMX29GL128E_GBA() {
	write_word()(0, 0xF0);
}

void idFlashrom_GBA() {
	write_word_rom(0, 0x90);
	flashid = read_word_rom(0x2) & 0xFF00 || read_word_rom(0x4) & 0xFF;
	if (flashid == 0x8802 || flashid == 8816) {
		iprintf("flashId: %04X \n");
		cartSize = 0x2000000;
		return;
	}
	
	cartSize = 0x1000000;	
	write_word_rom(0x555, 0xAA);
    	write_word_rom(0x2AA, 0x55);
    	write_word_rom(0x555, 0x90);
	flashid = read_word_rom(0x01);
	romType = read_word_rom(0x0);
	if (flashid == 0x227E) {
		resetMX29GL128E_GBA();
		return;
	}
	
	write_swapped_word_rom(0xAAA, 0xAA);
    	write_swapped_word_rom(0x555, 0x55);
    	write_swapped_word_rom(0xAAA, 0x90);
    	flashid = read_swapped_word_rom(0x01);
    	romType = read_swapped_word_rom(0x0);
	if (flashid == 0x227E) {
		busType = 1;
		resetMX29GL128E_GBA();
		return;
	}
	flashid = 0;
}

void resetIntel_GBA(unsigned long partitionSize) {
	for (unsigned long currPartition = 0; currPartition < cartSize; currPartition += partitionSize) {
		write_word()(currPartition, 0xFFFF);
	}
}

void eraseMX29GL128E_GBA() {

	write_word()(0x555, 0xAA);
	write_word()(0x2AA, 0x55);
	write_word()(0x555, 0x80);
	write_word()(0x555, 0xAA);
	write_word()(0x2AA, 0x55);
	write_word()(0x555, 0x10);

	// Read the status register
	u16 statusReg = 0;
	while ((statusReg | 0xFF7F) != 0xFFFF) {
		statusReg = read_word()(0);
		swiDelay(10);
	}
}

void eraseIntel4000_GBA() {
	// If the game is smaller than 16Mbit only erase the needed blocks
	unsigned long lastBlock = 0xFFFFFF;
	if (fileSize < 0xFFFFFF)
		lastBlock = fileSize;

	// Erase 4 blocks with 16kwords each
	for (unsigned long currBlock = 0x0; currBlock < 0x1FFFF; currBlock += 0x8000) {
		// Unlock Block
		write_word()(currBlock, 0x60);
		write_word()(currBlock, 0xD0);

		// Erase Command
		write_word()(currBlock, 0x20);
		write_word()(currBlock, 0xD0);

		// Read the status register
		u16 statusReg = 0;
		while ((statusReg | 0xFF7F) != 0xFFFF) {
			statusReg = read_word()(currBlock);
			swiDelay(10);
		}
	}

	// Erase 126 blocks with 64kwords each
	for (unsigned long currBlock = 0x20000; currBlock < lastBlock; currBlock += 0x1FFFF) {
		// Unlock Block
		write_word()(currBlock, 0x60);
		write_word()(currBlock, 0xD0);

		// Erase Command
		write_word()(currBlock, 0x20);
		write_word()(currBlock, 0xD0);

		// Read the status register
		u16 statusReg = 0;
		while ((statusReg | 0xFF7F) != 0xFFFF) {
			statusReg = read_word()(currBlock);
			swiDelay(10);
		}
	}

	// Erase the second chip
	if (fileSize > 0xFFFFFF) {
		// 126 blocks with 64kwords each
		for (unsigned long currBlock = 0x1000000; currBlock < 0x1FDFFFF; currBlock += 0x1FFFF) {
			// Unlock Block
			write_word()(currBlock, 0x60);
			write_word()(currBlock, 0xD0);

			// Erase Command
			write_word()(currBlock, 0x20);
			write_word()(currBlock, 0xD0);

			// Read the status register
			u16 statusReg = 0;
			while ((statusReg | 0xFF7F) != 0xFFFF) {
				statusReg = read_word()(currBlock);
				swiDelay(10);
			}
		}

		// 4 blocks with 16ku16 each
		for (unsigned long currBlock = 0x1FE0000; currBlock < 0x1FFFFFF; currBlock += 0x8000) {
			// Unlock Block
			write_word()(currBlock, 0x60);
			write_word()(currBlock, 0xD0);

			// Erase Command
			write_word()(currBlock, 0x20);
			write_word()(currBlock, 0xD0);

			// Read the status register
			u16 statusReg = 0;
			while ((statusReg | 0xFF7F) != 0xFFFF) {
				statusReg = read_word()(currBlock);
				swiDelay(10);
			}
		}
	}
}

void eraseIntel4400_GBA() {
	// If the game is smaller than 32Mbit only erase the needed blocks
	unsigned long lastBlock = 0x1FFFFFF;
	if (fileSize < 0x1FFFFFF)
		lastBlock = fileSize;

	// Erase 4 blocks with 16kwords each
	for (unsigned long currBlock = 0x0; currBlock < 0x1FFFF; currBlock += 0x8000) {
		// Unlock Block
		write_word()(currBlock, 0x60);
		write_word()(currBlock, 0xD0);

		// Erase Command
		write_word()(currBlock, 0x20);
		write_word()(currBlock, 0xD0);

		// Read the status register
		u16 statusReg = 0;
		while ((statusReg | 0xFF7F) != 0xFFFF) {
			statusReg = read_word()(currBlock);
			swiDelay(10);
		}
	}

	// Erase 255 blocks with 64kwords each
	for (unsigned long currBlock = 0x20000; currBlock < lastBlock; currBlock += 0x1FFFF) {
		// Unlock Block
		write_word()(currBlock, 0x60);
		write_word()(currBlock, 0xD0);

		// Erase Command
		write_word()(currBlock, 0x20);
		write_word()(currBlock, 0xD0);

		// Read the status register
		u16 statusReg = 0;
		while ((statusReg | 0xFF7F) != 0xFFFF) {
			statusReg = read_word()(currBlock);
			swiDelay(10);
		}
	}
}

void writeIntel4000_GBA() {
	for (unsigned long currBlock = 0; currBlock < fileSize; currBlock += 0x20000) {
		// Write to flashrom
		for (unsigned long currSdBuffer = 0; currSdBuffer < 0x20000; currSdBuffer += 512) {
			// Fill SD buffer
			fread(sdBuffer, 1, 512, fd);

			// Write 32 words at a time
			for (int currWriteBuffer = 0; currWriteBuffer < 512; currWriteBuffer += 64) {
				// Unlock Block
				write_word()(currBlock + currSdBuffer + currWriteBuffer, 0x60);
				write_word()(currBlock + currSdBuffer + currWriteBuffer, 0xD0);

				// Buffered program command
				write_word()(currBlock + currSdBuffer + currWriteBuffer, 0xE8);

				// Check Status register
				u16 statusReg = read_word()(currBlock + currSdBuffer + currWriteBuffer);
				while ((statusReg | 0xFF7F) != 0xFFFF) {
					statusReg = read_word()(currBlock + currSdBuffer + currWriteBuffer);
					swiDelay(10);
				}

				// Write u16 count (minus 1)
				write_word()(currBlock + currSdBuffer + currWriteBuffer, 0x1F);

				// Write buffer
				for (byte currByte = 0; currByte < 64; currByte += 2) {
					// Join two bytes into one word
					u16 currWord = ( ( sdBuffer[currWriteBuffer + currByte + 1] & 0xFF ) << 8 ) | ( sdBuffer[currWriteBuffer + currByte] & 0xFF );
					write_word()(currBlock + currSdBuffer + currWriteBuffer + currByte, currWord);
				}

				// Write Buffer to Flash
				write_word()(currBlock + currSdBuffer + currWriteBuffer + 62, 0xD0);

				// Read the status register at last written address
				statusReg = read_word()(currBlock + currSdBuffer + currWriteBuffer + 62);
				while ((statusReg | 0xFF7F) != 0xFFFF) {
					statusReg = read_word()(currBlock + currSdBuffer + currWriteBuffer + 62);
					swiDelay(10);
				}
			}
		}
	}
}

void writeMX29GL128E_GBA() {
	u16 currWord;
	unsigned long address;
	for (unsigned long currSector = 0; currSector < fileSize; currSector += 0x20000) {
		// Write to flashrom
		for (unsigned long currSdBuffer = 0; currSdBuffer < 0x20000; currSdBuffer += 512) {
		// Fill SD buffer
			fread(sdBuffer, 1, 512, fd);

			// Write 32 words at a time
			for (int currWriteBuffer = 0; currWriteBuffer < 512; currWriteBuffer += 2) {
				// Write Buffer command
				write_word()(0x555, 0xAA);
				write_word()(0x2AA, 0x55);
				write_word()(0x555, 0xA0);

				currWord = ( ( sdBuffer[currWriteBuffer + 1] & 0xFF ) << 8 ) | ( sdBuffer[currWriteBuffer] & 0xFF );
				address = (currSector + currSdBuffer + currWriteBuffer) / 2;
				write_word()(address, currWord);

				// Read the status register
				u16 statusReg = read_word()(address);

				while ((statusReg | 0xFF7F) != (currWord | 0xFF7F)) {
					statusReg = read_word()(address);
					swiDelay(10);
				}
			}
		}
	}
}

bool flashRepro_GBA() {	
	// Launch file browser
	std::vector<std::string> extensionList = {".gba"};
	std::string filename = browseForFile(extensionList);
	fd = fopen(&filename[0], "rb");
	// Check flashrom ID's
	idFlashrom_GBA();

	if (flashid == 0x8802 || flashid == 0x8816 || flashid == 0x227E) {
		iprintf("ID: %04X Size: %DMB\n", flashid, cartSize / 0x100000);
		// MX29GL128E or MSP55LV128(N)
		if (flashid == 0x227E) {
			// MX is 0xC2 and MSP55LV128 is 0x4 and MSP55LV128N 0x1
			if (romType == 0xC2) {
				iprintf("Macronix MX29GL128E\n");
			}
			else if (romType == 0x1 || romType == 0x4) {
				iprintf("Fujitsu MSP55LV128N\n");
			}
			else if (romType == 0x89) {
				iprintf("Intel PC28F256M29\n");
			}
			else if (romType == 0x20) {
				iprintf("ST M29W128GH\n");
			}
			else {
				iprintf("Unknown manufacturer\n");
			}
		}
		// Intel 4000L0YBQ0
		else if (flashid == 0x8802) {
			iprintf("Intel 4000L0YBQ0\n");
		}
		// Intel 4400L0ZDQ0
		else if (flashid == 0x8816) {
			iprintf("Intel 4400L0ZDQ0\n");
		}
	
		int prev = ftell(fd);
		fseek(fd, 0L, SEEK_END);
		fileSize = ftell(fd);
		fseek(fd,prev,SEEK_SET); 
    
		// Open file on sd card
		if (fileSize) {
			// Get rom size from file
			iprintf("File size: %DMB\nErasing...\n", fileSize / 0x100000);
      
			// Erase needed sectors
			if (flashid == 0x8802) {
				eraseIntel4000_GBA();
				resetIntel_GBA(0x200000);
			}
			else if (flashid == 0x8816) {
				eraseIntel4400_GBA();
				resetIntel_GBA(0x200000);
			}
			else if (flashid == 0x227E) {
				eraseMX29GL128E_GBA();
				resetMX29GL128E_GBA();
			}
			//Write flashrom
			iprintf("Writing ");
			if (flashid == 0x8802 || flashid == 0x8816) {
				writeIntel4000_GBA();
			}
			else if (flashid == 0x227E) {
				writeMX29GL128E_GBA();
			}
			
			return true;
		}
		else {
			iprintf("Can't open file");
		}
	}
  	else {
		iprintf("Error\n\nUnknown Flash\nFlash ID: %04X\n\n", flashid);
	}
	swiDelay(5000);
	
	return false;
}


