#pragma once
#include <stdint.h>

int WM8960_I2c_Init();
int WM8960_I2c_Send(uint8_t address_7bit, uint16_t data_9bit);
int WM8960_I2c_Deinit();