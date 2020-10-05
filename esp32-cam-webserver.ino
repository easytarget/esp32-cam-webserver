#include "esp_camera.h"
#include <WiFi.h>

/* This sketch is a extension/expansion/reork of the 'official' ESP32 Camera example
 *  sketch from Expressif:
 *  https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera/CameraWebServer
 *
 *  It is modified to allow control of Illumination LED Lamps's (present on some modules),
 *  greater feedback via a status LED, and the HTML contents are present in plain text
 *  for easy modification.
 *
 *  A camera name can now be configured, and wifi details can be stored in an optional 
 *  header file to allow easier updated of the repo.
 *
 *  The web UI has had changes to add the lamp control, rotation, a standalone viewer,
 *  more feeedback, new controls and other tweaks and changes,
 * note: Make sure that you have either selected ESP32 AI Thinker,
 *       or another board which has PSRAM enabled to use high resolution camera modes
 */


/* 
 *  FOR NETWORK AND HARDWARE SETTINGS COPY OR RENAME 'myconfig.sample.h' TO 'myconfig.h' AND EDIT THAT.
 *
 * By default this sketch will assume an AI-THINKER ESP-CAM and create
 * an accesspoint called "ESP32-CAM-CONNECT" (password: "InsecurePassword")
 *
 */

// Primary config, or defaults.
#if __has_include("myconfig.h")
    #include "myconfig.h"
#else
    #warning "Using Default Settings: "
    #warning "Copy myconfig.sample.h to myconfig.h and edit that to set your personal defaults"
    #define WIFI_AP_ENABLE
    #define CAMERA_MODEL_AI_THINKER
    struct station { const char ssid[64]; const char password[64]; const bool dhcp;} 
    stationList[] = {{"ESP32-CAM-CONNECT","InsecurePassword", false}};
#endif

// Pin Mappings
#include "camera_pins.h"

// Internal filesystem (SPIFFS)
// used for non-volatile camera settings and face DB store
#include "storage.h"

// Sketch Info
int sketchSize;
int sketchSpace;
String sketchMD5;

// IP address, Netmask and Gateway, populated when connected
IPAddress ip;
IPAddress net;
IPAddress gw;

// Declare external function from app_httpd.cpp
extern void startCameraServer(int hPort, int sPort);

// A Name for the Camera. (set in myconfig.h)
#if defined(CAM_NAME)
    char myName[] = CAM_NAME;
#else
    char myName[] = "ESP32 camera server";
#endif

// Ports for http and stream (override in myconfig.h)
#if defined(HTTP_PORT)
    int httpPort = HTTP_PORT;
#else
    int httpPort = 80;
#endif

#if defined(STREAM_PORT)
    int streamPort = STREAM_PORT;
#else
    int streamPort = 81;
#endif

#if !defined(WIFI_WATCHDOG)
    #define WIFI_WATCHDOG 5000
#endif

// The stream URL
char streamURL[64] = {"Undefined"};  // Stream URL to pass to the app.

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
        int lampVal = constrain(LAMP_DEFAULT,0,100); // initial lamp value, range 0-100
    #else
        int lampVal = 0; //default to off
    #endif
#else 
    int lampVal = -1; // no lamp pin assigned
#endif

int lampChannel = 7;           // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
const int pwmMax = pow(2,pwmresolution)-1;

#if defined(NO_FS)
    bool filesystem = false;
#else
    bool filesystem = true;
#endif

#if defined(FACE_DETECTION)
    int8_t detection_enabled = 1;
    #if defined(FACE_RECOGNITION)
        int8_t recognition_enabled = 1;
    #else
       int8_t recognition_enabled = 0;
    #endif
#else
    int8_t detection_enabled = 0;
    int8_t recognition_enabled = 0;
#endif

// Notification LED 
void flashLED(int flashtime) {
#ifdef LED_PIN                    // If we have it; flash it.
    digitalWrite(LED_PIN, LED_OFF);  // On at full power.
    delay(flashtime);               // delay
    digitalWrite(LED_PIN, LED_ON); // turn Off
#else
    return;                         // No notifcation LED, do nothing, no delay
#endif
} 

// Lamp Control
void setLamp(int newVal) {
    if (newVal != -1) {
        // Apply a logarithmic function to the scale.
        int brightness = round((pow(2,(1+(newVal*0.02)))-2)/6*pwmMax);
        ledcWrite(lampChannel, brightness);
        Serial.print("Lamp: ");
        Serial.print(newVal);
        Serial.print("%, pwm = ");
        Serial.println(brightness);
    }
}

