/*
 * gui_control.c
 *
 *  Created on: 19 de fev. de 2026
 *      Author: ferna
 */


#include "gui_control.h"

static volatile uint8_t gui_paused = 0;

void GUI_Pause(void)
{
    gui_paused = 1;
}

void GUI_Resume(void)
{
    gui_paused = 0;
}

uint8_t GUI_IsPaused(void)
{
    return gui_paused;
}

