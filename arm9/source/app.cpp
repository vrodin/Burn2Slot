#include "app.h"
#include "util.h"
#include "ID.h"

#define WRITE_BUS_COUNT 6
#define ADD_SEQ_COUNT 6
u8* sdBuffer = (u8*)malloc(sizeof(u8) * 512);
CartInfo* cart = (CartInfo*)malloc(sizeof(CartInfo) + 1);

u32 seq_0[] = { 0x555, 0x2AA, 0x555, 0x555, 0x2AA, 0x555};
u32 seq_1[] = { 0xAAA, 0x555, 0xAAA, 0xAAA, 0x555, 0xAAA};
u32 seq_2[] = { 0xAAA, 0x554, 0xAAA, 0xAAA, 0x554, 0xAAA};
u32 seq_3[] = { 0x5555, 0x2AAA, 0x5555, 0x5555, 0x2AAA, 0x5555};
u32 seq_4[] = { 0xAAAA, 0x5554, 0xAAAA, 0xAAAA, 0x5554, 0xAAAA};
u32 seq_5[] = { 0x1554, 0xAAA, 0x1554, 0x1554, 0xAAA, 0x1554};


u8  bus_1[] = { 15, 7, 14, 6, 13, 5, 12, 4, 0, 8, 1, 9, 2, 10, 3, 11};
u8  bus_2[] = { 0, 8, 1, 9, 6, 14, 7, 15, 2, 10, 3, 11, 4, 12, 5, 13};
u32 fileSize = 0;
u16 bufferSize = 0;
FILE* fd;

u32 lag = 0xF;

void wait(u32 count = 1)
{
	while(count) {
		asm("nop");
		count--;
	}	
}

u16 swapBits(u16 data) 
{ 
	u16 bit1 = (data >> 0) & 1;
	u16 bit2 = (data >> 1) & 1;
	u16 x = (bit1 ^ bit2);
	x = (x << 0) | (x << 1);
	u16 result = data ^ x;
	return result;
}

u16 swapTo(u8 *bus, u16 data)
{
	u16 result = 0;
	for(int i = 0; i < 16; ++i) {
		result |= ((data >> i) & 1) << bus[i];
	}

	return result;
}

u16 swapFrom(u8 *bus, u16 data) 
{
	u16 result = 0;
	for(int i = 0; i < 16; ++i) {
		result |= ((data >> bus[i]) & 1) << i;
	}

	return result;
}

u16 read_word_rom(u32 address) 
{
	return GBAROM[address];
}

u16 read_swapped_word_rom(u32 address) 
{
	return swapBits(read_word_rom(address));
}

u16 read_swapped_word_rom_bus_1(u32 address) 
{
	return swapFrom(bus_1, read_word_rom(address));
}

u16 read_swapped_word_rom_bus_2(u32 address) 
{
	return swapFrom(bus_2, read_word_rom(address));
}

void write_word_rom(u32 address, u16 byte) 
{
	GBA_BUS[address] = byte;
	wait(lag);
}

void write_swapped_word_rom(u32 address, u16 byte) 
{
	write_word_rom(address, swapBits(byte));
}

void write_swapped_word_rom_bus_1(u32 address, u16 byte) 
{
	write_word_rom(address, swapTo(bus_1, byte));
}

void write_swapped_word_rom_bus_2(u32 address, u16 byte) 
{
	write_word_rom(address, swapTo(bus_2, byte));
}

void write_doubled_word(u32 address, u16 byte) 
{
	write_word_rom(address, byte << 8 | byte);
}

void write_swapped_doubled_word(u32 address, u16 byte) 
{
	write_swapped_word_rom(address, byte << 8 | byte);
}

void ( *( write_word() ) )(u32 address, u16 byte) 
{ 
	switch(cart->busType) {
		case 0: return write_word_rom; 
		case 1: return write_swapped_word_rom; 
		case 2: return write_swapped_word_rom_bus_1;
		case 3: return write_swapped_word_rom_bus_2;
		case 4: return write_doubled_word;
		case 5: return write_swapped_doubled_word;
	}
	
} 

u16 ( *( read_word() ) )(u32 address) 
{ 
	switch(cart->busType) {
		case 0: return read_word_rom; 
		case 1: return read_swapped_word_rom;
		case 2: return read_swapped_word_rom_bus_1; 
		case 3: return read_swapped_word_rom_bus_2; 
		case 4: return read_word_rom; 
		case 5: return read_swapped_word_rom;
	}
	
} 

