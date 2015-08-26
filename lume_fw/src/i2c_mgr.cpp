#include "i2c_mgr.h"
#include "stm32f10x_i2c.h"
#include "core_cm3.h"

i2cMgr_t i2cMgr;

// ============================ Tasking and handling ===========================
#ifndef I2C_POLL_ONLY
void i2cMgr_t::Task() {
    if (IsError) {
        HwInit();
        return;
    }
    // Get next Cmd if current is zero
    for(uint8_t i=0; i<I2C_CMD_QUEUE_LENGTH; i++) {
        if(Cmd[RIndx] == 0) GetNext();
        else {
            if(Cmd[RIndx]->State == CmdPending) {
                // Check if cmd is ok
                if((Cmd[RIndx]->BufToWrite.Length == 0) and !Cmd[RIndx]->ByteWrite) {   // nothing to write
                    Cmd[RIndx]->State = CmdFailed;
                    GetNext();
                    return;
                }
                // Check if Busy and timeout after Stop
                while(IsBusy()) if(Delay.Elapsed(&Timer, 117)) {    // Timer is reset at StopAndGetNext()
                    Uart.Printf("Too busy\r");
                    IsError = true;   // Reset I2C
                    return;
                }
                // Clear flags
                I2C1->SR1 = 0;
                while(IsRxNotEmpty()) (void)I2C1->DR;   // Read DR until it empty
                ClearAddrFlag();
                // Start the job
                Cmd[RIndx]->State = CmdWrite;
                I2C1->CR2 |= I2C_IT_EVT | I2C_IT_ERR;    // Enable Evt & Err IRQ
                I2C1->CR1 |= 0x0100;        // Generate start
            } // if pending
            break;  // get out of cycle
        } // if != 0
    } // for
}


void i2cMgr_t::WriteMany() {
    // Prepare DMA
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &I2C1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t) Cmd[RIndx]->BufToWrite.P;
    DMA_InitStructure.DMA_BufferSize         = Cmd[RIndx]->BufToWrite.Length;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;  // From memory to I2C
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(I2C_DMA_CHNL_TX, &DMA_InitStructure);
    // Start transmission
    DMA_Cmd(I2C_DMA_CHNL_TX, ENABLE);   // Enable DMA channel
    I2C_DMACmd(I2C1, ENABLE);           // Enable DMA
    I2C_DMA_CHNL_TX->CCR |= DMA_IT_TC;  // Enable DMA irq
    DMA_ITConfig(I2C_DMA_CHNL_TX, DMA_IT_TC, ENABLE);
}

void i2cMgr_t::ReadMany() {
    // Enable Acknowledgement
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    // Prepare DMA
    DMA_InitTypeDef DMA_InitStructure;
    //DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &I2C1->DR;
    DMA_InitStructure.DMA_PeripheralBaseAddr = 0x40005410;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t) Cmd[RIndx]->BufToRead.P;
    DMA_InitStructure.DMA_BufferSize         = Cmd[RIndx]->BufToRead.Length;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;  // From I2C to memory
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(I2C_DMA_CHNL_RX, &DMA_InitStructure);
    //Inform the DMA that the next End Of Transfer Signal will be the last one, need to send NACK after last byte
    I2C_DMALastTransferCmd(I2C1, ENABLE);
    // Start transmission
    I2C_DMACmd(I2C1, ENABLE);           // Enable DMA
    DMA_Cmd(I2C_DMA_CHNL_RX, ENABLE);   // Enable DMA channel
    I2C_DMA_CHNL_RX->CCR |= DMA_IT_TC;  // Enable DMA irq
}

void i2cMgr_t::StopAndGetNext() {
    I2C_GenerateSTOP(I2C1, ENABLE);
    GetNext();
    Delay.Reset(&Timer);
}
#endif

