#include <avr/io.h>
#include "led_io.h"
#include "common.h"
#include "delay_util.h"

// ============================= Variables =====================================
struct LControl_t LControl;

// Tables of accordance
const uint8_t HTable[12] = {
    0b00101000, // H0 == H12
    0b00000000, // H1
    0b00000000, // H2
    0b00000001, // H3
    0b00001000, // H4
    0b00000010, // H5
    0b00010000, // H6
    0b00000011, // H7
    0b00011000, // H8
    0b00000100, // H9
    0b00100000, // H10
    0b00000101, // H11
};
const uint8_t MTable[24][2] = { // First byte for minutes decoder enable bits, second for enable bits & minute code
    {0b10000000, 0b00101000},   // M12
    {0b00000000, 0b01000000},   // M0.5
    {0b00000000, 0b10000000},   // M1
    {0b00000000, 0b01000001},   // M1.5
    {0b00000000, 0b10001000},   // M2
    {0b00000000, 0b01000010},   // M2.5
    {0b00000000, 0b10010000},   // M3
    {0b00000000, 0b01000011},   // M3.5
    {0b00000000, 0b10011000},   // M4
    {0b00000000, 0b01000100},   // M4.5
    {0b00000000, 0b10100000},   // M5
    {0b00000000, 0b01000101},   // M5.5
    {0b00000000, 0b10101000},   // M6
    {0b01000000, 0b00000000},   // M6.5
    {0b10000000, 0b00000000},   // M7
    {0b01000000, 0b00000001},   // M7.5
    {0b10000000, 0b00001000},   // M8
    {0b01000000, 0b00000010},   // M8.5
    {0b10000000, 0b00010000},   // M9
    {0b01000000, 0b00000011},   // M9.5
    {0b10000000, 0b00011000},   // M10
    {0b01000000, 0b00000100},   // M10.5
    {0b10000000, 0b00100000},   // M11
    {0b01000000, 0b00000101},   // M11.5
};

// =========================== Implementation ==================================
FORCE_INLINE void LedIOInit(void) {
    L_DDR  |=   (1<<DATA_IN)|(1<<LATCH)|(1<<SRCLK);
    L_PORT &= ~((1<<DATA_IN)|(1<<LATCH)|(1<<SRCLK));
    M_PWM_DDR  |= (1<<M0PWM)|(1<<M1PWM);
    H_PWM_DDR  |= (1<<H0PWM)|(1<<H1PWM);
    M_PWM_PORT |= (1<<M0PWM)|(1<<M1PWM);
    H_PWM_PORT |= (1<<H0PWM)|(1<<H1PWM);
    // Hours PWM
    TCCR1A = (0<<WGM11)|(1<<WGM10);
    TCCR1B = (0<<WGM13)|(1<<WGM12)|(0<<CS12)|(0<<CS11)|(1<<CS10);
    // Minutes PWM & delay counter
    TCCR0A = (1<<WGM01)|(1<<WGM00);
    TCCR0B = (0<<WGM02)|(0<<CS02)|(0<<CS01)|(1<<CS00);
    TIMSK0 = (1<<TOIE0);    // Interrupt is in delay_util

    // Serial port setup
    UCSR0B = (1<<TXEN0);    // TX enable, irqs disable
    UCSR0C = (1<<UMSEL01)|(1<<UMSEL00)|(0<<UDORD0)|(0<<UCPHA0)|(0<<UCPOL0); // MSB first, leading edge, idle low
    UBRR0  = 0; // Max speed

    LControl.PWM_TopValue = PWM_MAX;
    LControl.PWMDark      = PWM_MIN;
    LControl.PWMLight     = PWM_MAX;
    
    LControl.Min0PWM.OCRX = &OCR0A;
    LControl.Min1PWM.OCRX = &OCR0B;
    LControl.Hr0PWM.OCRX  = &OCR1AL;
    LControl.Hr1PWM.OCRX  = &OCR1BL;
}
FORCE_INLINE void LedIOShutdown(void) {
    // Power-off ports
    L_PORT &= ~((1<<DATA_IN)|(1<<LATCH)|(1<<SRCLK));
    M_PWM_PORT &= ~((1<<M0PWM)|(1<<M1PWM));
    H_PWM_PORT &= ~((1<<H0PWM)|(1<<H1PWM));
    HoursOff();
    MinutesOff();
    // Disable USART
    UCSR0B = 0;
}
FORCE_INLINE void LedIOreinit(void) {
    M_PWM_PORT |= (1<<M0PWM)|(1<<M1PWM);
    H_PWM_PORT |= (1<<H0PWM)|(1<<H1PWM);
    // Enable USART
    UCSR0B = (1<<TXEN0);    // TX enable, irqs disable
    UCSR0C = (1<<UMSEL01)|(1<<UMSEL00)|(0<<UDORD0)|(0<<UCPHA0)|(0<<UCPOL0); // MSB first, leading edge, idle low
    UBRR0  = 0;     // Max speed
    UCSR0A = 0xFF;  // reset interrupts
}

