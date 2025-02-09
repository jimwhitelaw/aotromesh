#include "oledHelper.h"
// Create display for OLED
SSD1306 display(0x3c, OLED_SDA, OLED_SCL);

void setupOLEDDisplay()
{
    // LOG_INFO("Starting OLED display...");
    // Configure OLED by setting the OLED Reset HIGH, LOW, and then back HIGH
    // pinMode(OLED_RST, OUTPUT);
    // digitalWrite(OLED_RST, HIGH);
    // delay(100);
    // digitalWrite(OLED_RST, LOW);
    // delay(100);
    // digitalWrite(OLED_RST, HIGH);
    display.init();
    display.flipScreenVertically();
    delay(200);
    display.clear();
    display.display();
    // Serial.println("OLED init complete.");
}

void displayData(String text)
{
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, text);
    display.display();
}

void displayLowerData(String text)
{
    // display.clear();
    display.setColor(BLACK);
    display.fillRect(0, 34, 128, 11);
    display.display();
    display.setColor(WHITE);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 37, text);
    display.display();
}