#include "delay_util.h"

volatile uint16_t TickCounter;

bool DelayElapsed(uint16_t *AVar, const uint16_t ADelay) {
    bool Result = false;
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        if ((TickCounter - *AVar) >= ADelay) {
            *AVar = TickCounter;   // Reset delay
            Result = true;
        }
    } // Atomic
    return Result;
}

void DelayReset(uint16_t *AVar) {
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        *AVar = TickCounter;
    }
}

// ================================ Interrupts =================================
// Delay counter
ISR(TIMER0_OVF_vect) {
    TickCounter++;
}
