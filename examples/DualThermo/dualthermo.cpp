/*
 * dualthermo.cpp
 *
 * Created: 28.10.2021
 * Author : Frank Bjørnø
 *
 * Purpose: Display temperature readings from two DS18B20 digital thermometers on the same line.
 *
 * License:
 *
 *          Copyright (C) 2021 Frank Bjørnø
 *
 *          1. Permission is hereby granted, free of charge, to any person obtaining a copy
 *          of this software and associated documentation files (the "Software"), to deal
 *          in the Software without restriction, including without limitation the rights
 *          to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *          of the Software, and to permit persons to whom the Software is furnished to do
 *          so, subject to the following conditions:
 *
 *          2. The above copyright notice and this permission notice shall be included in all
 *          copies or substantial portions of the Software.
 *
 *          3. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 *          INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *          PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *          HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *          CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 *          OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 *     If you are using AtmelStudio and get error messages like 'extended initializer lists 
 *     only available with -std=c++11 or -std=gnu++11', goto Project -> [Project Name] Properties (Alt + F7), 
 *     toolchain -> AVR/GNU C++ Compiler -> Miscellaneous and set other flags to '-std=c++11'
 *     (without the quotation marks).
 */  


/*
 *     F_CPU has to be defined in order to set the bitrate.
 *     If you use a 16Mhz crystal as a clock for the MCU, then leave this unchanged, 
 *     if not alter it accordingly
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif

#include <stdio.h>
#include <util/delay.h>
#include "lcd.h"
#include "ds18b20.h"

LCD   display;
char  display_buffer[6];								//  for preparing display output
float temp;

	//  known ROM codes of the two ds18b20 thermometers, stored in little-endian format	
unsigned char ROMa[8]  = {0x28, 0x6e, 0x38, 0xdd, 0x06, 0x00, 0x00, 0x39};
unsigned char ROMb[8]  = {0x28, 0x1c, 0x56, 0x5b, 0x0d, 0x00, 0x00, 0x6d};


/*
 *     Display temperature on a given line of the display
 *
 *     \param temp     temperature
 *     \param line     display line, either FIRST or SECOND. Enum defined in lcd.h
 */
void display_temp(float temp, lcd_line line)
{
	sprintf(display_buffer, "%2d.%1d%c", (int)temp, (int)(temp * 10) % 10, (char)223);
	display.pos(line, 11);
	display.print(display_buffer);
}



int main(void)
{
		//  initialize the display
    display.init();
	display.backlight(ON);
	
		//  the following text never changes
	display.print("Thermo 1:");
	display.line(SECOND);
	display.print("Thermo 2:");
	
		//  initialize thermometers
	ds18b20_set_rom(ROMa);								//  deactivate all but this
	ds18b20_set_resolution(DS18B20_9BIT);				//  set resolution to 9 bits
	
	ds18b20_set_rom(ROMb);
	ds18b20_set_resolution(DS18B20_9BIT);
		
	
		//  main loop		
	while(true)
	{		
			//  notify all devices on the line to start temperature conversion
		ds18b20_start_conversion();			
		_delay_ms(100);									//  wait for conversion
		
			//  get and display first temperature
		ds18b20_set_rom(ROMa);
		ds18b20_read_temp(&temp);
		display_temp(temp, FIRST);
		
			//  get and display second temperature
		ds18b20_set_rom(ROMb);
		ds18b20_read_temp(&temp);
		display_temp(temp, SECOND);
		
			//  wait 5 sec before reading again
		_delay_ms(5000);		
	}
}