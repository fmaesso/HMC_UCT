/*
 * arquivos.c
 *
 *  Created on: Nov 18, 2024
 *      Author: fmaes
 */

//*********************************************************************************
//#include "fatfs.h"
#include "main.h"
#include "string.h"
#include "arquivos.h"
#include "fatfs.h"
#include "diskio.h"
#include "cJSON.h"
#include "utils.h"
#include "quadspi_is.h"
//#include "W25Q.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "configs_json.h"

extern uint32_t TotalSize, FreeSpace;
extern FIL fil;
extern FATFS FatFs;

extern uint8_t BufferFile[];
extern cec_info_t cec;
extern configs_t Configs;

//extern TBal Bal;
//extern TAmpliCanal AmpliCanal[2];
//extern TFiltro Filtro;

DWORD fre_clust, fre_sect, tot_sect;

extern uint8_t ReadFlashBuf[];
//extern uint8_t WriteFlashBuf[SPIF_SECTOR_SIZE];

// Buffer temporário
uint8_t Buffer[4096];

uint8_t FormataMem(void){
	FRESULT FR_Status;
	BYTE work[4096];
	FR_Status = f_mkfs("0:", FM_FAT, 0, work, sizeof(work));
	return (uint8_t)FR_Status;
}


int8_t InitCard(void){
	FATFS *fs;
	FRESULT FR_Status;
	BYTE work[4096];
//	DWORD fre_sect, tot_sect;
//	CSP_QSPI_DisableMemoryMappedMode();
	FR_Status = f_mount(&FatFs, "0:", 1);
	if (FR_Status != FR_OK){
		FR_Status = f_mkfs("0:", FM_FAT, 0, work, sizeof(work));
//		if (FR_Status == FR_OK)
//		{
			FR_Status = f_mount(&FatFs, "0:", 1);   // <-- montar de novo
//		}
	}else{
		/* Get volume information and free clusters of drive 1 */
		FR_Status = f_getfree("0:", &fre_clust, &fs);
//		if (FR_Status) die(FR_Status);

		/* Get total sectors and free sectors */
		tot_sect = (fs->n_fatent - 2) * fs->csize;
		fre_sect = fre_clust * fs->csize;
	}
//	CSP_QSPI_EnableMemoryMappedMode();
	return (int8_t)FR_Status;
}

void SetInfosCEC(void){
	cec_info_to_json(&cec, (char *)&BufferFile, 4096);
	WriteFile((char *)"\\cec.txt", (char *)BufferFile);
}

void SetDefaultCEC(void){
	cec_info_defaults(&cec);
	SetInfosCEC();
}

void SetConfig(void){
	configs_to_json(&Configs, (char *)&BufferFile, 4096);
	WriteFile((char *)"\\config.txt", (char *)BufferFile);
}

void SetFabrica(void){
	configs_defaults(&Configs);
	SetConfig();
}

void GetConfig(void){
	uint8_t e = 0;
	if(ReadFile((char *)"\\config.txt", BufferFile) == 0){
		if(configs_from_json(&Configs, (char*)BufferFile) != 0){
			e = 1;
		}
	}else{
		e = 1;
	}
	if(e){
		SetFabrica();
	}
	e = 0;
	if(ReadFile((char *)"\\cec.txt", BufferFile) == 0){
		if(cec_info_from_json(&cec, (char*)BufferFile) != 0){
			e = 1;
		}
	}else{
		e = 1;
	}
	if(e){
		SetDefaultCEC();
	}
}


//*********************************************************************************
//
//*********************************************************************************
uint32_t LenFile(char *path){
  uint32_t t = 0;
  return t;
}

//*********************************************************************************
//
//*********************************************************************************
uint8_t listDir(void){
	uint8_t e = 1, l = 1;
    DIR dir;         // Diretório
    FILINFO fileInfo; // Informações sobre o arquivo
    FRESULT res;

    // Abre o diretório raiz
    res = f_opendir(&dir, (TCHAR *) "");
    if (res == FR_OK) {
        while (1) {
            // Lê o próximo item no diretório
            res = f_readdir(&dir, &fileInfo);
            if (res != FR_OK || fileInfo.fname[0] == '\0')
            	break; // Fim do diretório
            if (!(fileInfo.fattrib & AM_DIR)) {
                // É um arquivo, imprime o nome e tamanho
//            	printf("Nome: %s, Tamanho: %ln bytes\n", fileInfo.fname);
            	printf("% 2d - %s, Tamanho: %lu bytes\r\n", l++, fileInfo.fname, (unsigned long )fileInfo.fsize);
            } else {
                // É um diretório, imprime o nome
            	printf("% 2d - [DIRETORIO] %s\r\n", l++, fileInfo.fname);
            }
        }
        f_closedir(&dir); // Fecha o diretório
    } else {
    	printf("Erro ao abrir o diretorio: %d\r\n", res);
    }
	printf("\r\n% 2d - Fim\r\n", l);
	return e;
}


//*************************************************************
//AR/Config.txt
//*************************************************************
#define FILE_READ_BUFFER_SIZE 4096u

uint8_t ReadFile(char *path, uint8_t *dest){
	uint8_t e = 1;
	FILINFO finfo;
	FRESULT fres;
	UINT bytes_read = 0;
	e = 0;
//	uint8_t filbuf[200];

	if ((path == NULL) || (dest == NULL)) {
		return 1;
	}

	fres = f_open(&fil, (const TCHAR *)path, FA_READ);
	if (fres != FR_OK) {
		printf((char *)"f_open error\r\n");
		e = 1;
	}else{
		fres = f_stat((const TCHAR *)path, &finfo);
		if ((fres != FR_OK) || (finfo.fsize >= FILE_READ_BUFFER_SIZE)) {
			e = 1;
		}else{
			fres = f_read(&fil, dest, finfo.fsize, &bytes_read);
			if ((fres != FR_OK) || (bytes_read != finfo.fsize)) {
				e = 1;
			}else{
				dest[bytes_read] = '\0';
			}
		}
	}
	f_close(&fil);
	return e;
}

//*************************************************************
//
//*************************************************************
uint8_t WriteFile(char *path, char *message){
	uint8_t e = -1;
	UINT bytes_written, bw;
	FRESULT fres;
//	CSP_QSPI_DisableMemoryMappedMode();
	fres = f_open(&fil, path, FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS);
//	fres = f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (fres == FR_OK) {
		// Escreve os dados no arquivo
		bw = strlen(message);
		fres = f_write(&fil, message, bw, &bytes_written);
		if (fres == FR_OK && bytes_written == strlen(message)) {
			e = 0;
//			printf("Dados escritos com sucesso: \n");
//		} else {
//			printf("Erro ao escrever no arquivo: %d\n", fres);
		}
		f_close(&fil);  // Fecha o arquivo
	}
//	else {
//		printf("Open file error \n");
//	}
//	CSP_QSPI_EnableMemoryMappedMode();
	return e;
}

uint8_t DeleteFile(char* path)
{
    if (path == NULL || path[0] == '\0') {
        return FR_INVALID_NAME;
    }

    // Exemplo de path: "0:/logs/teste.txt"
    FRESULT fr = f_unlink(path);

    // (Opcional) log
//    if (fr == FR_OK) {
//        printf("Arquivo deletado: %s\r\n", path);
//    } else {
//        printf("Erro ao deletar (%s): %d\r\n", path, (int)fr);
//    }

    return fr;
}

