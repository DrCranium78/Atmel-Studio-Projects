/*
 * ds18b20.h
 *
 * Version: 1.0.0
 * Created: 15.12.2020
 *  Author: Frank Bjørnø
 *
 * Purpose: To facilitate communication with ds18b20 devices.
 *
 * Limitations:
 *
 *          This code is mainly limited by the capabilities of onewire.c which does not support
 *          the SEARCH ROM command.
 *
 * Dependencies:
 *
 *          util/delay.h: The function ds18b20_read_temp uses this to wait for the ds18b20
 *                        device while it converts temperature.
 *          onewire.h:    The ds18b20 is a one wire device.
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
 *     F_CPU has to be defined in order to use the delay function in util/delay.h
 *     if you use a 16Mhz crystal as a clock for the MCU, then leave this unchanged, 
 *     if not alter it accordingly.
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif

#include <util/delay.h>
#include "onewire.h"
#include "ds18b20.h"



/*
 *     Define commands.
 */
#define CMD_CONVERT_TEMP		0x44
#define CMD_READ_SCRATCHPAD		0xBE
#define CMD_WRITE_SCRATCHPAD	0x4E



/*
 *     An array of 3 chars used to write to the DS18B20 scratchpad.
 *     Keep in mind that even though owi_write_byte requires unsigned 
 *     char as parameter, this is, in the end, just ones and zeros.
 *     The initial numbers represent Th = 125 or 0x7D in hexadecimal, 
 *     Tl = -55 or 0xC9 in hexadecimal two's complement representation, 
 *     and 12 bit resolution meaning 0.0625 degrees resolution.
 */
static unsigned char _ds18b20_config[3] = {0x7D, 0xC9,DS18B20_12BIT};

/*
 *     An array of 9 unsigned chars to hold one byte used as a boolean, byte 0, 
 *     and a 64 bit ROM code, byte 1 - 8.
 */
static unsigned char _ds18b20_address[9] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*
 *     An array of 5 unsigned chars to hold the contents of the DS18B20 scratchpad
 *     the contents are stored in the order: [0] Temp LSB, [1] Temp MSB, [2] Th, [3] Tl, [4] Config.
 */
static unsigned char _ds18b20_scratchpad[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

/*
 *     This writes the contents of _ds18b20_config to the scratchpad.
 */
static int ds18b20_write_scratchpad()
{	
	if (!owi_detect_presence()) return 0;					//  check whether ds18b20 is online
	
	
	//  skip or match ROM and send command to write scratchpad
	if (_ds18b20_address[0]) owi_match_rom(&_ds18b20_address[1]);
	else owi_skip_rom();											//  send skip ROM command
	owi_write_byte(CMD_WRITE_SCRATCHPAD);					//  send write scratchpad command
	
	for (int i = 0; i < 3; i++)
	{
		owi_write_byte(_ds18b20_config[i]);					//  write data
	}
	return 1;
}

/*
 *     This reads the first 5 bytes of the scratchpad and stores
 *     in _ds18b20_scratchpad
 *
 *     \return     1 if success, 0 if not.
 */
static int ds18b20_read_scratchpad()
{
		//  send reset pulse and check for presence
	if (!owi_detect_presence()) return 0;
	
		//  skip or match ROM and send command to read scratchpad
	if (_ds18b20_address[0]) owi_match_rom(&_ds18b20_address[1]);
	else owi_skip_rom();
	
		//  read scratchpad
	owi_write_byte(CMD_READ_SCRATCHPAD);
	for (int i = 0; i < 5; i++) _ds18b20_scratchpad[i] = owi_read_byte();
	
		//  terminate data transfer and return
	owi_detect_presence();	
	return 1;
} 


static void ds18b20_reset_rom()
{
	for (int i = 0; i < 9; i++) _ds18b20_address[i] = 0x00;
}

/*
 *     This combines the information in lsb and msb into a floating point number.
 */     
static float ds18b20_convert(unsigned char lsb, unsigned char msb)
{
	float temp = 0.0, factor = 0.5;
	
		//  isolate integral part by combing right nybble of msb with left nybble of lsb.
	int8_t integral = (msb << 4) | (lsb >> 4);
	
		//  Isolate binary decimal part, that is right nybble of lsb.
	uint8_t decimal = (lsb & 0x0F);
	
		//  Determine position of last active bit in decimal part.
	uint8_t n = 0x08 >> ((_ds18b20_config[2] & 0x60) >> 5);
	
		//  Convert from bicimal to decimal.
	for (uint8_t mask = 0x08; mask >= n; mask >>= 1)
	{
		temp   += factor * ((decimal & mask) != 0);		
		factor *= 0.5;
	}
	
		//  If integral part is negative, so must decimal part.
	if (integral < 0) temp *= -1.0;
	temp += integral;
	
	return temp;
}


	
int ds18b20_is_connected()
{
	return owi_detect_presence();
}



int ds18b20_start_conversion()
{
	if (!owi_detect_presence()) return 0;
	
	owi_skip_rom();
	owi_write_byte(CMD_CONVERT_TEMP);
	return 1;
}



int ds18b20_read_temp(float *temp)
{		
	unsigned char lsb = 0x00;
	unsigned char msb = 0x00;
	
	while(owi_is_busy()) _delay_ms(5);					//  temp. conversion takes at least 93.75ms
	
	ds18b20_read_scratchpad();
	
		//  get LSB and MSB from scratchpad	
	lsb = _ds18b20_scratchpad[0];
	msb = _ds18b20_scratchpad[1];
	
	*temp = ds18b20_convert(lsb, msb);
	ds18b20_reset_rom();
	return 1;
}



int ds18b20_set_resolution(const int res)
{			
	ds18b20_read_scratchpad();
	
		//  don't change the alarms
	_ds18b20_config[0] = _ds18b20_scratchpad[2];
	_ds18b20_config[1] = _ds18b20_scratchpad[3];
	_ds18b20_config[2] = res;	
	if (!ds18b20_write_scratchpad()) return 0;
	ds18b20_reset_rom();
	return 1;
}



int ds18b20_set_alarms(int8_t tl, int8_t th)
{
		/*
		 *     tl must be higher than -55
		 *     tl must be lower than th
		 *     th must be below 125.
		 *     if any of these conditions are broken, return early.
		 */
	if (tl < -55 || tl > th || th > 125) return 0;	
	
	ds18b20_read_scratchpad();	
	
		//  don't change the configuration register
	_ds18b20_config[0] = th;
	_ds18b20_config[1] = tl;
	_ds18b20_config[2] = _ds18b20_scratchpad[4];	
	
		
	if (!ds18b20_write_scratchpad()) return 0;
	ds18b20_reset_rom();
	return 1;
}



int ds18b20_read_rom(uint8_t *romcode_array)
{		
		//  The array has to be zeroed.
	for (int i = 0; i < 8; i++) romcode_array[i] = 0x00;
		//  Read the ROM code.
	return owi_read_rom(romcode_array);	
}



void ds18b20_set_rom(uint8_t *romcode_array)
{
	_ds18b20_address[0] = 0x01;
	for (int i = 0; i < 8; i++) _ds18b20_address[i + 1] = romcode_array[i];	
}



int ds18b20_check_alarm()
{		
	return owi_alarm_search();
}