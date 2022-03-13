/*
 * helloworld.cpp
 *
 * Created: 31.01.2022
 *  Author: Frank Bjørnø
 *
 * Purpose:	The simplest possible example of using the lcd library. Useful for verifying that the code works.  
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

#include "lcd.h"

int main(void)
{
	LCD display;						//  declare
	display.init();					//  initialize display. this also enables twi
	display.backlight(ON);				//  turn backlight on
	
	display.print("Hello world.");
}

