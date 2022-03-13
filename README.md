# Small Atmel Studio projects demonstrating the use of libraries for DS18B20, DS1307 and I2C LCD.

This folder contains files that are common to the projects in the subfolders. All projects require the files
twi.h/c and lcd.h/cpp. Most projects use one or more additional files:

The ReadRomCode and DualThermo Projects require
	onewire.h
	onewire.c
	ds18b20.h
	ds18b20.c
