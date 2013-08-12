#include <avr/io.h>
#include <util/delay.h>



#include "eink.h"


#define DDR_SPI         DDRB
#define PORT_SPI        PORTB
#define DDR_EINK        DDRD
#define PORT_EINK       PORTD

#define DD_MOSI         PB2
#define DD_SCK          PB1
#define DD_SPI_SS       PB0 // not actually connected to the LCD, but needs to be pulled up (low signal on this line sets SPI to slave mode)

#define DD_EINK_CS1     PD6
#define DD_EINK_DC      PD5
#define DD_EINK_CS2     PD4

#define Eink_CS1_LOW  {DDR_EINK |= (1<<DD_EINK_CS1);PORT_EINK &=~ (1<<DD_EINK_CS1);}
#define Eink_CS1_HIGH {DDR_EINK |= (1<<DD_EINK_CS1);PORT_EINK |=  (1<<DD_EINK_CS1);}
#define Eink_DC_LOW   {DDR_EINK |= (1<<DD_EINK_DC); PORT_EINK &=~ (1<<DD_EINK_DC);}
#define Eink_DC_HIGH  {DDR_EINK |= (1<<DD_EINK_DC); PORT_EINK |=  (1<<DD_EINK_DC);}
#define GT_CS2_LOW    {DDR_EINK |= (1<<DD_EINK_CS2);PORT_EINK &=~ (1<<DD_EINK_CS2);}
#define GT_CS2_HIGH   {DDR_EINK |= (1<<DD_EINK_CS2);PORT_EINK |=  (1<<DD_EINK_CS2);}


 void clearScreen(void);
 void refreshScreen(void);

 void  writeComm(uint8_t Command);
 void  writeData(uint8_t data);

 void  initEink(void);
 void  closeBump(void);
 void  configureLUTRegister(void);
 void  setPositionXY(unsigned char Xs, unsigned char Xe,unsigned char Ys,unsigned char Ye);


void SPI_MasterTransmit (char cData)
{
    /* Start transmission */
    SPDR = cData;
    /* Wait for transmission complete */
    while (!(SPSR & (1 << SPIF)));
}

void writeComm (uint8_t c)
{
  Eink_CS1_HIGH;
  Eink_DC_LOW;
  Eink_CS1_LOW;

    SPI_MasterTransmit(c);
  Eink_CS1_HIGH;
}

void writeData (uint8_t d)
{
  Eink_CS1_HIGH;
  Eink_DC_HIGH;
  Eink_CS1_LOW;

    SPI_MasterTransmit(d);
  Eink_CS1_HIGH;
}



void initEink (void)
{
  writeComm(0x10);//exit deep sleep mode
  writeData(0x00);
  writeComm(0x11);//data enter mode
  writeData(0x03);
  writeComm(0x44);//set RAM x address start/end, in page 36
  writeData(0x00);//RAM x address start at 00h;
  writeData(0x11);//RAM x address end at 11h(17)->72: [because 1F(31)->128 and 12(18)->76]
  writeComm(0x45);//set RAM y address start/end, in page 37
  writeData(0x00);//RAM y address start at 00h;
  writeData(0xAB);//RAM y address start at ABh(171)->172: [because B3(179)->180]
  writeComm(0x4E);//set RAM x address count to 0;
  writeData(0x00);
  writeComm(0x4F);//set RAM y address count to 0;
  writeData(0x00);
  writeComm(0x21);//bypass RAM data
  writeData(0x03);
  writeComm(0xF0);//booster feedback used, in page 37
  writeData(0x1F);


  writeComm(0x2C);//vcom
  writeData(0xA0);
  writeComm(0x3C);//boarder waveform
  writeData(0x63);
  writeComm(0x22);//display updata sequence option ,in page 33
  writeData(0xC4);//enable sequence: clk -> CP -> LUT -> initial display -> pattern display

  configureLUTRegister();
}


void refreshScreen(void)
{
  writeComm(0x20); // Master Activation
  closeBump();
  _delay_ms(5000);
}

