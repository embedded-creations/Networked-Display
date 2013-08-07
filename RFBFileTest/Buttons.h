#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#define BUTTON_PORT         PORTF
#define BUTTON_PINS         PINF
#define BUTTON_1_PIN        PF1
#define BUTTON_2_PIN        PF4
#define BUTTON_3_PIN        PF5

void Buttons_Init(void);
void Buttons_Handler(void);

extern unsigned char keypress;

#endif
