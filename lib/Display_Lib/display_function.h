#ifndef _DISPLAY_FUNCTION_H_
#define _DISPLAY_FUNCTION_H_

void display_Setup(uint8_t bMODE);

void display_config(double temp);

void display_config1(double temp, int Bpm, int SPO2);

void display_config2(double temp);

void display_config3(int Bpm, int SPO2);

void display_single_temp_shot(double temp);

void display_single_spo2_shot(int Bpm, int SPO2);

void display_state(uint8_t bStateUserTask);

void display_server_connect_state(bool bConnectServerStatus);

#endif