/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include "main.h"
#include "gui_control.h"
#include "quadspi.h"
#include "quadspi_is.h"

#define QSPI_FS_BASE   			((uint8_t*)0x90800000)
#define SECTOR_SIZE    			4096u
#define FS_SIZE_BYTES  			(8u * 1024u * 1024u)   // seus 8MB
#define SECTOR_COUNT   			(FS_SIZE_BYTES / SECTOR_SIZE)
#define LOGICAL_SECTOR_SIZE    	512u
#define PHYS_SECTOR_SIZE       	4096u
#define LOGICALS_PER_PHYS      	(PHYS_SECTOR_SIZE / LOGICAL_SECTOR_SIZE) // 8
// Offset físico dentro do chip onde começa a partição FS
// (se TouchGFX ocupa os primeiros 8MB do chip de 16MB)
#define FS_FLASH_OFFSET        (8u * 1024u * 1024u)

// Total de setores lógicos do “disco”
#define LOGICAL_SECTOR_COUNT   (FS_SIZE_BYTES / LOGICAL_SECTOR_SIZE)

__attribute__((aligned(32))) static uint8_t phys_buf[PHYS_SECTOR_SIZE];

extern uint8_t ReadFlashBuf[4096];
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;



static inline uint32_t flash_addr_from_lba(DWORD sector_lba)
{
    return FS_FLASH_OFFSET + (uint32_t)(sector_lba * LOGICAL_SECTOR_SIZE);
}

static inline uint32_t phys_base_from_flash_addr(uint32_t flash_addr)
{
    return flash_addr & ~(PHYS_SECTOR_SIZE - 1u);  // alinhado 4KB
}
/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
    Stat = STA_NOINIT;
//    Stat = CSP_QUADSPI_Init();
    Stat &= (DSTATUS)~STA_NOINIT;
    return RES_OK;
//    return Stat;
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
//    Stat = STA_NOINIT;
    (void)pdrv;
    return Stat;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */

//	sector += REAL_SECTOR;
//	uint32_t w25blk = sector / (4096 / 512);
//	uint32_t resto = sector - (8 * w25blk);
//	uint32_t offset = resto * 512;
//	CSP_QSPI_Read_FS((uint8_t *)buff, w25blk, (count * 512), offset);
	DRESULT e;
	if (QSPI_IsMemoryMapped()){
		CSP_QSPI_DisableMemoryMappedMode();
	}
	e = RES_OK;
	if (pdrv != 0) e =  RES_PARERR;
	if (Stat & STA_NOINIT) e =  RES_NOTRDY;
	if ((sector + count) > LOGICAL_SECTOR_COUNT) e =  RES_PARERR;

	if(e == RES_OK){
		uint32_t addr = FS_FLASH_OFFSET + (uint32_t)sector * LOGICAL_SECTOR_SIZE;
		uint32_t len  = (uint32_t)count * LOGICAL_SECTOR_SIZE;

		if (CSP_QSPI_Read(buff, addr, len) != HAL_OK){
			e = RES_ERROR;
		}
	}
	if (!QSPI_IsMemoryMapped()){
		CSP_QSPI_EnableMemoryMappedMode();
	}
	return e;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
  /* USER CODE HERE */

//
//	sector += REAL_SECTOR;
//	uint32_t w25blk = sector / (4096 / 512);
//	CSP_QSPI_Read_FS(ReadFlashBuf, w25blk, IS25LP128_SUBSECTOR_SIZE, 0);
//	uint32_t resto = sector - (8 * w25blk);
//	uint32_t offset = resto * 512;
//	memcpy(&ReadFlashBuf[offset], buff, (count * 512));
//	CSP_QSPI_EraseSector_FS(w25blk);
//	CSP_QSPI_Write_FS(ReadFlashBuf, w25blk, IS25LP128_SUBSECTOR_SIZE, 0);

//	GUI_Pause();
//	GUI_Resume();
//    QSPI_Lock();
//    QSPI_Unlock();

	if (pdrv != 0) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if ((sector + count) > LOGICAL_SECTOR_COUNT) return RES_PARERR;

	// IMPORTANTÍSSIMO: escrever exige sair do memory-mapped
	CSP_QSPI_DisableMemoryMappedMode();

	while (count--)
	{
		uint32_t addr      = FS_FLASH_OFFSET + (uint32_t)sector * LOGICAL_SECTOR_SIZE; // byte addr
		uint32_t phys_base = addr & ~(PHYS_SECTOR_SIZE - 1u);                          // alinhado 4KB
		uint32_t within    = addr - phys_base;                                         // 0..3584

		// 1) lê 4KB
		if (CSP_QSPI_Read(phys_buf, phys_base, PHYS_SECTOR_SIZE) != HAL_OK) {
		  CSP_QSPI_EnableMemoryMappedMode();
		  return RES_ERROR;
		}

		// 2) altera só 512B
		memcpy(&phys_buf[within], buff, LOGICAL_SECTOR_SIZE);

		// 3) apaga 4KB
		if (CSP_QSPI_Erase_Block(phys_base) != HAL_OK) {
		  CSP_QSPI_EnableMemoryMappedMode();
		  return RES_ERROR;
		}

		while(QSPI_AutoPollingMemReady() != HAL_OK);

		// 4) grava 4KB (o driver já faz page program)
		if (CSP_QSPI_Write(phys_buf, phys_base, PHYS_SECTOR_SIZE) != HAL_OK) {
		  CSP_QSPI_EnableMemoryMappedMode();
		  return RES_ERROR;
		}

		buff   += LOGICAL_SECTOR_SIZE;
		sector += 1;
	}

	// volta memory-mapped para TouchGFX
	CSP_QSPI_EnableMemoryMappedMode();
	return RES_OK;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
    DRESULT res = RES_ERROR;
    res = QSPI_IOCTL(0, cmd, buff);
    return res;
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

