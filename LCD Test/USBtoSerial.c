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

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      USARTtoUSB_Buffer_Data[128];

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
    {
        .Config =
            {
                .ControlInterfaceNumber         = 0,

                .DataINEndpointNumber           = CDC_TX_EPNUM,
                .DataINEndpointSize             = CDC_TXRX_EPSIZE,
                .DataINEndpointDoubleBank       = false,

                .DataOUTEndpointNumber          = CDC_RX_EPNUM,
                .DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
                .DataOUTEndpointDoubleBank      = false,

                .NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
                .NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
                .NotificationEndpointDoubleBank = false,
            },
    };


#define VNC_BUFFER_MAX 200

uint8_t vncBuffer[VNC_BUFFER_MAX];
uint8_t vncResponseBuffer[MAXIMUM_VNCRESPONSE_SIZE];

unsigned int vncBufferSize = 0;

volatile bool usbConnectionReset = false;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
    unsigned int vncRemainder;
    unsigned int vncResponseSize = 0;

    SetupHardware();
    Vnc_Init();

    RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
    RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
    sei();

    TransmitString("Init!");

    for (;;)
    {
        if(usbConnectionReset)
        {
            // reset system
            vncRemainder = 0;
            vncBufferSize = 0;
            Vnc_ResetSystem();

            usbConnectionReset = false;
        }

        while ( vncBufferSize < VNC_BUFFER_MAX )
        {
            int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
            if(ReceivedByte < 0)
                break;

            vncBuffer[vncBufferSize++] = (uint8_t)ReceivedByte;
        }

        vncRemainder = Vnc_ProcessVncBuffer(vncBuffer, vncBufferSize);

        // move unused data to the front of the buffer
        memcpy(vncBuffer, vncBuffer + (vncBufferSize - vncRemainder), vncRemainder);

        vncBufferSize = vncRemainder;

        // collect any data to send to the Vnc server and send it
        if(!vncResponseSize)
            vncResponseSize = Vnc_LoadResponseBuffer(vncResponseBuffer);

        int i;
        for(i=0; i<vncResponseSize; i++)
        {
            if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
                                    vncResponseBuffer[i]) != ENDPOINT_READYWAIT_NoError)
            {
                TransmitString("TXfull");
                if(i != 0)
                {
                    memcpy(vncResponseBuffer, vncResponseBuffer + i,
                            vncResponseSize - i);
                    vncResponseSize -= i;
                    i=0;
                }
                break;
            }
        }
        if(i == vncResponseSize)
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



        CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
        USB_USBTask();
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

    USB_Init();

    /* Start the flush timer so that overflows occur rapidly to push received bytes to the USB interface */
    TCCR0B = (1 << CS02);
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
    Serial_SendByte('E');
}

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
    Serial_SendByte('!');

    // we can assume we just lost or gained connection with the VNC server, clear any buffers we have
    usbConnectionReset = true;
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;

    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

    LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
    uint8_t ReceivedByte = UDR1;

    if (USB_DeviceState == DEVICE_STATE_Configured)
      RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
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
        TransmitByte('A' + temp - 0x0A);
    else TransmitByte('0' + temp);

    temp = data & 0x0F;
    if(temp >= 0x0A)
        TransmitByte('A' + temp - 0x0A);
    else TransmitByte('0' + temp);
}
