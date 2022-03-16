/*
 * onewire.h
 *
 * Version: 1.0.0
 * Created: 11.12.2020
 *  Author: Frank Bjørnø
 *
 * Purpose: To facilitate communication with 1-wire devices using the 1-wire protocol.
 *
 * Limitations:
 *
 *          This is an incomplete implementation of the protocol and does not support the
 *          SEARCH ROM command.
 *
 *          The ALARM SEARCH command is only partially implemented, meaning that it targets
 *          DS18B20 devices. It can, in theory, be used with other alarm devices but this
 *          has not been tested.
 *
 * Dependencies:
 *
 *          util/delay.h: The delay function is used to control the amount of time the line is held high
 *                        or low in accordance with the Onewire protocol.
 *          avr/io.h:     This library relies on some port definitions in avr/io.h.
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
 *     F_CPU has to be defined in order to use the delay function in util/delay.h.
 *     If you use a 16Mhz crystal as a clock for the MCU, then leave this unchanged, 
 *     if not, alter it accordingly.
 */
#ifndef F_CPU
#define F_CPU 16000000ul
#endif

#include <util/delay.h>
#include <avr/io.h>
#include "onewire.h"



/*
 *     Define commands:
 *
 *     The default owi pin is PB0, that is pin 14 on an AtMega328. If you
 *     want to use a different port, change the following macros accordingly
 */
#define OWI_PORT		PORTB
#define OWI_DDR			DDRB
#define OWI_PIN         	PINB
#define OWI_PINMASK		0x01
#define OWI_IPINMASK		0xFE					//  inverted pin mask

/*
 *     When a system is initially powered up, the master must identify the ROM codes
 *     of all slave devices on the bus, which allows the master to determine the 
 *     number of slaves and their device types. The master learns the ROM codes through
 *     a process of elimination that requires the master to perform a search ROM cycle
 *     as many times as necessary to identify all of the slave devices.
 */
#define OWI_SEARCH_ROM		0xF0

/*
 *     This command can only be used when there is one slave on the bus. It allows the 
 *     bus master to read the slave's 64-bit ROM code without using the search ROM
 *     procedure. If this command is used when there is more than one slave present
 *     on the bus, a data collision will occur when all the slaves attempt to respond 
 *     at the same time.
 */
#define OWI_READ_ROM		0x33

/*
 *     The match ROM command followed by a 64-bit ROM code sequence allows the bus master
 *     to address a specific slave device on a multi-drop or single-drop bus. Only the slave
 *     that exactly matches the 64-bit ROM code sequence will respond to the function 
 *     command issued by the master. All other slaves will wait for a reset pulse.
 */
#define OWI_MATCH_ROM		0x55

/*
 *     The master can use this command to address all devices on the bus simultaneously
 *     without sending out any ROM code information. For example, the master can make
 *     all DS18B20s on the bus perform simultaneous temperature conversions by issuing
 *     a skip ROM command followed by a convert t command.
 *
 *     Note that the read scratchpad command can follow the skip ROM command only if there
 *     is a single slave device on the bus.
 */
#define OWI_SKIP_ROM		0xCC

/*
 *     The operation of this command is identical to the operation of the search ROM command
 *     except that only slaves with a set alarm flag will respond. This command allows the 
 *     master device to determine if any device experienced an alarm condition during the most
 *     recent temperature conversion. After every alarm search cycle, that is alarm search 
 *     command followed by data exchange, the bus master must return to step 1 (initialization)
 *     in the transaction sequence.
 */
#define OWI_ALARM_SEARCH	0xEC



/*
 *     Static functions are only visible to other functions in this file.
 */

/*
 *     This function is based on the crc generation outlined in the
 *     DS18B20 datasheet pp. 8-9. 
 *     The procedure for generating the CRC is basically to XOR each 
 *     bit of the data byte with the current least significant bit of 
 *     CRC. The result of this operation is then fed back into the
 *     CRC according to the polynomial function of the CRC. Note that 
 *     the CRC is shifted before the feedback, therefore we feed the 
 *     result back into the third, fourth and eight position and 
 *     not the fourth, fifth and eight.
 *
 *     \parameter databyte  The data from which we generate the CRC.
 *     \parameter crc       The initial value of CRC.
 *     \return              the CRC of data with crc as initial value.
 *
 *     Note:                Setting the parameter crc to 0 generates
 *                          the CRC of the parameter databyte.
 *                          Repeatedly passing the return value of 
 *                          this function will generate the CRC of a 
 *                          string of data.
 */