bool waitForFlash(u32 address, bool (*isReady)(u16), int timeout){
	while(timeout && !isReady(read_word_rom(address)) ){
		wait();
		timeout--;
	}
	if(!timeout){
		return false;
	}
	return true;
}


u32* getAddressSeq22XX() 
{
	u32* add_sec;
	switch(cart->adressSeqType) {
		case 0: add_sec = seq_0; break;
		case 1: add_sec = seq_1; break;
		case 2: add_sec = seq_2; break;
		case 3: add_sec = seq_3; break;
		case 4: add_sec = seq_4; break;
		case 5: add_sec = seq_5; break;
	}
	
	return add_sec;
}

void reset22XX() 
{
	u8 data_sec[] = { 0xAA, 0x55, 0xF0};
	u32* addr_sec = getAddressSeq22XX();
	
	for(u8 i = 0; i < 3; ++i) {
		write_word()(addr_sec[i], data_sec[i]);
	}
}

void detect22XX() 
{
	u8 data_sec[] = { 0xAA, 0x55, 0x90};
	u16 flashid, manufactorID;
	for(cart->adressSeqType = 0; cart->adressSeqType < ADD_SEQ_COUNT; cart->adressSeqType++) {
		u32* addr_sec = getAddressSeq22XX();
		for(cart->busType = 0; cart->busType < WRITE_BUS_COUNT; cart->busType++) {
			for(u8 i = 0; i < 3; ++i) {
				write_word()(addr_sec[i], data_sec[i]);
			}
			flashid = read_word()(0x1);
			manufactorID = read_word()(0x0);
			if (((flashid >> 8) & 0xFF) == 0x22 || flashid == 0x7E7E || flashid == 0x57 || flashid == 0xF8) {
				cart->flashid = flashid;
				cart->manufactorID = manufactorID;
				reset22XX();
				return;
			}
		}
	}
	cart->adressSeqType = 0;
	cart->busType = 0;
}

void eraseSector22XX(u32 addr)
{
	u8 data_sec[] = { 0xAA, 0x55, 0x80, 0xAA, 0x55};
	u32* addr_sec = getAddressSeq22XX();

	for(u8 i = 0; i < 5; ++i) {
		write_word()(addr_sec[i], data_sec[i]);
	}
	write_word()(addr, 0x30);

	u16 statusReg = 0;
	while ((statusReg | 0xFF7F) != 0xFFFF) {
		statusReg = read_word()(addr);
		wait();
	}
	reset22XX();
}

void writeWord22XX(u32 addr, u16 data) 
{
	u8 data_sec[] = { 0xAA, 0x55, 0xA0};
	u32* addr_sec = getAddressSeq22XX();
	
	for(u8 i = 0; i < 3; ++i) {
		write_word()(addr_sec[i], data_sec[i]);
	}
	write_word_rom(addr, data);
	
	int timeout = 0x2000;
	u16 statusReg = read_word_rom(addr);
	while(timeout && (statusReg != data)){
		timeout--;
		wait();
		statusReg = read_word_rom(addr);
	}
	
	if(!timeout){
		reset22XX();
		eraseSector22XX(addr);
		writeWord22XX(addr, data);
	}	
	
}

void resetIntel(u32 partitionSize) 
{
	for (u32 currPartition = 0; currPartition < cart->size; currPartition += partitionSize) {
		write_word()(currPartition, 0xFFFF);
	}
}

int eraseSectorIntel(u32 addr)
{
	write_word_rom(addr,0x50);
	write_word_rom(addr,0x60);
	write_word_rom(addr,0xD0);
	write_word_rom(addr,0x20);
	write_word_rom(addr,0xD0);
	
	waitForFlash(addr, WAIT_NON_FFFF, 0x10000);
	
	u16 statusReg = 0;
	while ((statusReg | 0xFF7F) != 0xFFFF) {
		statusReg = read_word_rom(addr);
		wait();
	}
	
	write_word_rom(addr, 0xFF);
	return 0;
}

