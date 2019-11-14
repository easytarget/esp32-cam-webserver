#include "esp_camera.h"
#include <WiFi.h>

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

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
  // I keep my settings in a seperate header file that is in my .gitignore file
  #include "mywifi.h"
#else
  // OWEN: todo
  // Leave as is to create the default accesspoint, or comment out ACCESSPOINT 
  //  line and supply your own network SSID and PASSWORD.
  //bool accessPoint = true;
  //const char ap_ssid[] = "ESP-CAM-SERVER";
  //const char ap_password[]  = "ESP-CAM-DEMO";
  const char* ssid = "my-access-point-ssid";
  const char* password = "my-access-point-password";
#endif

#include "camera_pins.h"

#ifdef LED_PIN // Do we have a LED pin?
  const int ledPin = LED_PIN;
#else 
  const int ledPin = -1;
#endif
long int  ledVal = 10; // Start with LED dim
const int ledChannel = 8; // chose a free PWM channel (some channels apparently used by camera)
const int pwmfreq = 50000; // 5Khz was very audible on my PSU, 50K better.
const int pwmresolution = 8; // duty cycle has 8 bit range
  
void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);  // OWEN: Change? ProperDebug with #ifdef & stuff..?
  Serial.println();

  if (ledPin != -1) {
    ledcSetup(ledChannel, pwmfreq, pwmresolution); // configure LED PWM channel
    ledcAttachPin(LED_PIN, ledChannel);            // attach the GPIO pin to the channel 
    ledcWrite(ledChannel, ledVal);                 // set initial value
  }

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
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void flashLED(int power)
{
  if (ledPin == -1) return; // no led.
  if (ledVal < 64) { // Only flash at power if dim
    ledcWrite(ledChannel, power);     // A flash at requested power.
    delay(5);                         // small delay
    ledcWrite(ledChannel, ledVal);    // turn the LED back to set power
  } else { // otherwise blink off
    ledcWrite(ledChannel, 0);         // blink off
    delay(5);                         // small delay
    ledcWrite(ledChannel, ledVal);    // turn the LED back to set power
  }
 }

void loop() {
  // put your main code here, to run repeatedly:
  delay(10000);
}
