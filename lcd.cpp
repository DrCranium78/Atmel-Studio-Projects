/*
 * lcd.h
 *
 * Version: 1.0.0
 * Created: 01.10.2021
 *  Author: Frank Bjørnø
 *
 * Purpose:	To transmit instructions to a 1602 LCD via a TWI interface.
 *
 * Limitations:
 *
 *	    The functionality is deliberately kept to a minimum and no error checking of any
 *          kind will be performed. This is by design.
 *
 *          This library is designed to work with an I2C interface that is usually soldered
 *          to the display. It is therefore necessary to include the files twi.h and twi.c
 *          that should be distributed together with lcd.h and lcd.cpp.
 *
 * Dependencies:
 *
 *          util/delay.h: After transmitting an instruction to the display, the program will
 *                        wait for 39us or 1.53ms while the instruction executes. Failure to
 *                        this will almost certainly cause errors. This library is provided
 *                        by AtmelStudio.
 *          stdint.h      This library uses data types defined in this header. stdint.h is
 *                        part of the C standard library.
 *          twi.h:        This library is designed to work with an TWI interface.
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
 *     if not alter it accordingly
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif

#include <util/delay.h>
#include "twi.h"
#include "lcd.h"



#define LCD_COMMAND_MODE  0x00
#define LCD_DATA_MODE     0x01
#define LCD_ENABLE        0x04
#define LCD_DISABLE       0xFB
#define LCD_BACKLIGHT     0x08

/*
 *     definitions of DDRAM addresses
 */

#define LCD_LINE_SIZE         0x40     //  DDRAM address of first char of line 2

/*
 *     definitions of commands
 */
#define CMD_SET_DDRAM_ADDRESS 0x80
#define CMD_DISPLAY_ON        0x0C
#define CMD_DISPLAY_OFF       0x08
#define CMD_CARET_HOME        0x02
#define CMD_DISPLAY_CLEAR     0x01



/*
 *     Constructor
 *
 *     \param address    i2c address
 */
LCD::LCD(unsigned char address) : _lcd_backlight{0x00}, _twi_address{address} {}
				
/*
 *     Call this before using the display
 */				
void LCD::init()
{
	if (!twi_is_enabled()) twi_enable();
	
		/*  
		 *    In case this is the first thing that is called in the main program.
		 *    16ms is the power-on initialization time for LCD16x2.
		 */
	_delay_ms(16);							//  wait for more than 15ms after Vcc rises to 4.5V
	
		//  Start transmission on the TWI bus
	twi_open(_twi_address);

		/*
		 *  Initialize the display to 8-bit mode: 0b0011 0000 = 0x30 (Function Set: 8-bit interface)
		 *  the initialization sequence is as follows:
		 *  send 0x30, wait for more than 4.1ms
		 *  send 0x30, wait for more than 100us
		 *  send 0x30, wait for more than 100us		
		 */		
	latch_data(0x30);						//  latch_data sends the data and toggles the enable pin
	_delay_us(4100);						//  wait for more than 4.1ms
	
	latch_data(0x30);
	_delay_us(100);
	
	latch_data(0x30);
	_delay_us(100);
	
		/*
		 *  Configure 4-bit mode. 0b0010 0000 = 0x20: Function Set: 4-bit interface
		 *  This command, sent while the display is in 8 bit mode, changes the display to 4 bit mode	
		 */	
	latch_data(0x20);
	_delay_us(100);
	
		//  Stop transmission. From here, the display only accepts 4 bit input
	twi_close();	
	
		//  Reconfigure 4-bit mode. Interface is now 4 bits.
	command(0x28);							// Function Set: 4-bit interface, dual line, 5x8 font
		
	clear();							//  clear screen
	display(ON);							//  display on
}

		
/*
 *     Write 0x00 to DDRAM and set DDRAM address to 0x00 from AC (Address Counter).
 */		
void LCD::clear() const
{
	transmit(CMD_DISPLAY_CLEAR);
	_delay_us(1530);
}



/*
 *     Set DDRAM address to 0x00 from AC and return cursor to its original position 
 *     if shifted. The contents of DDRAM are not changed.
 */
void LCD::home() const
{
	transmit(CMD_CARET_HOME);
	_delay_us(1530);
}



/*
 *     Select display line
 *     
 *     \param line     FIRST or SECOND
 */	
void LCD::line(lcd_line line) const
{
	transmit(CMD_SET_DDRAM_ADDRESS | line * LCD_LINE_SIZE);
	_delay_us(39);
}

/*
 *     Set caret position
 *
 *     \param line     FIRST or SECOND
 *     \param col      0 - 15
 *
 *     Note:      If col is larger than 15, it will be set to zero
 */
