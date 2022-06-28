#ifndef _WIFI_FUNCTION_H_
#define _WIFI_FUNCTION_H_

bool wifi_connect(char* ssid, char* password);

bool wifi_setup_mqtt(void (*callback)(char* topic, uint8_t* message, unsigned int length), char* ssid, char* password, char* mqtt_server, uint16_t port);

bool wifi_loop(char* DeviceID);

void wifi_disconnect(void);

bool wifi_mqtt_isConnected(void);

void wifi_mqtt_publish(uint8_t *DeviceID, String topic, char* dataBuffer);

String wifi_ntp_getTime(void);

int wifi_ntp_getYears(void);

int wifi_ntp_getMonths(void);

int wifi_ntp_getDays(void);

#endif