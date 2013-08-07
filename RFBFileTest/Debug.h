#ifndef _DEBUG_H_
#define _DEBUG_H_

void DebugInit(void);
void TransmitString(char * string);
void TransmitByte(unsigned char byte);
void TransmitHex(unsigned char data);
void DebugInit(void);

#define DEBUGOUTPUT_LUFA_UART 0
#define DEBUGOUTPUT_UART      1
#define DEBUGOUTPUT_NONE      2

#define DEBUGOUTPUT_TARGET DEBUGOUTPUT_UART


#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_LUFA_UART
    #include <LUFA/Drivers/Peripheral/Serial.h>
    #define DEBUG_INIT()            DebugInit()
    #define DEBUG_PRINTSTRING(s)    TransmitString(s)
    #define DEBUG_PRINTBYTE(b)      TransmitByte(b)
    #define DEBUG_PRINTHEX(h)       TransmitHex(h)
#endif

#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_NONE
    #define DEBUG_INIT()
    #define DEBUG_PRINTSTRING(s)
    #define DEBUG_PRINTBYTE(b)
    #define DEBUG_PRINTHEX(h)
#endif

#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_UART
    #include "buffuart.h"
    #define DEBUG_INIT()            BuffUart_Setup()
    #define DEBUG_PRINTSTRING(s)    BuffUart_TransmitString(s)
    #define DEBUG_PRINTBYTE(b)      BuffUart_Transmit(b)
    #define DEBUG_PRINTHEX(h)       BuffUart_TransmitHex(h)
#endif

#endif
