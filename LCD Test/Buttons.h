#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#define BUTTON_PORT         PORTD
#define BUTTON_PINS         PIND
#define BUTTON_1_PIN        PD0

void Buttons_Init(void);
void Buttons_Handler(void);


#endif
