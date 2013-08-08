#ifndef _HEXTILE_H_
#define _HEXTILE_H_

#include <stdint.h>
#include "SpiLcd.h"
#include "Debug.h"

#define BPP 16

#define MAX_TILE_SIZE   (16 * 16 * (BPP/8) + 1)

#define rfbHextileRaw           (1 << 0)
#define rfbHextileBackgroundSpecified   (1 << 1)
#define rfbHextileForegroundSpecified   (1 << 2)
#define rfbHextileAnySubrects       (1 << 3)
#define rfbHextileSubrectsColoured  (1 << 4)

#define rfbHextilePackXY(x,y) (((x) << 4) | (y))
#define rfbHextilePackWH(w,h) ((((w)-1) << 4) | ((h)-1))
#define rfbHextileExtractX(byte) ((byte) >> 4)
#define rfbHextileExtractY(byte) ((byte) & 0xf)
#define rfbHextileExtractW(byte) (((byte) >> 4) + 1)
#define rfbHextileExtractH(byte) (((byte) & 0xf) + 1)


void SetupHandleHextile(int rectx, int recty, int rectw, int recth);

unsigned int
HandleHextile16 (uint8_t * rfbBuffer, unsigned int buffersize);

#endif
