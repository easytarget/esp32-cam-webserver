#include <HTTPClient.h>

#ifdef NO_OTA
#include <ESP32httpUpdate.h>
#define FIRMWARE_FOLDER "http://192.168.108.43:8080/win/winweb/espfw/esp32-cam/"
#define FIRMWARE_FILE "esp32_cam_last.bin"
bool need_update = false;
#endif

#define SWITCHER_COUNT 3
byte switcher_count = SWITCHER_COUNT;
const byte switcher_pin[SWITCHER_COUNT] = {2, 14, 15}; // 4 - Flash

#define DHT_PIN 13

bool switcher_revert[SWITCHER_COUNT] = {true, true, true};
unsigned int switcher_wait[SWITCHER_COUNT] = {1000, 300, 300};

#include <DHT.h>
DHT dht11(DHT_PIN, DHT11);
DHT dht21(DHT_PIN, DHT21);
int8_t dht_type = 0; // 0 - None, 1 - dht 11, 2 - dht 21
int dht_interval = 0;
bool is_dht_inited = false;
float humidity, temp;  // Values read from sensor

unsigned long dht_prevMs = 0;
unsigned long dht_curMs = 0;

#define TB_SERVER  "192.168.108.43:8081"

/* Auto rename if last part of IP address is in the list */
/* Instead of ___ at the end of CAM_NAME will appear index 1,2,3...15...  from this list */
#include "vsc.h"
int camera_data_index = -1;

#ifdef NO_OTA
void update_fw(void) {

  Serial.println("Updating with " FIRMWARE_FILE "...");
  //
  stop_httpds();
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(FIRMWARE_FOLDER FIRMWARE_FILE);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      need_update = false;
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}
#endif

void switcher_init(void){
  int L0;
  for (byte i = 0 ; i < switcher_count; i++) {
    L0 = 0;
    if (switcher_revert[i]) L0 = 1;
    pinMode(switcher_pin[i], OUTPUT);
    digitalWrite(switcher_pin[i], L0);
  }
}

void switcher(byte index) {
  int L0 = 0;
  int L1 = 1;
  if (switcher_revert[index]) {
    L0 = 1;
    L1 = 0;
  }
  pinMode(switcher_pin[index], OUTPUT);
  digitalWrite(switcher_pin[index], L1);
  if (switcher_wait[index] > 0) {
    delay(switcher_wait[index]);
    digitalWrite(switcher_pin[index], L0);
  }
}

unsigned long previousMillis = 0;  // will store last temp was read
const long interval = 2000;        // interval at which to read sensor

void gettemperature(void) {

  if (dht_type) {
    if (!is_dht_inited){
      switch (dht_type) {
        case 1: dht11.begin();
           break;
        case 2: dht21.begin();
           break;
      }
      is_dht_inited = true;
      delay(1000);
    }
  
    // Wait at least 2 seconds seconds between measurements.
    // if the difference between the current time and last time you read
    // the sensor is bigger than the interval you set, read the sensor
    // Works better than delay for things happening elsewhere also
    unsigned long currentMillis = millis();
  
    if (currentMillis - previousMillis >= interval) {
      // save the last time you read the sensor
      previousMillis = currentMillis;
  
      // Reading temperature for humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
      // Check if any reads failed and exit early (to try again).
      for (int i = 0; i < 10; i++) {
        switch (dht_type) {
          case 1: humidity = dht11.readHumidity();  
                  temp = dht11.readTemperature(false);
                  break;
          case 2: humidity = dht21.readHumidity();
                  temp = dht21.readTemperature(false);
                  break;
        }        
        if (isnan(humidity) || isnan(temp) || temp > 1000 || humidity > 1000) {
          delay(500);
        } else {
          return;
        }
      }
    }

    humidity = 0;
    temp = 0;
  }
}

char tb_url[128] = "";

void handleThingsBoard(void) {
  if (dht_type && camera_data_index >= 0 && dht_interval) {            
    dht_curMs = millis();
    if (dht_prevMs == 0 || dht_curMs - dht_prevMs >= dht_interval * 1000) {
      dht_prevMs = dht_curMs;
      gettemperature();

      char buf[128];

      HTTPClient http;
      if (strlen(tb_url) == 0)
        sprintf(tb_url, "http://" TB_SERVER "/api/v1/%s/telemetry", camera_datas[camera_data_index].token);     
      http.begin(tb_url);
      http.addHeader("Content-Type", "application/json");

      sprintf(buf, "{\"temperature\": %.1f, \"humidity\": %.0f}", temp, humidity);
      int httpCode = http.POST(buf);
      if (httpCode != 200) {
          Serial.println(tb_url);
          Serial.println(buf);
          Serial.print("HTTP Response code is: ");
          Serial.println(httpCode);
      }
      http.end();
    }  
  }
}
