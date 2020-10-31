#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "src/version.h"
#include "myconfig.h"
#include "camera_pins.h"
#include "storage.h"
#include <WiFiClient.h>
#include <WebServer.h>

// IP address, Netmask and Gateway, populated when connected
extern IPAddress espIP;
extern IPAddress espSubnet;
extern IPAddress espGateway;

// The app and stream URLs
extern char httpURL[64];
extern char streamURL[64];
extern char camera_name[];

extern void startWiFi();
extern void startCameraServer();
extern void startStreamServer();
extern void startMQTTClient();
extern void handleMQTT();

// Primary config, or defaults.
#if __has_include("myconfig.h")
#include "myconfig.h"
#else
#warning "Using Defaults: Copy myconfig.sample.h to myconfig.h and edit that to use your own settings"
#define CAMERA_MODEL_AI_THINKER
#endif

// Sketch Info
int sketchSize;
int sketchSpace;
String sketchMD5;

// Select bvetween full and simple index as the default.
#if defined(DEFAULT_INDEX_FULL)
char default_index[] = "full";
#else
char default_index[] = "simple";
#endif

// This will be displayed to identify the firmware
char myVer[] PROGMEM = __DATE__ " @ " __TIME__;

// initial rotation
// can be set in myconfig.h
#if !defined(CAM_ROTATION)
#define CAM_ROTATION 0
#endif
int myRotation = CAM_ROTATION;

// Illumination LAMP/LED
#if defined(LAMP_DISABLE)
int lampVal = -1; // lamp is disabled in config
#elif defined(LAMP_PIN)
#if defined(LAMP_DEFAULT)
int lampVal = constrain(LAMP_DEFAULT, 0, 100); // initial lamp value, range 0-100
#else
int lampVal = 0; //default to off
#endif
#else
int lampVal = -1; // no lamp pin assigned
#endif

int lampChannel = 7;         // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;   // 50K pwm frequency
const int pwmresolution = 9; // duty cycle bit range
const int pwmMax = pow(2, pwmresolution) - 1;

#if defined(NO_FS)
bool filesystem = false;
#else
bool filesystem = true;
#endif

// Debug Data for stream and capture
#if defined(DEBUG_DEFAULT_ON)
bool debugData = true;
#else
bool debugData = false;
#endif

//defs
void flashLED(int flashtime);
void setLamp(int newVal);
void WifiSetup();

// Notification LED
void flashLED(int flashtime)
{
#ifdef LED_PIN                      // If we have it; flash it.
    digitalWrite(LED_PIN, LED_ON);  // On at full power.
    delay(flashtime);               // delay
    digitalWrite(LED_PIN, LED_OFF); // turn Off
#else
    return; // No notifcation LED, do nothing, no delay
#endif
}

// Lamp Control
void setLamp(int newVal)
{
    if (newVal != -1)
    {
        // Apply a logarithmic function to the scale.
        int brightness = round((pow(2, (1 + (newVal * 0.02))) - 2) / 6 * pwmMax);
        ledcWrite(lampChannel, brightness);
        Serial.print("Lamp: ");
        Serial.print(newVal);
        Serial.print("%, pwm = ");
        Serial.println(brightness);
    }
}
void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println("====");
    Serial.print("esp32-cam-webserver: ");
    Serial.println(camera_name);
    Serial.print("Code Built: ");
    Serial.println(myVer);
    Serial.print("Base Release: ");
    Serial.println(baseVersion);

    startWiFi();

    while (!WiFi.isConnected())
    {
        delay(50);
    }

