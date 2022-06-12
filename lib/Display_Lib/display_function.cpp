#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "display_function.h"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)

#define MAX_BRIGHTNESS 255

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void display_Setup(void)
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setCursor(25,15);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("FPT Wearable");
  display.setCursor(25,35);
  display.setTextSize(1);
  display.print("Starting...!");
  display.display();
}

void display_config(double temp)
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setCursor(30,15);  
  display.setTextSize(1);
  display.print("TEMPERATURE");

  display.setCursor(25,35);  
  display.setTextSize(2);
  display.print(temp);
  display.print((char)247);
  display.print("C");
  
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