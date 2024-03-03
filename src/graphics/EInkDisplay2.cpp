#include "configuration.h"

#ifdef USE_EINK
#include "EInkDisplay2.h"
#include "SPILock.h"
#include "main.h"
#include <SPI.h>

/*
    The macros EINK_DISPLAY_MODEL, EINK_WIDTH, and EINK_HEIGHT are defined as build_flags in a variant's platformio.ini
    Previously, these macros were defined at the top of this file.

    For archival reasons, note that the following configurations had also been tested during this period:
    * ifdef RAK4631
        - 4.2 inch
          EINK_DISPLAY_MODEL: GxEPD2_420_M01
          EINK_WIDTH: 300
          EINK_WIDTH: 400

        - 2.9 inch
          EINK_DISPLAY_MODEL: GxEPD2_290_T5D
          EINK_WIDTH: 296
          EINK_HEIGHT: 128

        - 1.54 inch
          EINK_DISPLAY_MODEL: GxEPD2_154_M09
          EINK_WIDTH: 200
          EINK_HEIGHT: 200
*/

// Constructor
EInkDisplay::EInkDisplay(uint8_t address, int sda, int scl, OLEDDISPLAY_GEOMETRY geometry, HW_I2C i2cBus)
{
    // Set dimensions in OLEDDisplay base class
    this->geometry = GEOMETRY_RAWMODE;
    this->displayWidth = EINK_WIDTH;
    this->displayHeight = EINK_HEIGHT;

    // Round shortest side up to nearest byte, to prevent truncation causing an undersized buffer
    uint16_t shortSide = min(EINK_WIDTH, EINK_HEIGHT);
    uint16_t longSide = max(EINK_WIDTH, EINK_HEIGHT);
    if (shortSide % 8 != 0)
        shortSide = (shortSide | 7) + 1;

    this->displayBufferSize = longSide * (shortSide / 8);
}
this->displayWidth = 250;
this->displayHeight = 122;
this->displayBufferSize = 250 * (128 / 8);

#elif defined(HELTEC_WIRELESS_PAPER)
// GxEPD2_213_BN - 2.13 inch b/w 250x122
setGeometry(GEOMETRY_RAWMODE, 250, 122);
#elif defined(MAKERPYTHON)
// GxEPD2_290_T5D
setGeometry(GEOMETRY_RAWMODE, 296, 128);

#elif defined(PCA10059)

// GxEPD2_420_M01
setGeometry(GEOMETRY_RAWMODE, 300, 400);

#elif defined(M5_COREINK)

// M5Stack_CoreInk 200x200
// 1.54 inch 200x200 - GxEPD2_154_M09
setGeometry(GEOMETRY_RAWMODE, EPD_HEIGHT, EPD_WIDTH);
#elif defined(my)

// GxEPD2_290_T5D
setGeometry(GEOMETRY_RAWMODE, 296, 128);
LOG_DEBUG("GEOMETRY_RAWMODE, 296, 128\n");

#elif defined(ESP32_S3_PICO)

// GxEPD2_290_T94_V2
setGeometry(GEOMETRY_RAWMODE, EPD_WIDTH, EPD_HEIGHT);
LOG_DEBUG("GEOMETRY_RAWMODE, 296, 128\n");

#elif defined(ESP32_S3_PICO)

// GxEPD2_290_T94_V2
setGeometry(GEOMETRY_RAWMODE, EPD_WIDTH, EPD_HEIGHT);
LOG_DEBUG("GEOMETRY_RAWMODE, 296, 128\n");

#endif
// setGeometry(GEOMETRY_RAWMODE, 128, 64); // old resolution
// setGeometry(GEOMETRY_128_64); // We originally used this because I wasn't sure if rawmode worked - it does
}

// FIXME quick hack to limit drawing to a very slow rate
uint32_t lastDrawMsec;

/**
 * Force a display update if we haven't drawn within the specified msecLimit
 */
