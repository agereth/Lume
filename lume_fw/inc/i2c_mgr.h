/*
 * File:   i2c_mgr.h
 * Author: Kreyl
 *
 * Created on May 31, 2011, 8:34 PM
 */

#ifndef I2C_MGR_H
#define	I2C_MGR_H

#include "stm32f10x.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_dma.h"
#include "misc.h"

#include "kl_lib.h"

//#define I2C_POLL_ONLY   // Decide which features needed. When POLL_ONLY, no Cmd Query and DMA will be used.
#define I2C_CLOCK_FREQ          800000  // 100 or 400 kHz

// DMA
#ifndef I2C_POLL_ONLY
#define I2C_DMA_CHNL_TX         DMA1_Channel6
#define I2C_DMA_CHNL_RX         DMA1_Channel7
#define I2C_DMA_FLAG_TC_TX      DMA1_FLAG_TC6
#define I2C_DMA_FLAG_TC_RX      DMA1_FLAG_TC7
#endif

// ================================= Constants =================================
#define I2C_CMD_QUEUE_LENGTH    9
#define I2C_TIMEOUT             50004

// Error codes
#define I2C_OK                  0
#define I2C_ERR_TIMEOUT         1
#define I2C_ERR_NOMASTER        2
#define I2C_ERR_SLAVE_NACK      3
#define I2C_WAITING             4


// ================================ Data types =================================
struct Buf8_t {
    uint8_t *P;
    uint32_t Length;
};

enum CmdState_t {CmdPending=0, CmdSucceded=1, CmdFailed=2,
    CmdWrite=3, CmdRead=4,
    CmdWritingByte=6, CmdWritingBufSingle=7, CmdWritingBufMany=8,
    CmdReadingBufSingle=10, CmdReadingBufMany=11
};

/* Any i2c transaction consists of one or two stages:
 * 1) Start - Addr+Write - [one byte to write] - [n bytes to write] - Stop
 * 2) Start - Addr+Write - [one byte to write] - [n bytes to write] - RepStart - Addr+Read - [m bytes to read] - Stop
 */

struct I2C_Cmd_t {
    uint8_t Byte;     // Byte to write and/or read
    bool ByteWrite;
    Buf8_t BufToWrite, BufToRead;	// Buffers of data to read or write
    uint8_t Address;        		// Device address
    CmdState_t State;
};

class i2cMgr_t {
private:
    // Flags
    bool IsNack() { return (I2C1->SR1 & 0x0400); }
    bool IsAddrSent() { return (I2C1->SR1 & 0x0002); }
    bool IsTransferCompleted() { return (I2C1->SR1 & 0x0004); }
    bool IsRxNotEmpty() { return (I2C1->SR1 & 0x0040); }
    void ClearAddrFlag() { (void)I2C1->SR1; (void)I2C1->SR2; }
    bool IsBusy() {return (I2C1->SR2 & 0x0002); }
    // Hardware init
    void HwInit();
#ifndef I2C_POLL_ONLY
    uint32_t Timer;
    bool IsError;
    uint32_t RIndx, WIndx;
    I2C_Cmd_t *Cmd[I2C_CMD_QUEUE_LENGTH];
    // Task-based functions
    void WriteMany();
    void ReadMany();
    void GetNext();
    void StopAndGetNext();
#endif
    // Polling-based functions
    uint8_t BusyWait();
    uint8_t SendStart();
    uint8_t SendAddrTXPoll(uint8_t AAddr);
    uint8_t SendAddrRXPoll(uint8_t AAddr);
    uint8_t WriteOneBytePoll();
public:
    void IrqEvHandler();
    void IrqErrHandler();
    void IrqDmaTxHandler();
    void IrqDmaRxHandler();
    void Init();
#ifndef I2C_POLL_ONLY
    void Task();
    void AddCmd(I2C_Cmd_t *PCmd);
#endif
    uint8_t CmdPoll(I2C_Cmd_t *PCmd);	// Perform Cmd in polling way
};

extern i2cMgr_t i2cMgr;

// Interrupts
extern "C" {
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void DMA1_Channel6_IRQHandler(void);
void DMA1_Channel7_IRQHandler(void);
}

#endif	/* I2C_MGR_H */

