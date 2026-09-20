---
name: icm-42688-driver
description: Use when implementing, debugging, or explaining the ICM-42688-P accelerometer/gyroscope driver in this firmware. Covers the register map, I2C read/write patterns via STM32 HAL, power modes, ODR/FSR selection, data registers, and the EXTI5 data-ready interrupt. Grounds driver code in the verified datasheet facts in docs/datasheet-icm-42688.txt.
---

# ICM-42688-P Driver

The ICM-42688-P is a 6-axis (3-axis gyro + 3-axis accel) MotionTracking device with a 2 KB FIFO, 16-bit ADCs, and I2C/SPI/I3C interfaces. In this project it is wired on **I2C1 (PB6=SCL, PB7=SDA) at 400 kHz**, with the data-ready interrupt on **PB5 (EXTI5, rising)**.

Full reference: `docs/datasheet-icm-42688.txt` (searchable text). Register map is in section 13; detailed descriptions in section 14.

## I2C address

- 7-bit slave address is `b110100X`; the LSB comes from the `AP_AD0` pin.
- `AP_AD0 = 0` → `0x68`; `AP_AD0 = 1` → `0x69`.
- For STM32 HAL, pass the **8-bit** address: `0x68 << 1 = 0xD0` (or `0x69 << 1 = 0xD2`).

## I2C read/write via STM32 HAL

Use `HAL_I2C_Mem_Read` / `HAL_I2C_Mem_Write` on `hi2c1`. The device auto-increments the register address for burst reads/writes.

```c
#define ICM42688_ADDR   (0x68 << 1)   /* 8-bit address for HAL */

/* Read a single register */
uint8_t icm_read_reg(uint8_t reg) {
    uint8_t val = 0;
    HAL_I2C_Mem_Read(&hi2c1, ICM42688_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
    return val;
}

/* Write a single register */
void icm_write_reg(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(&hi2c1, ICM42688_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

/* Burst-read N bytes starting at reg (e.g. 6 accel bytes from 0x1F) */
void icm_read_burst(uint8_t reg, uint8_t *buf, uint16_t n) {
    HAL_I2C_Mem_Read(&hi2c1, ICM42688_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, n, 100);
}
```

## Key registers (User Bank 0)

| Reg | Name | Purpose |
|---|---|---|
| 0x11 | `DEVICE_CONFIG` | bit0 `SOFT_RESET_CONFIG` (write 1, wait 1 ms) |
| 0x1D/0x1E | `TEMP_DATA1/0` | temperature, 16-bit two's complement |
| 0x1F–0x24 | `ACCEL_DATA_X1..Z0` | accel X/Y/Z, 16-bit two's complement |
| 0x25–0x2A | `GYRO_DATA_X1..Z0` | gyro X/Y/Z, 16-bit two's complement |
| 0x2D | `INT_STATUS` | bit3 `DATA_RDY_INT` (clears on read) |
| 0x4E | `PWR_MGMT0` | bits1:0 `ACCEL_MODE`, bits3:2 `GYRO_MODE` |
| 0x4F | `GYRO_CONFIG0` | bits7:5 `GYRO_FS_SEL`, bits3:0 `GYRO_ODR` |
| 0x50 | `ACCEL_CONFIG0` | bits7:5 `ACCEL_FS_SEL`, bits3:0 `ACCEL_ODR` |
| 0x75 | `WHO_AM_I` | should read `0x47` |

## Power modes (`PWR_MGMT0`, 0x4E)

- `ACCEL_MODE` (bits1:0): `00`=off, `10`=Low Power (LP), `11`=Low Noise (LN).
- `GYRO_MODE` (bits3:2): `00`=off, `01`=standby, `11`=Low Noise (LN).
- Device powers up in **sleep mode** (both off). After turning a sensor on, do not issue register writes for **200 µs**; gyro needs **45 ms** to be ready.

## ODR selection (`ACCEL_CONFIG0` 0x50 / `GYRO_CONFIG0` 0x4F, bits3:0)

