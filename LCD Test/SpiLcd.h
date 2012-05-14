#ifndef SPILCD_H_
#define SPILCD_H_

void SetupLcd(void);
void lcd_initial (void);
void ClearDisplay(void);
void DrawHextile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH, unsigned char bytes_per_pixel, uint8_t hextileBuffer[16][16*bytes_per_pixel]);
void DrawRawTile(unsigned int pixelCount, unsigned char bytes_per_pixel, uint8_t pixelBuffer[]);
void SetupTile(unsigned int tileX, unsigned int tileY, unsigned char tileW, unsigned char tileH);

#endif /* SPILCD_H_ */
