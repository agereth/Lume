/*
 * kl_lib.h
 *
 *  Created on: 18.02.2012
 *      Author: kreyl
 */

#ifndef KL_GPIO_H_
#define KL_GPIO_H_

#include <inttypes.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_dma.h"

// =============================== General =====================================
#define PACKED __attribute__ ((__packed__))
#ifndef countof
#define countof(A)  (sizeof(A)/sizeof(A[0]))
#endif

// Functional types
typedef void(*ftVoid_Void)(void);

// ===================== Single pin manipulations ==============================
/*
 * GPIO_Mode_AIN, GPIO_Mode_IN_FLOATING, GPIO_Mode_IPD, GPIO_Mode_IPU,
 * GPIO_Mode_Out_OD, GPIO_Mode_Out_PP, GPIO_Mode_AF_OD, GPIO_Mode_AF_PP
 */
// Set/clear
static inline void klGpioSetByN     (GPIO_TypeDef *PGpioPort, uint16_t APinNumber) { PGpioPort->BSRR = (uint16_t)(1<<APinNumber); }
static inline void klGpioSetByMsk   (GPIO_TypeDef *PGpioPort, uint16_t APinMask)   { PGpioPort->BSRR = APinMask; }
static inline void klGpioClearByN   (GPIO_TypeDef *PGpioPort, uint16_t APinNumber) { PGpioPort->BRR  = (uint16_t)(1<<APinNumber); }
static inline void klGpioClearByMsk (GPIO_TypeDef *PGpioPort, uint16_t APinMask)   { PGpioPort->BRR  = APinMask; }
static inline void klGpioToggleByN  (GPIO_TypeDef *PGpioPort, uint16_t APinNumber) { PGpioPort->ODR ^= (uint16_t)(1<<APinNumber); }
static inline void klGpioToggleByMsk(GPIO_TypeDef *PGpioPort, uint16_t APinMask)   { PGpioPort->ODR ^= APinMask; }
// Check state
static inline bool klGpioIsSetByMsk  (GPIO_TypeDef *PGpioPort, uint16_t APinMask)   { return (bool)(PGpioPort->IDR & APinMask); }
static inline bool klGpioIsSetByN    (GPIO_TypeDef *PGpioPort, uint16_t APinNumber) { return klGpioIsSetByMsk(PGpioPort, (uint16_t)(1<<APinNumber)); }
static inline bool klGpioIsClearByMsk(GPIO_TypeDef *PGpioPort, uint16_t APinMask)   { return !((bool)(PGpioPort->IDR & APinMask)); }
static inline bool klGpioIsClearByN  (GPIO_TypeDef *PGpioPort, uint16_t APinNumber) { return klGpioIsClearByMsk(PGpioPort, (uint16_t)(1<<APinNumber)); }
// Setup
void klGpioSetupByMsk  (GPIO_TypeDef *PGpioPort, uint16_t APinMask, GPIOMode_TypeDef AMode);
static inline void klGpioSetupByN (GPIO_TypeDef *PGpioPort, uint16_t APinNumber, GPIOMode_TypeDef AMode) { klGpioSetupByMsk(PGpioPort, (uint16_t)(1<<APinNumber), AMode); }
// Disable JTAG, leaving SWD
static inline void klJtagDisable(void) {
    bool AfioWasEnabled = (RCC->APB2ENR & RCC_APB2ENR_AFIOEN);
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;     // Enable AFIO
    uint32_t tmp = AFIO->MAPR;
    tmp &= ~0x07000000; // Clear SWJ_CFG bits
    tmp |= 0x02000000;  // Set SWJ_CFG bits to 010 => JTAG disabled and SWD enabled
    AFIO->MAPR = tmp;   // Write data back to register
    if (!AfioWasEnabled) RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
}

// ==== Class for single pin manipulation ====
class klPin_t {
protected:
    GPIO_TypeDef* IGPIO;
    uint16_t IPinMask, IPinNumber;
public:
    klPin_t(GPIO_TypeDef *PGpioPort, uint16_t APinNumber, GPIOMode_TypeDef AMode) { Init(PGpioPort, APinNumber, AMode); }
    klPin_t(void) { IGPIO = 0; IPinMask = 0; IPinNumber = 0; }
    bool operator =(const bool AValue) {
        if (AValue) Set();
        else Clear();
        return true;
    }
    operator bool() { return klGpioIsSetByMsk(IGPIO, IPinMask); }
    void Init(GPIO_TypeDef *PGpioPort, uint16_t APinNumber, GPIOMode_TypeDef AMode);
    void Set(void)    { klGpioSetByMsk   (IGPIO, IPinMask); }
    void Clear(void)  { klGpioClearByMsk (IGPIO, IPinMask); }
    void Toggle(void) { klGpioToggleByMsk(IGPIO, IPinMask); }
};

