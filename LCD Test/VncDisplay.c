#include "VncDisplay.h"


void VncDisplayInit(void)
{
#if VNC_LCD_SELECTION == VNC_LCD_PARALLEL
    ParallelDisplay_Init();
#else
    SpiLcd_Init();
#endif
}