#if defined(LED_PIN) // If we have a notification LED, set it to output
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ON);
#endif

    // Create camera config structure; and populate with hardware and other defaults
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    //init with highest supported specs to pre-allocate large buffers
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_XGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err == ESP_OK)
    {
        Serial.println("Camera init succeeded");
    }
    else
    {
        delay(100); // need a delay here or the next serial o/p gets missed
        Serial.println("Halted: Camera sensor failed to initialise");
        Serial.println("Will reboot to try again in 10s\r\n");
        delay(10000);
        ESP.restart();
    }
    sensor_t *espCamSensor = esp_camera_sensor_get();

    // Dump camera module, warn for unsupported modules.
    switch (espCamSensor->id.PID)
    {
    case OV9650_PID:
        Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation");
        break;
    case OV7725_PID:
        Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation");
        break;
    case OV2640_PID:
        Serial.println("OV2640 camera module detected");
        break;
    case OV3660_PID:
        Serial.println("OV3660 camera module detected");
        break;
    default:
        Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
    }

    // OV3660 initial sensors are flipped vertically and colors are a bit saturated
    if (espCamSensor->id.PID == OV3660_PID)
    {
        espCamSensor->set_vflip(espCamSensor, 1);       //flip it back
        espCamSensor->set_brightness(espCamSensor, 1);  //up the blightness just a bit
        espCamSensor->set_saturation(espCamSensor, -2); //lower the saturation
    }

// M5 Stack Wide has special needs
#if defined(CAMERA_MODEL_M5STACK_WIDE)
    espCamSensor->set_vflip(espCamSensor, 1);
    espCamSensor->set_hmirror(espCamSensor, 1);
#endif

// Config can override mirror and flip
#if defined(H_MIRROR)
    espCamSensor->set_hmirror(espCamSensor, H_MIRROR);
#endif
#if defined(V_FLIP)
    sespCamSensor->set_vflip(espCamSensor, V_FLIP);
#endif

// set initial frame rate
#if defined(DEFAULT_RESOLUTION)
    espCamSensor->set_framesize(espCamSensor, DEFAULT_RESOLUTION);
#else
    espCamSensor->set_framesize(espCamSensor, FRAMESIZE_SVGA);
#endif

    /*
    * Add any other defaults you want to apply at startup here:
    * uncomment the line and set the value as desired (see the comments)
    * 
    * these are defined in the esp headers here:
    * https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h#L149
    */

    if (filesystem)
    {
        filesystemStart();
        loadPrefs(SPIFFS);
        loadFaceDB(SPIFFS);
    }
    else
    {
        Serial.println("No Internal Filesystem, cannot save preferences");
    }

    // Initialise and set the lamp
    if (lampVal != -1)
    {
        ledcSetup(lampChannel, pwmfreq, pwmresolution); // configure LED PWM channel
        setLamp(lampVal);                               // set default value
        ledcAttachPin(LAMP_PIN, lampChannel);           // attach the GPIO pin to the channel
    }
    else
    {
        Serial.println("No lamp, or lamp disabled in config");
    }

    // Now we have a network we can start the two http handlers for the UI and Stream.
    startCameraServer();
    startStreamServer();

    Serial.printf("\r\nCamera Ready!\r\nUse '%s' to connect\r\n", httpURL);

    /*
        * Main camera streams running, now enable MQTT
        * */

    startMQTTClient();

    // Used when dumping status; these are slow functions, so just do them once during startup
    sketchSize = ESP.getSketchSize();
    sketchSpace = ESP.getFreeSketchSpace();
    sketchMD5 = ESP.getSketchMD5();
}

void loop()
{
    /* 
     *  Just loop forever, reconnecting Wifi As necesscary in client mode
     * The stream and URI handler processes initiated by the startCameraServer() call at the
     * end of setup() will handle the camera and UI processing from now on.
    */
    // client mode can fail; so reconnect as appropriate
    static bool warned = false;
    if (WiFi.status() == WL_CONNECTED)
    {
        // We are connected, wait a bit and re-check
        if (warned)
        {
            // Tell the user if we have just reconnected
            Serial.println("WiFi reconnected");
            warned = false;
        }
        handleMQTT();
    }
    else
    {
        // disconnected; attempt to reconnect
        if (!warned)
        {
            // Tell the user if we just disconnected
            WiFi.disconnect(); // ensures disconnect is complete, wifi scan cleared
            Serial.println("WiFi disconnected, retrying");
            warned = true;
        }
        startWiFi();
    }
}
