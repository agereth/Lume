/* 
 * File:   luminocity.h
 * Author: Kreyl Laurelindo
 *
 * Created on 23 Август 2010 г., 14:57
 */

#ifndef LUMINOCITY_H
#define	LUMINOCITY_H

#include <avr/io.h>
#include <stdbool.h>

// ============================== Defines ======================================
#define BRIGHTNESS_TRESHOLD 18 // About 1 V

#define LUMI_P       PC4
#define LUMI_DDR     DDRC
#define LUMI_PORT    PORTC

// Number of measures as power of 2: 0 means 1, 1 means 2, 3 means 8 and so on
// Needed for averaging of values
#define ADC_POWER_OF_MEASURES   3   // Power of 2
#define ADC_NUMBER_OF_MEASURES  (1<<ADC_POWER_OF_MEASURES)

#define ADC_PREPARE_TIMEOUT 200  // Time for ADC to prepare
#define ADC_MEASURE_PERIOD  4000 // Period of battery measurement

#define ADC_MEASUREMENT_COMPLETED() bit_is_clear (ADCSRA, ADSC)
// Enable ADC, no IRQ, CLK/8 = 1 Mhz/8 = 125 kHz
#define ADC_START_MEASUREMENT()     ADCSRA = (1<<ADEN)|(1<<ADSC)|(0<<ADATE)|(0<<ADIE)|(0<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)
#define ADC_DISABLE()               ADCSRA = 0
// Ref=Vcc, left-adj, ADC4
#define ADC_REF_ENABLE()            ADMUX = (0<<REFS1)|(1<<REFS0)|(1<<ADLAR)|(0<<MUX3)|(1<<MUX2)|(0<<MUX1)|(0<<MUX0)
#define ADC_REF_DISABLE()           ADMUX = 0

// ========================= Types & variables =================================
enum ADC_State_t {ADCNoMeasure, ADCInit, ADCMeasuring};
struct Lumi_t {
    uint16_t Timer;
    bool ItsDark;
    uint8_t MeasuresCounter;
    uint16_t ADCValue;
    enum ADC_State_t ADCState;
};
extern struct Lumi_t Lumi;

// ============================== Prototypes ===================================
void LumiInit(void);
void LumiShutdown(void);
void TASK_Lumi(void);

void EVENT_ADCMeasureCompleted(void);


#endif	/* LUMINOCITY_H */

