#ifndef _VNCSERVER_H_
#define _VNCSERVER_H_

#include <stdbool.h>
#include <stdint.h>

        #include "Descriptors.h"

        #include <LUFA/Version.h>
        #include <LUFA/Drivers/Board/LEDs.h>
        #include <LUFA/Drivers/Peripheral/Serial.h>
        #include <LUFA/Drivers/Misc/RingBuffer.h>
        #include <LUFA/Drivers/USB/USB.h>

        void EVENT_USB_Device_Connect(void);
        void EVENT_USB_Device_Disconnect(void);
        void EVENT_USB_Device_ConfigurationChanged(void);
        void EVENT_USB_Device_ControlRequest(void);

        void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);



int16_t VncServerGetData(uint8_t * buffer, uint16_t maxsize);
void VncServerInit(void);
uint16_t VncServerSendResponse(uint8_t * buffer, uint16_t length);

#endif
