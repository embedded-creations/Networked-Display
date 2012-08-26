/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for USBtoSerial.c.
 */

#ifndef _USB_SERIAL_H_
#define _USB_SERIAL_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/interrupt.h>
		#include <avr/power.h>


	/* Function Prototypes: */
		void SetupHardware(void);


        void TransmitString(char * string);
        void TransmitByte(unsigned char byte);
        void TransmitHex(unsigned char data);

#if 1
        #define DEBUG_PRINTSTRING(s)    TransmitString(s)
        #define DEBUG_PRINTBYTE(b)      TransmitByte(b)
        #define DEBUG_PRINTHEX(h)       TransmitHex(h)
#else
        #define DEBUG_PRINTSTRING(s)
        #define DEBUG_PRINTBYTE(b)
        #define DEBUG_PRINTHEX(h)
#endif

#endif

