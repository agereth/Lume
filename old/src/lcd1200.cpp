#include "lcd1200.h"
#include "kl_lib.h"
#include "lcd_font.h"
#include "string.h"
#include <stdarg.h>

#include "tiny_sprintf.h"

Lcd_t Lcd;

void Lcd_t::Task(void) {
    // Output one byte of bufer every time
    static uint16_t i=0;
    WriteData(IBuf[i++]);
    if (i == LCD_VIDEOBUF_SIZE) i=0;
}

void Lcd_t::Init(void) {
    // ==== Backlight: Timer15 Ch2 ====
    // Setup pin
    klGpioSetupByMsk(GPIOB, GPIO_Pin_15, GPIO_Mode_AF_PP);
    // Remap Timer15 to PB14 & PB15
    AFIO->MAPR2 |= 0x00000001;
    // Setup timer15
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM15, ENABLE);
    TIM15->CR1 = 0x01;      // Enable timer
    TIM15->CR2 = 0;
    TIM15->PSC = 0;         // Do not divide input freq
    TIM15->ARR = 100;       // Autoreload register: full brightness=100
    TIM15->BDTR = 0xC000;   // Main output Enable
    TIM15->CCMR1 = 0x6000;  // PWM mode1 on Ch2 enabled
    TIM15->CCER = 0x0010;   // Output2 enabled, polarity not inverted

    // ==== GPIOs ====
    // Configure LCD_XRES, LCD_XCS, LCD_SCLK & LCD_SDA as Push-Pull output
    klGpioSetupByMsk(LCD_GPIO, LCD_XRES | LCD_XCS | LCD_SCLK | LCD_SDA, GPIO_Mode_Out_PP);

    // ========================= Init LCD ======================================
    SCLK_Lo();
    XCS_Hi();
    // Reset display
    XRES_Lo();
    Delay.ms(7);
    XRES_Hi();
    WriteCmd(0xAF);    // display ON
    // Reset display again
    XRES_Lo();
    Delay.ms(7);
    XRES_Hi();
    Delay.ms(7);
    // Initial commands
    WriteCmd(0xAF);    // display ON
    WriteCmd(0xA4);    // Set normal display mode
    WriteCmd(0x2F);    // Charge pump on
    WriteCmd(0x40);    // Set start row address = 0

    WriteCmd(0xC8);    // mirror Y axis
    //WriteCmd(0xA1);    // Mirror X axis
    // Set x=0, y=0
    WriteCmd(0xB0);    // Y axis initialization
    WriteCmd(0x10);    // X axis initialisation1
    WriteCmd(0x00);    // X axis initialisation2
    Cls();             // clear LCD buffer

    draw_mode = OVERWRITE;
}


void Lcd_t::Shutdown(void) {
    DMA_Cmd(DMA1_Channel2, DISABLE);
    XRES_Lo();
    XCS_Lo();
    SCLK_Lo();
    SDA_Lo();
    Backlight(0);
}

void Lcd_t::WriteCmd(uint8_t AByte) {
    SCLK_Lo();
    XCS_Lo();   // Select chip
    // Send "Cmd" bit
    SDA_Lo();
    SCLK_Hi();
    SCLK_Lo();
    // Send byte
    for(uint8_t i=0; i<8; i++) {
        if(AByte & 0x80) SDA_Hi();
        else SDA_Lo();
        SCLK_Hi();
        SCLK_Lo();
        AByte <<= 1;
    }
    XCS_Hi();
}
void Lcd_t::WriteData(uint8_t AByte) {
    SCLK_Lo();
    XCS_Lo();   // Select chip
    // Send "Data" bit
    SDA_Hi();
    SCLK_Hi();
    SCLK_Lo();
    // Send byte
    for(uint8_t i=0; i<8; i++) {
        if(AByte & 0x80) SDA_Hi();
        else SDA_Lo();
        SCLK_Hi();
        SCLK_Lo();
        AByte <<= 1;
    }
    XCS_Hi();
}


void Lcd_t::Symbols(int column, int row, ...) {
    int index = column*6 + row*LCD_WIDTH;
    uint8_t FCharCode=1, RepeatCount;
    va_list Arg;
    va_start(Arg, row);    // Set pointer to last argument
    while(1) {
        FCharCode = (uint8_t)va_arg(Arg, int32_t);
        if(FCharCode == 0) break;
        RepeatCount = (uint8_t)va_arg(Arg, int32_t);
        for(uint8_t j=0; j<RepeatCount; j++) DrawChar(&index, FCharCode);
    }
    va_end(Arg);
}


void Lcd_t::Printf(int column, int row, const char *format, ...) {
	char buf[LCD_STR_HEIGHT*LCD_STR_WIDTH+1];
    va_list args;
    va_start(args, format);
    tiny_vsprintf(buf, format, args);
    va_end(args);

    int index = column*6 + row*LCD_WIDTH;
    for (int i = 0; buf[i] != 0; i++) DrawChar(&index, buf[i]);
}

// ================================ Graphics ===================================
/*
 * Prints char at specified buf indx, returns next indx
 */
void Lcd_t::DrawChar(int *index, uint8_t AChar) {
    for(uint8_t i=0; i < 6; i++) {
        DrawBlock((*index)++, Font_6x8_Data[AChar][i], 0xFF);
        if (*index >= LCD_VIDEOBUF_SIZE) *index = 0;
    }
}

void Lcd_t::DrawImage(int x, int y, const uint8_t* img) {
	y /= 8; // y is now rows, not pixels
    uint8_t width = *img++;
    uint8_t height = *img++;
    for(int fy = y; fy < y+height; fy++) {
        int index = x + fy*LCD_WIDTH;
        for(int fx = x; fx < x+width; fx++) {
            DrawBlock(index++, *img++, 255);
            if (index > LCD_VIDEOBUF_SIZE) break;
        }
    }
}

void Lcd_t::DrawBlock(int index, uint8_t data, uint8_t mask) {
    uint8_t *b = &IBuf[index];
	switch (draw_mode) {
        case DRAW: 	                *b |= data;		                   break;
        case CLEAR:	                *b &= ~data;		               break;
        case OVERWRITE:             *b = (*b & ~mask) | data;          break;
        case OVERWRITE_INVERTED:    *b = (*b & ~mask) | (data ^ mask); break;
        case INVERT:                *b ^= data;		                   break;
	}
}
