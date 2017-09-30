/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "hal.h"
#include "MsgQ.h"
#include "shell.h"
#include "kl_lib.h"
#include "SimpleSensors.h"
#include "buttons.h"
#include "lcd1200.h"
#include "led.h"
#include "kl_time.h"
#include "interface.h"
#include "main.h"
#include "ws2812b.h"
#include "Mirilli.h"
#include "kl_adc.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
void OnCmd(Shell_t *PShell);
void ITask();

Settings_t Settings;
Interface_t Interface;
State_t State = stIdle;
bool DateTimeHasChanged = false;

TmrKL_t TmrMenu {MS2ST(3600), evtIdMenuTimeout, tktOneShot};

enum Btns_t {btnUp=0, btnDown=1, btnPlus=2, btnMinus=3};

static Hypertime_t Hypertime;
ColorHSV_t ClrActiveH(144, 100, 100), ClrPassiveH(144, 0, 0);
ColorHSV_t ClrActiveM(144, 100, 100), ClrPassiveM(144, 0, 0);

static void MenuHandler(Btns_t Btn);
static void EnterIdle();
static void IndicateNewSecond(bool ForceIndicate = false);

#endif

int main(void) {
    // ==== Init Clock system ====
//    Clk.SetupFlashLatency(16);
//    Clk.SetupPLLDividers(1, pllMul4, plsHSIdiv2);
//    Clk.SwitchTo(csPLL);
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();

    // ==== Init hardware ====
    EvtQMain.Init();
    Uart.Init(115200);
    Printf("\r%S %S\r", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    SimpleSensors::Init();

    // LEDs: must be set up before LCD to allow DMA irq
    InitMirilli();

    chThdSleepMilliseconds(180); // Let power to stabilize
    PinSetupOut(LCD_PWR, omPushPull);
    PinSetHi(LCD_PWR);
    chThdSleepMilliseconds(18);
    Lcd.Init();

    // Time and backup space
    BackupSpc::EnableAccess();
    // Load settings
    Settings.R1 = RTC->BKP1R;
    Settings.R2 = RTC->BKP2R;
    Settings.R3 = RTC->BKP3R;

    ClrActiveH.H = Settings.ClrIdH;
    ClrActiveM.H = Settings.ClrIdM;

    Interface.Reset();
    EnterIdle();

    Time.Init();

    // Adc
    PinSetupAnalog(LUM_MEAS_PIN);
    Adc.Init();
    Adc.EnableVRef();
    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            case evtIdEverySecond:
//                Printf("RTC: %X\r", RTC->TR);
                if(State == stIdle) {
                    Time.GetDateTime();
//                Time.CurrentDT.Print();
                    Interface.DisplayDateTime();
                }
                IndicateNewSecond();
                break;

            case evtIdAdcRslt: {
                uint32_t Lum = Msg.Value / 36;
//                Printf("Lum: %u\r", Msg.Value);
                Interface.DisplayLum(Lum);
                } break;

            case evtIdButtons:
//                Printf("Btn %u\r", Msg.BtnEvtInfo.BtnID);
                MenuHandler((Btns_t)Msg.BtnEvtInfo.BtnID);
                break;

            case evtIdMenuTimeout:
                Printf("MenuTimeout\r");
                EnterIdle();
                break;
            default: break;
        } // switch
    } // while true
} // ITask()

void IndicateNewSecond(bool ForceIndicate) {
    Hypertime.ConvertFromTime();
    if(Hypertime.NewH or ForceIndicate) {
        if(Hypertime.H == 0) {
            SetTargetClrH(11, ClrPassiveH);
            SetTargetClrH(0, ClrActiveH);
            SetTargetClrH(1, ClrPassiveH);
        }
        else {
            SetTargetClrH(Hypertime.H, ClrActiveH);
            SetTargetClrH(Hypertime.H-1, ClrPassiveH);
        }
    }
    if(Hypertime.NewM or ForceIndicate) {
        if(Hypertime.M == 0) {
            SetTargetClrM(1, ClrPassiveM);
            SetTargetClrM(0, ClrActiveM);
            SetTargetClrM(11, ClrActiveM);
            SetTargetClrM(10, ClrPassiveM);
        }
        else if(Hypertime.M == 23) {
            SetTargetClrM(10, ClrPassiveM);
            SetTargetClrM(11, ClrActiveM);
            SetTargetClrM(0, ClrPassiveM);
        }
        else {
            uint32_t N = Hypertime.M / 2;
            if(Hypertime.M & 1) { // Odd, single
                SetTargetClrM(N, ClrActiveM);
                if(N == 0) SetTargetClrM(11, ClrPassiveM);
                else SetTargetClrM(N-1, ClrPassiveM);
            }
            else { // Even, couple
                SetTargetClrM(N, ClrActiveM);
                SetTargetClrM(N+1, ClrPassiveM); // hide next in case of time going back
            }
        }
    }
    if(Hypertime.NewH or Hypertime.NewM or ForceIndicate) {
        Hypertime.NewH = false;
        Hypertime.NewM = false;
        WakeMirilli();
    }
}

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

