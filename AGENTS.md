# OmniSenseVibro — AI Agent Instructions

Firmware for a small STM32F405RGT6 device that reads drone-motor vibration from an ICM-42688-P accelerometer and shows statistics on a 1.3" SH1106 OLED (I2C). CMake project built with the STM32Cube VS Code extension.

## Build

```bash
cmake --preset Debug          # configure (Ninja, GCC ARM, C11, cortex-m4)
cmake --build build/Debug     # build
```

- Output: `build/Debug/OmniSenseVibro.elf` (+ `.map` linker map).
- Presets: `Debug` (default, `-O0 -g3`) and `Release` (`-Os -g0`). Toolchain: `cmake/gcc-arm-none-eabi.cmake`.
- `Drivers/` is **git-ignored** — library changes are not version-controlled.

## Pinout (verified against `stm32f4xx_hal_msp.c` / `.ioc`)

| Function | Pin | Peripheral |
|---|---|---|
| Display SCL / SDA | PB10 / PB11 | I2C2 (100 kHz) |
| Accel SCL / SDA | PB6 / PB7 | I2C1 (400 kHz) |
| Accel INT | PB5 | EXTI5 (rising) |
| Voltage sense (VBUS) | PB0 | ADC1_IN8, divider ratio **0.4286** |
| System LED | PA15 | GPIO out (bicolor low/high, highZ off) |

## Current state

- `Src/main.c` `while(1)` loop is **empty** — no application logic yet. All `USER CODE BEGIN/END` blocks are empty.
- Peripherals initialized: GPIO, ADC1, I2C1, I2C2, USART1, USB_OTG_FS. `SystemClock_Config()` → SYSCLK 168 MHz.
- No ICM-42688-P driver, no FFT, no display code, no voltage/LED logic yet.

## Conventions

- STM32CubeMX style: `MX_*_Init()` for peripheral init, `HAL_*_MspInit()` for MSP, `Error_Handler()`, handles `h<periph><n>` (e.g. `hi2c1`).
- Keep `main.c` small — split logic into dedicated `.c`/`.h` files.
- Write doc comments in files. Ask if unsure. **Compile to test** after changes.
- After every change, append a timestamped entry to `CHANGELOG.md`:
  ```
  [2026-09-19T19:10]
  Added a correction for DC offset in fft.c
  ```
- Write code in user sections -> "USER CODE START XXX" CODE "USER CODE END XXX"

## Pitfalls

- **`ssd1306_conf.h` uses `hi2c3`** — the display is on `hi2c2`. It will not build until changed.
- **CMSIS DSP (FFT) is not wired into the build** — `Drivers/CMSIS/DSP` sources aren't compiled/linked. FFT is unavailable until added to `cmake/stm32cubemx/CMakeLists.txt`.
- **ssd1306 library not in the build** — must be added to `cmake/stm32cubemx/CMakeLists.txt` (sources + include path).
- I2C1 (accel) = 400 kHz, I2C2 (display) = 100 kHz.
- Memory budget: 128K RAM total, heap 0x200, stack 0x400. Large FFT buffers (e.g. 1024-pt f32 = 8 KB) must fit; consider CCMRAM (64K).

## Docs

- `README.md` — user docs: architecture, features, usage, commands.
- `CHANGELOG.md` — change log (append after every change).
- `docs/datasheet-icm-42688.txt` — ICM-42688-P datasheet (searchable text). See the `icm-42688-driver` skill for the register map and I2C patterns.