void clearScreen(void)
{
   int i;
   initEink();
   writeComm(0x24); // Write RAM
   for(i=0;i<3096;i++)
   {
     writeData(0xff);
   }
   _delay_ms(1000);
 }

void setPositionXY(unsigned char Xs, unsigned char Xe,unsigned char Ys,unsigned char Ye)
{
  writeComm(0x44);//set RAM x address start/end  command
  writeData(Xs);
  writeData(Xe);
  writeComm(0x45);//set RAM y address start/end  command
  writeData(Ys);
  writeData(Ye);
  writeComm(0x4E);//set RAM x address count to Xs;
  writeData(Xs);
  writeComm(0x4F);//set RAM y address count to Ys;
  writeData(Ys);
}

void closeBump(void)
{
  writeComm(0x22);   // display update
  writeData(0x03);  // disable CP, then disable clock signal
  writeComm(0x20);  // master activation
}

void configureLUTRegister(void)
{
  unsigned char i;
const unsigned char init_data[]={
    0x82,0x00,0x00,0x00,0xAA,0x00,0x00,0x00,
    0xAA,0xAA,0x00,0x00,0xAA,0xAA,0xAA,0x00,
    0x55,0xAA,0xAA,0x00,0x55,0x55,0x55,0x55,
    0xAA,0xAA,0xAA,0xAA,0x55,0x55,0x55,0x55,
    0xAA,0xAA,0xAA,0xAA,0x15,0x15,0x15,0x15,
    0x05,0x05,0x05,0x05,0x01,0x01,0x01,0x01,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x41,0x45,0xF1,0xFF,0x5F,0x55,0x01,0x00,
    0x00,0x00,};
  writeComm(0x32);  //write data to LUT register
  for(i=0;i<90;i++)
    writeData(init_data[i]);
}

#if 0
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
#endif

void SetupTile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH)
{
#if 0
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
#endif
}

static CARDBPP hextileBuffer[16][16];

void DrawRawTile(unsigned int pixelCount, uint8_t pixelBuffer[]) {
#if 0
    for (unsigned int i = 0; i < pixelCount; i++) {
#if (LCD_BPP == 16)
        WritePixel(pixelBuffer[i*2] + pixelBuffer[(i*2)+1]*256);
#endif
#if (LCD_BPP == 8)
        Write8bitPixel(pixelBuffer[i]);
#endif
#if (LCD_BPP == 1)
        uint16_t pixel = 0;
        if(pixelBuffer[i/8] & (1<<(i%8)))
            pixel = 0xffff;
        WritePixel(pixel);
#endif
    }
#endif
}


void DrawHextile(unsigned char tileW, unsigned char tileH) {
#if 0
    for (unsigned char j = 0; j < tileH; j++) {
        for (unsigned char i = 0; i < tileW; i++) {
#if (LCD_BPP == 16)
            WritePixel(hextileBuffer[j][i]);
#endif
#if (LCD_BPP == 8)
            Write8bitPixel(hextileBuffer[j][i]);
#endif
#if (LCD_BPP == 1)
            uint16_t pixel = 0;
            if(hextileBuffer[j][i])
                pixel = 0xffff;
            WritePixel(pixel);
#endif

        }
    }
#endif
}

extern int memafter;

void FillSubRectangle(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned int pixel) {
#if 0
    for (unsigned char j = y; j < y + h; j++) {
        for (unsigned char i = x; i < x + w; i++) {
            hextileBuffer[j][i] = pixel;
            memafter = freeMemory();
        }
    }
#endif
}


void EinkSetup(void)
{
    GT_CS2_HIGH;

    /* Set MOSI and SCK output, all others input */
    DDR_SPI |= (1 << DD_MOSI);
    DDR_SPI |= (1 << DD_SCK);

    PORT_SPI |= (1 << DD_SPI_SS); // make this pin pulled up to avoid low level during SPI transfer, which would set SPI to slave mode

    /* Enable SPI, Master */
    SPCR = (1 << SPE) | (1 << MSTR);

    // set SCK frequency = fosc/2
    //SPSR = (1 << SPI2X);
}


void LcdInit(void) {
    EinkSetup();

    clearScreen();
}