static uint8_t owi_crc(uint8_t databyte, uint8_t crc)
{
	uint8_t feedback;
	
	for (uint8_t c = 0; c < 8; c++)					//  For all 8 bits ...
	{
		feedback = (crc ^ databyte) & 0x01;			//  ... xor lsbit of data and accumulated crc.
		crc >>= 1;						//  Shift and ...
		crc ^= 0x8c * feedback;					//  ... feed result back into x4, x5 (shifted) and x8.
		databyte >>= 1;						//  Shift databyte into position for next xor.
	}
	return crc;
}



/*
 *     Set the line high by enabling the internal pull-up resistor.
 *     The internal pull-up is enabled when the port bit is set 
 *     and the data direction is cleared.
 */
static void release_line(void)
{
	OWI_DDR  &= OWI_IPINMASK;			//  clear the data direction 
	OWI_PORT |= OWI_PINMASK;			//  set the port bit
}



/*
 *     Pull the line low by disabling the internal pull-up resistors.
 */
static void pull_line(void)
{
	OWI_DDR  |= OWI_PINMASK;			//  set the data direction
	OWI_PORT &= OWI_IPINMASK;			//  clear the port bit		
}



/*
 *     Write a high bit.
 *     This is done by pulling the line down for 1 - 15us and then 
 *     releasing it for the rest of the 65us bit period.
 */
static void owi_write_1(void)
{	
	pull_line();
	_delay_us(1);
	release_line();
	_delay_us(64);					//  leave line hight for the rest of the 65us bit period
}



/*
 *     Write a low bit.
 *     This is done by pulling the line down for 60 - 120us and then
 *     releasing it.
 */
static void owi_write_0(void)
{	
	pull_line();
	_delay_us(60);
	release_line();
	_delay_us(5);					//  leave line for at least 5us
}



/*
 *     Read bit, returns 1 or 0.
 *     This is done by sending a high bit, that is pulling the line down 
 *     for 1 - 15us, then releasing it. After a few microseconds, the line 
 *     level is checked. If the line is high, 1 is read, and if the line 
 *     is low, a 0 is read. 
 */
static int owi_read_bit(void)
{
	int bit = 0;
	
	pull_line();			
	_delay_us(1);
	release_line();
	_delay_us(14);					//  the line should be sampled 15us after the line was pulled low
		
	if (OWI_PIN & OWI_PINMASK) bit = 1;
	
	_delay_us(45);
	return bit;
}



/*
 *     Send a reset signal and listen for presence signal. Returns true for 
 *     presence or false for no presence.
 */
int owi_detect_presence(void)
{
	int presence = 1;
		
		
		//  Start transaction by holding the line down for 
		//  480 - 560us. This is the RESET PULSE		 
	pull_line();							//  Pull line down for 480us.
	_delay_us(480);
	release_line();								
	_delay_us(60);							//  Wait for 60us to sample the line.
		
	if (OWI_PIN & OWI_PINMASK) presence = 0;			//  Is line level is high? If so, no presence detected.
	_delay_us(420);
	
	return presence;
}



/*
 *     Send one byte of data on the one wire bus, lsb first. Notice that in the 
 *     for loop, mask is an 8 bit integer, initiated as binary 0000 0001, and 
 *     is used to mask the data to check each individual bit. Mask is shifted 
 *     one bit to the left with each iteration of the loop so that the bits in 
 *     data are transmitted lsb first. The eight time mask is shifted, it becomes 
 *     0 and the loop condition fails, terminating the loop. 
 */
void owi_write_byte(unsigned char data)
{		
	for (uint8_t mask = 0x01; mask > 0x00; mask <<= 1)		//  For each bit in the byte ...
	{
		if (data & mask)					//  ... if bit is 1 ...
		{
			owi_write_1();					//  ... write a 1 on the bus.
		}
		else							//  ... If bit is 0 ... 
		{
			owi_write_0();					//  ... write a 0 on the bus.
		}			
	}	
}



/*
 *     Read one byte of data from the one wire bus, lsb first see comment for 
 *     owi_write_byte for explanation.
 */
unsigned char owi_read_byte(void)
{
	unsigned char data = 0x00;					//  Initialize destination buffer.
	
	for (uint8_t mask = 0x01; mask > 0x00; mask <<= 1)		//  Read 8 bits.
	{
		if (owi_read_bit())					//  Read one bit from the bus.
		{
			data |= mask;					//  Copy to buffer if bit is set.
		}		
	}
	
	return data;
}



