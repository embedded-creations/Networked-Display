// read first couple bytes of stream or image into buffer.
// from that, get hextile encoding, position, width, height, save in globals
// read more of image: Read full image, or at least the max size of a tile
// call processhextile, which will read all the full tiles available, write them to the hextile buffer, and call a function to draw them to the screen
// when processhextile returns, it needs more data or is done
// read more data in (minimum of max size of a tile or full image), repeat until done

// in processhextile function, first check to see if the data for a full tile is available
// peek at the subencoding-mask byte
// if raw: read in width*height values
// add one for each of bg/fg if specified
// if anysubrects, add one and peek at number of subrects values
// if subrectscolored, add BPP to subrectsize
// multiply numsubrects by subrectsize, add to totalsize

// can skip the peeking and save a few cycles if the full image is known to be available

// CopyRect is really DrawRawTile
// StartSubRectangles (pass in rectangle bounds)
// Fill(Sub)Rectangle (draw single or one out of a series of rectangles)
// FinishSubRectangles (if needed, push subrectangles to screen)



/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * hextile.c - handle hextile encoding.
 *
 * This file shouldn't be compiled directly.  It is included multiple times by
 * rfbproto.c, each time with a different definition of the macro BPP.  For
 * each value of BPP, this file defines a function which handles a hextile
 * encoded rectangle with BPP bits per pixel.
 */

#include "hextile.h"
#include <avr/io.h>


#if (HEXTILE_BPP == 16)
#   define CARDBPP uint16_t
#   define GET_PIXEL(pix, ptr) (((uint8_t*)&(pix))[0] = *(ptr), \
                   ((uint8_t*)&(pix))[1] = *(ptr+1))
#endif
#if (HEXTILE_BPP == 8)
#   define CARDBPP uint8_t
#   define GET_PIXEL(pix, ptr) (((uint8_t*)&(pix))[0] = *(ptr))
#endif
#if (HEXTILE_BPP == 1)
#   define CARDBPP uint8_t
#   define GET_PIXEL(pix, ptr) (((uint8_t*)&(pix))[0] = *(ptr))
#endif

static int x, y, w, h;
static int rx, ry, rw, rh;
static CARDBPP bg, fg;

void SetupHandleHextile(int rectx, int recty, int rectw, int recth) {
    rx = rectx;
    ry = recty;
    rw = rectw;
    rh = recth;

    x = rx;
    y = ry;
}


unsigned int
HandleHextile (uint8_t * rfbBuffer, unsigned int buffersize)
{
    uint8_t *ptr;
    int sx, sy, sw, sh;
    uint8_t subencoding;
    uint8_t nSubrects;
    unsigned int readLength;

    unsigned int progress = 0;

    for (y = y; y < ry+rh; y += 16, x=rx) {
        for (x = x; x < rx+rw; x += 16) {
            w = h = 16;
            if (rx+rw - x < 16)
                w = rx+rw - x;
            if (ry+rh - y < 16)
                h = ry+rh - y;

            nSubrects = 0;

            if(buffersize-progress < 1)
                return progress;

#if 0
            DEBUG_PRINTSTRING("Progress=");
            DEBUG_PRINTHEX(progress/256);
            DEBUG_PRINTHEX(progress);
#endif

            subencoding = rfbBuffer[progress];

#if 0
            DEBUG_PRINTSTRING("Subenc=");
            DEBUG_PRINTHEX(subencoding);
#endif

            // calculate length here:
            // start with subencoding byte
            readLength = 1;

            if(subencoding & rfbHextileRaw) {
#if HEXTILE_BPP >= 8
                readLength += w * h * (HEXTILE_BPP / 8);
#else
                readLength += w * h / 8;
                if(w*h % 8)
                    readLength++;
#endif
                if(buffersize-progress < readLength)
                    return progress;

                SetupTile(x,y,w,h);
                DrawRawTile(w*h, rfbBuffer + progress + 1);

                progress += readLength;
                continue;
            } else {
                if(subencoding & rfbHextileBackgroundSpecified) {
                    if(buffersize-progress < readLength + sizeof(bg)) {
                        return progress;
                    }

                    GET_PIXEL(bg, rfbBuffer + progress + readLength);
                    readLength += sizeof(bg);
                }

                if(subencoding & rfbHextileForegroundSpecified) {
                    if(buffersize-progress < readLength + sizeof(fg)) {
                        return progress;
                    }

                    GET_PIXEL(fg, rfbBuffer + progress + readLength);
                    readLength += sizeof(fg);
                }

                if (subencoding & rfbHextileAnySubrects) {
                    if(buffersize-progress < readLength) {
                        return progress;
                    }

                    nSubrects = rfbBuffer[progress + readLength];

                    readLength += 1;

#if 0
                    DEBUG_PRINTSTRING("nSubrects=");
                    DEBUG_PRINTHEX(nSubrects);
#endif

                    ptr = rfbBuffer + progress + readLength;

                    if (subencoding & rfbHextileSubrectsColoured)
                        readLength += nSubrects * (2 + (HEXTILE_BPP / 8));
                    else
                        readLength += nSubrects * 2;
                }

                if(buffersize-progress < readLength)
                    return progress;

#if 0
                DEBUG_PRINTSTRING("readLen=");
                DEBUG_PRINTHEX(readLength/256);
                DEBUG_PRINTHEX(readLength);
#endif

                DEBUG_SET_GPIO0_HIGH();

                SetupTile(x,y,w,h);
                FillSubRectangle(0,0,w,h,bg);

                DEBUG_SET_GPIO0_LOW();

                int i;

                for (i = 0; i < nSubrects; i++) {
                    DEBUG_SET_GPIO1_HIGH();

                    if (subencoding & rfbHextileSubrectsColoured) {
                        GET_PIXEL(fg, ptr);
                        ptr += sizeof(fg);
                    }

                    sx = rfbHextileExtractX(*ptr);
                    sy = rfbHextileExtractY(*ptr);
                    ptr++;
                    sw = rfbHextileExtractW(*ptr);
                    sh = rfbHextileExtractH(*ptr);
                    ptr++;

                    FillSubRectangle(sx, sy, sw, sh, fg);

                    DEBUG_SET_GPIO1_LOW();
                }
                DrawHextile(w,h);

                progress += readLength;
                continue;
            }
        }
    }

    return progress;
}

#undef CARDBPP