Accel ODR codes: `0110`=1 kHz (default), `0111`=200 Hz, `1000`=100 Hz, `1001`=50 Hz, `1010`=25 Hz, `1111`=500 Hz.
Gyro ODR codes: `0110`=1 kHz (default), `0111`=200 Hz, `1000`=100 Hz, `1001`=50 Hz, `1111`=500 Hz.

For drone-motor vibration analysis, a 1 kHz accel ODR is a good default (Nyquist 500 Hz covers motor harmonics).

## FSR selection (bits7:5)

Accel (`ACCEL_FS_SEL`): `000`=±16g, `001`=±8g, `010`=±4g, `011`=±2g.
Gyro (`GYRO_FS_SEL`): `000`=±2000dps, `001`=±1000dps, `010`=±500dps, `011`=±250dps, `100`=±125dps.

Sensitivity (LSB/g): ±16g→2048, ±8g→4096, ±4g→8192, ±2g→16384.

## Reading accel data

```c
typedef struct {
    int16_t x, y, z;
} icm_accel_t;

icm_accel_t icm_read_accel(void) {
    uint8_t buf[6];
    icm_read_burst(0x1F, buf, 6);          /* ACCEL_DATA_X1 .. Z0 */
    icm_accel_t a;
    a.x = (int16_t)((buf[0] << 8) | buf[1]);
    a.y = (int16_t)((buf[2] << 8) | buf[3]);
    a.z = (int16_t)((buf[4] << 8) | buf[5]);
    return a;
}
```

Convert to g: `g = raw / sensitivity` (e.g. `raw / 2048.0f` at ±16g).

## Data-ready interrupt (EXTI5)

- Configure the ICM to assert INT1 on data ready: set `UI_DRDY_INT1_EN` (bit3) in `INT_SOURCE0` (0x65).
- INT1 polarity/drive: `INT_CONFIG` (0x14) — bit0 `INT1_POLARITY` (0=active low, 1=active high), bit1 `INT1_DRIVE_CIRCUIT` (0=open drain, 1=push-pull).
- The MCU side is already set up: PB5 is `GPIO_MODE_IT_RISING` (EXTI5). Add the handler in `Src/stm32f4xx_it.c` (`EXTI5_IRQHandler` → `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5)`), and in `HAL_GPIO_EXTI_Callback` set a flag / read the sensor.
- Note: datasheet recommends setting `INT_ASYNC_RESET` (bit4 of `INT_CONFIG1`, 0x64) to 0 from its default of 1 for proper INT pin operation.

## Init sequence (typical)

1. Soft reset: write `0x01` to `DEVICE_CONFIG` (0x11), wait 1 ms.
2. (Optional) verify `WHO_AM_I` (0x75) == `0x47`.
3. Set `PWR_MGMT0` (0x4E): accel LN mode (`0x03`), gyro off.
4. Set `ACCEL_CONFIG0` (0x50): e.g. ±16g + 1 kHz → `(0 << 5) | 0x06` = `0x06`.
5. Enable data-ready on INT1: `INT_SOURCE0` (0x65) |= `0x08`.
6. Configure `INT_CONFIG` (0x14) for active-high push-pull if needed.

## Pitfalls

- **Register banks**: most config regs are in Bank 0 (default). Bank 1/2/4 regs (filters, APEX, offsets) require switching via `REG_BANK_SEL` (0x76) — not needed for basic accel reads.
- **Do not modify non-ODR/FSR/mode registers while sensors are running** — turn sensors off, change, turn back on (datasheet §12.9).
- **Invalid data**: before the first ODR sample, accel/gyro/temp read `-32768` (or hold last valid). Check `DATA_RDY_INT` before trusting a sample.
- **Temperature**: `°C = (TEMP_DATA / 132.48) + 25`.
- **I2C bus**: accel is on `hi2c1` (400 kHz), NOT `hi2c2` (display).