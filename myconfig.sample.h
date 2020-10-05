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
struct station
{
    const char ssid[64];      // ssid (max 64 chars)
    const char password[64];  // password (max 64 chars)
    const bool dhcp;          // dhcp
};

struct station stationList[] = {{"my_ssid","my_password", false}};

/* 
 * Extend the list above with additional SSID+Password pairs like this:
 * The third (dhcp) column controls whether that station uses dhcp; or the optional static IP settings (below)
 * Note the use of nested braces { and }, to group each entry, and commas to seperate them.

struct station stationList[] = {{"ssid1", "pass1", true},
                                {"ssid2", "pass2", true},
                                {"ssid3", "pass3", false}};
 */

/*
 * Static network settings for client mode
 * 
 * Note: These settings will be applied to all client connections where .staticIP is true
 * You must define all three: IP, Gateway and NetMask
 */
// warning - IP addresses must be seperated with commas (,) and not decimals (.)
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
 *  The AP ssid and password will be taken from the 1st entry in the stationList[] above.
 *  
 *  This is a 'fallback' mode. The remaining stationList[] is first scanned; if a matching network
 *  is found we will loop attempting to connect to that.
 *  If no matching networks are found during the initial scan the AccessPoint mode will
 *  be permanently enabled and started; no further scan+connection attempts will be made.
 *  If you only list one entry in the stationList[] you will effectively create a dedicated
 *  AccessPoint setup.
 *  
 *  ENHANCEMENT: use dhcp field to create a mdns captive portal? currently it is unused.
 */
// #define WIFI_AP_ENABLE

// AccessPoint; optionally change the ip address (default = 192.168.4.1)
// warning - IP addresses must be seperated with commas (,) and not decimals (.)
// #define AP_ADDRESS 192,168,4,1

// AccessPoint; Uncomment this to force the channel number, default = 1
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
 *  Http and Stream prefix; useful for embedding and casual snooping
 *  Prefix will be added to the beginning of the UIR path;  
 *  eg http:/192.168.1.4/  becomes http:/192.168.1.4/<HTTP_PREFIX>/ etc.
 *  You must prefix it with '/'.
 *  
 *  Handy against casual 'evesdropping', but (important!) this is
 *  not an effective privacy measure until HTTPS is salso supported!
 */
#define HTTP_PREFIX "/the/http/interface"
#define STREAM_PREFIX "/the-stream-itself"

/*
 * Camera Hardware Settings
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
 */

// Uncomment to disable the illumination lamp features
// #define LAMP_DISABLE

// Define a initial lamp setting as a percentage, defaults to 0%
// #define LAMP_DEFAULT 0

// Assume we have SPIFFS/LittleFS partition, uncomment if not
// #define NO_FS

// Uncomment to enable Face Detection (+ Recognition if desired) by default 
//  Notes: You must set DEFAULT_RESOLUTION, above, to FRAMESIZE_CIF or lower
//         Face recognition enrolements will be lost between reboots.
// #define FACE_DETECTION
// #define FACE_RECOGNITION
