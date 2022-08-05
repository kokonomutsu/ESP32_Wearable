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
bool App_BLE_SendTestConnection(uint8_t bConnectionStatus);
bool App_BLE_SendACK(Msg_teID_Type bBLEID);

void App_mqtt_callback(char* topic, byte* message, unsigned int length);
bool App_mqtt_SendTemp(double temp);
bool App_mqtt_SendSensor(double temp, int HeartRate, int SPO2);
bool App_mqtt_SendSPO2(int HeartRate, int SPO2);
bool App_mqtt_SendJWT(String jwt);
bool App_mqtt_SendStatus(uint8_t ErrorStt, uint8_t TempStt, uint8_t Spo2Stt);

void App_Parameter_Read(StrConfigPara *StrCfg);
void App_Parameter_Save(StrConfigPara *StrCfg);

bool vWifiTask(void);
void vUpdatePrivateKeyString(void);

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
bool bFlagTestConnectionStart = false;
char deviceprivatekey[100];
/* Working mode */
uint8_t bDeviceMode = MODE_BLE;
static bool bFlagStartAutoMeasuring = true;
/* Wifi task */
static bool bFlag1stWifiConnect = true;
static bool bFlag1stServerConnect = true;

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
          //display_Setup(bDeviceMode);
          /* Display mode */
          vTaskDelay(2000/ portTICK_PERIOD_MS);
          display_state(eUserTask_State);
          vTaskDelay(1000/ portTICK_PERIOD_MS);
          display_config(sensor_getTemp(), StrCfg1.Parameter.bWorkingMode);
          bFlag_1st_TaskState = false;
        }
        else{
          //Check finger in?
          display_config(sensor_getTemp(), StrCfg1.Parameter.bWorkingMode);
          /* STARTUP-IDLE just measure, not send BLE data */
          //App_BLE_SendTemp(sensor_getTemp());
          /* Ping server */
          #ifdef PING_TEST_MODE
            static uint8_t bCountPing;
            if((bCountPing++>20)&&(wifi_mqtt_isConnected()==true))
            {
              vTaskDelay(100);
              /* One shot measure temp to ping server */
              bFlag_1st_TaskState = true;
              eUserTask_State = E_STATE_ONESHOT_TASK_TEMP;
            }
          #endif /**/
        }
        break;
      case E_STATE_CANCEL_CONNECTION_TASK:
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: CANCEL CONNECTION TASK!");
          bFlag_1st_TaskState = false;
          display_state(eUserTask_State);
          /* Cancel wifi */
          bFlagTestConnectionStart = false;
          wifi_cancel_connect();
        }
        else
        {
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_STARTUP_TASK;
        }
      break;
      case E_STATE_ONESHOT_TASK:
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: ONESHOT TASK!");
          display_state(eUserTask_State);
          vTaskDelay(1000/ portTICK_PERIOD_MS);
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
          vTaskDelay(1000/ portTICK_PERIOD_MS);
          display_single_temp_shot(bSingleTempShot);
          bSingleTempShot = 0;
          bFlag_1st_TaskState = false;
        }
        else{
          /* Delay 1s to display 0 */
          vTaskDelay(500/ portTICK_PERIOD_MS);
          bSingleTempShot = sensor_getTemp();
          App_BLE_SendTemp(bSingleTempShot);
          App_mqtt_SendTemp(bSingleTempShot);
          /* Delay 1s to display */
          vTaskDelay(1000/ portTICK_PERIOD_MS);
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
          vTaskDelay(1000/ portTICK_PERIOD_MS);
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
            vTaskDelay(5000/ portTICK_PERIOD_MS);
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
          vTaskDelay(1000/ portTICK_PERIOD_MS);
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
          vTaskDelay(1000/ portTICK_PERIOD_MS);
          display_config1(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
          bFlag_1st_TaskState = false;
        }
        else{
          display_config1(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
          App_mqtt_SendSensor(sensor_getTemp(), sensor_getHeardBeat(), sersor_getSPO2());
        }
        break;
      case E_STATE_FACTORY_RESET_TASK:
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: FACTORY RESET!");
          bFlag_1st_TaskState = false;
          LED_BLUE_TOG;
          LED_RED_TOG;
          LED_GREEN_TOG;
          display_factory_reset();
        }
        else
        {
          vTaskDelay(2000 / portTICK_PERIOD_MS);
          ESP.restart();
        }
      break;
      case E_STATE_TEST_CONNECTION_TASK:
        static uint32_t bTestConnectionTimeOut = 0;
        if(bFlag_1st_TaskState)
        {
          Serial.println("[DEBUG]: TEST CONNECTION TASK!");
          display_state(eUserTask_State);
          bFlag_1st_TaskState = false;
          /* Connect server */
          if(wifi_connect_status()==true)
          {
            wifi_disconnect();
          }
          /* Reset timeout */
          bTestConnectionTimeOut = 0;
          bFlagTestConnectionStart = true;
          bFlag1stWifiConnect = true;
          bFlag1stServerConnect = true;
        }
        else{
          if(wifi_connect_status()==true)
          {
            if(wifi_mqtt_isConnected())
            {
              display_server_connect_state(1);
              eUserTask_State = E_STATE_STARTUP_TASK;
              bFlag_1st_TaskState = true;
              bFlagTestConnectionStart = false;
              LED_BLUE_TOG;
              bDeviceMode = MODE_WIFI;
              StrCfg1.Parameter.bLastMode = bDeviceMode;
              App_Parameter_Save(&StrCfg1);
              /* Feedback to BLE */
              App_BLE_SendTestConnection(1);
              vTaskDelay(2000/ portTICK_PERIOD_MS);
              //ESP.restart();
            }
            else
            {
              if(bTestConnectionTimeOut++<=20)//20s
              {
                display_server_connect_state(2);
                /* Feedback to BLE */
                App_BLE_SendTestConnection(2);
                vTaskDelay(2000/ portTICK_PERIOD_MS);
                LED_GREEN_TOG;
              }
              else
              {
                /* Feedback to BLE */
                App_BLE_SendTestConnection(1);
                vTaskDelay(2000/ portTICK_PERIOD_MS);
                eUserTask_State = E_STATE_STARTUP_TASK;
                bFlag_1st_TaskState = true;
                bFlagTestConnectionStart = false;
                LED_GREEN_TOG;
              }
            }
          }
          /* Wifi cannot connect */
          else
          {
            if(bTestConnectionTimeOut++<=20)//20s
            {
              display_server_connect_state(0);
              App_BLE_SendTestConnection(0);
              vTaskDelay(2000/ portTICK_PERIOD_MS);
              LED_RED_TOG;
              Serial.print(".");
            }
            else
            {
              display_server_connect_state(0);
              /* Feedback to BLE */
              eUserTask_State = E_STATE_STARTUP_TASK;
              bFlag_1st_TaskState = true;
              bFlagTestConnectionStart = false;
              LED_RED_TOG;
              /* Feedback to BLE */
              App_BLE_SendTestConnection(0);
            }
          }
        }
      break;
    }
    /* Task delay */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task_IO(void *parameter)
{
  for(;;) {
    if(START_BUT_VAL == eButtonSingleClick)
    {
      START_BUT_VAL = eButtonHoldOff;
      if(StrCfg1.Parameter.bWorkingMode == MODE_MANUAL){
        StrCfg1.Parameter.bWorkingMode = MODE_AUTO;
        LED_BLUE_TOG;
      }
      else if(StrCfg1.Parameter.bWorkingMode == MODE_AUTO){
        StrCfg1.Parameter.bWorkingMode = MODE_MANUAL;
        LED_GREEN_TOG;
      }
      App_Parameter_Save(&StrCfg1);
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
    else if(MODE_BUT_VAL == eButtonHoldOn){
      MODE_BUT_VAL = eButtonHoldOff;
      bDeviceMode = MODE_BLE;
      StrCfg1.Parameter.bLastMode = bDeviceMode;
      memset(StrCfg1.Parameter.WifiPASS,0,sizeof(StrCfg1.Parameter.WifiPASS));
      memset(StrCfg1.Parameter.WifiSSID,0,sizeof(StrCfg1.Parameter.WifiSSID));
      memset(StrCfg1.Parameter.ServerURL,0,sizeof(StrCfg1.Parameter.ServerURL));
      App_Parameter_Save(&StrCfg1);
      eUserTask_State = E_STATE_FACTORY_RESET_TASK;
      bFlag_1st_TaskState = true;
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

void task_TimingAuto(void *parameter)
{
  for(;;) {
    if((StrCfg1.Parameter.bWorkingMode == MODE_AUTO)&&(bFlagStartAutoMeasuring==true))
    {
      /* Check auto mode to measure */
      if(StrCfg1.Parameter.bLastMeasureCommand == eMEASURE_TEMP)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK_TEMP;
        Serial.println("[DEBUG]: AUTO TIMING MEASURE TEMP!");
      }
      else if(StrCfg1.Parameter.bLastMeasureCommand == eMEASURE_SPO2)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK_SPO2;
        Serial.println("[DEBUG]: AUTO TIMING MEASURE SPO2!");
      }
      vTaskDelay((StrCfg1.Parameter.bAutoMeasureInterval*1000) / portTICK_PERIOD_MS);
    }
    else
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
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
//  sprintf(fullDeviceID, "FPT_FCCIoT_%s", "09DE");//device Tai
//  sprintf(fullDeviceID, "FPT_FCCIoT_%s", "0A22");//device Khai

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

  if(StrCfg1.Parameter.bAutoMeasureInterval > 9999)
  {
    StrCfg1.Parameter.bAutoMeasureInterval = 1;
    App_Parameter_Save(&StrCfg1);
  }

  /* Testing auto-manual mode */
  //StrCfg1.Parameter.bWorkingMode = MODE_AUTO;
  //StrCfg1.Parameter.bAutoMeasureInterval = 10;/* 10s*/
  //StrCfg1.Parameter.bLastMeasureCommand = eMEASURE_TEMP;

  /* MODE MANUAL */
  //StrCfg1.Parameter.bWorkingMode = MODE_MANUAL;

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
//  device = new CloudIoTCoreDevice(
//      project_id, location, registry_id, device_id,
//      private_key_str);

  /* Device setup */
  sensor_Setup();
  display_Setup(bDeviceMode);
  /* Check device mode */
  if((bDeviceMode == MODE_BLE)||(bDeviceMode == MODE_DUAL))
  {
    Serial.println("[DEBUG]: BLE INIT!");
    BLE_Init(StrCfg1.Parameter.DeviceID, auBLETxBuffer, sizeof(auBLETxBuffer), auBLERxBuffer, sizeof(auBLERxBuffer));
  }
  
  /* Create task */
  if((bDeviceMode == MODE_BLE)||(bDeviceMode == MODE_DUAL))
  {
    xTaskCreate(task_BLE,"Task 1",8192,NULL,2,NULL);
  }
  xTaskCreate(task_IO,"Task 2",8192,NULL,1,NULL);
  xTaskCreate(task_Kernel_IO,"Task 3",8192,NULL,1,NULL);
  xTaskCreate(task_Application,"Task 4",8192,NULL,1,NULL);
  xTaskCreate(task_TimingAuto,"Task 5",4096,NULL,2,NULL);
}

bool vWifiTask(void)
{
    static bool bFlagGetJWT = true;
    static bool bReturn = false;
    /* Check flag to conenct wifi */
    if((bDeviceMode == MODE_WIFI)||(bFlagTestConnectionStart == true))
    {
      if(bFlag1stWifiConnect==true)
      {
        /* Connect wifi */
        wifi_connect(StrCfg1.Parameter.WifiSSID, StrCfg1.Parameter.WifiPASS);
        bFlag1stWifiConnect = false;
      }
      else
      {
        /* Check Wifi connect status */
        if(bFlag1stServerConnect==true)
        {
          if(wifi_connect_status() == true)
          {
              /* add device with private key */
              vUpdatePrivateKeyString();
              device = new CloudIoTCoreDevice(
                project_id, location, registry_id, device_id,
                deviceprivatekey);
              wifi_setup_mqtt(&App_mqtt_callback, StrCfg1.Parameter.ServerURL, 1883);
              bFlag1stServerConnect = false;
              bReturn = true;
          }
          else
          {
              Serial.print(".");
              vTaskDelay(500/ portTICK_PERIOD_MS);
              bReturn = false;
          }
        }
        else
        {
          /* Wifi loop */
          wifi_loop(fullDeviceID);
          if((wifi_mqtt_isConnected()==true)&&(bFlagGetJWT==true))
          {
            bFlagGetJWT = false;
            /* Sync time */
            configTime(0, 0, ntp_primary, ntp_secondary);
            Serial.println("Waiting on time sync...");
            while (time(nullptr) < 1510644967) {
              delay(10);
            }
            /* Update NTP */
            wifi_ntp_update();
            /* Try get JWT */
            Serial.println(getJwt().c_str());
            /* Send jwt */
            App_mqtt_SendJWT(jwt);
          }
        }
      }
    }
    return bReturn;
}

void loop() {
  // put your main code here, to run repeatedly:
  sensor_updateValue();
  /* Wifi task */
  vWifiTask();
}

void vUpdatePrivateKeyString(void)
{
  int counter = 0;
  /* onvert to private key string format */
  for(int bIndex=0;bIndex<(sizeof(StrCfg1.Parameter.PrivateKey)-1);bIndex+=2)
  {
      deviceprivatekey[counter] = StrCfg1.Parameter.PrivateKey[bIndex];
      counter++;
      deviceprivatekey[counter] = StrCfg1.Parameter.PrivateKey[bIndex+1];
      counter++;
      deviceprivatekey[counter] = ':'; 
      counter++;
  }
  /* erase last byte */
  counter--;
  deviceprivatekey[counter] = NULL; 
}
/****************************************************************************/
/***        BLE Function                                                  ***/
/****************************************************************************/
void App_BLE_ProcessMsg(uint8_t MsgID, uint8_t MsgLength, uint8_t* pu8Data)
{
  switch (MsgID)
  {
    case E_SSID_CFG_ID:
      memset(StrCfg1.Parameter.WifiSSID,0,sizeof(StrCfg1.Parameter.WifiSSID));
      if((MsgLength - 8) <= SSID_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.WifiSSID[i] = pu8Data[i];
        for(int j=i;j<SSID_MAX_SIZE;j++)
          StrCfg1.Parameter.WifiSSID[j] = 0x00;
        Serial.println(StrCfg1.Parameter.WifiSSID);
        Serial.println("Wifi ssid!\r\n");
        //App_Parameter_Save(&StrCfg1);
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
    case E_CANCEL_TEST_CONNECTION_ID:
      if(eUserTask_State == E_STATE_TEST_CONNECTION_TASK)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_CANCEL_CONNECTION_TASK;
      }
    break;
    case E_PASS_CFG_ID:
      memset(StrCfg1.Parameter.WifiPASS,0,sizeof(StrCfg1.Parameter.WifiPASS));
      if((MsgLength - 8) <= PASS_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.WifiPASS[i] = pu8Data[i];
        for(int j=i;j<PASS_MAX_SIZE;j++)
          StrCfg1.Parameter.WifiPASS[j] = 0x00;
        Serial.println(StrCfg1.Parameter.WifiPASS);
        Serial.println("Wifi password!\r\n");
        //App_Parameter_Save(&StrCfg1);
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
        StrCfg1.Parameter.bAutoMeasureInterval = temp;
        App_Parameter_Save(&StrCfg1);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    }
    case E_WORK_MODE_ID:
      /* Parse and get mode */
      if(pu8Data[0]==0)
      {
        StrCfg1.Parameter.bWorkingMode = MODE_MANUAL;
        bFlagStartAutoMeasuring = false;
      }
      else if(pu8Data[0]==1)
      {
        StrCfg1.Parameter.bWorkingMode = MODE_AUTO;
        bFlagStartAutoMeasuring = true;
      }
      /* Working mode */
      App_Parameter_Save(&StrCfg1);
      break;
    case E_ONESHOT_TEMP_ID:
      LED_GREEN_TOG;
      if(eUserTask_State != E_STATE_ONESHOT_TASK_TEMP)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK_TEMP;
        if(StrCfg1.Parameter.bWorkingMode == MODE_AUTO)
        {
          if(StrCfg1.Parameter.bLastMeasureCommand != eMEASURE_TEMP)
          {
            StrCfg1.Parameter.bLastMeasureCommand = eMEASURE_TEMP;
            App_Parameter_Save(&StrCfg1);
          }
          bFlagStartAutoMeasuring = true;
        }
      }
      break;
    case E_ONESHOT_SPO2_ID:
      LED_BLUE_TOG;
      if(eUserTask_State != E_STATE_ONESHOT_TASK_SPO2)
      {
        bFlag_1st_TaskState = true;
        eUserTask_State = E_STATE_ONESHOT_TASK_SPO2;
        if(StrCfg1.Parameter.bWorkingMode == MODE_AUTO)
        {
          if(StrCfg1.Parameter.bLastMeasureCommand != eMEASURE_SPO2)
          {
            StrCfg1.Parameter.bLastMeasureCommand = eMEASURE_SPO2;
            App_Parameter_Save(&StrCfg1);
          }
          bFlagStartAutoMeasuring = true;
        }
      }
      break;
    case E_URL_CFG_ID:
      memset(StrCfg1.Parameter.ServerURL,0,sizeof(StrCfg1.Parameter.ServerURL));
      if((MsgLength - 8) <= URL_MAX_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
          StrCfg1.Parameter.ServerURL[i] = pu8Data[i];
        for(int j=i;j<URL_MAX_SIZE;j++)
          StrCfg1.Parameter.ServerURL[j] = 0x00;
        Serial.println("URL!\r\n");
        Serial.println(StrCfg1.Parameter.ServerURL);
        //App_Parameter_Save(&StrCfg1);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    case E_DEVICE_DATA_ID:
      BLE_configMsg(12, 0, E_DEVICE_DATA_ID, SeqID++, 2, StrCfg1.Parameter.DeviceID);
      break;
    case E_PRIVATE_KEY_ID:
      memset(deviceprivatekey,0,sizeof(deviceprivatekey));
      if((MsgLength - 8) <= PRIVATE_KEY_SIZE)
      {
        int i;
        for(i=0;i<(MsgLength - 8);i++)
        {
          StrCfg1.Parameter.PrivateKey[i] = pu8Data[i];
        }
        for(int j=i;j<PRIVATE_KEY_SIZE;j++)
        {
          StrCfg1.Parameter.PrivateKey[j] = 0x00;
        }
        /* Update key string */
        vUpdatePrivateKeyString();
        Serial.println("Private key!\r");
        Serial.println(StrCfg1.Parameter.PrivateKey);
        Serial.println("Private key string!\r");
        Serial.println(deviceprivatekey);
        //App_Parameter_Save(&StrCfg1);
        App_BLE_SendACK((Msg_teID_Type)MsgID);
      }
      break;
    case E_STOP_MEASURE_ID:
      if(StrCfg1.Parameter.bWorkingMode == MODE_AUTO)
      {

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

bool App_BLE_SendACK(Msg_teID_Type bBLEID)
{
  if(BLE_isConnected())
  {
      BLE_configMsg(9, 0, bBLEID, SeqID++, 1, (uint8_t*)"1");
      return true;
  }
  return false;
}

bool App_BLE_SendTestConnection(uint8_t bConnectionStatus)
{
  if(BLE_isConnected())
  {
    if(bConnectionStatus==1)
    {
      BLE_configMsg(9, 0, E_TEST_CONNECTION_ID, SeqID++, 1, (uint8_t*)"1");
    }
    else if(bConnectionStatus==0)
    {
      BLE_configMsg(9, 0, E_TEST_CONNECTION_ID, SeqID++, 1, (uint8_t*)"0");
    }
    else if(bConnectionStatus==2)
    {
      BLE_configMsg(9, 0, E_TEST_CONNECTION_ID, SeqID++, 1, (uint8_t*)"2");
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
          if(StrCfg1.Parameter.bWorkingMode == MODE_AUTO)
          {
            if(StrCfg1.Parameter.bLastMeasureCommand != eMEASURE_TEMP)
            {
              StrCfg1.Parameter.bLastMeasureCommand = eMEASURE_TEMP;
              App_Parameter_Save(&StrCfg1);
            }
            bFlagStartAutoMeasuring = true;
          }
        }
      }
      else if(MQTT_JsonReceiveDoc["type"] == 8){
        if(eUserTask_State != E_STATE_ONESHOT_TASK_SPO2)
        {
          bUserId = MQTT_JsonReceiveDoc["data"]["userId"];
          bFlag_1st_TaskState = true;
          eUserTask_State = E_STATE_ONESHOT_TASK_SPO2;
          if(StrCfg1.Parameter.bWorkingMode == MODE_AUTO)
          {
            if(StrCfg1.Parameter.bLastMeasureCommand != eMEASURE_SPO2)
            {
              StrCfg1.Parameter.bLastMeasureCommand = eMEASURE_SPO2;
              App_Parameter_Save(&StrCfg1);
            }
            bFlagStartAutoMeasuring = true;
          }
        }
      }
      else if(MQTT_JsonReceiveDoc["type"] == 2){
          Serial.println("[DEBUG][MQTT]: Get config command from server!\r\n");
      }
    }
}
bool App_mqtt_SendJWT(String jwt)
{
  if(wifi_mqtt_isConnected())
  {
    /* Json send message */
    sprintf(strTime,
            "%04d-%02d-%02dT%s.000Z",wifi_ntp_getYears(),
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
    MQTT_JsonDoc["type"]    = iotReqCon;
    MQTT_JsonDoc["index"]   = strMQTTSendPackage.index;
    MQTT_JsonDoc["total"]   = strMQTTSendPackage.total;
    MQTT_JsonDoc["messageId"]   = strMQTTSendPackage.messageId++;
    MQTT_JsonDoc["data"]["deviceId"] = fullDeviceID;
    MQTT_JsonDoc["data"]["userId"] = bUserId;
    MQTT_JsonDoc["data"]["jwt"] = jwt;
    serializeJson(MQTT_JsonDoc, msg);
    Serial.println(msg);
    wifi_mqtt_publish(StrCfg1.Parameter.DeviceID, "iot", msg);
    return true;
  }
  return false;
}

bool App_mqtt_SendSensor(double temp, int HeartRate, int SPO2)
{
  if(wifi_mqtt_isConnected())
  {
    /* Json send message */
    sprintf(strTime,
            "%04d-%02d-%02dT%s.000Z",wifi_ntp_getYears(),
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
            "%04d-%02d-%02dT%s.000Z",wifi_ntp_getYears(),
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
            "%04d-%02d-%02dT%s.000Z",wifi_ntp_getYears(),
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
