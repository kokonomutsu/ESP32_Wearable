#include "wifi_function.h"
#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "For-Dev";
const char* password = "developeronly";
const char* mqtt_server = "206.189.158.67";

WiFiClient espClient;
PubSubClient client(espClient);

static void callback(char* topic, byte* message, unsigned int length) {
  
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  
  if (String(topic) == "NXP/CtrRL") 
  {
    
  }
  else if(String(topic) == "NXP/RSSI")
  {
    
  }
}

static void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      //Serial.println("connected");
      // Subscribe
      client.subscribe("NXP/CtrRL");
      client.subscribe("NXP/RSSI");
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void wifi_Setup(void)
{
    //Serial.println();
    //Serial.print("Connecting to ");
    //Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    //erial.println("");
    //Serial.println("WiFi connected");
    //Serial.println("IP address: ");
    //Serial.println(WiFi.localIP());

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void wifi_loopTask(void)
{
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}