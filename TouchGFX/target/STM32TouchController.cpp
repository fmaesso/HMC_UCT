/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.25.0. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>

#include "gt911.h"
#include "drv_gt911.h"
#include "main.h"
#include "stdbool.h"

extern uint8_t g_irqPending;

void STM32TouchController::init()
{
    /**
     * Initialize touch controller and driver
     *
     */
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    /**
     * By default sampleTouch returns false,
     * return true if a touch has been detected, otherwise false.
     *
     * Coordinates are passed to the caller by reference by x and y.
     *
     * This function is called by the TouchGFX framework.
     * By default sampleTouch is called every tick, this can be adjusted by HAL::setTouchSampleRate(int8_t);
     *
     */
//	static int32_t prev_x = 0, prev_y = 0;
	int32_t	xp, yp;
	bool pp;

	if(g_irqPending){
		if(DRV_GT911_ST_ReadTouch(&xp, &yp, &pp)){
			x = xp;
			y = yp;
//			if ((abs(prev_x - x) > 30) &(abs(prev_y- y) > 30)) {
//				if ((x != 0) & (y != 0))
			if(pp)
				{
//					prev_x = x;
//					prev_y = y;
					return true;
				}
			}
//		}
	}
    return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