void LCD::pos(lcd_line line, uint8_t col) const
{
	if (col > 15) col = 0;
	transmit(CMD_SET_DDRAM_ADDRESS | (line * LCD_LINE_SIZE + col));
	_delay_us(39);
}


/*
 *     Set backlight
 *
 *     \param mode    ON or OFF
 */		
void LCD::backlight(lcd_mode mode)
{
	_lcd_backlight = mode * LCD_BACKLIGHT;
	
	twi_open(_twi_address);
	twi_write_ch(_lcd_backlight);
	twi_close();
	_delay_us(39);
}



/*
 *     Turn display on or off
 *
 *     \param mode     ON or OFF
 */ 
void LCD::display(lcd_mode mode) const
{
	transmit(mode * CMD_DISPLAY_ON | !mode * CMD_DISPLAY_OFF);	//  branchless. works because mode is either true or false.
	_delay_ms(39);
}



/*
 *     Print a string of characters
 *  
 *     \param *string     A pointer to C style string of characters
 *
 *     Note:      It is the programmers responsibility to format the
 *                string before printing it. The print function does
 *                not handle line breaks.
 *
 *     Example:   Print a simple string
 *                display.print("  Hello World!  ");
 *
 *     Example:   Print a formatted string
 *                char buffer[17];					//  16 characters + string terminator
 *                sprintf(buffer, "%-16s", "Hello!");			//  format string using sprintf
 *                display.print(buffer);
 */
void LCD::print(const char *string) const
{
	transmit(string);
}

		

/*
 *     Use this function to transmit custom commands to the display.
 *
 *     \param command     A byte sized command. Consult the instruction table 
 *                        in the 1602 datasheet for for the bit pattern. 
 *
 *     Example:     In order to make the cursor visible and blinking, check the 
 *                  instruction set and notice that the cursor visibility and 
 *                  blinking is controlled by the first and second bit. For 
 *                  blinking cursor, the first bit has to be set and for a visible 
 *                  cursor, the second bit must be set. The third bit controls the 
 *                  display. Set it for display on and clear it for display off. 
 *                  Finally the fourth bit must be set to specify the Display on/off
 *                  instruction. The bit pattern is therefore 0000 1111 = 0x0F.
 *                  Call the function like this: command(0x0F);
 */
void LCD::command(unsigned char command)
{
	transmit(command);
	_delay_us(39);							//  all commands require 39us, except clear display and return home
}



/*
 *     Private function to write data to the display and latch
 *
 *     \param data     Data to write to the display.
 
 *     Note:  The LCD only grabs the the data at its register line when the
 *            enable pin goes from low to high. The enable pin is always held
 *            low while not transmitting data. When data is transmitted, it is 
 *            placed on the bus with the enable pin high, then the enable pin
 *            is set low. 
 */	
void LCD::latch_data(const unsigned char data) const
{
	twi_write_ch(data | LCD_ENABLE);				//  transmit data with ENABLE bit set high
	_delay_us(1);
	twi_write_ch(data & LCD_DISABLE);				//  transmit data with ENABLE bit set low
}



/* 
 *     Private function used to transmit one byte of data to the
 *     display, typically a command. The data is transmitted 4 bits
 *     at the time.
 *
 *    \param data     Data to write to the display
 */	
void LCD::transmit(const unsigned char data) const
{
	twi_open(_twi_address);
		
		//  prepare command part of data nybble	
	unsigned char cmd  = _lcd_backlight | LCD_COMMAND_MODE;
	
		//  prepare high data nybble	
	cmd = (data & 0xF0) | _lcd_backlight;	
	latch_data(cmd);	
	
		//  prepare low data nybble	
	cmd = (data << 4) | _lcd_backlight;	
	latch_data(cmd);	
	
		//  set all data pins high (inactive) and close TWI connection	
	twi_write_ch(cmd | 0xF0);
	twi_close();	
}



/*
 *     Private function used to transmit a string of bytes to the
 *     display, typically text to be displayed.
 *
 *     \param *data     Pointer to first character of zero terminated 
 *                      data string.
 */
void LCD::transmit(const char *data) const
{
	twi_open(_twi_address);
	
		//  prepare command nybble
	unsigned char ctrl = _lcd_backlight | LCD_DATA_MODE;
	unsigned char cmd  = 0x00;
	
	while (*data != 0)
	{
			//  prepare high data nybble
		cmd = (*data & 0xF0) | ctrl;				//  mask low data nybble and OR in control nybble			
		latch_data(cmd);
		
			//  prepare low data nybble			//  shift low data into position and OR in control nybble
		cmd = (*data++ << 4) | ctrl;			
		latch_data(cmd);			
	}
		//  set all data pins high (inactive) and close i2c connection	
	twi_write_ch(cmd | 0xF0);
	twi_close();
	_delay_us(37);
}
