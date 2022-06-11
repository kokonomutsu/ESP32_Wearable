/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#include <IO_function.h>
#include <Kernel_IO_function.h>
#include <BLE_function.h>
#include <sensor_function.h>
#include <display_function.h>

#include <wifi_function.h>

#include "main.h"

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
void App_BLE_ProcessMsg(uint8_t MsgID, uint8_t MsgLength, uint8_t* pu8Data);
bool App_BLE_SendTemp(double temp);
bool App_BLE_SendSensor(int SPO2, int HeartRate);

void App_mqtt_callback(char* topic, byte* message, unsigned int length);
bool App_mqtt_SendTemp(double temp);
bool App_mqtt_SendSensor(double temp, int HeartRate, int SPO2);
bool App_mqtt_SendSPO2(int HeartRate, int SPO2);
bool App_mqtt_SendStatus(uint8_t ErrorStt, uint8_t TempStt, uint8_t Spo2Stt);

void App_Parameter_Read(StrConfigPara *StrCfg);
void App_Parameter_Save(StrConfigPara *StrCfg);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern IO_Struct pLED1, pLED1, pLED3;
extern IO_Struct pBUT_1, pBUT_2;
/*  IO Variable */
structIO_Button strIO_Button_Value, strOld_IO_Button_Value;
structIO_Manage_Output strLED_RD, strLED_GR, strLED_BL;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
//char* ssid = "lau 1 nha 1248 - mr";
//char* password = "88888888";
//char* mqtt_server = "206.189.158.67";
/**	BLE QUEUE	**/
uint8_t auBLERxBuffer[200];
uint8_t auBLETxBuffer[200];
/* Sensor Varialble */
int MaxSPO2 = 0;
int MaxHearbeat = 0;
/*  User Task Varialble */
eUSER_TASK_STATE eUserTask_State = E_STATE_STARTUP_TASK;
bool bFlag_1st_TaskState = true;
bool startFlag = false;
uint16_t SeqID = 0;
StrConfigPara StrCfg1;

