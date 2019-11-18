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
 *  The web UI has had minor changes to add the Lamp control when present, I have made the 
 *  'Start Stream' controls more accessible, and add feedback of the camera name/firmware.
 *  
 *  
 * note: Make sure that you have either selected ESP32 AI Thinker,
 *            or another board which has PSRAM enabled to use High resolution Modes
*/

// Select camera board model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

// Select camera module used on the board
#define CAMERA_MODULE_OV2640
//#define CAMERA_MODULE_OV3660

#if __has_include("mywifi.h")
  // I keep my settings in a seperate header file
  #include "mywifi.h"
#else
  const char* ssid = "my-access-point-ssid";
  const char* password = "my-access-point-password";
#endif

// A Name for the Camera. (can be set in wifi.h)
#ifdef CAM_NAME
  char myName[] = CAM_NAME;
#else
  char myName[] = "ESP32 camera server";
#endif

// This will be displayed to identify the firmware
char myVer[] PROGMEM = __DATE__ " @ " __TIME__;



#include "camera_pins.h"

// Status and illumination LED's
#ifdef LAMP_PIN 
  int lampVal = 0; // Current Lamp value, range 0-100, Start off
#else 
  int lampVal = -1; // disable Lamp
#endif         
int lampChannel = 7;     // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
// https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms
const int pwmIntervals = 100;  // The number of Steps between the output being on and off
float lampR;                   // The R value in the PWM graph equation (calculated in setup)

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.print("esp32-cam-webserver: ");
  Serial.println(myName);
  Serial.print("Code Built: ");
  Serial.println(myVer);

#ifdef LED_PIN  // If we have  notification LED, set it to output
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF); 
#endif

#ifdef LAMP_PIN
  ledcSetup(lampChannel, pwmfreq, pwmresolution); // configure LED PWM channel
  ledcWrite(lampChannel, lampVal);                // set initial value
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
  //initial sensors are flipped vertically and colors are a bit saturated
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

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {    // Owen, pulse LED for this.
    delay(250);
  }

  // feedback that we are connected
  flashLED(200);
  delay(100);
  flashLED(200);
  delay(100);
  flashLED(200);
 
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void flashLED(int flashtime)
{
#ifdef LED_PIN // Notification LED; If we have it; flash it.
  digitalWrite(LED_PIN, LED_ON);      // On at full power.
  delay(flashtime);               // delay
  digitalWrite(LED_PIN, LED_OFF);    // turn Off
#else
  return; // No notifcation LED, do nothing
#endif
} 


void loop() {
  // put your main code here, to run repeatedly:
  delay(10000);
}
