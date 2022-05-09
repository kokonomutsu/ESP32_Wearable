#include <Arduino.h>
//#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "MAX30105.h"
#include "heartRate.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define bleServerName "FPT_HC_IoTS_BTempHRate01-1E9E"
#define SERVICE_UUID "7bde7b9d-547e-4703-9785-ceedeeb2863e"
#define CHARACTERISTIC_UUID "9d45a73a-b19f-4739-8339-ecad527b4455"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)

#define MAX_BRIGHTNESS 255
#define bufferLength 100

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
MAX30105 particleSensor;

/* BLE Variable */
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

uint8_t value[] = "HR:xxx,SPO2:xx,TEMP:xx";

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
/* Temp varialble */
double temp_obj;

/* Heartbeat varialble */
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg = 0;

/* SPO2 varialble */
#define SAMPLING 5
#define TIMETOBOOT 3000
#define FINGER_ON 3000
#define MINIMUM_SPO2 0.0

int Num = 100; //calculate SpO2 by this sampling interval
int oxygen;
int i = 0;
double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;

double ESpO2 = 95.0;    //initial value of estimated SpO2
double FSpO2 = 0.7;
double frate = 0.95;    //low pass filter for IR/red LED value to eliminate AC component

void BLE_Setup(void);
void OLED_Display(double temp, int heartBeat, int32_t SPO2);

void task_Temp(void *parameter) {
  for(;;) {
    temp_obj = mlx.readObjectTempC();
    OLED_Display(temp_obj, beatAvg, oxygen);

    if (deviceConnected) 
    {
      value[3] = beatAvg/100 + 48;
      value[4] = (beatAvg - ((value[3]-48)*100))/10 + 48;
      value[5] = beatAvg%10 + 48;

      value[12] = oxygen/10 + 48;
      value[13] = oxygen%10 + 48;

      value[20] = (int)temp_obj/10 + 48;
      value[21] = (int)temp_obj%10 + 48;

      pCharacteristic->setValue((uint8_t*)&value, sizeof(value));
      pCharacteristic->notify();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task_MAX3010x(void *parameter) {
  for(;;) {
    uint32_t ir, red;
    double fred, fir;
    double SpO2 = 0;

    particleSensor.check();
    red = particleSensor.getFIFOIR();
    ir = particleSensor.getFIFORed();
    
    while(particleSensor.available()){
      if (checkForBeat(ir) == true)
      {
        //We sensed a beat!
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20)
        {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;

          beatAvg = 0;
          for (byte x = 0 ; x < RATE_SIZE ; x++)
            beatAvg += rates[x];
          beatAvg /= RATE_SIZE;
        }
      }
      i++;
      fred = (double)red;
      fir = (double)ir;
      avered = avered * frate + (double)red * (1.0 - frate);
      aveir = aveir * frate + (double)ir * (1.0 - frate);
      sumredrms += (fred - avered) * (fred - avered);
      sumirrms += (fir - aveir) * (fir - aveir);

      if ((i % SAMPLING) == 0)
      {
        if (millis() > TIMETOBOOT)
        {
          if (ir < FINGER_ON){
            beatAvg = 0;
            ESpO2 = MINIMUM_SPO2;
          }
          //float temperature = particleSensor.readTemperatureF();
          if (ESpO2 <= -1)
          {
            ESpO2 = 0;
          }
          else if (ESpO2 > 100)
          {
            ESpO2 = 100;
          }

          oxygen = ESpO2;
        }
      }
      if ((i % Num) == 0)
      {
        double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
        // Serial.println(R);
        SpO2 = -23.3 * (R - 0.4) + 100;               //http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
        ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2; //low pass filter
        sumredrms = 0.0;
        sumirrms = 0.0;
        i = 0;
        break;
      }
      particleSensor.nextSample();
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  
  byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 400; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.enableDIETEMPRDY();
  
  mlx.begin(0x5A, &Wire);         //Initialize MLX90614
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
  display.display();

  BLE_Setup();
  delay(2000);
  xTaskCreate(task_Temp,"Task 1",8192,NULL,2,NULL); // (hàm thực thi, tên đặt cho hàm, stack size, context đưa vào argument của task, độ ưu tiên của task, reference để điều khiển task)
  xTaskCreate(task_MAX3010x,"Task 2",8192,NULL,1,NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

void BLE_Setup(void)
{
  BLEDevice::init(bleServerName);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
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