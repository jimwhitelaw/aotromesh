#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <SSD1306.h>

// OLED pins for LilyGO TTGO LorA32 v2.1 1.6
#define OLED_SDA 21
#define OLED_SCL 22
// #undef OLED_RST
// #define OLED_RST 23

void setupOLEDDisplay();
void displayData(String text);
