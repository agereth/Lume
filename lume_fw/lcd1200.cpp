#include "lcd1200.h"
#include "lcd_font.h"
#include "string.h"
#include <stdarg.h>
#include "uart.h"

#include "kl_sprintf.h"

#if LCD_DMA_BASED
#define LCD_DMA_TX_MODE     DMA_PRIORITY_LOW | \
                            STM32_DMA_CR_MSIZE_HWORD | \
                            STM32_DMA_CR_PSIZE_HWORD | \
                            STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                            STM32_DMA_CR_DIR_M2P |    /* Direction is memory to peripheral */ \
                            STM32_DMA_CR_CIRC
#endif

Lcd_t Lcd;

void Lcd_t::Init(void) {
    BckLt.Init();
    // ==== GPIOs ====
    // Configure LCD_XRES, LCD_XCS, LCD_SCLK & LCD_SDA as Push-Pull output
    PinSetupOut(LCD_GPIO, LCD_XRES, omPushPull);
    PinSetupOut(LCD_GPIO, LCD_XCS,  omPushPull);
    PinSetupOut(LCD_GPIO, LCD_SCLK, omPushPull);
    PinSetupOut(LCD_GPIO, LCD_SDA,  omPushPull);
    // ========================= Init LCD ======================================
    SCLK_Lo();
    XCS_Hi();
    // Reset display
    XRES_Lo();
    chThdSleepMilliseconds(7);
    XRES_Hi();
    IWriteCmd(0xAF);    // display ON
    // Reset display again
    XRES_Lo();
    chThdSleepMilliseconds(7);
    XRES_Hi();
    chThdSleepMilliseconds(7);
    // Initial commands
    IWriteCmd(0xAF);    // display ON
    IWriteCmd(0xA4);    // Set normal display mode
    IWriteCmd(0x2F);    // Charge pump on
    IWriteCmd(0x40);    // Set start row address = 0

#if LCD_MIRROR_Y_AXIS
    IWriteCmd(0xC8);    // Mirror Y axis
#endif
#if LCD_MIRROR_X_AXIS
    IWriteCmd(0xA1);    // Mirror X axis
#endif
    // Set x=0, y=0
    IWriteCmd(0xB0);    // Y axis initialization
    IWriteCmd(0x10);    // X axis initialisation1
    IWriteCmd(0x00);    // X axis initialisation2
    Cls();

#if LCD_DMA_BASED // ================ Switch to USART + DMA ====================
    PinSetupAlterFunc(LCD_GPIO, LCD_SCLK, omPushPull, pudNone, AF7, ps40MHz);
    PinSetupAlterFunc(LCD_GPIO, LCD_SDA,  omPushPull, pudNone, AF7, ps40MHz);
    // ==== USART init ==== clock enabled, idle low, first edge, enable last bit pulse
    rccEnableUSART3(FALSE);
    USART3->CR1 = USART_CR1_UE;     // Enable
    USART3->BRR = Clk.APB1FreqHz / LCD_UART_SPEED;
    USART3->CR2 = USART_CR2_CLKEN | USART_CR2_LBCL; // Enable clock, enable last bit clock
    USART3->CR1 = USART_CR1_UE | USART_CR1_M | USART_CR1_TE;
    USART3->CR3 = USART_CR3_DMAT;   // Enable DMA at transmitter
    // DMA
    dmaStreamAllocate     (LCD_DMA, IRQ_PRIO_LOW, nullptr, NULL);
    dmaStreamSetPeripheral(LCD_DMA, &USART3->DR);
    dmaStreamSetMemory0   (LCD_DMA, IBuf);
    dmaStreamSetTransactionSize(LCD_DMA, LCD_VIDEOBUF_SIZE);
    dmaStreamSetMode      (LCD_DMA, LCD_DMA_TX_MODE);
    // Start transmission
    XCS_Lo();
    dmaStreamEnable(LCD_DMA);
#else

#endif
    chSemInit(&semLcd, 1);
}

void Lcd_t::Shutdown(void) {
#if LCD_DMA_BASED
    dmaStreamDisable(LCD_DMA);
#endif
    XRES_Lo();
    XCS_Lo();
    SCLK_Lo();
    SDA_Lo();
    Backlight(0);
    chSemReset(&semLcd, 1);
}