/*
 *     The one wire device might be busy performing some task, the DS18B20, 
 *     for example, will be busy while converting temperature. The device can 
 *     be polled using this function. The function returns 1 if busy and 0 if 
 *     ready.
 * 
 *     Example: while(owi_is_busy()) _delay_ms(10);
 */
int owi_is_busy(void)
{
	return !(owi_read_bit());
}



/*
 *     NOT IMPLEMENTED:
 *     Send a SEARCH ROM command on the one wire bus
 */
void owi_search_rom(void) {}



/*
 *     Returns 1 if success, 0 if fail
 */
int owi_read_rom(uint8_t *rom_array)
{
		/*
		 *     Use a temporary array to store the ROM code. If the
		 *     data is read correctly, it will be copied to rom_array.
		 */
	uint8_t temp_array[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		
		//  Cyclic redundancy check. Used to confirm the ROM code.
	uint8_t crc = 0x00;		
	
		//  Detect presence, return 0 if no presence.
	if (!owi_detect_presence()) return 0;
	
		/*
		 *     After sending the READ ROM command, the master has to
		 *     generate 64 read time slots, lsb first.
		 */
	owi_write_byte(OWI_READ_ROM);
	
		/*
		 *     Read the ROM code, byte for byte.
		 */
	for (uint8_t i = 0; i < 8; i++)
	{
		for (uint8_t mask = 0x01; mask > 0x00; mask <<= 1)
		{
			if (owi_read_bit()) temp_array[i] |= mask;
		}		
	}
	
		//  Confirm CRC
	for (int i = 0; i < 7; i++) crc = owi_crc(temp_array[i], crc);
	if (crc != temp_array[7]) return crc;
	
		//  If CRC is confirmed, copy temp_array into rom_array parameter.
	for (int i = 0; i < 8; i++) rom_array[i] = temp_array[i];
	
	return 1;
}



/*
 *     The match ROM command followed by a 64-bit ROM code sequence allows
 *     the bus master to address a specific slave device on a multi-drop or
 *     single-drop bus. Only the slave that exactly matches the 64-bit ROM
 *     code sequence will respond to the function command issued by the master;
 *     all other slaves on the bus will wait for a reset pulse.
 */
void owi_match_rom(uint8_t *romcode_array)
{
	owi_write_byte(OWI_MATCH_ROM);
	for (int i = 0; i < 8; i++) owi_write_byte(romcode_array[i]);
}



/*
 *     Address all devices on the bus simultaneously without sending any ROM 
 *     code information.
 */
void owi_skip_rom(void)
{
	owi_write_byte(OWI_SKIP_ROM);	
}



/*
 *     The sequence for checking the alarm flag is:
 *     - Master issues reset pulse, slave responds with presence pulse.
 *     - Master transmits Alarm Search Command [0xEC]. The device will only 
 *       respond if the alarm flag is set.
 *     - To check if the device is still active, the master will read one bit 
 *       from the bus. If the slave is active, it will respond by placing the 
 *       value of the first bit of its ROM data onto the bus. The master will 
 *       then read another bit. The slave will place the complement of the first 
 *       bit on the bus.
 *     - If the master reads 0b11, that is, the line is never pulled low, the 
 *       device is not responding, meaning that the alarm flag is not set. If 
 *       it reads 0b01 or 0b10, the device is responding and the alarm flag is set.
 *     - Finally, the master writes the first bit of the slave's ROM code to 
 *       keep the device selected.
 */
int owi_alarm_search(void)
{	
	int first, second;
	
		//  Issue the Alarm Search Command
	if (!owi_detect_presence()) return 0;
	owi_write_byte(OWI_ALARM_SEARCH);
	
		//  Read the response
	first =  owi_read_bit();			
	second = owi_read_bit();		
	
		/*
		 *     Interpret the response.
		 *     XOR truth table:
		 *     0 XOR 0 = 0      Will never happen with only one device on the bus.
		 *     0 XOR 1 = 1      Meaning device responded.
		 *     1 XOR 0 = 1                 "
		 *     1 XOR 1 = 0      Meaning no response.
		 *     The ROM code of all DS18B20 ends with 0x28 = 0b0010 1000 so the 
		 *     response will be either 01 or 11.
		 */
	if (!(first ^ second)) return 0;     //  no response, no alarm
	
		//  Write the first bit on the bus to keep the device selected.
	if (first) owi_write_1();
	else       owi_write_0();
	return 1;
}
