/*
 *  microVNC - An 8-bit VNC Client for Embedded Systems
 *  Version 0.1 Copyright (C) 2002 Louis Beaudoin
 *
 *  http://www.embedded-creations.com
 *
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
 *
 */


#include "vnc.h"
#include "SpiLcd.h"

#include "USBtoSerial.h"



enum {  VNCSTATE_DEAD,
        VNCSTATE_AUTHFAILED,
        VNCSTATE_NOTCONNECTED,
        VNCSTATE_WAITFORAUTHTYPE,
        VNCSTATE_WAITFORVNCAUTHWORD,
        VNCSTATE_WAITFORVNCAUTHRESPONSE,
        VNCSTATE_SENTCLIENTINITMESSAGE,
        VNCSTATE_WAITFORSERVERNAME,
        VNCSTATE_CONNECTED,
        VNCSTATE_CONNECTED_REFRESH,
        VNCSTATE_WAITFORUPDATE,
        VNCSTATE_PROCESSINGUPDATE,
        VNCSTATE_PROCESSDONE,
          };

enum {  VNCSENDSTATE_INIT,
        VNCSENDSTATE_VERSIONSENT,
        VNCSENDSTATE_AUTHWORDSENT,
        VNCSENDSTATE_CLIENTINITMESSAGESENT,
        VNCSENDSTATE_PIXELFORMATSENT,
        VNCSENDSTATE_ENCODINGTYPESENT,
        VNCSENDSTATE_PARTIALREFRESHSENT
          };

enum {  VNCPPSTATE_IDLE,
        VNCPPSTATE_OUTBOUNDS,
        VNCPPSTATE_HEXTILE,
        VNCPPSTATE_COPYRECT
          };

enum {  HEXTILESTATE_INIT,
        HEXTILESTATE_DONE,
        HEXTILESTATE_PARSEHEADER,
        HEXTILESTATE_DRAWRAW,
        HEXTILESTATE_DROPRAW,
        HEXTILESTATE_FILLBUFFER,
        HEXTILESTATE_DRAWTILE
          };

// masked used to decode the hextile subencoding byte
#define HEXTILE_RAW_MASK           0x01
#define HEXTILE_BACKGROUND_MASK    0x02
#define HEXTILE_FOREGROUND_MASK    0x04
#define HEXTILE_SUBRECT_MASK       0x08
#define HEXTILE_SUBRECTCOLOR_MASK  0x10




// temporary buffer for sending TCP data to the VNC server
unsigned char messageBuffer[MAXIMUM_VNCMESSAGE_SIZE];

/*****************************************************************************
*
*  Constant messages for communicating with the VNC server
*
*****************************************************************************/

prog_uchar versionMessage[] = "RFB 003.003\n";

// to get around the WinVNC requirement of using encryption, the WinVNC source
// was modified to use the fixed key {23,82,107,6,35,78,88,7} instead of a
// random key, so the result will always be the same when using the same
// password.  The word below corresponds to a password of "asdf"
prog_uchar VNCauthWord[] =
    { 0x54, 0x44, 0xC7, 0x4E, 0xDE, 0xDC, 0x8C, 0xDF,
      0x97, 0xB6, 0x51, 0x93, 0x04, 0xD9, 0x9E, 0x90 };

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   160

#define BYTES_PER_PIXEL  2
prog_uchar pixelFormatMessage[] = VNC_PIXEL_FORMAT_MESSAGE_16BIT;

// partial refresh request
prog_char refreshMessage[] = { 0x03,0x01,0,0,0,0,SCREEN_WIDTH/256,SCREEN_WIDTH,SCREEN_HEIGHT/256,SCREEN_HEIGHT };


// requests hextile encoding only
prog_uchar encodingTypeMessage[] = VNC_ENCODING_TYPE_HEXTILE_AND_COPYRECT;





// counters used to keep track of how many pixels and rectangles to go before
// the frameupdate is complete
static unsigned int rectangleCount;
static unsigned long interPixelCount;

