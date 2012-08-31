#include <avr/io.h>
#include <avr/interrupt.h>

#include "buffUART.h"

static unsigned char 			UART_RxBuf[UART_RX_BUFFER_SIZE] ;
static volatile unsigned int 	UART_RxHead ;
static volatile unsigned int 	UART_RxTail ;
static unsigned char 			UART_TxBuf[UART_TX_BUFFER_SIZE] ;
static volatile unsigned char 	UART_TxHead ;
static volatile unsigned char 	UART_TxTail ;


/* sends a null-terminated string of length <256 to the UART */
void BuffUart_TransmitString( char *stringPointer )
{
	unsigned char index = 0 ;
	while( stringPointer[index] != '\0' )
	{
	    BuffUart_Transmit( stringPointer[index++] ) ;
	}
	
	return ;
}

void BuffUart_Setup( void )
{
    /* enable RxD/TxD and interrupt on character received */
	UCSR1B = (1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1);
    
    /* set baud rate */
	UBRR1 = UART_BAUD_SELECT;

	UART_RxTail = 0;						/* flush the transmit and receive buffers */

	UART_RxHead = 0;
	UART_TxTail = 0;
	UART_TxHead = 0;

	return ;
}



SIGNAL(USART1_RX_vect)
{
	unsigned char data;
	unsigned int tmphead;

	data = UDR1 ; 								/* read the received data */

	
	tmphead = (UART_RxHead + 1) ;						/* calculate buffer index */
	//tmphead &= UART_RX_BUFFER_MASK  ;
    if(tmphead == UART_RX_BUFFER_SIZE)
        tmphead = 0;
	
	UART_RxHead = tmphead; 								/* store new index */
	
	if ( tmphead == UART_RxTail )
	{
		/* ERROR! Receive buffer overflow */
	}
	
	UART_RxBuf[tmphead] = data; /* store received data in buffer */
}

SIGNAL( USART1_UDRE_vect )
{
	unsigned char tmptail;

	/* check if all data is transmitted */
	if ( UART_TxHead != UART_TxTail )
	{
		tmptail = ( UART_TxTail + 1 ) ;					/* calculate buffer index */
		tmptail &= UART_TX_BUFFER_MASK ;
		UART_TxTail = tmptail; 							/* store new index */
		UDR1 = UART_TxBuf[tmptail];				/* start transmition */
	}
	else
	{
														/* Buffer Empty - 		*/
		UCSR1B &= ~_BV(UDRIE1); 							/* disable UDRE interrupt */
	}
}



char BuffUart_Receive( void )
{
	unsigned int tmptail;
	
	while ( UART_RxHead == UART_RxTail ) ;				/* wait for incomming data */
	
	tmptail = ( UART_RxTail + 1 ) ;
	//tmptail &= UART_RX_BUFFER_MASK ;					/* calculate buffer index */
    if(tmptail == UART_RX_BUFFER_SIZE)
        tmptail = 0;
	UART_RxTail = tmptail; 								/* store new index */
	return UART_RxBuf[tmptail]; 						/* return data */
}


void BuffUart_Transmit( char data )
{
	unsigned char tmphead;
	
	tmphead = ( UART_TxHead + 1 ) ;
	tmphead &= UART_TX_BUFFER_MASK ;					/* calculate buffer index */
	
#if 1
	while ( tmphead == UART_TxTail );					/* wait for free space in buffer */
#else
	// don't wait for space if buffer is full
	if(tmphead == UART_TxTail)
	    return;
#endif
	
	UART_TxBuf[tmphead] = data; 						/* store data in buffer */
	UART_TxHead = tmphead; 								/* store new index */
	UCSR1B |= _BV(UDRIE1) ;								/* enable UDRE interrupt */

	return ;
}


unsigned char BuffUart_DataInReceiveBuffer( void )
{
	return ( UART_RxHead != UART_RxTail ); 			/* return 0 (FALSE) if the receive buffer is empty */
}

#if 1
void BuffUart_TransmitHex(unsigned char data)
{
    unsigned char temp = data & 0xF0;
    temp >>= 4;

    if(temp >= 0x0A)
        BuffUart_Transmit('A' + temp - 0x0A);
    else BuffUart_Transmit('0' + temp);

    temp = data & 0x0F;
    if(temp >= 0x0A)
        BuffUart_Transmit('A' + temp - 0x0A);
    else BuffUart_Transmit('0' + temp);
}
#endif

