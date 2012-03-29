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


void SlowLoadDisplay(void)
{
    uint8_t i,j;
 //RamAdressSet();
 for (i=0;i<160;i++)
    for (j=0;j<128;j++)
    {
        LCD_DataWrite(0,0);
        _delay_ms(25);
    }
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

    // ST7735R Frame Rate (Frame rate=fosc/((RTNA + 20) x (LINE + FPA + BPA)))
    // default values are written for all (unnecessary?)
    // normal/full
    write_command(0xB1);
    write_data(0x01); // RTNA = 1
    write_data(0x2C); // FPA = 0x2C
    write_data(0x2D); // BPA = 0x2D

    // idle/8
    write_command(0xB2);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);

    // partial/full
    write_command(0xB3);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    //------------------------------------End ST7735R Frame Rate-----------------------------------------//
    write_command(0xB4); //Column inversion
    write_data(0x07); // frame inversion for all modes
    //------------------------------------ST7735R Power Sequence-----------------------------------------//
    write_command(0xC0);
    write_data(0xA2); // values and number of values don't seem to match spec - default is 2, 0xA2 is setting don't cares
    write_data(0x02); // default is 0x70
    write_data(0x84); // there's only two parameters, what's this third?
    write_command(0xC1);
    write_data(0xC5); // default is 5, C5 is setting don't care bits
    write_command(0xC2);
    write_data(0x0A); // default is 1, 0xA = medium low + don't cares
    write_data(0x00); // default is 1, 0 = lower step-up cycle in booster 2,4
    write_command(0xC3);
    write_data(0x8A);
    write_data(0x2A);
    write_command(0xC4);
    write_data(0x8A);
    write_data(0xEE);
    //---------------------------------End ST7735R Power Sequence-------------------------------------//
    write_command(0xC5); //VCOM voltage setting
    write_data(0x0E);
    write_command(0x36); //MX, MY, RGB mode
    write_data(0xC8);   // MH=0 : horiz refresh left to right
                        // RGB=1 : BGR color filter panel
                        // ML=0 : vert refresh top to bottom
                        // MV=0, MX=MY=1 (controls MCU to memory write/read direction)
    //write_data(0xE8);  //MV, MX, MY
    //write_data(0x48); //!MV, !MY, MX
    //write_data(0x08); // ! ! !

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

    // column address set: XS = 2, XE = 129
    write_command(0x2A);
    write_data(0x00);
    write_data(0x02);
    write_data(0x00);
    write_data(0x81);

    // row address set: YS = 1, YE = 160
    write_command(0x2B);
    write_data(0x00);
    write_data(0x01);
    write_data(0x00);
    write_data(0xA0);
    //------------------------------------End ST7735R Gamma Sequence-----------------------------------------//

    // interface pixel format:
    write_command(0x3A);
    write_data(0x05); // 16-bit per pixel
    //write_command(0x3A);//65k mode
    //write_data(0x05);

    // display on
    write_command(0x29);
}


void ClearDisplay(void)
{
    write_command(0x2C); // memory write
    dsp_single_colour(0xff, 0xff);
}

void Write8bitPixel(uint8_t pixel)
{
    uint16_t pixel16 = 0;

    pixel16 |= (pixel & 0x07) << 13;
    pixel16 |= (pixel & (0x07 << 3)) << 5;
    pixel16 |= (pixel & (0x03 << 6)) << 3;


    LCD_DataWrite(pixel16/256, pixel16);
}


#define ROW_OFFSET_L    1
#define COL_OFFSET_L    2

void DrawHextile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH, void * hextileBuffer)
{
    unsigned int low, high;

    // setup tile boundaries
    // column address set: XS = 2, XE = 129
    low = COL_OFFSET_L + tileX;
    high = COL_OFFSET_L + tileX + tileW - 1;
    write_command(0x2A);
    write_data(low/256);
    write_data(low);
    write_data(high/256);
    write_data(high);

    // row address set: YS = 1, YE = 160
    low = ROW_OFFSET_L + tileY;
    high = ROW_OFFSET_L + tileY + tileH - 1;
    write_command(0x2B);
    write_data(low/256);
    write_data(low);
    write_data(high/256);
    write_data(high);


    // write tile
    write_command(0x2C); // memory write

    for (unsigned int j = 0; j < tileH; j++)
    {
        for (unsigned int i = 0; i < tileW; i++)

        {
            Write8bitPixel(*(unsigned char*)(hextileBuffer + i + j*16));
        }
    }
}