class klPinIrq_t : public klPin_t {
private:
    uint32_t IChannel;
public:
    // Interrupt
    void IrqSetup(EXTITrigger_TypeDef ATriggerType);
    void IrqEnable(void);
    void IrqDisable(void);
};

// ==== Timer ====
#define TIM_FREQ_MAX    0xFFFFFFFF
class klTimer_t {
protected:
    TIM_TypeDef* ITimer;
public:
    void Init(TIM_TypeDef* PTimer, uint16_t ATopValue, uint32_t AFreqHz);
    void Enable(void)  { ITimer->CR1 |= TIM_CR1_CEN; }
    void Disable(void) { ITimer->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN)); }
    void SetFreqHz(uint32_t AFreqHz);
};

/*
 * TIM_OCPolarity_High, TIM_OCPolarity_Low
 */
class klPwmChannel_t : klTimer_t {
private:
    uint8_t INumber;
public:
    void Init(TIM_TypeDef* PTimer, uint16_t ATopValue, uint32_t AFreqHz, uint8_t ANumber, uint16_t APolarity);
    void Enable(void);
    void Disable(void);
    void DisableTimer(void) { klTimer_t::Disable(); }
    void Set(uint16_t AValue);
    void SetFreqHz(uint32_t AFreqHz) { klTimer_t::SetFreqHz(AFreqHz); }
};

// =========================== Colors ==========================================
struct Color_t {
    uint8_t Red, Green, Blue;
    bool operator == (const Color_t AColor) { return ((Red == AColor.Red) and (Green == AColor.Green) and (Blue == AColor.Blue)); }
    //bool operator != (const Color_t AColor) { return ((this->Red != AColor.Red) || (this->Green != AColor.Green) || (this->Blue != AColor.Blue)); }
    //bool IsOn(void) const { return (Red || Green || Blue); }
};
#define clBlack     ((Color_t){0,   0,   0})
#define clRed       ((Color_t){255, 0,   0})
#define clGreen     ((Color_t){0,   255, 0})
#define clBlue      ((Color_t){0,   0,   255})
#define clYellow    ((Color_t){255, 255, 0})

// ========================= System clock and ID ===============================
enum Clk_t {clk1MHzInternal, clk2MHzInternal, clk4MHzInternal, clk8MHzInternal, clk8MHzExternal,
#if defined STM32F10X_HD
	clk36MHzExternal, clk72MHzExternal,
#endif
};
void InitClock(Clk_t AClk);

void GetUniqueID(uint32_t *IDArr);

// ============================ Delay and time =================================
// Systick is used here
extern uint32_t ITickCounter;
class Delay_t {
public:
    void Init(void);
    // Simple loop-based delays, no init needed
    void Loop (volatile uint32_t ACounter) { while(ACounter--); }
    void ms (uint32_t Ams);
    // Every tick is 0.5 ms long => prescaler = SYSCLK/2000. 0.5 ms needed for correct work when SysCoreClk >= 65 535 000 Hz
    //void PrescalerUpdate(void) { TIM2->PSC = (uint16_t)(SystemCoreClock / 2000) - 1; }
    // Timer-driven delays
    bool Elapsed(uint32_t *AVar, const uint32_t ADelay);
    void Reset  (uint32_t *AVar) { *AVar = ITickCounter; }
    void Bypass (uint32_t *AVar, const uint32_t ADelay) { *AVar = (uint32_t)(ITickCounter - ADelay); }
};
extern Delay_t Delay;
// IRQ handler
extern "C" {
void SysTick_Handler(void);
}

// ============================== UART command =================================
#define UART_TXBUF_SIZE     45
#define UART_DMA_CHNL       DMA1_Channel4

//#define RX_ENABLED

#ifdef RX_ENABLED
#define UART_RXBUF_SIZE     45
enum CmdState_t {csNone, csInProgress, csReady};
#endif

class CmdUnit_t {
private:
    uint8_t TXBuf1[UART_TXBUF_SIZE], TXBuf2[UART_TXBUF_SIZE];
    uint8_t *PBuf, TxIndx;
    bool IDmaIsIdle;
#ifdef RX_ENABLED
    CmdState_t CmdState;
    char RXBuf[UART_RXBUF_SIZE];
    uint8_t RxIndx;
    void CmdReset(void) { RxIndx = 0; CmdState = csNone; }
#endif
    void IStartTx(void);
    void IBufWrite(uint8_t AByte);
public:
    char UintToHexChar (uint8_t b) { return ((b<=0x09) ? (b+'0') : (b+'A'-10)); }
    void Printf(const char *S, ...);
    void FlushTx(void);
    void Init(uint32_t ABaudrate);
    void Task(void);
#ifdef RX_ENABLED
    void NewCmdHandler(void);   // Place it where needed
#endif
    // IRQ
    void IRQHandler(void);
};

// RX IRQ
#ifdef RX_ENABLED
extern "C" {
void USART1_IRQHandler(void);
}
#endif

extern CmdUnit_t Uart;


#endif /* KL_GPIO_H_ */
