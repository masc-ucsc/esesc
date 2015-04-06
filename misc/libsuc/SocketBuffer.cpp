/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <alloca.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>

#include "SocketBuffer.h"
#include "NodeInt.h"

int64_t SocketBuffer::pos;
char SocketBuffer::b[4096];

SocketBuffer::SocketBuffer () {
  pos = 0;
  bzero(b, 4096);  
}

void SocketBuffer::add (const char *s) {
  int64_t i = 0;
  while(s[i]) {
    b[pos] = s[i];
    pos++;
    if (pos >= 2048 && s[i] == ';') {
      NodeInt::write_buffer(b, 0);
      pos = 0;
      bzero(b, 4096);
    }
    i++;
  }
}

void SocketBuffer::flush (char *cpid) {
  if (pos != 0) {
    NodeInt::write_buffer(b, 0);
    pos = 0;
    bzero(b, 4096);
  }

  //Create and send the flush message
  char buffer[20];
  bzero(buffer, 20);
  sprintf(buffer, "p,%s;", cpid);
  NodeInt::write_buffer(buffer, 0);
}