void MenuHandler(Btns_t Btn) {
    // Switch backlight on
    Lcd.Backlight(100);
    TmrMenu.StartOrRestart();
    // Process menu
    switch(State) {
        case stIdle:
            switch(Btn) {
                case btnDown:
                    State = stHours;
                    Interface.DisplayDateTime();
                    break;
                case btnUp:
                    State = stClrM;
                    Interface.DisplayClrM();
                    break;
                default: break; // do not react on +-
            }
            break;

#if 1 // ==== Time ====
        case stMinutes:
            switch(Btn) {
                case btnDown: State = stYear; break;
                case btnUp: State = stHours; break;
                case btnPlus:  Time.Curr.IncM(); Time.Curr.S = 0; DateTimeHasChanged = true; break;
                case btnMinus: Time.Curr.DecM(); Time.Curr.S = 0; DateTimeHasChanged = true; break;
            }
            Interface.DisplayDateTime();
            break;

        case stHours:
            switch(Btn) {
                case btnDown: State = stMinutes; break;
                case btnUp: EnterIdle(); break;
                case btnPlus:  Time.Curr.IncH(); DateTimeHasChanged = true; break;
                case btnMinus: Time.Curr.DecH(); DateTimeHasChanged = true; break;
            }
            Interface.DisplayDateTime();
            break;

#endif

#if 1 // ==== Date ====
        case stDay:
            switch(Btn) {
                case btnDown:
                    State = stThreshold;
                    Interface.DisplayThreshold();
                    break;
                case btnUp: State = stMonth; break;
                case btnPlus:  Time.Curr.IncDay(); DateTimeHasChanged = true; break;
                case btnMinus: Time.Curr.DecDay(); DateTimeHasChanged = true; break;
            }
            Interface.DisplayDateTime();
            break;

        case stMonth:
            switch(Btn) {
                case btnDown: State = stDay; break;
                case btnUp: State = stYear; break;
                case btnPlus:  Time.Curr.IncMonth(); DateTimeHasChanged = true; break;
                case btnMinus: Time.Curr.DecMonth(); DateTimeHasChanged = true; break;
            }
            Interface.DisplayDateTime();
            break;

        case stYear:
            switch(Btn) {
                case btnDown: State = stMonth; break;
                case btnUp: State = stMinutes; break;
                case btnPlus:  Time.Curr.IncYear(); DateTimeHasChanged = true; break;
                case btnMinus: Time.Curr.DecYear(); DateTimeHasChanged = true; break;
            }
            Interface.DisplayDateTime();
            break;
#endif

#if 1 // ==== Brightness threshold ====
        case stThreshold:
            switch(Btn) {
                case btnDown:
                    State = stBrtHi;
                    Interface.DisplayBrtHi();
                    break;
                case btnUp:
                    State = stDay;
                    Interface.DisplayDateTime();
                    break;
                case btnPlus:
                    if(Settings.Threshold == 99) Settings.Threshold = 0;
                    else Settings.Threshold++;
                    break;
                case btnMinus:
                    if(Settings.Threshold == 0) Settings.Threshold = 99;
                    else Settings.Threshold--;
                    break;
            }
            Interface.DisplayThreshold();
            break;

        case stBrtHi:
            switch(Btn) {
                case btnDown:
                    State = stBrtLo;
                    Interface.DisplayBrtLo();
                    break;
                case btnUp:
                    State = stThreshold;
                    Interface.DisplayThreshold();
                    break;
                case btnPlus:
                    if(Settings.BrtHi == 99) Settings.BrtHi = 0;
                    else Settings.BrtHi++;
                    break;
                case btnMinus:
                    if(Settings.BrtHi == 0) Settings.BrtHi = 99;
                    else Settings.BrtHi--;
                    break;
            }
            Interface.DisplayBrtHi();
            break;

        case stBrtLo:
            switch(Btn) {
                case btnDown:
                    State = stClrH;
                    Interface.DisplayClrH();
                    break;
                case btnUp:
                    State = stBrtHi;
                    Interface.DisplayBrtHi();
                    break;
                case btnPlus:
                    if(Settings.BrtLo == 99) Settings.BrtLo = 0;
                    else Settings.BrtLo++;
                    break;
                case btnMinus:
                    if(Settings.BrtLo == 0) Settings.BrtLo = 99;
                    else Settings.BrtLo--;
                    break;
            }
            Interface.DisplayBrtLo();
            break;
#endif

#if 1 // ==== Colors ====
        case stClrH:
            switch(Btn) {
                case btnDown:
                    State = stClrM;
                    Interface.DisplayClrM();
                    break;
                case btnUp:
                    State = stBrtLo;
                    Interface.DisplayBrtLo();
                    break;
                case btnPlus:
                    if(Settings.ClrIdH == 360) Settings.ClrIdH = 0;
                    else Settings.ClrIdH++;
                    ClrActiveH.H = Settings.ClrIdH;
                    IndicateNewSecond(true);    // Show new color
                    break;
                case btnMinus:
                    if(Settings.ClrIdH == 0) Settings.ClrIdH = 360;
                    else Settings.ClrIdH--;
                    ClrActiveH.H = Settings.ClrIdH;
                    IndicateNewSecond(true);    // Show new color
                    break;
            }
            Interface.DisplayClrH();
            break;

        case stClrM:
            switch(Btn) {
                case btnDown: EnterIdle(); break;
                case btnUp:
                    State = stClrH;
                    Interface.DisplayClrH();
                    break;
                case btnPlus:
                    if(Settings.ClrIdM == 360) Settings.ClrIdM = 0;
                    else Settings.ClrIdM++;
                    ClrActiveM.H = Settings.ClrIdM;
                    IndicateNewSecond(true);    // Show new color
                    break;
                case btnMinus:
                    if(Settings.ClrIdM == 0) Settings.ClrIdM = 360;
                    else Settings.ClrIdM--;
                    ClrActiveM.H = Settings.ClrIdM;
                    IndicateNewSecond(true);    // Show new color
                    break;
            }
            Interface.DisplayClrM();
            break;
#endif
    } // switch state
}

