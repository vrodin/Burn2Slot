#define ALLIANCE_ID		0x52
#define ATMEL_ID		0x1F
#define FUJITSU_ID		0x04
#define INTEL_ID		0x89
#define MACRONIX_ID		0xC2
#define SANYO_ID		0x62
#define SHARP_ID		0xB0
#define SPANSION_ID		0x01
#define SST_ID			0xBF
#define ST_ID			0x20
#define WINBOND_ID		0xDA
#define MITSUBISHI_ID	0x1C

char* manufactur = (char*)malloc(sizeof(char) * 12);

static char* getManufacturByID(u8 ID){
	switch(ID) {
		case ALLIANCE_ID:
			manufactur = "Alliance";
			break;
		case ATMEL_ID:
			manufactur = "Atmel";
			break;
		case FUJITSU_ID:
			manufactur = "Fujitsu";
			break;
		case INTEL_ID:
			manufactur = "Intel";
			break;			
		case MACRONIX_ID:
			manufactur = "Macronix";
		case SHARP_ID:
			manufactur = "Sharp";
			break;
		case SPANSION_ID:
			manufactur = "Spansion";
			break;
		case SST_ID:
			manufactur = "SST";
			break;
		case ST_ID:
			manufactur = "ST";
			break;
		case WINBOND_ID:
			manufactur = "Winbond";
			break;
		case MITSUBISHI_ID:
			manufactur = "Mitsubishi";
			break;
		default:
			manufactur = "Unknown";
	}
	return manufactur;
}
