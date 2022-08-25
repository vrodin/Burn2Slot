#include "app.h"
#include "ID.h"

u8* sdBuffer = (u8*)malloc(sizeof(u8) * 512);
CartI CartInfo;

u16 addr_1[] = { 0x555, 0x2AA, 0x555, 0x555, 0x2AA, 0x555};
u16 addr_2[] = { 0xAAA, 0x555, 0xAAA, 0xAAA, 0x555, 0xAAA};
u16 addr_3[] = { 0x5555, 0x2AAA, 0x5555, 0x5555, 0x2AAA, 0x5555};

u16 data_1[] = { 0xAA, 0x55, 0x90};
u16 data_2[] = { 0xA9, 0x56, 0x90};
u16 data_3[] = { 0xAA, 0x55, 0x80, 0xAA, 0x55, 0xA0};
u16 data_4[] = { 0xA9, 0x56, 0x80, 0xA9, 0x56, 0xA0};

u16* getAddr(){
	switch(CartInfo.AddrType){
		case 0: return addr_1;
		case 1: return addr_2;
		case 2: return addr_3;
	}
}


u16* getDataType(){
	switch(CartInfo.DataType){
		case 0: return data_1;
		case 1: return data_2;
	}
}

u16* getDataBurn(){
	switch(CartInfo.DataType){
		case 0: return data_3;
		case 1: return data_4;
	}
}


u8  busMSP512[] = { 15, 7, 14, 6, 13, 5, 12, 4, 0, 8, 1, 9, 2, 10, 3, 11};
u32 fileSize = 0;
FILE* fd;


void wait(u32 count = 1){
	while(count){
		asm("nop");
		count--;
	}	
}


u16 swapBits(u16 data){ 
	u16 bit1 = (data >> 0) & 1;
	u16 bit2 = (data >> 1) & 1;
	u16 x = (bit1 ^ bit2);
	x = (x << 0) | (x << 1);
	u16 result = data ^ x;
	return result;
}


u16 swapBitsToMSP512(u16 data){
	u16 result = 0;
	for(int i = 0; i < 16; ++i){
		result |= ((data >> busMSP512[i]) & 1) << i;
	}
	return result;
}


u16 swapBitsFromMSP512(u16 data){
	u16 result = 0;
	for(int i = 0; i < 16; ++i){
		result |= ((data >> i) & 1) << busMSP512[i];
	}
	return result;
}


u16 read_word(u32 address){//GBAROM  ((u16*)0x08000000)
	return GBAROM[address];
}


u16 read_swapword(u32 address){
	return swapBits(read_word(address));
}


u16 read_swapword_msp512(u32 address){
	return swapBitsFromMSP512(read_word(address));
}


void write_word(u32 address, u16 byte){// GBA_BUS   ((vu16 *)(0x08000000))
	GBA_BUS[address] = byte;
	wait(0xF);
}


void write_swapword(u32 address, u16 byte){
	write_word(address, swapBits(byte));
}


void write_swapword_msp512(u32 address, u16 byte){
	write_word(address, swapBitsToMSP512(byte));
}


void write_cho(u32 address, u16 byte){
	switch(CartInfo.busType){
		case 0: return write_word(address, byte); 
		case 1: return write_swapword(address, byte);
		case 2: return write_swapword_msp512(address, byte);
	}
}

u16 read_cho(u32 address){
	switch(CartInfo.busType){
		case 0: return read_word(address);
		case 1: return read_swapword(address);
		case 2: return read_swapword_msp512(address); 
	}
}


void eraseSectorAMD(u32 addr){
	u16* data_sec = getDataBurn();
	u16* addr_sec = getAddr();
	for(u8 i = 0; i < 4; ++i){//5
		write_cho(addr_sec[i], data_sec[i]);
	}
	write_cho(addr, 0x30);
	u16 statusReg = 0;
	while ((statusReg | 0xFF7F) != 0xFFFF){
		statusReg = read_cho(addr);
		wait();
	}
	write_cho(0, 0xF0);
}


void writeWordAMD(u32 addr, u16 data){
	u16* data_sec = getDataBurn();
	u16* addr_sec = getAddr();
	for(u8 i = 0; i < 2; ++i){//3
		write_cho(addr_sec[i], data_sec[i]);
	}
	write_word(addr, data);
	int timeout = 0x1000;
	u16 statusReg = read_word(addr);
	while(timeout && (statusReg != data)){
		timeout--;
		statusReg = read_word(addr);
		wait();
	}
	if(!timeout){
		eraseSectorAMD(addr);
		writeWordAMD(addr, data);
	}	
}


