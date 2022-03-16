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


#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



/*
 *     All transactions on the one wire bus begin with an initialization sequence.
 *     This sequence consists of a reset pulse transmitted by the bus master followed
 *     by presence pulse(s) transmitted by the slave(s). The presence pulse lets the
 *     bus master know that slave devices are on the bus and ready to operate.
 *
 *     \return     1 for presence, 0 for no presence 
 */
int owi_detect_presence(void);

/*
 *     Send one byte of data on the one wire bus.
 *
 *     \param data    one byte of data to send.
 *  
 *     note: The parameter is an unsigned char, but it is, in the end, just zeros
 *           and ones. The emphasis is on data, not how it is represented.
 */
void owi_write_byte(unsigned char data);

/*
 *     Read one byte of data from the one wire bus
 *     
 *     \return        one byte of data
 */
unsigned char owi_read_byte(void);

/*
 *     Check if one wire device is busy.
 *
 *     \return        1 for busy, 0 for not busy
 */
int owi_is_busy(void);

/*
 *     NOT IMPLEMENTED!
 *     After the bus master has detected a presence pulse, it can issue a ROM
 *     command. These commands operate on the unique 64-bit ROM codes of each
 *     slave device and allow the master to single out a specific device if 
 *     many are present on the bus. There are five ROM commands.
 */
void owi_search_rom(void);


/*
 *     Read the 64-bit lasered ROM code into an 8 byte array of uint8_t.
 *     
 *     \param *romcode_array   Pointer to an 8 byte array of uint8_t
 *                             to receive the ROM code
 *     \return                 1 for success, 0 for failure
 *
 *     note:  The ROM code will be stored in the following order:
 *            romcode_array[0]:          8-bit family code
 *            romcode_array[1 - 6]:     48-bit serial number
 *            romcode_array[7]:          8-bit crc
 *
 *     note:  The read ROM command can only be used when there is only one slave 
 *            on the bus. It allows the master to read the slave's 64-bit ROM code 
 *            without using the search ROM procedure. If this command is used when 
 *            there is more than one slave present on the bus, a data collision 
 *            will occur when all the slaves attempt to respond at the same time.
 */
int owi_read_rom(uint8_t *rom_array);

/*
 *     NOT IMPLEMENTED
 */
void owi_match_rom(uint8_t *romcode_array);

/*
 *     Address all devices on the bus simultaneously without sending any ROM code
 *     information. For example, the master can make all DS18B20s on the bus perform
 *     simultaneous temperature conversions by issuing a skip ROM command followed 
 *     by a convert t command.
 */
void owi_skip_rom(void);

/*
 *     Check if any device on the bus has an alarm flag set.
 *
 *     \return: 1 if alarm, 0 if no alarm
 */
int owi_alarm_search(void);

	
#ifdef __cplusplus
}
#endif