// specifies the locally displayed portion of the server's display
// X0,Y0 = upper-right coordinate, X1,Y1 equal last displayed column,row + 1
static unsigned int windowX0 = 0;
static unsigned int windowY0 = 0;
static unsigned int windowX1 = SCREEN_WIDTH;
static unsigned int windowY1 = SCREEN_HEIGHT;

// specifies the coordinates of the current frameBufferUpdate rectangle
// X0,Y0 = upper-right coordinate, X1,Y1 equal last column,row + 1
static unsigned int updateWindowX0;
static unsigned int updateWindowY0;
static unsigned int updateWindowX1;
static unsigned int updateWindowY1;


// VNCstate is used by the processNewData and sendMessage functions to
// determine when to send message while connecting to a VNC server, and when
// to send refresh requests once connected
static unsigned char VNCstate = VNCSTATE_NOTCONNECTED;

// VNCsendstate is incremented when a message is sent to the VNC server.
// If a message needs to be retransmitted, decrement VNCstate and call the
// sendMessage function
static unsigned char VNCsendState = VNCSENDSTATE_INIT;

// the pixelProcessor is in the IDLE state when waiting for a rectangle to
// process
static unsigned char VNCpixelProcessorState;

// indicates the current state of the hextile processing function
static unsigned char hextileState;



// dataPtr points to the current location in the incoming data buffer to read
static unsigned char * dataPtr;
// dataSize indicates how many bytes are left in the incoming data buffer
static unsigned int dataSize = 0;


void Vnc_ResetSystem(void)
{
    VNCstate = VNCSTATE_NOTCONNECTED;
    VNCsendState = VNCSENDSTATE_INIT;
}

//  removes the amount of pixels specified by interPixelCount from the buffer
//  or as many as are currently in the buffer
// returns 0 if not enough data in buffer to process
int dropOutOfViewPixels(void)
{
    // clear the rest of the pixels out of the receive buffer
    if( (dataSize >= interPixelCount) )
    {
        dataPtr += interPixelCount;
        dataSize -= interPixelCount;
        interPixelCount = 0;
        return 1;
    }

    // clear out the receive buffer - more pixels in transit
    if( (interPixelCount > dataSize) && (dataSize != 0) )
    {
        dataPtr += dataSize;
        interPixelCount-=dataSize;
        dataSize = 0;
    }

    return 0;
}


// returns 0 if not enough data in buffer to process
int processCopyRect(void)
{
    // wait for both copyrect coordinates
    if( dataSize < 4 )
      return 0;

    dataSize -= 4;
    dataPtr += 4;

    VNCpixelProcessorState = VNCPPSTATE_IDLE;

    return 1;
}


