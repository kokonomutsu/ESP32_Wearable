#ifndef _BLE_FUNCTION_H_
#define _BLE_FUNCTION_H_

#include <stdint.h>

void BLE_Init(uint8_t *pu8TxBuffer, uint32_t u32TxBufferLen, uint8_t *pu8RxBuffer, uint32_t u32RxBufferLen);

void BLE_sendData(unsigned char* pValue, int length);

bool BLE_isConnected(void);

#endif