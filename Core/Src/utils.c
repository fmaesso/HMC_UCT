/*
 * utils.c
 *
 *  Created on: Sep 23, 2023
 *      Author: ferna
 */
#include "main.h"
#include "utils.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "diskio.h"
#include "cec.h"
#include "leds.h"

char LEDOUT_Status;
//uint64_t eep_buffer[256];
char StrValores[MAX_STRS][MAX_STRS_LEN];
volatile unsigned char BitTempo10ms, BitTempo100ms, BitTempo500ms, BitTempo1s, BitTempo2s, BitTempo5s, DivTimeLed, TempoPadraoLed;

// Magic numbers for 32-bit hashing.  Copied from Murmur3.
static const uint32_t c1 = 0xcc9e2d51;
static const uint32_t c2 = 0x1b873593;

extern TTimers Timers;

extern I2C_HandleTypeDef hi2c3;

//extern I2C_HandleTypeDef hi2c1;
//#define I2C_TEMP 		&hi2c1

//***********************************************************
//HAL_IncTick
//***********************************************************
void HAL_IncTick(void){
	uwTick += (uint32_t)uwTickFreq;
	if((uwTick % TEMPO10MS) == 0){
		BitTempo10ms = 1;
	}
	if((uwTick % TEMPO100MS) == 0){
		BitTempo100ms = 1;
	}
	if((uwTick % TEMPO500MS) == 0){
		BitTempo500ms = 1;
	}
	if((uwTick % TEMPO1S) == 0){
		BitTempo1s = 1;
	}
	if((uwTick % TEMPO2S) == 0){
		BitTempo2s = 1;
	}
	if((uwTick % TEMPO5S) == 0){
		BitTempo5s = 1;
	}
}


//******************************************************************************
//
//******************************************************************************
void GetDados(char *p, int m){
  const char marcas[3][6] = {{";,/:"}, {","}, {";="}};
  char * pch;
  int i = 0;
  memset((unsigned char *)&StrValores[0], 0, MAX_STRS * MAX_STRS_LEN);

  pch = strtok (p, marcas[m]);        //",/-:");
  while (pch != NULL){
    strcpy(StrValores[i], pch);
    pch = strtok (NULL, marcas[m]);   //",/-:");
    i++;
  }
}

//******************************************************************************
//
//******************************************************************************
void SetLedPWM(int V){
	int  p;
	p = V * 0xFFFF / 100;
	TIM4->CCR2 = p;
}



//*********************************************************************/
// V é o valor percentual e o retorno é o valor referente a porcentagem
// V = 70% e retorna 178
//*********************************************************************/
unsigned char PercentToValue(unsigned char V){
  return(V * 257 / 100);
}

//*********************************************************************/
// V é o valor que se deseja saber quanto é a porcentagem
// V é 178 e retorna 70%
//*********************************************************************/
unsigned char ValueToPercent(unsigned char V){
  return(V * 100 / 257);
}


