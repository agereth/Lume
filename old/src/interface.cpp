/*
 * interface.cpp of Armlet1 project
 *
 *  Created on: 15.09.2012
 *      Author: kreyl
 */

#include "interface.h"
#include "keys.h"
#include "showtime.h"

Interface_t Interface;

void Interface_t::IInc(bool AFast, int32_t *PValue, int32_t AMinValue, int32_t AMaxValue) {
    if(AFast and ((*PValue) %10 == 0)) (*PValue) +=10;
    else (*PValue)++;
    if((*PValue) > AMaxValue) (*PValue) = AMinValue;
}
void Interface_t::IDec(bool AFast, int32_t *PValue, int32_t AMinValue, int32_t AMaxValue) {
    if(AFast and ((*PValue) %10 == 0)) (*PValue) -=10;
    else (*PValue)--;
    if((*PValue) < AMinValue) (*PValue) = AMaxValue;
}

void Interface_t::Task() {
    static uint32_t IBlinkTmr=0, IStateTmr=0;
    static bool IsDisplayed;
    static uint8_t OldH=27, OldM=99;    // Dummy values for first time
    switch(State) {
        case msIdle:
            // Display time if needed
            if(Time.SecElapsed(&TimeChangeTmr, 1)) {
                Time.GetDateTime(&IDateTime);
                DisplayHour(true);
                DisplayMinute(true);
                DisplaySecond(true);
                DisplayYear(true);
                DisplayMonth(true);
                DisplayDay(true);
                ShowTime.ShowMinute(IDateTime.S);
                // Check if showtime has come
                if(IDateTime.M != OldM) {
                    OldM = IDateTime.M;
                    //ShowTime.ShowMinute(IDateTime.M);
                }
                if(IDateTime.H != OldH) {
                   OldH = IDateTime.H;
                   ShowTime.ShowHour(IDateTime.H);
               }
            }
            // Handle keys
            if(Keys.AnyKeyWasJustPressed()) {
                Lcd.Backlight(72);
                Delay.Reset(&BckLtTmr);
            }
            if(Keys.Enter.WasJustPressed()) {
                Keys.Up.ResetPresses();
                Keys.Down.ResetPresses();
                State = msHour;
                IsDisplayed = true;
                Delay.Reset(&IBlinkTmr);
                Delay.Reset(&IStateTmr);    // to return to idle
            }
            // Handle backlight
            if(Delay.Elapsed(&BckLtTmr, BCKLT_DELAY)) Lcd.Backlight(0);
            break;

        case msHour:
            // Handle autoexit
            if(Delay.Elapsed(&IStateTmr, STATE_DELAY)) { EnterIdle(); break; }
            // Handle keys
            if(Keys.Down.WasJustPressed())    IDec(false, &IDateTime.H, 0, 23);
            else if(Keys.Up.WasJustPressed()) IInc(false, &IDateTime.H, 0, 23);
            else if(Keys.Enter.WasJustPressed()) {
                IDateTime.S = 0;
                DisplaySecond(true);
                State = msMinute;
            }
            if(Keys.AnyKeyWasJustPressed()) {
                Delay.Reset(&IStateTmr);
                Delay.Reset(&IBlinkTmr);
                DisplayHour(IsDisplayed = true);
            }
            // Handle blink
            else if(!Keys.Down.IsPressed() and !Keys.Up.IsPressed() and Delay.Elapsed(&IBlinkTmr, BLINK_DELAY))
                DisplayHour(IsDisplayed = !IsDisplayed);   // Blink the value
            break;

        case msMinute:
            // Handle autoexit
            if(Delay.Elapsed(&IStateTmr, STATE_DELAY)) { EnterIdle(); break; }
            // Handle keys
            if(Keys.Down.WasJustPressed())       IDec(false, &IDateTime.M, 0, 59);
            else if(Keys.Up.WasJustPressed())    IInc(false, &IDateTime.M, 0, 59);
            else if(Keys.Enter.WasJustPressed()) State = msYear;
            if(Keys.AnyKeyWasJustPressed()) {
                Delay.Reset(&IStateTmr);
                Delay.Reset(&IBlinkTmr);
                DisplayMinute(IsDisplayed = true);
            }
            // Handle blink
            else if(!Keys.Down.IsPressed() and !Keys.Up.IsPressed() and Delay.Elapsed(&IBlinkTmr, BLINK_DELAY))
                DisplayMinute(IsDisplayed = !IsDisplayed);   // Blink the value
            break;

        case msYear:
            // Handle autoexit
            if(Delay.Elapsed(&IStateTmr, STATE_DELAY)) { EnterIdle(); break; }
            // Handle keys
            if(Keys.Down.WasJustPressed())       IDec(false, &IDateTime.Year, YEAR_MIN, YEAR_MAX);
            else if(Keys.Up.WasJustPressed())    IInc(false, &IDateTime.Year, YEAR_MIN, YEAR_MAX);
            else if(Keys.Enter.WasJustPressed()) State = msMonth;
            if(Keys.AnyKeyWasJustPressed()) {
                Delay.Reset(&IStateTmr);
                Delay.Reset(&IBlinkTmr);
                DisplayYear(IsDisplayed = true);
            }
            // Handle blink
            else if(!Keys.Down.IsPressed() and !Keys.Up.IsPressed() and Delay.Elapsed(&IBlinkTmr, BLINK_DELAY))
                DisplayYear(IsDisplayed = !IsDisplayed);   // Blink the value
            break;

        case msMonth:
            // Handle autoexit
            if(Delay.Elapsed(&IStateTmr, STATE_DELAY)) { EnterIdle(); break; }
            // Handle keys
            if(Keys.Down.WasJustPressed())       IDec(false, &IDateTime.Month, 1, 12);
            else if(Keys.Up.WasJustPressed())    IInc(false, &IDateTime.Month, 1, 12);
            else if(Keys.Enter.WasJustPressed()) State = msDay;
            if(Keys.AnyKeyWasJustPressed()) {
                Delay.Reset(&IStateTmr);
                Delay.Reset(&IBlinkTmr);
                DisplayMonth(IsDisplayed = true);
            }
            // Handle blink
            else if(!Keys.Down.IsPressed() and !Keys.Up.IsPressed() and Delay.Elapsed(&IBlinkTmr, BLINK_DELAY))
                DisplayMonth(IsDisplayed = !IsDisplayed);   // Blink the value
            break;

        case msDay:
            // Handle autoexit
            if(Delay.Elapsed(&IStateTmr, STATE_DELAY)) { EnterIdle(); break; }
            // Handle keys
            if(Keys.Down.WasJustPressed())       IDec(false, &IDateTime.Day, 1, MonthDays[LEAPYEAR(IDateTime.Year)][IDateTime.Month-1]);
            else if(Keys.Up.WasJustPressed())    IInc(false, &IDateTime.Day, 1, MonthDays[LEAPYEAR(IDateTime.Year)][IDateTime.Month-1]);
            else if(Keys.Enter.WasJustPressed()) EnterIdle();
            if(Keys.AnyKeyWasJustPressed()) {
                Delay.Reset(&IStateTmr);
                Delay.Reset(&IBlinkTmr);
                DisplayDay(IsDisplayed = true);
            }
            // Handle blink
            else if(!Keys.Down.IsPressed() and !Keys.Up.IsPressed() and Delay.Elapsed(&IBlinkTmr, BLINK_DELAY))
                DisplayDay(IsDisplayed = !IsDisplayed);   // Blink the value
            break;
    }
}

void Interface_t::EnterIdle() {
    DisplayHour(true);
    DisplayMinute(true);
    DisplaySecond(true);
    DisplayYear(true);
    DisplayMonth(true);
    DisplayDay(true);
    State = msIdle;
    Time.SetDateTime(IDateTime);
    Delay.Reset(&TimeChangeTmr);  // Without this it could wait very long before displaying next second
    Delay.Reset(&BckLtTmr);
}

void Interface_t::Init() {
    // Draw background
    Lcd.Printf(0, 2, "Time:  ");
    Lcd.Printf(0, 4, "Year:  ");
    Lcd.Printf(0, 5, "Month: ");
    Lcd.Printf(0, 6, "Day:   ");

    Time.GetDateTime(&IDateTime);
    DisplayHour(true);
    DisplayMinute(true);
    DisplaySecond(true);
    DisplayYear(true);
    DisplayMonth(true);
    DisplayDay(true);

    State = msIdle;
}
