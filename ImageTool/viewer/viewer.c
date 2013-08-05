/**
 * @example ppmtest.c
 * A simple example of an RFB client
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <rfb/rfbclient.h>
#include <wand/magick_wand.h>

    #define ThrowWandException(wand) \
    { \
      char \
        *description; \
     \
      ExceptionType \
        severity; \
     \
      description=MagickGetException(wand,&severity); \
      (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
      description=(char *) MagickRelinquishMemory(description); \
      exit(-1); \
    }


static void PrintRect(rfbClient* client, int x, int y, int w, int h) {
    rfbClientLog("Received an update for %d,%d,%d,%d.\n",x,y,w,h);
}


static void SaveFramebufferAsImage(rfbClient* client, int x, int y, int w, int h) {
    MagickWand *wand;
    MagickBooleanType status;
    PixelWand * background = NewPixelWand();

    MagickWandGenesis();
    wand=NewMagickWand();

    status = MagickNewImage(wand, client->width, client->height, background);
    if (status == MagickFalse)
        ThrowWandException(wand);

    status = MagickImportImagePixels(wand,0,0,client->width,client->height,"RGBO",CharPixel,client->frameBuffer);
    if (status == MagickFalse)
        ThrowWandException(wand);

    status = MagickWriteImage(wand, "output.png");

    if (status == MagickFalse)
        ThrowWandException(wand);

    if(wand) wand = DestroyMagickWand(wand);
    MagickWandTerminus();
}

int
main(int argc, char **argv)
{
    rfbClient* client = rfbGetClient(8,3,4);
    time_t t=time(NULL);

    if(argc>1 && !strcmp("-print",argv[1])) {
        client->GotFrameBufferUpdate = PrintRect;
        argv[1]=argv[0]; argv++; argc--;
    } else
        client->GotFrameBufferUpdate = SaveFramebufferAsImage;

    /* The -listen option is used to make us a daemon process which listens for
       incoming connections from servers, rather than actively connecting to a
       given server. The -tunnel and -via options are useful to create
       connections tunneled via SSH port forwarding. We must test for the
       -listen option before invoking any Xt functions - this is because we use
       forking, and Xt doesn't seem to cope with forking very well. For -listen
       option, when a successful incoming connection has been accepted,
       listenForIncomingConnections() returns, setting the listenSpecified
       flag. */

    if (!rfbInitClient(client,&argc,argv))
        return 1;

    /* TODO: better wait for update completion */
    while (time(NULL)-t<5) {
        static int i=0;
        fprintf(stderr,"\r%d",i++);
        if(WaitForMessage(client,50)<0)
            break;
        if(!HandleRFBServerMessage(client))
            break;
    }

    rfbClientCleanup(client);

    return 0;
}

