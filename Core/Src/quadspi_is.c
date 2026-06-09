/**
  ******************************************************************************
  * @file    quadspi.c
  * @brief   This file provides code for the configuration
  *          of the QUADSPI instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "quadspi_is.h"


/*
 * Driver adapted for ISSI IS25LP128 (128Mbit, 16MBytes) in Quad-SPI mode.
 *
 * This project originally used an N25Q/Winbond-oriented header (n25q128a.h)
 * with "Volatile Configuration Register" dummy-cycle programming.
 *
 * For IS25LP128, we:
 *  - Use Status Register (RDSR 0x05 / WRSR 0x01)
 *  - Set QE bit in SR bit6
 *  - Use common SPI/QSPI opcodes (WREN 0x06, RDSR 0x05, PP 0x32, QREAD 0x6B)
 *  - Keep dummy cycles configured on the STM32 side (QSPI peripheral).
 */

/* If you have an IS25LP128 header, include it here. Otherwise we provide
 * minimal command/geometry defines below.
 */
// #include "is25lp128.h"

/* All IS25LP128 opcodes/geometry/timings are in quadspi_is.h */

/* USER CODE BEGIN 0 */
static uint8_t QSPI_WriteEnable(void);

static uint8_t QSPI_Configuration(void);
static uint8_t QSPI_ResetChip(void);

/* USER CODE END 0 */

extern QSPI_HandleTypeDef hqspi;



/* QUADSPI init function */

/* USER CODE BEGIN 1 */

/* QUADSPI init function */

