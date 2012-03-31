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
        VNCPPSTATE_HEXTILE
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


// partial refresh request
prog_char refreshMessage[] = { 0x03,0x01,0,0,0,0,SCREEN_WIDTH/256,SCREEN_WIDTH,SCREEN_HEIGHT/256,SCREEN_HEIGHT };

// sets the pixel format for 8-bit BGR format
prog_uchar pixelFormatMessage[] = { 0x00, 0,0,0,
                                    8,8,
                                    0,1,
                                    0x00, 0x07,
                                    0x00, 0x07,
                                    0x00, 0x03,
                                    0, 3, 6,
                                    0,0,0 };

// requests hextile encoding only
prog_uchar encodingTypeMessage[] = { 2, 0, 0, 1, 0, 0, 0, 5 } ;




	  
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

// keeps track of the pixel position to draw to the display
// these must be initialized before calling the drawPixelsInView() function
static unsigned int currentDrawX;
static unsigned int currentDrawY;


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




//  removes the amount of pixels specified by interPixelCount from the buffer
//  or as many as are currently in the buffer
void dropOutOfViewPixels(void)
{
	// clear the rest of the pixels out of the receive buffer
	if( (dataSize >= interPixelCount) )
	{
		dataPtr += interPixelCount;
		dataSize -= interPixelCount;
		interPixelCount = 0;
		return;
	}
	
	// clear out the receive buffer - more pixels in transit
	if( (interPixelCount > dataSize) && (dataSize != 0) )
	{
		dataPtr += dataSize;
		interPixelCount-=dataSize;
		dataSize = 0;
	}
}



