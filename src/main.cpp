/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <Arduino.h>
#include <EEPROM.h>
#include <IO_function.h>
#include <Kernel_IO_function.h>
#include <BLE_function.h>
#include <sensor_function.h>
#include <display_function.h>
#include <wifi_function.h>
#include "main.h"
#include <ArduinoJson.h>
#include "CloudFPTIoTCoreMqtt.h"
#include "ciotc_config.h"

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
void App_BLE_ProcessMsg(uint8_t MsgID, uint8_t MsgLength, uint8_t* pu8Data);
bool App_BLE_SendTemp(double temp);
bool App_BLE_SendSensor(int SPO2, int HeartRate);
bool App_BLE_SendTestConnection(bool bConnectionStatus);
bool App_BLE_SendACK(Msg_teID_Type bBLEID);

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
//char* ssid = "KOKONO";
//char* password = "kokono26988";
//char* mqtt_server = "103.170.123.115";
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
char fullDeviceID[20];
char strTime[30];
char fullTopic[50];
char msg[2000];
uint8_t bUserId = 6;
/* Working mode */
uint8_t bDeviceMode = MODE_BLE;

/* Json */
DynamicJsonDocument MQTT_JsonDoc(1024);
// Initialize WiFi and MQTT for this board
//Client *netClient;
CloudIoTCoreDevice *device;
unsigned long iat = 0;
String jwt;
// Forward global callback declarations
String getJwt();

/* Struct mqtt package */
#define typeOwnerbe   "be"
#define typeOwneriot  "iot"
#define typeOwnermb   "mb"

#define iotReqCon                   1
#define serverSendDeviceConfig      2
#define iotSendSensors              3
#define iotSendTemp                 4
#define iotSendSpo2                 5
#define iotSendStatus               6
#define iotSendProductInfo          11

#define serverSendMeasureTemp       7
#define serverSendMeasureSpo2       8
#define serverSendMeasureStop       9
#define serverAskProductInfo        10
#define serverSendMeasureRestart    12


typedef struct{
  String deviceId = "Device Id";
  String jwt = "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2NTQwNzc2NjksImV4cCI6MTY1NDA3ODg2OSwiYXVkIjoiZnB0LWRlbW8tdGVzdGluZy1qd3QifQ.m9zLfMLgNwutRKb6X8nG2Ih2uB-TJZ-DNRjfuNvFNhRdPxlznnfrkvKj_28Go07JhFmtSvtrhKRsVM1fhkAE4w";
}structMQTTData;

/* Struct mqtt package */
typedef struct{
	  String owner = typeOwneriot; //value : iot
    String topic = "tele/FPT_FCCIoT_xxxx/topic";
    char type = iotSendTemp; //value : 3
    uint index = 0; //default
    uint total = 1; //default
    uint messageId = 1;//default
    structMQTTData strMQTTdata;
}structMQTTSendPackage;

/* Global variable */
structMQTTSendPackage strMQTTSendPackage;

