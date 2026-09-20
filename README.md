# OmniSenseVibro

Firmware for a small STM32F405RGT6 device that reads drone-motor vibration from an **ICM-42688-P** accelerometer and shows statistics on a 1.3" **SH1106** OLED (I2C).

## Architecture

```
ICM-42688-P (I2C1, 400 kHz) ──► sample accel ──► FFT (CMSIS DSP) ──► stats ──► SH1106 OLED (I2C2, 100 kHz)
        ▲                                                                              ▲
        └── INT (PB5, EXTI5) data-ready                                            VBUS sense (PB0, ADC1)
```

- **Accelerometer**: ICM-42688-P on I2C1 (PB6/PB7), data-ready interrupt on PB5 (EXTI5). See the `icm-42688-driver` skill for the register map and I2C patterns.
- **Display**: SH1106 (SSD1306-compatible) on I2C2 (PB10/PB11) via the `stm32-ssd1306` library.
- **FFT**: CMSIS DSP library for vibration spectrum analysis.
- **Voltage sense**: VBUS on PB0 (ADC1_IN8) with a divider ratio of 0.4286.
- **System LED**: PA15, bicolor (low/high), high-Z off.

## Current status

- Peripherals initialized: GPIO, ADC1, I2C1, I2C2, USART1, USB_OTG_FS. SYSCLK 168 MHz.
- Not yet implemented: ICM-42688-P driver, FFT, display code, voltage/LED logic.

## Build

Requires the STM32Cube VS Code extension (or a CMake + GCC ARM toolchain).

```bash
cmake --preset Debug          # configure (Ninja, GCC ARM, C11, cortex-m4)
cmake --build build/Debug     # build
```

- Output: `build/Debug/OmniSenseVibro.elf` (+ `.map` linker map).
- Presets: `Debug` (default, `-O0 -g3`) and `Release` (`-Os -g0`).

## Pinout

| Function | Pin | Peripheral |
|---|---|---|
| Display SCL / SDA | PB10 / PB11 | I2C2 (100 kHz) |
| Accel SCL / SDA | PB6 / PB7 | I2C1 (400 kHz) |
| Accel INT | PB5 | EXTI5 (rising) |
| Voltage sense (VBUS) | PB0 | ADC1_IN8, divider ratio 0.4286 |
| System LED | PA15 | GPIO out (bicolor low/high, highZ off) |

## Known issues / TODOs

- `ssd1306_conf.h` uses `hi2c3` — must be changed to `hi2c2` (display bus) before the display library will build.
- CMSIS DSP (FFT) and the ssd1306 library are not yet wired into `cmake/stm32cubemx/CMakeLists.txt`.
- `Drivers/` is git-ignored — library changes are not version-controlled.

## Docs

- `AGENTS.md` — AI-agent instructions (build, conventions, pitfalls).
- `CHANGELOG.md` — change log (append after every change).
- `docs/datasheet-icm-42688.txt` — ICM-42688-P datasheet (searchable text).