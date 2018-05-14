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

#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <alloca.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Report.h"
#include "SescConf.h"
#include "nanassert.h"

FILE *      Report::rfd[MAXREPORTSTACK];
const char *Report::fns[MAXREPORTSTACK];
int32_t     Report::tos = 0;

Report::Report() {
  rfd[0] = stdout;
  fns[0] = "stdout"; //?
  tos    = 1;
}

const char *Report::getNameID() {
  // return fname (last one, notice that there is a stack), use top rfd)
  return fns[tos - 1];
}

void Report::openFile(const char *name) {

  I(tos < MAXREPORTSTACK);

  FILE *ffd;
  char *fname = NULL;
  if(strstr(name, "XXXXXX")) {
    int32_t fd;

    fname = strdup(name);
    fd    = mkstemp(fname);
    if(fd == -1) {
      perror("Report::openFile could not assign file name:");
      exit(-1);
    }

    // FIXME: remember the fname so that getNameID

    ffd = fdopen(fd, "a");
  } else {
    ffd = fopen(name, "a");
  }

  if(ffd == 0) {
    fprintf(stderr, "NANASSERT::REPORT could not open temporary file [%s]\n", name);
    exit(-3);
  }

  fns[tos]   = fname;
  rfd[tos++] = ffd;
}

void Report::close() {
  if(tos) {
    tos--;
    fclose(rfd[tos]);
  }
}

void Report::field(int32_t fn, const char *format, ...) {
  va_list ap;

  I(fn < tos);
  FILE *ffd = rfd[fn];

  va_start(ap, format);

  vfprintf(ffd, format, ap);

  va_end(ap);

  fprintf(ffd, "\n");
}

void Report::field(const char *format, ...) {
  FILE *ffd = NULL;

  I(tos);
  ffd = rfd[tos - 1];

  va_list ap;
  va_start(ap, format);
  vfprintf(ffd, format, ap);
  va_end(ap);

  fprintf(ffd, "\n");
}

void Report::flush() {
  if(tos == 0)
    return;

  fflush(rfd[tos - 1]);
}
