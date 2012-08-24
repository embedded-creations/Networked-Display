#include <avr/io.h>
#include <util/delay.h>
#include "SpiLcd.h"

#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define DD_MOSI PB2
#define DD_SCK  PB1
#define DD_SS   PB0
#define DD_A0   PB4
#define DD_RES  PB5




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


#define PLCD_DATA_DDR       DDRD
#define PLCD_DATA_PORT      PORTD

#define PLCD_CONTROL_DDR    DDRF
#define PLCD_CONTROL_PORT   PORTF
#define PLCD_RESET          PF4
#define PLCD_CS             PF1
#define PLCD_RD             PF7
#define PLCD_WR             PF6
#define PLCD_RS             PF5

#define PLCD_DRIVE_RESET_LOW()    (PLCD_CONTROL_PORT &= ~(1 << PLCD_RESET))
#define PLCD_DRIVE_RESET_HIGH()   (PLCD_CONTROL_PORT |= (1 << PLCD_RESET))

#define PLCD_DRIVE_CS_LOW()    (PLCD_CONTROL_PORT &= ~(1 << PLCD_CS))
#define PLCD_DRIVE_CS_HIGH()   (PLCD_CONTROL_PORT |= (1 << PLCD_CS))

#define PLCD_DRIVE_RD_LOW()    (PLCD_CONTROL_PORT &= ~(1 << PLCD_RD))
#define PLCD_DRIVE_RD_HIGH()   (PLCD_CONTROL_PORT |= (1 << PLCD_RD))

#define PLCD_DRIVE_WR_LOW()    (PLCD_CONTROL_PORT &= ~(1 << PLCD_WR))
#define PLCD_DRIVE_WR_HIGH()   (PLCD_CONTROL_PORT |= (1 << PLCD_WR))

#define PLCD_DRIVE_RS_LOW()    (PLCD_CONTROL_PORT &= ~(1 << PLCD_RS))
#define PLCD_DRIVE_RS_HIGH()   (PLCD_CONTROL_PORT |= (1 << PLCD_RS))


void Lcd_Write_Data(uint16_t DH)
{
    PLCD_DRIVE_RS_HIGH();
    PLCD_DRIVE_CS_LOW();
    PORTD = DH >>8; //LCD_DataPortH=DH>>8;
    PLCD_DRIVE_WR_LOW();
    PLCD_DRIVE_WR_HIGH();
    PORTD = DH;//LCD_DataPortH=DH;
    PLCD_DRIVE_WR_LOW();
    PLCD_DRIVE_WR_HIGH();
    PLCD_DRIVE_CS_HIGH();
}

void Lcd_Write_Com( uint16_t  DH)
{
    PLCD_DRIVE_RS_LOW();
    PLCD_DRIVE_CS_LOW();
    PORTD = DH >>8; //LCD_DataPortH=DH>>8;
    PLCD_DRIVE_WR_LOW();
    PLCD_DRIVE_WR_HIGH();
    PORTD = DH;//LCD_DataPortH=DH;
    PLCD_DRIVE_WR_LOW();
    PLCD_DRIVE_WR_HIGH();
    PLCD_DRIVE_CS_HIGH();
}

void Lcd_Write_Com_Data( uint16_t com1,uint16_t dat1)
{
   Lcd_Write_Com(com1);
   Lcd_Write_Data(dat1);
}


void Address_set(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2)
{

  Lcd_Write_Com_Data(0x0002,0x0000);
  Lcd_Write_Com_Data(0x0003,x1);
  Lcd_Write_Com_Data(0x0004,0x0000);
  Lcd_Write_Com_Data(0x0005,x2);

  Lcd_Write_Com_Data(0x0006,0x0000);
  Lcd_Write_Com_Data(0x0007,y1);
  Lcd_Write_Com_Data(0x0008,0x0000);
  Lcd_Write_Com_Data(0x0009,y2);

  Lcd_Write_Com (0x22);//LCD_WriteCMD(GRAMWR);

}

void Pant(unsigned int color)
{
    int i,j;
    Address_set(0,0,175,219);

    for(i=0;i<220;i++)
     {
      for (j=0;j<176;j++)
        {
         Lcd_Write_Data(color);
        }

      }
}

