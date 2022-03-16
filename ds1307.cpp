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



#include "twi.h"
#include "ds1307.h"


/*
 *     F_CPU has to be defined in order to use the delay function in util/delay.h
 *     if you use a 16Mhz crystal as a clock for the mcu, then leave this unchanged, 
 *     if not alter it accordingly
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif
#include <util/delay.h>

#define DS1307_I2C_ADDRESS 0x68

 
/*
 *     Check whether a year is a leap year
 *
 *     \param year     The year to check.
 *     \return         True if leap year, false if not.
 */
static bool is_leap(unsigned int year)
{
	/*
	 *     A year is a leap year if it is evenly divisible by 4 but not by 100, 
	 *     except if it is evenly divisible by 400.
	 *     The first leap year after the adoption of the above rule was 1752, 
	 *     but this function does not take this into consideration.
	 *     The leap year logic is taken from Kernighan and Ritchie, 
	 *     'The C Programming Language 2nd ed.', 1988, p. 41.
	 */
	return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

/*
 *     This function is used to verify that a given date is valid.
 *     f.ex. 03.02.2021 is valid, but 31.04.2021 is not.
 *
 *     \param y     Year  (0 - 99)
 *     \param m     Month (1 - 12)
 *     \param d     Day   (1 - 31)
 *     \return      True if date is valid, false if not.
 */
static bool is_valid_date(uint8_t y, uint8_t m, uint8_t d)
{
	if (d > 31 || d == 0) return false;						//  Day can not be 0 or larger than 31 in any case.
	else if (m == 4 || m == 6 || m == 9 || m == 11)					//  If month is apr, jun, sep or nov ...
	{
		if (d == 31) return false;						//  ... and day is 31, the date is invalid.
	}
	else if (m == 2)								//  If month is feb ...
	{
		if (d >= 30) return false;						//  ... and day is 30 or 31 ...
		else if(d == 29 && !is_leap(y)) return false;				//  ... or 29 in a non-leap year, the date is invalid.
	}
	return true;
}

/*
 *     This function is used to verify that a given time is valid.
 *     f.ex. 17:02:08 is valid, but 43:02:08 is not.
 *
 *     \param h     Year  (0 - 99)
 *     \param m     Month (1 - 12)
 *     \param s     Day   (1 - 31)
 *     \mode  m     Meridiem Indicator, either AM or PM.
 *     \return      True if time is valid, false if not. 
 */
static bool is_valid_time(uint8_t h, uint8_t m, uint8_t s, DSMODE mode)
{	
	if (mode) return !(h > 11 || h < 1 || m > 59 || s > 59);			//  12 hour mode
	else return !(h > 23 || m > 59 || s > 59);					//  24 hour mode	
}

/*
 *     The contents of the time and calendar registers in the ds1307 are
 *     in the binary coded decimal format. We therefore need the following
 *     two converter functions.
 *
 *     In the bcd numbering system, the given decimal number is segregated into
 *     chunks of four bits for each decimal digit within the number. Each decimal 
 *     digit is converted into its direct binary form, usually represented in
 *     four bits.
 *     Example: decimal 45 will be converted to decimal 69. the binary representation
 *     of 69 is 0100 0101 (which by no coincidence is hexadecimal 45).
 *     0100 = 4 and 0101 = 5.
 *     For our purposes, we only have to convert numbers no larger than 99, so the
 *     following simple functions will suffice:
 */

/*
 *     Convert from decimal to binary coded decimal.
 *
 *     \param dec    Decimal number to be converted to BCD.
 *     \return       Binary coded decimal.
 */
static uint8_t dec2bcd(uint8_t dec)
{
	return ((dec / 10) << 4) | (dec % 10);
}

/*
 *     Convert from binary coded decimal to decimal.
 *
 *     \param bcd    Binary coded decimal to be converted.
 *     \return       Decimal number.
 */
static uint8_t bcd2dec(uint8_t bcd)
{
	return (bcd & 0x0f) + ((bcd >> 4) * 10);
}

/*
 *     The DS1307 class has a pointer to a function as a member. That pointer will
 *     point to one of the two following functions depending on 12/24 hour mode.
 */

/*
 *     Process the bits from DS1307 register 0x00 in 12 hour mode
 *
 *     \param data    Raw data read from DS1307 register 0x00.
 *     \return        Processed data with only the hour part.
 */
static uint8_t read_12_hr(char data)
{
	return (data & 0x1F);							//  mask bits 7, 6 and 5
}

/*
 *     Process the bits from DS1307 register 0x00 in 24 hour mode
 *
 *     \param data    Raw data read from DS1307 register 0x00.
 *     \return        Processed data with only the hour part.
 */
static uint8_t read_24_hr(char data)
{
	return (data & 0x3F);							//  mask bits 7 and 6
}





/*
 *     Constructor
 *
 *     Note:     The date and time fields are initialized to valid time and date 
 *               but not transfered to the DS1307 data registers.       
 */ 
DS1307::DS1307()
{
	     //  initialize to Wed. 1. Jan. 2022, 00:00:00, 24 hour mode
	_yr  = 22;
	_mth = 1;
	_day = 1;
	_dow = SAT;
	_hr  = 0;
	_min = 0;
	_sec = 0;
	
	_clkh = 0x00;
	_mode = DSMODE24;
	_mi = AM;
	     
	     //  set function pointer to read_24_hr (24 hour mode)
	read_hr = &read_24_hr;
}

/*
 *     Call before using the DS1307
 *
 *     Note:     If TWI is already enabled, this can be skipped and replaced with
 *               manual calls to start to clear the clock halt bit and optionally 
 *               a call to sqw to turn it off or on. 
 */
void DS1307::init()
{
		//  Initialize TWI
	if (!twi_is_enabled()) twi_enable();
	
		//  clear the clock halt bit (BIT7) in DS1307 register 0x00. This starts the clock.	
	start();
	
		//  disable square wave output	
	sqw(SQWOFF);
}


/*
 *     Set 12 or 24 hour mode
 *
 *     \param m     Either DSMODE24 or DSMODE12
 */
void DS1307::set_mode(DSMODE m)
{
	unsigned char temp_data;
	read_register(0x02, &temp_data);					//  register 0x02 is the hour register
	
	if (m == DSMODE24)
	{
		temp_data &= 0xBF;						//  clear BIT6 (the 12/24 hour mode bit. 0 means 24 hour mode)
		read_hr = &read_24_hr;						//  redirect pointer to function
		_mode = DSMODE24;
	}
	else if (m == DSMODE12)
	{
		temp_data |= 0x40;						//  set BIT6. 1 means 12 hour mode.
		read_hr = &read_12_hr;
		_mode = DSMODE12;
	}
	write_register(0x02, temp_data);
}

/*
 *     Call this to set time in 12 hour mode.
 *
 *     \param h     Hour    (1 - 12)
 *     \param m     Minute  (0 - 59)
 *     \param s     Seconds (0 - 59)
 *     \param mi    Meridiem Indicator, either AM or PM
 *     \return      Returns false if invalid time.
 *
 *     Note:        This function only updates member variables. No data is
 *                  transferred to the DS1307.
 */
bool DS1307::set_12hms(uint8_t h, uint8_t m, uint8_t s, DSAMPM mi)
{
	if (!is_valid_time(h, m, s, DSMODE12)) return false;
	
	_hr = h;
	_min = m;
	_sec = s;
	_mi = mi;
	
	return true;
}

/*
 *     Call this to set time in 24 hour mode.
 *
 *     \param h     Hour    (0 - 23)
 *     \param m     Minute  (0 - 59)
 *     \param s     Seconds (0 - 59) 
 *     \return      Returns false if invalid time.
 *
 *     Note:        This function only updates member variables. No data is
 *                  transferred to the DS1307.
 */
bool DS1307::set_24hms(uint8_t h, uint8_t m, uint8_t s)
{
	if (!is_valid_time(h, m, s, DSMODE24)) return false;
	
	_hr = h;
	_min = m;
	_sec = s;	
	
	return true;
}

/*
 *     Call this to set the date.
 *
 *     \param y     Year  (0 - 99)
 *     \param m     Month (1 - 12)
 *     \param d     Day   (1 - 31)
 *     'return      Returns false if date is invalid.
 *
 *     Note:        This function only updates member variables. No data is
 *                  transferred to the DS1307.   
 */
bool DS1307::set_ymd(uint8_t y, uint8_t m, uint8_t d)
{
		//  validate date. if invalid return early.
	if (!is_valid_date(y, m, d)) return false;
		
	_yr  = y;
	_mth = m;
	_day = d;
		
	return true;
}

/*
 *     Call this to set the Day of Week
 *
 *     \param dow     Day of Week (MON - SUN)
 *
 *     Note:     Days of the week are defined in ds1307.h in the 
 *               enum DOW.
 *
 *     Note:     This function only updates member variables. No data is
 *               transferred to the DS1307.
 */
void DS1307::set_dow(DOW dow)
{
	_dow = dow;
}

/*
 *     Call this to get the time in 12 hour mode
 *
 *     \param &h    A reference to the variable receiving hours.
 *     \param &m    A reference to the variable receiving minutes.
 *     \param &s    A reference to the variable receiving seconds.
 *     \param &mi   A reference to the variable receiving AM/PM.
 *
 *     Note:        This function only grabs data from member variables, and will 
 *                  therefore usually be preceeded by a call to update().
 *
 *     Note:        Because the meridien indicator is either 0 or 1, a time string can be 
 *                  made like this: 
 *                  sprintf(buffer, "%02i:%02i:%02i %2s", h, m, s, (MI ? "PM" : "AM"));
 */  
void DS1307::get_12hms(uint8_t &h, uint8_t &m, uint8_t &s, DSAMPM &mi) const
{
	h = _hr;
	m = _min;
	s = _sec;
	mi = _mi;
}

/*
 *     Call this to get the time in 24 hour mode
 *
 *     \param &h    A reference to the variable receiving hours.
 *     \param &m    A reference to the variable receiving minutes.
 *     \param &s    A reference to the variable receiving seconds. 
 *
 *     Note:        This function only grabs data from member variables, and will 
 *                  therefore usually be preceeded by a call to update().
 */
void DS1307::get_24hms(uint8_t &h, uint8_t &m, uint8_t &s) const
{
	h = _hr;
	m = _min;
	s = _sec;
}

/*
 *     Call this to get the date
 *
 *     \param &h    A reference to the variable receiving years.
 *     \param &m    A reference to the variable receiving month.
 *     \param &s    A reference to the variable receiving day. 
 *
 *     Note:        This function only grabs data from member variables, and will 
 *                  therefore usually be preceeded by a call to update().
 */
void DS1307::get_ymd(uint8_t &y, uint8_t &m, uint8_t &d) const
{
	y = _yr;
	m = _mth;
	d = _day;
}

/*
 *     Call this to get the date
 *
 *     \param &dow  A reference to the variable receiving years. 
 *
 *     Note:        This function only grabs data from member variables, and will 
 *                  therefore usually be preceeded by a call to update().
 */
void DS1307::get_dow(uint8_t &dow) const
{
	dow = _dow;
}

/*
 *     Transfer the data from member variables to the DS1307 registers. This
 *     will usually be preceded by calls to set set the members, such as 
 *     set_24hms, set_ymd and set_dow.
 */
void DS1307::transfer_data() const
{
		//  don't overwrite clock halt bit
	uint8_t sec = dec2bcd(_sec);
	sec |= (_clkh << 7);								//  will set BIT7 if _clck == 0x01;
	
		//  the hour data have to be processed before transferred
	uint8_t hr = dec2bcd(_hr);
	
		//  set BIT6 if 12hour mode and bit5 if PM. _mi is always AM(0x00) if in 24 hour mode
	hr |= (_mode << 6) | (_mi << 5);		
	
		//  transfer data
	twi_open(DS1307_I2C_ADDRESS);							//  Start transmission.
	twi_write_ch(0x00);								//  Select register 0x00.
	twi_write_ch(sec);								//  Register 0x00 stores clock halt bit and seconds.
	twi_write_ch(dec2bcd(_min));							//  The register pointer automatically increments...
	twi_write_ch(hr);								//  ... after each data byte is written.
	twi_write_ch(dec2bcd(_dow));
	twi_write_ch(dec2bcd(_day));
	twi_write_ch(dec2bcd(_mth));
	twi_write_ch(dec2bcd(_yr));
	twi_close();									//  Terminate data transfer by generating a stop condition.
	_delay_us(100);									//  Wait for chip.
}

/*
 *     Update the member variables by connecting to the DS1307 and load the data
 *     from the DS1307 registers.
 */
void DS1307::update()
{
	unsigned char data[7];								//  Prepare to read 7 bytes of data from DS1307.
	
	twi_open(DS1307_I2C_ADDRESS);							//  Open connection and start transmission.
	twi_read_str(0x00, data, 7);							//  Read 7 bytes starting with register 0x00 into data array.
	twi_close();									//  Close connection.
	
		//  if 12hour mode, check bit 5 of hour. If set, PM, if clear AM
	if (_mode == DSMODE12)
	{
		_mi = (DSAMPM)((data[2] & 0x20) >> 5);					//  Isolate bit 5 and shift right 5 positions.
	}	
	
		//  process data
	_sec = bcd2dec(data[0] & 0x7F);							//  Mask clock halt bit (bit 7).
	_min = bcd2dec(data[1]);
	_dow = bcd2dec(data[3]);
	_day = bcd2dec(data[4]);
	_mth = bcd2dec(data[5]);
	_yr  = bcd2dec(data[6]);
	
	_hr = bcd2dec(read_hr(data[2]));						//  Use pointed function to read/process data.
}

/*
 *     Set the square wave output on PIN7 of the DS1307.
 *
 *     \param mode     Values are defined in ds1307.h in the enum SQWM
 */
void DS1307::sqw(SQWM mode) const
{
	write_register(0x07, mode);	
}

/*
 *     Stop the clock by setting the clock halt bit (BIT7) in register 0x00.
 */
void DS1307::halt()
{
	_clkh = 0x01;
	unsigned char temp_data;
	
	read_register(0x00, &temp_data);	
	write_register(0x00, temp_data | 0x80);
}

/*
 *     Start the clock by clearing the clock halt bit in register 0x00.
 */
void DS1307::start()
{
	_clkh = 0x00;
	unsigned char temp_data;
	
	read_register(0x00, &temp_data);	
	write_register(0x00, temp_data & 0x7F);
}

/*
 *     Private function to read a single byte from a DS1307 register.
 *
 *     \param  reg     The register to read.
 *     \param *data    Pointer to an unsigned char that will receive the data.
 */
void DS1307::read_register(unsigned char reg, unsigned char *data) const
{
	twi_open(DS1307_I2C_ADDRESS);
	twi_read_ch(reg, data);
	twi_close();	
	_delay_ms(50);
}

/*
 *     Private function to write a single byte to a DS1307 register.
 *
 *     \param reg     The register to write to.
 *     \param data    The databyte to write.
 */
void DS1307::write_register(unsigned char reg, unsigned char data) const
{
	twi_open(DS1307_I2C_ADDRESS);
	twi_write_ch(reg);
	twi_write_ch(data);
	twi_close();
	_delay_ms(50);
}
