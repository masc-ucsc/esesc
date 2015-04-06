/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2013 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Sina Hassani

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

********************************************************************************
File name:      NodeInt
********************************************************************************/ 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


class NodeInt {
  private:
  protected:
  public:
    static int64_t sockfd;
    static void socket_connect(int64_t portno);
    static void socket_disconnect();
    static char read_command(int64_t params[]);
    static int64_t write_command(char cmd, int64_t params[], int64_t nparams); 
    static void write_buffer(char *buffer, int len);
    static char read_buffer(char *buffer, int len);
    static char encoding_table[];
    static char *decoding_table;
    static int mod_table[];
    static char *base64_encode (const unsigned char *data, size_t input_length, size_t *output_length);
    static unsigned char *base64_decode (const unsigned char *data, size_t input_length, size_t *output_length);
    static void build_decoding_table();
    static void base64_cleanup();
};