// draws interPixelCount number of pixels to the display, starting at
// currentX, currentY and incrementing currentX until there are no more
// pixels in the buffer, or all on the row have been drawn
// there is no bounds checking on currentX and and the side of the display
void drawPixelsInView(void)
{

	unsigned char pixel8offset ;
	unsigned char i, j, byte ;
	unsigned int writeAddress = (currentDrawX >> 3)
	                          + (unsigned int)( currentDrawY * ( 256/8 ) ) ;
	

	// if current x=1 offset will be 7, indicating 7 pixels until
	//   the next byte boundary
	pixel8offset = (8-currentDrawX) % 8 ;
	
#if 0
	// set the start address for drawing here, 
	*COMMAND_ADDRESS = CSRW_COMMAND ;
	*READWRITE_ADDRESS = (unsigned char)writeAddress ;
	*READWRITE_ADDRESS = (unsigned char)(writeAddress>>8) ;

#endif

	// off 8bit boundary
	if( pixel8offset )
	{
			// decrease interPixelCounter by offset
			// draw until the 8-bit boundary


			if( interPixelCount < pixel8offset )
			{
				if( dataSize < interPixelCount)
					return;

				
				// create the mask for the existing data
				byte=0;
				j=0x80;
				for(i=0; i<8-pixel8offset; i++)
				{
					byte |= j;
					j>>=1;
				}
				
				for( ; i<(8-pixel8offset)+interPixelCount; i++)
				{
				  j>>=1;
				}
				
				for( ; i<8; i++)
				{
					byte |= j;
					j>>=1;
				}
				
#if 0
				// read in and mask the byte from the display
				*COMMAND_ADDRESS = READ_COMMAND ;
				byte &= *COMMAND_ADDRESS ;
				
				// reset the address
				*COMMAND_ADDRESS = CSRW_COMMAND ;
				*READWRITE_ADDRESS = (unsigned char)writeAddress ;
				*READWRITE_ADDRESS = (unsigned char)(writeAddress>>8) ;	
				
				// AND/OR the two together
				for(i=0; i<pixel8offset; i++)
				{
					if( dataPtr[i] != 0xFF )
						byte |= j; 
					else byte &= ~j;
					
					j>>=1;
				}
				
				// draw it back
				*COMMAND_ADDRESS = WRITE_COMMAND ;
				*READWRITE_ADDRESS = byte ;
#endif
				
				//update the counters
				
				dataSize -= interPixelCount;
				dataPtr += interPixelCount;
				
				interPixelCount = 0;
				writeAddress++;
				currentDrawX+=interPixelCount;
				
				return;
			}
			
			
			if( dataSize < pixel8offset )
				return;
							
#if 0
			// create the mask for the existing data
			byte=0;
			j=0x80;
			for(i=pixel8offset; i<8; i++)
			{
				byte |= j;
				j>>=1;
			}

			// read in and mask the byte from the display
			*COMMAND_ADDRESS = READ_COMMAND ;
			byte &= *COMMAND_ADDRESS ;
			
			// reset the address
			*COMMAND_ADDRESS = CSRW_COMMAND ;
			*READWRITE_ADDRESS = (unsigned char)writeAddress ;
			*READWRITE_ADDRESS = (unsigned char)(writeAddress>>8) ;	
			
			// AND/OR the two together
			for(i=0; i<pixel8offset; i++)
			{
				if( dataPtr[i] != 0xFF )
					byte |= j; 
				else byte &= ~j;
				
				j>>=1;
			}
			
			// draw it back
			*COMMAND_ADDRESS = WRITE_COMMAND ;
			*READWRITE_ADDRESS = byte ;
#endif
			
			//update the counters
			
			dataSize -= pixel8offset;
			dataPtr += pixel8offset;
			
			interPixelCount -= pixel8offset;
			writeAddress++;
			currentDrawX+=pixel8offset;
			pixel8offset=0;
			
			// continue to draw the rest of the line
			
	}
	
	if( !pixel8offset )
	{

		while( interPixelCount >= 8 )
		{
			if( dataSize < 8 )
				return;
			
#if 0
			// draw 8
			byte=0x00;
			j=0x80;
			
			for(i=0;i<8;i++)
			{
				byte <<= 1;	
				if( dataPtr[i] != 0xFF )
					byte |= (0x01);
			}
			
			//draw byte to screen
			*COMMAND_ADDRESS = WRITE_COMMAND ;
			*READWRITE_ADDRESS = byte ;
#endif
			
			// decrease interpixelcounter by 8
			dataPtr += 8;
			dataSize -= 8;
			
			currentDrawX += 8 ;
			writeAddress++;
			interPixelCount -= 8;

		}

		// if here - there's only a few pixels left, draw them
		if( interPixelCount != 0 )
		{
			if( dataSize < interPixelCount )
				return;
			
#if 0
			// create the mask for the existing data
			byte=0x00;
			j=0x01;
			for(i=interPixelCount; i<8; i++)
			{
				byte |= j;
				j<<=1;
			}
	
			// read in and mask the byte from the display
			*COMMAND_ADDRESS = READ_COMMAND ;
			byte &= *COMMAND_ADDRESS ;
			
			// reset the address
			*COMMAND_ADDRESS = CSRW_COMMAND ;
			*READWRITE_ADDRESS = (unsigned char)writeAddress ;
			*READWRITE_ADDRESS = (unsigned char)(writeAddress>>8) ;	
			
			j=0x80;
			// AND/OR the two together
			for(i=0; i<interPixelCount; i++)
			{
				if( dataPtr[i] != 0xFF )
					byte |= j; 
				else byte &= ~j;
				
				j>>=1;
			}
			
			// draw it back
			*COMMAND_ADDRESS = WRITE_COMMAND ;
			*READWRITE_ADDRESS = byte ;
#endif
			
			dataPtr += interPixelCount;
			dataSize -= interPixelCount;
			currentDrawX += interPixelCount;			
			interPixelCount=0;
		}
	}

}





