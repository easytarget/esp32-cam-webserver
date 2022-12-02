#include <HTTPClient.h>
#include <ESP32httpUpdate.h>

#define SWITCH_PIN 2
#define SWITCH_WAIT 300

#define FIRMWARE_FOLDER "http://192.168.108.43:8080/win/winweb/espfw/cam205/"
#define FIRMWARE_FILE "esp32_cam205_last.bin"

#define RELAY_PIN 15
int8_t relay_on = 0;

#include <DHT.h>
#define DHT_PIN 13
DHT dht11(DHT_PIN, DHT11);
DHT dht21(DHT_PIN, DHT21);
float humidity, temp;  // Values read from sensor

void update() {

  Serial.println("Updating with " FIRMWARE_FILE "...");
  //stop_httpds();
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(FIRMWARE_FOLDER FIRMWARE_FILE);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      //need_update(false);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

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

void switcher(int waitmsec, bool revert = false) {
  int L0 = 0;
  int L1 = 1;
  if (revert) {
    L0 = 1;
    L1 = 0;
  }
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, L1);
  if (waitmsec > 0) {
    delay(waitmsec);
    digitalWrite(SWITCH_PIN, L0);
  }
}

unsigned long previousMillis = 0;  // will store last temp was read
const long interval = 2000;        // interval at which to read sensor
bool is_dht_inited = false;

void gettemperature(bool is_dht21 = false) {

  if (!is_dht_inited){
    is_dht_inited = true;
    if (is_dht21) {
      dht21.begin();
    } else {
      dht11.begin();
    }
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
      if (is_dht21) {
        humidity = dht21.readHumidity();      // Read humidity (percent)
        temp = dht21.readTemperature(false);  // Read temperature as Fahrenheit
      } else {
        humidity = dht11.readHumidity();      // Read humidity (percent)
        temp = dht11.readTemperature(false);  // Read temperature as Fahrenheit
      }

      if (isnan(humidity) || isnan(temp) || temp > 1000 || humidity > 1000) {
        delay(500);
      } else {
        return;
      }
    }

    humidity = 0;
    temp = 0;
  }
}
