#ifndef SPILCD_H_
#define SPILCD_H_

#include <stdint.h>

#define LCD_BPP     16

void LcdInit(void);
void SetupTile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH);
void DrawHextile(unsigned char tileW, unsigned char tileH);
void DrawRawTile(unsigned int pixelCount, uint8_t pixelBuffer[]);
void FillSubRectangle(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned int pixel);

#endif /* SPILCD_H_ */
