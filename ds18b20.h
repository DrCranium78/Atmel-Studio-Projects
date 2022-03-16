/*
 * ds18b20.h
 *
 * Version: 1.0.0
 * Created: 15.12.2020
 *  Author: Frank Bjørnø
 *
 * Purpose: To facilitate communication with ds18b20 devices.
 *	
 * Limitations:
 *
 *          This code is mainly limited by the capabilities of onewire.c which does not support 
 *          the SEARCH ROM command.
 *
 * Dependencies:
 *
 *          util/delay.h: The function ds18b20_read_temp uses this to wait for the ds18b20 
 *                        device while it converts temperature.
 *          onewire.h:    The ds18b20 is a one wire device.
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

#ifdef __cplusplus
extern "C" {
#endif

enum DS18B20_RESOLUTION {DS18B20_9BIT = 0x1F, DS18B20_10BIT = 0x3F, DS18B20_11BIT = 0x5F, DS18B20_12BIT = 0x7F};
	
/*
 *     check whether DS18B20 is connected.
 *
 *     \return     1 if connected, 0 if not
 */
int ds18b20_is_connected();

/*
 *     Initiate a temperature conversion.
 *
 *     \return     1 if successful, 0 if not (only if DS18B20 is not responding)
 *
 *     Note:  The thermometer needs some time to convert the temperature.
 *            For this reason it is best to issue a start conversion command,
 *            do other things while the thermometer is busy and then read
 *            the temperature.
 */
int ds18b20_start_conversion();

/*
 *     Reads the two first bytes of the DS18B20 scratchpad and combines the two 
 *     bytes into a float. 
 *
 *     \param *temp     Pointer to float that will receive temperature.
 *     \return          0 if reading failed (only if DS18B20 is not responding),
 *                      1 if success
 */
int ds18b20_read_temp(float *temp);

/*
 *     Set the temperature resolution from 0.5 degrees to 0.0625 degrees. This 
 *     will significantly affect temperature conversion time from 93.75ms at 
 *     9 bit resolution (0.5 degrees) to 750ms at 12 bit resolution (0.0625).
 * 
 *     \param res       Use values from enumeration DS18B20_RESOLUTION, if not
 *                      behavior will be undefined. Default is 12 bit resolution.
 *     \return          0 if function failed (only if DS18B20 is not responding),
 *                      1 if success.
 */
int ds18b20_set_resolution(const int res);

/*
 *     Set low and high temperatures that will trigger an alarm flag in the DS18B20 
 *     after a temperature conversion. 
 *
 *     \param tl        Low alarm temperature. Default is -55.
 *     \param th        High alarm temperature. Default is 125
 *     \return          0 if function failed (only if DS18B20 is not responding),
 *                      1 if success.
 *
 *
 *     Note:  Note that for the DS18B20, it is impossible to check directly
 *            whether a low or a high temperature set the alarm flag.
 *            Internally in the DS18B20, the alarm flag is only set after a 
 *            temperature conversion, so for most applications involving 
 *            only one thermometer, it will be more efficient to just do a 
 *            temperature reading and handle any alarm conditions in software.
 *
 *     Note:  Only the integral part of the temperature is used in the Th and Tl 
 *            comparison. If the measured temperature is lower than or equal to
 *            Tl or higher or equal to Th, an alarm condition exists and an alarm
 *            flag is set inside the DS18B20. This means that if, f.ex, Tl is set 
 *            to 18, the alarm flag is set when the temperature drops below 19.0, not
 *            when it drops below 18.0.
 */
int ds18b20_set_alarms(int8_t tl, int8_t th);

/*
 *     Read the 64-bit lasered ROM code into an 8 byte array of uint8_t.
 *     
 *     \param *romcode_array   Pointer to an 8 byte array of uint8_t
 *                             that will receive the ROM code.
 *     \return                 1 for success, 0 for failure
 *
 *     Note:  This command can only be used when there is one slave on
 *            the bus, otherwise a data collision will occur when all
 *            the slaves attempt to respond at the same time.
 *
 *     Note:  The ROM code will be stored in the following order:
 *            romcode_array[0]:          8-bit family code
 *            romcode_array[1 - 6]:     48-bit serial number LSB first
 *            romcode_array[7]:          8-bit crc
 */
int ds18b20_read_rom(uint8_t *romcode_array);

/*
 *     Set a 64-bit ROM code that will be used in the next, and only
 *     the next call to the following functions:
 *          ds18b20_read_temp
 *          ds18b20_set_resolution
 *          ds18b20_set_alarms
 *
 *     \param *romcode_array Pointer to an 8 byte array of uint8_t
 *                           holding the ROM code.
 *     \return               1 if device is active, 0 if not.
 *
 *     Note:  The Rom code must be formatted in a little-endian system:
 *            romcode_array[0]:          8-bit family code
 *            romcode_array[1 - 6]:     48-bit serial number LSB first
 *            romcode_array[7]:          8-bit crc
 */
void ds18b20_set_rom(uint8_t *romcode_array);

/*
 *     Check if the latest temperature conversion triggered an alarm. 
 *
 *     \return   0 means no alarm flag, 1 means alarm flag is set
 *
 *     Note:  This only detects if an alarm flag was set, it does
 *            not say anything about whether it was triggered by 
 *            comparing the temperature to Tl of Th.
 */
int ds18b20_check_alarm();


#ifdef __cplusplus
}
#endif
