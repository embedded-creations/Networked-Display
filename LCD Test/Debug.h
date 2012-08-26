#ifndef _DEBUG_H_
#define _DEBUG_H_

void DebugInit(void);
void TransmitString(char * string);
void TransmitByte(unsigned char byte);
void TransmitHex(unsigned char data);
void DebugInit(void);

#if 1
        #define DEBUG_INIT()            DebugInit()
        #define DEBUG_PRINTSTRING(s)    TransmitString(s)
        #define DEBUG_PRINTBYTE(b)      TransmitByte(b)
        #define DEBUG_PRINTHEX(h)       TransmitHex(h)
#else
        #define DEBUG_INIT()
        #define DEBUG_PRINTSTRING(s)
        #define DEBUG_PRINTBYTE(b)
        #define DEBUG_PRINTHEX(h)
#endif

#endif
