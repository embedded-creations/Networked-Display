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
//#include "ParallelLcd.h"
#include "vnc.h"
#include "VncServerComms.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include "debug.h"
#include "Buttons.h"
#include "hextile.h"

#define VNC_BUFFER_MAX 200

uint8_t vncBuffer[VNC_BUFFER_MAX];
uint8_t vncResponseBuffer[MAXIMUM_VNCRESPONSE_SIZE];

unsigned int vncBufferSize = 0;


/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    //clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    Buttons_Init();
}

uint16_t debugcounter2 = 0;



unsigned char tempbuffer[MAX_TILE_SIZE];

PROGMEM
#include "image.h"



/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
    unsigned int vncRemainder;
    unsigned int vncResponseSize = 0;
    int retval;

    SetupHardware();
    DEBUG_INIT();
    VncServerInit();
    Vnc_Init();

    sei();

    DEBUG_PRINTSTRING("\r\nInit!");

#define BYTES_PER_PIXEL  2

    uint16_t background = 0;
    int i, j;

    static unsigned char hextileBuffer[16][16 * BYTES_PER_PIXEL];

    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 16 * BYTES_PER_PIXEL; i+=BYTES_PER_PIXEL)
        {
            hextileBuffer[ j ][ i ] = background;
            if(BYTES_PER_PIXEL == 2)
                hextileBuffer[ j ][ i+1 ] = background/256;
        }
    }


#if 0
    SetupTile(0,0,16,16);
    //DrawHextile(16,16, 2, hextileBuffer);
    DrawRawTile(16*16, 2, rawData + 1);
#else

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

      int ret = HandleHextile16(tempbuffer, remaining);

      position += ret;
    }
#endif


    for(;;) {

    }


#if 0
    for (;;)
    {
        int response = VncServerGetData(vncBuffer + vncBufferSize, VNC_BUFFER_MAX - vncBufferSize);

        if(response < 0)
        {
            // reset system
            vncRemainder = 0;
            vncBufferSize = 0;
            Vnc_ResetSystem();
            DEBUG_PRINTSTRING("ResetSystem ");
            continue;
        }
        else
        {
            vncBufferSize += response;
        }

        retval = Vnc_ProcessVncBuffer(vncBuffer, vncBufferSize);

        if (vncRemainder < 0)
        {
            // reset system
            vncRemainder = 0;
            vncBufferSize = 0;
            Vnc_ResetSystem();
            DEBUG_PRINTSTRING("ResetSystem2 ");
            continue;
        }

        vncRemainder = (unsigned int)retval;

        // move unused data to the front of the buffer
        memcpy(vncBuffer, vncBuffer + (vncBufferSize - vncRemainder), vncRemainder);

        vncBufferSize = vncRemainder;

        Buttons_Handler();

        // collect any data to send to the Vnc server and send it
        if(!vncResponseSize)
            vncResponseSize = Vnc_LoadResponseBuffer(vncResponseBuffer);

        uint16_t vncResponseRemainder = VncServerSendResponse(vncResponseBuffer, vncResponseSize) - vncResponseSize;

        if(vncResponseRemainder)
        {
            DEBUG_PRINTSTRING("TXfull");
            memcpy(vncResponseBuffer, vncResponseBuffer + vncResponseRemainder,
                    vncResponseSize - vncResponseRemainder);
            vncResponseSize -= vncResponseRemainder;
        }
        else
            vncResponseSize = 0;
    }
#endif
}