/****************************************************************************/
/***        RTOS Task                                                     ***/
/****************************************************************************/
void task_BLE(void *parameter)
{
  for(;;) {
    /*	Check BLE Rx Buffer	*/
    if(BLE_RxDataProcess())
    {
      //Serial.printf("MsgID: 0x%02X, MsgSize: %d \n", BLE_getMsgID(), BLE_getMsgSize());
      App_BLE_ProcessMsg(BLE_getMsgID(), BLE_getMsgSize(), BLE_getMsgData());
    }
    /*	Send BLE Tx Buffer	*/
    if(BLE_isConnected()){
      BLE_sendMsg();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void task_Application(void *parameter)
{
  for(;;) {
    switch (eUserTask_State)
    {
      case E_STATE_STARTUP_TASK:
        if(bFlag_1st_TaskState)
        {
          //read work mode
          display_config(sensor_getTemp());
          bFlag_1st_TaskState = false;
        }
        else{
          //Check finger in?
          display_config(sensor_getTemp());
          App_BLE_SendTemp(sensor_getTemp());
        }
        break;
      case E_STATE_ONESHOT_TASK:
        if(bFlag_1st_TaskState)
        {
          display_config1(sensor_getTemp(), MaxHearbeat, MaxSPO2);
          bFlag_1st_TaskState = false;
        }
        else{
          if(startFlag){
            startFlag = false;
            MaxSPO2 = 0;
            MaxHearbeat = 0;

            eUserTask_State = E_STATE_PROCESSING_TASK;
            bFlag_1st_TaskState = true;
          }
          else{
            display_config1(sensor_getTemp(), MaxHearbeat, MaxSPO2);
          }
        }
        break;
      case E_STATE_PROCESSING_TASK:
        if(bFlag_1st_TaskState)
        {
          display_config2(sensor_getTemp());
          bFlag_1st_TaskState = false;
        }
        else{
          if(sensor_processing(MaxSPO2, MaxHearbeat))
          {
            App_BLE_SendSensor(MaxSPO2, MaxHearbeat);
            eUserTask_State = E_STATE_ONESHOT_TASK;
            bFlag_1st_TaskState = true;
          }
          else{
            display_config2(sensor_getTemp());
          }
        }
        break;
      case E_STATE_CONTINUOUS_TASK:
        if(bFlag_1st_TaskState)
        {
          display_config1(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
          bFlag_1st_TaskState = false;
        }
        else{
          display_config1(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
          App_mqtt_SendSensor(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
        }
        break;
    }
    vTaskDelay(StrCfg1.Parameter.interval*1000 / portTICK_PERIOD_MS);
  }
}

void task_IO(void *parameter)
{
  for(;;) {
    if(START_BUT_VAL == eButtonSingleClick)
    {
      START_BUT_VAL = eButtonHoldOff;
      LED_BLUE_TOG;
      if(eUserTask_State == E_STATE_ONESHOT_TASK){
        startFlag = true;
        display_config2(sensor_getTemp());
      }
    }
    else if(START_BUT_VAL == eButtonDoubleClick)
    {
      START_BUT_VAL = eButtonHoldOff;
      LED_RED_TOG;
      if(wifi_mqtt_isConnected())
        wifi_disconnect();
      else
        if(wifi_setup_mqtt(&App_mqtt_callback, StrCfg1.Parameter.WifiSSID, StrCfg1.Parameter.WifiPASS, StrCfg1.Parameter.ServerURL, 1883))
          App_Parameter_Save(&StrCfg1);
    }

    if(MODE_BUT_VAL == eButtonSingleClick)
    {
      MODE_BUT_VAL = eButtonHoldOff;
      LED_GREEN_TOG;
      bFlag_1st_TaskState = true;
      if((eUserTask_State == E_STATE_STARTUP_TASK) || (eUserTask_State == E_STATE_CONTINUOUS_TASK))
        eUserTask_State = E_STATE_ONESHOT_TASK;
      else
        eUserTask_State = E_STATE_CONTINUOUS_TASK;
    }
    else if(MODE_BUT_VAL == eButtonDoubleClick){
      MODE_BUT_VAL = eButtonHoldOff;
      LED_RED_TOG;
      bFlag_1st_TaskState = true;
      eUserTask_State = E_STATE_STARTUP_TASK;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void task_Kernel_IO(void *parameter)
{
  for(;;) {
    vIO_Output(&strLED_GR, &pLED1);
	  vIO_Output(&strLED_RD, &pLED2);
	  vIO_Output(&strLED_BL, &pLED3);

    /*** Get BT Value  ***/
    vGetIOButtonValue(eButton1, pBUT_1.read(), &strOld_IO_Button_Value, &strIO_Button_Value);
    vGetIOButtonValue(eButton2, pBUT_2.read(), &strOld_IO_Button_Value, &strIO_Button_Value);

    if(strOld_IO_Button_Value.bButtonState[eButton1] != strIO_Button_Value.bButtonState[eButton1])
    {
      strOld_IO_Button_Value.bButtonState[eButton1] = strIO_Button_Value.bButtonState[eButton1];
      strIO_Button_Value.bFlagNewButton = eTRUE;
    }
    if(strOld_IO_Button_Value.bButtonState[eButton2] != strIO_Button_Value.bButtonState[eButton2])
    {
      strOld_IO_Button_Value.bButtonState[eButton2] = strIO_Button_Value.bButtonState[eButton2];
      strIO_Button_Value.bFlagNewButton = eTRUE;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/****************************************************************************/
/***        Application Function                                          ***/
/****************************************************************************/
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(512);

  App_Parameter_Read(&StrCfg1);
  if((StrCfg1.Parameter.DeviceID[0] == 0xFF) && (StrCfg1.Parameter.DeviceID[1] == 0xFF))
  {
    BLE_Init(StrCfg1.Parameter.DeviceID, auBLETxBuffer, sizeof(auBLETxBuffer), auBLERxBuffer, sizeof(auBLERxBuffer));
    BLE_getMAC(StrCfg1.Parameter.DeviceID);
    App_Parameter_Save(&StrCfg1);
    ESP.restart();
  }
  if(StrCfg1.Parameter.interval > 9999)
  {
    StrCfg1.Parameter.interval = 1;
    App_Parameter_Save(&StrCfg1);
  }

  LED1_Init(&pLED1);
  LED2_Init(&pLED2);
  LED3_Init(&pLED3);

  strIO_Button_Value.bFlagNewButton =  eFALSE;
  BUTTON1_Init(&pBUT_1);
  strIO_Button_Value.bButtonState[eButton1] = eButtonRelease;
  strOld_IO_Button_Value.bButtonState[eButton1] = eButtonRelease;

  BUTTON2_Init(&pBUT_2);
  strIO_Button_Value.bButtonState[eButton2] = eButtonRelease;
  strOld_IO_Button_Value.bButtonState[eButton2] = eButtonRelease;
  
  sensor_Setup();
  display_Setup();
  BLE_Init(StrCfg1.Parameter.DeviceID, auBLETxBuffer, sizeof(auBLETxBuffer), auBLERxBuffer, sizeof(auBLERxBuffer));
  delay(1000);
  //wifi_Setup();
  xTaskCreate(task_BLE,"Task 1",8192,NULL,2,NULL);
  xTaskCreate(task_IO,"Task 2",8192,NULL,1,NULL);
  xTaskCreate(task_Kernel_IO,"Task 3",8192,NULL,1,NULL);
  xTaskCreate(task_Application,"Task 4",8192,NULL,1,NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  sensor_updateValue();
  wifi_loop(StrCfg1.Parameter.DeviceID);
}

/****************************************************************************/
/***        BLE Function                                                  ***/
/****************************************************************************/
void App_BLE_ProcessMsg(uint8_t MsgID, uint8_t MsgLength, uint8_t* pu8Data)
{
  switch (MsgID)
  {
    case E_SSID_CFG_ID:
      if((MsgLength - 8) <= SSID_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.WifiSSID[i] = pu8Data[i];
        for(int j=i;j<SSID_MAX_SIZE;j++)
          StrCfg1.Parameter.WifiSSID[j] = 0x00;
        
        Serial.println(StrCfg1.Parameter.WifiSSID);
      }
      break;
    case E_PASS_CFG_ID:
      if((MsgLength - 8) <= PASS_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.WifiPASS[i] = pu8Data[i];
        for(int j=i;j<PASS_MAX_SIZE;j++)
          StrCfg1.Parameter.WifiPASS[j] = 0x00;
        Serial.println(StrCfg1.Parameter.WifiPASS);
      }
      break;
    case E_INTERVAL_CFG_ID:
    {
      uint16_t temp = 0;
      if(MsgLength > 8){
        for(int i=0;i<(MsgLength - 8);i++){
          temp = temp*10 + *(pu8Data + i) - 48;
        }
        StrCfg1.Parameter.interval = temp;
        App_Parameter_Save(&StrCfg1);
      }
      break;
    }
    case E_WORK_MODE_ID:
      
      break;
    case E_URL_CFG_ID:
      if((MsgLength - 8) <= URL_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.ServerURL[i] = pu8Data[i];
        for(int j=i;j<URL_MAX_SIZE;j++)
          StrCfg1.Parameter.ServerURL[j] = 0x00;
        Serial.println(StrCfg1.Parameter.ServerURL);
      }
      break;
    case E_DEVICE_DATA_ID:
      BLE_configMsg(12, 0, E_DEVICE_DATA_ID, SeqID++, 2, StrCfg1.Parameter.DeviceID);
      break;
    case E_PRIVATE_KEY_ID:
      
      break;
    case E_ONESHOT_MODE_ID:
      LED_GREEN_TOG;
      if(eUserTask_State != E_STATE_ONESHOT_TASK)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK;
      }
      break;
    case E_CONTINUOUS_MODE_ID:
      LED_GREEN_TOG;
      if(eUserTask_State != E_STATE_CONTINUOUS_TASK)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_CONTINUOUS_TASK;
      }
      break;
    case E_PRODUCT_INFO_ID:
      
      break;
    case E_RESTART_DEVICE_ID:
      
      break;
  }
}

bool App_BLE_SendTemp(double temp)
{
  if(BLE_isConnected() && (temp < 1000))
  {
    int value = (int)(temp*10);
    uint8_t tempMsg[5] = {(uint8_t)(value/1000 + 48), (uint8_t)((value%1000)/100 + 48), (uint8_t)((value%100)/10 + 48), '.', (uint8_t)(value%10 + 48)};
    BLE_configMsg(13, 0, E_TEMP_DATA_ID, SeqID++, 1, tempMsg);
    return true;
  }
  return false;
}

bool App_BLE_SendSensor(int SPO2, int HeartRate)
{
  if(BLE_isConnected() && (SPO2 < 101) && (HeartRate < 1000))
  {
    uint8_t tempMsg[6] = {(uint8_t)(SPO2/100 + 48), (uint8_t)((SPO2%100)/10 + 48), (uint8_t)(SPO2%10 + 48), (uint8_t)(HeartRate/100 + 48), (uint8_t)((HeartRate%100)/10 + 48), (uint8_t)(HeartRate%10 + 48)};
    BLE_configMsg(14, 0, E_HRSPO2_DATA_ID, SeqID++, 2, tempMsg);
    return true;
  }
  return false;
}
/****************************************************************************/
/***        WiFi Function                                                 ***/
/****************************************************************************/
void App_mqtt_callback(char* topic, uint8_t* message, unsigned int length)
{
    String messageTemp;
    StaticJsonDocument<200> doc;

    for (int i = 0; i < length; i++) {
        //Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    DeserializationError error = deserializeJson(doc, messageTemp);
    Serial.println(messageTemp);
    
    if(strstr(topic,"productinfo") != NULL)
    {

    }
    else if(strstr(topic,"restart") != NULL)
    {

    }
    else if(strstr(topic,"measure") != NULL)
    {

    }
    else if(strstr(topic,"modeselect") != NULL)
    {
      LED_GREEN_TOG;
      if(doc["mode"] == '0'){
        if(eUserTask_State != E_STATE_ONESHOT_TASK)
        {
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_ONESHOT_TASK;
        }
      }
      else if(doc["mode"] == '1'){
        if(eUserTask_State != E_STATE_CONTINUOUS_TASK)
        {
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_CONTINUOUS_TASK;
        }
      }
    }
    else if(strstr(topic,"interval") != NULL)
    {
      StrCfg1.Parameter.interval = doc["sampleinterval"];
      App_Parameter_Save(&StrCfg1);
    }
    else if(strstr(topic,"wificonfig") != NULL)
    {
      int i=0;
      char *ssid = strstr((char *)message, "ssid");
      while(*(ssid + 7 + i) != '"'){
        StrCfg1.Parameter.WifiSSID[i] = *(ssid + 7 + i);
        i++;
      }
      for(int j=i;j<SSID_MAX_SIZE;j++)
        StrCfg1.Parameter.WifiSSID[j] = 0x00;

      i=0;
      char *pass = strstr((char *)message, "password");
      while(*(pass + 11 + i) != '"'){
        StrCfg1.Parameter.WifiPASS[i] = *(pass + 11 + i);
        i++;
      }
      for(int j=i;j<PASS_MAX_SIZE;j++)
        StrCfg1.Parameter.WifiPASS[j] = 0x00;

      wifi_disconnect();
      if(wifi_connect(StrCfg1.Parameter.WifiSSID, StrCfg1.Parameter.WifiPASS))
        App_Parameter_Save(&StrCfg1);
    }
}

bool App_mqtt_SendSensor(double temp, int HeartRate, int SPO2)
{
  if(wifi_mqtt_isConnected())
  {
    char dataSend[76];
    int value = (int)(temp*10);
    sprintf(dataSend,
            "{\"Temp\":\"%d%d.%d\",\"Heartrate\":\"%d\",\"Spo2\":\"%d\",\"Time\":\"%d-%d-%dT%s\"}",
            value/100,
            (value%100)/10,
            value%10,
            HeartRate,
            SPO2,
            wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "sensor", dataSend);
    return true;
  }
  return false;
}

bool App_mqtt_SendTemp(double temp)
{
  if(wifi_mqtt_isConnected())
  {
    char dataSend[45];
    int value = (int)(temp*10);
    sprintf(dataSend,
            "{\"Temp\":\"%d%d.%d\",\"Time\":\"%d-%d-%dT%s\"}",
            value/100,
            (value%100)/10,
            value%10,
            wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "temp", dataSend);
    return true;
  }
  return false;
}

bool App_mqtt_SendSPO2(int HeartRate, int SPO2)
{
  if(wifi_mqtt_isConnected())
  {
    char dataSend[60];
    sprintf(dataSend,
            "{\"Heartrate\":\"%d\",\"Spo2\":\"%d\",\"Time\":\"%d-%d-%dT%s\"}",
            HeartRate,
            SPO2,
            wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "spo2", dataSend);
    return true;
  }
  return false;
}

bool App_mqtt_SendStatus(uint8_t ErrorStt, uint8_t TempStt, uint8_t Spo2Stt)
{
  if(wifi_mqtt_isConnected())
  {
    char dataSend[82];
    sprintf(dataSend,
            "{\"Error\":\"%d\",\"TempSenStatus\":\"%d\",\"Spo2SenStatus\":\"%d\",\"Time\":\"%d-%d-%dT%s\"}",
            ErrorStt,
            TempStt,
            Spo2Stt,
            wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "status", dataSend);
    return true;
  }
  return false;
}

/****************************************************************************/
/***        EEPROM Parameter Function                                     ***/
/****************************************************************************/
void App_Parameter_Read(StrConfigPara *StrCfg)
{
  for(int i=0;i<sizeof(StrCfg->paraBuffer);i++){
    StrCfg->paraBuffer[i] = EEPROM.read(i);
    delay(5);
  }
}

void App_Parameter_Save(StrConfigPara *StrCfg)
{
  for(int i=0;i<sizeof(StrCfg->paraBuffer);i++){
    EEPROM.write(i, StrCfg->paraBuffer[i]);
    delay(5);
  }
  EEPROM.commit();
}
