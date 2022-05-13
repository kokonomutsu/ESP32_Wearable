#include <Arduino.h>
#include <BLE_function.h>
//#include <wifi_function.h>
#include <sensor_function.h>
#include <IO_function.h>
#include <Kernel_IO_function.h>
//#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)

#define MAX_BRIGHTNESS 255
#define bufferLength 100

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//uint8_t value[] = "HR:xxx,SPO2:xx,TEMP:xx";
char value[] = "HR:%d,SPO2:%d,TEMP:%d";

/* Temp varialble */
double temp_obj;
/* Heartbeat varialble */
int beatAvg = 0;
/* SPO2 varialble */
int oxygen;

/*  IO Variable */
extern IO_Struct pLED1, pLED1, pLED3;
structIO_Manage_Output strLED_1, strLED_2, strLED_3;
extern IO_Struct pBUT_1, pBUT_2, pBUT_3;
structIO_Button strIO_Button_Value, strOld_IO_Button_Value;

void OLED_Display(double temp, int heartBeat, int32_t SPO2);

void task_Temp(void *parameter) {
  for(;;) {
    temp_obj = mlx_getTemp();
    OLED_Display(temp_obj, beatAvg, oxygen);
    sprintf(value, "HR:%d,SPO2:%d,TEMP:%d", beatAvg, oxygen, temp_obj);
    if(beatAvg > 99)
    {
      BLE_sendData((uint8_t *)&value, sizeof(value) + 1);
    }
    else{
      BLE_sendData((uint8_t *)&value, sizeof(value));
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task_MAX3010x(void *parameter) {
  for(;;) {
    MAX30105_getValue(oxygen, beatAvg);
  }
}
void task_IO(void *parameter)
{
  for(;;) {
    vIO_Output(&strLED_1, &pLED1);
	  vIO_Output(&strLED_2, &pLED2);
	  vIO_Output(&strLED_3, &pLED3);

    /*** Get BT Value  ***/
    vGetIOButtonValue(eButton1, (enumbool)pBUT_1.read(), &strOld_IO_Button_Value, &strIO_Button_Value);
    if(memcmp(strOld_IO_Button_Value.bButtonState,strIO_Button_Value.bButtonState,NUMBER_IO_BUTTON_USE))
		{
			/* Update OLD data value */
			memcpy(strOld_IO_Button_Value.bButtonState,strIO_Button_Value.bButtonState, NUMBER_IO_BUTTON_USE);
			/* Set flag new button */
			strIO_Button_Value.bFlagNewButton = eTRUE;
		}
		else
		{
			/* Clear flag new button */
			strIO_Button_Value.bFlagNewButton = eFALSE;
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
  /*
  sensor_setUp();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
 
  //Serial.println("Temperature Sensor MLX90614");
 
  display.clearDisplay();
  display.setCursor(25,15);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("FPT Wearable");
  display.setCursor(25,35);
  display.setTextSize(1);
  display.print("Initializing");
  display.display();*/

  //BLE_Setup();
  delay(2000);
  //wifi_Setup();
  //xTaskCreate(task_Temp,"Task 1",8192,NULL,2,NULL); // (hàm thực thi, tên đặt cho hàm, stack size, context đưa vào argument của task, độ ưu tiên của task, reference để điều khiển task)
  //xTaskCreate(task_MAX3010x,"Task 2",8192,NULL,1,NULL);
  xTaskCreate(task_IO,"Task 3",8192,NULL,1,NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  //wifi_loopTask();
  if(Serial.available())
  {
    char inChar = Serial.read();
    if(inChar == 'a'){
      vIO_ConfigOutput(&strLED_1, eTRUE, 1, 1, eFALSE);
    }
    else if(inChar == 'b'){
      vIO_ConfigOutput(&strLED_1, eFALSE, 1, 1, eFALSE);
    }
    else if(inChar == 'c'){
      vIO_ConfigOutput(&strLED_1, eTRUE, 4, 50, eFALSE);
    }
  }
  if(strIO_Button_Value.bButtonState[eButton1]==eButtonSingleClick)
  {
      strIO_Button_Value.bButtonState[eButton1] = eButtonHoldOff;
      vIO_ConfigOutput(&strLED_1, (enumbool)(1 - (int)pLED1.writeSta()), 1, 1, eFALSE);
  }
  else if(strIO_Button_Value.bButtonState[eButton1]==eButtonDoubleClick)
  {
      vIO_ConfigOutput(&strLED_1, eTRUE, 4, 50, eFALSE);
  }
}

void OLED_Display(double temp, int heartBeat, int32_t SPO2)
{
  display.clearDisplay();
  display.setCursor(0,10);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Temperature: ");
  display.print(temp);
  display.print((char)247);
  display.println("C");
  //display.display();

  display.setCursor(0,20);
  display.print("BPM = ");
  display.println(heartBeat);
  display.print("SPO2 = ");
  display.println(SPO2);
  
  display.display(); 
}