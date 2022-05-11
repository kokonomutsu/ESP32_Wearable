#include "sensor_function.h"
#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_MLX90614.h>

#include "MAX30105.h"
#include "heartRate.h"

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
MAX30105 particleSensor;

/* Heartbeat varialble */
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;

/* SPO2 varialble */
#define SAMPLING 5
#define TIMETOBOOT 3000
#define FINGER_ON 3000
#define MINIMUM_SPO2 0.0

int Num = 100; //calculate SpO2 by this sampling interval

int i = 0;
double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;

double ESpO2 = 95.0;    //initial value of estimated SpO2
double FSpO2 = 0.7;
double frate = 0.95;    //low pass filter for IR/red LED value to eliminate AC component

void sensor_setUp(void)
{
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
    {
        Serial.println("MAX30105 was not found. Please check wiring/power. ");
        while (1);
    }
    /*
    byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
    byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
    byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    int sampleRate = 400; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411; //Options: 69, 118, 215, 411
    int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
    */
    particleSensor.setup(0x1F, 4, 2, 400, 411, 16384);
    particleSensor.enableDIETEMPRDY();
    
    mlx.begin(0x5A, &Wire);         //Initialize MLX90614
}

double mlx_getTemp(void)
{
    return mlx.readObjectTempC();
}

void MAX30105_getValue(int &oxy, int &Avg)
{
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

          Avg = 0;
          for (byte x = 0 ; x < RATE_SIZE ; x++)
            Avg += rates[x];
          Avg /= RATE_SIZE;
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
            Avg = 0;
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

          oxy = ESpO2;
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