// function used to receive and draw hextile updates to the screen
// this definitely needs to be optimized and probably separated into smaller
// sub-functions to reduce the stack usage, but for now, it works...
// Any tile that only partially overlaps the local display window will be
// dropped
// returns 0 if not enough data in buffer to process, or processing is complete
int processHextile(void)
{
  // tile x,y w,h
  static unsigned int tileX, tileY;
  static unsigned char tileW, tileH;
  
  static unsigned int background, foreground;
  
  static unsigned char subencodingByte;
  
  static unsigned char numSubrects;
  
  unsigned char temp1, temp2, temp3;

  unsigned int rectangleColor;
  
  unsigned char i,j;
  
  unsigned char x0, y0, x1, y1;
  
  // temporary 16x16 pixel buffer
  static unsigned char hextileBuffer[16][16 * BYTES_PER_PIXEL];
  
  switch( hextileState )
  {
    case HEXTILESTATE_INIT:
    
      // set the current x,y
      tileX = updateWindowX0 ;
      tileY = updateWindowY0 ;
      
      // set the tile x,y width, height
      tileW = updateWindowX1 < updateWindowX0+16 ?
                updateWindowX1-updateWindowX0 : 16;
      tileH = updateWindowY1 < updateWindowY0+16 ?
                updateWindowY1-updateWindowY0 : 16;
      
      // go to parseheader
      hextileState = HEXTILESTATE_PARSEHEADER ;
      return 1;
    
    case HEXTILESTATE_DONE:
      // increment the tile x,y
      tileX += tileW;
           
      // if next is the end column, set the width<16
      if( tileX == updateWindowX1 )
      {
        // if the last tile has been drawn - exit
        if( tileY+tileH == updateWindowY1 )
        {
          VNCpixelProcessorState = VNCPPSTATE_IDLE;
          hextileState = HEXTILESTATE_INIT;
          return 0;
        }
        
        tileX = updateWindowX0;
        tileY += tileH;
        tileH = updateWindowY1 < tileY+16 ? updateWindowY1-tileY : 16;
      }
      
      // if the next tile is the last column, set the width to < 16
      tileW = updateWindowX1 < tileX+16 ? updateWindowX1-tileX : 16;
      
      hextileState = HEXTILESTATE_PARSEHEADER;
      return 1;
    
    case HEXTILESTATE_PARSEHEADER:
      // wait for subencoding byte
      if( dataSize < 1 )
        return 0;
        
      subencodingByte = *dataPtr++;
      dataSize--;
      
      temp3 = 0;
      
      if( subencodingByte & HEXTILE_BACKGROUND_MASK )
        temp3 += BYTES_PER_PIXEL;
        
      if( subencodingByte & HEXTILE_FOREGROUND_MASK )
          temp3 += BYTES_PER_PIXEL;
        
      if( subencodingByte & HEXTILE_SUBRECT_MASK )
          temp3 += BYTES_PER_PIXEL;
        
      // if the optional bytes are not in the buffer, exit
      if( dataSize < temp3 )
      {
        dataPtr--;
        dataSize++;
        return 0;
      }
      
      // if raw bit, set state to raw and goto drawraw
      if( subencodingByte & HEXTILE_RAW_MASK )
      {
        if( tileX < windowX0 || (tileX + tileW) > windowX1 || tileY < windowY0
            || (tileY + tileH) > windowY1 )
        {
          interPixelCount = tileW * tileH * BYTES_PER_PIXEL;
          hextileState = HEXTILESTATE_DROPRAW;
          return 1;
        }
        
        interPixelCount = tileW * tileH * BYTES_PER_PIXEL;

        SetupTile(tileX, tileY, tileW, tileH);

        hextileState = HEXTILESTATE_DRAWRAW;
        return 1;
      }
      
      // else - if background specified read in pixel value
      if( subencodingByte & HEXTILE_BACKGROUND_MASK )
      {
        background = *dataPtr++;
        if(BYTES_PER_PIXEL == 2)
            background |= (*dataPtr++)*256;
        dataSize -= BYTES_PER_PIXEL;
      }
    
      if( subencodingByte & HEXTILE_FOREGROUND_MASK )
      {
        foreground = *dataPtr++;
          if(BYTES_PER_PIXEL == 2)
              foreground |= (*dataPtr++)*256;
          dataSize -= BYTES_PER_PIXEL;
      }

      if( subencodingByte & HEXTILE_SUBRECT_MASK )
      {
        //TransmitByte('S');
        numSubrects = *dataPtr++;
        dataSize--;
      }
      else numSubrects = 0;

      // fill the hextile buffer with the background
        for (j = 0; j < 16; j++)
        {
            for (i = 0; i < 16 * BYTES_PER_PIXEL; i+=BYTES_PER_PIXEL)

            {
                hextileBuffer[ j ][ i ] = background;
                if(BYTES_PER_PIXEL == 2)
                    hextileBuffer[ j ][ i+1 ] = background/256;
            }
        }
      
      hextileState = HEXTILESTATE_FILLBUFFER ;
      
      return 1;
    
    case HEXTILESTATE_DROPRAW:
      if(!dropOutOfViewPixels())
          return 0;
      
      if( interPixelCount == 0)
      {
        hextileState = HEXTILESTATE_DONE;
        return 1;
      }
        
      break;


    case HEXTILESTATE_DRAWRAW:
    if(interPixelCount > dataSize)
    {
        int bytesToDraw = dataSize;
        if(dataSize % 2)
            bytesToDraw -= 1;
        DrawRawTile(bytesToDraw, BYTES_PER_PIXEL, dataPtr);
        interPixelCount -= bytesToDraw;
        dataPtr += bytesToDraw;
        dataSize -= bytesToDraw;
    }
    else
    {
        DrawRawTile(interPixelCount, BYTES_PER_PIXEL, dataPtr);
        dataPtr += interPixelCount;
        dataSize -= interPixelCount;
        interPixelCount = 0;

        hextileState = HEXTILESTATE_DONE;
        return 1;
    }

      return 0;

    case HEXTILESTATE_FILLBUFFER:
      // draw all the subrectangles to the buffer
      while( numSubrects != 0 )
      {
        rectangleColor = foreground;
      
        // is subrect in buffer (return if not)
        if( subencodingByte & HEXTILE_SUBRECTCOLOR_MASK )
        {
          if( dataSize < 2 + BYTES_PER_PIXEL )
            return 0;
          
          // subrect color;
          rectangleColor = dataPtr[0];
          if(BYTES_PER_PIXEL == 2)
              rectangleColor |= dataPtr[1] * 256;
          
          dataSize-=BYTES_PER_PIXEL;
          dataPtr+=BYTES_PER_PIXEL;
        }
        else if( dataSize < 2 )
          return 0;

        
        // read in a subrectangle (with pixelvalue?)

        // x,y offset
        temp1 = dataPtr[0];
      
        // width/height
        temp2 = dataPtr[1];
      
        dataSize -= 2;
        dataPtr += 2;
      
      
        // rectangle starts at x0,y0 and stops at x1,y1 including the stop row/col
        x0 = (temp1 & 0xF0) >> 4;
        x1 = x0 + ((temp2 & 0xF0) >> 4);
      
        y0 = (temp1 & 0x0F);
        y1 = y0 + (temp2 & 0x0F);
      

        for (j = y0; j <= y1; j++)
        {
            for (i = x0 * BYTES_PER_PIXEL; i <= x1 * BYTES_PER_PIXEL; i+=BYTES_PER_PIXEL)
            {
                hextileBuffer[ j ][ i ] = rectangleColor;
                if(BYTES_PER_PIXEL == 2)
                    hextileBuffer[ j ][ i+1 ] = rectangleColor / 256;
            }
        }

        numSubrects--;
     }
   
     // state = drawtile - continue
     hextileState = HEXTILESTATE_DRAWTILE ;
     return 1;
    
  case HEXTILESTATE_DRAWTILE:
        // only draw the buffer if it fits cleanly into the currentWindow
        if( tileX < windowX0 || (tileX + tileW) > windowX1 || tileY < windowY0
        || (tileY + tileH) > windowY1 )
        {
            TransmitString("outofbounds ");
            hextileState = HEXTILESTATE_DONE ;
            return 1;
        }

        DrawHextile(tileX, tileY, tileW, tileH, BYTES_PER_PIXEL, hextileBuffer);

        hextileState = HEXTILESTATE_DONE ;
        return 1;

    default:
      break;

    }
  return 1;
}




