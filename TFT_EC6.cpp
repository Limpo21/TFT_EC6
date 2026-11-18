#include "TFT_EC6.h"

// ========== КОНСТРУКТОР ==========
TFT_EC6::TFT_EC6(uint8_t mosi, uint8_t sck, uint8_t cs, uint8_t a0, uint8_t rst) {
    _mosi = mosi;
    _sck = sck;
    _cs = cs;
    _a0 = a0;
    _rst = rst;
    _currentColor = 0xFFFF;
    _initialized = false;
    _rotation = 0;
}

// ========== ОРИЕНТАЦИЯ ==========
void TFT_EC6::updateMadctl() {
    uint8_t madctl = 0x00;
    switch (_rotation) {
        case 0: madctl = 0x00; break;
        case 1: madctl = 0x60; break;
        case 2: madctl = 0xC0; break;
        case 3: madctl = 0xA0; break;
    }
    sendCommand(ST7735_MADCTL);
    sendData(madctl);
}

void TFT_EC6::DrawRotation(uint8_t rotation) {
    if (!_initialized) return;
    _rotation = rotation % 4;
    updateMadctl();
}

// ========== НИЗКОУРОВНЕВАЯ ОТПРАВКА ==========
void TFT_EC6::sendByte(uint8_t data, bool isData) {
    digitalWrite(_a0, isData ? HIGH : LOW);
    digitalWrite(_cs, LOW);
    
    for (int i = 7; i >= 0; i--) {
        digitalWrite(_mosi, (data >> i) & 1);
        digitalWrite(_sck, HIGH);
        delayMicroseconds(1);
        digitalWrite(_sck, LOW);
        delayMicroseconds(1);
    }
    
    digitalWrite(_cs, HIGH);
}

void TFT_EC6::sendCommand(uint8_t cmd) { sendByte(cmd, false); }
void TFT_EC6::sendData(uint8_t data) { sendByte(data, true); }
void TFT_EC6::sendData16(uint16_t data) { sendByte(data >> 8, true); sendByte(data & 0xFF, true); }

void TFT_EC6::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    sendCommand(ST7735_CASET);
    sendData16(x1);
    sendData16(x2);
    
    sendCommand(ST7735_RASET);
    sendData16(y1);
    sendData16(y2);
    
    sendCommand(ST7735_RAMWR);
}

// ========== ИНИЦИАЛИЗАЦИЯ ==========
void TFT_EC6::begin() {
    pinMode(_mosi, OUTPUT);
    pinMode(_sck, OUTPUT);
    pinMode(_cs, OUTPUT);
    pinMode(_a0, OUTPUT);
    pinMode(_rst, OUTPUT);
    
    digitalWrite(_cs, HIGH);
    digitalWrite(_a0, HIGH);
    digitalWrite(_sck, LOW);
    digitalWrite(_mosi, LOW);
    
    digitalWrite(_rst, LOW);
    delay(50);
    digitalWrite(_rst, HIGH);
    delay(150);
    
    sendCommand(ST7735_SWRESET);
    delay(150);
    
    sendCommand(ST7735_SLPOUT);
    delay(150);
    
    sendCommand(ST7735_COLMOD);
    sendData(0x05);
    delay(10);
    
    updateMadctl();
    
    sendCommand(ST7735_DISPON);
    delay(100);
    
    _initialized = true;
    DrawClear(0x000000);
}

bool TFT_EC6::StatusDisplay() { return _initialized; }

