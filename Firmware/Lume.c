#include "Lume.h"
#include "common.h"
#include "led_io.h"
#include "delay_util.h"
#include "luminocity.h"
#include <avr/sleep.h>
#include <util/atomic.h>

// ============================ Types & variables ==============================
struct {
    uint8_t Hour;
    uint8_t HyperMinute;
    uint8_t Second;
} Time;

struct {
    bool MenuPressed, UpPressed, DownPressed;
    uint16_t Timer;
} Keys;

enum {ModeRegular, ModeSetHours, ModeSetMinutes} Mode;

// ============================ Implementation =================================
int main (void) {
    GeneralInit ();
    sei();
    while (1) {  // forever
        if(POWER_OK()) {    // Handle tasks only if power is ok. Time is counted in interrupt.
            TASK_Toggle();
            TASK_Keys();
            TASK_Lumi();
        } // If Power ok
        else {  // power failure, enter sleep mode
            KeysShutdown();
            LumiShutdown();
            LedIOShutdown();
            // Enter sleep
            set_sleep_mode(SLEEP_MODE_PWR_SAVE);    // Left timer2 enabled
            sleep_enable();
            do {
                sleep_cpu();
            } while(!POWER_OK());
            // Power restored
            KeysInit();
            LumiInit();
            LedIOreinit();

            Mode = ModeRegular;

            EVENT_NewHour();
            EVENT_NewHyperMinute();
        }
    }	// while 1
}

FORCE_INLINE void GeneralInit(void) {
    // Light control
    LedIOInit();
    LumiInit();

    // Power ok
    PWROK_DDR  &= ~(1<<PWROK_P);
    PWROK_PORT &= ~(1<<PWROK_P);

    KeysInit();

    // Timer2: realtime clock counter
    TCCR2A = 0;
    ASSR  = (1<<AS2);				// Set Async mode of the timer
    //TCCR2B = (0<<CS22)|(0<<CS21)|(1<<CS20);	// DEBUG: no division
    //TCCR2B = (0<<CS22)|(1<<CS21)|(0<<CS20);	// DEBUG: div 8
    //TCCR2B = (0<<CS22)|(1<<CS21)|(1<<CS20);	// DEBUG: div 32
    TCCR2B = (1<<CS22)|(0<<CS21)|(1<<CS20);	// RELEASE: div 128, once per second
    TIMSK2 |= (1<<TOIE2);

    // Setup initial values
    Time.Second = 1;
    Time.HyperMinute = 0;
    Time.Hour = 1;

    Mode = ModeRegular;

    // Start-up time setup
    EVENT_NewHour();
    EVENT_NewHyperMinute();
}

void KeysInit(void) {
    KEYS_DDR  &= ~((1<<KEY_MENU)|(1<<KEY_UP)|(1<<KEY_DOWN));    // Keys are inputs
    KEYS_PORT |=   (1<<KEY_MENU)|(1<<KEY_UP)|(1<<KEY_DOWN);     // Pull-ups on
    Keys.DownPressed = false;
    Keys.MenuPressed = false;
    Keys.UpPressed   = false;
    DelayReset(&Keys.Timer);
}
FORCE_INLINE void KeysShutdown(void) {
    KEYS_PORT &= ~((1<<KEY_MENU)|(1<<KEY_UP)|(1<<KEY_DOWN));
    KEYS_DDR  |=   (1<<KEY_MENU)|(1<<KEY_UP)|(1<<KEY_DOWN);
}

// =============================== Tasks =======================================
// Checks if need to toggle something
void TASK_Toggle(void) {
    // Toggle minutes & hours PWM
    TogglePWM(&LControl.Min0PWM);
    TogglePWM(&LControl.Min1PWM);
    TogglePWM(&LControl.Hr0PWM);
    TogglePWM(&LControl.Hr1PWM);
}

void TASK_Keys(void) {
    if (!DelayElapsed(&Keys.Timer, KEYS_POLL_TIME)) return;

    // ******** Up key *******
    if(!Keys.UpPressed && KEY_UP_PRESSED()) {       // Keypress occured
        Keys.UpPressed = true;
        EVENT_KeyUpPressed();
    }
    else if(Keys.UpPressed && !KEY_UP_PRESSED()) {  // Depress occured
        Keys.UpPressed = false;
    }

    // ******** Down key *******
    if(!Keys.DownPressed && KEY_DOWN_PRESSED()) {       // Keypress occured
        Keys.DownPressed = true;
        EVENT_KeyDownPressed();
    }
    else if(Keys.DownPressed && !KEY_DOWN_PRESSED()) {  // Depress occured
        Keys.DownPressed = false;
    }

    // ******** Menu key *******
    if(!Keys.MenuPressed && KEY_MENU_PRESSED()) {       // Keypress occured
        Keys.MenuPressed = true;
        EVENT_KeyMenuPressed();
    }
    else if (Keys.MenuPressed && !KEY_MENU_PRESSED()) { // Depress occured
        Keys.MenuPressed = false;
    }
}

