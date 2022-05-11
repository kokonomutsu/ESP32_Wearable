#ifndef _BLE_FUNCTION_H_
#define _BLE_FUNCTION_H_

void BLE_Setup(void);

void BLE_sendData(unsigned char* pValue, int length);

bool BLE_isConnected(void);

#endif