// ========== БАЗОВЫЕ ГРАФИЧЕСКИЕ ФУНКЦИИ ==========
void TFT_EC6::drawPixelRaw(uint16_t x, uint16_t y, uint16_t color) {
    if (!_initialized || x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    setAddrWindow(x, y, x, y);
    sendData16(color);
}

void TFT_EC6::DrawClear(uint32_t color) {
    if (!_initialized) return;
    FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_EC6::DrawColor(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    _currentColor = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void TFT_EC6::DrawPixel(uint16_t x, uint16_t y) {
    drawPixelRaw(x, y, _currentColor);
}

void TFT_EC6::FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if (!_initialized) return;
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    
    // Обрезка по границам
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TFT_WIDTH) w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    
    setAddrWindow(x, y, x + w - 1, y + h - 1);
    for (int32_t i = 0; i < (int32_t)w * h; i++) {
        sendData16(rgb565);
    }
}

void TFT_EC6::DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t thickness) {
    if (!_initialized) return;
    
    int dx = abs((int)x2 - (int)x1);
    int dy = abs((int)y2 - (int)y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;
    
    while (true) {
        for (int t = -thickness/2; t <= thickness/2; t++) {
            if (thickness % 2 == 0 && t == thickness/2) continue;
            if (x + t >= 0 && x + t < TFT_WIDTH && y >= 0 && y < TFT_HEIGHT) {
                drawPixelRaw(x + t, y, _currentColor);
            }
            if (thickness > 1 && x >= 0 && x < TFT_WIDTH && y + t >= 0 && y + t < TFT_HEIGHT) {
                drawPixelRaw(x, y + t, _currentColor);
            }
        }
        if (x == x2 && y == y2) break;
        
        int e2 = err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

// ========== ПРЯМОУГОЛЬНИКИ (центр = точка) ==========
void TFT_EC6::DrawRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    // Преобразуем координаты центра в левый верхний угол и размеры
    int16_t width = abs(x2 - x1);
    int16_t height = abs(y2 - y1);
    int16_t x = min(x1, x2) - width / 2;
    int16_t y = min(y1, y2) - height / 2;
    FillRect(x, y, width, height, _currentColor);
}

void TFT_EC6::DrawLineRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thickness) {
    int16_t width = abs(x2 - x1);
    int16_t height = abs(y2 - y1);
    int16_t x = min(x1, x2) - width / 2;
    int16_t y = min(y1, y2) - height / 2;
    
    DrawRectOutline(x, y, width, height, thickness, _currentColor);
}

// Вспомогательная функция для рисования полого прямоугольника
void TFT_EC6::DrawRectOutline(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t thickness, uint32_t color) {
    uint16_t oldColor = _currentColor;
    DrawColor(color);
    
    // Верхняя линия
    DrawLine(x, y, x + w, y, thickness);
    // Нижняя линия
    DrawLine(x, y + h, x + w, y + h, thickness);
    // Левая линия
    DrawLine(x, y, x, y + h, thickness);
    // Правая линия
    DrawLine(x + w, y, x + w, y + h, thickness);
    
    DrawColor(oldColor);
}

// ========== ТРЕУГОЛЬНИКИ ==========
// Закраска треугольника с помощью горизонтальных линий
void fillTriangleLow(TFT_EC6* tft, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t temp;
    if (y0 > y1) { temp = y0; y0 = y1; y1 = temp; temp = x0; x0 = x1; x1 = temp; }
    if (y0 > y2) { temp = y0; y0 = y2; y2 = temp; temp = x0; x0 = x2; x2 = temp; }
    if (y1 > y2) { temp = y1; y1 = y2; y2 = temp; temp = x1; x1 = x2; x2 = temp; }
    
    float invSlope1 = (x1 - x0) / (float)(y1 - y0);
    float invSlope2 = (x2 - x0) / (float)(y2 - y0);
    
    if (invSlope1 > invSlope2) {
        float tempSlope = invSlope1;
        invSlope1 = invSlope2;
        invSlope2 = tempSlope;
    }
    
    float curx1 = x0;
    float curx2 = x0;
    
    for (int16_t y = y0; y <= y2; y++) {
        tft->DrawLine((int16_t)curx1, y, (int16_t)curx2, y, 1);
        curx1 += invSlope1;
        curx2 += invSlope2;
    }
}

void TFT_EC6::DrawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3) {
    if (!_initialized) return;
    
    // Сортируем вершины по Y
    int16_t temp;
    if (y1 > y2) { temp = y1; y1 = y2; y2 = temp; temp = x1; x1 = x2; x2 = temp; }
    if (y1 > y3) { temp = y1; y1 = y3; y3 = temp; temp = x1; x1 = x3; x3 = temp; }
    if (y2 > y3) { temp = y2; y2 = y3; y3 = temp; temp = x2; x2 = x3; x3 = temp; }
    
    // Если высота треугольника 0 - не рисуем
    if (y1 == y3) return;
    
    // Рисуем верхнюю часть (от y1 до y2)
    if (y2 != y1) {
        float invSlope1 = (float)(x2 - x1) / (y2 - y1);
        float invSlope2 = (float)(x3 - x1) / (y3 - y1);
        float curx1 = x1;
        float curx2 = x1;
        
        for (int y = y1; y <= y2; y++) {
            DrawLine((int16_t)curx1, y, (int16_t)curx2, y, 1);
            curx1 += invSlope1;
            curx2 += invSlope2;
        }
    }
    
    // Рисуем нижнюю часть (от y2 до y3)
    if (y2 != y3) {
        float invSlope1 = (float)(x3 - x2) / (y3 - y2);
        float invSlope2 = (float)(x3 - x1) / (y3 - y1);
        float curx1 = x2;
        float curx2 = x1 + invSlope2 * (y2 - y1);
        
        for (int y = y2; y <= y3; y++) {
            DrawLine((int16_t)curx1, y, (int16_t)curx2, y, 1);
            curx1 += invSlope1;
            curx2 += invSlope2;
        }
    }
}