void ParallelDisplay_Init(void)
{
    PLCD_CONTROL_PORT |= (1 << PLCD_RESET) | (1 << PLCD_CS) | (1 << PLCD_RD) | (1 << PLCD_WR) | (1 << PLCD_RS);
    PLCD_CONTROL_DDR |= (1 << PLCD_RESET) | (1 << PLCD_CS) | (1 << PLCD_RD) | (1 << PLCD_WR) | (1 << PLCD_RS);

    PLCD_DATA_DDR = 0xFF;

    _delay_ms(5);
    PLCD_DRIVE_RESET_LOW();
    _delay_ms(10);
    PLCD_DRIVE_RESET_HIGH();
    _delay_ms(20);



  Lcd_Write_Com_Data(0x0026,0x0084); //PT=10,GON=0, DTE=0, D=0100
  _delay_ms(40);
  Lcd_Write_Com_Data(0x0026,0x00B8); //PT=10,GON=1, DTE=1, D=1000
  _delay_ms(40);
  Lcd_Write_Com_Data(0x0026,0x00BC); //PT=10,GON=1, DTE=1, D=1100


  _delay_ms(20);                           // 080421
  // Lcd_Write_Com_Data(0x0001,0x0000);     // PTL='1' Enter Partail mode

  //Driving ability Setting
  Lcd_Write_Com_Data(0x0060,0x0000); //PTBA[15:8]
  Lcd_Write_Com_Data(0x0061,0x0006); //PTBA[7:0]
  Lcd_Write_Com_Data(0x0062,0x0000); //STBA[15:8]
  Lcd_Write_Com_Data(0x0063,0x00C8); //STBA[7:0]
  _delay_ms(20);                        //   080421

  //Gamma Setting
  Lcd_Write_Com_Data(0x0073,0x0070); //
  Lcd_Write_Com_Data(0x0040,0x0000); //
  Lcd_Write_Com_Data(0x0041,0x0040); //
  Lcd_Write_Com_Data(0x0042,0x0045); //
  Lcd_Write_Com_Data(0x0043,0x0001); //
  Lcd_Write_Com_Data(0x0044,0x0060); //
  Lcd_Write_Com_Data(0x0045,0x0005); //
  Lcd_Write_Com_Data(0x0046,0x000C); //
  Lcd_Write_Com_Data(0x0047,0x00D1); //
  Lcd_Write_Com_Data(0x0048,0x0005); //

  Lcd_Write_Com_Data(0x0050,0x0075); //
  Lcd_Write_Com_Data(0x0051,0x0001); //
  Lcd_Write_Com_Data(0x0052,0x0067); //
  Lcd_Write_Com_Data(0x0053,0x0014); //
  Lcd_Write_Com_Data(0x0054,0x00F2); //
  Lcd_Write_Com_Data(0x0055,0x0007); //
  Lcd_Write_Com_Data(0x0056,0x0003); //
  Lcd_Write_Com_Data(0x0057,0x0049); //
  _delay_ms(20);                          //     080421

  //Power Setting
  Lcd_Write_Com_Data(0x001F,0x0003); //VRH=4.65V     VREG1GAMMA 00~1E  080421
  Lcd_Write_Com_Data(0x0020,0x0000); //BT (VGH~15V,VGL~-12V,DDVDH~5V)
  Lcd_Write_Com_Data(0x0024,0x0024); //VCOMH(VCOM High voltage3.2V)     0024/12    080421    11~40
  Lcd_Write_Com_Data(0x0025,0x0034); //VCOML(VCOM Low voltage -1.2V)    0034/4A    080421    29~3F
  //****VCOM offset**///
  Lcd_Write_Com_Data(0x0023,0x002F); //VMF(no offset)
  _delay_ms(20);                          //  080421            10~39

  //##################################################################
  // Power Supply Setting
  Lcd_Write_Com_Data(0x0018,0x0044); //I/P_RADJ,N/P_RADJ Noraml mode 60Hz
  Lcd_Write_Com_Data(0x0021,0x0001); //OSC_EN='1' start osc
  Lcd_Write_Com_Data(0x0001,0x0000); //SLP='0' out sleep
  Lcd_Write_Com_Data(0x001C,0x0003); //AP=011
  Lcd_Write_Com_Data(0x0019,0x0006); // VOMG=1,PON=1, DK=0,
  _delay_ms(20);                          //  080421

  //##################################################################
  // Display ON Setting
  Lcd_Write_Com_Data(0x0026,0x0084); //PT=10,GON=0, DTE=0, D=0100
  _delay_ms(40);
  Lcd_Write_Com_Data(0x0026,0x00B8); //PT=10,GON=1, DTE=1, D=1000
  _delay_ms(40);
  Lcd_Write_Com_Data(0x0026,0x00BC); //PT=10,GON=1, DTE=1, D=1100
  _delay_ms(20);                    //  080421

  //SET GRAM AREA
  Lcd_Write_Com_Data(0x0002,0x0000);
  Lcd_Write_Com_Data(0x0003,0x0000);
  Lcd_Write_Com_Data(0x0004,0x0000);
  Lcd_Write_Com_Data(0x0005,0x00AF);
  Lcd_Write_Com_Data(0x0006,0x0000);
  Lcd_Write_Com_Data(0x0007,0x0000);
  Lcd_Write_Com_Data(0x0008,0x0000);
  Lcd_Write_Com_Data(0x0009,0x00DB);

  _delay_ms(20);                //080421
  Lcd_Write_Com_Data(0x0016,0x0008);  //MV MX MY ML SET  0028
  Lcd_Write_Com_Data(0x0005,0x00DB);  Lcd_Write_Com_Data(0x0009,0x00AF);
  Lcd_Write_Com_Data(0x0017,0x0005);//COLMOD Control Register (R17h)
    Lcd_Write_Com (0x0021);//LCD_WriteCMD(GRAMWR)


      Lcd_Write_Com(0x0022);

      Pant(0xf800);
      Pant(0X07E0);
      Pant(0x001f);

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
    // hardware reset (minimum 10us pulse)
    DDR_SPI |= (1 << DD_RES);
    _delay_ms(1);
    PORT_SPI |= (1 << DD_RES);

    // sleep out command can't be sent for 120ms after releasing Reset
    _delay_ms(120);
//------------------------------------------------------------------//
//-------------------Software Reset-------------------------------//
    write_command(0x11); //Sleep out

    // When IC is in Sleep In mode, it is necessary to wait 120msec before sending next command
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

    // column address set: XS = 0, XE = 127
    write_command(0x2A);
    write_data(0x00);
    //write_data(0x02);
    write_data(0x00);
    write_data(0x00);
    //write_data(0x81);
    write_data(0x7F);

    // row address set: YS = 0, YE = 159
    write_command(0x2B);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0x9F);
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