// this function is called immediately after a VNC FramebufferUpdate header is
// received.  The function continues until all the rectangles contained in the
// FramebufferUpdate group have been processed and drawn or discarded
// returns 0 if not enough data in buffer to process, or processing is complete
int pixelProcessor(void)
{
    switch(VNCpixelProcessorState)
    {
        case VNCPPSTATE_IDLE:
            // done? - quit
            if( !(rectangleCount) )
            {
                VNCstate = VNCSTATE_CONNECTED_REFRESH;
                return 0;
            }

            // read rectangle header
            if( dataSize < 12 )
                return 0;

            rectangleCount--;

            updateWindowX0 = (dataPtr[0]<<8) + dataPtr[1];
            updateWindowX1 = updateWindowX0 + (dataPtr[4]<<8) + dataPtr[5];

            updateWindowY0 = (dataPtr[2]<<8) + dataPtr[3];
            updateWindowY1 = updateWindowY0 + (dataPtr[6]<<8) + dataPtr[7];

            // check for bad encoding
            if( dataPtr[8] != 0x00  || dataPtr[9] != 0x00 ||
                dataPtr[10] != 0x00 )
            {
              TransmitByte('R');
              return 0;
            }

            // get the encoding type
            switch( dataPtr[11] )
            {
              case 0:  // raw
                  // if anything raw is received, drop all the pixels and continue
                  // number of pixels = width * height
                  interPixelCount = ((dataPtr[4]<<8) + dataPtr[5]) ;
                  interPixelCount *= ((dataPtr[6]<<8) + dataPtr[7]);

                  dataPtr += 12;
                  dataSize -= 12;

                  VNCpixelProcessorState = VNCPPSTATE_OUTBOUNDS ;
                  return 1;

              case 1:  // CopyRect
                  dataPtr += 12;
                  dataSize -= 12;
                  VNCpixelProcessorState = VNCPPSTATE_COPYRECT;

                  TransmitString("#$copyrect$#");

                  return 1;

              case 5:  // hextile
                  dataPtr += 12;
                  dataSize -= 12;

                  VNCpixelProcessorState = VNCPPSTATE_HEXTILE;
                  hextileState = HEXTILESTATE_INIT;

                  return 1;

              default:
                TransmitByte('R');
                return 0;
            }
            break;


        case VNCPPSTATE_OUTBOUNDS:
            // drop all pixels in the current rectangle
            if(!dropOutOfViewPixels())
                return 0;

            // when finished, go back to the idle state
            if( interPixelCount == 0)
            {
                VNCpixelProcessorState = VNCPPSTATE_IDLE;
                    TransmitByte('O');
            }
            break;

        case VNCPPSTATE_HEXTILE:
            while( processHextile() );
            return 0;

        case VNCPPSTATE_COPYRECT:
          if(!processCopyRect())
              return 0;
          break;

        default:
            // shouldn't be here
            break;
    }

    return 1;
}


