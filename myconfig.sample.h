// Rename this example to 'myconfig.h' and fill in your details.
// This is in the '.gitignore' file, which helps to keep details secret.

// Wifi Credentials
const char* ssid = "my-access-point-ssid";
const char* password = "my-access-point-password";

// Optional AccessPoint setup, uncomment to enable
// #define WIFI_AP_ENABLE
// Set this to fix the accesspoint channel number if desired
// #define AP_CHAN 5

// Give the camera a name for the web interface 
//  (nb: this is not the network hostname)
#define CAM_NAME "ESP32 camera server"

// Initial rotation (one of: -90,0,90)
// #define CAM_ROTATION 0

// Uncomment to disable the led/lamp feature
// #define LAMP_DISABLE

// Uncomment one, and only one, of the lines below to select your board model.
// Remember to select the appropriate board in the Boards Manager of your IDE/toolchain
// This is not optional
#define CAMERA_MODEL_AI_THINKER       // default
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
//#define CAMERA_MODEL_M5STACK_ESP32CAM   // Originally: CAMERA_MODEL_M5STACK_NO_PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL
