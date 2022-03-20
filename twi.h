/*
 * twi.h
 *
 * Version: 1.0.0
 * Created: 10.12.2020
 *  Author: Frank Bjørnø
 *
 * Purpose: To facilitate communication between a single microcontroller and simple
 *          I2C devices on the TWI bus.
 *
 * Limitations:
 *
 *          This library relies on the built in TWI module to generate I2C compatible
 *          output, and is therefore not compatible with microcontrollers that don't
 *          have a dedicated TWI module.
 *
 *          The use of the term "Two Wire Interface" (TWI) in this code indicates an
 *          incomplete implementation of the I2C specification. This code does not
 *          support arbitration or clock stretching, but can still be used for a single
 *          master communicating with simple devices that never stretch the clock.
 *
 * Dependencies:
 *
 *          avr/io.h: This code was developed in Atmel Studio for Atmel microcontrollers
 *                    and relies on definitions of ports and TWI registers defined in
 *                    avr/io.h. It is therefore not necessarily portable between different
 *                    development environments.
 *
 * License:
 *
 *          This software is covered by a modified MIT License, see paragraph 4
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
 *
 *          4. Parts of this software were adapted from example code in the Microchip Atmega328P
 *          data sheet. Additional license restrictions from Microchip may apply.
 */


#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	
/*
 *     Initialize the TWI module
 */
void twi_enable(void);

/*
 *     Disable the TWI module
 */
void twi_disable(void);

/*
 *     Check if the TWI module is enabled.
 *
 *     \return     1 if enabled, 0 if disabled.
 */
int twi_is_enabled(void);

/*
 *     Open a connection to a device on a given address
 *     for reading or writing
 *
 *     \param address    7 bit i2c address
 *     \return           1 if successful, 0 if not.
 */
int twi_open(unsigned char address);

/*
 *     Generate a stop condition
 */
void twi_close(void);

/*
 *     Write one byte of data on an open connection
 *
 *     \param data    One byte of data to be transmitted.    
 *     \return        1 if successful, 0 if not.
 */
int twi_write_ch(unsigned char data);

/*
 *     Write a string of data. n is the length of the string.
 *
 *     \param *data   Pointer to string of bytes to be transmitted.
 *     \param  n      Number of bytes to transmit.
 *     \return        1 if successful, 0 if not.
 */
int  i2c_write_str(const unsigned char *data, int n);

/*
 *     read one byte from a given register from an open connection
 *     
 *     \param  reg    The slave's register to read from.   
 *     \param *data   Pointer to a string of bytes to receive the data.
 *     \return        1 if successful, 0 if not.
 */
int twi_read_ch(unsigned char reg, unsigned char *data);

/*
 *     Read a string of data
 *
 *     \param  reg    The slave's register to read from.
 *     \param *data   Pointer to a string of bytes to receive the data.
 *     \param  n      Number of bytes to transmit.
 *     \return        1 if successful, 0 if not.
 */
int twi_read_str(unsigned char reg, unsigned char *data, int n);

#ifdef __cplusplus
}
#endif
