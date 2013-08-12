#ifndef _EINK_H_
#define _EINK_H_

#include <stdint.h>

#define LCD_BPP     1

#if (LCD_BPP == 1)
#   define CARDBPP uint8_t
#endif

void LcdInit(void);
void SetupTile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH);
void DrawHextile(unsigned char tileW, unsigned char tileH);
void DrawRawTile(unsigned int pixelCount, uint8_t pixelBuffer[]);
void FillSubRectangle(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned int pixel);

void clearScreen(void);
void refreshScreen(void);


#endif /* _EINK_H_ */
