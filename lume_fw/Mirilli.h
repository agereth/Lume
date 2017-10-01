/*
 * Mirilli.h
 *
 *  Created on: 30 сент. 2017 г.
 *      Author: Kreyl
 */

#pragma once

#include "color.h"

#define SMOOTH_VALUE    360
#define OFF_LUMINOCITY  1       // Set to 0 if "backlight all" is not required

#define MIRILLI_H_CNT   13
#define MIRILLI_M_CNT   12

// Tables of accordance between hours/minutes and LED indxs
static const uint32_t H2LedN[MIRILLI_H_CNT] = { 12, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14};
#define SECOND_0_LED_INDX   13
static const uint32_t M2LedN[MIRILLI_H_CNT] = { 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };

// Do not touch
#define LED_CNT         (MIRILLI_H_CNT + MIRILLI_M_CNT)   // Number of WS2812 LEDs

void InitMirilli();
void SetTargetClrH(uint32_t H, ColorHSV_t Clr);
void SetTargetClrM(uint32_t M, ColorHSV_t Clr);
void WakeMirilli();
void ResetColors(ColorHSV_t ClrH, ColorHSV_t ClrM);
