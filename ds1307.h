/*
 * ds1307.h
 *
 * Version: 1.0.0
 * Created: 10.01.2022
 *  Author: Frank Bjørnø
 *
 * Purpose:	To communicate with a DS1307 Real Time Clock via a TWI interface. 
 *
 * Dependencies:
 *          util/delay.h: After transmitting data to the DS1307, the program will wait for 
 *                        it to process the data.
 *          stdint.h      This library uses data types defined in this header. stdint.h is
 *                        part of the C standard library.
 *          twi.h:        This library is designed to work with a TWI interface.
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
#pragma once

#include <stdint.h>

enum SQWM   : uint8_t {SQWOFF = 0x00, SQW1HZ = 0x10, SQW4K = 0x11, SQW8K = 0x12, SQW32K = 0x13};
enum DOW    : uint8_t {MON = 1, TUE = 2, WED = 3, THU = 4, FRI = 5, SAT = 6, SUN = 7};
enum DSMODE : uint8_t {DSMODE24 = 0x00, DSMODE12 = 0x01};
enum DSAMPM	: uint8_t {AM = 0x00, PM = 0x01};
	
class DS1307
{
	public:
		/*
		 *     The default mode is 24 hour clock
		 */
		DS1307();
		
		/*
		 *     init() clears the clock halt bit and disables square wave.
		 */
		void init();
		void set_mode(DSMODE m = DSMODE24);
		
		/*
		 *     Functions to set member fields
		 */
		bool set_12hms(uint8_t h, uint8_t m, uint8_t s, DSAMPM mi);				//  use this to set 12 hour mode
		bool set_24hms(uint8_t h, uint8_t m, uint8_t s);						//  use this to set 24 hour mode
		bool set_ymd(uint8_t y, uint8_t m, uint8_t d);
		void set_dow(DOW dow);												//  dow = day of week
		
		/*
		 *     Functions to get member fields
		 */
		 
		void get_12hms(uint8_t &h, uint8_t &m, uint8_t &s, DSAMPM &mi) const;
		void get_24hms(uint8_t &h, uint8_t &m, uint8_t &s) const;
		void get_ymd(uint8_t &y, uint8_t &m, uint8_t &d) const;
		void get_dow(uint8_t &dow) const;		
	
		/*
		 *     transfer_data() transfers member fields to the ds1307. update()
		 *     reads date and time from the DS1307 and updates member fields.
		 */
		void transfer_data() const;
		void update();
	
		/*
		 *     Functions to alter the output of the square wave generator, halt and
		 *     start the clock. 
		 */
		void sqw(SQWM mode) const;
		void halt();
		void start();
		
	private:
		/*
		 *     Functions that read or write a single byte to DS1307.
		 *     This shortens and simplifies code in other functions.
		 */
		void read_register(unsigned char reg, unsigned char *data) const;
		void write_register(unsigned char reg, unsigned char data) const;
		
		/*
		 *     Member fields
		 */
		uint8_t (*read_hr)(char);				//  pointer to a function
		uint8_t _yr, _mth, _day, _dow;			//  year, month, day and day of week
		uint8_t _hr, _min, _sec;				//  hour, minutes, seconds
		
		uint8_t _clkh;							//  clock halt
		
		DSAMPM _mi;								//  meridien indicator (AM/PM)
		DSMODE _mode;							//  12 hour or 24 hour mode (24 is default)
};