bool EInkDisplay::forceDisplay(uint32_t msecLimit)
{
    // No need to grab this lock because we are on our own SPI bus
    // concurrency::LockGuard g(spiLock);

    uint32_t now = millis();
    uint32_t sinceLast = now - lastDrawMsec;

    if (adafruitDisplay && (sinceLast > msecLimit || lastDrawMsec == 0))
        lastDrawMsec = now;
    else
        return false;

    // FIXME - only draw bits have changed (use backbuf similar to the other displays)
    for (uint32_t y = 0; y < displayHeight; y++)
    {
        for (uint32_t x = 0; x < displayWidth; x++)
        {
            // get src pixel in the page based ordering the OLED lib uses FIXME, super inefficient
            auto b = buffer[x + (y / 8) * displayWidth];
            auto isset = b & (1 << (y & 7));
            adafruitDisplay->drawPixel(x, y, isset ? GxEPD_BLACK : GxEPD_WHITE);
        }
    }

    LOG_DEBUG("Updating E-Paper... ");

#if false
    // Currently unused; rescued from commented-out line during a refactor
    // Use a meaningful macro here if variant doesn't want fast refresh

    // Full update mode (slow)
    adafruitDisplay->display(false)
#else
    // Fast update mode
    adafruitDisplay->nextPage();
#endif

#ifndef EINK_NO_HIBERNATE // Only hibernate if controller IC will preserve image memory
    // Put screen to sleep to save power (possibly not necessary because we already did poweroff inside of display)
    adafruitDisplay->hibernate();
#endif

    LOG_DEBUG("done\n");
    return true;
}

// Write the buffer to the display memory
void EInkDisplay::display(void)
{
    // We don't allow regular 'dumb' display() calls to draw on eink until we've shown
    // at least one forceDisplay() keyframe.  This prevents flashing when we should the critical
    // bootscreen (that we want to look nice)

    if (lastDrawMsec)
    {
        forceDisplay(slowUpdateMsec); // Show the first screen a few seconds after boot, then slower
    }
}

// Send a command to the display (low level function)
void EInkDisplay::sendCommand(uint8_t com)
{
    (void)com;
    // Drop all commands to device (we just update the buffer)
}

void EInkDisplay::setDetected(uint8_t detected)
{
    (void)detected;
}

