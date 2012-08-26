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
#include "SpiLcd.h"
#include "ParallelLcd.h"
#include "vnc.h"
#include "VncServerComms.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include "debug.h"

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
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    //LEDs_Init();
}


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
    unsigned int vncRemainder;
    unsigned int vncResponseSize = 0;

    SetupHardware();
    DEBUG_INIT();
    VncServerInit();
    Vnc_Init();

    sei();

    DEBUG_PRINTSTRING("Init!");

    for (;;)
    {
        int response = VncServerGetData(vncBuffer + vncBufferSize, VNC_BUFFER_MAX - vncBufferSize);

        if(response < 0)
        {
            // reset system
            vncRemainder = 0;
            vncBufferSize = 0;
            Vnc_ResetSystem();
            continue;
        }
        else
        {
            vncBufferSize += response;
        }

        vncRemainder = Vnc_ProcessVncBuffer(vncBuffer, vncBufferSize);

        // move unused data to the front of the buffer
        memcpy(vncBuffer, vncBuffer + (vncBufferSize - vncRemainder), vncRemainder);

        vncBufferSize = vncRemainder;

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
}






