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
 *  The web UI has had minor changes to add the lamp control when present, I have made the 
 *  'Start Stream' controls more accessible, and add feedback of the camera name/firmware.
 *  
 * note: Make sure that you have either selected ESP32 AI Thinker,
 *       or another board which has PSRAM enabled to use high resolution camera modes
 */


/* 
 *  FOR NETWORK AND HARDWARE SETTINGS COPY OR RENAME 'myconfig.sample.h' to 'myconfig.h' AND EDIT THAT.
 *
 * By default this sketch will assume an AI-THINKER ESP-CAM and create 
 * an accesspoint called "ESP32-CAM-CONNECT" (password: "InsecurePassword")
 *
 */

#if __has_include("myconfig.h")
  // I keep my settings in a seperate header file
  #include "myconfig.h"
#else
  // These are the defaults.. dont edit these.
  // copy myconfig.sample.h to myconfig.h and edit that instead
  //  SSID, Password and Mode
  const char* ssid = "ESP32-CAM-CONNECT";
  const char* password = "InsecurePassword";
  #define WIFI_AP_ENABLE
  // Default Board and Camera:
  #define CAMERA_MODEL_AI_THINKER
#endif

// A Name for the Camera. (can be set in myconfig.h)
#ifdef CAM_NAME
  char myName[] = CAM_NAME;
#else
  char myName[] = "ESP32 camera server";
#endif

// This will be displayed to identify the firmware
char myVer[] PROGMEM = __DATE__ " @ " __TIME__;

// current rotation direction
char myRotation[5];

#include "camera_pins.h"

// Illumination LED's
#ifdef LAMP_DISABLE
  int lampVal = -1; // lamp disabled by config
#elif LAMP_PIN 
  int lampVal = 0; // current lamp value, range 0-100, default off
#else 
  int lampVal = -1; // no lamp pin assigned
#endif         

int lampChannel = 7;           // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
// https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms
const int pwmIntervals = 100;  // The number of Steps between the output being on and off
float lampR;                   // The R value in the PWM graph equation (calculated in setup)

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("====");
  Serial.print("esp32-cam-webserver: ");
  Serial.println(myName);
  Serial.print("Code Built: ");
  Serial.println(myVer);

  // initial rotation
  // can be set in myconfig.h
  #ifndef CAM_ROTATION
    #define CAM_ROTATION 0
  #endif

  // set the initialisation for image rotation
  // ToDo; might be better to handle this with an enum?
  int n __attribute__((unused)) = snprintf(myRotation,sizeof(myRotation),"%d",CAM_ROTATION); 


#ifdef LED_PIN  // If we have a notification LED set it to output
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF);
#endif

#ifdef LAMP_PIN
  // ledcXXX functions are esp32 core pwm control functions
  ledcSetup(lampChannel, pwmfreq, pwmresolution); // configure LED PWM channel
  ledcWrite(lampChannel, 0);                      // Off by default
  ledcAttachPin(LAMP_PIN, lampChannel);           // attach the GPIO pin to the channel 
  // Calculate the PWM scaling R factor: 
  // https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms
  lampR = (pwmIntervals * log10(2))/(log10(pow(2,pwmresolution)));
#endif

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
  //init with high specs to pre-allocate larger buffers
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
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // Dump camera module, warn for unsupported modules.
  switch (s->id.PID) {
    case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
    case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
    case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
    case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
    // case OV5640_PID: Serial.println("WARNING: OV5640 camera module is not properly supported, will fallback to OV2640 operation"); break;
    default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
  }

  // OV3660 initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }

  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_SVGA);

  #if defined(CAMERA_MODEL_M5STACK_WIDE)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  #endif

  // Feedback that hardware init is complete and we are now attempting to connect
  Serial.println("Wifi Initialisation");
  flashLED(400);
  delay(100);

#ifdef WIFI_AP_ENABLE
  #ifdef AP_CHAN
    WiFi.softAP(ssid, password, AP_CHAN);
    Serial.println("Setting up Fixed Channel AccessPoint");
    Serial.print("SSID     : ");
    Serial.println(ssid);
    Serial.print("Password : ");
    Serial.println(password);
    Serial.print("Channel  : ");    
    Serial.println(AP_CHAN);
  # else
    WiFi.softAP(ssid, password);
    Serial.println("Setting up AccessPoint");
    Serial.print("SSID     : ");
    Serial.println(ssid);
    Serial.print("Password : ");
    Serial.println(password);
  #endif
#else
  Serial.print("Connecting to Wifi Network: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);  // Wait for Wifi to connect. If this fails wifi the code basically hangs here.
                 // - It would be good to do something else here as a future enhancement.
                 //   (eg: go to a captive AP config portal to configure the wifi)
  }
#endif

  // feedback that we are connected
  Serial.println("WiFi connected");
  flashLED(200);
  delay(100);
  flashLED(200);
  delay(100);
  flashLED(200);

  // Start the Stream server, and the handler processes for the Web UI.
  startCameraServer();

  Serial.println();
  Serial.print("Camera Ready!  Use 'http://");
#ifdef WIFI_AP_ENABLE
  Serial.print(WiFi.softAPIP());
#else
  Serial.print(WiFi.localIP());
#endif
  Serial.println("' to connect");
  Serial.println();
}

// Notification LED 
void flashLED(int flashtime)
{
#ifdef LED_PIN                    // If we have it; flash it.
  digitalWrite(LED_PIN, LED_ON);  // On at full power.
  delay(flashtime);               // delay
  digitalWrite(LED_PIN, LED_OFF); // turn Off
#else
  return;                         // No notifcation LED, do nothing, no delay
#endif
} 


void loop() {
  // Just loop forever.
  // The stream and URI handler processes initiated by the startCameraServer() call at the
  // end of setup() will handle the camera and UI processing from now on.
  delay(10000);
}
