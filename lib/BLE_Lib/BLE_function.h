#ifndef _BLE_FUNCTION_H_
#define _BLE_FUNCTION_H_

#include <stdint.h>

void BLE_Init(void);

void BLE_sendData(unsigned char* pValue, int length);

bool BLE_isConnected(void);

#endif