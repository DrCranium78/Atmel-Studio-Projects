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


/*
 *     F_CPU has to be defined in order to set the bitrate.
 *     If you use a 16Mhz crystal as a clock for the MCU, then leave this unchanged, 
 *     if not alter it accordingly
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif

#ifndef F_TWI
#define F_TWI 100000ul						//  100kbps is standard mode
#endif

#include <avr/io.h>							//  contains definitions of f.ex. PORTC and DDRC
#include "twi.h"

/*
 *     The TWI bus connections on an Atmega328 are pins 4 and 5 on PORT C.
 *     To set bits 4 and 5 use the following mask: 0b0011 0000 = 0x30.
 *     If you are using a different microcontroller, you will most likely
 *     have to change the following three macros:
 */
#define TWI_PORT         PORTC
#define TWI_DDR          DDRC
#define TWI_PINMASK      0x30

/*
 *     The following macros are intended to make the code more readable.
 */

//               Two Wire Status Register TWSR:   | TWS7 | TWS6 | TWS5 | TWS4 | TWS3 |  -  | TWPS1 | TWPS2 |
#define TWI_PRESCALER_MASK     0xFC       //      |   1  |   1  |   1  |   1  |   1  |  1  |   0   |   0   |


//              Two Wire Control Register TWCR:   | TWINT | TWEA | TWSTA | TWSTO | TWWC | TWEN |  -  | TWIE |
#define TWI_ENABLE_MASK        0x45       //      |   0   |   1  |   0   |   0   |   0  |   1  |  0  |   1  |
#define TWI_START_CONDITION    0xA4       //      |   1   |   0  |   1   |   0   |   0  |   1  |  0  |   0  | 
#define TWI_STOP_CONDITION     0x94       //      |   1   |   0  |   0   |   1   |   0  |   1  |  0  |   0  |
#define TWI_INTERRUPT_FLAG     0x80       //      |   1   |   0  |   0   |   0   |   0  |   0  |  0  |   0  |
#define TWI_START_TRANSMISSION 0x84       //      |   1   |   0  |   0   |   0   |   0  |   1  |  0  |   0  |
#define TWI_RETURN_ACK         0xC4       //      |   1   |   1  |   0   |   0   |   0  |   1  |  0  |   0  |
#define TWI_RETURN_NACK        0x84       //      |   1   |   0  |   0   |   0   |   0  |   1  |  0  |   0  |

/*
 *     Status codes for master transmitter (MT) and master receiver (MR) mode. 
 */
#define TWI_START              0x08
#define TWI_REP_START          0x10					
#define TWI_MT_SLA_ACK         0x18
#define TWI_MT_DATA_ACK        0x28
#define TWI_MR_SLA_ACK         0x40
#define TWI_MR_SLA_NACK        0x48
#define TWI_MR_DATA_ACK        0x50
#define TWI_MR_DATA_NACK       0x58



static unsigned char _twi_enabled = 0;					//  keep track of i2c initialization
static unsigned char _twi_address = 0x00;				//  keep track of address for repeat start condition


/******************************************************************************************************
 *                                        ENABLE / DISABLE                                            *
 ******************************************************************************************************/

void twi_enable(void)
{	
	if (_twi_enabled) return;							//  is i2c already initialized/enabled?
	_twi_enabled = 1;									//  only initialize once
	
		/*
		 *     Activate internal pull-ups for TWI. The internal pull-up is enabled when 
		 *     the port bit is set and the data direction is cleared. These macros are 
		 *     defined at the top of the file.
		 */
	TWI_DDR  &= ~TWI_PINMASK;							//  clear the data direction
	TWI_PORT |=  TWI_PINMASK;							//  set the port bit
	
		/*
		 *     Set TWI bit rate.
		 */			
	TWBR = ((F_CPU / F_TWI) - 16) / 2;
	
		/*	 
		 *     Set enable acknowledge bit (TWEA), enable TWI module (TWEN), 
		 *     and enable TWI interrupt (TWIE).
		 */	
	TWCR |= TWI_ENABLE_MASK;
}



void twi_disable(void)
{
	if (!_twi_enabled) return;				//  is i2c already disabled?
	_twi_enabled = 0;
	
	TWCR     &= ~TWI_ENABLE_MASK;			//  Reset TWI interrupt flag, disable TWI module and TWI interrupt	
	TWI_PORT &= ~TWI_PINMASK;					//  deactivate internal pull-ups for TWI
}



int twi_is_enabled(void)
{
	return _twi_enabled;
}

/******************************************************************************************************
 *                                          OPEN / CLOSE                                              *
 ******************************************************************************************************/

