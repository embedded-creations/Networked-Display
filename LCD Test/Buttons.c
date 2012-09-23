#include "Buttons.h"
#include <avr/io.h>
#include "SpiLcd.h"

int oneshot = 0;

void Buttons_Handler(void)
{
    if(!(BUTTON_PINS & (1 << BUTTON_1_PIN)))
    {
        if(!oneshot)
        {
            oneshot = 1;

            dsp_single_colour(0xAA, 0xAA);
        }
    }
}

void Buttons_Init(void)
{
    // enable pullups on buttons
    BUTTON_PORT |= (1 << BUTTON_1_PIN);
}
