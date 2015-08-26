#include "luminocity.h"
#include <util/atomic.h>
#include "delay_util.h"
#include "Lume.h"
#include "common.h"

struct Lumi_t Lumi;

void LumiInit(void) {
    LUMI_DDR  &= ~(1<<LUMI_P);
    LUMI_PORT &= ~(1<<LUMI_P);
    Lumi.ADCState = ADCNoMeasure;
    DelayReset(&Lumi.Timer);
}
FORCE_INLINE void LumiShutdown(void) {
    ADC_DISABLE();
    ADC_REF_DISABLE();
}

void TASK_Lumi(void) {
    switch(Lumi.ADCState) {
        case ADCNoMeasure:
            if (DelayElapsed(&Lumi.Timer, ADC_MEASURE_PERIOD)) { // Check if timeout has passed
                ADC_REF_ENABLE();           // Start ADC
                ADC_START_MEASUREMENT();    // Start new measure - dummy one for the first time
                Lumi.ADCState = ADCInit;
            }
            break;
        case ADCInit:
            // Check if dummy measurement completed & timeout passed
            if (ADC_MEASUREMENT_COMPLETED() && DelayElapsed(&Lumi.Timer, ADC_PREPARE_TIMEOUT)) {
                ADC_START_MEASUREMENT();    // Start new measure
                Lumi.ADCValue = 0;
                Lumi.MeasuresCounter = 0;
                Lumi.ADCState = ADCMeasuring;
            }
            break;
        case ADCMeasuring:
            if (ADC_MEASUREMENT_COMPLETED()) {  // Check if current measurement completed
                Lumi.MeasuresCounter++;
                ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                    Lumi.ADCValue += ADCH;     // Add ADC result
                } // atomic
                // Check if series of measurements is completed
                if (Lumi.MeasuresCounter >= ADC_NUMBER_OF_MEASURES) {
                    // Disable ADC to save energy
                    ADC_DISABLE();
                    ADC_REF_DISABLE();
                    // Prepare result
                    Lumi.ADCValue >>= ADC_POWER_OF_MEASURES;
                    Lumi.ADCState = ADCNoMeasure;
                    DelayReset(&Lumi.Timer);
                    EVENT_ADCMeasureCompleted();
                }
                // if not, start measurement again
                else ADC_START_MEASUREMENT();
            } // if current measurement completed
            break;
    } // switch
}

void EVENT_ADCMeasureCompleted(void) {
    if((Lumi.ADCValue >  BRIGHTNESS_TRESHOLD) && !Lumi.ItsDark) {   // Darkness fell upon the watch
        Lumi.ItsDark = true;
        EVENT_LumiChanged();
    }
    else if ((Lumi.ADCValue <= BRIGHTNESS_TRESHOLD) &&  Lumi.ItsDark) { // Light poured upon the watch again
        Lumi.ItsDark = false;
        EVENT_LumiChanged();
    }
}