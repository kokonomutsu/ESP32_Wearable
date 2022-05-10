#ifndef SENSOR_FUNCTION_H
#define SENSOR_FUNCTION_H

void sensor_setUp(void);

double mlx_getTemp(void);

void MAX30105_getValue(int &oxy, int &Avg);

#endif