#include "VncServerComms.h"
#include <avr/io.h>
#include "buffuart.h"

volatile bool usbConnectionReset = false;


#if VNCSERVER_SOURCE == VNCSERVER_SOURCE_LUFA_USB
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



void VncServerInit(void)
{
    USB_Init();

    /* Start the flush timer so that overflows occur rapidly to push received bytes to the USB interface */
    TCCR0B = (1 << CS02);

    RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
    RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));
}

uint16_t debugcounter = 0;

int16_t VncServerGetData(uint8_t * buffer, uint16_t maxsize)
{
    uint16_t size = 0;

    if(usbConnectionReset)
    {
        usbConnectionReset = false;
        return -1;
    }

    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();

    while(debugcounter > 64)
    {
        DDRF |= (1 << 0);
        PORTF |= (1 << 0);
        debugcounter -= 64;
        PORTF &= ~(1 << 0);
    }

    while ( size < maxsize )
    {
        int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
        if(ReceivedByte < 0)
            break;

        buffer[size++] = (uint8_t)ReceivedByte;
        debugcounter++;
    }

    return size;
}

uint16_t VncServerSendResponse(uint8_t * buffer, uint16_t length)
{
    int i;
    for(i=0; i<length; i++)
    {
        if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
                buffer[i]) != ENDPOINT_READYWAIT_NoError)
        {
            break;
        }
    }

    return i;
}


/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    //LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
    //DEBUG_PRINTBYTE('E');
}

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
    //DEBUG_PRINTBYTE('!');

    // we can assume we just lost or gained connection with the VNC server, clear any buffers we have
    usbConnectionReset = true;
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    //LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;

    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

    //LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
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
#endif


#if VNCSERVER_SOURCE == VNCSERVER_SOURCE_TEENSY_USB

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

void VncServerInit(void)
{
    CPU_PRESCALE(0);
    usb_init();
}

uint16_t debugcounter = 0;

int16_t VncServerGetData(uint8_t * buffer, uint16_t maxsize)
{
    uint16_t size = 0;

    if(usbConnectionReset)
    {
        usbConnectionReset = false;
        return -1;
    }

    while(debugcounter > 64)
    {
        DDRF |= (1 << 0);
        PORTF |= (1 << 0);
        debugcounter -= 64;
        PORTF &= ~(1 << 0);
    }

    while ( size < maxsize )
    {
        int n = usb_serial_getchar();

        if(n < 0)
            break;

        buffer[size++] = (uint8_t)n;
        debugcounter++;
    }

    return size;
}

uint16_t VncServerSendResponse(uint8_t * buffer, uint16_t length)
{
    int i;
    for(i=0; i<length; i++)
    {
        usb_serial_putchar(buffer[i]);
    }

    return i;
}

#endif


#if VNCSERVER_SOURCE == VNCSERVER_SOURCE_UART

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

void VncServerInit(void)
{
    CPU_PRESCALE(0);

    BuffUart_Setup();
}

uint16_t debugcounter = 0;

int16_t VncServerGetData(uint8_t * buffer, uint16_t maxsize)
{
    uint16_t size = 0;

    while(debugcounter > 64)
    {
        DDRF |= (1 << 0);
        PORTF |= (1 << 0);
        debugcounter -= 64;
        PORTF &= ~(1 << 0);
    }

    while ( size < maxsize )
    {
        if(!BuffUart_DataInReceiveBuffer())
            break;

        buffer[size++] = BuffUart_Receive();
        debugcounter++;
    }

    return size;
}

uint16_t VncServerSendResponse(uint8_t * buffer, uint16_t length)
{
    int i;
    for(i=0; i<length; i++)
    {
        BuffUart_Transmit(buffer[i]);
    }

    return i;
}

#endif
