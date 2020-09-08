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

// Credentials
const char* ssid = "my-ssid";
const char* password = "my-password";


// AccessPoint; uncomment to enable AP mode, 
// otherwise we will attempt to connect to an existing network.
// #define WIFI_AP_ENABLE

// AccessPoint; change the ip address (optional, default = 192.168.4.1)
// #define AP_ADDRESS 192,168,4,1

// AccessPoint; Uncomment this to force the channel number, default = 1
// #define AP_CHAN 1

// Static network settings for use when connected to existing network when DHCP is unavailable/unreliable
// You must define all three: IP, Gateway and NetMask
// #define ST_IP      192,168,0,16
// #define ST_GATEWAY 192,168,0,2 
// #define ST_NETMASK 255,255,255,0
// one or two optional DNS servers can be supplied, but these are not used by current code.
// #define ST_DNS1 192,168,0,2
// #define ST_DNS2 8,8,8,8


/*
 *  Port numbers for WebUI and Stream, defaults to 80 and 81.
 *  Uncomment and edit as appropriate
 */

// #define HTTP_PORT 80
// #define STREAM_PORT 81

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

// Uncomment to enable Face Detection (+ Recognition if desired) by default 
//  Notes: You must set DEFAULT_RESOLUTION, above, to FRAMESIZE_CIF or lower
//         Face recognition enrolements will be lost between reboots.
// #define FACE_DETECTION
// #define FACE_RECOGNITION
