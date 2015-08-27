/*
 * File:   lcd110x.h
 * Author: Kreyl Laurelindo
 *
 * Created on 24 �������� 2010 �., 14:25
 */

#ifndef LCD110X_H
#define	LCD110X_H

#include "kl_lib.h"
//#include "lcd_LargeFonts.h"

#define LCD1200_VERSION     "20150827 2337"

#define LCD_DMA_BASED       FALSE

#if 1 // ========================== GPIO etc ===================================
#define LCD_GPIO            GPIOB
#define LCD_SCLK            12
#define LCD_SDA             10
#define LCD_XRES            11
#define LCD_XCS             14
// UART, DMA & Buff sz
#if LCD_DMA_BASED
#define LCD_UART_SPEED      100000
#define LCD_DMA             STM32_DMA1_STREAM2
#define LCD_VIDEOBUF_SIZE   864     // = 96 * 9
#endif
// Backlight
#define LCD_BCKLT_GPIO      GPIOB
#define LCD_BCKLT_PIN       15
#define LCD_BCKLT_TMR       TIM15
#define LCD_BCKLT_CHNL      2
#define LCD_TOP_BRIGHTNESS  100 // i.e. 100%

// LCD settings, do not change
#define LCD_WIDTH		    96
#define LCD_HEIGHT		    65

// Settings
#define LCD_MIRROR_X_AXIS   FALSE
#define LCD_MIRROR_Y_AXIS   TRUE

#define LCD_STR_HEIGHT      8
#define LCD_STR_WIDTH       16
#endif

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

enum Invert_t {NotInverted, Inverted};

class Lcd_t {
private:
    PinOutputPWM_t<LCD_TOP_BRIGHTNESS, invNotInverted, omPushPull> BckLt{LCD_BCKLT_GPIO, LCD_BCKLT_PIN, LCD_BCKLT_TMR, LCD_BCKLT_CHNL};
#if LCD_DMA_BASED
    uint16_t IBuf[LCD_VIDEOBUF_SIZE] PACKED;
    uint16_t CurrentPosition;   // x, y to place data to
    void GotoXY(uint8_t x, uint8_t y) { CurrentPosition =  x + y*96; }
    void GotoCharXY(uint8_t x, uint8_t y) { CurrentPosition =  x*6 + y*96; }
#else
    void IWriteData(uint8_t AByte);
    void GotoXY(uint8_t x, uint8_t y);
    void GotoCharXY(uint8_t x, uint8_t y) { GotoXY(x*6, y); }   // 0 <= x <= 16
#endif
    Semaphore semLcd;
    void IWriteCmd(uint8_t ACmd);
    // Pin driving functions
    void XRES_Hi() { PinSet  (LCD_GPIO, LCD_XRES); }
    void XRES_Lo() { PinClear(LCD_GPIO, LCD_XRES); }
    void SCLK_Hi() { PinSet  (LCD_GPIO, LCD_SCLK); }
    void SCLK_Lo() { PinClear(LCD_GPIO, LCD_SCLK); }
    void SDA_Hi () { PinSet  (LCD_GPIO, LCD_SDA);  }
    void SDA_Lo () { PinClear(LCD_GPIO, LCD_SDA);  }
    void XCS_Hi () { PinSet  (LCD_GPIO, LCD_XCS);  }
    void XCS_Lo () { PinClear(LCD_GPIO, LCD_XCS);  }
public:
    uint16_t Brightness;
    void Init();
    void Shutdown();
    void Backlight(uint8_t ABrightness)  { BckLt.Set(ABrightness); }
    // High-level
    void DrawChar(uint8_t AChar, Invert_t AInvert);
    void Printf(const uint8_t x, const uint8_t y, const char *S, ...);
    void PrintfInverted(const uint8_t x, const uint8_t y, const char *S, ...);
#ifdef LCD_LARGEFONTS_H_
    void PrintfFont(const uint8_t *PFont, uint8_t x, uint8_t y, const char *S, ...);
#endif
    void Cls();
    void DrawImage(const uint8_t x, const uint8_t y, const uint8_t *Img);
    // Symbols printing
    void Symbols(const uint8_t x, const uint8_t y, ...);
    // Constructor
//    Lcd_t(): CurrentPosition(0), Brightness(0) { for(uint32_t i=0; i<LCD_VIDEOBUF_SIZE; i++) IBuf[i] = 0; }
};

extern Lcd_t Lcd;


#endif	/* LCD110X_H */

