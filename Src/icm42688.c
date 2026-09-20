#include "icm42688.h"

#define ICM42688_ADDRESS              (0x68U << 1)
#define ICM42688_REG_DEVICE_CONFIG    0x11U
#define ICM42688_REG_INT_CONFIG       0x14U
#define ICM42688_REG_ACCEL_DATA       0x1FU
#define ICM42688_REG_PWR_MGMT0        0x4EU
#define ICM42688_REG_ACCEL_CONFIG0    0x50U
#define ICM42688_REG_INT_SOURCE0      0x65U
#define ICM42688_REG_WHO_AM_I         0x75U
#define ICM42688_WHO_AM_I             0x47U
#define ICM42688_TIMEOUT_MS           100U
#define ICM42688_READ_TIMEOUT_MS      1U
#define ICM42688_READ_RETRIES         3U

static HAL_StatusTypeDef ICM42688_WriteRegister(I2C_HandleTypeDef *hi2c,
                                                 uint8_t reg,
                                                 uint8_t value)
{
    return HAL_I2C_Mem_Write(hi2c, ICM42688_ADDRESS, reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1,
                             ICM42688_TIMEOUT_MS);
}

static HAL_StatusTypeDef ICM42688_ReadRegister(I2C_HandleTypeDef *hi2c,
                                                uint8_t reg,
                                                uint8_t *value)
{
    return HAL_I2C_Mem_Read(hi2c, ICM42688_ADDRESS, reg,
                            I2C_MEMADD_SIZE_8BIT, value, 1,
                            ICM42688_TIMEOUT_MS);
}

HAL_StatusTypeDef ICM42688_CheckConnection(I2C_HandleTypeDef *hi2c)
{
    uint8_t who_am_i;
    HAL_StatusTypeDef status;

    status = ICM42688_ReadRegister(hi2c, ICM42688_REG_WHO_AM_I, &who_am_i);
    if (status != HAL_OK) {
        return status;
    }

    return (who_am_i == ICM42688_WHO_AM_I) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef ICM42688_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;

    status = ICM42688_CheckConnection(hi2c);
    if (status != HAL_OK) {
        return status;
    }

    status = ICM42688_WriteRegister(hi2c, ICM42688_REG_DEVICE_CONFIG, 0x01U);
    if (status != HAL_OK) {
        return status;
    }
    HAL_Delay(1);

    /* Active-high, push-pull INT1; enable the data-ready interrupt. */
    status = ICM42688_WriteRegister(hi2c, ICM42688_REG_INT_CONFIG, 0x03U);
    if (status != HAL_OK) {
        return status;
    }
    status = ICM42688_WriteRegister(hi2c, ICM42688_REG_INT_SOURCE0, 0x08U);
    if (status != HAL_OK) {
        return status;
    }

    /* ACCEL_FS_SEL=0 (+/-16 g), ACCEL_ODR=0101 (2 kHz). */
    status = ICM42688_WriteRegister(hi2c, ICM42688_REG_ACCEL_CONFIG0, 0x05U);
    if (status != HAL_OK) {
        return status;
    }

    /* Accelerometer low-noise mode; keep the gyroscope powered down. */
    status = ICM42688_WriteRegister(hi2c, ICM42688_REG_PWR_MGMT0, 0x03U);
    if (status != HAL_OK) {
        return status;
    }
    HAL_Delay(1);

    return HAL_OK;
}

HAL_StatusTypeDef ICM42688_ReadAccel(I2C_HandleTypeDef *hi2c,
                                     ICM42688_AccelSample *sample)
{
    uint8_t data[6];
    HAL_StatusTypeDef status;
    uint32_t attempt;

    status = HAL_ERROR;
    for (attempt = 0U; attempt < ICM42688_READ_RETRIES; ++attempt) {
        status = HAL_I2C_Mem_Read(hi2c, ICM42688_ADDRESS, ICM42688_REG_ACCEL_DATA,
                                  I2C_MEMADD_SIZE_8BIT, data, sizeof(data),
                                  ICM42688_READ_TIMEOUT_MS);
        if (status == HAL_OK) {
            break;
        }
    }
    if (status != HAL_OK) {
        return status;
    }

    sample->x = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    sample->y = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    sample->z = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    return HAL_OK;
}
