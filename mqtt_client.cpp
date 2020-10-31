#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "PubSubClient.h"
#include <Preferences.h>
#include <ESPmDNS.h>
#include <cstring>
#include "myconfig.h"
#include <string.h>

extern UBaseType_t activeClients;
extern int mqtt_port;
extern bool updating;

char KEY_CAM_NAME[] = "cam_name";
char KEY_MMQT_HOST[] = "mmqt_host";
char KEY_MMQT_PORT[] = "mmqt_port";
char KEY_MMQT_USER[] = "mmqt_user";
char KEY_MMQT_PASS[] = "mmqt_pass";
char KEY_MMQT_TOPIC[] = "mmqt_topic";
char KEY_MMQT_CERT[] = "mmqt_cert";

char camera_name[64];
char client_id[64];
char mqtt_topic[64];
const unsigned long publishPeriod = 60000;
unsigned long lastMQTTPublish = 0;

const static char *MQTT_JSON = "{ \
\"ClientId\":\"%s\", \
\"CameraName\":\"%s\", \
\"Millis\":\"%lu\", \
\"ActiveClients\":%d \
}";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void mqttconnect();
void startMQTTClient(char *host, char *topic, char *cert);

void mqttconnect()
{

    Preferences preferences;
    preferences.begin("ESP32-CAM", true);

    String tmp_camera_name = preferences.getString(KEY_CAM_NAME, "Camera Name");
    String tmp_mqtt_topic = preferences.getString(KEY_MMQT_TOPIC, "default/topic");
    int mqtt_port = preferences.getInt(KEY_MMQT_PORT, 1883);
    String mqtt_host = preferences.getString(KEY_MMQT_HOST, "");
    String mqtt_user = preferences.getString(KEY_MMQT_USER, "");
    String mqtt_pass = preferences.getString(KEY_MMQT_PASS, "");
    String mqtt_cert = preferences.getString(KEY_MMQT_CERT, "");

    preferences.end();

    if (mqtt_host == "" || mqtt_pass == "" || mqtt_user == "")
    {
        Serial.printf("Couldn't establish MQTT Connection: %s, %s, %s", mqtt_host.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
        return;
    }

    sprintf(camera_name, "%s", tmp_camera_name.c_str());
    sprintf(mqtt_topic, "%s", tmp_mqtt_topic.c_str());

    /* Configure the MQTT server with IPaddress and port. */
    Serial.printf("Querying '%s:%d'...\r\n", mqtt_host.c_str(), mqtt_port);

    IPAddress serverIp = MDNS.queryHost(mqtt_host);

    if ((uint32_t)serverIp == 0)
    {
        WiFi.hostByName(mqtt_host.c_str(), serverIp);
    }

    if ((uint32_t)serverIp == 0)
    {
        return;
    }

    Serial.printf("IP address of MQTT server: %s\r\n", serverIp.toString().c_str());

    tmp_camera_name.toUpperCase();
    char random[32] = {'-'};
    char char1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int index = 1; index < 31; index++)
    {
        random[index] = char1[rand() % (sizeof char1 - 1)];
    }
    sprintf(client_id, "%s%s", tmp_camera_name.c_str(), random);

    /* connect now */
    Serial.printf("MQTT connecting as '%s' (%s:%s)...\r\n", client_id, mqtt_host.c_str(), mqtt_pass.c_str());
    try
    {

        mqttClient.setServer(serverIp, mqtt_port);

        if (mqttClient.connect(client_id, mqtt_user.c_str(), mqtt_pass.c_str()))
        {
            Serial.printf("MQTT Connected to '%s'\r\n", mqtt_host.c_str());
        }
        else
        {
            mqttClient.disconnect();
            Serial.print("failed, status code =");
            Serial.print(mqttClient.state());
            Serial.printf("try again in %d seconds/n", 5);
        }
    }
    catch (const std::exception &e)
    {                             // reference to the base of a polymorphic object
        Serial.println(e.what()); // information from length_error printed
    }
}

void handleMQTT()
{

    if (updating)
    {
        Serial.println("Updating, killing MQTT Server");
        mqttClient.disconnect();
    }

    mqttClient.loop();

    unsigned long now = millis();

    if (lastMQTTPublish == 0 || ((now - lastMQTTPublish) >= publishPeriod))
    {
        /* if client was disconnected then try to reconnect again */
        if (!mqttClient.connected())
        {
            mqttconnect();
        }

        lastMQTTPublish = now;

        char msg[128] = "";
        snprintf(msg, 128, MQTT_JSON, client_id, camera_name, lastMQTTPublish, activeClients);
        Serial.printf("Publishing message %s\r\n", msg);
        /* publish the message */
        mqttClient.publish(mqtt_topic, msg);
    }
}

void startMQTTClient()
{
    Serial.printf("Starting MQTT Client '%s'\r\n", camera_name);

    /* Set SSL/TLS certificate */
    // wifiClient.setCACert(ca_cert);

    if (!mqttClient.connected())
    {
        mqttconnect();
    }
}
