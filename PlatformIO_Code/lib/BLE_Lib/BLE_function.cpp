#include "BLE_function.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLE2902.h>

#define bleServerName "FPT_HC_IoTS_BTempHRate01-1E9E"
#define SERVICE_UUID "7bde7b9d-547e-4703-9785-ceedeeb2863e"
#define CHARACTERISTIC_UUID "9d45a73a-b19f-4739-8339-ecad527b4455"

/* BLE Variable */
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
//bool oldDeviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void BLE_Setup(void)
{
  BLEDevice::init(bleServerName);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

void BLE_sendData(unsigned char* pValue, int length)
{
    pCharacteristic->setValue(pValue, length);
    pCharacteristic->notify();
}

bool BLE_isConnected(void)
{
    return deviceConnected;
}