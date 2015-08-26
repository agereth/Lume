/*
 * File:   lcd110x.h
 * Author: Kreyl Laurelindo
 *
 * Created on 24 �������� 2010 �., 14:25
 */

#ifndef LCD110X_H
#define	LCD110X_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include <string.h>

// ================================ Defines ====================================
#define LCD_GPIO        GPIOB

#define LCD_SCLK        GPIO_Pin_12
#define LCD_SDA         GPIO_Pin_10
#define LCD_XRES        GPIO_Pin_11
#define LCD_XCS         GPIO_Pin_14
#define LCD_BCKLT       GPIO_Pin_15

#define LCD_WIDTH		96
#define LCD_HEIGHT		65

#define LCD_STR_HEIGHT  8
#define LCD_STR_WIDTH   16

// Data sizes
#define LCD_VIDEOBUF_SIZE       864     // = 96 * 9

enum PseudoGraph_t {
    CornerTopLeftDouble = 0x99,
    CornerTopRightDouble = 0x8B,
    CornerBottomLeftDouble = 0x98,
    CornerBottomRightDouble = 0x8C,
    LineHorizDouble = 0x9D,
    LineHorizDoubleUp = 0x9A,
    LineHorizDoubleDown = 0x9B,
    LineVertDouble = 0x8A,
    LineVertDoubleLeft = 0x89,
    LineVertDoubleRight = 0x9C,
    LineCrossDouble = 0x9E,
};

enum DrawMode_t {
	DRAW, // zero has no effect
	CLEAR, // zero has no effect, one clears
	OVERWRITE, // write both zeros and ones
	OVERWRITE_INVERTED, // write both with negated meaning
	INVERT // zero has no effect, one inverts
};

class Lcd_t {
private:
    uint8_t IBuf[LCD_VIDEOBUF_SIZE];
    DrawMode_t draw_mode;
    // Pin driving functions
    void XRES_Hi(void) { LCD_GPIO->BSRR = LCD_XRES; }
    void XRES_Lo(void) { LCD_GPIO->BRR  = LCD_XRES; }
    void SCLK_Hi(void) { LCD_GPIO->BSRR = LCD_SCLK; }
    void SCLK_Lo(void) { LCD_GPIO->BRR  = LCD_SCLK; }
    void SDA_Hi (void) { LCD_GPIO->BSRR = LCD_SDA;  }
    void SDA_Lo (void) { LCD_GPIO->BRR  = LCD_SDA;  }
    void WriteCmd(uint8_t ACmd);
    void WriteData(uint8_t AData);
    // High-level
    void DrawBlock(int index, uint8_t data, uint8_t mask);
    void DrawChar(int *index, uint8_t AChar);

public:
    // IRQ
    void XCS_Hi (void) { LCD_GPIO->BSRR = LCD_XCS;  }
    void XCS_Lo (void) { LCD_GPIO->BRR  = LCD_XCS;  }
    // General use
    void Init(void);
    void Task(void);
    void Shutdown(void);
    void Backlight(uint8_t ABrightness)  { TIM15->CCR2 = ABrightness; }

    // High-level
    void SetDrawMode(DrawMode_t mode) { draw_mode = mode; }

    void Printf(int column, int row, const char *S, ...);
    void Cls(void) { for(int i=0; i < LCD_VIDEOBUF_SIZE; i++) IBuf[i] = 0x00; }
    void DrawImage(int x, int y, const uint8_t *img);

    /* ==== Pseudographics ====
     *  Every command consists of PseudoGraph_t AChar, uint8_t RepeatCount.
     *  Example: LineHorizDouble, 11 => print LineHorizDouble 11 times.
     *  Cmds end up with 0.
     */
    void Symbols(int column, int row, ...);

    void PutPixel(int x, int y, int c) {
    	int index = x + (y/8)*LCD_WIDTH;
    	uint8_t mask = 1 << (7-y%8);
		DrawBlock(index, c ? mask : 0, mask);
    }
};

extern Lcd_t Lcd;


#endif	/* LCD110X_H */