void Lcd_t::IWriteCmd(uint8_t AByte) {
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

#if !LCD_DMA_BASED
void Lcd_t::IWriteData(uint8_t AByte) {
    SCLK_Lo();
    XCS_Lo();   // Select chip
    // Send "Data" bit
    SDA_Hi();
    SCLK_Hi();
    SCLK_Lo();
    // Send byte
    for(uint8_t i=0; i<8; i++) {
        if(AByte & 0x01) SDA_Hi();
        else SDA_Lo();
        SCLK_Hi();
        SCLK_Lo();
        AByte >>= 1;
    }
    XCS_Hi();
}

void Lcd_t::GotoXY(uint8_t x, uint8_t y) {
    // ==== Row ==== only 4 LSB's are send
    IWriteCmd(0xB0 | (y & 0x0F));
    // ==== Column ====
    // b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0
    //    |<- 2nd CMD  ->|<-   1St CMD   ->|
    IWriteCmd(0x00 | (x & 0x0F));  // LCD_CHANGE_ROW_LSB == 0x00
    IWriteCmd(0x10 | ((x >> 4) & 0x07)); // LCD_CHANGE_ROW_MSB == 0x10
}
#endif

// ================================= Printf ====================================
// Prints char at current buf indx
void Lcd_t::DrawChar(uint8_t AChar, Invert_t AInvert) {
    for(uint8_t i=0; i<6; i++) {
        uint8_t b = Font_6x8_Data[AChar][i];
        if(AInvert == Inverted) b = ~b;
#if LCD_DMA_BASED
        uint16_t w = b;
        w = (w << 1) | 0x0001;
        IBuf[CurrentPosition++] = w;
        if(CurrentPosition >= LCD_VIDEOBUF_SIZE) CurrentPosition = 0;
#else
        IWriteData(b);
#endif
    }
}

static inline void FLcdPutChar(char c) { Lcd.DrawChar(c, NotInverted); }
static inline void FLcdPutCharInverted(char c) { Lcd.DrawChar(c, Inverted); }

void Lcd_t::Printf(const uint8_t x, const uint8_t y, const char *S, ...) {
    msg_t msg = chSemWait(&semLcd);
    if(msg == RDY_OK) {
        GotoCharXY(x, y);
        va_list args;
        va_start(args, S);
        kl_vsprintf(FLcdPutChar, 16, S, args);
        va_end(args);
        chSemSignal(&semLcd);
    }
}

void Lcd_t::PrintfInverted(const uint8_t x, const uint8_t y, const char *S, ...) {
    msg_t msg = chSemWait(&semLcd);
    if(msg == RDY_OK) {
        GotoCharXY(x, y);
        va_list args;
        va_start(args, S);
        kl_vsprintf(FLcdPutCharInverted, 16, S, args);
        va_end(args);
        chSemSignal(&semLcd);
    }
}

// ================================ Graphics ===================================
void Lcd_t::DrawImage(const uint8_t x, const uint8_t y, const uint8_t* Img) {
    /* TODO
    uint8_t *p = (uint8_t*)Img;
    uint16_t w;
    uint8_t Width = *p++, Height = *p++;
    for(uint8_t fy=y; fy < y+Height; fy++) {
        GotoXY(x, fy);
        for(uint8_t fx=x; fx < x+Width; fx++) {
            w = *p++;
            w = (w << 1) | 0x0001;
            IBuf[CurrentPosition++] = w;
            if(CurrentPosition >= LCD_VIDEOBUF_SIZE) continue;
        } // fx
    } // fy
    */
}

/* Composition of symbols must be terminated with '0'.
 * Example:
 * Lcd.Symbols(0, 4,
        LineHorizDouble, 7,
        LineHorizDoubleDown, 1,
        LineHorizDouble, 8,
        0);
        Lcd.Symbols(7, 6, LineVertDouble, 1, 0);
 */
void Lcd_t::Symbols(const uint8_t x, const uint8_t y, ...) {
    GotoCharXY(x, y);
    va_list Arg;
    va_start(Arg, y);    // Set pointer to last argument
    while(true) {
        uint8_t FCharCode = (uint8_t)va_arg(Arg, int32_t);
        if(FCharCode == 0) break;
        uint8_t RepeatCount = (uint8_t)va_arg(Arg, int32_t);
        for(uint8_t j=0; j<RepeatCount; j++) DrawChar(FCharCode, NotInverted);
    }
    va_end(Arg);
}

void Lcd_t::Cls() {
#if LCD_DMA_BASED
    for(uint32_t i=0; i < LCD_VIDEOBUF_SIZE; i++) IBuf[i] = 0x0001;
#else
    GotoXY(0, 0);
    for(uint32_t i=0; i < 864; i++) IWriteData(0);
#endif
}

#ifdef LCD_LARGEFONTS_H_ // ================== LargeFonts ======================
#define FNT_CHAR_BUF_SZ     9
struct {
    char Buf[FNT_CHAR_BUF_SZ];
    uint32_t Cnt;
} FntBuf;
static inline void FLcdPutFontChar(char c) { FntBuf.Buf[FntBuf.Cnt++] = c; }

void Lcd_t::PrintfFont(const uint8_t *PFont, uint8_t x, uint8_t y, const char *S, ...) {
    // Print to buf
    FntBuf.Cnt = 0;
    va_list args;
    va_start(args, S);
    kl_vsprintf(FLcdPutFontChar, FNT_CHAR_BUF_SZ, S, args);
    va_end(args);
    // Display what printed
    uint32_t height = PFont[2], MaxWidth = PFont[1];
    uint8_t FirstSymbolCode = PFont[0];
    for(uint32_t i=0; i<FntBuf.Cnt; i++) {
        char c = FntBuf.Buf[i];
        // ==== Draw char ====
        uint8_t *P = (uint8_t*)PFont + 3 + (c - FirstSymbolCode) * (MaxWidth*height + 1);  // Pointer to char
        uint32_t width = 1 + *P++;
        for(uint8_t i=0; i<width; i++) {
            for(uint8_t h=0; h<height; h++) {
                uint32_t Indx = x + i + (y + h) * 96;
                uint32_t dw = *P++;
                dw = __RBIT(dw);
                dw >>= 23;
                uint16_t w = dw | 0x0001;
                IBuf[Indx] = w;
            }
        }
        S++;
        x += width;
    }
}
#endif

