#include "Buttons.h"
#include <avr/io.h>
#include "SpiLcd.h"

int oneshot = 0;

unsigned char keypress = 0;

void Buttons_Handler(void)
{
    // for debouncing, after a button press, ignore all other button presses until all buttons are released
    if(oneshot)
    {
        if(!(BUTTON_PINS & (1 << BUTTON_1_PIN)) || !(BUTTON_PINS & (1 << BUTTON_2_PIN)) || !(BUTTON_PINS & (1 << BUTTON_3_PIN)))
            return;
        else
            oneshot = 0;
    }
    if(!(BUTTON_PINS & (1 << BUTTON_1_PIN)))
    {
        oneshot = 1;
        keypress = 'a';
    }
    if(!(BUTTON_PINS & (1 << BUTTON_2_PIN)))
    {
        oneshot = 1;
        keypress = 'b';
    }
    if(!(BUTTON_PINS & (1 << BUTTON_3_PIN)))
    {
        oneshot = 1;
        keypress = 'c';
    }
}

void Buttons_Init(void)
{
    // enable pullups on buttons
    BUTTON_PORT |= (1 << BUTTON_1_PIN) | (1 << BUTTON_2_PIN) | (1 << BUTTON_3_PIN);
}
