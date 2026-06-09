/*
 * utils.h
 *
 *  Created on: Sep 23, 2023
 *      Author: ferna
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#include "main.h"
#define TEMPO10MS 			10         	//10ms
#define TEMPO100MS 			100         	//100ms
#define TEMPO500MS 			500         	//100ms
#define TEMPO1S 			1000			//1S
#define TEMPO2S 			2000
#define TEMPO5S 			5000       	//5 sec


#define MAX_STRS		10
#define MAX_STRS_LEN	50

extern char StrValores[MAX_STRS][MAX_STRS_LEN];
extern volatile uint8_t BitTempo10ms, BitTempo100ms, BitTempo500ms, BitTempo1s, BitTempo2s, BitTempo5s, DivTimeLed, TempoPadraoLed;



#define _EE_USE_FLASH_PAGE_OR_SECTOR	(127)
#define _EE_SIZE              			2048
#define _EE_ADDR_INUSE        			(((uint32_t)0x08000000) | (_EE_SIZE * _EE_USE_FLASH_PAGE_OR_SECTOR))
#define _EE_FLASH_BANK        			FLASH_BANK_1
#define _EE_PAGE_OR_SECTOR    			PAGE_NUM

void read_eep(uint32_t add, uint32_t len, uint8_t *data);
char write_eep(uint32_t add, uint32_t len, uint8_t *data);
char format_eep(void);


unsigned char PercentToValue(unsigned char V);
unsigned char ValueToPercent(unsigned char V);
void GetDados(char *p, int m);
uint32_t millis(void);
uint8_t i2c_detect(void);
char get_asc(unsigned char v, unsigned char p);
unsigned char get_hex(unsigned char i);
unsigned char AtoHex(uint8_t *S);
void DumpSD(uint32_t sector);
void memory_dump(const void *mem, size_t size);
uint8_t decode_h_files(float *buffer, char* input, int* size) ;
void USBIrQ(uint8_t v);
void SetLedPWM(int V);

#endif /* INC_UTILS_H_ */
