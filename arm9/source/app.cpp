#include "app.h"
#include "ID.h"

u16 flashid = 0;
u16 cartSize = 0;
u16 busType = 0;
u8  adressSeqType = 0;
u16 intelType = 0;
u16 manufactorID = 0;
u16 seq_1[] = { 0x555, 0x2AA, 0x555, 0x555, 0x2AA, 0x555};
u16 seq_2[] = { 0xAAA, 0x555, 0xAAA, 0xAAA, 0x555, 0xAAA}; 

unsigned long fileSize = 0;
FILE* fd;
u8* sdBuffer = (u8*)malloc(sizeof(u8) * 512);

u16 swapBits(u16 n) 
{
	u16 p1 = 0;
	u16 p2 = 1;
	u16 bit1 = (n >> p1) & 1;
	u16 bit2 = (n >> p2) & 1;
	u16 x = (bit1 ^ bit2);
	x = (x << p1) | (x << p2);
	u16 result = n ^ x;
	return result;
}

u16 read_word_rom(unsigned long address) 
{
	return GBAROM[address];
}
u16 read_swapped_word_rom(unsigned long address) 
{
	return swapBits(read_word_rom(address));
}

void write_word_rom(unsigned long address, u16 byte) 
{
	GBA_BUS[address] = byte;
	swiDelay(10);
}
void write_swapped_word_rom(unsigned long address, u16 byte) 
{
	write_word_rom(address, swapBits(byte));
}

void ( *( write_word() ) )(unsigned long address, u16 byte) 
{ 
	switch(busType) {
		case 0: return write_word_rom; 
		case 1: return write_swapped_word_rom; 
	}
	
} 

u16 ( *( read_word() ) )(unsigned long address) 
{ 
	switch(busType) {
		case 0: return read_word_rom; 
		case 1: return read_swapped_word_rom; 
	}
	
} 

u16* getAddressSeq22XX() 
{
	u16* data_sec;
	switch(adressSeqType) {
		case 0: data_sec = seq_1; 
		case 1: data_sec = seq_2; 
	}
	
	return data_sec;
}

void reset22XX() 
{
	write_word()(0, 0xF0);
}

void detect22XX() 
{
	u8 data_sec[] = { 0xAA, 0x55, 0x90};
	for(adressSeqType = 0; adressSeqType <= 1; ++adressSeqType) {
		u16* addr_sec = getAddressSeq22XX();
		for(busType = 0; busType <= 1; ++busType) {
			for(u16 i = 0; i < 3; ++i) {
				write_word()(*(addr_sec + i), data_sec[i]);
			}
			flashid = read_word()(0x1);
			manufactorID = read_word()(0x0);
			if (((flashid >> 8) & 0xFF) == 0x22) {
				reset22XX();
				return;
			}
		}
	}
	
	flashid = 0;
	busType = 0;
}

void programmCycle22XX() 
{
	u8 data_sec[] = { 0xAA, 0x55, 0xA0};
	u16* addr_sec = getAddressSeq22XX();
	
	for(u16 i = 0; i < 3; ++i) {
		write_word()(*(addr_sec + i), data_sec[i]);
	}
}

void erase22XX() 
{
	u8 data_sec[] = { 0xAA, 0x55, 0x80, 0xAA, 0x55, 0x10};
	u16* addr_sec = getAddressSeq22XX();

	for(u16 i = 0; i < 6; ++i) {
		write_word()(*(addr_sec + i), data_sec[i]);
	}

	u16 statusReg = 0;
	while ((statusReg | 0xFF7F) != 0xFFFF) {
		statusReg = read_word()(0x0);
		swiDelay(10);
	}	
}

void resetIntel_GBA(unsigned long partitionSize) 
{
	for (unsigned long currPartition = 0; currPartition < cartSize; currPartition += partitionSize) {
		write_word()(currPartition, 0xFFFF);
	}
}

void detectIntel() {

	u16 manufactorID = read_word_rom(0x0);
	u16 bufSigns[] = {0x8902, 0x8904, 0x887D, 0x887E, 0x88B0};
	
	write_word_rom(0, 0xFF);
	write_word_rom(0, 0x50);
	write_word_rom(0, 0x90);
	u16 flashid = read_word_rom(0x2) & 0xFF00 || read_word_rom(0x4) & 0xFF;
	if (flashid == 0x8802 || flashid == 8816) {
		write_word_rom(0, 0xFF);
		intelType = 3;
		return;
	}
	
	flashid = read_word_rom(0x1);
	
	for(unsigned int i = 0;i < 4;i++){
		if(flashid == bufSigns[i]) {
			intelType = 2;
			return;
		}
	}
	
	if(((flashid >> 8) & 0xFF) == 0x88) {
		intelType = 1;
		return;
	}
	
	if(flashid == 0xB0 && read_word_rom(0) == 0xB0){
		intelType = 1;
		return;
	}
		
	intelType = 0;
	flashid = 0;
}


void idFlashrom_GBA() 
{
	detectIntel();
	if (flashid != 0) {
		return;
	}
	
	detect22XX();
	if(flashid != 0) {
		return;
	}
	
	write_word_rom(0x154, 0xF0F0);
	write_word_rom(0x154, 0x9898);
	flashid = read_word_rom(0x40);
	if(flashid == 0x5152 && read_word_rom(0x44) == 0x5251){
		write_word_rom(0x154, 0xF0F0);
		return;
	}
	
	write_word_rom(0x0, 0xF0F0);
	write_word_rom(0xAA, 0x9898);
	flashid = read_word_rom(0x20);
	if(flashid == 0x5152){
		write_word_rom(0x0, 0xF0F0);
		return;
	}
	


	flashid = 0;
}

void eraseIntel4000_GBA() 
{
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

void write22XX() {
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
				programmCycle22XX();

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

	if (((flashid >> 8) & 0xFF) == 0x22 || ((flashid >> 8) & 0xFF) == 0x88) {
		iprintf("ID: %04X Size: %DMB\n", flashid, cartSize / 0x100000);

		iprintf("%s \n", getManufacturByID(manufactorID));
	
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
			else if (((flashid >> 8) & 0XFF) == 0x22) {
				erase22XX();
				reset22XX();
			}
			//Write flashrom
			iprintf("Writing ");
			if (intelType > 0) {
				writeIntel4000_GBA();
			}
			else if (((flashid >> 8) & 0xFF) == 0x22) {
				write22XX();
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


