/**
  ******************************************************************************
  * @file    quadspi_is.h
  * @brief   Single header for ISSI IS25LP128 QSPI driver (STM32H7).
  *
  * This header intentionally concentrates:
  *  - All IS25LP128 opcodes/geometry/timings
  *  - Status register bits
  *  - Driver public prototypes
  *
  * Target:
  *  - STM32H743 (HAL QSPI)
  *  - IS25LP128 in Quad-SPI (QSPI controller in indirect mode and memory-mapped)
  ******************************************************************************
  */

#ifndef __QUADSPI_IS_H__
#define __QUADSPI_IS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"  /* brings in HAL + QSPI handle + project clock setup */
#include "diskio.h"
/* --------------------------- Memory geometry ------------------------------ */
#define IS25LP128_FLASH_SIZE_BYTES           (16U * 1024U * 1024U) /* 128Mbit */
#define IS25LP128_PAGE_SIZE                  256U
#define IS25LP128_SUBSECTOR_SIZE             4096U   /* 4KB */
#define IS25LP128_SECTOR_SIZE_64K            65536U  /* 64KB */

#define REAL_SECTOR							0x90800000 / IS25LP128_SUBSECTOR_SIZE
#define QSPI_FS_OFFSET						0x90800000

/* ------------------------------ Opcodes ----------------------------------- */
/* Write/Erase */
#define IS25LP128_CMD_WRITE_ENABLE           0x06
#define IS25LP128_CMD_WRITE_DISABLE          0x04

#define IS25LP128_CMD_SUBSECTOR_ERASE        0x20    /* 4KB erase */
#define IS25LP128_CMD_SECTOR_ERASE_64K       0xD8    /* 64KB erase */
#define IS25LP128_CMD_BULK_ERASE             0xC7    /* or 0x60 on some flashes */

#define IS25LP128_CMD_PAGE_PROGRAM           0x02
#define IS25LP128_CMD_QUAD_PAGE_PROGRAM      0x32    /* QPP: 1-1-4 */
#define IS25LP128_CMD_QPP					 0x32

/* Read */
#define IS25LP128_CMD_READ_ID_JEDEC          0x9F
#define IS25LP128_CMD_READ_STATUS            0x05
#define IS25LP128_CMD_WRITE_STATUS           0x01

#define IS25LP128_CMD_FAST_READ              0x0B
#define IS25LP128_CMD_QUAD_READ              0x6B    /* Quad Output Fast Read: 1-1-4 */
#define IS25LP128_CMD_QUAD_IO_READ           0xEB    /* Quad I/O Fast Read: 1-4-4 */

/* Reset / mode */
#define IS25LP128_CMD_RESET_ENABLE           0x66
#define IS25LP128_CMD_RESET_MEMORY           0x99
#define IS25LP128_CMD_EXIT_QPI               0xF5

/* ------------------------- Status Register bits --------------------------- */
/* SR bit mapping for IS25LP128 (RDSR 0x05): */
#define IS25LP128_SR_WIP                     (1U << 0) /* Write In Progress */
#define IS25LP128_SR_WEL                     (1U << 1) /* Write Enable Latch */
#define IS25LP128_SR_BP0                     (1U << 2)
#define IS25LP128_SR_BP1                     (1U << 3)
#define IS25LP128_SR_BP2                     (1U << 4)
#define IS25LP128_SR_BP3                     (1U << 5)
#define IS25LP128_SR_QE                      (1U << 6) /* Quad Enable */
#define IS25LP128_SR_SRP0                    (1U << 7)

/* ------------------------------ Timings ----------------------------------- */
/*
 * Your setup:
 *  - QSPI kernel clock = 192 MHz
 *  - Prescaler = 1  => QSPI clock = 96 MHz
 *
 * For IS25LP128 at ~96 MHz:
 *  - 0xEB (1-4-4) typically uses 6 dummy cycles (plus 8 "mode" bits via alternate bytes)
 *  - 0x6B (1-1-4) typically uses 8 dummy cycles
 */
#define IS25LP128_DUMMY_CYCLES_QUAD_IO_READ   6U  /* for 0xEB */
#define IS25LP128_DUMMY_CYCLES_QUAD_READ      8U  /* for 0x6B */

/* Maximum times (ms) for timeouts/polling (kept conservative) */
#define IS25LP128_BULK_ERASE_MAX_TIME         90000U  /* ~90s max */
#define IS25LP128_SECTOR_ERASE_MAX_TIME       1000U   /* 64KB */
#define IS25LP128_SUBSECTOR_ERASE_MAX_TIME    300U    /* 4KB */

/* If project didn't define a default QSPI timeout, define one. */
#ifndef HAL_QPSI_TIMEOUT_DEFAULT_VALUE
#define HAL_QPSI_TIMEOUT_DEFAULT_VALUE        5000U
#endif

/* --------------------------- Public prototypes ---------------------------- */
extern QSPI_HandleTypeDef hqspi;

uint8_t CSP_QUADSPI_Init(void);
uint8_t CSP_QSPI_Erase_Chip(void);
uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
uint8_t CSP_QSPI_Erase_Block(uint32_t BlockAddress);
uint8_t CSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
uint8_t CSP_QSPI_Write_FS(uint8_t* pData, uint32_t WriteAddr, uint32_t Size, uint32_t Offset);
uint8_t CSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
uint8_t CSP_QSPI_Read_FS(uint8_t* pData, uint32_t ReadAddr, uint32_t Size, uint32_t Offset);
uint8_t CSP_QSPI_EnableMemoryMappedMode(void);
uint8_t CSP_QSPI_DisableMemoryMappedMode(void);
DRESULT QSPI_IOCTL(BYTE drv, BYTE ctrl, void *buff);
uint8_t CSP_QSPI_EraseSector_FS(uint32_t EraseStartAddress);
uint8_t QSPI_AutoPollingMemReady(void);
uint8_t QSPI_IsMemoryMapped(void);

#ifdef __cplusplus
}
#endif

#endif /* __QUADSPI_IS_H__ */
