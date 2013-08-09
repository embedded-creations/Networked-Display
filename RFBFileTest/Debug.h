#ifndef _DEBUG_H_
#define _DEBUG_H_

void DebugInit(void);
void TransmitString(char * string);
void TransmitByte(unsigned char byte);
void TransmitHex(unsigned char data);
void DebugInit(void);


#define DEBUG_GPIO_0 PB6
#define DEBUG_GPIO_1 PB7

#define DEBUG_GPIO_PORT PORTB
#define DEBUG_GPIO_DDR  DDRB

#define DEBUG_SET_GPIO0_HIGH()      (DEBUG_GPIO_PORT |= (1 << DEBUG_GPIO_0))
#define DEBUG_SET_GPIO0_LOW()       (DEBUG_GPIO_PORT &= ~(1 << DEBUG_GPIO_0))
#define DEBUG_SET_GPIO1_HIGH()      (DEBUG_GPIO_PORT |= (1 << DEBUG_GPIO_1))
#define DEBUG_SET_GPIO1_LOW()       (DEBUG_GPIO_PORT &= ~(1 << DEBUG_GPIO_1))

#define DEBUG_SET_GPIOS_OUTPUT()    (DEBUG_GPIO_DDR |= (1 << DEBUG_GPIO_0) | (1 << DEBUG_GPIO_1) )


#define DEBUGOUTPUT_LUFA_UART 0
#define DEBUGOUTPUT_UART      1
#define DEBUGOUTPUT_NONE      2

#define DEBUGOUTPUT_TARGET DEBUGOUTPUT_UART


#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_LUFA_UART
    #include <LUFA/Drivers/Peripheral/Serial.h>
    #define DEBUG_INITUART()        DebugInit()
    #define DEBUG_PRINTSTRING(s)    TransmitString(s)
    #define DEBUG_PRINTBYTE(b)      TransmitByte(b)
    #define DEBUG_PRINTHEX(h)       TransmitHex(h)
#endif

#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_NONE
    #define DEBUG_INITUART()
    #define DEBUG_PRINTSTRING(s)
    #define DEBUG_PRINTBYTE(b)
    #define DEBUG_PRINTHEX(h)
#endif

#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_UART
    #include "buffuart.h"
    #define DEBUG_INITUART()        BuffUart_Setup()
    #define DEBUG_PRINTSTRING(s)    BuffUart_TransmitString(s)
    #define DEBUG_PRINTBYTE(b)      BuffUart_Transmit(b)
    #define DEBUG_PRINTHEX(h)       BuffUart_TransmitHex(h)
#endif

#endif