// ============================ Events =========================================
void EVENT_NewHour(void) {
    uint8_t HCurrent, HPrev;
    LControl.HByte &= 0b11000000;   // Clear hours bits
    HoursOff();                     // Shutdown both PWM channels
    // Here is switching logic
    HCurrent = Time.Hour;
    HPrev = (HCurrent == 0)? 11 : HCurrent-1;
    switch(Mode) {
        case ModeRegular:   
            SetupHour(HCurrent, LED_Rise);
            SetupHour(HPrev,    LED_Fade);
            break;
        case ModeSetHours:
            SetupHour(HCurrent, LED_Blink);
            break;
        default:
            SetupHour(HCurrent, LED_On);
            break;
    } // switch
    WriteControlBytes();    // Write bytes to setup LEDs
    HoursOn();              // Switch on needed PWM channels
}
// Switch on needed minute channels, setup their start PWM and pepare to change
void EVENT_NewHyperMinute(void) {
    uint8_t MCurrent, MPrev;
    LControl.HByte &= 0b00111111;   // Clear minutes bits
    LControl.MByte = 0;             // Clear minutes byte
    MinutesOff();                   // Shutdown both PWM channels
    // Here is switching logic
    MCurrent = Time.HyperMinute;
    MPrev = (MCurrent == 0)? 23 : MCurrent-1;
    switch(Mode) {
        case ModeRegular:
            SetupMinute(MCurrent, LED_Rise);
            SetupMinute(MPrev,    LED_Fade);
            break;
        case ModeSetMinutes:
            SetupMinute(MCurrent, LED_Blink);
            break;
        default:
            SetupMinute(MCurrent, LED_On);
            break;
    } // switch
    WriteControlBytes();    // Write bytes to setup LEDs
    MinutesOn();            // Switch on needed PWM channels
}

// Keys events
void EVENT_KeyUpPressed(void) {
    // Behave depending on current mode
    switch (Mode) {
        case ModeSetHours:
            if(Time.Hour == 11) Time.Hour = 0;
            else Time.Hour++;
            EVENT_NewHour();
            break;
        case ModeSetMinutes:
            Time.HyperMinute++;
            if(Time.HyperMinute > 23) Time.HyperMinute = 0;
            EVENT_NewHyperMinute();
            break;
        case ModeRegular: // Change brightness of dark and light modes
            if(Lumi.ItsDark) {
                if(LControl.PWMDark < PWM_MAX) LControl.PWMDark += PWM_STEP;
                LControl.PWM_TopValue = LControl.PWMDark;
            }
            else {
                if(LControl.PWMLight < PWM_MAX) LControl.PWMLight += PWM_STEP;
                LControl.PWM_TopValue = LControl.PWMLight;
            }
            break;
    } // switch
}
void EVENT_KeyDownPressed(void) {
    // Behave depending on current mode
    switch (Mode) {
        case ModeSetHours:
            if(Time.Hour == 0) Time.Hour = 11;
            else Time.Hour--;
            EVENT_NewHour();
            break;
        case ModeSetMinutes:
            if(Time.HyperMinute == 0) Time.HyperMinute = 23;
            else Time.HyperMinute--;
            EVENT_NewHyperMinute();
            break;
        case ModeRegular: // Change brightness of dark and light modes
            if(Lumi.ItsDark) {
                if(LControl.PWMDark > PWM_MIN) LControl.PWMDark -= PWM_STEP;
                LControl.PWM_TopValue = LControl.PWMDark;
            }
            else {
                if(LControl.PWMLight > PWM_MIN) LControl.PWMLight -= PWM_STEP;
                LControl.PWM_TopValue = LControl.PWMLight;
            }
            break;
    } // switch
}
void EVENT_KeyMenuPressed(void) {
    // Behave depending on current mode
    switch (Mode) {
        case ModeRegular:   // Enter SetHours mode
            Mode = ModeSetHours;
            break;
        case ModeSetHours:  // Enter SetMinutes mode
            Mode = ModeSetMinutes;
            break;
        case ModeSetMinutes:    // Return to regular
            Mode = ModeRegular;
            break;
    } // switch
    EVENT_NewHour();
    EVENT_NewHyperMinute();
}

// Luminocity change event
void EVENT_LumiChanged(void) {
    if(Lumi.ItsDark) LControl.PWM_TopValue = LControl.PWMDark;
    else             LControl.PWM_TopValue = LControl.PWMLight;
}

// =========================== Interrupts ======================================
// Time counter
ISR(TIMER2_OVF_vect) {
    // Get out if in mode of settings
    if (Mode != ModeRegular) return;

    Time.Second++;
    if(Time.Second > 150) {    // 150 seconds in one hyperminute
        Time.Second = 1;
        Time.HyperMinute++;
        if(Time.HyperMinute > 23) { // 24 HyperMinutes in one hour
        //if(Time.HyperMinute > 3) { // DEBUG: 4 HyperMinutes in one hour
//        if(Time.HyperMinute > 11) { // DEBUG: HyperMinutes as hours
            Time.HyperMinute = 0;
            Time.Hour++;
            if(Time.Hour > 11) Time.Hour = 0;
            if (POWER_OK()) EVENT_NewHour();
        } // if Hyperminute > 23
        if (POWER_OK()) EVENT_NewHyperMinute(); // Call after EVENT_NewHour to write correct control bytes
    }// Second
}
