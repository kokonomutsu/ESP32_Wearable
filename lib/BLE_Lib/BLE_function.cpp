#include "BLE_function.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLE2902.h>
#include <Queue.h>

//#define bleServerName "FPT_IoTS_BTempHRate01-1E9E"
#define bleServerName "FPT_IoTS_BTempHRate01-D20A"//devkit
#define SERVICE_UUID "7bde7b9d-547e-4703-9785-ceedeeb2863e"
#define CHARACTERISTIC_UUID "9d45a73a-b19f-4739-8339-ecad527b4455"

#define MAX_PACKET_SIZE   100
#define SL_START_CHAR     0x24
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/* Enumerated list of states for receive state machine */
typedef enum {
    E_STATE_WAIT_START,
    E_STATE_WAIT_SIZE,
    E_STATE_WAIT_ID,
    E_STATE_WAIT_SEQ_MSB,
    E_STATE_WAIT_SEQ_LSB,
    E_STATE_WAIT_STAM_MSB,
    E_STATE_WAIT_STAM_LSB,
    E_STATE_WAIT_DATA,
    E_STATE_WAIT_CRC,
    E_STATE_WAIT_CRC1
} APP_teDataState;
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

tsQueue asBLE_TxQueue;
tsQueue asBLE_RxQueue;

static uint8_t MsgTxSize = 0;
static uint8_t MsgTxID;

static uint8_t  MsgRxBuffer[MAX_PACKET_SIZE];
static uint8_t  MsgRxSize = 0;
static uint8_t  MsgRxID;
static uint16_t MsgCRC;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      Serial.print("New value: ");
      for (int i = 0; i < value.length(); i++)
      {
        Serial.print(value[i]);
      }
      Serial.println();
    }
  }
};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void BLE_Init(void)
{
  BLEDevice::init(bleServerName);
  BLEDevice::setMTU(105);
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
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  /* Initialise Tx & Rx Queue's */
	//vQueue_Init(&asBLE_TxQueue, pu8TxBuffer, u32TxBufferLen);
	//vQueue_Init(&asBLE_RxQueue, pu8RxBuffer, u32RxBufferLen);
}
/*
void BLE_sendMsg(void)
{
  static APP_teDataState eTxState = E_STATE_WAIT_START;
	static uint8_t u8Bytes;

	uint8_t u8TxByte;
  if(bQueue_Read(&asBLE_TxQueue, &u8TxByte))
  {
    switch(eTxState)
    {
      case E_STATE_WAIT_START:
        if(u8TxByte == SL_START_CHAR)
          u8Bytes = 0;
          eTxState = E_STATE_WAIT_SIZE;
        break;
      case E_STATE_WAIT_SIZE:
        //Cấp phát động mãng:
        MsgTxSize = u8TxByte;
        unsigned char *value = new unsigned char[(int)MsgTxSize + 2];
        value[u8Bytes] = SL_START_CHAR;
        u8Bytes++;
        eTxState = E_STATE_WAIT_DATA;
        break;
      case E_STATE_WAIT_DATA:
        if(u8Bytes < MsgTxSize){
          value[u8Bytes++] = u8TxByte;
        }
        else{
          eTxState = E_STATE_WAIT_CRC;
        }
        break;
      case E_STATE_WAIT_CRC:
        MsgCRC = u8TxByte*256;
        eTxState = E_STATE_WAIT_CRC1;
        break;
      case E_STATE_WAIT_CRC1:
        MsgCRC += u8TxByte;
        //Check CRC
        if(MsgCRC)
        {
          value[(int)MsgTxSize + 1] = MsgCRC/256;
          value[(int)MsgTxSize + 2] = MsgCRC%256;
          //Send data
          pCharacteristic->setValue(value, (int)MsgTxSize + 3);
          pCharacteristic->notify();
        }
        
        delete[] value;
        eTxState = E_STATE_WAIT_START;
        break;
    }
  }
}
*/
void BLE_sendData(unsigned char* pValue, int length)
{
    pCharacteristic->setValue(pValue, length);
    pCharacteristic->notify();
}

bool BLE_isConnected(void)
{
  return deviceConnected;
}

static uint8_t APP_u8CalculateCRC(uint8_t *pu8Data)
{
  int n;
  uint8_t u8CRC;

  u8CRC = SL_START_CHAR & 0xff;
  for (n = 1; n < 19; n++) {
      u8CRC ^= pu8Data[n];
  }
  return (u8CRC);
}