// function used to receive and draw hextile updates to the screen
// this definitely needs to be optimized and probably separated into smaller
// sub-functions to reduce the stack usage, but for now, it works...
// Any tile that only partially overlaps the local display window will be
// dropped
void processHextile(void)
{
  // tile x,y w,h
  static unsigned int tileX, tileY;
  static unsigned char tileW, tileH;
  
  static unsigned char background, foreground;
  
  static unsigned char subencodingByte;
  
  static unsigned char numSubrects;
  
  unsigned char byte, temp1, temp2, temp3;
  
  unsigned char i,j;
  
  unsigned char x0, y0, x1, y1;
  
  unsigned int writeAddress;
  
  // temporary 16x16 pixel buffer
  static unsigned char hextileBuffer[16][16];
  
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
      goto hextile_parseheader;
    
    case HEXTILESTATE_DONE:
hextile_done:    
      
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
          return;
        }
        
        tileX = updateWindowX0;
        tileY += tileH;
        tileH = updateWindowY1 < tileY+16 ? updateWindowY1-tileY : 16;
      }
      
      // if the next tile is the last column, set the width to < 16
      tileW = updateWindowX1 < tileX+16 ? updateWindowX1-tileX : 16;
      
      hextileState = HEXTILESTATE_PARSEHEADER;
      
      // continue to parseheader
    
    case HEXTILESTATE_PARSEHEADER:
hextile_parseheader:
    
      // wait for subencoding byte
      if( dataSize < 1 )
        return;
        
      subencodingByte = *dataPtr++;
      dataSize--;
      
      
      temp3 = 0;
      
      if( subencodingByte & HEXTILE_BACKGROUND_MASK )
        temp3++;
        
      if( subencodingByte & HEXTILE_FOREGROUND_MASK )
        temp3++;
        
      if( subencodingByte & HEXTILE_SUBRECT_MASK )
        temp3++;
        
      // if the optional bytes are not in the buffer, exit
      if( temp3 > dataSize )
      {
        dataPtr--;
        dataSize++;
        return;
      }
      
      // if raw bit, set state to raw and goto drawraw
      if( subencodingByte & HEXTILE_RAW_MASK )
      {
        //pixelCount = tileW * tileH;
        
        if( tileX < windowX0 || (tileX + tileW) > windowX1 || tileY < windowY0
            || (tileY + tileH) > windowY1 )
        {
          interPixelCount = tileW * tileH;
          hextileState = HEXTILESTATE_DROPRAW;
          goto hextile_dropraw;
        }
        
        currentDrawX = tileX;
        currentDrawY = tileY;
        interPixelCount = tileW;

        hextileState = HEXTILESTATE_DRAWRAW;
        goto hextile_drawraw;
      }
      
      // else - if background specified read in pixel value
      if( subencodingByte & HEXTILE_BACKGROUND_MASK )
      {
		//TransmitByte('B');
        background = *dataPtr++;
        dataSize--;
      }
    
      if( subencodingByte & HEXTILE_FOREGROUND_MASK )
      {
		//TransmitByte('F');
        foreground = *dataPtr++;
        dataSize--;
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
            for (i = 0; i < 16; i++)

            {
                hextileBuffer[ j ][ i ] = background;
            }
        }
      
      hextileState = HEXTILESTATE_FILLBUFFER ;
      
      goto hextile_fillbuffer;
    
    case HEXTILESTATE_DROPRAW:
hextile_dropraw:

      //TransmitString("drop");
      dropOutOfViewPixels();
      
      if( interPixelCount == 0)
      {
        hextileState = HEXTILESTATE_DONE;
        goto hextile_done; 
      }
        
      break;


    case HEXTILESTATE_DRAWRAW:
hextile_drawraw:
    
      do
      {
        drawPixelsInView();
        if( interPixelCount == 0 )
        {
          // increment currentdrawX and currentdrawY
          currentDrawX = tileX;
          currentDrawY++;
          if( currentDrawY >= tileY+tileH)
          {
            hextileState = HEXTILESTATE_DONE;
            goto hextile_done; 
          }
          interPixelCount = tileW;
        }
      }
      while( dataSize >= interPixelCount );

      break;

    case HEXTILESTATE_FILLBUFFER:
hextile_fillbuffer:

	  // draw all the subrectangles to the buffer
      while( numSubrects != 0 )
      {
      
        byte = foreground;
      
        // is subrect in buffer (return if not)
        if( subencodingByte & HEXTILE_SUBRECTCOLOR_MASK )
        {
          if( dataSize < 3 )
            return;
          
          // subrect color;
          byte = dataPtr[0];
          
          dataSize--;
          dataPtr++;
        }
        else if( dataSize < 2 )
          return;

        
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
            for (i = x0; i <= x1; i++)
            {
                hextileBuffer[ j ][ i ] = byte;
            }
        }

#if 0
        // are there any pixels in the first byte?
        if( x0 < 0x08 )
        {
          temp1 = 0x80;
          temp2 = 0x00;
      
          // create mask
          for(i=0; i<0x08; i++)
          {
            if(i > x1)
              break;
            if(i >= x0)
              temp2 |= temp1 ;
            temp1 >>= 1;
          }
     
          // if rectangle is white:
          if( byte == 0xFF )
            temp2 = ~temp2;
       
          // fill in the left side of the buffer
          for(i=y0; i<=y1; i++)
          {
            if( byte != 0xFF )
              hextileBuffer[0][i] |= temp2;
            else hextileBuffer[0][i] &= temp2;
          }
        }
   
        // are there any pixels in the second byte?
        if( x1 >= 0x08 )
        {
          temp1 = 0x80;
     
          // did the rectangle start in the first byte?
          x0 = x0 > 0x08 ? x0 : 0x08;
       
          temp2 = 0x00;
     
          for(i=0x08; i<0x10; i++)
          {
            if(i > x1)
              break;
            if(i >= x0)
              temp2 |= temp1 ;
            temp1 >>= 1;
          }
   
          // if white:
          if( byte == 0xFF )
            temp2 = ~temp2;
       
          // fill in the right side of the buffer
          for(i=y0; i<=y1; i++)
          {
            if( byte != 0xFF )
              hextileBuffer[1][i] |= temp2;
            else hextileBuffer[1][i] &= temp2;
          }
   
        }
#endif
     
        numSubrects--;
     }
   
      
     // state = drawtile - continue
     hextileState = HEXTILESTATE_DRAWTILE ;
    
  case HEXTILESTATE_DRAWTILE:
    
    // only draw the buffer if it fits cleanly into the currentWindow
    if( tileX < windowX0 || (tileX + tileW) > windowX1 || tileY < windowY0
        || (tileY + tileH) > windowY1 )
    {
      hextileState = HEXTILESTATE_DONE ;
      goto hextile_done; 
    }
    
    DrawHextile(tileX, tileY, tileW, tileH, hextileBuffer);