int eraseSectorIntel(u32 addr){
	write_word(addr,0x50);
	write_word(addr,0x60);
	write_word(addr,0xD0);
	write_word(addr,0x20);
	write_word(addr,0xD0);
	
	wait(0x10000);
	
	u16 statusReg = 0;
	while ((statusReg | 0xFF7F) != 0xFFFF){
		statusReg = read_word(addr);
		wait();
	}
	write_word(addr, 0xFF);
	return 0;
}


int writeWordIntel(u32 addr, u16 data){
	write_word(addr, 0x50);
	write_word(addr, 0xFF);
	write_word(addr, 0x40);
	write_word(addr, data);
	int timeout = 0x1000;
	while(timeout && read_word(addr) == 0xFFFF){timeout--;wait();}
	while(timeout && read_word(addr) == 0){timeout--;wait();}
	while(timeout && !(read_word(addr)&0x80)){timeout--;wait();}
	//Sector was locked, unlock & erase, then try again
	if(read_word(addr) & 3){
		eraseSectorIntel(addr);
		writeWordIntel(addr, data);
	}
	if(!timeout || (read_word(addr) & 0x10)){
		write_word(addr, 0xFF);
		return -1;
	}
	write_word(addr, 0xFF);
	return 0;
}


void erase(u32 needSpace, bool isIntel){
	for(int addr = 0; addr < needSpace; addr += 0x8000){
		if(isIntel) eraseSectorIntel(addr);
		else eraseSectorAMD(addr);
		printTop("\rERACE %d\%", (addr + 0x8000) * 100/needSpace );
	}
	printTop("\n");
}


void detectIntel(){
	u16 flashid, manufactorID;
	write_word(0x0, 0xF0);//增加reset
	write_word(0x0, 0xFF);
	write_word(0x0, 0x50);
	write_word(0x0, 0x90);
	u16 ITSigns[] = {0x88C6, 0x88C4, 0x880D, 0x8813, 0x880C, 0x880F, 0x8812, 0x8816, 0x8815, 0x8810, 0x880E};
	u16 BFSigns[] = {0x8902, 0x8904, 0x887D, 0x88B0, 0x887E};
	
	flashid = read_word(0x1);
	manufactorID = read_word(0x0);
	
	for(u16 i = 0; i < 11;i++){//{INTEL, 0
		if(flashid == ITSigns[i]){
			CartInfo.FID = flashid;
			CartInfo.MID = manufactorID;
			write_word(0x55, 0x98);//cfi
			CartInfo.size = (u32)(1<<read_word(0x27));
			write_word(0x0, 0xFF);
			CartInfo.flashType = 1;
			return;
		}
	}
	for(u16 i = 0;i < 5;i++){//{INTEL_BUFFERED,0,
		if(flashid == BFSigns[i]){
			CartInfo.FID = flashid;
			CartInfo.MID = manufactorID;
			write_word(0x55, 0x98);//cfi
			CartInfo.size = (u32)(2<<read_word(0x27));//2左移
			write_word(0x0, 0xFF);
			CartInfo.flashType = 11;
			return;
		}
	}
	if(flashid == 0x00B0 || flashid == 0x00E2){//sharp增加e2//{INTEL, 0 && manufactorID == 0xB0
		CartInfo.FID = flashid;
		CartInfo.MID = manufactorID;
		write_word(0x55, 0x98);//cfi
		CartInfo.size = (u32)(1<<read_word(0x27));
		write_word(0x0, 0xFF);
		CartInfo.flashType = 1;
		return;
	}
	if(manufactorID == 0x1C){//MITSUBISHI002B和1C卡带识别//{INTEL, 0
		u16 Turbo = read_word(0x2);
		write_word(0x2, 0xF0);//增加reset
		write_word(0x2, 0xFF);
		write_word(0x2, 0x50);
		write_word(0x2, 0x90);
		if( Turbo == read_word(0x4)){
			CartInfo.FID = read_word(0x3);//readflash(0x1);2=0x2 0x20不等于0x0
			CartInfo.MID = manufactorID;
			write_word(0x57, 0x98);//cfi
			CartInfo.size = (u32)(2<<read_word(0x29));//27改26均无效
			write_word(0x0, 0xFF);
			write_word(0x2, 0xFF);
			CartInfo.flashType = 16;
			return;
		}else{
			write_word(0x2, 0xF0);
			CartInfo.FID = flashid;//readflash(0x1);2=0x2 0x20不等于0x0
			CartInfo.MID = manufactorID;
			write_word(0x55, 0x98);//cfi
			CartInfo.size = (u32)(1<<read_word(0x27));//27改26均无效
			write_word(0x0, 0xFF);
			CartInfo.flashType = 1;
			return;
		}
	}
	write_word(0x0, 0xFF);
}



