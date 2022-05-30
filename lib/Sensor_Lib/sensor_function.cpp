#include "sensor_function.h"
#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_MLX90614.h>

#include "MAX30105.h"
#include "heartRate.h"

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
MAX30105 particleSensor;

static bool spo2Val = false;
static bool hearVal = false;
/* Heartbeat varialble */
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.

byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
unsigned long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;

int hearBeat;

/* SPO2 varialble */
#define SAMPLING 5
#define TIMETOBOOT 3000
#define FINGER_ON 3000
#define MINIMUM_SPO2 0.0

const double frate = 0.95;    //low pass filter for IR/red LED value to eliminate AC component
const double FSpO2 = 0.7;

double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;

double ESpO2 = 95.0;    //initial value of estimated SpO2
double SPO2Value;

void sensor_Setup(void)
{
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
    {
        //Serial.println("MAX30105 was not found. Please check wiring/power. ");
        //while (1);
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

double sensor_getTemp(void)
{
  return mlx.readObjectTempC();
}

int sensor_getHeardBeat(void)
{
  return hearBeat;
}

int sersor_getSPO2(void)
{
  return (int)SPO2Value;
}

void sensor_updateValue(void)
{
    static int sampleCnt = 0;

    double SpO2 = 0;
    int sumhearBeat = 0;

    particleSensor.check();
    uint32_t redValue = particleSensor.getFIFOIR();
    uint32_t irValue = particleSensor.getFIFORed();
    
    while(particleSensor.available())
    {
      if (checkForBeat(irValue) == true)
      {
        //We sensed a beat!
        unsigned long delta = (unsigned long)(millis() - lastBeat);
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20)
        {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;

          if(rateSpot == 0)
          {
            sumhearBeat = 0;
            for (byte x = 0 ; x < RATE_SIZE ; x++)
              sumhearBeat += rates[x];
            hearBeat = sumhearBeat/RATE_SIZE;
          }
        }
      }

      sampleCnt++;
      avered = avered * frate + (double)redValue * (1.0 - frate);
      aveir = aveir * frate + (double)irValue * (1.0 - frate);

      sumredrms += ((double)redValue - avered) * ((double)redValue - avered);
      sumirrms += ((double)irValue - aveir) * ((double)irValue - aveir);

      if ((sampleCnt % SAMPLING) == 0)
      {
        if (millis() > (unsigned long)TIMETOBOOT)
        {
          if (irValue < FINGER_ON){
            hearBeat = 0;
            ESpO2 = MINIMUM_SPO2;
          }

          if (ESpO2 <= -1)
            ESpO2 = 0;
          else if (ESpO2 > 100)
            ESpO2 = 100;

          SPO2Value = ESpO2;
        }
      }

      if ((sampleCnt % 100) == 0)
      {
        double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
        // Serial.println(R);
        SpO2 = -23.3 * (R - 0.4) + 100;               //http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
        ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2; //low pass filter

        sumredrms = 0.0;
        sumirrms = 0.0;
        sampleCnt = 0;
        break;
      }
      particleSensor.nextSample();
    }
}

bool sensor_processing(int &maxSPO2, int maxHB)
{
  static int timeout1 = 0;
  static int timeout2 = 0;

  if((maxSPO2 < SPO2Value) && (!spo2Val))
    maxSPO2 = SPO2Value;
  else if(maxSPO2 == SPO2Value)
    timeout1++;
  else
    spo2Val = true;

  if((maxHB < hearBeat) && (!hearVal))
    maxHB = hearBeat;
  else if(maxHB == hearBeat)
    timeout2++;
  else
    hearVal = true;

  if((spo2Val && hearVal) || ((timeout1 > 5) && (timeout2 > 5)) || (hearVal&&(timeout1 > 5)) || (spo2Val&&(timeout2 > 5))){
    spo2Val = false;
    hearVal = false;
    timeout1 = 0;
    timeout2 = 0;
    return 1;
  }
  return 0;
}