#if 0
	// compute the offset of the hextileBuffer from the 8-pixel SED1330 columns
    temp1 = tileX & 0x07;

	// shift the buffer by the offset
	for( i=0; i<tileH; i++ )
	{
	  byte  = hextileBuffer[0][i];
	  temp2 = hextileBuffer[1][i];
	  temp3 = hextileBuffer[2][i];
	  for(j=0; j<temp1; j++)
	  {
	    // shift the row once to the right, spanning all three columns
	    asm volatile("lsr %0" "\n\t"
	                 "ror %1" "\n\t"
	                 "ror %2" "\n\t"
	                 : "=r" (byte),
	                   "=r" (temp2),
	                   "=r" (temp3)
	                 : "r" (temp3),
	                   "r" (temp2),
	                   "r" (byte) );
	  }
	  
	  hextileBuffer[0][i] = byte;
	  hextileBuffer[1][i] = temp2;
	  hextileBuffer[2][i] = temp3;
	}

    
    

	// draw the three columns to the display    
	for( j=0; j<3; j++)
	{
	    // create a mask for the column
	    temp2 = 0x00;
	    byte = 0x80;
	    
		// the first column may not start on an 8-pixel boundary, the others will
		if(j==0)
		{
		    for(i=0; i<8; i++)
		    { 
		      if( i >= (tileW+temp1))
		        break;
		       
		      if(i >= temp1)
		        temp2 |= byte;
		        
		      byte>>=1;
		    }
		}
		else
		{
		    for(i=0; i<8; i++)
		    { 
		      if( i >= (tileW+temp1-(j*8)))
		        break;
		
		      temp2 |= byte;
		        
		      byte>>=1;
		    }
		}
	
		// a mask of 0x00 means there is no data to draw to the display
		// a mask of 0xFF means all the existing display data will be overwritten
		
	    // if mask != 0x00 or 0xFF, read in from top-bottom and update buffer
	    if( (temp2 != 0x00) && (temp2 != 0xFF) )
	    {
	      // set address and cursor direction to down
		  *COMMAND_ADDRESS = CSRDIR_DOWN_COMMAND ;
	      
	      writeAddress = (tileX >> 3) + j
		               + (unsigned int)( tileY * ( 256/8 ) ) ;
	
	      
	      *COMMAND_ADDRESS = CSRW_COMMAND ;
		  *READWRITE_ADDRESS = (unsigned char)writeAddress ;
		  *READWRITE_ADDRESS = (unsigned char)(writeAddress>>8) ;	
	      
		  *COMMAND_ADDRESS = READ_COMMAND ;
		  
	      for( i=0; i<tileH; i++ )
	      {
	        hextileBuffer[j][i] &= temp2;
	        hextileBuffer[j][i] |= (~temp2 & *COMMAND_ADDRESS);
	      }
	      
	      *COMMAND_ADDRESS = CSRDIR_RIGHT_COMMAND ;
	    }
	    
	    // if mask != 0x00, write to display from top-bottom
	    if( temp2 != 0x00 )
	    {
	      // set address and cursor direction to down
		  *COMMAND_ADDRESS = CSRDIR_DOWN_COMMAND ;
	      
	      writeAddress = (tileX >> 3) + j
		               + (unsigned int)( tileY * ( 256/8 ) ) ;
	
	      
	      *COMMAND_ADDRESS = CSRW_COMMAND ;
		  *READWRITE_ADDRESS = (unsigned char)writeAddress ;
		  *READWRITE_ADDRESS = (unsigned char)(writeAddress>>8) ;	
	      
		  *COMMAND_ADDRESS = WRITE_COMMAND ;
		  
	      for( i=0; i<tileH; i++ )
	      {
	        // buffer[0][i] = displaybyte & mask
	        	*READWRITE_ADDRESS = hextileBuffer[j][i] ;
	      }
	      
	      *COMMAND_ADDRESS = CSRDIR_RIGHT_COMMAND ;
	    
	    }
	  }
#endif
	    
	  hextileState = HEXTILESTATE_DONE ;
	  goto hextile_done; 
	    

    default:
	  break;
	  
    }
}




