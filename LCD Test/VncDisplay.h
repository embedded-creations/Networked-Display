#ifndef _VNCDISPLAY_H_
#define _VNCDISPLAY_H_


#define VNC_LCD_SPI         0
#define VNC_LCD_PARALLEL    1

#define VNC_LCD_SELECTION VNC_LCD_SPI


#if VNC_LCD_SELECTION == VNC_LCD_SPI
    #include "SpiLcd.h"
#else
    #include "ParallelLcd.h"
#endif

void VncDisplayInit(void);


#endif
