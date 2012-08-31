#ifndef _VNCSERVER_H_
#define _VNCSERVER_H_

#include <stdbool.h>
#include <stdint.h>

#define VNCSERVER_SOURCE_LUFA_USB       0
#define VNCSERVER_SOURCE_TEENSY_USB     1
#define VNCSERVER_SOURCE_UART           2

#define VNCSERVER_SOURCE VNCSERVER_SOURCE_TEENSY_USB



#if VNCSERVER_SOURCE == VNCSERVER_SOURCE_LUFA_USB
        #include "Descriptors.h"

        #include <LUFA/Version.h>
        #include <LUFA/Drivers/Board/LEDs.h>
        #include <LUFA/Drivers/Misc/RingBuffer.h>
        #include <LUFA/Drivers/USB/USB.h>

        void EVENT_USB_Device_Connect(void);
        void EVENT_USB_Device_Disconnect(void);
        void EVENT_USB_Device_ConfigurationChanged(void);
        void EVENT_USB_Device_ControlRequest(void);

        void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);
#else

    #include "usb_serial.h"

    int16_t VncServerGetData(uint8_t * buffer, uint16_t maxsize);
    void VncServerInit(void);
    uint16_t VncServerSendResponse(uint8_t * buffer, uint16_t length);

#endif

#endif
