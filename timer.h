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

#pragma once

class Timer
{
	public:
		Timer();
		void start();				//  resets milliseconds count
		unsigned int stop();			//  returns number of milliseconds since start (roughly)
		
		void operator++();			//  prefix increase number of milliseconds. generally called from timer interrupt
		void operator++(int);			//  postfix
	private:	
		unsigned int _count;
};
