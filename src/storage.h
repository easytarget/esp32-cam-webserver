#ifndef storage_h
#define storage_h

#include <FS.h>

#if __has_include("../myconfig.h")
#include "../myconfig.h"
#else
#include "app_config.h"
#endif

#define STORAGE_UNITS_BT 0
#define STORAGE_UNITS_MB 2

#ifdef USE_LittleFS
#include <LITTLEFS.h>
#define FORMAT_LITTLEFS_IF_FAILED true
#define STORAGE_UNITS STORAGE_UNITS_BT
#elif defined(CAMERA_MODEL_LILYGO_T_SIMCAM)
#include "camera_pins.h"
#include "SD.h"
#define STORAGE_UNITS STORAGE_UNITS_MB
#else
#include "SD_MMC.h"
#define STORAGE_UNITS STORAGE_UNITS_MB
#endif

/**
 * @brief Storage Manager
 * Encapsulates access to the file system, which can be either external (SD card) or internal (LittleFS).
 * 
 */
class CLStorage {
    public:
        /// @brief Load a file to a String
        /// @param path file name
        /// @param s pointer to the String buffer
        /// @return OK(0) or FAIL(1)
        int readFileToString(char *path, String *s);
        
        bool init();

        /// @brief dumps the folder content to the Serial output. 
        /// @param dirname 
        /// @param levels 
        void listDir(const char * dirname, uint8_t levels);

        int getSize();
        int getUsed();
        int capacityUnits();

        File open(const String &path, const char *mode = "r", const bool create = false) {return fsStorage->open(path, mode, create);};
        bool exists(const String &path) {return fsStorage->exists(path);};
        bool remove(const String &path) {return fsStorage->remove(path);};

#ifdef USE_LittleFS
        fs::LITTLEFSFS & getFS() {return *fsStorage;};
#elif defined(CAMERA_MODEL_LILYGO_T_SIMCAM)
        fs::SDFS & getFS() {return *fsStorage;};
#else
        fs::SDMMCFS & getFS() {return *fsStorage;};
#endif        

    private:
#ifdef USE_LittleFS
        fs::LITTLEFSFS * const fsStorage = &LITTLEFS;
#elif defined(CAMERA_MODEL_LILYGO_T_SIMCAM)
        fs::SDFS * const fsStorage = &SD;
#else
        fs::SDMMCFS * const fsStorage = &SD_MMC; 
#endif

};

extern CLStorage Storage;

#endif