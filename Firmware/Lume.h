#ifndef _LUME_H
#define	_LUME_H

// ============================ Includes =======================================
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/sleep.h>
#include <stdbool.h>

// ========================== Ports & pins =====================================
// Power ok
#define PWROK_DDR   DDRC
#define PWROK_PORT  PORTC
#define PWROK_PIN   PINC
#define PWROK_P     PC3
#define POWER_OK()  bit_is_set (PWROK_PIN, PWROK_P)

// Keys
#define KEYS_DDR    DDRC
#define KEYS_PORT   PORTC
#define KEYS_PIN    PINC
#define KEY_MENU    PC1
#define KEY_UP      PC2
#define KEY_DOWN    PC0

#define KEYS_POLL_TIME 504

#define KEY_UP_PRESSED()    bit_is_clear(KEYS_PIN, KEY_UP)
#define KEY_DOWN_PRESSED()  bit_is_clear(KEYS_PIN, KEY_DOWN)
#define KEY_MENU_PRESSED()  bit_is_clear(KEYS_PIN, KEY_MENU)

// ========================== Functions prototypes =============================
void GeneralInit (void);

void KeysInit(void);
void KeysShutdown(void);

// Tasks
void TASK_Toggle(void);
void TASK_Keys(void);

// Events
void EVENT_NewHour(void);
void EVENT_NewHyperMinute(void);

void EVENT_KeyUpPressed(void);
void EVENT_KeyDownPressed(void);
void EVENT_KeyMenuPressed(void);

void EVENT_LumiChanged(void);

#endif