void detectAMD(){
	u16 flashid, manufactorID;
	for(CartInfo.AddrType = 0; CartInfo.AddrType < 3; CartInfo.AddrType++){
		u16* addr_sec = getAddr();
		for(CartInfo.DataType = 0; CartInfo.DataType < 2; CartInfo.DataType++){
			u16* data_sec = getDataType();
			for(CartInfo.busType = 0; CartInfo.busType < 3; CartInfo.busType++){
				for(u8 i = 0; i < 2; ++i){
					write_cho(addr_sec[i], data_sec[i]);
				}
				flashid = read_cho(0x1);
				manufactorID = read_cho(0x0);
				if (((flashid >> 8) & 0xFF) == 0x22){// || flashid == 0x0084){//关键判断部分，决定模式的
					CartInfo.FID = flashid;
					CartInfo.MID = manufactorID;
					write_cho(addr_sec[0], 0x98);//获取CFI
					CartInfo.size = (u32)(1<<read_cho(0x27));
					CartInfo.flashType = 2;
					printf("\x1b[15;2Haddr:%d data:%d type:%d ", CartInfo.AddrType, CartInfo.DataType, CartInfo.busType);
					write_cho(0, 0xF0);
					return;
				}
			}
		}
	}
	write_cho(0, 0xF0);
	CartInfo.AddrType = 0;
	CartInfo.DataType = 0;
	CartInfo.busType = 0;
}


void idFlashrom_GBA(){
	CartInfo.FID = 0;
	
	detectIntel();
	if (CartInfo.FID != 0){
		return;
	}
	
	detectAMD();
	if(CartInfo.FID != 0){
		return;
	}

	CartInfo.FID = 0;
}


void writeByWord(bool isIntel){
	u16 currWord;
	u32 address;
	for (u32 currSector = 0; currSector < fileSize; currSector += 0x20000){
		for (u32 currSdBuffer = 0; currSdBuffer < 0x20000; currSdBuffer += 512){
			fread(sdBuffer, 1, 512, fd);
			for (u16 currWriteBuffer = 0; currWriteBuffer < 512; currWriteBuffer += 2){
				currWord = ( ( sdBuffer[currWriteBuffer + 1] & 0xFF ) << 8 ) | ( sdBuffer[currWriteBuffer] & 0xFF );
				address = (currSector + currSdBuffer + currWriteBuffer) / 2;
				if(isIntel) writeWordIntel(address, currWord);
				else writeWordAMD(address, currWord);
			}
			printTop("\rWriting %d\%", (currSector + currSdBuffer + 512) * 100/fileSize );
		}
	}
	printTop("\n");
}


bool flashRepro_GBA(){
	std::string filename = browseForFile({""});
	fd = fopen(&filename[0], "rb");
	idFlashrom_GBA();
	u16 FID = CartInfo.FID;
	u16 MID = CartInfo.MID;
	if (FID != 0){
		u32 prev = ftell(fd);
		fseek(fd, 0L, SEEK_END);
		fileSize = ftell(fd);
		fseek(fd,prev,SEEK_SET); 
		if (fileSize){
			printTop("File size: %DMB\n", fileSize / 0x100000);
			//erase
			if (CartInfo.flashType == 1 ){
				erase(fileSize/2, true);
			}else if (CartInfo.flashType == 2){
				erase(fileSize/2, false);
			}
			//Write flashrom
			if (CartInfo.flashType == 1){
				writeByWord(true);
			}else if (CartInfo.flashType == 2){
				writeByWord(false);
			}
			return true;
		}else {
			printTop("Can't open file");
		}
	}else {
		printTop("Error\n\nUnknown Flash\nFlash ID: %04X\n\n", FID);
	}
	return false;
}


void check(){
	idFlashrom_GBA();
	if (CartInfo.FID != 0){
		printTop("ID: %04x Size: %dMB\n", CartInfo.FID, CartInfo.size / 0x100000);
		printTop("Manuf:%02x %s \n", CartInfo.MID, getManufacturByID(CartInfo.MID));
		return;
	}
	printTop("unknown");
}

