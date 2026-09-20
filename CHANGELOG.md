[2026-09-20T00:25]
Added ping-pong CDC packet buffers to prevent USB backpressure from overwriting an in-flight packet.
[2026-09-20T00:20]
Decoupled USB backpressure from queue advancement, added overwrite-on-overrun buffering, 64-byte CDC draining, and bounded I2C read retries.
[2026-09-20T00:10]
Added loss-detecting 2 kHz accelerometer streaming over USB CDC with sequence-numbered raw samples.
[2026-09-20T00:01]
Enabled float formatting in newlib-nano so OLED RMS values using snprintf %f are displayed.
[2026-09-20T00:03]
Fixed firmware stuck on "Starting..." by enabling the EXTI9_5 NVIC interrupt for the ICM-42688 data-ready line (PB5), which was configured but never enabled.
[2026-09-20T00:02]
Added USB CDC virtual COM port output (CDC_Transmit_FS) and fixed the link error by adding icm42688.c to the CMake build.
[2026-09-20T00:00]
Added an explicit ICM-42688-P WHO_AM_I connection check before reset and an OLED no-connection diagnostic.
[2026-09-19T00:00]
Added ICM-42688-P 2 kHz, +/-16 g initialization, 500-sample DC-removed RMS vibration measurement, and OLED display output.
[2026-09-19T00:01]
Enabled the STM32F405 EXTI9_5 interrupt vector for accelerometer data-ready samples.