// ================================ Init & Add Cmd =============================
void i2cMgr_t::HwInit() {
	Uart.Printf("I2C HwInit\r");
	Uart.FlushTx();
    I2C_DeInit(I2C1);
    // ==== First, check if bus is not in BUSY state ====
    klPin_t Scl, Sda;
    Scl.Init(GPIOB, 6, GPIO_Mode_Out_OD);
    Sda.Init(GPIOB, 7, GPIO_Mode_IN_FLOATING);
    Scl = 1;    // Clk Idle High
    Sda = 1;    // Try to set 1 on SDA
    if(Sda == 0) {
    	Uart.Printf("SDA 0\r");
    	Uart.FlushTx();
    	while(1) {  // Look at result
			// Send clock pulse
			Scl = 0; Delay.ms(1);
			Scl = 1; Delay.ms(1);
			if(Sda == 1) {  // looks like released bus, send stop (SDA Lo-Hi when SCL High)
				Scl = 0; Delay.ms(1);
				Sda.Init(GPIOB, 7, GPIO_Mode_Out_OD);
				Sda = 0; Delay.ms(1);
				Scl = 1; Delay.ms(1);
				Sda = 1; Delay.ms(1);
				break;
			}
		} // while
    	Uart.Printf("SDA ok\r");
    }
    // ==== Proceed with init ====
    // Init GPIO - SCL & SDA
    klGpioSetupByMsk(GPIOB, GPIO_Pin_6 | GPIO_Pin_7, GPIO_Mode_AF_OD);
    // I2C
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);    // I2C clock
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x30;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_CLOCK_FREQ;
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
    I2C_GetLastEvent(I2C1); // Clear flags
    // ==== Init DMA ====
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
#ifndef I2C_POLL_ONLY
    IsError = false;
#endif
}

void i2cMgr_t::Init() {
	HwInit();
    // ==== NVIC ====
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);     // Configure the NVIC Preemption Priority Bits
    // Enable I2C Evt IRQ
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    // Enable I2C Err IRQ
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    // Enable DMA I2C_tx IRQ
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    // Enable DMA I2C_rx IRQ
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_Init(&NVIC_InitStructure);

#ifndef I2C_POLL_ONLY
    // ==== Init manager ====
    RIndx = 0;
    WIndx = 0;
    // Reset stage of all commands
    for (uint8_t i=0; i<I2C_CMD_QUEUE_LENGTH; i++) Cmd[i] = 0; // Reset all pointers
#endif
}

#ifndef I2C_POLL_ONLY
void i2cMgr_t::AddCmd(I2C_Cmd_t *PCmd) {
    //Uart.Printf("WIndx: %u\r", WIndx);
    if(Cmd[WIndx] == 0) {   // Check if buffer cell is empty
        PCmd->State = CmdPending;
        Cmd[WIndx] = PCmd; // Add PCmd to buffer
        //Uart.Printf("Cmd Addr: %X\r", PCmd->Address);
        if (++WIndx >= I2C_CMD_QUEUE_LENGTH) WIndx = 0; // Process command queue
    }
    else Uart.Printf("i2c q ovf\r");
}

void i2cMgr_t::GetNext() {
    Cmd[RIndx] = 0; // Zero current pointer to disallow repeatable Cmd execution
    RIndx++;
    if (RIndx >= I2C_CMD_QUEUE_LENGTH) RIndx = 0;
}