// Connect to the display - variant specific
bool EInkDisplay::connect()
{
    LOG_INFO("Doing EInk init\n");

#ifdef PIN_EINK_PWR_ON
    pinMode(PIN_EINK_PWR_ON, OUTPUT);
    digitalWrite(PIN_EINK_PWR_ON, HIGH); // If we need to assert a pin to power external peripherals
#endif

#ifdef PIN_EINK_EN
    // backlight power, HIGH is backlight on, LOW is off
    pinMode(PIN_EINK_EN, OUTPUT);
    digitalWrite(PIN_EINK_EN, LOW);
#endif

#if defined(TTGO_T_ECHO)
    {
        auto lowLevel = new EINK_DISPLAY_MODEL(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RES, PIN_EINK_BUSY, SPI1);

        adafruitDisplay = new GxEPD2_BW<EINK_DISPLAY_MODEL, EINK_DISPLAY_MODEL::HEIGHT>(*lowLevel);
        adafruitDisplay->init();
        adafruitDisplay->setRotation(3);
        adafruitDisplay->setPartialWindow(0, 0, displayWidth, displayHeight);
    }
#elif defined(RAK4630) || defined(MAKERPYTHON)
    {
        if (eink_found)
        {
            auto lowLevel = new EINK_DISPLAY_MODEL(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RES, PIN_EINK_BUSY);
            adafruitDisplay = new GxEPD2_BW<EINK_DISPLAY_MODEL, EINK_DISPLAY_MODEL::HEIGHT>(*lowLevel);
            adafruitDisplay->init(115200, true, 10, false, SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
            // RAK14000 2.13 inch b/w 250x122 does actually now support partial updates
            adafruitDisplay->setRotation(3);
            // Fast refresh support for  1.54, 2.13 RAK14000 b/w , 2.9 and 4.2
            // adafruitDisplay->setRotation(1);
            adafruitDisplay->setPartialWindow(0, 0, displayWidth, displayHeight);
        }
        else
        {
            (void)adafruitDisplay;
        }
    }

#elif defined(HELTEC_WIRELESS_PAPER_V1_0) || defined(HELTEC_WIRELESS_PAPER)
    {
        // Is this a normal boot, or a wake from deep sleep?
        esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();

        // If waking from sleep, need to reverse rtc_gpio_isolate(), called in cpuDeepSleep()
        // Otherwise, SPI won't work
        if (wakeReason != ESP_SLEEP_WAKEUP_UNDEFINED)
        {
            // HSPI + other display pins
            rtc_gpio_hold_dis((gpio_num_t)PIN_EINK_SCLK);
            rtc_gpio_hold_dis((gpio_num_t)PIN_EINK_DC);
            rtc_gpio_hold_dis((gpio_num_t)PIN_EINK_RES);
            rtc_gpio_hold_dis((gpio_num_t)PIN_EINK_BUSY);
            rtc_gpio_hold_dis((gpio_num_t)PIN_EINK_CS);
            rtc_gpio_hold_dis((gpio_num_t)PIN_EINK_MOSI);
        }

        // Start HSPI
        hspi = new SPIClass(HSPI);
        hspi->begin(PIN_EINK_SCLK, -1, PIN_EINK_MOSI, PIN_EINK_CS); // SCLK, MISO, MOSI, SS

        // Enable VExt (ACTIVE LOW)
        // Unsure if called elsewhere first?
        delay(100);
        pinMode(Vext, OUTPUT);
        digitalWrite(Vext, LOW);
        delay(100);

        // Create GxEPD2 objects
        auto lowLevel = new EINK_DISPLAY_MODEL(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RES, PIN_EINK_BUSY, *hspi);
        adafruitDisplay = new GxEPD2_BW<EINK_DISPLAY_MODEL, EINK_DISPLAY_MODEL::HEIGHT>(*lowLevel);

        // Init GxEPD2
        adafruitDisplay->init();
        adafruitDisplay->setRotation(3);
    }
#elif defined(PCA10059)
    {
        auto lowLevel = new EINK_DISPLAY_MODEL(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RES, PIN_EINK_BUSY);
        adafruitDisplay = new GxEPD2_BW<EINK_DISPLAY_MODEL, EINK_DISPLAY_MODEL::HEIGHT>(*lowLevel);
        adafruitDisplay->init(115200, true, 10, false, SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
        adafruitDisplay->setRotation(3);
        adafruitDisplay->setPartialWindow(0, 0, displayWidth, displayHeight);
    }
#elif defined(M5_COREINK)
    auto lowLevel = new EINK_DISPLAY_MODEL(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RES, PIN_EINK_BUSY);
    adafruitDisplay = new GxEPD2_BW<EINK_DISPLAY_MODEL, EINK_DISPLAY_MODEL::HEIGHT>(*lowLevel);
    adafruitDisplay->init(115200, true, 40, false, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    adafruitDisplay->setRotation(0);
    adafruitDisplay->setPartialWindow(0, 0, EINK_WIDTH, EINK_HEIGHT);
#elif defined(my) || defined(ESP32_S3_PICO)
    {
        auto lowLevel = new EINK_DISPLAY_MODEL(PIN_EINK_CS, PIN_EINK_DC, PIN_EINK_RES, PIN_EINK_BUSY);
        adafruitDisplay = new GxEPD2_BW<EINK_DISPLAY_MODEL, EINK_DISPLAY_MODEL::HEIGHT>(*lowLevel);
        adafruitDisplay->init(115200, true, 40, false, SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
        adafruitDisplay->setRotation(1);
        adafruitDisplay->setPartialWindow(0, 0, EINK_WIDTH, EINK_HEIGHT);
    }
#endif

    return true;
}

// Use a mix of full refresh, fast refresh, and update skipping, to balance urgency and display health
#if defined(USE_EINK_DYNAMIC_REFRESH)

// Suggest that subsequent updates should use fast-refresh
void EInkDisplay::highPriority()
{
    isHighPriority = true;
}

// Suggest that subsequent updates should use full-refresh
void EInkDisplay::lowPriority()
{
    isHighPriority = false;
}

// Full-refresh is explicitly requested for next one update - no skipping please
void EInkDisplay::demandFullRefresh()
{
    demandingFull = true;
}

// configure display for partial-refresh
void EInkDisplay::configForPartialRefresh()
{
    // Display-specific code can go here
#if defined(PRIVATE_HW)
#else
    // Otherwise:
    adafruitDisplay->setPartialWindow(0, 0, adafruitDisplay->width(), adafruitDisplay->height());
#endif
}

// Configure display for full-refresh
void EInkDisplay::configForFullRefresh()
{
    // Display-specific code can go here
#if defined(PRIVATE_HW)
#else
    // Otherwise:
    adafruitDisplay->setFullWindow();
#endif
}

#ifdef EINK_PARTIAL_ERASURE_LIMIT
// Count black pixels in an image. Used for "erasure tracking"
int32_t EInkDisplay::countBlackPixels()
{
    int32_t blackCount = 0; // Signed, to avoid underflow when comparing
    for (uint16_t b = 0; b < (displayWidth / 8) * displayHeight; b++)
    {
        for (uint8_t i = 0; i < 7; i++)
        {
            // Check if each bit is black or white
            blackCount += (buffer[b] >> i) & 1;
        }
    }
    return blackCount;
}

// Evaluate the (rough) amount of black->white pixel change since last full refresh
bool EInkDisplay::tooManyErasures()
{
    // Ideally, we would compare the new and old buffers, to count *actual* white-to-black pixel changes
    // but that would require substantially more "code tampering"

    // Get the black pixel stats for this image
    int32_t blackCount = countBlackPixels();
    int32_t blackDifference = blackCount - prevBlackCount;

    // Update the running total of "erasures" - black pixels which have become white, since last full-refresh
    if (blackDifference < 0)
        erasedSinceFull -= blackDifference;

    // Store black pixel count for next time
    prevBlackCount = blackCount;

    // Log the running total - help devs setup new boards
    LOG_DEBUG("Dynamic Partial: erasedSinceFull=%hu, EINK_PARTIAL_ERASURE_LIMIT=%hu\n", erasedSinceFull,
              EINK_PARTIAL_ERASURE_LIMIT);

    // Check if too many pixels have been erased
    if (erasedSinceFull > EINK_PARTIAL_ERASURE_LIMIT)
        return true; // Too many
    else
        return false; // Still okay
}
#endif // ifdef EINK_PARTIAL_BRIGHTEN_LIMIT_PX

bool EInkDisplay::newImageMatchesOld()
{
    uint32_t newImageHash = 0;

    // Generate hash: sum all bytes in the image buffer
    for (uint16_t b = 0; b < (displayWidth / 8) * displayHeight; b++)
    {
        newImageHash += buffer[b];
    }

    // Compare hashes
    bool hashMatches = (newImageHash == prevImageHash);

    // Update the cached hash
    prevImageHash = newImageHash;

    // Return the comparison result
    return hashMatches;
}

// Choose between, full-refresh, fast refresh, and update skipping, to balance urgency and display health.
bool EInkDisplay::determineRefreshMode()
{
    uint32_t now = millis();
    uint32_t sinceLast = now - lastUpdateMsec;

    // If rate-limiting dropped a high-priority update:
    // promote this update, so it runs ASAP
    if (missedHighPriorityUpdate)
    {
        isHighPriority = true;
        missedHighPriorityUpdate = false;
    }

    // Abort: if too soon for a new frame (unless demanding full)
    if (!demandingFull && isHighPriority && partialRefreshCount > 0 && sinceLast < highPriorityLimitMsec)
    {
        LOG_DEBUG("Dynamic Partial: update skipped. Exceeded EINK_HIGHPRIORITY_LIMIT_SECONDS\n");
        missedHighPriorityUpdate = true;
        return false;
    }
    if (!demandingFull && !isHighPriority && !demandingFull && sinceLast < lowPriorityLimitMsec)
    {
        if (!demandingFull && !isHighPriority && !demandingFull && sinceLast < lowPriorityLimitMsec)
        {
            return false;
        }

        // If demanded full refresh: give it to them
        if (demandingFull)
            needsFull = true;

        // Check if old image (partial) should be redrawn (as full), for image quality
        if (partialRefreshCount > 0 && !isHighPriority)
            needsFull = true;

        // If too many partials, require a full-refresh (display health)
        if (partialRefreshCount >= partialRefreshLimit)
            needsFull = true;

#ifdef EINK_PARTIAL_ERASURE_LIMIT
        // Some displays struggle with erasing black pixels to white, during partial refresh
        if (tooManyErasures())
            needsFull = true;
#endif

        // If image matches
        // (Block must run, even if full already selected, to store hash for next time)
        if (newImageMatchesOld())
        {
            // (Block must run, even if full already selected, to store hash for next time)
            if (newImageMatchesOld())
            {
                // If low priority: limit rate
                // otherwise, every loop() will run the hash method
                if (!isHighPriority)
                    lastUpdateMsec = now;

                // If update is *not* for display health or image quality, skip it
                if (!needsFull)
                    return false;
            }

            // Conditions assessed - not skipping - load the appropriate config

            // If options require a full refresh
            if (!isHighPriority || needsFull)
            {
                if (fastRefreshCount > 0)
                    configForFullRefresh();

                LOG_DEBUG("Dynamic Partial: conditions met for full-refresh\n");
                partialRefreshCount = 0;
                needsFull = false;
                demandingFull = false;
                erasedSinceFull = 0; // Reset the count for EINK_PARTIAL_ERASURE_LIMIT - tracks ghosting buildup
            }

            // If options allow a fast-refresh
            else
            {
                if (fastRefreshCount == 0)
                    configForFastRefresh();

                LOG_DEBUG("Dynamic Partial: conditions met for partial-refresh\n");
                partialRefreshCount++;
            }

            lastUpdateMsec = now; // Mark time for rate limiting
            return true;          // Instruct calling method to continue with update
        }

#endif // End USE_EINK_DYNAMIC_REFRESH

#endif
