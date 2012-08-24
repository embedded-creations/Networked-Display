#include <avr/io.h>
#include <util/delay.h>

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

