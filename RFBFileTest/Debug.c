#include <string.h>
#include <avr/io.h>
#include "Debug.h"


#if DEBUGOUTPUT_TARGET == DEBUGOUTPUT_LUFA_UART

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

void DebugInit(void)
{
    /* Set the new baud rate before configuring the USART */
    UBRR1  = SERIAL_2X_UBBRVAL(57600);

    uint8_t ConfigMask = ((1 << UCSZ11) | (1 << UCSZ10));

    /* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
    UCSR1C = ConfigMask;
    UCSR1A = (1 << U2X1);
    UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
}

#endif
