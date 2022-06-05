/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "wifi_function.h"

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void reconnect(uint8_t *DeviceID);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
WiFiClient espClient;
PubSubClient client(espClient);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

bool wifi_setup_mqtt(void (*callback)(char* topic, byte* message, unsigned int length), char* ssid, char* password, char* mqtt_server, uint16_t port)
{
    int ConnectCnt = 0;
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        ConnectCnt++;
        if(ConnectCnt > 30)
            return false;
    }
    client.setServer(mqtt_server, port);
    client.setCallback(callback);
    return true;
}

void wifi_disconnect(void)
{
    WiFi.disconnect(true);
}

bool wifi_loop(uint8_t *DeviceID)
{
    if(WiFi.status() == WL_CONNECTED)
    {
        if (!client.connected()) {
            reconnect(DeviceID);
        }
        client.loop();
        return true;
    }
    return false;
}
/****************************************************************************/
/***              Local Function			                               **/
/****************************************************************************/
static void reconnect(uint8_t *DeviceID) 
{
  // Loop until we're reconnected
    while (!client.connected()) 
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP32Client")) 
        {
            Serial.println("connected");
            // Subscribe
            char topic[43];
            sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "productinfo");
            client.subscribe(topic);
            sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "restart");
            client.subscribe(topic);
            sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "measure");
            client.subscribe(topic);
            sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "modeselect");
            client.subscribe(topic);
            sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "interval");
            client.subscribe(topic);
            sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "wificonfig");
            client.subscribe(topic);
        }
        else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