// returns 0 if not enough data in buffer to process
int Vnc_StateMachine(void)
{
    switch(VNCstate)
    {
        case VNCSTATE_NOTCONNECTED:
            if( dataSize >= 12 )
            {
                VNCstate = VNCSTATE_WAITFORAUTHTYPE;
                dataPtr += 12;
                dataSize -= 12;
                return 1;
            }
            return 0;

        case VNCSTATE_WAITFORAUTHTYPE:
            if( dataSize >= 4 )
            {
                if( dataPtr[3] == 0x02 )
                    VNCstate = VNCSTATE_WAITFORVNCAUTHWORD;
                // no authentication
                else if( dataPtr[3] == 0x01 )
                {
                    TransmitString("noauth");
                    VNCsendState = VNCSENDSTATE_AUTHWORDSENT;
                    VNCstate = VNCSTATE_SENTCLIENTINITMESSAGE;
                }
                else VNCstate = VNCSTATE_AUTHFAILED;

                dataPtr += 4;
                dataSize -= 4;

                return 1;
            }
            return 0;

        case VNCSTATE_WAITFORVNCAUTHWORD:
            if( dataSize >= 16 )
            {
                VNCstate = VNCSTATE_WAITFORVNCAUTHRESPONSE;
                dataPtr += 16;
                dataSize -= 16;
                return 1;
            }
            return 0;

        case VNCSTATE_WAITFORVNCAUTHRESPONSE:
            if( dataSize >= 4 )
            {
                if( dataPtr[3] == 0x00 )
                    VNCstate = VNCSTATE_SENTCLIENTINITMESSAGE;
                else VNCstate = VNCSTATE_AUTHFAILED;
                dataPtr += 4;
                dataSize -= 4;
                return 1;
            }
            return 0;

        case VNCSTATE_SENTCLIENTINITMESSAGE:
            if( dataSize >= 24 )
            {
                // ignore framebuffer-width/height, pixel-format

                // use interPixelCount to keep track of the name length
                interPixelCount = dataPtr[20];
                interPixelCount <<= 8;
                interPixelCount |= dataPtr[21];
                interPixelCount <<= 8;
                interPixelCount |= dataPtr[22];
                interPixelCount <<= 8;
                interPixelCount |= dataPtr[23];

                VNCstate = VNCSTATE_WAITFORSERVERNAME;
                dataPtr += 24;
                dataSize -= 24;

                TransmitString("ipc:");
                TransmitHex(interPixelCount/256);
                TransmitHex(interPixelCount);
                TransmitString(" ");

                return 1;
            }
            return 0;

        case VNCSTATE_WAITFORSERVERNAME:
            if( dataSize >= interPixelCount)
            {
                TransmitString("ipc:");
                TransmitHex(interPixelCount/256);
                TransmitHex(interPixelCount);
                TransmitString(" ");


                TransmitString("name:");
                for(int i=0; i<interPixelCount; i++)
                {
                    TransmitByte(dataPtr[i]);
                    //TransmitByte(' ');
                }
                TransmitString(":: ");

                // remove the name from the buffer (and ignore)
                dataPtr += interPixelCount;
                dataSize -= interPixelCount;

                VNCstate = VNCSTATE_CONNECTED;
                return 1;
            }
            return 0;

        case VNCSTATE_CONNECTED:
            VNCstate = VNCSTATE_WAITFORUPDATE ;
            return 1;

        case VNCSTATE_CONNECTED_REFRESH:
        case VNCSTATE_WAITFORUPDATE:
            // data should be FrameBufferUpdate
            if( dataSize >= 4 )
            {
                // currently only looks for framebufferupdates, anything else breaks
                if( dataPtr[0] != 0x00 )
                {
                    TransmitString("unrec\n\r");
                    VNCstate = VNCSTATE_DEAD;
                    return 0;
                }

                rectangleCount = (dataPtr[2]<<8) + dataPtr[3];

                dataSize -= 4;
                dataPtr += 4;

                VNCpixelProcessorState = VNCPPSTATE_IDLE;
                VNCstate = VNCSTATE_PROCESSINGUPDATE;

#if 0
                TransmitString("fb:");
                TransmitHex(rectangleCount/256);
                TransmitHex(rectangleCount);
                TransmitString(" ");
#endif
                return 1;
            }
            return 0;

        case VNCSTATE_PROCESSINGUPDATE:
            while( pixelProcessor() );
            return 0;

        default:
            TransmitString("err:pro");
            dataSize = 0;
            while(1);
            return 0;
    }
}