void TFT_EC6::DrawLineTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t thickness) {
    DrawLine(x1, y1, x2, y2, thickness);
    DrawLine(x2, y2, x3, y3, thickness);
    DrawLine(x3, y3, x1, y1, thickness);
}

/*
// ========== ОВАЛЫ И КРУГИ ==========
void TFT_EC6::DrawOval(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!_initialized) return;
    
    int16_t rx = w / 2;
    int16_t ry = h / 2;
    int16_t xc = x;
    int16_t yc = y;
    
    int16_t a2 = rx * rx;
    int16_t b2 = ry * ry;
    int16_t xc2 = xc * xc;  // для смещения центра (не используется в алгоритме Брезенхема)
    
    // Алгоритм Брезенхема для эллипса
    int16_t err = 0;
    int16_t xx = 0;
    int16_t yy = ry;
    
    err = b2 - (a2 * ry);
    
    while (a2 * (yy - 1) > b2 * (xx + 1)) {
        DrawLine(xc - xx, yc - yy, xc + xx, yc - yy, 1);
        DrawLine(xc - xx, yc + yy, xc + xx, yc + yy, 1);
        
        if (err < 0) {
            err += (2 * xx + 1) * b2;
            xx++;
        } else {
            err += (2 * xx + 1) * b2 - (2 * yy - 1) * a2;
            xx++;
            yy--;
        }
    }
    
    // Оставшаяся часть
    while (yy >= 0) {
        DrawLine(xc - xx, yc - yy, xc + xx, yc - yy, 1);
        DrawLine(xc - xx, yc + yy, xc + xx, yc + yy, 1);
        
        if (err < 0) {
            err += (2 * xx + 1) * b2;
            xx++;
        } else {
            err += (2 * xx + 1) * b2 - (2 * yy - 1) * a2;
            xx++;
            yy--;
        }
    }
}

void TFT_EC6::DrawLineOval(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t thickness) {
    if (!_initialized) return;
    
    int16_t rx = w / 2;
    int16_t ry = h / 2;
    int16_t xc = x;
    int16_t yc = y;
    
    int16_t a2 = rx * rx;
    int16_t b2 = ry * ry;
    
    // Рисуем контур эллипса с толщиной
    int16_t err = 0;
    int16_t xx = 0;
    int16_t yy = ry;
    
    err = b2 - (a2 * ry);
    
    while (a2 * (yy - 1) > b2 * (xx + 1)) {
        for (int t = -thickness/2; t <= thickness/2; t++) {
            if (thickness % 2 == 0 && t == thickness/2) continue;
            drawPixelRaw(xc + xx + t, yc - yy, _currentColor);
            drawPixelRaw(xc - xx - t, yc - yy, _currentColor);
            drawPixelRaw(xc + xx + t, yc + yy, _currentColor);
            drawPixelRaw(xc - xx - t, yc + yy, _currentColor);
        }
        
        if (err < 0) {
            err += (2 * xx + 1) * b2;
            xx++;
        } else {
            err += (2 * xx + 1) * b2 - (2 * yy - 1) * a2;
            xx++;
            yy--;
        }
    }
    
    while (yy >= 0) {
        for (int t = -thickness/2; t <= thickness/2; t++) {
            if (thickness % 2 == 0 && t == thickness/2) continue;
            drawPixelRaw(xc + xx + t, yc - yy, _currentColor);
            drawPixelRaw(xc - xx - t, yc - yy, _currentColor);
            drawPixelRaw(xc + xx + t, yc + yy, _currentColor);
            drawPixelRaw(xc - xx - t, yc + yy, _currentColor);
        }
        
        if (err < 0) {
            err += (2 * xx + 1) * b2;
            xx++;
        } else {
            err += (2 * xx + 1) * b2 - (2 * yy - 1) * a2;
            xx++;
            yy--;
        }
    }
}
*/
// ========== КРУГИ ==========

