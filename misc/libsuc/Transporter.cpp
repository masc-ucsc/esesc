#if ESESC_LIVE
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2014 University of California, Santa Cruz.

   Contributed by Sina Hassani
                  Jose Renau

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
File name:      Transporter.CPP
********************************************************************************/

#include "Transporter.h"

char * Transporter::host;
int Transporter::portno;
int Transporter::sockfd;
string Transporter::buffer[MAX_BUFFER_SIZE];
byte Transporter::orig_key[CryptoPP::AES::DEFAULT_KEYLENGTH];
byte Transporter::key[CryptoPP::AES::DEFAULT_KEYLENGTH];
byte Transporter::old_key[CryptoPP::AES::DEFAULT_KEYLENGTH];
byte Transporter::iv[CryptoPP::AES::BLOCKSIZE];
int Transporter::key_valid;
byte Transporter::toggle;

void Transporter::connect_to_server(char * h, int pn) {
  host = h;
  portno = pn;
  key_valid = KEY_CHANGE_INTERVAL;
  toggle = 1;
  struct sockaddr_in serv_addr;
  struct hostent * server;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  fprintf(stderr,"Connect to %s %d\n",h,pn);
  if(sockfd < 0)  {
    perror("ERROR opening socket");
    exit(-1);
  }
  server = gethostbyname(host);
  if(server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(-1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    perror("ERROR connecting");
  //encryption
  //ifstream in("key");
  //string keys((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  string keys(":sx0K2-&94l+cv**");
  //ifstream in2("iv");
  //string ivs((istreambuf_iterator<char>(in2)), istreambuf_iterator<char>());
  string ivs("0sd+3jj*)__sdf&1");
  //ifstream in3("passkey");
  //string passkey((istreambuf_iterator<char>(in3)), istreambuf_iterator<char>());
  string passkey("Llkd8fv93nBB83209ucdvy8cbnNYT^^^12-++049dc78vn234bJMZx54r12l2349");
  for(int i = 0; i < CryptoPP::AES::DEFAULT_KEYLENGTH; i++)
    key[i] = (unsigned char)keys.at(i);
  for(int i = 0; i < CryptoPP::AES::BLOCKSIZE; i++)
    iv[i] = (unsigned char)ivs.at(i);
  //registration
  send_string("passkey", passkey);
  string dontcare = receive('r', "registered");
}

void Transporter::disconnect() {
  close(sockfd);
}

void Transporter::send(unsigned char * buf, int length) {
  string ciphertext;
  CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
  stfEncryptor.Put( reinterpret_cast<const unsigned char*>(buf), length + 1);
  stfEncryptor.MessageEnd();

  int len = ciphertext.length();
  byte blen[5];
  for (int i = 0; i < 4; i++)
    blen[3 - i] = (len >> (i * 8));
  blen[4] = toggle;
  int n = write(sockfd, blen, 5);
  fprintf(stderr,"Transporter::send write1 socket %d bytes %d\n", sockfd, n);
  if (n < 0)
    perror("ERROR writing to socket"); 
  n = write(sockfd, ciphertext.c_str(), len);
  fprintf(stderr,"Transporter::send write2 socket %d bytes %d len %d\n", sockfd, n, len);
  if (n < 0)
    perror("ERROR writing to socket");
  key_valid--;
  if(key_valid <= 0) {
    renew_key();
  }
}

void Transporter::send_schema(string name, string schema) {
  int len = schema.length();
  int length = len + 45;
  unsigned char buf[MAX_REPORT_BUFFER];
  unsigned char * name_arr = (unsigned char *) name.c_str();
  unsigned char * data_arr = (unsigned char *) schema.c_str();
  buf[0] = 's';
  memcpy(buf + 21, name_arr, 20);
  for (int i = 0; i < 4; i++)
    buf[44 - i] = (len >> (i * 8));

  //TODO: add bounds checking to prevent stack overflow 
  memcpy(buf + 45, data_arr, len);
  send(buf, length);
  string ack = receive('s', name);
}

void Transporter::send_data(char * name, unsigned char * data, int len, char * sid) {
  int length = 45 + len;
  unsigned char buf[MAX_REPORT_BUFFER];
  buf[0] = 'd';
  memcpy(buf + 1, name, 20);
  memcpy(buf + 21, sid, 20);
  for (int i = 0; i < 4; i++)
    buf[44 - i] = (len >> (i * 8));
  memcpy(buf + 45, data, len);
  send(buf, length);
}

void Transporter::send_string(string message, string data) {
  int len = data.length();
  int length = len + 45;
  unsigned char buf[MAX_REPORT_BUFFER];
  unsigned char * message_arr = (unsigned char *) message.c_str();
  unsigned char * data_arr = (unsigned char *) data.c_str();
  buf[0] = 'c';
  memcpy(buf + 1, message_arr, 20);
  for (int i = 0; i < 4; i++)
    buf[44 - i] = (len >> (i * 8));
  memcpy(buf + 45, data_arr, len);
  send(buf, length);
}

string Transporter::receive(char type, string message) {
  string data;

  string fm = buffer_pull(type, message);
  if(fm != "") {
    string::size_type t = fm.find(":", 0);
    return fm.substr(t + 1);
  }

  unsigned char b[5];
  bzero(b, 5);
  int n = read(sockfd, b, 5);
  if (n < 0)
    perror("ERROR reading from socket");
  int len;
  memcpy(&len, b, 4);
  unsigned char enc[MAX_DATA];
  bzero(enc, MAX_DATA);
  int rd = 0;
  while(rd < len) {
    unsigned char buf[MAX_DATA];
    bzero(buf, MAX_DATA);
    int tr = (len - rd > MAX_DATA) ? MAX_DATA : len - rd;
    n = read(sockfd, buf, tr);
    memcpy(enc + rd, buf, n);
    rd += n;
  }

  if(len == 0) {
    printf("Socket error\n");
    return receive(type, message);
  }

  byte * thiskey;
  if(b[4] == toggle)
    thiskey = key;
  else
    thiskey = old_key;

  CryptoPP::AES::Decryption aesDecryption(thiskey, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption( aesDecryption, iv );
  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink( data ) );
  stfDecryptor.Put( reinterpret_cast<const unsigned char*>( enc ), len );
  stfDecryptor.MessageEnd();

  string::size_type t = data.find(":", 0);
  if(t == string::npos)
    perror("error");

  string msg = data.substr(1, t - 1);
  if(data.at(0) != type || msg != message) {
    buffer_push(data);
    return receive(type, message);
  }

  return data.substr(t + 1);
}

string Transporter::receive_data(string message) {
  string data = receive('d', message);
  if(data == "")
    return "";
  string::size_type t = data.find(":", 0);
  if(t == string::npos)
    perror("error");
  return data.substr(t + 1);
}

void Transporter::receive_fast(string message, const char * format, ...) {
  string data = receive('c', message);
  va_list args;
  va_start(args, format);
  vsscanf(data.c_str(), format, args);
  va_end(args);
}

void Transporter::send_fast(string message, const char * format, ...) {
  char data[1024];
  va_list args;
  va_start(args, format);
  vsprintf(data, format, args);
  va_end(args);
  string str = data;
  send_string(message, data);
}

void Transporter::buffer_push(string data) {
  for(int i = 0; i < MAX_BUFFER_SIZE; i++) {
    if(buffer[i] == "") {
      buffer[i] = data;
      return;
    }
  }
  perror("buffer full");
}

string Transporter::buffer_pull(char type, string message) {
  for(int i = 0; i < MAX_BUFFER_SIZE; i++) {
    if(buffer[i] == "")
      return "";
    if(buffer[i].at(0) == type) {
      string::size_type t = buffer[i].find(":", 0);
      if(t == string::npos)
        perror("error");
      string msg = buffer[i].substr(1, t - 1);
      if(msg == message) {
        string out = buffer[i];
        buffer[i] = "";
        return out;
      }
    }
  }
  return "error";
}

void Transporter::renew_key() {
  key_valid = KEY_CHANGE_INTERVAL + 1;
  send_string("newkey", "");
  string newkey = receive('k', "newkey");
  toggle = 1 - toggle;
  for(int i = 0; i < 16; i++) {
    old_key[i] = key[i];
    key[i] = (unsigned char)newkey.at(i);
  }
}
#endif
