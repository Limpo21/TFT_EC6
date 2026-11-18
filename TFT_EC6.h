#ifndef TFT_EC6_H
#define TFT_EC6_H

#include <Arduino.h>
#include <stdarg.h>

// Команды ST7735
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_COLMOD  0x3A
#define ST7735_RAMWR   0x2C
#define ST7735_DISPON  0x29
#define ST7735_MADCTL  0x36
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B

// Размеры дисплея
#define TFT_WIDTH  160
#define TFT_HEIGHT 160

class TFT_EC6 {
private:
    uint8_t _mosi, _sck, _cs, _a0, _rst;
    uint16_t _currentColor;
    bool _initialized;
    uint8_t _rotation;
    
    void sendByte(uint8_t data, bool isData);
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendData16(uint16_t data);
    void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void updateMadctl();
    void drawPixelRaw(uint16_t x, uint16_t y, uint16_t color);
    
    // Шрифты
    void drawCharRaw(int16_t x, int16_t y, char c, int size);
    void drawChar(int16_t x, int16_t y, char c, int fontSize);
    uint8_t* getCharBitmap(char c);
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int size);

    
public:
    TFT_EC6(uint8_t mosi, uint8_t sck, uint8_t cs, uint8_t a0, uint8_t rst);
    
    void begin();
    bool StatusDisplay();
    
    void DrawClear(uint32_t color);
    void DrawColor(uint32_t color);
    void DrawPixel(uint16_t x, uint16_t y);
    void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t thickness);
    void DrawRotation(uint8_t rotation);
    
    // Прямоугольники
    void DrawRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void DrawLineRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thickness);
    void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
    void DrawRectOutline(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t thickness, uint32_t color);
    
    // Треугольники
    void DrawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);
    void DrawLineTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t thickness);
    
    // Круги
    void DrawCircle(int16_t x, int16_t y, int16_t radius);
    void DrawLineCircle(int16_t x, int16_t y, int16_t radius, uint8_t thickness);
    
    // Текст
    void DrawPrintf(int16_t x, int16_t y, const char* format, ...);
    void DrawPrintf(int16_t x, int16_t y, int fontSize, const char* format, ...);
};

#endif
