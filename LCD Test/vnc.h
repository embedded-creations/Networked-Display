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


/* UIP_APPSTATE_SIZE: The size of the application-specific state
   stored in the uip_conn structure. */
#define UIP_APPSTATE_SIZE 1

#include "uip.h"
#include "buffUART.h"
#include "display.h"
#include <progmem.h>

void vnc_init(void);
void vnc_app(void);



#define RAW_DECODING 0


enum { 	VNCSTATE_DEAD,
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
		  
enum {	VNCSENDSTATE_INIT,
		VNCSENDSTATE_VERSIONSENT,
		VNCSENDSTATE_AUTHWORDSENT,
		VNCSENDSTATE_CLIENTINITMESSAGESENT,
		VNCSENDSTATE_PIXELFORMATSENT,
		VNCSENDSTATE_ENCODINGTYPESENT,
		VNCSENDSTATE_PARTIALREFRESHSENT
		  };
		  
enum {	VNCPPSTATE_IDLE,
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
          


// up to 16 bytes of TCP data can be left unused and buffered until the next
// segment arrives
#define REMAINDERBUFFER_SIZE 16

#define MAXIMUM_VNCMESSAGE_SIZE 20


#define FS_STATISTICS 0



#define UIP_APPCALL     vnc_app


#endif