int twi_open(unsigned char address)
{
	_twi_address = address;
	
		/*
		 *     1. Send start condition.
		 *     2. Wait for TWINT flag to clear. This indicates that the START condition has been transmitted.
		 *     3. check value of TWI status register. Mask prescaler bits. Status != START indicates error.
		 */
	TWCR = TWI_START_CONDITION;										//  1.	send start condition		
	while (!(TWCR & TWI_INTERRUPT_FLAG));							//  2.	wait for TWINT flag.
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_START) return 0;			//  3.  check value of status register while masking prescaler bits 
	
		/*
		 *     3. Transmit SLA + W.
		 *     4. Wait for TWINT flag to clear.
		 *     5. Verify that slave has acknowledged.
		 */	
	TWDR = (_twi_address << 1);										//  Load SLA + W into TWI data register
	TWCR = TWI_START_TRANSMISSION;									//  3.  Set TWI interrupt bit to start transmission of address	
	while (!(TWCR & TWI_INTERRUPT_FLAG));							//  4.	wait for TWINT flag to clear.
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_MT_SLA_ACK) return 0;	//  5.  verify MT_SLA_ACK is received
	
	return 1;														//  connection open
}



void twi_close(void)
{	
	TWCR = TWI_STOP_CONDITION;
}

/******************************************************************************************************
 *                                             WRITE                                                  *
 ******************************************************************************************************/

int twi_write_ch(unsigned char data)
{
		/*
		 *     1. Load data into TWDR. clear TWINT bit in TWCR to start transmission.
		 *     2. Wait for TWINT flag. this indicates that the data has been transmitted 
		 *        and ACK/NACK has been received.
		 *     3. Check value of TWI status register. mask prescaler bits. If status
		 *        is different from MT_DATA_ACK, this indicates an error.
		 */
	TWDR = data;
	TWCR = TWI_START_TRANSMISSION;										// 1.
	while(!(TWCR & TWI_INTERRUPT_FLAG));								// 2. 
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_MT_DATA_ACK) return 0;		// 3.
	
	return 1;
}



int twi_write_str(const char *data, int n)
{
	/*
	 *     Calls twi_write for each character.
	 */
	const char *ptr = data;
	for (int c = 0; c < n; c++)
	{
		if (!twi_write_ch(*ptr)) return 0;
	}
	return 1;
}

/******************************************************************************************************
 *                                             READ                                                   *
 ******************************************************************************************************/

/*
 *     The slave needs to know which register the receiver wants to read.
 *     This function is used by the twi_read functions for this purpose
 */
static int twi_register(unsigned char reg)
{
		/*
		 *     1. transmit register
		 *     2. wait for TWINT flag. This indicates that the data has been transmitted
		 *        and ACK/NACK has been received
		 *     3. check value of TWI status register. mask prescaler bits. if status
		 *        is different from TWI_MT_DATA_ACK, this indicates an error
		 */
		
	TWDR = reg;
	TWCR = TWI_START_TRANSMISSION;										//  1.
	while(!(TWCR & TWI_INTERRUPT_FLAG));								//  2.
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_MT_DATA_ACK) return 0;		//  3.
	
		/*  
		 *  Repeat start condition, wait for REP_START.
		 */  
	TWCR = TWI_START_CONDITION;
	while (!(TWCR & TWI_INTERRUPT_FLAG));
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_REP_START) return 0;
	
		/*
		 *  1. send SLA + R
		 *  2. wait for ACK/NACK
		 *  3. check value of TWI status register. 
		 */
	TWDR = (_twi_address << 1) | 0x01;
	TWCR = (TWI_START_TRANSMISSION);										//  1.
	while(!(TWCR & TWI_INTERRUPT_FLAG));									//  2.
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_MR_SLA_ACK) return 0;			//  3. MR_SLA_ACK = 0x40: SLA+R has been transmitted, ACK received
	
	return 1;
}



int twi_read_ch(unsigned char reg, unsigned char *data)
{
	twi_register(reg);
	
		/*
		 *     1. Receive data and return NACK.
		 *     2. Wait for TWINT flag.
		 *     3. Check status register and mask prescaler bits. If status
		 *        is different from MR_DATA_ACK, this indicates an error.
		 */
	TWCR = (TWI_RETURN_NACK);										//  1. 
	while(!(TWCR & TWI_INTERRUPT_FLAG));									//  2.
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_MR_DATA_NACK) return 0;			//  3.
	
	*data = TWDR;
	return 1;	
}



/*
 *     This function does the same thing as twi_read_ch, but uses
 *     a loop to read several characters in a row
 */
int twi_read_str(unsigned char reg, unsigned char *data, int n)
{
	twi_register(reg);
	unsigned char *ptr = data;	
	
	for (int c = 0; c < n - 1; c++)
	{
		TWCR = TWI_RETURN_ACK;											//  read data and return ACK
		while(!(TWCR & TWI_INTERRUPT_FLAG));
		if ((TWSR & TWI_PRESCALER_MASK) != TWI_MR_DATA_ACK) return 0;
		*ptr++ = TWDR;													//  fetch data from TWI Data Register
	}	
	TWCR = TWI_RETURN_NACK;												//  Return NACK to stop transmission
	while(!(TWCR & TWI_INTERRUPT_FLAG));								//  wait for interrupt flag to clear
	if ((TWSR & TWI_PRESCALER_MASK) != TWI_MR_DATA_NACK) return 0;
	*ptr = TWDR;														//  fetch data

	return 1;
}