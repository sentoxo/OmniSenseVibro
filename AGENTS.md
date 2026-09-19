## Project
Project of small device with stm32f405rgt6, accelerometer ICM-42688-P and i2c oled 1.3" SH1106. Goal is to read vibration of drone motor and display statistic on a display. IT's cmake project used in vscode with stm32cube extension.
### Pinout
Display on PB10/PB11
accelerometer on PB7/PB6 + INT on PB5
voltage sense of vbus on PB0 with divider ratio of 0.4286
system led on PA15, bicolor low/high, highZ off
## Libs
There is library for display in Drivers\stm32-ssd1306-master
There is library for FFT in Drivers\CMSIS
### Docs
There is big datasheet in text format for searching information about ICM-42688-P
README.md is place for writing docs for user, project architecture, features, bugs, commands, instructions for use etc.

## Code format
Write comments for docs in files
Ask question if unsure
Dont make main.C to big, dive in smaller files .c/.h
Compile code for testing

## Status
After every change in code write new sentence in CHANGELOG.md about what was added/changed, example:
'''
[2026-09-19T19:10]
Added a correction for DC offset in fft.c

'''