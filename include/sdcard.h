#ifndef _SDCARD_H
#define _SDCARD_H

#include <globals.h>
#include <stdio.h>
#include <SPI.h>

#if (HAS_SDCARD)
#if HAS_SDCARD == 1
#include <mySD.h>
#elif HAS_SDCARD == 2
#include <SD_MMC.h>
#else
#error HAS_SDCARD unknown card reader value, must be either 1 or 2
#endif
#endif

#ifdef HAS_SDS011
#include "sds011read.h"
#endif

#ifndef SDCARD_CS
#define SDCARD_CS SS
#endif

#ifndef SDCARD_MOSI
#define SDCARD_MOSI MOSI
#endif

#ifndef SDCARD_MISO
#define SDCARD_MISO MISO
#endif

#ifndef SDCARD_SCLK
#define SDCARD_SCLK SCK
#endif

// Default config for SDMMC_HOST_DEFAULT (4-bit bus width, slot 1)
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sdmmc_host.html

#ifndef SDCARD_DATA0
#define SDCARD_DATA0 2
#endif

#ifndef SDCARD_DATA1
#define SDCARD_DATA1 4
#endif

#ifndef SDCARD_DATA2
#define SDCARD_DATA2 12
#endif

#ifndef SDCARD_DATA3
#define SDCARD_DATA3 13
#endif

#define SDCARD_FILE_NAME "/paxcount.%02d"
#define SDCARD_FILE_HEADER "date, time, wifi, bluet"

#if (COUNT_ENS)
#define SDCARD_FILE_HEADER_CWA ",cwa"
#endif

bool sdcard_init(void);
void sdcardWriteData(uint16_t, uint16_t, uint16_t = 0);

#endif // _SDCARD_H