int writeWordIntel(u32 addr, u16 data)
{
	write_word_rom(addr, 0x50);
	write_word_rom(addr, 0xFF);
	
	write_word_rom(addr, 0x40);
	write_word_rom(addr, data);
	int timeout = 0x1000;
	
	while(timeout && read_word_rom(addr) == 0xFFFF){timeout--;wait();}
	while(timeout && read_word_rom(addr) == 0){timeout--;wait();}
	while(timeout && !(read_word_rom(addr)&0x80)){
		timeout--;wait();
	}
	
	//Sector was locked, unlock & erase, then try again
	if(read_word_rom(addr) & 3){
		eraseSectorIntel(addr);
		writeWordIntel(addr, data);
	}

	if(!timeout || (read_word_rom(addr) & 0x10)){
		write_word_rom(addr, 0xFF);
		return -1;
	}
	
	write_word_rom(addr, 0xFF);
	
	return 0;

}

void erase(u32 needSpace, bool isIntel)
{
	for(int addr = 0; addr < needSpace; addr += 0x8000) {
		if(isIntel) {
			for(int smallAddr = addr; smallAddr < addr + 0x8000; smallAddr += 0x2000)
				eraseSectorIntel(smallAddr);
		}
		else eraseSector22XX(addr);
		printTop("\rERASE %d\%", (addr + 0x8000) * 100/needSpace );
	}
	printTop("\n");
}

void detectIntel() 
{
	cart->intelType = 0;
	u16 bufSigns[] = {0x8902, 0x8904, 0x887D, 0x887E, 0x88B0};
	write_word_rom(0x0, 0xFF);
	write_word_rom(0x0, 0x50);
	write_word_rom(0x0, 0x90);
	u16 manufactorID = read_word_rom(0x0);
	u16 flashid = read_word_rom(0x1);
	
	for(u8 i = 0; i < 4; i++){
		if(flashid == bufSigns[i]) {
			cart->manufactorID = manufactorID;
			cart->flashid = flashid;
			cart->intelType = 2;
			write_word_rom(0, 0xF0);
			return;
		}
	}
	
	if(((flashid >> 8) & 0xFF) == 0x88) {
		cart->manufactorID = manufactorID;
		cart->flashid = flashid;
		cart->intelType = 1;
		write_word_rom(0, 0xF0);
		return;
	}
	
	if(flashid == 0xB0 && manufactorID == 0xB0) {
		cart->manufactorID = manufactorID;
		cart->flashid = 0x88FF;
		cart->intelType = 1;
		write_word_rom(0, 0xF0);
		return;
	}
		
	if(manufactorID == 0x1C) {
		cart->manufactorID = manufactorID;
		cart->flashid = flashid;
		cart->intelType = 1;
		write_word_rom(0, 0xF0);
		return;
	}
	cart->intelType = 0;
}


void idFlashrom_GBA() 
{
	cart->flashid = 0;

	detectIntel();
	if (cart->flashid != 0) {
		return;
	}
	
	detect22XX();
	if(cart->flashid != 0) {
		reset22XX();
		return;
	}
	
	cart->flashid = 0;
}

void chipSize()
{
	write_word()(0x0, 0x50);
	write_word()(0x0, 0xFF);
	write_word()(0x55, 0x98);
	if( read_word()(0x10) == 0x51 &&
	    read_word()(0x11) == 0x52 &&
	    read_word()(0x12) == 0x59
	    ) {
	    	cart->size = (u32)(1<<read_word()(0x27));
	    	cart->bufferSize = (u16)(1<<read_word()(0x2A));
	    	write_word()(0x0, 0x50);
		write_word()(0x0, 0xFF);
		write_word()(0, 0xF0);
	    }
}

