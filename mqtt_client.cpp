#include "Arduino.h"
#include <WiFiClientSecure.h>
#include "PubSubClient.h"
#include <ESPmDNS.h>
#include <cstring>

extern UBaseType_t activeClients;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
uint16_t mqtt_port = 1883;
char *mqtt_host = new char[64];
char *mqtt_pub_topic = new char[256];
char *mqtt_client_id = new char[64];
const char *MQTT_JSON = "{\"ClientId\":\"%s\",\"ActiveClients\":%d}";

void startMQTTClient(char *host, char *topic, char *cert);
void mqttconnect();
void mqttCB(void *pvParameters);

TaskHandle_t tMQTT; // handles client connection to the MQTT Server
void mqttCB(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = pdMS_TO_TICKS(60000); //Every minute

    while (true)
    {
        /* if client was disconnected then try to reconnect again */
        if (!mqttClient.connected())
        {
            mqttconnect();
        }

        Serial.printf("ActiveClients %d\n", activeClients);

        char msg[64] = "";
        snprintf(msg, 64, MQTT_JSON, mqtt_client_id, activeClients);
        Serial.printf("Publishing message %s\n", msg);
        /* publish the message */
        mqttClient.publish(mqtt_pub_topic, msg);

        //  Let other tasks run after serving every client
        taskYIELD();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void mqttconnect()
{
    u32_t defaultIp = 0;
    IPAddress noConnIP = IPAddress(defaultIp);
    IPAddress serverIp = IPAddress(defaultIp);
    unsigned long period = 5000;
    unsigned long lastRun = 0;

    /* Loop until reconnected */
    while (!mqttClient.connected())
    {

        if (millis() - lastRun < period)
        {
            yield();
            continue;
        }

        lastRun = millis();

        /* Configure the MQTT server with IPaddress and port. */
        while (noConnIP == serverIp)
        {
            Serial.printf("Querying '%s:%d'...\n", mqtt_host, mqtt_port);

            serverIp = MDNS.queryHost(mqtt_host);

            Serial.printf("IP address of server: %s\n", serverIp.toString().c_str());

            if (noConnIP == serverIp)
            {
                WiFi.hostByName(mqtt_host, serverIp);
                Serial.printf("IP address of server: %s\n", serverIp.toString().c_str());
                if ((uint32_t)serverIp == 0)
                {
                    serverIp = noConnIP;
                }
            }

            if (noConnIP != serverIp)
            {
                break;
            }
            yield();
        }

        mqttClient.setServer(serverIp, mqtt_port);

        /* connect now */
        char clientId[100] = {};
        char random[32] = {'-'};

        srand((unsigned int)(time(NULL)));
        int index = 0;

        char char1[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        for (index = 1; index < 31; index++)
        {
            random[index] = char1[rand() % (sizeof char1 - 1)];
        }
        Serial.println(random);
        sprintf(clientId, "%s", mqtt_client_id);
        strcat(clientId, random);

        Serial.printf("MQTT connecting as '%s'...\n", clientId);
        if (mqttClient.connect(clientId, "testUser", "1234")) //, "security/healthcheck", 20, true, "WillMessage", true))
        {
            Serial.println("connected");
        }
        else
        {
            mqttClient.disconnect();
            Serial.print("failed, status code =");
            Serial.print(mqttClient.state());
            Serial.printf("try again in %d seconds/n", 5);
        }
    }
}

void startMQTTClient(const char *host, int port, const char *topic, const char *client_id, const void *cert)
{
    mqtt_port = port;
    strcpy(mqtt_pub_topic, topic);
    strcpy(mqtt_host, host);
    strcpy(mqtt_client_id, client_id);

    Serial.printf("Starting MQTT Client '%s'\n", client_id);

    /* Set SSL/TLS certificate */
    // wifiClient.setCACert(ca_cert);

    /* Start main task. */
    xTaskCreatePinnedToCore(
        mqttCB,
        "mqtt",
        4096,
        NULL,
        2,
        &tMQTT,
        1);
}
