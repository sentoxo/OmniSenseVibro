#ifndef ICM42688_H
#define ICM42688_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} ICM42688_AccelSample;

HAL_StatusTypeDef ICM42688_CheckConnection(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ICM42688_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ICM42688_ReadAccel(I2C_HandleTypeDef *hi2c, ICM42688_AccelSample *sample);

#endif
