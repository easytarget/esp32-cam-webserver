#ifndef storage_h
#define storage_h

#include <FS.h>

#include "app_config.h"

#define STORAGE_UNITS_BT 0
#define STORAGE_UNITS_MB 2

#ifdef USE_LittleFS
#include <LITTLEFS.h>
#define FORMAT_LITTLEFS_IF_FAILED true
#define STORAGE_UNITS STORAGE_UNITS_BT
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
#else
        fs::SDMMCFS & getFS() {return *fsStorage;};
#endif        

    private:
#ifdef USE_LittleFS
        fs::LITTLEFSFS * const fsStorage = &LITTLEFS;
#else
        fs::SDMMCFS * const fsStorage = &SD_MMC; 
#endif

};

extern CLStorage Storage;

#endif