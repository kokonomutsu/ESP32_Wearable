/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include "wifi_function.h"

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
static void reconnect(char* fullDeviceID);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org", 25200, 60000);
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

bool wifi_connect(char* ssid, char* password)
{
    int ConnectCnt = 0;
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    /*while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        ConnectCnt++;
        if(ConnectCnt > 30)
            return false;
    }*/
    return true;
}

bool wifi_setup_mqtt(void (*callback)(char* topic, uint8_t* message, unsigned int length), char* mqtt_server, uint16_t port)
{
    if(WiFi.status() == WL_CONNECTED)
    {
        client.setServer(mqtt_server, port);
        client.setCallback(callback);

        timeClient.begin();
        return true;
    }
    return false;
}

void wifi_disconnect(void)
{
    WiFi.disconnect(true);
}

bool wifi_loop(char* fullDeviceID)
{
    if(WiFi.status() == WL_CONNECTED)
    {
        if (!client.connected()) {
            reconnect(fullDeviceID);
        }
        client.loop();
        timeClient.update();
        timeClient.getFormattedDate();
        return true;
    }
    return false;
}

bool wifi_connect_status(void)
{
    if(WiFi.status() == WL_CONNECTED)
    {
        return true;
    }
    return false;
}

bool server_connect_status(void)
{
    if(client.connected())
    {
        return true;
    }
    return false;
}

bool wifi_mqtt_isConnected(void)
{
    if(WiFi.status() == WL_CONNECTED)
    {
        if (client.connected())
            return true;
    }
    return false;
}

void wifi_mqtt_publish(uint8_t *DeviceID, String topic, char* dataBuffer)
{
    char pubTopic[28];
    sprintf(pubTopic, "tele/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], topic);
    client.publish(pubTopic, dataBuffer);
}

String wifi_ntp_getTime(void)
{
    return (timeClient.getFormattedTime());
    //return "00:00:00";
}

int wifi_ntp_getYears(void)
{
    return timeClient.getYears();
    //return 0;
}
int wifi_ntp_getMonths(void)
{
    return timeClient.getMonths();
    //return 0;
}
int wifi_ntp_getDays(void)
{
    return timeClient.getDate();
    //return 0;
}
/****************************************************************************/
/***              Local Function			                               **/
/****************************************************************************/
static void reconnect(char* fullDeviceID) 
{
  // Loop until we're reconnected
    while (!client.connected()) 
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(fullDeviceID)) 
        {
            Serial.println("connected");
            // Subscribe
            char topic[43];
            //sprintf(topic, "cmnd/FPT_FCCIoT_%C%C%C%C/%s", DeviceID[0], DeviceID[1], DeviceID[2], DeviceID[3], "iot");
            sprintf(topic, "cmnd/%s/%s", fullDeviceID, "iot");
            Serial.println(topic);
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

