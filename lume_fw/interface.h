/*
 * interface.h
 *
 *  Created on: 22 марта 2015 г.
 *      Author: Kreyl
 */

#pragma once

#include "lcd1200.h"
#include "kl_time.h"
#include "TimeSettings.h"

class Interface_t {
public:
	void Reset() {
		Lcd.Printf(0, 0, "                ");
		Lcd.Printf(0, 1, "                ");
		Lcd.Printf(0, 2, "                ");
		Lcd.Printf(0, 3, "                ");
		Lcd.Printf(0, 4, "                ");
		Lcd.Printf(0, 5, "                ");
		Lcd.Printf(0, 6, "                ");
	}

    void ResetFirstScreen() {
        Lcd.Printf(0, 0, "Время:  00:00:00");
        Lcd.Printf(0, 1, "Дата: 0000/00/00");
    }

    void ResetSecondScreen() {
    	Lcd.Printf(0, 0, "Цвет часов:  000");
    	Lcd.Printf(0, 1, "Цвет минут:  000");
    	Lcd.Printf(0, 2, "Background:   00");
    	Lcd.Printf(0, 3, "Порог осв-ти: 00");
    	Lcd.Printf(0, 4, "Освещённость: 00");
        Lcd.Printf(0, 5, "Яркость выше: 00");
        Lcd.Printf(0, 6, "Яркость ниже: 00");
        }

    void DisplayDateTime() {
        DisplayHours();
        DisplayMinutes();
        DisplaySeconds();
        DisplayYear();
        DisplayDay();
        DisplayMonth();
    }

    void DisplaySecondScreen(uint32_t Lum) {
    	DisplayColorHour();
    	DisplayColorMin();
    	DisplayBackground();
    	DisplayLum(Lum);
    	DisplayThreshold();
    	DisplayBrHigh();
    	DisplayBrLow();
    }

    void DisplayHours() {
    	Lcd.Printf(8,  0, "%02u", Time.Curr.H);
    }

    void DisplayHoursInverted() {
       	Lcd.PrintfInverted(8,  0, "%02u", Time.Curr.H);
    }

    void DisplayMinutes() {
       	Lcd.Printf(11,  0, "%02u", Time.Curr.M);
    }

   void DisplayMinutesInverted() {
      	Lcd.PrintfInverted(11,  0, "%02u", Time.Curr.M);
   }

   void DisplayDay() {
    	Lcd.Printf(14,  1, "%02u", Time.Curr.Day);
   }

   void DisplayDayInverted() {
      	Lcd.PrintfInverted(14,  1, "%02u", Time.Curr.Day);
   }

   void DisplayMonth() {
       	Lcd.Printf(11,  1, "%02u", Time.Curr.Month);
   }

   void DisplayMonthInverted() {
       	Lcd.PrintfInverted(11,  1, "%02u", Time.Curr.Month);
   }

   void DisplayYear() {
       	Lcd.Printf(6,  1, "%02u", Time.Curr.Year);
   }

   void DisplayYearInverted() {
       	Lcd.PrintfInverted(6,  1, "%02u", Time.Curr.Year);
   }

   void DisplaySeconds() {
        Lcd.Printf(14, 0, "%02u", Time.Curr.S);
    }

    void DisplayLum(uint32_t Lum) {
        Lcd.Printf(14, 4, "%02u", Lum);
    }

    void DisplayThreshold() {
        Lcd.Printf(14, 3, "%02u", Settings.Threshold);
    }

    void DisplayThresholdInverted() {
            Lcd.PrintfInverted(14, 3, "%02u", Settings.Threshold);
        }

    void DisplayBrHigh() {
        Lcd.Printf(14, 5, "%02u", Settings.BrtHi);
    }

    void DisplayBrHighInverted() {
            Lcd.PrintfInverted(14, 5, "%02u", Settings.BrtHi);
        }

    void DisplayBrLow() {
        Lcd.Printf(14, 6, "%02u", Settings.BrtLo);
    }

    void DisplayBrLowInverted() {
            Lcd.PrintfInverted(14, 6, "%02u", Settings.BrtLo);
        }

    void DisplayColorHour() {
        Lcd.Printf(13, 0, "%03u", Settings.ClrIdH);
    }

    void DisplayColorHourInverted() {
            Lcd.PrintfInverted(13, 0, "%03u", Settings.ClrIdH);
        }

    void DisplayColorMin() {
        Lcd.Printf(13, 1, "%03u", Settings.ClrIdM);
    }

    void DisplayColorMinInverted() {
            Lcd.PrintfInverted(13, 1, "%03u", Settings.ClrIdM);
        }

    void DisplayBackground() {
    	Lcd.Printf(14, 2, "%02u", Settings.BckGrnd);
    }

    void DisplayBackgroundInverted() {
        	Lcd.PrintfInverted(14, 2, "%02u", Settings.BckGrnd);
    }

    void Error(const char* msg) { Lcd.PrintfInverted(0, 0, "%S", msg); }
};

extern Interface_t Interface;