// this function is called immediately after a VNC FramebufferUpdate header is
// received.  The function continues until all the rectangles contained in the
// FramebufferUpdate group have been processed and drawn or discarded
void pixelProcessor(void)
{
	switch(VNCpixelProcessorState)
	{
		case VNCPPSTATE_IDLE:
		vncppstate_idle:
			// done? - quit
			if( !(rectangleCount) )
			{
				VNCstate = VNCSTATE_CONNECTED_REFRESH;
				return;
			}
			
			// read rectangle header
			if( dataSize < 12 )
				return;

			rectangleCount--;
			
			updateWindowX0 = (dataPtr[0]<<8) + dataPtr[1];
			updateWindowX1 = updateWindowX0 + (dataPtr[4]<<8) + dataPtr[5];
			
			updateWindowY0 = (dataPtr[2]<<8) + dataPtr[3];
			updateWindowY1 = updateWindowY0 + (dataPtr[6]<<8) + dataPtr[7];
			
			// check for bad encoding
			if( dataPtr[8] != 0x00 	|| dataPtr[9] != 0x00 ||
				dataPtr[10] != 0x00 )
			{
			  TransmitByte('R');
			  return;
			}
			
			// get the encoding type
			switch( dataPtr[11] )
			{
			  case 0:  // raw
                goto decodeRaw;
			    break;
			    
			  case 5:  // hextile
			    goto decodeHextile;
			    break;
			    
			  default:
			    TransmitByte('R');
			    return;
			}

            
decodeRaw:
			// if anything raw is received, drop all the pixels and continue
			
			interPixelCount = ((dataPtr[4]<<8) + dataPtr[5]) ;
			interPixelCount *= ((dataPtr[6]<<8) + dataPtr[7]);
			
			dataPtr += 12;
			dataSize -= 12;
			
			VNCpixelProcessorState = VNCPPSTATE_OUTBOUNDS ;

			return;
			
			
decodeHextile:

			dataPtr += 12;
			dataSize -= 12;
			
			VNCpixelProcessorState = VNCPPSTATE_HEXTILE;
			hextileState = HEXTILESTATE_INIT;
			
			processHextile();

  			if( VNCpixelProcessorState == VNCPPSTATE_IDLE )
  			{
		    	//TransmitByte('H');
		    	goto vncppstate_idle;
		  	}
				
            return;			

		case VNCPPSTATE_OUTBOUNDS:
			// drop all pixels in the current rectangle
			dropOutOfViewPixels();
			
			// when finished, go back to the idle state
			if( interPixelCount == 0)
			{
				VNCpixelProcessorState = VNCPPSTATE_IDLE;
					TransmitByte('O');

				goto vncppstate_idle;
			}
			break;
			

		case VNCPPSTATE_HEXTILE:
		  processHextile();
		  
		  if( VNCpixelProcessorState == VNCPPSTATE_IDLE )
		  {
		      goto vncppstate_idle;
		  }
		  break;

		default:
			// shouldn't be here
			break;
	}
}


unsigned int Vnc_ProcessVncBuffer(uint8_t * buffer, unsigned int length)
{
	// init pointer to appdata - remainder
	dataPtr = buffer;
	
	// init total to appdata + remainder
	dataSize = length;

	do {
		switch(VNCstate)
		{
			case VNCSTATE_NOTCONNECTED:
				if( dataSize >= 12 )
				{
					VNCstate = VNCSTATE_WAITFORAUTHTYPE;
					dataPtr += 12;
					dataSize -= 12;
				}
				break;
				
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
				}
				break;
		
			case VNCSTATE_WAITFORVNCAUTHWORD:
				if( dataSize >= 16 )
				{
					VNCstate = VNCSTATE_WAITFORVNCAUTHRESPONSE;
					dataPtr += 16;	
					dataSize -= 16;
				}
				break;
			
			case VNCSTATE_WAITFORVNCAUTHRESPONSE:
				if( dataSize >= 4 )
				{
					if( dataPtr[3] == 0x00 )
						VNCstate = VNCSTATE_SENTCLIENTINITMESSAGE;
					else VNCstate = VNCSTATE_AUTHFAILED;
					dataPtr += 4;
					dataSize -= 4;
				}
				break;
		
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
				}
				break;
		
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
				}
				break;
	
			case VNCSTATE_CONNECTED:
				VNCstate = VNCSTATE_WAITFORUPDATE ;
				break;
				
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

					TransmitString("fb:");
					TransmitHex(rectangleCount/256);
					TransmitHex(rectangleCount);
					TransmitString(" ");
				}
				
				break;
	
			case VNCSTATE_PROCESSINGUPDATE:
				pixelProcessor();
				break;
	
			default:
				TransmitString("err:pro");
				dataSize = 0;
	            while(1);
				break;
		}	
	}
	while( dataSize > REMAINDERBUFFER_SIZE ) ;
	
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
                TransmitString("s:PREFS ");
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
