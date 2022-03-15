# Small Atmel Studio projects demonstrating the use of libraries for DS18B20, DS1307 and I2C LCD.

This folder contains files that are common to the projects in the subfolders. All projects require the files
twi.h/c and lcd.h/cpp. Most projects use one or more additional files:

HelloWorld project:

	The simplest example of using the lcd library. Displays "Hello World" on an LCD.

ReadRomCode project:

	Reads the 64-bit lasered ROM code from a DS18B20 digital Thermometer.
	Requires:
		onewire.h
		onewire.c
		ds18b20.h
		ds18b20.c
		
DualThermo project:

	Displays temperature readings from two DS18B20 digital thermometers on the same onewire line.
	Requires:
		onewire.h
		onewire.c
		ds18b20.h
		ds18b20.c

DigitalClock project:

	A simple digital clock demonstrating the use of the DS1307 real time clock, external interrupts 
	and the microcontroller's internal timer.
	Requires:
		ds1307.h
		ds1307.cpp
		timer.h
		timer.cpp
