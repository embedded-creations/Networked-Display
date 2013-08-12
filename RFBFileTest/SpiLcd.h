#ifndef SPILCD_H_
#define SPILCD_H_

#include <stdint.h>

#define SPI_LCD_ENABLED     0


#if (SPI_LCD_ENABLED == 1)
#define LCD_BPP     8

#if (LCD_BPP == 16)
#   define CARDBPP uint16_t
#endif
#if (LCD_BPP == 8)
#   define CARDBPP uint8_t
#endif
#if (LCD_BPP == 1)
#   define CARDBPP uint8_t
#endif

void LcdInit(void);
void SetupTile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH);
void DrawHextile(unsigned char tileW, unsigned char tileH);
void DrawRawTile(unsigned int pixelCount, uint8_t pixelBuffer[]);
void FillSubRectangle(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned int pixel);
#endif

#endif /* SPILCD_H_ */
