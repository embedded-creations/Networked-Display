#ifndef _VNC_H_
#define _VNC_H_

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

#include <avr/pgmspace.h>

unsigned int Vnc_ProcessVncBuffer(uint8_t * buffer, unsigned int length);
unsigned int Vnc_LoadResponseBuffer(uint8_t * buffer);
void Vnc_ResetSystem(void);


#define RAW_DECODING 0

#define VNC_PIXEL_FORMAT_MESSAGE_8BIT   { 0x00, 0,0,0, \
                                            8,8, \
                                            0,1, \
                                            0x00, 0x07, \
                                            0x00, 0x07, \
                                            0x00, 0x03, \
                                            0, 3, 6, \
                                            0,0,0 }

// this is specific to the LCD display, and should probably be defined outside of VNC.h
#define VNC_PIXEL_FORMAT_MESSAGE_16BIT   { 0x00, 0,0,0, \
                                            16,16, \
                                            0,1, \
                                            0x00, 0x1F, \
                                            0x00, 0x3F, \
                                            0x00, 0x1F, \
                                            11, 5, 0, \
                                            0,0,0 }

#define VNC_ENCODING_TYPE_HEXTILE_ONLY              { 2, 0, 0, 1, 0, 0, 0, 5 }
#define VNC_ENCODING_TYPE_HEXTILE_AND_COPYRECT      { 2, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5 }


// up to 16 bytes of TCP data can be left unused and buffered until the next
// segment arrives
#define REMAINDERBUFFER_SIZE 16

#define MAXIMUM_VNCMESSAGE_SIZE 20

// Todo: what's the real value?
#define MAXIMUM_VNCRESPONSE_SIZE 50


#endif
