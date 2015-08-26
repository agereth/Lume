/* 
 * File:   led_io.h
 * Author: Kreyl Laurelindo
 *
 * Created on 19 Август 2010 г., 22:02
 */

#ifndef LED_IO_H
#define	LED_IO_H

#include <inttypes.h>
#include "common.h"

// Lights
#define L_DDR       DDRD
#define L_PORT      PORTD
#define DATA_IN     PD1
#define LATCH       PD3
#define SRCLK       PD4

#define M_PWM_DDR   DDRD
#define M_PWM_PORT  PORTD
#define M0PWM       PD6
#define M1PWM       PD5
#define H_PWM_DDR   DDRB
#define H_PWM_PORT  PORTB
#define H0PWM       PB1
#define H1PWM       PB2

#define H0PWM_ON()  TCCR1A |= ((1<<COM1A1)|(1<<COM1A0))
#define H1PWM_ON()  TCCR1A |= ((1<<COM1B1)|(1<<COM1B0))
#define M0PWM_ON()  TCCR0A |= ((1<<COM0A1)|(1<<COM0A0))
#define M1PWM_ON()  TCCR0A |= ((1<<COM0B1)|(1<<COM0B0))

#define PWM_MAX     250
#define PWM_MIN     10
#define PWM_STEP    40

// ================================ Types ======================================
enum LEDMode_t {LED_Off, LED_On, LED_Rise, LED_Fade, LED_Blink};
struct PWM_t {
    enum LEDMode_t Mode;
    uint16_t Timer, Delay;
    io_uint8_t *OCRX;
};
struct LControl_t {
    uint8_t HByte, MByte;   // Control bytes
    // PWMs
    uint8_t PWM_TopValue, PWMDark, PWMLight;
    struct PWM_t Min0PWM, Min1PWM, Hr0PWM, Hr1PWM;
};

extern struct LControl_t LControl;

// ================================ Prototypes =================================
void LedIOInit(void);
void LedIOShutdown(void);
void LedIOreinit(void);

void WriteControlBytes(void);

void SetupMinute(uint8_t AMinute, enum LEDMode_t AMode);
void SetupHour(uint8_t AHour, enum LEDMode_t AMode);

void SetupPWM(struct PWM_t *pwm, enum LEDMode_t mode);

void PWM_Off(io_uint8_t *p);

void HoursOff(void);
void HoursOn(void);
void MinutesOff(void);
void MinutesOn(void);

void TogglePWM(struct PWM_t *pwm);

// ================================= Constants =================================
// Timings
#define FADE_DELAY1   99	// Low brightness
//#define PWMDelay2   4	// Mid brightness
//#define PWMDelay3   1	// High brightness

#define HOLD_DELAY  20
#define BLINK_DELAY 702

// Light constants
#define PWMStepOver1	27	// Where to switch to quick PWM change mode
#define PWMStepOver2	45


// Light control
#define M12     0
#define M0_5    1
#define M1      2
#define M1_5    3
#define M2      4
#define M2_5    5
#define M3      6
#define M3_5    7
#define M4      8
#define M4_5    9
#define M5      10
#define M5_5    11
#define M6      12
#define M6_5    13
#define M7      14
#define M7_5    15
#define M8      16
#define M8_5    17
#define M9      18
#define M9_5    19
#define M10     20
#define M10_5   21
#define M11     22
#define M11_5   23


#endif	/* LED_IO_H */