uint8_t CSP_QUADSPI_Init(void)
{
//prepare QSPI peripheral for ST-Link Utility operations
//	hqspi.Instance = QUADSPI;
//    if (HAL_QSPI_DeInit(&hqspi) != HAL_OK) {
//        return HAL_ERROR;
//    }

//    MX_QUADSPI_Init();

    if (QSPI_ResetChip() != HAL_OK) {
        return HAL_ERROR;
    }

    HAL_Delay(1);

    if (QSPI_AutoPollingMemReady() != HAL_OK) {
        return HAL_ERROR;
    }

    if (QSPI_WriteEnable() != HAL_OK) {

        return HAL_ERROR;
    }

    if (QSPI_Configuration() != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t QSPI_IsMemoryMapped(void)
{
    uint32_t fmode = (QUADSPI->CCR >> QUADSPI_CCR_FMODE_Pos) & 0x3;
    return (fmode == 0x3);  // 0b11 = memory mapped
}

uint8_t CSP_QSPI_Erase_Chip(void)
{
    QSPI_CommandTypeDef sCommand;


    if (QSPI_WriteEnable() != HAL_OK) {
        return HAL_ERROR;
    }


    /* Erasing Sequence --------------------------------- */
    sCommand.Instruction = IS25LP128_CMD_BULK_ERASE;
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.AddressMode = QSPI_ADDRESS_NONE;
    sCommand.Address = 0;
    sCommand.DataMode = QSPI_DATA_NONE;
    sCommand.DummyCycles = 0;


    if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
        != HAL_OK) {
        return HAL_ERROR;
    }

    if (QSPI_AutoPollingMemReady() != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t QSPI_AutoPollingMemReady(void)
{
    QSPI_CommandTypeDef     cmd = {0};
    QSPI_AutoPollingTypeDef cfg = {0};

    /* Poll WIP=0 using RDSR (0x05) */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = IS25LP128_CMD_READ_STATUS;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.NbData            = 1;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    cfg.Match           = 0x00;
    cfg.Mask            = IS25LP128_SR_WIP;
    cfg.MatchMode       = QSPI_MATCH_MODE_AND;
    cfg.StatusBytesSize = 1;
    cfg.Interval        = 0x10;
    cfg.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling(&hqspi, &cmd, &cfg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


static uint8_t QSPI_WriteEnable(void)
{
    QSPI_CommandTypeDef     cmd = {0};
    QSPI_AutoPollingTypeDef cfg = {0};

    /* Ensure not in memory-mapped mode */
    (void)HAL_QSPI_Abort(&hqspi);

    /* WREN (0x06) */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = IS25LP128_CMD_WRITE_ENABLE;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Poll WEL=1 using RDSR (0x05) */
    cmd.Instruction = IS25LP128_CMD_READ_STATUS;
    cmd.DataMode    = QSPI_DATA_1_LINE;
    cmd.NbData      = 1;

    cfg.Match           = IS25LP128_SR_WEL;
    cfg.Mask            = IS25LP128_SR_WEL;
    cfg.MatchMode       = QSPI_MATCH_MODE_AND;
    cfg.StatusBytesSize = 1;
    cfg.Interval        = 0x10;
    cfg.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling(&hqspi, &cmd, &cfg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}



/*Enable quad mode and set dummy cycles count*/
uint8_t QSPI_Configuration(void)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t sr = 0;

    /* Read Status Register (0x05) */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = IS25LP128_CMD_READ_STATUS;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.NbData            = 1;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;
    if (HAL_QSPI_Receive(&hqspi, &sr, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if (sr & IS25LP128_SR_QE)
        return HAL_OK;

    sr |= IS25LP128_SR_QE;

    if (QSPI_WriteEnable() != HAL_OK)
        return HAL_ERROR;

    cmd.Instruction = IS25LP128_CMD_WRITE_STATUS; /* 0x01 */

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;
    if (HAL_QSPI_Transmit(&hqspi, &sr, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if (QSPI_AutoPollingMemReady() != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}


uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{

    QSPI_CommandTypeDef sCommand;

    /* Align to 4KB erase boundary */
    EraseStartAddress = EraseStartAddress
                        - (EraseStartAddress % IS25LP128_SUBSECTOR_SIZE);

    /* Erasing Sequence -------------------------------------------------- */
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.Instruction = IS25LP128_CMD_SUBSECTOR_ERASE; /* 4KB */
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE;

    sCommand.DataMode = QSPI_DATA_NONE;
    sCommand.DummyCycles = 0;

    while (EraseEndAddress >= EraseStartAddress)
    {
        sCommand.Address = (EraseStartAddress & 0x0FFFFFFF);

        if (QSPI_WriteEnable() != HAL_OK) {
            return HAL_ERROR;
        }

        if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
            != HAL_OK) {
            return HAL_ERROR;
        }
        EraseStartAddress += IS25LP128_SUBSECTOR_SIZE;

        if (QSPI_AutoPollingMemReady() != HAL_OK) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

uint8_t CSP_QSPI_EraseSector_FS(uint32_t EraseStartAddress)
{

    QSPI_CommandTypeDef sCommand;

    /* Align to 4KB erase boundary */
    EraseStartAddress = EraseStartAddress;// - (EraseStartAddress % IS25LP128_SUBSECTOR_SIZE);

    EraseStartAddress = EraseStartAddress * IS25LP128_SUBSECTOR_SIZE;

    /* Erasing Sequence -------------------------------------------------- */
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.Instruction = IS25LP128_CMD_SUBSECTOR_ERASE; /* 4KB */
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE;

    sCommand.DataMode = QSPI_DATA_NONE;
    sCommand.DummyCycles = 0;

//    while (EraseEndAddress >= EraseStartAddress)
    {
        sCommand.Address = (EraseStartAddress & 0x0FFFFFFF);

        if (QSPI_WriteEnable() != HAL_OK) {
            return HAL_ERROR;
        }

        if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
            != HAL_OK) {
            return HAL_ERROR;
        }
        EraseStartAddress += IS25LP128_SUBSECTOR_SIZE;

        if (QSPI_AutoPollingMemReady() != HAL_OK) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}




//uint8_t CSP_QSPI_EnableMemoryMappedMode(void)
//{
//    QSPI_CommandTypeDef      cmd  = {0};
//    QSPI_MemoryMappedTypeDef mmap = {0};
//
//    /* 0xEB Quad I/O Fast Read (1-4-4) with 8 mode bits (alternate bytes) */
//    cmd.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
//    cmd.Instruction         = IS25LP128_CMD_QUAD_IO_READ;
//
//    cmd.AddressMode         = QSPI_ADDRESS_4_LINES;
//    cmd.AddressSize         = QSPI_ADDRESS_24_BITS;
//    cmd.Address             = 0;
//
//    cmd.AlternateByteMode   = QSPI_ALTERNATE_BYTES_4_LINES;
//    cmd.AlternateBytesSize  = QSPI_ALTERNATE_BYTES_8_BITS;
//    cmd.AlternateBytes      = 0x08;
//
//    cmd.DataMode            = QSPI_DATA_4_LINES;
//    cmd.NbData              = 0;
//
//    cmd.DummyCycles         = IS25LP128_DUMMY_CYCLES_QUAD_IO_READ;
//
//    cmd.DdrMode             = QSPI_DDR_MODE_DISABLE;
//    cmd.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
//    cmd.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;
//
//    mmap.TimeOutActivation  = QSPI_TIMEOUT_COUNTER_DISABLE;
//
//    if (HAL_QSPI_MemoryMapped(&hqspi, &cmd, &mmap) != HAL_OK)
//        return HAL_ERROR;
//
//    return HAL_OK;
//}
//


uint8_t CSP_QSPI_EnableMemoryMappedMode(void)
{
    QSPI_CommandTypeDef      cmd  = {0};
    QSPI_MemoryMappedTypeDef mmap = {0};

    /* 0xEB Quad I/O Fast Read (1-4-4) with 8 mode bits (alternate bytes) */
    cmd.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction         = IS25LP128_CMD_QUAD_IO_READ;

    cmd.AddressMode         = QSPI_ADDRESS_4_LINES;
    cmd.AddressSize         = QSPI_ADDRESS_24_BITS;
    cmd.Address             = 0;

    cmd.AlternateByteMode   = QSPI_ALTERNATE_BYTES_4_LINES;
    cmd.AlternateBytesSize  = QSPI_ALTERNATE_BYTES_8_BITS;
    cmd.AlternateBytes      = 0x08;

    cmd.DataMode            = QSPI_DATA_4_LINES;
    cmd.NbData              = 0;

    cmd.DummyCycles         = 4;	//IS25LP128_DUMMY_CYCLES_QUAD_IO_READ;

    cmd.DdrMode             = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;

    mmap.TimeOutActivation  = QSPI_TIMEOUT_COUNTER_DISABLE;

    if (HAL_QSPI_MemoryMapped(&hqspi, &cmd, &mmap) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

uint8_t CSP_QSPI_DisableMemoryMappedMode(void)
{
    if (HAL_QSPI_Abort(&hqspi) != HAL_OK)
        return HAL_ERROR;
    return HAL_OK;
}



uint8_t QSPI_ResetChip(void)
{
    QSPI_CommandTypeDef cmd = {0};

    (void)HAL_QSPI_Abort(&hqspi);

    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Try Exit QPI (0xF5) in 1-line and 4-line */
    cmd.Instruction       = IS25LP128_CMD_EXIT_QPI;

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);

    cmd.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    (void)HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);

    /* Reset Enable (0x66) */
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = IS25LP128_CMD_RESET_ENABLE;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    /* Reset (0x99) */
    cmd.Instruction       = IS25LP128_CMD_RESET_MEMORY;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}



uint8_t CSP_QSPI_Erase_Block(uint32_t BlockAddress)
{
  QSPI_CommandTypeDef s_command;

  /* Initialize the erase command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = IS25LP128_CMD_SUBSECTOR_ERASE;
  s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.Address           = BlockAddress;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_NONE;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Enable write operations */
  if (QSPI_WriteEnable() != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Send the command */
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure automatic polling mode to wait for end of erase */
  if (QSPI_AutoPollingMemReady() != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}


uint8_t CSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
  QSPI_CommandTypeDef s_command;
  uint32_t end_addr, current_size, current_addr;

  /* Calculation of the size between the write address and the end of the page */
  current_size = IS25LP128_PAGE_SIZE - (WriteAddr % IS25LP128_PAGE_SIZE);

  /* Check if the size of the data is less than the remaining place in the page */
  if (current_size > Size)
  {
    current_size = Size;
  }

  /* Initialize the adress variables */
  current_addr = WriteAddr;
  end_addr = WriteAddr + Size;

  /* Initialize the program command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = IS25LP128_CMD_QPP; /* 0x32 */
  s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Perform the write page by page */
  do
  {
    s_command.Address = current_addr;
    s_command.NbData  = current_size;

    /* Enable write operations */
    if (QSPI_WriteEnable() != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Configure automatic polling mode to wait for end of program */
    if (QSPI_AutoPollingMemReady() != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Update the address and size variables for next page programming */
    current_addr += current_size;
    pData += current_size;
    current_size = ((current_addr + IS25LP128_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : IS25LP128_PAGE_SIZE;
  } while (current_addr < end_addr);

  return HAL_OK;
}

uint8_t CSP_QSPI_Write_FS(uint8_t* pData, uint32_t WriteAddr, uint32_t Size, uint32_t Offset)
{
  QSPI_CommandTypeDef s_command;
  uint32_t end_addr, current_size, current_addr, wr_addr;
  wr_addr = WriteAddr * IS25LP128_SUBSECTOR_SIZE;
  /* Calculation of the size between the write address and the end of the page */
  current_size = IS25LP128_PAGE_SIZE - (wr_addr % IS25LP128_PAGE_SIZE);

  /* Check if the size of the data is less than the remaining place in the page */
  if (current_size > Size)
  {
    current_size = Size;
  }

  /* Initialize the adress variables */
  current_addr = wr_addr + Offset;
  end_addr = wr_addr + Size;

  /* Initialize the program command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = IS25LP128_CMD_QPP; /* 0x32 */
  s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = 0;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Perform the write page by page */
  do
  {
    s_command.Address = current_addr;
    s_command.NbData  = current_size;

    /* Enable write operations */
    if (QSPI_WriteEnable() != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Configure automatic polling mode to wait for end of program */
    if (QSPI_AutoPollingMemReady() != HAL_OK)
    {
      return HAL_ERROR;
    }

    /* Update the address and size variables for next page programming */
    current_addr += current_size;
    pData += current_size;
    current_size = ((current_addr + IS25LP128_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : IS25LP128_PAGE_SIZE;
  } while (current_addr < end_addr);

  return HAL_OK;
}



uint8_t CSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
  QSPI_CommandTypeDef s_command;

  /* Initialize the read command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = IS25LP128_CMD_QUAD_READ; /* 0x6B */
  s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.Address           = ReadAddr;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = IS25LP128_DUMMY_CYCLES_QUAD_READ;
  s_command.NbData            = Size;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Set S# timing for Read command */
  MODIFY_REG(hqspi.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_3_CYCLE);

  /* Reception of the data */
  if (HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Restore S# timing for nonRead commands */
  MODIFY_REG(hqspi.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_6_CYCLE);

  return HAL_OK;
}


uint8_t CSP_QSPI_Read_FS(uint8_t* pData, uint32_t ReadAddr, uint32_t Size, uint32_t Offset)
{
  QSPI_CommandTypeDef s_command;

  /* Initialize the read command */
  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction       = IS25LP128_CMD_QUAD_READ; /* 0x6B */
  s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
  s_command.Address           = (ReadAddr + Offset) * IS25LP128_SUBSECTOR_SIZE;
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode          = QSPI_DATA_4_LINES;
  s_command.DummyCycles       = IS25LP128_DUMMY_CYCLES_QUAD_READ;
  s_command.NbData            = Size;
  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Set S# timing for Read command */
  MODIFY_REG(hqspi.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_3_CYCLE);

  /* Reception of the data */
  if (HAL_QSPI_Receive(&hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Restore S# timing for nonRead commands */
  MODIFY_REG(hqspi.Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_6_CYCLE);

  return HAL_OK;
}



DRESULT QSPI_IOCTL(BYTE drv, BYTE ctrl, void *buff){
	DRESULT res;
	uint8_t *ptr = buff;

	/* pdrv should be 0 */
	if (drv) return RES_PARERR;
	res = RES_ERROR;

	if (ctrl == CTRL_POWER)
	{
		switch (*ptr){
		case 0:
			res = RES_OK;
			break;
		case 1:
			res = RES_OK;
			break;
		case 2:
			res = RES_OK;		/* Power Check */
			break;
		default:
			res = RES_PARERR;
		}
	}else{
		switch (ctrl){
		case GET_SECTOR_COUNT:
			/* SEND_CSD */
			*(DWORD*) buff = 16384;	//128;		//W25Q.SectorCnt;
			res = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			*(WORD*) buff =  512;		//4096;
			res = RES_OK;
			break;

		case GET_BLOCK_SIZE:
			*(DWORD*) buff = 8;			//4096;	//8192;	//W25Q.SectorCnt;
			res = RES_OK;
			break;

		case CTRL_SYNC:
//			while((SPIF_ReadReg1(pW25Q) & SPIF_STATUS1_BUSY) != 0);
			res = RES_OK;
			break;
		case MMC_GET_CSD:
			/* SEND_CSD */
			res = RES_OK;
			break;
		case MMC_GET_CID:
			/* SEND_CID */
			res = RES_OK;
			break;
		case MMC_GET_OCR:
			/* READ_OCR */
			res = RES_OK;
		default:
			res = RES_PARERR;
		}
	}

	return res;
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
