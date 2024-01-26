#include "EFlagsModule.h"
#include "MeshService.h"
#include "configuration.h"
#include "main.h"
// #include <Adafruit_GFX.h>
// #include <FastLED_NeoMatrix.h>
// #include <FastLED.h>
// #include <LEDMatrix.h>

// Testing with NeoPixel 7 seg module
#include <Adafruit_NeoPixel.h>
#define PIN 4

#define DELAY_INTERVAL 4000 // Send a flag change command every 4 sec
#define BASE_STATION 1      // Flag stations only receive and respond to command. Only base unit sends commands

EFlagsModule *eflagsModule;
uint8_t flagState = 0;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(7, PIN, NEO_GRB + NEO_KHZ800);

// CRGB leds[NUMMATRIX];
// // Define matrix width and height.
// FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, mw, mh,
//                                                   NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
//                                                       NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);

// void display_solid(uint16_t color)
// {
//     static uint16_t bmx, bmy;

//     // Clear the space under the bitmap that will be drawn as
//     // drawing a single color pixmap does not write over pixels
//     // that are nul, and leaves the data that was underneath
//     matrix->fillRect(bmx, bmy, bmx + 8, bmy + 8, LED_BLACK);
//     matrix->drawBitmap(bmx, bmy, mono_bmp[2], 8, 8, color);
//     bmx += 8;
//     if (bmx >= mw)
//         bmx = 0;
//     if (!bmx)
//         bmy += 8;
//     if (bmy >= mh)
//         bmy = 0;
//     matrix->show();
// }

// void display_checkered()
// {
//     static uint16_t bmx, bmy;

//     // Clear the space under the bitmap that will be drawn as
//     // drawing a single color pixmap does not write over pixels
//     // that are nul, and leaves the data that was underneath
//     matrix->fillRect(bmx, bmy, bmx + 8, bmy + 8, LED_BLACK);
//     matrix->drawBitmap(bmx, bmy, mono_bmp[0], 8, 8, LED_WHITE_MEDIUM);
//     bmx += 8;
//     if (bmx >= mw)
//         bmx = 0;
//     if (!bmx)
//         bmy += 8;
//     if (bmy >= mh)
//         bmy = 0;
//     matrix->show();
// }

ProcessMessage EFlagsModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!BASE_STATION) // Base (non-Flag Station) will ignore messages TODO: change this later? so that stations send an ack when
                       // they have changed state
    {
        auto &p = mp.decoded;
        LOG_INFO("Received flags msg from=0x%0x, id=0x%x, msg=%.*s\n", mp.from, mp.id, p.payload.size, p.payload.bytes);
        char *msg = new char[80];
        sprintf(msg, "Received flags msg from=0x%0x, id=0x%x, msg=%.*s\n", mp.from, mp.id, p.payload.size, p.payload.bytes);
        screen->print(msg);
        uint16_t car_num = (p.payload.bytes[2] << 8) | p.payload.bytes[3];
        setFlagState(p.payload.bytes[0], car_num);
    }
    return ProcessMessage::STOP;
}

int32_t EFlagsModule::runOnce()
{
    if (BASE_STATION) // base station - sends flag commands
    {
        NodeNum nodenum = 0x08ab5ecc;
        uint16_t car_number = UINT16_MAX;

        if (firstTime) // first time we run, do setup stuff
        {              // for testing, we alternate between sending green flag and yellow flag
            firstTime = false;
            flagState = FLAG_NONE;
            sendFlagCommand(nodenum, FLAG_NONE, UINT16_MAX);
            LOG_INFO("Intializing Base Station");
        } else // typical function
        {
            switch (flagState) {
            case FLAG_NONE:
            case RED_FLAG:
                flagState = GREEN_FLAG;
                sendFlagCommand(nodenum, GREEN_FLAG, car_number);
                break;
            case GREEN_FLAG:
                flagState = YELLOW_FLAG;
                sendFlagCommand(nodenum, YELLOW_FLAG, car_number);
                break;
            case YELLOW_FLAG:
                flagState = BLUE_FLAG;
                sendFlagCommand(nodenum, BLUE_FLAG, car_number);
                break;
            case BLUE_FLAG:
                flagState = WHITE_FLAG;
                sendFlagCommand(nodenum, WHITE_FLAG, car_number);
                break;
            case WHITE_FLAG:
                flagState = RED_FLAG;
                sendFlagCommand(nodenum, RED_FLAG, car_number);
                break;
            default:
                break;
            }
        }
        return DELAY_INTERVAL;
    } else // flag station - init led
    {
        if (firstTime) {
            strip.begin();
            strip.setBrightness(20);
            strip.show(); // Initialize all pixels to 'off'
            setFlagState(FLAG_NONE, UINT16_MAX);
            firstTime = false;
        }
        return 0;
    }
}

void EFlagsModule::setFlagState(uint8_t state, uint16_t car_num)
{
    switch (state) {
    case FLAG_NONE:
        strip.clear();
        strip.show();
        break;
    case GREEN_FLAG:
        strip.fill(strip.Color(0, 0xFF, 0), 0, 7);
        strip.show();
        break;
    case YELLOW_FLAG:
        strip.fill(strip.Color(0x77, 0x77, 0), 0, 7);
        strip.show();
        break;
    case RED_FLAG:
        strip.fill(strip.Color(0xFF, 0, 0), 0, 7);
        strip.show();
        break;
    case BLUE_FLAG:
        strip.fill(strip.Color(0, 0, 0xFF), 0, 7);
        strip.show();
        break;
    case WHITE_FLAG:
        strip.fill(strip.Color(0xFF, 0xFF, 0xFF), 0, 7);
        strip.show();
        break;
    default:
        break;
    }
}

void EFlagsModule::sendFlagCommand(NodeNum dest, uint8_t cmd, uint16_t car_num)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    if (p) {
        p->to = dest;
        // set flag state
        p->decoded.payload.bytes[0] = cmd;
        // car_num is is two bytes
        p->decoded.payload.bytes[1] = car_num & 0xFF00;
        p->decoded.payload.bytes[2] = car_num & 0xFF;
        p->decoded.payload.size = 3;
    } else {
        p->decoded.payload.size = 1;
    }
    service.sendToMesh(p);
}