void WifiSetup() {
    // Feedback that we are now attempting to connect
    flashLED(300);
    delay(100);
    flashLED(300);

    #if defined(WIFI_AP_ENABLE)
        #if defined(AP_ADDRESS)
            // User has specified the AP details, pre-configure AP
            IPAddress local_IP(AP_ADDRESS);
            IPAddress gateway(AP_ADDRESS);
            IPAddress subnet(255,255,255,0);
            WiFi.softAPConfig(local_IP, gateway, subnet);
        #endif
        #if defined(AP_CHAN)
            WiFi.softAP(stationList[0].ssid, stationList[0].password, AP_CHAN);
            Serial.println("Setting up Fixed Channel AccessPoint");
            Serial.print("SSID     : ");
            Serial.println(stationList[0].ssid);
            Serial.print("Password : ");
            Serial.println(stationList[0].password);
            Serial.print("Channel  : ");    
            Serial.println(AP_CHAN);
        # else
            WiFi.softAP(stationList[0].ssid, stationList[0].password);
            Serial.println("Setting up AccessPoint");
            Serial.print("SSID     : ");
            Serial.println(stationList[0].ssid);
            Serial.print("Password : ");
            Serial.println(stationList[0].password);
        #endif
        
        // find our IP details
        ip = WiFi.softAPIP();
        net = WiFi.subnetMask();
        gw = WiFi.gatewayIP();
    #else
        int stationCount = sizeof(stationList)/sizeof(stationList[0]);
        int bestStation = -1;
        long bestRSSI = -1024; 
        Serial.printf("Scanning local Wifi Stations\n");
        int stationsFound = WiFi.scanNetworks();
        if (stationsFound > 0) {
            Serial.printf("%i networks found\n", stationsFound);
            for (int i = 0; i < stationsFound; ++i) {
                // Print SSID and RSSI for each network found
                String thisSSID = WiFi.SSID(i);
                int thisRSSI = WiFi.RSSI(i);
                Serial.printf("%3i : %s (%i)", i + 1, thisSSID.c_str(), thisRSSI);
                // Scan our list of known stations.
                for (int sta = 0; sta < stationCount; sta++) {
                    if (strcmp(stationList[sta].ssid, thisSSID.c_str()) == 0) {
                        Serial.print("  -  Known!");
                        // Chose the strongest RSSI seen
                        if (thisRSSI > bestRSSI) {
                            bestStation = sta;
                            bestRSSI = thisRSSI;
                        }
                    }
                }
                Serial.println("");
            }
        }
        if (WiFi.scanComplete() == WIFI_SCAN_FAILED) {
            Serial.println("Scan Failed; no networks visible.");
            return;
        }

        if (bestStation == -1) {
            Serial.println("No known networks found.");
            return;
        }

        Serial.printf("Connecting to Wifi Network: %s\n", stationList[bestStation].ssid);
        if (stationList[bestStation].dhcp == false) {
            #if defined(ST_IP)
                Serial.println("Applying static IP settings");
                #if !defined (ST_GATEWAY)  || !defined (ST_NETMASK) 
                    #error "You must supply both Gateway and NetMask when specifying a static IP address"
                #endif
                IPAddress staticIP(ST_IP);
                IPAddress gateway(ST_GATEWAY);
                IPAddress subnet(ST_NETMASK);
                #if !defined(ST_DNS1)
                    WiFi.config(staticIP, gateway, subnet);
                #else
                    IPAddress dns1(ST_DNS1);
                #if !defined(ST_DNS2)
                    WiFi.config(staticIP, gateway, subnet, dns1);
                #else
                    IPAddress dns2(ST_DNS2);
                    WiFi.config(staticIP, gateway, subnet, dns1, dns2);
                #endif
                #endif
            #else
                Serial.println("Static IP settings requested but not defined in config, falling back to dhcp");
            #endif
        }

        // Initiate network connection request
        WiFi.begin(stationList[bestStation].ssid, stationList[bestStation].password);

        // Wait to connect, or timeout
        unsigned long start = millis(); 
        while ((millis() - start <= WIFI_WATCHDOG) && (WiFi.status() != WL_CONNECTED)) {
            delay(WIFI_WATCHDOG / 10);
            Serial.print('.');
        }

        // If we have connected, show details
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(" Client connection succeeded");

            // find our IP details
            ip = WiFi.localIP();
            net = WiFi.subnetMask();
            gw = WiFi.gatewayIP();

        } else {
            Serial.println(" Failed");
            WiFi.disconnect();   // Nothing to disconnect; but resets the WiFi scan etc.
            return;
        }
    #endif 
    Serial.printf("IP address: %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
    Serial.printf("Netmask   : %d.%d.%d.%d\n",net[0],net[1],net[2],net[3]);
    Serial.printf("Gateway   : %d.%d.%d.%d\n",gw[0],gw[1],gw[2],gw[3]);

    // Burst flash the LED to show we are connected
    for (int i = 0; i < 5; i++) {
        flashLED(80);
        delay(120);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println("====");
    Serial.print("esp32-cam-webserver: ");
    Serial.println(myName);
    Serial.print("Code Built: ");
    Serial.println(myVer);

    #if defined(LED_PIN)  // If we have a notification LED, set it to output
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
    if(psramFound()){
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    #if defined(CAMERA_MODEL_ESP_EYE)
        pinMode(13, INPUT_PULLUP);
        pinMode(14, INPUT_PULLUP);
    #endif

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err == ESP_OK) {
        Serial.println("Camera init succeeded");
    } else {
        delay(100);  // need a delay here or the next serial o/p gets missed
        Serial.println("Halted: Camera sensor failed to initialise");
        Serial.println("Will reboot to try again in 10s\n");
        delay(10000);
        ESP.restart();
    }
    sensor_t * s = esp_camera_sensor_get();

    // Dump camera module, warn for unsupported modules.
    switch (s->id.PID) {
        case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
        case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
        case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
        case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
        default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
    }

    // OV3660 initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);  //flip it back
        s->set_brightness(s, 1);  //up the blightness just a bit
        s->set_saturation(s, -2);  //lower the saturation
    }

    // M5 Stack Wide has special needs
    #if defined(CAMERA_MODEL_M5STACK_WIDE)
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
    #endif

    // Config can override mirror and flip
    #if defined(H_MIRROR)
        s->seror(s, H_MIRROR);
    #endif
    #if defined(V_FLIP)
        s->set_vflip(s, V_FLIP);
    #endif

    // set initial frame rate
    #if defined(DEFAULT_RESOLUTION)
        s->set_framesize(s, DEFAULT_RESOLUTION);
    #else
        s->set_framesize(s, FRAMESIZE_SVGA);
    #endif

    /*
    * Add any other defaults you want to apply at startup here:
    * uncomment the line and set the value as desired (see the comments)
    */

    //s->set_framesize(s, FRAMESIZE_SVGA); // FRAMESIZE_[QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)]);
    //s->set_quality(s, val);      // 10 to 63
    //s->set_brightness(s, 0);     // -2 to 2
    //s->set_contrast(s, 0);       // -2 to 2
    //s->set_saturation(s, 0);     // -2 to 2
    //s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    //s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    //s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    //s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    //s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    //s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    //s->set_ae_level(s, 0);       // -2 to 2
    //s->set_aec_value(s, 300);    // 0 to 1200
    //s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    //s->set_agc_gain(s, 0);       // 0 to 30
    //s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    //s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    //s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    //s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    //s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    //s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    //s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    //s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    //s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    // We now have camera with default init
    // check for saved preferences and apply them

    if (filesystem) {
        filesystemStart();
        loadPrefs(SPIFFS);
        loadFaceDB(SPIFFS);
    } else {
        Serial.println("No Internal Filesystem, cannot save preferences or face DB");
    }

    /* 
    * Camera setup complete; initialise the rest of the hardware.
    */

    // Initialise and set the lamp
    if (lampVal != -1) {
        ledcSetup(lampChannel, pwmfreq, pwmresolution);  // configure LED PWM channel
        setLamp(lampVal);                                // set default value
        ledcAttachPin(LAMP_PIN, lampChannel);            // attach the GPIO pin to the channel
    } else {
        Serial.println("No lamp, or lamp disabled in config");
    }

    // We need a working Wifi before we can start the http handlers
    Serial.println("Starting WiFi");
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    #if defined(WIFI_AP_ENABLE)
        WifiSetup();
    #else
        while (WiFi.status() != WL_CONNECTED) {
            // Loop until we connect
            WifiSetup();
            delay(1000);
        }
    #endif

    // Start the two http handlers for the HTTP UI and Stream.
    startCameraServer(httpPort, streamPort);
    
    // Construct the app and stream URLs
    char httpURL[64] = {"Unknown"};
    if (httpPort != 80) {
        sprintf(httpURL, "http://%d.%d.%d.%d:%d/", ip[0], ip[1], ip[2], ip[3], httpPort);
    } else {
        sprintf(httpURL, "http://%d.%d.%d.%d/", ip[0], ip[1], ip[2], ip[3]);
    }
    Serial.printf("\nCamera Ready!\nUse '%s' to connect\n", httpURL);
    // Construct the Stream URL
    sprintf(streamURL, "http://%d.%d.%d.%d:%d/", ip[0], ip[1], ip[2], ip[3], streamPort);
    Serial.printf("Stream viewer available at '%sview'\n", streamURL);
    Serial.printf("Raw stream URL is '%s'\n", streamURL);

    // Used when dumpung status; slow functions, so do them here
    sketchSize = ESP.getSketchSize();
    sketchSpace = ESP.getFreeSketchSpace();
    sketchMD5 = ESP.getSketchMD5();
}

void loop() {
    /* 
     *  Just loop forever, reconnecting Wifi As necesscary in client mode
     * The stream and URI handler processes initiated by the startCameraServer() call at the
     * end of setup() will handle the camera and UI processing from now on.
    */
    #if defined(WIFI_AP_ENABLE)
      // Accespoint is permanently up, so just loop
      delay(WIFI_WATCHDOG);
    #else
    static bool warned = false;
    if (WiFi.status() == WL_CONNECTED) {
        // We are connected, wait a bit and re-check
        if (warned) {
            // Tell the user if we have just reconnected 
            Serial.println("WiFi reconnected");
            warned = false;
        }
        delay(WIFI_WATCHDOG);
    } else {
        if (!warned) {
            // Tell the user if we just disconnected
            WiFi.disconnect();  // ensures disconnect is complete, wifi scan cleared
            Serial.println("WiFi disconnected, retrying");
            warned = true;
        }
        WifiSetup();
    }
    #endif
}