// Закрашенный круг
void TFT_EC6::DrawCircle(int16_t x, int16_t y, int16_t radius) {
    if (!_initialized) return;
    
    int16_t x0 = 0;
    int16_t y0 = radius;
    int16_t d = 3 - 2 * radius;
    
    while (y0 >= x0) {
        // Рисуем горизонтальные линии для закраски
        DrawLine(x - x0, y - y0, x + x0, y - y0, 1);
        DrawLine(x - x0, y + y0, x + x0, y + y0, 1);
        DrawLine(x - y0, y - x0, x + y0, y - x0, 1);
        DrawLine(x - y0, y + x0, x + y0, y + x0, 1);
        
        x0++;
        if (d > 0) {
            y0--;
            d = d + 4 * (x0 - y0) + 10;
        } else {
            d = d + 4 * x0 + 6;
        }
    }
}

// Полый круг (с толщиной линии)
void TFT_EC6::DrawLineCircle(int16_t x, int16_t y, int16_t radius, uint8_t thickness) {
    if (!_initialized) return;
    
    if (radius < 0) return;
    
    int16_t x0 = 0;
    int16_t y0 = radius;
    int16_t d = 3 - 2 * radius;
    
    while (y0 >= x0) {
        // Рисуем 8 точек с учётом толщины
        for (int t = -thickness/2; t <= thickness/2; t++) {
            if (thickness % 2 == 0 && t == thickness/2) continue;
            
            drawPixelRaw(x + x0 + t, y + y0, _currentColor);
            drawPixelRaw(x - x0 - t, y + y0, _currentColor);
            drawPixelRaw(x + x0 + t, y - y0, _currentColor);
            drawPixelRaw(x - x0 - t, y - y0, _currentColor);
            
            drawPixelRaw(x + y0 + t, y + x0, _currentColor);
            drawPixelRaw(x - y0 - t, y + x0, _currentColor);
            drawPixelRaw(x + y0 + t, y - x0, _currentColor);
            drawPixelRaw(x - y0 - t, y - x0, _currentColor);
        }
        
        x0++;
        if (d > 0) {
            y0--;
            d = d + 4 * (x0 - y0) + 10;
        } else {
            d = d + 4 * x0 + 6;
        }
    }
}