void EnterIdle() {
    State = stIdle;
    TmrMenu.Stop();
    Interface.DisplayDateTime();
    Interface.DisplayThreshold();
    Interface.DisplayBrtHi();
    Interface.DisplayBrtLo();
    Interface.DisplayClrH();
    Interface.DisplayClrM();
    Lcd.Backlight(0);
    // Save settings
    RTC->BKP1R = Settings.R1;
    RTC->BKP2R = Settings.R2;
    RTC->BKP3R = Settings.R3;
    // Save time if changed
    if(DateTimeHasChanged) Time.SetDateTime();
//    Time.EnableIrq();
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

    else if(PCmd->NameIs("t")) {
        Printf("%u\r", RTC->TR);
    }
    else if(PCmd->NameIs("i")) {
        EXTI->SWIER |= EXTI_SWIER_SWIER20;
    }

    else if(PCmd->NameIs("HSV")) {
//        ColorHSV_t ClrHsv(0,0,0);
//        if(PCmd->GetNextUint16(&ClrHsv.H) != retvOk) return;
//        if(PCmd->GetNextByte(&ClrHsv.S)   != OK) return;
//        if(PCmd->GetNextByte(&ClrHsv.V)   != OK) return;
//        Color_t Clr;
//        ClrHsv.ToRGB(Clr);
////        Clr.Print();
////        Uart.Printf("{%u; %u; %u}\r", R, G, B);
////        Led.SetColor(Clr, 100);
//        PShell->Ack(OK);
    }

    else if(PCmd->NameIs("RGB")) {
        Color_t FClr(0,0,0);
        if(PCmd->GetNext<uint8_t>(&FClr.R) != retvOk) return;
        if(PCmd->GetNext<uint8_t>(&FClr.G) != retvOk) return;
        if(PCmd->GetNext<uint8_t>(&FClr.B) != retvOk) return;
//        EffAllTogetherNow.SetupAndStart(FClr);
//        EffAllTogetherSmoothly.SetupAndStart(FClr, 360);
        PShell->Ack(retvOk);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
