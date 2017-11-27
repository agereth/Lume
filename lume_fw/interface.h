/*
 * interface.h
 *
 *  Created on: 22 ����� 2015 �.
 *      Author: Kreyl
 */

#pragma once

#include <EventHandlers.h>
#include "lcd1200.h"
#include "kl_time.h"

class Interface_t {
public:

    void Reset() {
            Lcd.Printf(0, 0, "�����:  00:00:00");
            Lcd.Printf(0, 1, "����: 0000/00/00");
            Lcd.Printf(0, 2, "������������: 00");
            Lcd.Printf(0, 3, "����� ���-��: 00");
            Lcd.Printf(0, 4, "������� ����: 00");
            Lcd.Printf(0, 5, "������� ����: 00");
            Lcd.Printf(0, 6, "���� �����:  000");
            Lcd.Printf(0, 7, "���� �����:  000");
        }

    void DisplayDateTime() {
        Lcd.Printf(8,  0, "%02u", Time.Curr.H);
        Lcd.Printf(11, 0, "%02u", Time.Curr.M);
        Lcd.Printf(14, 0, "%02u", Time.Curr.S);  // do not touch seconds
        Lcd.Printf(6,  1, "%04u", Time.Curr.Year);
        Lcd.Printf(11, 1, "%02u", Time.Curr.Month);
        Lcd.Printf(14, 1, "%02u", Time.Curr.Day);
    }

    void DisplayLum(uint32_t Lum) {
        Lcd.Printf(14, 2, "%02u", Lum);
    }

    void DisplayThreshold() {
        Lcd.Printf(14, 3, "%02u", Settings.Threshold);
    }
    void DisplayBrtHi() {
        Lcd.Printf(14, 4, "%02u", Settings.BrtHi);
    }
    void DisplayBrtLo() {
        Lcd.Printf(14, 5, "%02u", Settings.BrtLo);
    }

    void DisplayClrH() {
        Lcd.Printf(13, 6, "%03u", Settings.ClrIdH);
    }
    void DisplayClrM() {
        Lcd.Printf(13, 7, "%03u", Settings.ClrIdM);
    }

    void DisplayNotTimeSettings(uint32_t Lum) {
            DisplayLum(Lum);
            DisplayThreshold();
            DisplayBrtHi();
            DisplayBrtLo();
            DisplayClrH();
            DisplayClrM();

        }


    void Error(const char* msg) { Lcd.PrintfInverted(0, 0, "%S", msg); }
};

extern Interface_t Interface;
