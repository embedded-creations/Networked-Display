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
 *  Main source file for the USBtoSerial project. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include <util/delay.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "SpiLcd.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include "hextile.h"
PROGMEM
#include "image.h"


/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    //clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
}


unsigned char tempbuffer[MAX_TILE_SIZE];

int membefore;
int memafter;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
    SetupHardware();
    DEBUG_INITUART();
    DEBUG_SET_GPIOS_OUTPUT();
    LcdInit();

    sei();

    DEBUG_PRINTSTRING("\r\nInit!");

    // read in rectangle details from rfbfile
    unsigned int filex, filey, filew, fileh;
    filex = pgm_read_byte(rawData + 0)*256 + pgm_read_byte(rawData + 1);
    filey = pgm_read_byte(rawData + 2)*256 + pgm_read_byte(rawData + 3);
    filew = pgm_read_byte(rawData + 4)*256 + pgm_read_byte(rawData + 5);
    fileh = pgm_read_byte(rawData + 6)*256 + pgm_read_byte(rawData + 7);

    SetupHandleHextile(filex,filey,filew,fileh);

    int position = RFBFILE_HEADER_SIZE;
    int remaining;

    while(position < sizeof(rawData)) {
        remaining = sizeof(rawData) - position;
        if(remaining > MAX_TILE_SIZE)
            remaining = MAX_TILE_SIZE;

        memcpy_P(tempbuffer, rawData + position, remaining);

        membefore = freeMemory();

        int ret = HandleHextile16(tempbuffer, remaining);

        position += ret;

#if 0
        DEBUG_PRINTSTRING("before:");
        DEBUG_PRINTHEX(membefore/256);
        DEBUG_PRINTHEX(membefore);
        DEBUG_PRINTSTRING("after:");
        DEBUG_PRINTHEX(memafter/256);
        DEBUG_PRINTHEX(memafter);
#endif
    }

    for(;;) {

    }
}
