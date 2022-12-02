#ifdef NO_OTA
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>
#define FIRMWARE_FOLDER "http://192.168.108.43:8080/win/winweb/espfw/esp32-cam/"
#define FIRMWARE_FILE "esp32_cam_last.bin"
bool need_update = false;
#endif

#define SWITCH_PIN 2
#define SWITCH_WAIT 300


#define RELAY_PIN 15
int relay_on = 0;
bool switcher_revert = false;
int switcher_wait = 300;

#include <DHT.h>
#define DHT_PIN 13
DHT dht11(DHT_PIN, DHT11);
DHT dht21(DHT_PIN, DHT21);
int8_t dht_type = 0; // 0 - None, 1 - dht 11, 2 - dht 21
bool is_dht_inited = false;
float humidity, temp;  // Values read from sensor

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

void relay(int8_t value) {
  pinMode(RELAY_PIN, OUTPUT);
  if(value == -1)
    relay_on = abs(relay_on - 1);
  else
    relay_on = value;
  if (relay_on > 0)
    relay_on = 1;
  else if (relay_on < 0)
    relay_on = 0;
  digitalWrite(RELAY_PIN, relay_on);
}

void switcher(void) {
  int L0 = 0;
  int L1 = 1;
  if (switcher_revert) {
    L0 = 1;
    L1 = 0;
  }
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, L1);
  if (switcher_wait > 0) {
    delay(switcher_wait);
    digitalWrite(SWITCH_PIN, L0);
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
