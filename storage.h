#include "FS.h"
#include "SPIFFS.h"
#include "esp_camera.h"
#include <ArduinoJson.h>



#define FORMAT_SPIFFS_IF_FAILED true

#define PREFERENCES_FILE "/esp32cam-preferences.json"
#define FACE_DB_FILE  "/esp32cam-facedb.json"

extern void loadPrefs(fs::FS &fs);
extern void removePrefs(fs::FS &fs);
extern void savePrefs(fs::FS &fs);
extern void filesystemStart();
