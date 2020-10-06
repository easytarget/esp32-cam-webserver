/* 
 *  Rename this example to 'myconfig.h' and fill in your details.
 * 
 *  The local config is in the '.gitignore' file, which helps to keep details secret.
 */


// Give the camera a name for the web interface 
// note: this is not the network hostname
#define CAM_NAME "ESP32 camera server"


/*
 * WiFi Settings
 *
 * Note the the use of commas as seperators in IP addresses!
 */

// WiFi Credentials



/* 
 * Extend the stationList[] below with additional SSID+Password pairs.
 *  The first block defines /what/ the structure holds
 *  The second block is where our list of ssid/passwords live

struct station {
    const char ssid[64];      // - ssid (max 64 chars)
    const char password[64];  // - password (max 64 chars)
    const bool dhcp;          // - dhcp
} station stationList[] = {{"ssid1", "pass1", true},
                           {"ssid2", "pass2", true},
                           {"ssid3", "pass3", false}};

 * The first entry in the stationList[] is special, if WIFI_AP_ENABLE has been uncommented (below) 
 * it will be used for the AccessPoint ssid and password.
 * 
 * The 'dhcp' setting controls wether the station uses static IP settings (if in doubt leave 'true')
 * Note the use of nested braces '{' and '}' to group each entry, and commas ',' to seperate them.
 */
struct station stationList[] = {{"my_ssid","my_password", true}};


/*
 * Static network settings for client mode
 * 
 * Note: The same settings will be applied to all client connections where the dhcp setting is 'false'
 * You must define all three: IP, Gateway and NetMask
 */
// warning - IP addresses must be seperated with commas (,) and not decimals (.) here
// #define ST_IP      192,168,0,16
// #define ST_GATEWAY 192,168,0,2 
// #define ST_NETMASK 255,255,255,0
// One or two optional DNS servers can be supplied, but the current firmware never uses them ;-)
// #define ST_DNS1 192,168,0,2
// #define ST_DNS2 8,8,8,8

/* 
 *  AccessPoint; 
 *  
 *  Uncomment to enable AP mode; 
 *  
 */
// #define WIFI_AP_ENABLE

/*  AP Mode Notes:
 *   
 *  Once enabled the AP ssid and password will be taken from the 1st entry in the stationList[] above.
 *  
 *  If there are more entries listed they will be scanned at startup in the normal way and connected to 
 *  if they are found. AP then works as a fallback mode for when there are no 'real' networks available.
 *  
 *  Setting the 'dhcp' field to true for the AP enables a captive portal and attempts to send
 *  all visitors to the webcam page, with varying degrees of success depending on the visitors 
 *  browser and other settings.
 */
// Optionally change the AccessPoint ip address (default = 192.168.4.1)
// warning - IP addresses must be seperated with commas (,) and not decimals (.) here
// #define AP_ADDRESS 192,168,4,1

// Uncomment this to force the AccessPoint channel number, default = 1
// #define AP_CHAN 1

/*
 *  Port numbers for WebUI and Stream, defaults to 80 and 81.
 *  Uncomment and edit as appropriate
 */
// #define HTTP_PORT 80
// #define STREAM_PORT 81

/* 
 * Wifi Watchdog defines how long we spend waiting for a connection before retrying,
 * and how often we check to see if we are still connected, milliseconds
 * You may wish to increase this if your WiFi is slow at conencting,
 */
//#define WIFI_WATCHDOG 5000

/*
 * Camera Defaults
 *
 */
// Initial Reslolution, default SVGA
// available values are: FRAMESIZE_[QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)]
// #define DEFAULT_RESOLUTION FRAMESIZE_SVGA

// Hardware Horizontal Mirror, 0 or 1 (overrides default board setting)
// #define H_MIRROR 0

// Hardware Vertical Flip , 0 or 1 (overrides default board setting)
// #define V_FLIP 1

// Browser Rotation (one of: -90,0,90, default 0)
// #define CAM_ROTATION 0


/*
 * Additional Features
 * 
 */
// Default Page: uncomment to make the full control page the default, otherwise show simple viewer
// #define DEFAULT_INDEX_FULL

// Uncomment to disable the illumination lamp features
// #define LAMP_DISABLE

// Define a initial lamp setting as a percentage, defaults to 0%
// #define LAMP_DEFAULT 0

// Assume we have SPIFFS/LittleFS partition, uncomment to disable this. 
// Controls will still be shown in the UI but are inoperative.
// #define NO_FS

// Uncomment to enable Face Detection (+ Recognition if desired) by default 
//  Notes: You must set DEFAULT_RESOLUTION (above) to FRAMESIZE_CIF or lower before enabling this
//         Face recognition enrolements are currently lost between reboots.
// #define FACE_DETECTION
// #define FACE_RECOGNITION


/*
 * Camera Hardware Selectiom
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
