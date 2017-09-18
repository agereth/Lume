/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "hal.h"
#include "MsgQ.h"
#include "uart.h"
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
#include "IntelLedEffs.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
void OnCmd(Shell_t *PShell);
void ITask();

// Settings
union Settings_t {
    struct {
        uint32_t R1,R2,R3;
    } __packed;
    struct {
        uint8_t Threshold;
        uint8_t BrtHi;
        uint8_t BrtLo;
        uint16_t ClrIdH, ClrIdM;
    } __packed;
} __packed;

Settings_t Settings;
Interface_t Interface;

//ColorHSV_t hsv(319, 100, 100);
//PinOutput_t PwrPin { PWR_EN_PIN };
//TmrKL_t TmrAdc {MS2ST(450), evtIdEverySecond, tktPeriodic};
//Profile_t Profile;

//bool AdcFirstConv = true;
State_t State = stIdle;
bool DateTimeHasChanged = false;
//ColorHSV_t ClrM(hsvYellow), ClrH(hsvCyan);

TmrKL_t TmrMenu {MS2ST(3600), evtIdMenuTimeout, tktOneShot};

enum Btns_t {btnUp=0, btnDown=1, btnPlus=2, btnMinus=3};

static void MenuHandler(Btns_t Btn);
static void EnterIdle();

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
    LedEffectsInit();
    EffAllTogetherNow.SetupAndStart(clBlue);

    chThdSleepMilliseconds(180); // Let power to stabilize
    Lcd.Init();

    // Time and backup space
    BackupSpc::EnableAccess();
    // Load settings
    Settings.R1 = RTC->BKP1R;
    Settings.R2 = RTC->BKP2R;
    Settings.R3 = RTC->BKP3R;

    Interface.Reset();
    EnterIdle();

    Time.Init();

    // Adc
//    PinSetupAnalog(BAT_MEAS_PIN);
//    Adc.Init();
//    Adc.EnableVRef();
//    TmrAdc.InitAndStart();
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
                if(State == stIdle) {
                    Time.GetDateTime();
//                Time.CurrentDT.Print();
                    Interface.DisplayDateTime(&Time.CurrentDT);
                }
                break;

            case evtIdButtons:
                Printf("Btn %u\r", Msg.BtnEvtInfo.BtnID);
                MenuHandler((Btns_t)Msg.BtnEvtInfo.BtnID);
                break;

            case evtIdMenuTimeout:
                Printf("MenuTimeout\r");
                EnterIdle();
                break;
            default: break;
        } // switch

#if ADC_REQUIRED
        if(Evt & EVT_SAMPLING) Adc.StartMeasurement();
        if(Evt & EVT_ADC_DONE) {
            if(AdcFirstConv) AdcFirstConv = false;
            else {
                uint32_t VBat_adc = Adc.GetResult(ADC_BAT_CHNL);
                uint32_t VRef_adc = Adc.GetResult(ADC_VREFINT_CHNL);
                __unused int32_t Vbat_mv = (2 * Adc.Adc2mV(VBat_adc, VRef_adc));   // Resistor divider
//                Uart.Printf("VBat_adc: %u; Vref_adc: %u; VBat_mv: %u\r", VBat_adc, VRef_adc, Vbat_mv);
//                if(Vbat_mv < 3600) SignalEvt(EVT_BATTERY_LOW);
            } // if not big diff
        } // evt
#endif
    } // while true
} // ITask()

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
                    Interface.DisplayDateTime(&Time.CurrentDT);
                    break;
                case btnUp:
                    State = stClrM;
                    Interface.DisplayClrM(Settings.ClrIdM);
                    break;
                default: break; // do not react on +-
            }
            break;



            case stClrM:
                switch(Btn) {
                    case btnDown: EnterIdle(); break;
                    case btnUp:
                        State = stClrH;
                        Interface.DisplayClrM(Settings.ClrIdM);
                        Interface.DisplayClrM(Settings.ClrIdH);
                        break;
                    case btnPlus:
                        if(Settings.ClrIdM == 360) Settings.ClrIdM = 0;
                        else Settings.ClrIdM++;
                        Interface.DisplayClrM(Settings.ClrIdM);
                        break;
                    case btnMinus:
                        if(Settings.ClrIdM == 0) Settings.ClrIdM = 360;
                        else Settings.ClrIdM--;
                        Interface.DisplayClrM(Settings.ClrIdM);
                        break;
                }
                break;
    } // switch state
}

void EnterIdle() {
    State = stIdle;
    Interface.DisplayDateTime(&Time.CurrentDT);
    Interface.DisplayClrH(Settings.ClrIdH);
    Interface.DisplayClrM(Settings.ClrIdM);
    Lcd.Backlight(0);
    // Save settings
    RTC->BKP1R = Settings.R1;
    RTC->BKP2R = Settings.R2;
    RTC->BKP3R = Settings.R3;
    // Save time if changed
    if(DateTimeHasChanged) {
        // XXX
    }
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
//    else if(PCmd->NameIs("HSL")) {
//        ColorHSL_t ClrHsl(0,0,0);
//        if(PCmd->GetNextUint16(&ClrHsl.H) != OK) return;
//        if(PCmd->GetNextByte(&ClrHsl.S)   != OK) return;
//        if(PCmd->GetNextByte(&ClrHsl.L)   != OK) return;
//        Color_t Clr;
//        ClrHsl.ToRGB(Clr);
////        Led.SetColor(Clr, 100);
//        PShell->Ack(OK);
//    }

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
        EffAllTogetherNow.SetupAndStart(FClr);
//        EffAllTogetherSmoothly.SetupAndStart(FClr, 360);
        PShell->Ack(retvOk);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
