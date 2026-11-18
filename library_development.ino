#include "TFT_EC6.h"

//           mosi, sck, cs,  a0,  rst
TFT_EC6 tft(5, 7, 3, 15, 14);

void setup() {
    int a = 52;
    float b = 6.7;
    Serial.begin(115200);
    
    Serial.println("TFT Init...");
    tft.begin();
    tft.DrawRotation(4);
    tft.DrawColor(0x00FF00);
    tft.DrawLine(10, 40, 70, 40, 4);
    tft.DrawLine(70, 40, 60, 30, 4);
    tft.DrawLine(70, 40, 60, 50, 4);
    
    tft.DrawColor(0xFFFF00);
    tft.DrawLineRect(10, 80, 30, 60, 2);
    tft.DrawRect(14, 76, 26, 56);

    tft.DrawColor(0xFF0000);
    tft.DrawLineTriangle(40, 80, 60, 80, 50, 60, 3);
    tft.DrawTriangle(45, 67, 55, 63, 48, 54);

    tft.DrawColor(0x0000FF);
    tft.DrawLineCircle(30, 120, 6, 3);
    tft.DrawCircle(30, 120, 3);

    delay(5000);
    
    tft.DrawClear(0x000000);
    tft.DrawColor(0x00FF00);
    tft.DrawPrintf(10, 10, 1, "Hello World \na=%d b=%.2f", a, b);
    
    delay(15000);
}

void loop() {}