void writeIntel4000_GBA() 
{
	for (u32 currBlock = 0; currBlock < fileSize; currBlock += 0x20000) {
		// Write to flashrom
		for (u32 currSdBuffer = 0; currSdBuffer < 0x20000; currSdBuffer += 512) {
			// Fill SD buffer
			fread(sdBuffer, 1, 512, fd);

			// Write 32 words at a time
			for (u16 currWriteBuffer = 0; currWriteBuffer < 512; currWriteBuffer += 64) {
				// Unlock Block
				write_word_rom(currBlock + currSdBuffer + currWriteBuffer, 0x60);
				write_word_rom(currBlock + currSdBuffer + currWriteBuffer, 0xD0);

				// Buffered program command
				write_word_rom(currBlock + currSdBuffer + currWriteBuffer, 0xE8);

				// Check Status register
				u16 statusReg = read_word_rom(currBlock + currSdBuffer + currWriteBuffer);
				while ((statusReg | 0xFF7F) != 0xFFFF) {
					statusReg = read_word_rom(currBlock + currSdBuffer + currWriteBuffer);
					wait();
				}

				// Write u16 count (minus 1)
				write_word_rom(currBlock + currSdBuffer + currWriteBuffer, 0x1F);

				// Write buffer
				for (byte currByte = 0; currByte < 64; currByte += 2) {
					// Join two bytes into one word
					u16 currWord = ( ( sdBuffer[currWriteBuffer + currByte + 1] & 0xFF ) << 8 ) | ( sdBuffer[currWriteBuffer + currByte] & 0xFF );
					write_word_rom(currBlock + currSdBuffer + currWriteBuffer + currByte, currWord);
				}

				// Write Buffer to Flash
				write_word_rom(currBlock + currSdBuffer + currWriteBuffer + 62, 0xD0);

				// Read the status register at last written address
				statusReg = read_word_rom(currBlock + currSdBuffer + currWriteBuffer + 62);
				while ((statusReg | 0xFF7F) != 0xFFFF) {
					statusReg = read_word_rom(currBlock + currSdBuffer + currWriteBuffer + 62);
					wait();
				}
			}
		}
	}
}

void writeByWord(bool isIntel) 
{
	u16 currWord;
	u32 address;
	for (u32 currSector = 0; currSector < fileSize; currSector += 0x20000) {
		// Write to flashrom
		for (u32 currSdBuffer = 0; currSdBuffer < 0x20000; currSdBuffer += 0x200) {
		// Fill SD buffer
			fread(sdBuffer, 1, 512, fd);

			// Write 32 words at a time
			for (u16 currWriteBuffer = 0; currWriteBuffer < 512; currWriteBuffer += 2) {
				currWord = ( ( sdBuffer[currWriteBuffer + 1] & 0xFF ) << 8 ) | ( sdBuffer[currWriteBuffer] & 0xFF );
				address = (currSector + currSdBuffer + currWriteBuffer) / 2;
				if(isIntel) writeWordIntel(address, currWord);
				else writeWord22XX(address, currWord);
			}
			printTop("\rWriting %d\%", (currSector + currSdBuffer + 512) * 100/fileSize );
		}
	}
	printTop("\n");
}

void getFileSize()
{
	u32 prev = ftell(fd);
	fseek(fd, 0L, SEEK_END);
	fileSize = ftell(fd);
	fseek(fd,prev,SEEK_SET);
}

bool flashRepro_GBA() 
{
	std::string filename = browseForFile({".gba"});
	fd = fopen(&filename[0], "rb");
	// Check flashrom ID's
	idFlashrom_GBA();
	chipSize();
	u16 flashid = cart->flashid;
	u16 manufactorID = cart->manufactorID;
	u32 chipSize = cart->size;
	
	if (flashid != 0) {
		printTop("ID: %04X Size: %DMB\n", flashid, chipSize / 0x100000);
		printTop("%s \n", getManufacturByID(manufactorID));
		getFileSize();
    
		// Open file on sd card
		if (fileSize) {
			// Get rom size from file
			printTop("File size: %DMB\n", fileSize / 0x100000);
      
			// Erase needed sectors
			if ((((flashid >> 8) & 0XFF) == 0x88) || (((flashid >> 8) & 0XFF) == 0x89) || manufactorID == 0x1C) {
				erase(fileSize >> 1, true);
			}
			else if (((flashid >> 8) & 0XFF) == 0x22 || flashid == 0x7E7E || flashid == 0x57 || flashid == 0xF8) {
				erase(fileSize >> 1, false);
			}
			
			//Write flashrom
			if ((((flashid >> 8) & 0XFF) == 0x88) || (((flashid >> 8) & 0XFF) == 0x89) || manufactorID == 0x1C) {
				writeByWord(true);
			}
			else if (((flashid >> 8) & 0xFF) == 0x22 || flashid == 0x7E7E || flashid == 0x57 || flashid == 0xF8) {
				writeByWord(false);
			}
			
			return true;
		}
		else {
			printTop("Can't open file");
		}
	}
  	else {
		printTop("Error\n\nUnknown Flash\nFlash ID: %04X\n\n", flashid);
	}
	
	return false;
}
