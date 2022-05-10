#include <Arduino.h>
#include <sensor_function.h>
//#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

/* BLE Variable */
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

//uint8_t value[] = "HR:xxx,SPO2:xx,TEMP:xx";
char value[] = "HR:%d,SPO2:%d,TEMP:%d";

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
int beatAvg = 0;

/* SPO2 varialble */
int oxygen;

void BLE_Setup(void);
void OLED_Display(double temp, int heartBeat, int32_t SPO2);

void task_Temp(void *parameter) {
  for(;;) {
    temp_obj = mlx_getTemp();
    OLED_Display(temp_obj, beatAvg, oxygen);

    if (deviceConnected) 
    {
      /*
      value[3] = beatAvg/100 + 48;
      value[4] = (beatAvg - ((value[3]-48)*100))/10 + 48;
      value[5] = beatAvg%10 + 48;

      value[12] = oxygen/10 + 48;
      value[13] = oxygen%10 + 48;

      value[20] = (int)temp_obj/10 + 48;
      value[21] = (int)temp_obj%10 + 48;
      */
      sprintf(value, "HR:%d,SPO2:%d,TEMP:%d", beatAvg, oxygen, (int)temp_obj);
      if(beatAvg>99){
        pCharacteristic->setValue((uint8_t*)&value, sizeof(value) + 1);
        pCharacteristic->notify();
      }
      else{
        pCharacteristic->setValue((uint8_t*)&value, sizeof(value));
        pCharacteristic->notify();
      }
      
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void task_MAX3010x(void *parameter) {
  for(;;) {
    MAX30105_getValue(oxygen, beatAvg);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
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