#ifndef SENSOR_FUNCTION_H
#define SENSOR_FUNCTION_H

void sensor_Setup(void);

double sensor_getTemp(void);

int sensor_getHeardBeat(void);

int sersor_getSPO2(void);

void sensor_updateValue(void);

bool sensor_processing(int &maxSPO2, int maxHB);

#endif