#include <Arduino.h>
#include <BLE_function.h>
#include <sensor_function.h>
#include <display_function.h>

#include <IO_function.h>
#include <Kernel_IO_function.h>

#include "main.h"

/*  IO Variable */
extern IO_Struct pLED1, pLED1, pLED3;
structIO_Manage_Output strLED_RD, strLED_GR, strLED_BL;
extern IO_Struct pBUT_1, pBUT_2;
structIO_Button strIO_Button_Value, strOld_IO_Button_Value;

eUSER_TASK_STATE eUserTask_State = E_STATE_STARTUP_TASK;
bool bFlag_1st_TaskState = true;
bool startFlag = false;
bool WorkMode = false;

uint16_t intervalTimeMs;

/* Sensor Varialble */
int MaxSPO2 = 0;
int MaxHearbeat = 0;

void task_Application(void *parameter) {
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
        }
        break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task_IO(void *parameter) {
  for(;;) {
    if(START_BUT_VAL == eButtonSingleClick)
    {
      START_BUT_VAL = eButtonHoldOff;
      startFlag = true;
      LED_BLUE_TOG;
    }

    if(MODE_BUT_VAL == eButtonSingleClick)
    {
      MODE_BUT_VAL = eButtonHoldOff;
      LED_GREEN_TOG;
      bFlag_1st_TaskState = true;
      if(WorkMode){
        eUserTask_State = E_STATE_ONESHOT_TASK;
        WorkMode = false;
      }
      else{
        eUserTask_State = E_STATE_CONTINUOUS_TASK;
        WorkMode = true;
      }
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
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

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

  delay(1000);
  //wifi_Setup();
  //xTaskCreate(task_display,"Task 1",8192,NULL,2,NULL);
  xTaskCreate(task_IO,"Task 2",8192,NULL,1,NULL);
  xTaskCreate(task_Kernel_IO,"Task 3",8192,NULL,1,NULL);
  xTaskCreate(task_Application,"Task 4",8192,NULL,1,NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  sensor_updateValue();
  
}