#include <avr/io.h>
#include <util/delay.h>
#include "SpiLcd.h"

#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define DD_MOSI PB2
#define DD_SCK  PB1
#define DD_SS   PB0
#define DD_A0   PB4


void SPI_MasterTransmit (char cData)
{
    /* Start transmission */
    SPDR = cData;
    /* Wait for transmission complete */
    while (!(SPSR & (1 << SPIF)));
}

void SetupLcd(void)
{
    PORT_SPI |= (1 << DD_A0) | (1 << DD_SS);
    DDR_SPI |= (1 << DD_A0) | (1 << DD_SS);

    /* Set MOSI and SCK output, all others input */
    DDR_SPI |= (1 << DD_MOSI) | (1 << DD_SCK);

    /* Enable SPI, Master*/
    SPCR = (1 << SPE) | (1 << MSTR);

    // set SCK frequency = fosc/2
    SPSR = (1 << SPI2X);
}


void write_command (uint8_t c)
{
    PORT_SPI &= ~(1 << DD_A0);
    PORT_SPI &= ~(1 << DD_SS);

    SPI_MasterTransmit(c);
    PORT_SPI |= (1 << DD_SS);
}

void write_data (uint8_t d)
{
    PORT_SPI |= (1 << DD_A0);
    PORT_SPI &= ~(1 << DD_SS);

    SPI_MasterTransmit(d);
    PORT_SPI |= (1 << DD_SS);
}

void LCD_DataWrite(uint8_t LCD_DataH,uint8_t LCD_DataL)
{
    write_data(LCD_DataH);
    write_data(LCD_DataL);
}

void dsp_single_colour(uint8_t DH, uint8_t DL)
{
    uint8_t i,j;
 //RamAdressSet();
 for (i=0;i<160;i++)
    for (j=0;j<128;j++)
        LCD_DataWrite(DH,DL);
}






void lcd_initial (void)
{
    /*
     reset=0;
     delay(100);
     reset=1;
     delay(100);
     */
//------------------------------------------------------------------//
//-------------------Software Reset-------------------------------//
    write_command(0x11); //Sleep out
    _delay_ms(120);
    //ST7735R Frame Rate
    write_command(0xB1);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(0xB2);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(0xB3);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    //------------------------------------End ST7735R Frame Rate-----------------------------------------//
    write_command(0xB4); //Column inversion
    write_data(0x07);
    //------------------------------------ST7735R Power Sequence-----------------------------------------//
    write_command(0xC0);
    write_data(0xA2);
    write_data(0x02);
    write_data(0x84);
    write_command(0xC1);
    write_data(0xC5);
    write_command(0xC2);
    write_data(0x0A);
    write_data(0x00);
    write_command(0xC3);
    write_data(0x8A);
    write_data(0x2A);
    write_command(0xC4);
    write_data(0x8A);
    write_data(0xEE);
    //---------------------------------End ST7735R Power Sequence-------------------------------------//
    write_command(0xC5); //VCOM
    write_data(0x0E);
    write_command(0x36); //MX, MY, RGB mode
    write_data(0xC8);
    //------------------------------------ST7735R Gamma Sequence-----------------------------------------//
    write_command(0xe0);
    write_data(0x02);
    write_data(0x1c);
    write_data(0x07);
    write_data(0x12);
    write_data(0x37);
    write_data(0x32);
    write_data(0x29);
    write_data(0x2d);
    write_data(0x29);
    write_data(0x25);
    write_data(0x2b);
    write_data(0x39);
    write_data(0x00);
    write_data(0x01);
    write_data(0x03);
    write_data(0x10);
    write_command(0xe1);
    write_data(0x03);
    write_data(0x1d);
    write_data(0x07);
    write_data(0x06);
    write_data(0x2e);
    write_data(0x2c);
    write_data(0x29);
    write_data(0x2d);
    write_data(0x2e);
    write_data(0x2e);
    write_data(0x37);
    write_data(0x3f);
    write_data(0x00);
    write_data(0x00);
    write_data(0x02);
    write_data(0x10);
    write_command(0x2A);
    write_data(0x00);
    write_data(0x02);
    write_data(0x00);
    write_data(0x81);

    write_command(0x2B);
    write_data(0x00);
    write_data(0x01);
    write_data(0x00);
    write_data(0xA0);
    //------------------------------------End ST7735R Gamma Sequence-----------------------------------------//

    write_command(0x3A);
    write_data(0x05);
    //write_command(0x3A);//65k mode
    //write_data(0x05);
    write_command(0x29); //Displa
}


void FillDisplay(void)
{
    write_command(0x2C);
    dsp_single_colour(0xff, 0xff);
}

