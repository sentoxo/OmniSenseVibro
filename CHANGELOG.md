[2026-09-28T20:13]
Changed per-axis peak RMS to the maximum value in the rolling 5-second RMS window.

[2026-09-28T20:03]
Added a per-axis peak RMS column for the highest 500-sample RMS window.

[2026-09-28T19:38]
Restored OLED transmission, discard, queue-overrun, and I2C-failure statistics alongside axis RMS readings.

[2026-09-28T19:34]
Sized the OLED formatting buffer to avoid integer-format truncation warnings.

[2026-09-28T19:34]
Formatted OLED RMS values using integer centi-units instead of floating-point printf.

[2026-09-28T19:30]
Changed the OLED to show per-axis current RMS acceleration and its rolling 5-second average in m/s2.

[2026-09-21T00:35]
Preserved sequence IDs correctly when the sample ring is full instead of overwriting the oldest queued sample metadata.
[2026-09-21T00:30]
Reduced sequence storage overhead with a parallel queue and kept OLED RMS processing active while USB streaming is unavailable.
[2026-09-21T00:20]
Assigned sequence numbers at sensor capture, gated CDC output on DTR, discarded stale samples without a host, and added OLED counters for transmitted, discarded, overrun, and I2C-failed samples.
[2026-09-21T00:05]
Corrected batch tail handling and changed the OLED status line to show transmitted sample packets and queue drops.
[2026-09-21T00:00]
Optimized USB CDC streaming with 256-byte batches, loss-safe queue commits, and MCU-side transmitted packet counting on the OLED.
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