unsigned int Vnc_ProcessVncBuffer(uint8_t * buffer, unsigned int length)
{
    dataPtr = buffer;
    dataSize = length;

    while( Vnc_StateMachine() ) ;

    // copy any remainder data into the remainder buffer
    return dataSize;
}


unsigned int Vnc_LoadResponseBuffer(uint8_t * buffer)
{
    switch(VNCsendState)
    {
        case VNCSENDSTATE_INIT:
            if( VNCstate > VNCSTATE_NOTCONNECTED )
            {
                TransmitString("s:INIT ");
                memcpy_P(buffer, versionMessage, sizeof(versionMessage)-1);
                VNCsendState = VNCSENDSTATE_VERSIONSENT;
                return sizeof(versionMessage)-1;
            }
            break;

        case VNCSENDSTATE_VERSIONSENT:
            if( VNCstate > VNCSTATE_WAITFORVNCAUTHWORD )
            {
                TransmitString("s:VSENT ");
                memcpy_P(buffer, VNCauthWord, sizeof(VNCauthWord));
                VNCsendState = VNCSENDSTATE_AUTHWORDSENT;
                return sizeof(VNCauthWord);
            }
            break;

        case VNCSENDSTATE_AUTHWORDSENT:
            if( VNCstate > VNCSTATE_WAITFORVNCAUTHRESPONSE )
            {
                TransmitString("s:AWSENT ");
                buffer[0] = '\0';  // client init message is 0 indicating server should give exclusive access to this client
                VNCsendState = VNCSENDSTATE_CLIENTINITMESSAGESENT;
                return 1;
            }
            break;

        case VNCSENDSTATE_CLIENTINITMESSAGESENT:
            if( VNCstate > VNCSTATE_WAITFORSERVERNAME )
            {
                TransmitString("s:CLIMS ");
                memcpy_P(buffer, pixelFormatMessage, sizeof(pixelFormatMessage));
                VNCsendState = VNCSENDSTATE_PIXELFORMATSENT;
                return sizeof(pixelFormatMessage);
            }
            break;

        case VNCSENDSTATE_PIXELFORMATSENT:
            //TransmitString("<-");
            if( VNCstate > VNCSTATE_WAITFORSERVERNAME )
            {
                TransmitString("s:PFMSENT ");
                memcpy_P(buffer, encodingTypeMessage, sizeof(encodingTypeMessage));
                VNCsendState = VNCSENDSTATE_ENCODINGTYPESENT;
                //transmitSpace = 0;
                return sizeof(encodingTypeMessage);
            }
            break;

        case VNCSENDSTATE_ENCODINGTYPESENT:
            if( VNCstate > VNCSTATE_WAITFORSERVERNAME )
            {
                TransmitString("s:ENCTSENT ");
                memcpy_P(buffer, refreshMessage, sizeof(refreshMessage));
                // change the request to a full refresh
                buffer[1] = 0x00;
                VNCsendState = VNCSENDSTATE_PARTIALREFRESHSENT;
                return sizeof(refreshMessage);
            }
            break;

        case VNCSENDSTATE_PARTIALREFRESHSENT:
            //if( VNCstate == VNCSTATE_CONNECTED_REFRESH || uip_poll() )
            if( VNCstate == VNCSTATE_CONNECTED_REFRESH )
            {
#if 0
                static uint8_t counter;
                TransmitString("s:PREFS ");
                TransmitHex(counter++);
                TransmitByte(' ');
#endif
                memcpy_P(buffer, refreshMessage, sizeof(refreshMessage));
                return sizeof(refreshMessage);
            }
            break;

        default:
            TransmitString("err:ACK");
            break;
    }
    return 0;
}

#if 0
void vnc_app(void)
{
    unsigned char transmitSpace = 1;

    // check for connection status - if closed return - reconnect later
    if( uip_closed() )
    {
        TransmitByte('X');
        TransmitByte('C');
        return;
    }

    if( uip_aborted() )
    {
        TransmitByte('X');
        TransmitByte('A');
        return;
    }

    if( uip_timedout() )
    {
        TransmitByte('X');
        TransmitByte('T');
        return;
    }

    // call processing function
    processNewdata();

    // clear out the uIP data buffer
    uip_len = 0;

    // check for retransmit - send the last packet again
    if( uip_rexmit() )
    {
        TransmitString("NAK");

        // call transmit function with the previous state
        sendMessage(VNCsendState-1);

        // can't do anything more
        return;
    }


    // check for space in the buffer - send packet
    if( uip_acked() || transmitSpace)
    {
        transmitSpace = 1;
        uip_reset_acked();

        // call transmit function
        sendMessage(VNCsendState);

        // if something was sent, the transmit buffer is now full
        if( uip_len )
            transmitSpace = 0;
    }
}
#endif
