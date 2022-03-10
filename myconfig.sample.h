/*
 *  Rename this example to 'myconfig.h' and fill in your details.
 *
 *  The local config is in the '.gitignore' file, which helps to keep details secret.
 */


/* Give the camera a name for the web interface */
#define CAM_NAME "ESP32 camera server"

/*
 * Give the network name
 * It will be used as the hostname in ST modes
 * This is the name the camera will advertise on the network (mdns) for services and OTA
 */
#define MDNS_NAME "esp32-cam"

/*
 *    WiFi Settings
 *
 *    For the simplest connection to an existing network
 *    just replace your ssid and password in the line below.
 */

struct station stationList[] = {{"my_ssid","my_password", true}};

/*
 * You can extend the stationList[] above with additional SSID+Password pairs

struct station stationList[] = {{"ssid1", "pass1", true},
                                {"ssid2", "pass2", true},
                                {"ssid3", "pass3", false}};

 * Note the use of nested braces '{' and '}' to group each entry, and commas ',' to separate them.
 *
 * The first entry (ssid1, above) in the stationList is special, if WIFI_AP_ENABLE has been uncommented (below)
 * it will be used for the AccessPoint ssid and password. See the comments there for more.
 *
 * The 'dhcp' setting controls whether the station uses DHCP or static IP settings; if in doubt leave 'true'
 *
 * You can also use a BSSID (eg: "2F:67:94:F5:BB:6A", a colon separated mac address string) in place of
 * the ssid to force connections to specific networks even when the ssid's collide,
 */

/* Extended WiFi Settings */

/*
 * If defined: URL_HOSTNAME will be used in place of the IP address in internal URL's
 */
// #define URL_HOSTNAME "esp32-cam"

/*
 * Static network settings for client mode
 *
 * Note: The same settings will be applied to all client connections where the dhcp setting is 'false'
 * You must define all three: IP, Gateway and NetMask
 */
// warning - IP addresses must be separated with commas (,) and not decimals (.)
// #define ST_IP      192,168,0,123
// #define ST_GATEWAY 192,168,0,2
// #define ST_NETMASK 255,255,255,0
// One or two DNS servers can be supplied, only the NTP code currently uses them
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
 *  If there are further entries listed they will be scanned at startup in the normal way and connected to
 *  if they are found. AP then works as a fallback mode for when there are no 'real' networks available.
 *
 *  Setting the 'dhcp' field to true for the AP enables a captive portal and attempts to send
 *  all visitors to the webcam page, with varying degrees of success depending on the visitors
 *  browser and other settings.
 */
// Optionally change the AccessPoint ip address (default = 192.168.4.1)
// warning - IP addresses must be separated with commas (,) and not decimals (.)
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
 * You may wish to increase this if your WiFi is slow at conencting.
 */
// #define WIFI_WATCHDOG 15000

/*
 * Over The Air firmware updates can be disabled by uncommenting the folowing line
 * When enabled the device will advertise itself using the MDNS_NAME defined above
 */
// #define NO_OTA

/*
 * OTA can be password protected to prevent the device being hijacked
 */
// #define OTA_PASSWORD "SuperVisor"

/* NTP
 *  Uncomment the following to enable the on-board clock
 *  Pick a nearby pool server from: https://www.ntppool.org/zone/@
 *  Set the GMT offset to match your timezone IN SECONDS;
 *    see https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
 *    1hr = 3600 seconds; do the math ;-)
 *    Default is CET (Central European Time), eg GMT + 1hr
 *  The DST offset is usually 1 hour (again, in seconds) if used in your country.
 */
//#define NTPSERVER "<EDIT THIS>.pool.ntp.org"
//#define NTP_GMT_OFFSET 3600
//#define NTP_DST_OFFSET 3600

/*
 * Camera Defaults
 *
 */
// Initial Reslolution, default SVGA
// available values are: FRAMESIZE_[THUMB|QQVGA|HQVGA|QVGA|CIF|HVGA|VGA|SVGA|XGA|HD|SXGA|UXGA] + [FHD|QXGA] for 3Mp Sensors; eg ov3660
// #define DEFAULT_RESOLUTION FRAMESIZE_SVGA

// Hardware Horizontal Mirror, 0 or 1 (overrides default board setting)
// #define H_MIRROR 0

// Hardware Vertical Flip , 0 or 1 (overrides default board setting)
// #define V_FLIP 1

// Browser Rotation (one of: -90,0,90, default 0)
// #define CAM_ROTATION 0

// Minimal frame duration in ms, used to limit max FPS
// max_fps = 1000/min_frame_time
// #define MIN_FRAME_TIME 500

/*
 * Additional Features
 *
 */
// Default Page: uncomment to make the full control page the default, otherwise show simple viewer
// #define DEFAULT_INDEX_FULL

// Uncomment to disable the notification LED on the module
// #define LED_DISABLE

// Uncomment to disable the illumination lamp features
// #define LAMP_DISABLE

// Define the startup lamp power setting (as a percentage, defaults to 0%)
// Saved (SPIFFS) user settings will override this
// #define LAMP_DEFAULT 0

// Assume the module used has a SPIFFS/LittleFS partition, and use that for persistent setting storage
// Uncomment to disable this this, the controls will still be shown in the UI but are inoperative.
// #define NO_FS

// Uncomment to enable camera debug info on serial by default
// #define DEBUG_DEFAULT_ON

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
// #define CAMERA_MODEL_ARDUCAM_ESP32S_UNO

// Initial Camera module bus communications frequency
// Currently defaults to 8MHz
// The post-initialisation (runtime) value can be set and edited by the user in the UI
// For clone modules that have camera module and SPIFFS startup issues try setting
// this very low (start at 2MHZ and increase):
// #define XCLK_FREQ_MHZ 2
