#pragma once
#include "SinglePortModule.h"
#define BRIGHTNESS 16

// https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library
// MATRIX DECLARATION:
// Parameter 1 = width of EACH NEOPIXEL MATRIX (not total display)
// Parameter 2 = height of each matrix
// Parameter 3 = number of matrices arranged horizontally
// Parameter 4 = number of matrices arranged vertically
// Parameter 5 = pin number (most are valid)
// Parameter 6 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the FIRST MATRIX; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs WITHIN EACH MATRIX are
//     arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns WITHIN
//     EACH MATRIX proceed in the same order, or alternate lines reverse
//     direction; pick one.
//   NEO_TILE_TOP, NEO_TILE_BOTTOM, NEO_TILE_LEFT, NEO_TILE_RIGHT:
//     Position of the FIRST MATRIX (tile) in the OVERALL DISPLAY; pick
//     two, e.g. NEO_TILE_TOP + NEO_TILE_LEFT for the top-left corner.
//   NEO_TILE_ROWS, NEO_TILE_COLUMNS: the matrices in the OVERALL DISPLAY
//     are arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_TILE_PROGRESSIVE, NEO_TILE_ZIGZAG: the ROWS/COLUMS OF MATRICES
//     (tiles) in the OVERALL DISPLAY proceed in the same order for every
//     line, or alternate lines reverse direction; pick one.  When using
//     zig-zag order, the orientation of the matrices in alternate rows
//     will be rotated 180 degrees (this is normal -- simplifies wiring).
//   See example below for these values in action.

#define mw 16
#define mh 16
#define NUMMATRIX (mw * mh)

#define LED_BLACK 0

#define LED_RED_VERYLOW (3 << 11)
#define LED_RED_LOW (7 << 11)
#define LED_RED_MEDIUM (15 << 11)
#define LED_RED_HIGH (31 << 11)

#define LED_GREEN_VERYLOW (1 << 5)
#define LED_GREEN_LOW (15 << 5)
#define LED_GREEN_MEDIUM (31 << 5)
#define LED_GREEN_HIGH (63 << 5)

#define LED_BLUE_VERYLOW 3
#define LED_BLUE_LOW 7
#define LED_BLUE_MEDIUM 15
#define LED_BLUE_HIGH 31

#define LED_YELLOW_VERYLOW (LED_RED_VERYLOW + LED_GREEN_VERYLOW)
#define LED_YELLOW_LOW (LED_RED_LOW + LED_GREEN_LOW)
#define LED_YELLOW_MEDIUM (LED_RED_MEDIUM + LED_GREEN_MEDIUM)
#define LED_YELLOW_HIGH (LED_RED_HIGH + LED_GREEN_HIGH)

#define LED_PURPLE_VERYLOW (LED_RED_VERYLOW + LED_BLUE_VERYLOW)
#define LED_PURPLE_LOW (LED_RED_LOW + LED_BLUE_LOW)
#define LED_PURPLE_MEDIUM (LED_RED_MEDIUM + LED_BLUE_MEDIUM)
#define LED_PURPLE_HIGH (LED_RED_HIGH + LED_BLUE_HIGH)

#define LED_CYAN_VERYLOW (LED_GREEN_VERYLOW + LED_BLUE_VERYLOW)
#define LED_CYAN_LOW (LED_GREEN_LOW + LED_BLUE_LOW)
#define LED_CYAN_MEDIUM (LED_GREEN_MEDIUM + LED_BLUE_MEDIUM)
#define LED_CYAN_HIGH (LED_GREEN_HIGH + LED_BLUE_HIGH)

#define LED_WHITE_VERYLOW (LED_RED_VERYLOW + LED_GREEN_VERYLOW + LED_BLUE_VERYLOW)
#define LED_WHITE_LOW (LED_RED_LOW + LED_GREEN_LOW + LED_BLUE_LOW)
#define LED_WHITE_MEDIUM (LED_RED_MEDIUM + LED_GREEN_MEDIUM + LED_BLUE_MEDIUM)
#define LED_WHITE_HIGH (LED_RED_HIGH + LED_GREEN_HIGH + LED_BLUE_HIGH)

static const uint8_t PROGMEM mono_bmp[][8] = {
    {// 0: checkered 1
     B10101010, B01010101, B10101010, B01010101, B10101010, B01010101, B10101010, B01010101},

    {// 1: checkered 2
     B01010101, B10101010, B01010101, B10101010, B01010101, B10101010, B01010101, B10101010},

    {// 2: solid
     B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111},
};

class EFlagsModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    EFlagsModule() : SinglePortModule("eflags", meshtastic_PortNum_PRIVATE_APP), OSThread("eflagsModule") {}

    enum FlagState {
        FLAG_NONE,
        GREEN_FLAG,
        YELLOW_FLAG,
        BLUE_FLAG,
        WHITE_FLAG,
        BLACK_FLAG,
        RED_FLAG,
        BLACK_FLAG_ALL,
        MEATBALL_FLAG,
        SURFACE_FLAG,
        CHECKERED_FLAG,
        WAVED_YELLOW,
        DOUBLE_YELLOW,
        WAVED_WHITE,
        WHITE_AND_YELLOW,
        OPTION
    };

  protected:
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    // virtual meshtastic_MeshPacket *allocReply() override;
    virtual int32_t runOnce() override;

  private:
    void sendFlagCommand(NodeNum dest, uint8_t cmd, uint16_t car_num = UINT16_MAX);
    void setFlagState(uint8_t state, uint16_t car_num);
    bool firstTime = true;
};

extern EFlagsModule *eflagsModule;