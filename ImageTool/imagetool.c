/**
 * @example example.c
 * This is an example of how to use libvncserver.
 *
 * libvncserver example
 * Copyright (C) 2001 Johannes E. Schindelin <Johannes.Schindelin@gmx.de>
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

#ifdef WIN32
#define sleep Sleep
#include <Windows.h>
#else
#include <unistd.h>
#endif

#ifdef __IRIX__
#include <netdb.h>
#endif

#include <rfb/rfb.h>

#include <wand/magick_wand.h>

#include <stdio.h>

static const int bpp = 4;
static int maxx, maxy;



int main (int argc, char** argv)
{
    // open input file,
    MagickWand *mw = NULL;
    MagickWandGenesis();
    mw = NewMagickWand();
    MagickReadImage(mw,"input.bmp");
    maxx = MagickGetImageWidth(mw);
    maxy = MagickGetImageHeight(mw);

    rfbScreenInfoPtr rfbScreen = rfbGetScreen(&argc, argv, maxx, maxy, 8, 3, bpp);
    if (!rfbScreen)
        return 0;
    rfbScreen->desktopName = "LibVNCServer Example";
    rfbScreen->frameBuffer = (char*)malloc(maxx * maxy * bpp);
    rfbScreen->alwaysShared = TRUE;

    MagickExportImagePixels(mw,0,0,maxx,maxy,"RGBA",CharPixel,rfbScreen->frameBuffer);

    /* initialize the server */
    rfbInitServer(rfbScreen);

#if 1
    rfbClientPtr cl;

    cl = (rfbClientPtr)calloc(sizeof(rfbClientRec),1);

    cl->screen = rfbScreen;

    /* setup pseudo scaling */
    cl->scaledScreen = rfbScreen;
    cl->scaledScreen->scaledScreenRefCount++;

    // magic number to tell server to treat this special - don't send data just clear socket
    cl->sock = -99;

#if 0
    // setup client with 16bpp true color
    cl->format.bitsPerPixel = 16;
    cl->format.depth = 16;
    cl->format.redMax = 15;
    cl->format.greenMax = 15;
    cl->format.blueMax = 15;
    cl->format.redShift = 4;
    cl->format.greenShift = 0;
    cl->format.blueShift = 12;
#endif
#if 0
    // setup client with 8bpp true color
    cl->format.bitsPerPixel = 8;
    cl->format.depth = 8;
    cl->format.redMax = 3;
    cl->format.greenMax = 3;
    cl->format.blueMax = 3;
    cl->format.redShift = 6;
    cl->format.greenShift = 4;
    cl->format.blueShift = 2;
#endif

#if 1
    // setup client with 1bpp
    cl->format.bitsPerPixel = 8;
    cl->format.depth = 1;
    cl->format.redMax = 1;
    cl->format.greenMax = 1;
    cl->format.blueMax = 1;
    cl->format.redShift = 0;
    cl->format.greenShift = 0;
    cl->format.blueShift = 0;
#endif

    cl->format.trueColour = TRUE;
    cl->format.bigEndian = FALSE;
    cl->readyForSetColourMapEntries = TRUE;
    cl->screen->setTranslateFunction(cl);

    rfbResetStats(cl);

    rfbSendRectEncodingHextile(cl, 0, 0, maxx, maxy);

    rfbPrintStats(cl);

    // print number of bytes stored to buffer (should be equal to value in Stats)
    fprintf(stderr, "ublen = %d\n", cl->ublen);

#endif


    /* this is the non-blocking event loop; a background thread is started */
    // if we want to, let a client connect for comparison to offline encoding
#if 1
    rfbRunEventLoop(rfbScreen, -1, TRUE);
    fprintf(stderr, "Running background loop...\n");
    while (1)
    {
        sleep(5);
    }
#endif

    free(rfbScreen->frameBuffer);
    rfbScreenCleanup(rfbScreen);

    if(mw) mw = DestroyMagickWand(mw);
    MagickWandTerminus();

    return (0);
}