/* Get JWT function */
String getJwt(){
  iat = time(nullptr);
  Serial.println("Refreshing JWT");
  jwt = device->createJWT(iat, jwt_exp_secs);
  return jwt;
}

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
          Serial.println("[DEBUG]: STARTUP TASK!");
          display_state(eUserTask_State);
          vTaskDelay(1000);
          display_config(sensor_getTemp());
          bFlag_1st_TaskState = false;
        }
        else{
          //Check finger in?
          display_config(sensor_getTemp());
          /* STARTUP-IDLE just measure, not send BLE data */
          //App_BLE_SendTemp(sensor_getTemp());
        }
        break;
      case E_STATE_ONESHOT_TASK:
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: ONESHOT TASK!");
          display_state(eUserTask_State);
          vTaskDelay(1000);
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
      case E_STATE_ONESHOT_TASK_TEMP:
        static double bSingleTempShot = 0;
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: ONESHOT TASK TEMPERATURE!");
          display_state(eUserTask_State);
          vTaskDelay(1000);
          display_single_temp_shot(bSingleTempShot);
          bSingleTempShot = 0;
          bFlag_1st_TaskState = false;
        }
        else{
          /* Delay 1s to display 0 */
          vTaskDelay(500);
          bSingleTempShot = sensor_getTemp();
          App_BLE_SendTemp(bSingleTempShot);
          App_mqtt_SendTemp(bSingleTempShot);
          /* Delay 1s to display */
          vTaskDelay(1000);
          display_single_temp_shot(bSingleTempShot);
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_STARTUP_TASK;
        }
        break;
        case E_STATE_ONESHOT_TASK_SPO2:
        static int Bpm = 0, SPO2 = 0;
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: ONESHOT TASK SP02!");
          display_state(eUserTask_State);
          vTaskDelay(1000);
          display_single_spo2_shot(0,0);
          bFlag_1st_TaskState = false;
          startFlag = true;
          /* Wakeup device */
          sersor_SPO2_wakeup();
          sersor_reset_data_Value();
          MaxSPO2 = 0;
          MaxHearbeat = 0;
          
        }
        else{
          if(sensor_processing(MaxSPO2, MaxHearbeat))
          {
            Bpm = MaxHearbeat;
            SPO2 = MaxSPO2;
            App_BLE_SendSensor(SPO2,Bpm);
            display_single_spo2_shot(Bpm,SPO2);
            App_mqtt_SendSPO2(Bpm,SPO2);
            /* Delay 5s to display */
            vTaskDelay(5000);
            eUserTask_State = E_STATE_STARTUP_TASK;
            bFlag_1st_TaskState = true;
            /* Shut down device */
            sersor_SPO2_shutdown();
          }
          else
          {
            Bpm = sensor_getHeardBeat();
            SPO2 = sersor_getSPO2();
            display_config3(Bpm,SPO2);
          }
        }
        break;
      case E_STATE_PROCESSING_TASK:
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: PROCESING TASK!");
          display_state(eUserTask_State);
          vTaskDelay(1000);
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
          Serial.println("[DEBUG]: CONTINOUS TASK!");
          display_state(eUserTask_State);
          vTaskDelay(1000);
          display_config1(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
          bFlag_1st_TaskState = false;
        }
        else{
          display_config1(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
          App_mqtt_SendSensor(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
        }
        break;
      case E_STATE_TEST_CONNECTION_TASK:
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: TEST CONNECTION TASK!");
          display_state(eUserTask_State);
          bFlag_1st_TaskState = false;
          /* Connect server */
          wifi_setup_mqtt(&App_mqtt_callback, StrCfg1.Parameter.WifiSSID, StrCfg1.Parameter.WifiPASS, StrCfg1.Parameter.ServerURL, 1883);
        }
        else{
          static uint32_t bTestConnectionTimeOut = 0;
          if(wifi_mqtt_isConnected())
          {
            display_server_connect_state(true);
            eUserTask_State = E_STATE_STARTUP_TASK;
            bFlag_1st_TaskState = true;
            LED_BLUE_TOG;
            bDeviceMode = MODE_WIFI;
            StrCfg1.Parameter.bLastMode = bDeviceMode;
            App_Parameter_Save(&StrCfg1);
            /* Feedback to BLE */
            App_BLE_SendTestConnection(true);
            vTaskDelay(1000);
            //ESP.restart();
          }
          else
          {
            if(bTestConnectionTimeOut++<=10)//10s
            {
              vTaskDelay(1000);
            }
            else
            {
              display_server_connect_state(false);
              /* Feedback to BLE */
              eUserTask_State = E_STATE_STARTUP_TASK;
              bFlag_1st_TaskState = true;
              LED_RED_TOG;
              bDeviceMode = MODE_WIFI;
              StrCfg1.Parameter.bLastMode = bDeviceMode;
              App_Parameter_Save(&StrCfg1);
              /* Feedback to BLE */
              App_BLE_SendTestConnection(false);
            }
          }
        }
      break;
    }
    vTaskDelay((StrCfg1.Parameter.interval*1000) / portTICK_PERIOD_MS);
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

    if(MODE_BUT_VAL == eButtonSingleClick)
    {
      MODE_BUT_VAL = eButtonHoldOff;
      /* Button single press */
      Serial.println("[DEBUG]: Button single press!");
      if(bDeviceMode == MODE_WIFI)
      {
        LED_GREEN_TOG;
        bDeviceMode = MODE_BLE;
        StrCfg1.Parameter.bLastMode = bDeviceMode;
        App_Parameter_Save(&StrCfg1);
        /* Delay before restart */
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
      }
      else if(bDeviceMode == MODE_BLE)
      {
        LED_BLUE_TOG;
        bDeviceMode = MODE_WIFI;
        StrCfg1.Parameter.bLastMode = bDeviceMode;
        App_Parameter_Save(&StrCfg1);
        /* Delay before restart */
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
      }
    }
    else if(MODE_BUT_VAL == eButtonLongPressT1){
      MODE_BUT_VAL = eButtonHoldOff;
      bDeviceMode = MODE_DUAL;
      StrCfg1.Parameter.bLastMode = bDeviceMode;
      memset(StrCfg1.Parameter.WifiPASS,0,sizeof(StrCfg1.Parameter.WifiPASS));
      memset(StrCfg1.Parameter.WifiSSID,0,sizeof(StrCfg1.Parameter.WifiSSID));
      memset(StrCfg1.Parameter.ServerURL,0,sizeof(StrCfg1.Parameter.ServerURL));
      App_Parameter_Save(&StrCfg1);
      /* Delay before restart */
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      ESP.restart();
    }
  }
  vTaskDelay(10 / portTICK_PERIOD_MS);
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
  if(((StrCfg1.Parameter.DeviceID[0] == 0xFF) && (StrCfg1.Parameter.DeviceID[1] == 0xFF))
      ||((StrCfg1.Parameter.bLastMode!=MODE_BLE)&&(StrCfg1.Parameter.bLastMode!=MODE_WIFI)&&(StrCfg1.Parameter.bLastMode!=MODE_DUAL)))
  {
    BLE_Init(StrCfg1.Parameter.DeviceID, auBLETxBuffer, sizeof(auBLETxBuffer), auBLERxBuffer, sizeof(auBLERxBuffer));
    BLE_getMAC(StrCfg1.Parameter.DeviceID);
    /* Default is mode BLE */
    StrCfg1.Parameter.bLastMode = MODE_BLE;
    App_Parameter_Save(&StrCfg1);
    ESP.restart();
  }
  /* Load last mode */
  bDeviceMode = StrCfg1.Parameter.bLastMode;
  
  /* Test default wifi */
  //memcpy(&StrCfg1.Parameter.WifiSSID,"KOKONO",sizeof("KOKONO"));
  //memcpy(&StrCfg1.Parameter.WifiPASS, "kokono26988", sizeof("kokono26988"));
  //memcpy(&StrCfg1.Parameter.WifiSSID,"lau 1 nha 1248 - mr",sizeof("lau 1 nha 1248 - mr"));
  //memcpy(&StrCfg1.Parameter.WifiPASS, "88888888", sizeof("88888888"));
  //memcpy(&StrCfg1.Parameter.ServerURL, "206.189.158.67", sizeof("206.189.158.67"));
  //memcpy(&StrCfg1.Parameter.ServerURL, "103.170.123.115", sizeof("103.170.123.115"));//server PicopPiece
  //memcpy(&StrCfg1.Parameter.ServerURL, "34.146.132.228", sizeof("34.146.132.228"));//server FPT
  /* Make Full device */
  sprintf(fullDeviceID, "FPT_FCCIoT_%C%C%C%C", StrCfg1.Parameter.DeviceID[0], 
                                                StrCfg1.Parameter.DeviceID[1],
                                                StrCfg1.Parameter.DeviceID[2],
                                                StrCfg1.Parameter.DeviceID[3]);

  /* Serial json */
  /*MQTT_JsonDoc["owner"]   = typeOwneriot;
  MQTT_JsonDoc["topic"]   = strMQTTSendPackage.topic;
  MQTT_JsonDoc["type"]    = strMQTTSendPackage.type;
  MQTT_JsonDoc["index"]   = strMQTTSendPackage.index;
  MQTT_JsonDoc["total"]   = strMQTTSendPackage.total;
  MQTT_JsonDoc["messageId"]   = strMQTTSendPackage.messageId;
  MQTT_JsonDoc["data"]["deviceId"] = strMQTTSendPackage.strMQTTdata.deviceId;
  MQTT_JsonDoc["data"]["jwt"] = strMQTTSendPackage.strMQTTdata.jwt;
  serializeJson(MQTT_JsonDoc, Serial);*/

  if(StrCfg1.Parameter.interval > 9999)
  {
    StrCfg1.Parameter.interval = 1;
    App_Parameter_Save(&StrCfg1);
  }
  Serial.println("[DEBUG]: SYSTEM INIT!");
  Serial.println("[DEBUG]: LED INIT!");
  LED1_Init(&pLED1);
  LED2_Init(&pLED2);
  LED3_Init(&pLED3);

  Serial.println("[DEBUG]: BUTTON INIT!");
  strIO_Button_Value.bFlagNewButton =  eFALSE;
  BUTTON1_Init(&pBUT_1);
  strIO_Button_Value.bButtonState[eButton1] = eButtonRelease;
  strOld_IO_Button_Value.bButtonState[eButton1] = eButtonRelease;

  BUTTON2_Init(&pBUT_2);
  strIO_Button_Value.bButtonState[eButton2] = eButtonRelease;
  strOld_IO_Button_Value.bButtonState[eButton2] = eButtonRelease;

  /* Add device cloud */
  device = new CloudIoTCoreDevice(
      project_id, location, registry_id, device_id,
      private_key_str);

  /* Device setup */
  sensor_Setup();
  display_Setup(bDeviceMode);
  Serial.println("[DEBUG]: BLE INIT!");
  /* Check device mode */
  if((bDeviceMode == MODE_BLE)||(bDeviceMode == MODE_DUAL))
  {
    BLE_Init(StrCfg1.Parameter.DeviceID, auBLETxBuffer, sizeof(auBLETxBuffer), auBLERxBuffer, sizeof(auBLERxBuffer));
  }
  if(bDeviceMode == MODE_WIFI)
  {
    delay(1000);
    wifi_setup_mqtt(&App_mqtt_callback, StrCfg1.Parameter.WifiSSID, StrCfg1.Parameter.WifiPASS, StrCfg1.Parameter.ServerURL, 1883);
  }
  
  /* Create task */
  if((bDeviceMode == MODE_BLE)||(bDeviceMode == MODE_DUAL))
  {
    xTaskCreate(task_BLE,"Task 1",8192,NULL,2,NULL);
  }
  xTaskCreate(task_IO,"Task 2",8192,NULL,1,NULL);
  xTaskCreate(task_Kernel_IO,"Task 3",8192,NULL,1,NULL);
  xTaskCreate(task_Application,"Task 4",8192,NULL,1,NULL);
}

