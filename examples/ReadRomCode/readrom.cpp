/*
 * readrom.cpp
 *
 * Created: 19.02.2022
 * Author : Frank Bjørnø
 *
 * Purpose: Read the 64-bit lasered ROM code from a ds18b20 digital thermometer
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

#include <stdio.h>
#include <stdint.h>

#include "lcd.h"
#include "ds18b20.h"

int main(void)
{
		//  declare buffers
	char    buffer[17];							//  used to prepare display output. 16 chars + terminator.
	uint8_t romcode[8];							//  used to store ROM code.
		
		//  declare, instantiate and initialize the display
	LCD display;
	display.init();
	display.backlight(ON);
	
		//  check that ds18b20 is connected, exit on failure
	if (!ds18b20_is_connected())
	{
		display.print("DS18b20 offline.");
		return 1;							//  terminate program
	}
	
		//  read ROM, exit on failure
	if(!ds18b20_read_rom(romcode))
	{
		display.print("Reading failed.");
		return 1;
	}
	
		//  display message
	display.pos(FIRST, 4);							//  position cursor on first line, 4th column
	display.print("ROM CODE");
	
		//  prepare character string with ROM code
		//  ROM code is stored in a little-endian system, meaning that least significant byte 
		//  is stored at the smallest address and the most significant byte at the largest.
	char *ptr = buffer;
	for (int i = 7; i >= 0; i--)						//  read most significant byte first
	{
		sprintf(ptr, "%02x", romcode[i]);
		ptr += 2;
	}
	
		//  display ROM code
	display.line(SECOND);
	display.print(buffer);	
}
