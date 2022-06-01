#ifndef _MAIN_H_
#define _MAIN_H_

#define START_BUT_VAL   strIO_Button_Value.bButtonState[eButton2]
#define MODE_BUT_VAL    strIO_Button_Value.bButtonState[eButton1]

#define LED_GREEN_TOG   vIO_ConfigOutput(&strLED_GR, (enumbool)(1 - (int)(pLED1.writeSta())), 2, 50, eFALSE);
#define LED_RED_TOG     vIO_ConfigOutput(&strLED_RD, (enumbool)(1 - (int)(pLED2.writeSta())), 2, 50, eFALSE);
#define LED_BLUE_TOG    vIO_ConfigOutput(&strLED_BL, (enumbool)(1 - (int)(pLED3.writeSta())), 2, 50, eFALSE);

#define PRODUCT_ID_SIZE (int)4
#define SSID_MAX_SIZE   (int)20
#define PASS_MAX_SIZE   (int)20

typedef enum {
    E_STATE_STARTUP_TASK,
    E_STATE_PROCESSING_TASK,
    E_STATE_ONESHOT_TASK,
    E_STATE_CONTINUOUS_TASK,
} eUSER_TASK_STATE;

typedef union Struct_Flash_Config_Parameter{
    struct
    {
        uint8_t workMode;
        uint16_t interval;
        uint8_t Product_ID[PRODUCT_ID_SIZE];
        uint8_t WifiSSID[SSID_MAX_SIZE];
        uint8_t WifiPASS[PASS_MAX_SIZE]; 
        /* data */
    }Parameter;
    unsigned char paraBuffer[sizeof(Parameter)];
}StrConfigPara;

#endif