#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "display_function.h"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)

#define MAX_BRIGHTNESS 255

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MODE_BLE  1
#define MODE_WIFI 2
#define MODE_DUAL 3

void display_Setup(uint8_t bMODE)
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setCursor(25,15);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("FPT Wearable");
  display.setCursor(25,30);
  display.setTextSize(1);
  display.print("Starting...!");
  display.setCursor(25,45);
  display.setTextSize(1);
  if(bMODE == MODE_BLE)
  {
    display.print("MODE BLE!");
  }
  else if(bMODE == MODE_WIFI)
  {
    display.print("MODE WIFI!");
  }
  else if(bMODE == MODE_DUAL)
  {
    display.print("MODE DUAL!");
  }
  display.display();
}

#define MODE_MANUAL  1
#define MODE_AUTO    2

void display_config(double temp, uint8_t mode)
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setCursor(30,15);  
  display.setTextSize(1);
  display.print("TEMPERATURE");

  display.setCursor(25,30);  
  display.setTextSize(2);
  display.print(temp);
  display.print((char)247);
  display.print("C");
  
  display.setCursor(40,50);  
  display.setTextSize(1);
  if(mode == MODE_MANUAL)
  {
    display.print("ONE SHOT");
  }
  else
  {
    display.print("AUTO");
  }

  display.display(); 
}

void display_factory_reset(void)
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setCursor(30,15);  
  display.setTextSize(2);
  display.print("FACTORY");
  display.setCursor(30,35);
  display.print("RESET");  
  
  display.display(); 
}

void display_config1(double temp, int Bpm, int SPO2)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  //display.setCursor(0,0);
  //display.print("ContinuousMode");
  display.setCursor(5,15);  
  display.print("TEMPERATURE:");
  display.print(temp);
  display.print((char)247);
  display.print("C");

  display.setTextSize(1);
  display.setCursor(20,30);  
  display.print("%SpO2");
  display.setCursor(80,30);  
  display.print("PRbpm");

  display.setTextSize(2);
  display.setCursor(23,45);  
  display.print(SPO2);
  display.setCursor(80,45);  
  display.print(Bpm);
  
  display.display(); 
}

typedef enum {  
    E_STATE_STARTUP_TASK,
    E_STATE_PROCESSING_TASK,
    E_STATE_ONESHOT_TASK,
    E_STATE_ONESHOT_TASK_TEMP,
    E_STATE_ONESHOT_TASK_SPO2,
    E_STATE_CONTINUOUS_TASK,
    E_STATE_TEST_CONNECTION_TASK,
    E_STATE_FACTORY_RESET_TASK,
    E_STATE_CANCEL_CONNECTION_TASK,
} eUSER_TASK_STATE;

void display_state(uint8_t bStateUserTask)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(25,15);  
  display.print("STATE:");
  display.setTextSize(1);
  display.setCursor(10,40); 
  switch (bStateUserTask)
  {
  case E_STATE_TEST_CONNECTION_TASK/* constant-expression */:
    /* code */
    display.print("TEST CONNECTION ");
    break;
  case E_STATE_STARTUP_TASK/* constant-expression */:
    /* code */
    display.print("START UP ");
    break;
  case E_STATE_ONESHOT_TASK_TEMP/* constant-expression */:
    /* code */
    display.print("MEASURE TEMP ");
    break;
  case E_STATE_ONESHOT_TASK_SPO2/* constant-expression */:
    /* code */
    display.print("MEASURE SPO2 ");
    break;
  case E_STATE_CANCEL_CONNECTION_TASK/* constant-expression */:
    /* code */
    display.print("CANCEL CONNECTION ");
    break;
  default:
    break;
  }
  display.display(); 
}

void display_server_connect_state(uint8_t bConnectServerStatus)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(5,15);  
  display.print("Connect server:");
  display.setCursor(1,30); 
  if(bConnectServerStatus == 1)
  {
    display.print("SERVER SUCCESS!");
  }
  else if(bConnectServerStatus == 2)
  {
    display.print("WIFI SUCCESS!");
  }
  else if(bConnectServerStatus == 0)
  {
    display.print("WIFI CONNECTING...");
  }
  display.display(); 
}

void display_config2(double temp)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(5,15);  
  display.print("TEMPERATURE:");
  display.print(temp);
  display.print((char)247);
  display.print("C");

  display.setCursor(25,35);
  display.print("PROCESSING..."); 

  display.display();
}

void display_config3(int Bpm, int SPO2)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10,15);  
  display.print("MESURING......");

  display.setTextSize(1);
  display.setCursor(20,30);  
  display.print("%SpO2");
  display.setCursor(80,30);  
  display.print("PRbpm");

  display.setTextSize(2);
  display.setCursor(23,45);  
  display.print(SPO2);
  display.setCursor(80,45);  
  display.print(Bpm);
  
  display.display(); 
}

void display_single_temp_shot(double temp)
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setCursor(10,15);  
  display.setTextSize(1);
  display.print("SINGLE TEMP SHOT");

  display.setCursor(25,35);  
  display.setTextSize(2);
  display.print(temp);
  display.print((char)247);
  display.print("C");
  
  display.display(); 
}

void display_single_spo2_shot(int Bpm, int SPO2)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10,15);  
  display.print("SINGLE SPO2 SHOT");

  display.setTextSize(1);
  display.setCursor(20,30);  
  display.print("%SpO2");
  display.setCursor(80,30);  
  display.print("PRbpm");

  display.setTextSize(2);
  display.setCursor(23,45);  
  display.print(SPO2);
  display.setCursor(80,45);  
  display.print(Bpm);
  
  display.display(); 
}