#endif
// =========================== Polling functions ===============================
// Wait for Idle Bus
uint8_t i2cMgr_t::BusyWait() {
	uint32_t FTmr;
	Delay.Reset(&FTmr);
	uint16_t ISR2;
	do {
		ISR2 = I2C1->SR2;
		if(Delay.Elapsed(&FTmr, 4)) {
            Uart.Printf("I2C Bsy: %04X\r", ISR2);
            return I2C_ERR_TIMEOUT;
		}
	} while(ISR2 & 0x0002);
    return I2C_OK;
}
uint8_t i2cMgr_t::SendStart(void) {
    I2C_GenerateSTART(I2C1, ENABLE);
    uint32_t FTmr;
    Delay.Reset(&FTmr);
    uint16_t ISR1;
    do {
        ISR1 = I2C1->SR1;
        if(Delay.Elapsed(&FTmr, 4)) {
            Uart.Printf("S evt: %04X\r", ISR1);
            return I2C_ERR_NOMASTER;
        }
    } while (!(ISR1 &0x0001));	// Start condition generated
    return I2C_OK;
}
uint8_t i2cMgr_t::SendAddrTXPoll(uint8_t AAddr) {
    //Uart.Printf("Addr: %X\r", AAddr);
    uint32_t IEvt;
    I2C_Send7bitAddress(I2C1, (AAddr << 1), I2C_Direction_Transmitter);
    uint32_t FTmr;
    Delay.Reset(&FTmr);
    do {
        IEvt = I2C_GetLastEvent(I2C1);
        if (IEvt & I2C_EVENT_SLAVE_ACK_FAILURE) {   // NACK occured, slave doesn't answer
            Uart.Printf("Addr TX NACK\r");
            return I2C_ERR_SLAVE_NACK;
        }
        if(Delay.Elapsed(&FTmr, 4)) {
            Uart.Printf("Addr Err: %08X\r", IEvt);  // May occur if CPU clock is too low
            return I2C_ERR_TIMEOUT;
        }
    } while (IEvt != I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
    //Uart.Printf("I2C Slave Addr TX ACK\r");
    return I2C_OK;   // all right
}
uint8_t i2cMgr_t::SendAddrRXPoll(uint8_t AAddr) {
    uint32_t IEvt;
    I2C_Send7bitAddress(I2C1, (AAddr << 1), I2C_Direction_Receiver);
    uint32_t FTimeout = I2C_TIMEOUT;
    do {
        IEvt = I2C_GetLastEvent(I2C1);
        if (IEvt & I2C_EVENT_SLAVE_ACK_FAILURE) {   // NACK occured, slave doesn't answer
            Uart.Printf("Addr RX NACK\r");
            return I2C_ERR_SLAVE_NACK;
        }
        if ((FTimeout--) == 0) {
            Uart.Printf("Addr RX Err\r");
            return I2C_ERR_TIMEOUT;
        }
    } while (IEvt != I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
    //Uart.Printf("I2C Slave Addr RX ACK\r");
    return I2C_OK;   // all right
}

uint8_t i2cMgr_t::CmdPoll(I2C_Cmd_t *PCmd) {
	//Uart.Printf("i2c Cmd Poll\r");
	while(BusyWait()) HwInit();
	if(PCmd->BufToWrite.Length == 0) return I2C_OK;
	// ==== First stage ====
    //Uart.Printf("i2c write: %A\r", ACmd.BufToWrite.P, ACmd.BufToWrite.Length);
    while(SendStart() != I2C_OK) HwInit();
	uint8_t rslt=I2C_OK;
    uint32_t IEvt, ITimeout;
    uint8_t i, b;
	rslt = SendAddrTXPoll(PCmd->Address);
	i = 0;
	while((i < PCmd->BufToWrite.Length) and (rslt == I2C_OK)) {
		b = PCmd->BufToWrite.P[i];
		//Uart.Printf("%X\r", b);
		I2C_SendData(I2C1, b);
		ITimeout = I2C_TIMEOUT;
		do {	// Check if transmitted
			IEvt = I2C_GetLastEvent(I2C1);
			if (IEvt & I2C_EVENT_SLAVE_ACK_FAILURE) {   // NACK occured, slave doesn't answer
				Uart.Printf("\rNACK\r");
				rslt = I2C_ERR_SLAVE_NACK;
				break;
			}
			if(ITimeout-- == 0) {
				rslt = I2C_ERR_TIMEOUT;
				break;
			}
		} while(IEvt != I2C_EVENT_MASTER_BYTE_TRANSMITTED);
		i++;
	} // while
	if(rslt != I2C_OK) return rslt;

	// ==== Second stage ====
	if(PCmd->BufToRead.Length == 0) {
		I2C_GenerateSTOP(I2C1, ENABLE);
		return I2C_OK;
	}
	else {
		//Uart.Printf("i2c read %u\r", ACmd.DataToRead.Length);
		if((rslt = SendStart()) == I2C_OK) {
			if((rslt = SendAddrRXPoll(PCmd->Address)) == I2C_OK) {
				// Act accordingly to number of bytes to receive
				if(PCmd->BufToRead.Length == 1) {		// 1 byte to read
					I2C_AcknowledgeConfig(I2C1, DISABLE);
					I2C_GenerateSTOP(I2C1, ENABLE);
					// Wait for the byte to be received
					ITimeout = I2C_TIMEOUT;
					while(1) {	// Check if received
						if(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == SET) {
						    PCmd->BufToRead.P[0] = I2C_ReceiveData(I2C1);
							break;
						}
						if(ITimeout-- == 0) {
							rslt = I2C_ERR_TIMEOUT;
							break;
						}
					} // while
				} // if 1
				else if(PCmd->BufToRead.Length == 2) {	// 2 bytes to read
					I2C_AcknowledgeConfig(I2C1, DISABLE);
					// Wait for the two bytes to be received
					ITimeout = I2C_TIMEOUT;
					while(1) {
						IEvt = I2C_GetLastEvent(I2C1);
						if(IEvt & I2C_FLAG_BTF) {
							I2C_GenerateSTOP(I2C1, ENABLE);
							PCmd->BufToRead.P[0] = I2C_ReceiveData(I2C1);
							PCmd->BufToRead.P[1] = I2C_ReceiveData(I2C1);
							break;
						}
						if(ITimeout-- == 0) {
							rslt = I2C_ERR_TIMEOUT;
							break;
						}
					} // while
				}
				else {	// many bytes to read
					I2C_AcknowledgeConfig(I2C1, ENABLE);
					i = 0;
					while((i < PCmd->BufToRead.Length) and (rslt == I2C_OK)) {
						ITimeout = I2C_TIMEOUT;
						while(1) {
							IEvt = I2C_GetLastEvent(I2C1);
							if(i == PCmd->BufToRead.Length-3) {	// Catch BTF
								if(IEvt & I2C_FLAG_BTF) {
									__disable_irq();
									I2C_AcknowledgeConfig(I2C1, DISABLE);
									PCmd->BufToRead.P[i] = I2C_ReceiveData(I2C1);
									I2C_GenerateSTOP(I2C1, ENABLE);
									i++;
									PCmd->BufToRead.P[i] = I2C_ReceiveData(I2C1);
									__enable_irq();
									break;
								}
							}
							else if(IEvt & I2C_FLAG_RXNE) {
							    PCmd->BufToRead.P[i] = I2C_ReceiveData(I2C1);
								break;
							}

							if(ITimeout-- == 0) {
								rslt = I2C_ERR_TIMEOUT;
								break;
							}
						} // while 1
						i++;
					} // while
				} // many bytes to read
			} // Send addr
		} // send repstart
	} // if second stage
	return rslt;
}

// ===================================== IRQs ==================================
// Flags SB, ADDR and BTF are handled here
void i2cMgr_t::IrqEvHandler() {
    uint32_t IEvt = I2C1->SR1;
    //Uart.Printf("Ev: %04X\r", IEvt);
    if(Cmd[RIndx] == 0) {
        IsError = true;
        I2C1->CR2 &= ~I2C_IT_EVT;
        return;
    }
    // ==== Start sent, send addr. SB flag will be cleared that way. ====
    if(IEvt & 0x0001) {
        //Uart.Printf("Addr: %X\r", Cmd[RIndx]->Address);
        if(Cmd[RIndx]->State == CmdWrite) I2C1->DR = ((Cmd[RIndx]->Address) << 1);
        else I2C1->DR = ((Cmd[RIndx]->Address) << 1) | 0x01;    // Set read bit
    }

    // ==== Addr sent, write or read depending on state ====
    else if(IEvt & 0x0002) {                 // Addr sent, write data
        //Uart.Printf("Addr\r");
        if(Cmd[RIndx]->State == CmdWrite) {
            (void)I2C1->SR2;                // EV6. Clear Addr Flag
            if(Cmd[RIndx]->ByteWrite) {     // write byte first
                I2C1->DR = Cmd[RIndx]->Byte;
                Cmd[RIndx]->State = CmdWritingByte;
            }
            else if(Cmd[RIndx]->BufToWrite.Length == 1) {
                I2C1->DR = Cmd[RIndx]->BufToWrite.P[0];
                Cmd[RIndx]->State = CmdWritingBufSingle;
            }
            else {  // Many bytes to write
                I2C1->CR2 &= ~I2C_IT_EVT;   // Disable I2C evt IRQ
                WriteMany();
                Cmd[RIndx]->State = CmdWritingBufMany;
            }
        } // if write
        else if(Cmd[RIndx]->State == CmdRead) {
            if (Cmd[RIndx]->BufToRead.Length == 1) {    // Single byte to read
                I2C_AcknowledgeConfig(I2C1, DISABLE);   // Disable ACK
                (void)I2C1->SR2;                        // Clear Addr Flag
                I2C_GenerateSTOP(I2C1, ENABLE);         // Send stop after byte rcvd
                Cmd[RIndx]->State = CmdReadingBufSingle;
            }
            else {  // Many bytes to read
                (void)I2C1->SR2;                        // Clear Addr Flag
                I2C1->CR2 &= ~I2C_IT_EVT;   // Disable I2C evt IRQ
                ReadMany();
                Cmd[RIndx]->State = CmdReadingBufMany;
            } // many
        }   // read
    } // if addr

    // ==== Byte transfer done ====
    else if(IEvt & 0x0004) {    // BTF
        switch(Cmd[RIndx]->State) {
            case CmdWritingByte:
                if(Cmd[RIndx]->BufToWrite.Length == 0) {        // Nothing to write
                    if (Cmd[RIndx]->BufToRead.Length == 0) {    // Nothing to read
                        Cmd[RIndx]->State = CmdSucceded;
                        StopAndGetNext();
                    }
                    else {                                      // Read needed, Send RepeatedStart
                        Cmd[RIndx]->State = CmdRead;
                        I2C1->CR1 |= 0x0100;
                    }
                }
                else if(Cmd[RIndx]->BufToWrite.Length == 1) {   // Write one byte
                    //Uart.Printf("Buf0: %X\r", Cmd[RIndx]->BufToWrite.P[0]);
                    I2C_SendData(I2C1, Cmd[RIndx]->BufToWrite.P[0]);
                    Cmd[RIndx]->State = CmdWritingBufSingle;
                }
                else {  // Many bytes to write
                    I2C1->CR2 &= ~I2C_IT_EVT;   // Disable I2C evt IRQ
                    WriteMany();
                    Cmd[RIndx]->State = CmdWritingBufMany;
                }
                break;

            case CmdWritingBufSingle:   // Byte sent, check if reading needed
                if (Cmd[RIndx]->BufToRead.Length == 0) {    // Nothing to read
                    Cmd[RIndx]->State = CmdSucceded;
                    StopAndGetNext();
                }
                else {                                      // Read needed, Send RepeatedStart
                    Cmd[RIndx]->State = CmdRead;
                    I2C1->CR1 |= 0x0100;
                }
                break;

            case CmdReadingBufSingle:
                Cmd[RIndx]->BufToRead.P[0] = I2C1->DR;
                Cmd[RIndx]->State = CmdSucceded;
                GetNext(); // Stop is already sent
                break;
            default: break;
        } // switch
    } // if BTF
}

void i2cMgr_t::IrqErrHandler() {
    uint32_t IEvt = I2C1->SR1;
    Uart.Printf("Err: %04X\r", IEvt);
    if(Cmd[RIndx] != 0) {
        Cmd[RIndx]->State = CmdFailed;
        if(IEvt & 0x0400) {
            Uart.Printf("NACK\r");
            I2C1->SR1 &= ~0x0400;   // Clear AF flag
            StopAndGetNext();
            return;    // Just get out, no need in reset
        }
    }
    IsError = true;
    I2C1->CR2 &= ~I2C_IT_ERR;    // Disable Err IRQ
}

// ==== Writing many bytes ended ====
void i2cMgr_t::IrqDmaTxHandler() {
    //Uart.Printf("WM\r");
    I2C_DMA_CHNL_TX->CCR &= ~DMA_IT_TC;     // Disable DMA tx irq
    DMA_DeInit(I2C_DMA_CHNL_TX);
    // Bytes sent, check if reading needed
    if (Cmd[RIndx]->BufToRead.Length != 0) {    // Read needed, Send RepeatedStart
        Cmd[RIndx]->State = CmdRead;
        I2C1->CR1 |= 0x0100;
        I2C1->CR2 |= I2C_IT_EVT;    // Enable Evt IRQ
    }
    else {  // nothing to read
        Cmd[RIndx]->State = CmdSucceded;
        StopAndGetNext();
        //Uart.Printf("Cmd Done\r");
    }
}

// ==== Reading many bytesa ended ====
void i2cMgr_t::IrqDmaRxHandler() {
    //Uart.Printf("RM\r");
    I2C_DMA_CHNL_RX->CCR &= ~DMA_IT_TC;     // Disable DMA rx irq
    DMA_DeInit(I2C_DMA_CHNL_RX);
    Cmd[RIndx]->State = CmdSucceded;
    StopAndGetNext();
}

// ============================== C externed irqs ==============================
void I2C1_EV_IRQHandler(void) {
    i2cMgr.IrqEvHandler();
}
void I2C1_ER_IRQHandler(void) {
    i2cMgr.IrqErrHandler();
}
void DMA1_Channel6_IRQHandler(void) {
    if (DMA_GetFlagStatus(DMA1_FLAG_TC6)) { // if transmission completed
        DMA_ClearFlag(DMA1_FLAG_TC6);
        i2cMgr.IrqDmaTxHandler();
    }
}
void DMA1_Channel7_IRQHandler(void) {
    if (DMA_GetFlagStatus(DMA1_FLAG_TC7)) { // if transmission completed
        DMA_ClearFlag(DMA1_FLAG_TC7);
        i2cMgr.IrqDmaRxHandler();
    }
}