//***********************************************************
//
//***********************************************************
static uint32_t UNALIGNED_LOAD32(const char *p) {
  uint32_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

#define BKPSRAM_SIZE 4096
#define BKPSRAM_BASE_ADDR ((uint32_t*)0x38800000)  // Endereço base da Backup RAM

void BackupRAM_Init(void) {
    // Habilitar acesso ao Backup Domain
    //RCC->APB1LENR |= RCC_APB1LENR_PWREN;  // Habilita o clock do PWR
	HAL_PWR_EnableBkUpAccess();

	// Libera acesso ao Backup Domain

//	__HAL_RCC_PWR_CLK_ENABLE();
    PWR->CR1 |= PWR_CR1_DBP;

    HAL_PWREx_EnableBkUpReg();
    // Aguardar até que o acesso seja liberado
    while (!(PWR->CR1 & PWR_CR1_DBP));

    // Habilita a retenção da Backup RAM
    PWR->CR2 |= PWR_CR2_BREN;

    // Aguarda ativação
    while (!(PWR->CR2 & PWR_CR2_BREN));
}

//***********************************************************
//
//***********************************************************
static uint32_t Fetch32(const char *p) {
  return UNALIGNED_LOAD32(p);
}

//***********************************************************
//
//***********************************************************
static uint32_t Rotate32(uint32_t val, int shift) {
  // Avoid shifting by 32: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

//***********************************************************
// A 32-bit to 32-bit integer hash copied from Murmur3.
//***********************************************************
static uint32_t fmix(uint32_t h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

//***********************************************************
//
//***********************************************************
static uint32_t Mur(uint32_t a, uint32_t h) {
  // Helper from Murmur3 for combining two 32-bit values.
  a *= c1;
  a = Rotate32(a, 17);
  a *= c2;
  h ^= a;
  h = Rotate32(h, 19);
  return h * 5 + 0xe6546b64;
}

//***********************************************************
//
//***********************************************************
uint32_t Hash32Len5to12(const char *s, size_t len) {
  uint32_t a = (uint32_t)len, b = a * 5, c = 9, d = b;
  a += Fetch32(s);
  b += Fetch32(s + len - 4);
  c += Fetch32(s + ((len >> 1) & 4));
  return fmix(Mur(c, Mur(b, Mur(a, d))));
}

//***********************************************************
//
//***********************************************************
void CreateSN(void){
//	uint8_t uidstr[12];
//    memcpy (&uidstr, (uint8_t *) DBGMCU->IDCODE, 12);
//    uint32_t uid = Hash32Len5to12 ((const char *)uidstr, 12);
//	memcpy((uint8_t *)&Config.Placa.SN[4], (uint8_t *)&uid, 4);
}

//******************************************************************************
//
//******************************************************************************
uint32_t millis(void){
	return HAL_GetTick();
}


//***********************************************************
//
//***********************************************************
char format_eep(void){
//	uint32_t error;
//	FLASH_EraseInitTypeDef flashErase;

//	HAL_FLASH_Unlock();
//	flashErase.NbPages = 1;
//	flashErase.Page = _EE_USE_FLASH_PAGE_OR_SECTOR;
//	flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
//	flashErase.Banks = _EE_FLASH_BANK;
//	if (HAL_FLASHEx_Erase(&flashErase, &error) == HAL_OK){
//	    HAL_FLASH_Lock();
//	    return 0;
//	}
//	HAL_FLASH_Lock();
	return 1;

}

//***********************************************************
//
//***********************************************************
void read_eep(uint32_t add, uint32_t len, uint8_t *data){
	for(uint32_t i = 0; i < len + add; i++){
		*data++ = (*(__IO uint8_t*) (i + _EE_ADDR_INUSE));
	}
}

//***********************************************************
//
//***********************************************************
char write_eep(uint32_t add, uint32_t len, uint8_t *data){
#define LEN_EEP		8
//	uint32_t qi = len / LEN_EEP;
//	while((qi * LEN_EEP) < len){
//		qi++;
//	}
//	memset((uint8_t *)&eep_buffer, 0, qi);
//	memcpy((uint8_t *)&eep_buffer, data, len);
//
//	HAL_FLASH_Unlock();
//    for (uint32_t i = 0; i < qi; i++){
//        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, ((i * LEN_EEP) + add) + _EE_ADDR_INUSE, eep_buffer[i]) != HAL_OK){
//	    	HAL_FLASH_Lock();
//	    	return 0;
//	    }
//	}
//	HAL_FLASH_Lock();
	return 1;
}

//***********************************************************
//
//***********************************************************
char get_asc(unsigned char v, unsigned char p){
	if(p == 0){
		v = v >> 4;
	}
	v &= 0x0f;
	if (v >= 0x0a){
		v += 0x37;
	}else{
		v |= 0x30;
	}
	return(v);
}

//*************************************************************************
//
//*************************************************************************/
unsigned char get_hex(unsigned char i){
  if (i <= 0x39){
    i&= 0x0f;
  }else{
    i-= 0x37;
  }
  return(i);
}

//*************************************************************************
//retorna uint8_t de 2 bytes em ASCII. "F1" -> 0xF1
//*************************************************************************/
unsigned char AtoHex(uint8_t *S){
	uint8_t r = 0;
	r = get_hex(*S++) << 4;
	r |= get_hex(*S);
	return r;
}



//*************************************************************************
//00010101
//*************************************************************************/
uint8_t i2c_detect(void){
	uint8_t devices = 0u;
	HAL_StatusTypeDef result;

	for (uint8_t i = 1; i < 128; i++){
		result = HAL_I2C_IsDeviceReady(&hi2c3, (uint16_t)(i<<1), 2, 2);
		if (result == HAL_OK){
			printf("Device found: 0x%02X\n", i);
			devices++;
		}
	}
	if (0u == devices){
		printf("No device found.\n");
	}else{
		printf("Total found devices: %d\n", devices);

	}
	return devices;
}


void memory_dump(const void *mem, size_t size) {
    const unsigned char *data = (const unsigned char *)mem;
    const size_t bytes_per_line = 16; // 16 bytes por linha

    for (size_t i = 0; i < size; i += bytes_per_line) {
        printf("%08X  ", (unsigned int)(data + i));
        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < size) {
                printf("%02X ", data[i + j]);
            } else {
                printf("   "); // Preenchimento para alinhar
            }
        }
        printf(" ");

        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < size) {
                unsigned char ch = data[i + j];
                printf("%c", isprint(ch) ? ch : '.'); // Substitui não imprimíveis por '.'
            }
        }

        printf("\r\n");
    }
    printf("\r\n");
}

void DumpSD(uint32_t sector){
	uint8_t b[512];
	int8_t res;
	res = disk_read(0, b, sector, 1);
	if (res == 0){
		memory_dump(b, 512);
	}
}


uint8_t decode_h_files(float *buffer, char* input, int* size) {
    // Procura pelo tamanho da matriz dentro dos colchetes
    char* start_bracket = strchr(input, '[');
    char* end_bracket = strchr(input, ']');

    if (!start_bracket || !end_bracket || start_bracket > end_bracket) {
        printf("Erro: Formato inválido.\n");
        return 0;
    }

    // Extrai o número entre colchetes (tamanho da matriz)
    *size = atoi(start_bracket + 1);

    if (*size <= 0) {
        printf("Erro: Tamanho inválido.\n");
        return 0;
    }

    // Procura o início dos valores (após '{')
    char* start_values = strchr(input, '{');
    if (!start_values) {
        printf("Erro: Formato inválido.\n");
        return 0;
    }

    // Lê os valores do array e armazena no buffer
    char* token = strtok(start_values + 1, ", }");  // Ignora '{' e usa delimitadores ", }"
    int index = 0;

    while (token && index < *size) {
        buffer[index++] = atof(token);  // Converte string para float
        token = strtok(NULL, ", }");    // Próximo valor
    }

    // Verifica se o número de valores lidos é correto
    if (index != *size) {
        printf("Aviso: Número de valores lidos (%d) diferente do esperado (%d).\n", index, *size);
    }

    return 1;
}