// ========== ШРИФТ 5x7 (ASCII 32-127) ==========
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 пробел
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x08, 0x04, 0x08, 0x10, 0x08}, // 126 ~
    {0x00, 0x00, 0x00, 0x00, 0x00}  // 127 DEL
};

// ========== ПОЛУЧЕНИЕ БИТМАПА СИМВОЛА ==========
uint8_t* TFT_EC6::getCharBitmap(char c) {
    if (c < 32 || c > 127) c = 32;
    return (uint8_t*)font5x7[c - 32];
}

void TFT_EC6::drawCharRaw(int16_t x, int16_t y, char c, int size) {
    if (!_initialized) return;

    uint8_t* bitmap = getCharBitmap(c);   // 5 байт = 5 столбцов

    const int charWidth  = 5;
    const int charHeight = 7;

    // Максимальные локальные координаты внутри символа с учётом scale
    int max_lx = charWidth  * size - 1;
    int max_ly = charHeight * size - 1;

    // Проходим по столбцам (col) и строкам (row)
    for (int col = 0; col < charWidth; col++) {
        uint8_t columnData = bitmap[col];          // байт текущего столбца
        for (int row = 0; row < charHeight; row++) {
            // Бит 0 – верхний пиксель, бит 6 – нижний (всего 7 значащих битов)
            if (columnData & (1 << row)) {
                // Масштабируем пиксель
                for (int dy = 0; dy < size; dy++) {
                    for (int dx = 0; dx < size; dx++) {
                        int16_t lx = col * size + dx;
                        int16_t ly = row * size + dy;

                        int16_t rx, ry;
                        switch (_rotation % 4) {
                            case 0:  // 0°
                                rx = lx;
                                ry = ly;
                                break;
                            case 1:  // 90° по часовой стрелке
                                rx = ly;
                                ry = max_lx - lx;        // (ширина - 1) - x
                                break;
                            case 2:  // 180°
                                rx = max_lx - lx;
                                ry = max_ly - ly;
                                break;
                            case 3:  // 270° (или -90°)
                                rx = max_ly - ly;
                                ry = lx;
                                break;
                            default:
                                rx = lx;
                                ry = ly;
                        }

                        int16_t px = x + rx;
                        int16_t py = y + ry;

                        if (px >= 0 && px < TFT_WIDTH &&
                            py >= 0 && py < TFT_HEIGHT) {
                            drawPixelRaw(px, py, _currentColor);
                        }
                    }
                }
            }
        }
    }
}

void TFT_EC6::drawChar(int16_t x, int16_t y, char c, int fontSize) {
    drawCharRaw(x, y, c, fontSize);
}

// ========== DRAWPRINTF (С УЧЁТОМ ПОВОРОТА) ==========

void TFT_EC6::DrawPrintf(int16_t x, int16_t y, const char* format, ...) {
    DrawPrintf(x, y, 1, format);
}

void TFT_EC6::DrawPrintf(int16_t x, int16_t y, int fontSize, const char* format, ...) {
    if (!_initialized) return;

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Ширина и высота одного символа с учётом поворота
    int charWidth, charHeight;
    if (_rotation % 2 == 0) {   // 0° или 180°
        charWidth  = 5 * fontSize;
        charHeight = 7 * fontSize;
    } else {                    // 90° или 270° – ширина и высота меняются местами
        charWidth  = 7 * fontSize;
        charHeight = 5 * fontSize;
    }

    int16_t cursorX = x;
    int16_t cursorY = y;

    for (int i = 0; buffer[i] != '\0'; i++) {
        char c = buffer[i];

        if (c == '\n') {
            cursorX = x;
            cursorY += charHeight + 2;
            continue;
        }

        // Перенос строки, если символ не влезает по горизонтали
        if (cursorX + charWidth > TFT_WIDTH) {
            cursorX = x;
            cursorY += charHeight + 2;
        }

        drawCharRaw(cursorX, cursorY, c, fontSize);
        cursorX += charWidth + 1;   // +1 межсимвольный отступ
    }
}
