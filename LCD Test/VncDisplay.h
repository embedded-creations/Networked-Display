#ifndef _VNCDISPLAY_H_
#define _VNCDISPLAY_H_

#include "SpiLcd.h"
#include "ParallelLcd.h"

#define VNC_LCD_SPI         0
#define VNC_LCD_PARALLEL    1

#define VNC_LCD_SELECTION VNC_LCD_SPI


void VncDisplayInit(void);


#endif
