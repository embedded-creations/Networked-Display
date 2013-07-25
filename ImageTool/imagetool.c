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
#include <rfb/keysym.h>

#include <wand/magick_wand.h>

#include <stdio.h>

static const int bpp = 4;
static int maxx = 128, maxy = 160;

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

/* switch to new framebuffer contents */

static void newframebuffer (rfbScreenInfoPtr screen, int width, int height)
{
    unsigned char *oldfb, *newfb;

    maxx = width;
    maxy = height;
    oldfb = (unsigned char*)screen->frameBuffer;
    newfb = (unsigned char*)malloc(maxx * maxy * bpp);
    rfbNewFramebuffer(screen, (char*)newfb, maxx, maxy, 8, 3, bpp);
    free(oldfb);
}

static void initBuffer(unsigned char* buffer)
{
  int i,j;
  for(j=0;j<maxy;++j) {
    for(i=0;i<maxx;++i) {
      buffer[(j*maxx+i)*bpp+0]=(i+j)*128/(maxx+maxy); /* red */
      buffer[(j*maxx+i)*bpp+1]=i*128/maxx; /* green */
      buffer[(j*maxx+i)*bpp+2]=j*256/maxy; /* blue */
    }
    buffer[j*maxx*bpp+0]=0xff;
    buffer[j*maxx*bpp+1]=0xff;
    buffer[j*maxx*bpp+2]=0xff;
    buffer[j*maxx*bpp+3]=0xff;
  }
}

uint8_t savedkeypress = 0;

static void dokey(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    if(down) {
    if(key==XK_Escape)
      rfbCloseClient(cl);
    else if(key==XK_a || key==XK_b || key==XK_c || key==XK_r) {
      uint8_t keyPress = key;
      const char * filename = "c:\\vncinput.bin";
      FILE * ft = fopen(filename, "wb");
      if(ft)
      {
          fwrite(&keyPress, 1, 1, ft);
          fclose(ft);
      }
      else
          // the file is locked, record the press and write it to the file later
          savedkeypress = keyPress;
    }
  }
}

/* Initialization */

int runonce = 0;

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
    rfbScreen->kbdAddEvent = dokey;

    MagickWandGenesis();

    /* Create a wand */
    mw = NewMagickWand();

    /* Read the input image */
    MagickReadImage(mw,"input.bmp");


    /* initialize the server */
    rfbInitServer(rfbScreen);

    /* this is the non-blocking event loop; a background thread is started */
    rfbRunEventLoop(rfbScreen, -1, TRUE);
    fprintf(stderr, "Running background loop...\n");
    while (1)
    {
        sleep(5);

        // try again to open the file and write a keypress we captured in dokey() but was locked before
        if(savedkeypress)
        {
            const char * filename = "c:\\vncinput.bin";

            FILE * ft = fopen(filename, "wb");
            if(ft)
            {
                fwrite(&savedkeypress, 1, 1, ft);
                fclose(ft);
                savedkeypress = 0;
            }
        }

        const char * filename = "c:\\pixelbuffer.bin";
        unsigned char pixelbuffer[ maxx * maxy * bpp ];
        int byteswritten = 0;
        FILE * ft = fopen(filename, "rb");
        if (ft)
        {
            fread(pixelbuffer, 1, maxx * maxy * bpp, ft);

            if (!runonce)
            {
                runonce = 1;
                memcpy(rfbScreen->frameBuffer, pixelbuffer, maxx * maxy * bpp);
            }

            int modx0 = -1, modx1 = -1, mody0 = -1, mody1 = -1;
            int j, i;

            // find the smallest (single) rectangle that covers all the modified pixels
#if 1
            for (j = 0; j < maxy; j++)
            {
                for (i = 0; i < maxx; i++)
                {
                    if (pixelbuffer[ j * maxx * bpp + i * bpp + 0 ] != (unsigned char)rfbScreen->frameBuffer[ j * maxx * bpp + i * bpp + 0 ]
                     || pixelbuffer[ j * maxx * bpp + i * bpp + 1 ] != (unsigned char)rfbScreen->frameBuffer[ j * maxx * bpp + i * bpp + 1 ]
                     || pixelbuffer[ j * maxx * bpp + i * bpp + 2 ] != (unsigned char)rfbScreen->frameBuffer[ j * maxx * bpp + i * bpp + 2 ])
                    {
                        if (modx0 < 0 || i < modx0)
                            modx0 = i;
                        if (i > modx1)
                            modx1 = i + 2;
                        if (mody0 < 0 || j < mody0)
                            mody0 = j;
                        if (j > mody1)
                            mody1 = j + 2;

                    }
                }
            }
#endif
            // if there were any modified pixels, mark the rectangle
            if (modx0 >= 0)
            {
                memcpy(rfbScreen->frameBuffer, pixelbuffer, maxx * maxy * bpp);

                rfbMarkRectAsModified(rfbScreen, modx0, mody0, modx1, mody1);
            }
            fclose(ft);
        }
        else
        {
            printf("couldn't open file");
            return -1;
        }
    }

    free(rfbScreen->frameBuffer);
    rfbScreenCleanup(rfbScreen);

    if(mw) mw = DestroyMagickWand(mw);
    MagickWandTerminus();

    return (0);
}
