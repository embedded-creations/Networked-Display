#ifndef SPILCD_H_
#define SPILCD_H_

void SetupLcd(void);
void lcd_initial (void);
void ClearDisplay(void);
void DrawHextile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH, void * hextileBuffer);



#endif /* SPILCD_H_ */
