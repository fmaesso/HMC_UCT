/*
 * leds.h
 *
 *  Created on: Jan 2, 2025
 *      Author: fmaes
 */

#ifndef INC_LEDS_H_
#define INC_LEDS_H_

void InitLed(void);
void LedOff(int l);
void LedOn(int l);
void LedChange(int l);
void ExecLed(void);
void Complementa_Led(int l);



#endif /* INC_LEDS_H_ */
