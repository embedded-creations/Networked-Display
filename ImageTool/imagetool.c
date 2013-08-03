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
static int maxx = 160, maxy = 150;

/* Here we create a structure so that every client has it's own pointer */

typedef struct ClientData
{
    rfbBool oldButton;
    int oldx, oldy;
} ClientData;

static void clientgone (rfbClientPtr cl)
{
    free(cl->clientData);
}

static enum rfbNewClientAction newclient (rfbClientPtr cl)
{
    cl->clientData = (void*)calloc(sizeof(ClientData), 1);
    cl->clientGoneHook = clientgone;
    return RFB_CLIENT_ACCEPT;
}


int main (int argc, char** argv)
{
    MagickWand *mw = NULL;

    rfbScreenInfoPtr rfbScreen = rfbGetScreen(&argc, argv, maxx, maxy, 8, 3, bpp);
    if (!rfbScreen)
        return 0;
    rfbScreen->desktopName = "LibVNCServer Example";
    rfbScreen->frameBuffer = (char*)malloc(maxx * maxy * bpp);
    rfbScreen->alwaysShared = TRUE;
    rfbScreen->newClientHook = newclient;

    MagickWandGenesis();

    /* Create a wand */
    mw = NewMagickWand();

    /* Read the input image */
    MagickReadImage(mw,"input.bmp");

    //FILE * ft = fopen(filename, "rb");
    MagickExportImagePixels(mw,0,0,160,150,"RGBA",CharPixel,rfbScreen->frameBuffer);

    /* initialize the server */
    rfbInitServer(rfbScreen);

    /* this is the non-blocking event loop; a background thread is started */
    rfbRunEventLoop(rfbScreen, -1, TRUE);
    fprintf(stderr, "Running background loop...\n");
    while (1)
    {
        sleep(5);
    }

    free(rfbScreen->frameBuffer);
    rfbScreenCleanup(rfbScreen);

    if(mw) mw = DestroyMagickWand(mw);
    MagickWandTerminus();

    return (0);
}
