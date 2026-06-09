/******************************************************************************
* Copyright (c) 2018(-2025) STMicroelectronics.
* All rights reserved.
*
* This file is part of the TouchGFX 4.26.0 distribution.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
*******************************************************************************/

#include <platform/driver/touch/SDL2TouchController.hpp>
#include <platform/hal/simulator/sdl2/HALSDL2.hpp>
#include <touchgfx/hal/HAL.hpp>
//#include "gt911.h"
//#include "drv_gt911.h"

namespace touchgfx
{
void SDL2TouchController::init()
{
}

bool SDL2TouchController::sampleTouch(int32_t& x, int32_t& y)
{

//	GT911_State_t p;
//	if(DRV_GT911_ST_ReadTouch(&p)){
//		x = p.x;
//		y = p.y;
//		return true;
//	}
//
//	return false;

    return static_cast<HALSDL2*>(HAL::getInstance())->doSampleTouch(x, y);
}
} // namespace touchgfx
