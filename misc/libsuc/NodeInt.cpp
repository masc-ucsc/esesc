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
#include <stdlib.h>
#include "NodeInt.h"

int64_t NodeInt::sockfd = 0;

void NodeInt::socket_connect(int64_t portno) {
  struct sockaddr_in serv_addr;
  struct hostent *server;
  int64_t ret = socket(AF_INET, SOCK_STREAM, 0);
  if (ret < 0) 
      perror("ERROR opening socket");
  server = gethostbyname("localhost");
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
       (char *)&serv_addr.sin_addr.s_addr,
       server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(ret,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
      perror("ERROR connecting");
  NodeInt::sockfd = ret;
};

void NodeInt::socket_disconnect() {
  close(NodeInt::sockfd);
}

char NodeInt::read_command (int64_t params[]) {
  int64_t i, n;
  char buffer[1024];
  bzero(buffer,1024);
  n = read(NodeInt::sockfd, buffer, 255);
  if (n < 0) {
    perror("ERROR reading from socket");
    return 0;
  }
  //printf("Socket read: %s\n", buffer);
  int64_t index = 0;
  int64_t val = 0;
  for(i = 2; i < 1024; i++) {
    if(buffer[i] == ',') {
      params[index] = val;
      index++;
      val = 0;
    } else if(buffer[i] == 0) {
      params[index] = val;
      return buffer[0];
    } else {
      val = val * 10 + (buffer[i] - '0');
    }
  }
  return buffer[0];
};

//This function is used to send commands to NodeJS server
int64_t NodeInt::write_command (char cmd, int64_t params[], int64_t nparams) {
  int64_t i, n, d;
  char t[20];
  char buffer[1024];
  bzero(buffer, 1024);
  buffer[0] = cmd;
  for(i = 0; i < nparams; i++) {
    bzero(t, 20);
    d = params[i];
    sprintf(t, ",%ld", d);
    strcat(buffer, t);
  }
  strcat(buffer, ";");
  //printf("Socket write: %s\n", buffer);
  n = write(NodeInt::sockfd, buffer, strlen(buffer));
  if (n < 0) {
    perror("ERROR writing to socket");
    return 0;
  } else {
    return 1;
  }
};

void NodeInt::write_buffer(char *buffer, int len) {
  if(len == 0)
    len = strlen(buffer);
  int n = write(NodeInt::sockfd, buffer, len);
  if (n < 0)
    perror("ERROR writing to socket");  
}

char NodeInt::read_buffer(char *buffer, int len) {
  bzero(buffer, len);
  int n = read(NodeInt::sockfd, buffer, len);
  if (n < 0)
    perror("ERROR reading from socket");
  return buffer[0];
}

char NodeInt::encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
char *NodeInt::decoding_table = NULL;
int NodeInt::mod_table[] = {0, 2, 1};

char *NodeInt::base64_encode (const unsigned char *data, size_t input_length, size_t *output_length) {
    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = (char *)malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = NodeInt::encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = NodeInt::encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = NodeInt::encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = NodeInt::encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < NodeInt::mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char *NodeInt::base64_decode (const unsigned char *data, size_t input_length, size_t *output_length) {
    if (NodeInt::decoding_table == NULL) NodeInt::build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = (unsigned char *)malloc(sizeof(unsigned char)* (*output_length));
    if (decoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : NodeInt::decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : NodeInt::decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : NodeInt::decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : NodeInt::decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}


void NodeInt::build_decoding_table() {
    NodeInt::decoding_table = (char*)malloc(256);

    for (int i = 0; i < 64; i++)
        NodeInt::decoding_table[(unsigned char) NodeInt::encoding_table[i]] = i;
}


void NodeInt::base64_cleanup() {
    free(NodeInt::decoding_table);
}
