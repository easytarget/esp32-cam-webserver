#ifndef app_config_h
#define app_config_h


/* Give the camera a name for the web interface */
#define CAM_NAME "ESP32 CAM Web Server"

/* Base application version */
#define BASE_VERSION "5.0"


/* Extended WiFi Settings */

/*
 * Wifi Watchdog defines how long we spend waiting for a connection before retrying,
 * and how often we check to see if we are still connected, milliseconds
 * You may wish to increase this if your WiFi is slow at connecting.
 */
#define WIFI_WATCHDOG 15000

/*
 * Additional Features
 *
 */

// Uncomment to disable the notification LED on the module
// #define LED_DISABLE

// Uncomment to enable MJPEG streaming support
// This mode is still under development and is not very stable / not recommended
// #define STREAM_MJPEG

// Uncomment this line to use LittleFS instead of SD. 
// NOTE!
// LittleFS is still experimental, not recommended. The 'official' library installed from the Library Manager 
// seems to be broken, but fixed in this PR: https://github.com/lorol/LITTLEFS/pull/56 
// To install it, please navigate to you /libraries sub-folder of your sketch location and then execute 
// git clone https://github.com/Michael2MacDonald/LITTLEFS.

// #define USE_LittleFS

/*
 * Camera Hardware Selection
 *
 * You must uncomment one, and only one, of the lines below to select your board model.
 * Remember to also select the board in the Boards Manager
 * This is not optional
 */
#define CAMERA_MODEL_AI_THINKER       // default
// #define CAMERA_MODEL_WROVER_KIT
// #define CAMERA_MODEL_ESP_EYE
// #define CAMERA_MODEL_M5STACK_PSRAM
// #define CAMERA_MODEL_M5STACK_V2_PSRAM
// #define CAMERA_MODEL_M5STACK_WIDE
// #define CAMERA_MODEL_M5STACK_ESP32CAM   // Originally: CAMERA_MODEL_M5STACK_NO_PSRAM
// #define CAMERA_MODEL_TTGO_T_JOURNAL
// #define CAMERA_MODEL_ARDUCAM_ESP32S_UNO


#endif
