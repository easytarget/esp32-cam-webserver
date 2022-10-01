#ifndef storage_h
#define storage_h

#include <FS.h>

#include "app_config.h"

#define STORAGE_UNITS_BT 0
#define STORAGE_UNITS_MB 2

#ifdef USE_LittleFS
#include <LITTLEFS.h>
fs::LITTLEFSFS * const fsStorage = &LITTLEFS;
#define FORMAT_LITTLEFS_IF_FAILED true
#define STORAGE_UNITS STORAGE_UNITS_BT
#else
#include "SD_MMC.h"
fs::SDMMCFS * const fsStorage = &SD_MMC; 
#define STORAGE_UNITS STORAGE_UNITS_MB
#endif




int readFileToString(char *path, String *s);

extern bool init_storage();
extern void listDir(const char * dirname, uint8_t levels);
extern int storageSize();
extern int storageUsed();
extern int capacityUnits();

#endif