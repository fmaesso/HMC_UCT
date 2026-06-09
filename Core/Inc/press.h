/*
 * press.h
 *
 *  Created on: 10 de mar. de 2026
 *      Author: ferna
 */

#ifndef INC_PRESS_H_
#define INC_PRESS_H_
#include "cec.h"

#define MOVING_AVG_SIZE 50

void Sensor_CalibrateZero(TPressao *p, adc_type raw_adc_ptr);
//void Sensor_CalibrateZero(TPressao *p);


#endif /* INC_PRESS_H_ */