void loop() {
  static bool bFlagGetJWT = true;
  // put your main code here, to run repeatedly:
  sensor_updateValue();
  if((bDeviceMode == MODE_WIFI)||(eUserTask_State == E_STATE_TEST_CONNECTION_TASK))
  {
    wifi_loop(fullDeviceID);
    if((wifi_mqtt_isConnected()==true)&&(bFlagGetJWT==true))
    {
      bFlagGetJWT = false;
      /* Try get JWT */
      Serial.println(getJwt().c_str());
    }
  }
  /*if(wifi_mqtt_isConnected()==false)
  {
    bFlagGetJWT = false;
  }*/
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
        App_Parameter_Save(&StrCfg1);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    case E_TEST_CONNECTION_ID:
      LED_GREEN_TOG;
      LED_RED_TOG;
      /* Start connect wifi and server */
      if(eUserTask_State != E_STATE_TEST_CONNECTION_TASK)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_TEST_CONNECTION_TASK;
      }
      break;
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
        App_Parameter_Save(&StrCfg1);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
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
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    }
    case E_WORK_MODE_ID:
      
      break;
    case E_ONESHOT_TEMP_ID:
      LED_GREEN_TOG;
      if(eUserTask_State != E_STATE_ONESHOT_TASK_TEMP)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK_TEMP;
      }
      break;
    case E_ONESHOT_SPO2_ID:
      LED_BLUE_TOG;
      if(eUserTask_State != E_STATE_ONESHOT_TASK_SPO2)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK_SPO2;
      }
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
        App_Parameter_Save(&StrCfg1);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    case E_DEVICE_DATA_ID:
      BLE_configMsg(12, 0, E_DEVICE_DATA_ID, SeqID++, 2, StrCfg1.Parameter.DeviceID);
      break;
    case E_PRIVATE_KEY_ID:
      if((MsgLength - 8) <= URL_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.PrivateKey[i] = pu8Data[i];
        for(int j=i;j<URL_MAX_SIZE;j++)
          StrCfg1.Parameter.PrivateKey[j] = 0x00;
        Serial.println(StrCfg1.Parameter.PrivateKey);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    case E_STOP_MEASURE_ID:
      
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

bool App_BLE_SendACK(Msg_teID_Type bBLEID)
{
  if(BLE_isConnected())
  {
      BLE_configMsg(9, 0, bBLEID, SeqID++, 1, (uint8_t*)"1");
      return true;
  }
  return false;
}

bool App_BLE_SendTestConnection(bool bConnectionStatus)
{
  if(BLE_isConnected())
  {
    if(bConnectionStatus==true)
    {
      BLE_configMsg(9, 0, E_TEST_CONNECTION_ID, SeqID++, 1, (uint8_t*)"1");
    }
    else
    {
      BLE_configMsg(9, 0, E_TEST_CONNECTION_ID, SeqID++, 1, (uint8_t*)"0");
    }
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
    StaticJsonDocument<1024> MQTT_JsonReceiveDoc;

    for (int i = 0; i < length; i++) {
        //Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    DeserializationError error = deserializeJson(MQTT_JsonReceiveDoc, messageTemp);
    Serial.println(messageTemp);
    
    /* Only search topic IoT, parse follow type */
    if(strstr(topic,"iot") != NULL)
    {
      LED_GREEN_TOG;
      LED_BLUE_TOG;
      if(MQTT_JsonReceiveDoc["type"] == 7){
        if(eUserTask_State != E_STATE_ONESHOT_TASK_TEMP)
        {
          bUserId = MQTT_JsonReceiveDoc["data"]["userId"];
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_ONESHOT_TASK_TEMP;
        }
      }
      else if(MQTT_JsonReceiveDoc["type"] == 8){
        if(eUserTask_State != E_STATE_ONESHOT_TASK_SPO2)
        {
          bUserId = MQTT_JsonReceiveDoc["data"]["userId"];
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_ONESHOT_TASK_SPO2;
        }
      }
    }
}

bool App_mqtt_SendSensor(double temp, int HeartRate, int SPO2)
{
  if(wifi_mqtt_isConnected())
  {
    /* Json send message */
    sprintf(strTime,
            "%d-%d-%dT%s",wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    Serial.println(strTime);
    sprintf(fullTopic, "tele/%s/iot", fullDeviceID);
    Serial.println(fullTopic);
    /* Add to json */
    MQTT_JsonDoc.clear();
    MQTT_JsonDoc["owner"]   = typeOwneriot;
    MQTT_JsonDoc["topic"]   = fullTopic;
    MQTT_JsonDoc["type"]    = iotSendSensors;
    MQTT_JsonDoc["index"]   = strMQTTSendPackage.index;
    MQTT_JsonDoc["total"]   = strMQTTSendPackage.total;
    MQTT_JsonDoc["messageId"]   = strMQTTSendPackage.messageId++;
    MQTT_JsonDoc["data"]["deviceId"] = fullDeviceID;
    MQTT_JsonDoc["data"]["userId"] = bUserId;
    MQTT_JsonDoc["data"]["temp"] = temp;
    MQTT_JsonDoc["data"]["pulse"] = HeartRate;
    MQTT_JsonDoc["data"]["spo2"] = SPO2;
    MQTT_JsonDoc["data"]["dateTime"] = strTime;
    MQTT_JsonDoc["data"]["evaluationResult"] = "";
    MQTT_JsonDoc["data"]["position"] = "finger";
    serializeJson(MQTT_JsonDoc, msg);
    Serial.println(msg);
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "iot", msg);
    return true;
  }
  return false;
}

bool App_mqtt_SendTemp(double temp)
{
  if(wifi_mqtt_isConnected())
  {
    /* Json send message */
    sprintf(strTime,
            "%d-%d-%dT%s",wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    Serial.println(strTime);
    sprintf(fullTopic, "tele/%s/iot", fullDeviceID);
    Serial.println(fullTopic);
    /* Add to json */
    MQTT_JsonDoc.clear();
    MQTT_JsonDoc["owner"]   = typeOwneriot;
    MQTT_JsonDoc["topic"]   = fullTopic;
    MQTT_JsonDoc["type"]    = iotSendTemp;
    MQTT_JsonDoc["index"]   = strMQTTSendPackage.index;
    MQTT_JsonDoc["total"]   = strMQTTSendPackage.total;
    MQTT_JsonDoc["messageId"]   = strMQTTSendPackage.messageId++;
    MQTT_JsonDoc["data"]["deviceId"] = fullDeviceID;
    MQTT_JsonDoc["data"]["userId"] = bUserId;
    MQTT_JsonDoc["data"]["temp"] = temp;
    MQTT_JsonDoc["data"]["dateTime"] = strTime;
    MQTT_JsonDoc["data"]["evaluationResult"] = "";
    MQTT_JsonDoc["data"]["position"] = "finger";
    serializeJson(MQTT_JsonDoc, msg);
    Serial.println(msg);
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "iot", msg);
    return true;
  }
  return false;
}

bool App_mqtt_SendSPO2(int HeartRate, int SPO2)
{
  if(wifi_mqtt_isConnected())
  {
    /* Json send message */
    sprintf(strTime,
            "%d-%d-%dT%s",wifi_ntp_getYears(),
            wifi_ntp_getMonths(),
            wifi_ntp_getDays(),
            wifi_ntp_getTime());
    Serial.println(strTime);
    sprintf(fullTopic, "tele/%s/iot", fullDeviceID);
    Serial.println(fullTopic);
    /* Add to json */
    MQTT_JsonDoc.clear();
    MQTT_JsonDoc["owner"]   = typeOwneriot;
    MQTT_JsonDoc["topic"]   = fullTopic;
    MQTT_JsonDoc["type"]    = iotSendSpo2;
    MQTT_JsonDoc["index"]   = strMQTTSendPackage.index;
    MQTT_JsonDoc["total"]   = strMQTTSendPackage.total;
    MQTT_JsonDoc["messageId"]   = strMQTTSendPackage.messageId++;
    MQTT_JsonDoc["data"]["deviceId"] = fullDeviceID;
    MQTT_JsonDoc["data"]["userId"] = bUserId;
    MQTT_JsonDoc["data"]["pulse"] = HeartRate;
    MQTT_JsonDoc["data"]["spo2"] = SPO2;
    MQTT_JsonDoc["data"]["dateTime"] = strTime;
    MQTT_JsonDoc["data"]["evaluationResult"] = "";
    MQTT_JsonDoc["data"]["position"] = "finger";
    serializeJson(MQTT_JsonDoc, msg);
    Serial.println(msg);
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "iot", msg);
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
