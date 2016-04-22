#ifdef ESESC_LIVE
#ifndef ESESC_TRANSPORTER_H
#define ESESC_TRANSPORTER_H
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
File name:      Transporter.h
********************************************************************************/ 

#define MAX_BUFFER_SIZE 1024
#define KEY_CHANGE_INTERVAL 10
#define MAX_DATA 70000


//#define MAX_REPORT_BUFFER 40960
#define MAX_REPORT_BUFFER 81920

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "crypto++/modes.h"
#include "crypto++/aes.h"
#include "crypto++/filters.h"


using namespace std;

class Transporter {
  private:
    static char * host;
    static int portno;
    static int sockfd;
    static string buffer[MAX_BUFFER_SIZE];
    static byte orig_key[CryptoPP::AES::DEFAULT_KEYLENGTH];
    static byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
    static byte old_key[CryptoPP::AES::DEFAULT_KEYLENGTH];
    static byte iv[CryptoPP::AES::BLOCKSIZE];
    static int key_valid;
    static byte toggle;

    static void buffer_push(string data);
    static string buffer_pull(char type, string message);
    static void send(unsigned char * buf, int length);
    static string receive(char type, string message);
    static void renew_key();

  public:
    static void connect_to_server(char * h, int pn);
    static void disconnect();
    static void send_json(string message, string data);
    static void send_schema(string name, string schema);
    static void send_data(char * name, unsigned char * data, int len, char * sid);
    static void send_string(string message, string data);
    static string receive_data(string message);
    static void send_fast(string message, const char * format, ...);
    static void receive_fast(string message, const char * format, ...);
};

#endif
#endif
