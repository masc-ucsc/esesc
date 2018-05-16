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

#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string>

class Report {
private:
  static const int32_t MAXREPORTSTACK = 32;

  static FILE *      rfd[MAXREPORTSTACK];
  static const char *fns[MAXREPORTSTACK];
  static int32_t     tos;

  // static FILE *createTmp(const char *name);
  Report();

public:
  // Creates a new report file. Notice that if the name has the syntax
  // .XXXXXX it would modify the XXXXXX for the final file name.
  static void        openFile(const char *name);
  static void        field(int32_t fn, const char *format, ...);
  static const char *getNameID();
  static void        field(const char *format, ...);
  static void        close();
  static void        flush();
};

// Report::field("bla bla bla:",a);

#endif // REPORT_H
