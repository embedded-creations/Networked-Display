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

#include "USBtoSerial.h"
#include <util/delay.h>
#include "SpiLcd.h"
#include "ParallelLcd.h"
#include "vnc.h"
#include "VncServerComms.h"


#define VNC_BUFFER_MAX 200

uint8_t vncBuffer[VNC_BUFFER_MAX];
uint8_t vncResponseBuffer[MAXIMUM_VNCRESPONSE_SIZE];

unsigned int vncBufferSize = 0;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
    unsigned int vncRemainder;
    unsigned int vncResponseSize = 0;

    SetupHardware();
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
            vncBufferSize += response;

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

#if 0
        // modified to make USB loopback
        /* Check if the UART receive buffer flush timer has expired or the buffer is nearly full */
        uint16_t BufferCount = RingBuffer_GetCount(&USBtoUSART_Buffer);
        if ((TIFR0 & (1 << TOV0)) || (BufferCount > (uint8_t)(sizeof(USBtoUSART_Buffer) * (3 / 4))))
        {
            /* Clear flush timer expiry flag */
            TIFR0 |= (1 << TOV0);

            /* Read bytes from the USART receive buffer into the USB IN endpoint */
            while (BufferCount--)
            {
                /* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
                if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
                                        RingBuffer_Peek(&USBtoUSART_Buffer)) != ENDPOINT_READYWAIT_NoError)
                {
                    break;
                }

                /* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
                RingBuffer_Remove(&USBtoUSART_Buffer);
            }
        }
#endif



    }
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    LEDs_Init();

    /* Set the new baud rate before configuring the USART */
    UBRR1  = SERIAL_2X_UBBRVAL(57600);

    uint8_t ConfigMask = ((1 << UCSZ11) | (1 << UCSZ10));

    /* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
    UCSR1C = ConfigMask;
    UCSR1A = (1 << U2X1);
    UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

}



void TransmitString(char * string)
{
    for(int i=0; i<strlen(string); i++)
        Serial_SendByte(string[i]);
}

void TransmitByte(unsigned char byte)
{
    Serial_SendByte(byte);
}

void TransmitHex(unsigned char data)
{
    unsigned char temp = data & 0xF0;
    temp >>= 4;

    if(temp >= 0x0A)
        DEBUG_PRINTBYTE('A' + temp - 0x0A);
    else DEBUG_PRINTBYTE('0' + temp);

    temp = data & 0x0F;
    if(temp >= 0x0A)
        DEBUG_PRINTBYTE('A' + temp - 0x0A);
    else DEBUG_PRINTBYTE('0' + temp);
}
