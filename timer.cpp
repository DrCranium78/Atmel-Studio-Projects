/*
 * timer.h
 *
 * Version: 1.0.0
 * Created: 16.12.2020
 *  Author: Frank Bjørnø
 *
 * Purpose:	
 *
 *          To use the internal timer/counter of the AtMega328p to count milliseconds
 *          between short periods of time. The main application is to time the main loop
 *          of a program. 
 *
 * Limitations:
 *
 *          Timer uses an unsigned int (16 bits in Atmel Studio) to count milliseconds. 
 *          This limits the timer to about 65536ms, which should be ample for its main 
 *          application.
 *
 *          Timer only gives an approximate measure of milliseconds and should never 
 *          be used for anything where accuracy is an issue.
 *
 *          Timer assumes that the MCU is driven by a 16MHz crystal. Using a different 
 *          crystal requires changes to register TCCR0B and OCR0A, initialized in the
 *          constructor. The Datasheet should be consulted.
 *
 * Dependencies:
 *          avr/io.h:     This library uses definitions in this header.
 *                        it to process the data. 
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

#include <avr/io.h>
#include "timer.h"



/*
 *     set timer A to roughly count milliseconds
 */
Timer::Timer() : _count{0}
{
	/*
	 *  TCCR0A - Timer/Counter 0 Control Register A
	 *
	 *  bit        7        6        5        4      3     2     1       0
	 *  name     COM0A1   COM0A0   COM0B1   COM0B0   -     -   WGM01   WGM00
	 *  set to     0        0        0        0      0     0     1       0
	 *
	 *
	 *  WGM01 = 1: CTC (Clear Timer on Compare match) mode
	 *  WGM00 = 0: TCNT0 will count up to 255, then signal timer 0 compare interrupt
	 */
	TCCR0A = 0x02;
	
	/*
	 *  TCCR0B - Timer/Counter 0 Control Register B
	 *
	 *  bit        7       6        5        4      3      2      1     0
	 *  name     FOC0A   FOC0B      -        -    WGM02   CS02   CS01  CS00
	 *  set to     0       0        0        0      0      1      0     1
	 *
	 *  CSC02 = 1
	 *  CSC01 = 0		clock / 1024
	 *  CSC00 = 1
	 */
	TCCR0B = 0x05;
	
	/*
	 *     count to 16 before resetting the timer/counter
	 *     this makes _count a rough measure of milliseconds
	 *     16Mhz / 1024 = 16384 and 16384 / 16 = 1024
	 */
	OCR0A = 0x10;			// 16
	
	/*
	 *  TIMSK0 - Timer/Counter 0 Interrupt Mask Register
	 *
	 *  bit        7       6       5       4       3       2      1      0
	 *  name       -       -       -       -       -    OCIE0B OCIE0A  TOIE0
	 *  set to     0       0       0       0       0       0      1      0
	 *
	 *  OCIE0B = 0  disable Timer/Counter 0 Output Compare Match B Interrupt
	 *  OCIE0A = 0  disable Timer/Counter 0 Output Compare Match A Interrupt
	 *  TOIE0  = 1  enable Timer/Counter 0 Overflow Interrupt
	 */	
	TIMSK0 = 0x02;
}

/*
 *     Start the timer
 */
void Timer::start()
{
	
	_count = 0;
}

/*
 *     Stop the timer
 *
 *     \return     number of milliseconds since last start
 */
unsigned int Timer::stop()
{
	return _count;
}

/*
 *     Prefix increment count of milliseconds.
 *
 *     Note: There is, in general, no point in calling this from any 
 *           other point in the program than from a Timer ISR
 */
void Timer::operator++()
{
	_count++;
}

/*
 *     Postfix increment count of milliseconds.
 *
 *     Note: There is, in general, no point in calling this from any 
 *           other point in the program than from a Timer ISR
 */
void Timer::operator++(int)
{
	_count++;
}