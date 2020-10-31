#include <WiFiManager.h>
#include <Preferences.h>
#include <ESPmDNS.h>

extern void flashLED(int flashtime);

WiFiManager wifiManager;
IPAddress espIP;
IPAddress espSubnet;
IPAddress espGateway;

extern char KEY_CAM_NAME[];
extern char KEY_MMQT_HOST[];
extern char KEY_MMQT_PORT[];
extern char KEY_MMQT_USER[];
extern char KEY_MMQT_PASS[];
extern char KEY_MMQT_TOPIC[];
extern char KEY_MMQT_CERT[];
void resetWiFiConfig()
{
    wifiManager.resetSettings();
    WiFi.disconnect(true);
    delay(2000);
    ESP.restart();
}

void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.printf("Entered config mode. SSID '%s' AP: '%s'\r\n", myWiFiManager->getConfigPortalSSID().c_str(), WiFi.softAPIP().toString().c_str());
}

//callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Saving config");
}

void startWiFi()
{
    Serial.println("Starting WiFi");

    // Feedback that we are now attempting to connect
    flashLED(300);
    delay(100);
    flashLED(300);

    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.setConnectTimeout(30);
    wifiManager.setTimeout(30);

    Preferences preferences;
    preferences.begin("ESP32-CAM", false);

    String camera_name = preferences.getString(KEY_CAM_NAME, "ESP32-CAM");
    String mqtt_port = preferences.getString(KEY_MMQT_PORT, "1883");
    String mqtt_host = preferences.getString(KEY_MMQT_HOST, "");
    String mqtt_user = preferences.getString(KEY_MMQT_USER, "");
    String mqtt_pass = preferences.getString(KEY_MMQT_PASS, "");
    String mqtt_topic = preferences.getString(KEY_MMQT_TOPIC, "");
    String mqtt_cert = preferences.getString(KEY_MMQT_CERT, "");

    WiFiManagerParameter camName(KEY_CAM_NAME, "Camera Name", camera_name.c_str(), 64);
    WiFiManagerParameter mqttHost(KEY_MMQT_HOST, "MQTT Host", mqtt_host.c_str(), 64);
    WiFiManagerParameter mqttPort(KEY_MMQT_PORT, "MQTT Port", mqtt_port.c_str(), 6);
    WiFiManagerParameter mqttUser(KEY_MMQT_USER, "MQTT User", mqtt_user.c_str(), 64);
    WiFiManagerParameter mqttPass(KEY_MMQT_PASS, "MQTT Password", mqtt_pass.c_str(), 256);
    WiFiManagerParameter mqttTopic(KEY_MMQT_TOPIC, "MQTT Publish Topic", mqtt_topic.c_str(), 256);
    WiFiManagerParameter mqttCert(KEY_MMQT_CERT, "MQTT Certificate", mqtt_cert.c_str(), 4096);

    wifiManager.addParameter(&camName);
    wifiManager.addParameter(&mqttHost);
    wifiManager.addParameter(&mqttPort);
    wifiManager.addParameter(&mqttUser);
    wifiManager.addParameter(&mqttPass);
    wifiManager.addParameter(&mqttTopic);
    wifiManager.addParameter(&mqttCert);

    wifiManager.setHostname(camera_name.c_str());
    if (!wifiManager.autoConnect(camera_name.c_str()))
    {
        ESP.restart();
    }

    espIP = WiFi.localIP();
    espSubnet = WiFi.subnetMask();
    espGateway = WiFi.gatewayIP();

    preferences.putString(KEY_CAM_NAME, camName.getValue());
    preferences.putString(KEY_MMQT_PORT, mqttPort.getValue());
    preferences.putString(KEY_MMQT_HOST, mqttHost.getValue());
    preferences.putString(KEY_MMQT_USER, mqttUser.getValue());
    preferences.putString(KEY_MMQT_PASS, mqttPass.getValue());
    preferences.putString(KEY_MMQT_TOPIC, mqttTopic.getValue());
    preferences.putString(KEY_MMQT_CERT, mqttCert.getValue());

    preferences.end();

    Serial.printf("Host: '%s'\r\nUser: '%s'\r\nPass: '%s'\r\nTopic: '%s'\r\nCert: '%s'\r\n",
                  preferences.getString("mqttHost", "").c_str(),
                  preferences.getString("mqtt_user", "").c_str(),
                  preferences.getString("mqttPass", "").c_str(),
                  preferences.getString("mqttTopic", "").c_str(),
                  preferences.getString("mqttCert", "").c_str());

    if (!MDNS.begin(camera_name.c_str()))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");

    delay(500);

    Serial.println("Wifi Started");
}