void WritePixel(uint16_t pixel)
{
    LCD_DataWrite(pixel/256, pixel);
}

#define ROW_OFFSET_L    0
#define COL_OFFSET_L    0


void SetupTile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH)
{
    unsigned int low, high;

    // setup tile boundaries
    // column address set with offset
    low = COL_OFFSET_L + tileX;
    high = COL_OFFSET_L + tileX + tileW - 1;
    write_command(0x2A);
    write_data(low/256);
    write_data(low);
    write_data(high/256);
    write_data(high);

    // row address set with offset
    low = ROW_OFFSET_L + tileY;
    high = ROW_OFFSET_L + tileY + tileH - 1;
    write_command(0x2B);
    write_data(low/256);
    write_data(low);
    write_data(high/256);
    write_data(high);

    // write tile
    write_command(0x2C); // memory write
}

void DrawRawTile(unsigned int pixelCount, unsigned char bytes_per_pixel, uint8_t pixelBuffer[])
{
    for (unsigned int i = 0; i < pixelCount * bytes_per_pixel; i+=bytes_per_pixel)
    {
            if(bytes_per_pixel == 1)
                Write8bitPixel(pixelBuffer[i]);
            else
                WritePixel(pixelBuffer[i] + pixelBuffer[i+1]*256);
    }
}

void DrawHextile(unsigned char tileW, unsigned char tileH, unsigned char bytes_per_pixel, uint8_t hextileBuffer[16][16*bytes_per_pixel])
{
    for (unsigned int j = 0; j < tileH; j++)
    {
        for (unsigned int i = 0; i < tileW * bytes_per_pixel; i += bytes_per_pixel)
        {
            if(bytes_per_pixel == 1)
                Write8bitPixel(hextileBuffer[j][i]);
            else
                WritePixel(hextileBuffer[j][i] + hextileBuffer[j][i+1]*256);
        }
    }
}

void VncDisplay_Init(void)
{
    ParallelDisplay_Init();


    SetupLcd();
    lcd_initial();
    ClearDisplay();
}

