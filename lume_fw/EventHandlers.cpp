/*
 * hypertime.cpp
 *
 *  Created on: 13 ????. 2017 ?.
 *      Author: juice
 */
#include <EventHandlers.h>
#include "mirilli.h"
#include "color.h"

Hypertime_t Hypertime;

Settings_t Settings;

ColorHSV_t ClrH(144, 100, 100);
ColorHSV_t ClrM(144, 100, 100);
uint32_t CurrentLum = 0;
TmrKL_t TmrMenu {MS2ST(9999), TIMEOUT_SIG, tktOneShot};

void Hypertime_t::ConvertFromTime() {
    // Hours
    int32_t FH = Time.Curr.H;
    if(FH > 11) FH -= 12;
    if(H != FH) {
        H = FH;
        NewH = true;
    }
    // Minutes
    int32_t S = Time.Curr.M * 60 + Time.Curr.S;
    int32_t FMin = S / 150;    // 150s in one hyperminute (== 2.5 minutes)
    if(M != FMin) {
        M = FMin;
        NewM = true;
    }
}

void IndicateNewSecond() {
    Hypertime.ConvertFromTime();
//    Printf("HyperH: %u; HyperM: %u\r", Hypertime.H, Hypertime.M);
    ResetColors(ClrH, ClrM);

    // Calculate brightness
    if(CurrentLum > Settings.Threshold) {
        ClrH.V = Settings.BrtHi;
        ClrM.V = Settings.BrtHi;
    }
    else {
        ClrH.V = Settings.BrtLo;
        ClrM.V = Settings.BrtLo;
    }

    // ==== Process hours ====
    SetTargetClrH(Hypertime.H, ClrH);

    // ==== Process minutes ====
    if(Hypertime.M == 0) {
        SetTargetClrM(0, ClrM);
        SetTargetClrM(11, ClrM);
    }
    else {
        uint32_t N = Hypertime.M / 2;
        if(Hypertime.M & 1) { // Odd, single
            SetTargetClrM(N, ClrM);
        }
        else { // Even, couple
            SetTargetClrM(N, ClrM);
            SetTargetClrM(N-1, ClrM);
        }
    }
    WakeMirilli();
}

void InitSettings() {
		// Load settings
		    Settings.R1 = RTC->BKP1R;
		    Settings.R2 = RTC->BKP2R;
		    Settings.R3 = RTC->BKP3R;

		    // Check settings, setup top brightness if both are empty
		    if(Settings.BrtHi == 0 and Settings.BrtLo == 0) {
		        Settings.BrtHi = TOP_BRIGHTNESS;
		        Settings.BrtLo = TOP_BRIGHTNESS;
		    }
		    ClrH.H = Settings.ClrIdH;
		    ClrM.H = Settings.ClrIdM;
}

#if UART_RX_ENABLED // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("SetTime")) {
        DateTime_t dt = Time.Curr;
        if(PCmd->GetNext<int32_t>(&dt.H) != retvOk) return;
        if(PCmd->GetNext<int32_t>(&dt.M) != retvOk) return;
        Time.Curr = dt;
        Time.SetDateTime();
        IndicateNewSecond();
        PShell->Ack(retvOk);
    }

    else if(PCmd->NameIs("Fast")) {
        Time.BeFast();
        PShell->Ack(retvOk);
    }
    else if(PCmd->NameIs("Norm")) {
        Time.BeNormal();
        PShell->Ack(retvOk);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