FORCE_INLINE void WriteControlBytes(void) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        UCSR0A = (1<<TXC0);                     // Reset TXC0 flag
        while (bit_is_clear(UCSR0A, UDRE0));    // Wait until buffer is empty
        UDR0 = LControl.MByte;
        while (bit_is_clear(UCSR0A, UDRE0));    // Wait until buffer is empty
        UDR0 = LControl.HByte;
        // Toggle latch
        while (bit_is_clear(UCSR0A, TXC0));     // Wait until shift register is empty
        L_PORT |=  (1<<LATCH);
        nop();
        L_PORT &= ~(1<<LATCH);
    }
}

void SetupMinute(uint8_t AMinute, enum LEDMode_t AMode) {
    // Setup channel
    LControl.HByte |= MTable[AMinute][0];
    LControl.MByte |= MTable[AMinute][1];
    // Setup PWM
    if(AMinute & 0x01) SetupPWM(&LControl.Min0PWM, AMode);    // even => integers: 1 means M0_5, 2 - M1 and so on
    else               SetupPWM(&LControl.Min1PWM, AMode);
}
void SetupHour(uint8_t AHour, enum LEDMode_t AMode) {
    LControl.HByte |= HTable[AHour];                        // Setup byte
    if(AHour & 0x01) SetupPWM(&LControl.Hr0PWM, AMode);     // Odd: 1, 3, 5...
    else             SetupPWM(&LControl.Hr1PWM, AMode);     // Even: 2, 4, 6...
}

void SetupPWM(struct PWM_t *pwm, enum LEDMode_t mode) {
    pwm->Mode = mode;
    DelayReset(&(pwm->Timer));
    switch(mode) {
        case LED_Rise:
            *pwm->OCRX = 0;
            pwm->Delay = FADE_DELAY1;
            break;
        case LED_Fade:
            *pwm->OCRX = LControl.PWM_TopValue;
            pwm->Delay = FADE_DELAY1;
            break;
        case LED_On:
            *pwm->OCRX = LControl.PWM_TopValue;
            pwm->Delay = HOLD_DELAY;
            break;
        case LED_Blink:
            *pwm->OCRX = LControl.PWM_TopValue;
            pwm->Delay = BLINK_DELAY;
            break;
        default:
            break;
    } // switch
}

FORCE_INLINE void PWM_Off(io_uint8_t *p) {
    if     (p == &OCR0A)  TCCR0A &= ~((1<<COM0A1)|(1<<COM0A0));
    else if(p == &OCR0B)  TCCR0A &= ~((1<<COM0B1)|(1<<COM0B0));
    else if(p == &OCR1AL) TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));
    else if(p == &OCR1BL) TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));
}

FORCE_INLINE void HoursOff(void) {
    TCCR1A = (0<<WGM11)|(1<<WGM10);
    LControl.Hr0PWM.Mode = LED_Off;
    LControl.Hr1PWM.Mode = LED_Off;
}
FORCE_INLINE void HoursOn(void) {
    if(LControl.Hr0PWM.Mode != LED_Off) H0PWM_ON();
    if(LControl.Hr1PWM.Mode != LED_Off) H1PWM_ON();
}
FORCE_INLINE void MinutesOff(void) {
    TCCR0A = (1<<WGM01)|(1<<WGM00);
    LControl.Min0PWM.Mode = LED_Off;
    LControl.Min1PWM.Mode = LED_Off;
}
FORCE_INLINE void MinutesOn(void) {
    if(LControl.Min0PWM.Mode != LED_Off) M0PWM_ON();
    if(LControl.Min1PWM.Mode != LED_Off) M1PWM_ON();
}

void TogglePWM(struct PWM_t *pwm) {
    if(!DelayElapsed(&(pwm->Timer), pwm->Delay)) return;   // Not in time
    // Handle Rise or Fade
    uint8_t Value = *pwm->OCRX;
    switch(pwm->Mode) {
        
        case LED_On:
            if(Value != LControl.PWM_TopValue) {
                if(Value > LControl.PWM_TopValue) Value--;
                else Value++;
                *(pwm->OCRX) = Value;
            }
            break;

        case LED_Rise:
            if(Value < LControl.PWM_TopValue) {
                Value++;
                *pwm->OCRX = Value;
            }
            else {
                pwm->Mode = LED_On;
                pwm->Delay = HOLD_DELAY;
            }
            break;

        case LED_Fade:
            if(Value > 0) {
                Value--;
                *pwm->OCRX = Value;
            }
            else {
                pwm->Mode = LED_Off;
                PWM_Off(pwm->OCRX);
            }
            break;

        case LED_Blink:
            if(Value != 0) Value = 0; // Toggle light
            else Value = LControl.PWM_TopValue;
            *(pwm->OCRX) = Value;
            break;

        default: break;
    } // switch
}
