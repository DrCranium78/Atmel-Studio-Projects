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
 *	       The functionality is deliberately kept to a minimum and no error checking of any 
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


#pragma once

#define SHIFT_DISPLAY_LEFT  0x1C			//  text moves right
#define SHIFT_DISPLAY_RIGHT 0x18			//  text moves left

#include <stdint.h>

enum lcd_mode : bool    {OFF = false, ON = true};
enum lcd_line : uint8_t {FIRST = 0, SECOND = 1};
	


class LCD
{
	public:
		/*
		 *     0x27 is the default address. Unless you physically modified the
		 *     i2c serial interface with a soldering iron, this is the address.
		 */
		LCD(unsigned char address = 0x27);
		
		/*
		 *     Call this before using the display. It will initialize the display
		 *     to the following settings:
		 *          backlight off
		 *          clear display, display on
		 *          caret home, hidden, direction left to right
		 */
		void init();
		
		/*
		 *     The following functions define the most commonly used functionality, 
		 *     at least by the author. For more exotic functionality, consider the 
		 *     command function with the macros defined at the top of this file.
		 */       
		void clear() const;									//  clears the display or screen
		void home() const;									//  sets the caret in the first position of the first line
		void line(lcd_line line) const;						//  sets the caret in the first position of the given line (FIRST or SECOND)
		void pos(lcd_line line, uint8_t col) const;			//  sets the caret in the position indicated by col (0 - 15) of the given line
		
		void backlight(lcd_mode mode = ON);					//  turns the backlight ON or OFF (ON is default)
		void display(lcd_mode mode = ON) const;				//  turns the display ON or OFF (ON is default)
		
		/*
		 *     This function prints a string of characters at the current caret position.
		 *     If any formatting is needed, this should be done before calling print, using 
		 *     sprintf from stdio.h
		 *     For example, to print a right justified string on the bottom line, do something like this:
		 *     char buffer[17];								//  declare a buffer somewhere in your code
		 *     sprintf(buffer, "%-16s", "Hello!");			//  use sprintf to format the text and copy it into the buffer
		 *     lcd.line(SECOND);							//  caret to first position, bottom line
		 *     lcd.print(buffer);							//  call the display function to print the string
		 */
		void print(const char *string) const;
		
		/*
		 *     The following function accepts custom instructions and commands defined at the top of
		 *     this file. 
		 *     For example, to shift the entire display left, you would, according to the datasheet,
		 *     use the binary instruction code 0001 1100 = 0x1C, defined as SHIFT_DISPLAY_LEFT in this file
		 *     lcd.command(SHIFT_DISPLAY_LEFT);
		 *     For further examples see the comment accompanying this function in the file lcd.cpp.
		 */
		void command(unsigned char command);
	private:
		void latch_data(const unsigned char data) const;	
		void transmit(const unsigned char data) const;				//  transmits display commands
		void transmit(const char *data) const;						//  transmits data
				
		unsigned char _lcd_backlight;
		unsigned char _twi_address;
};