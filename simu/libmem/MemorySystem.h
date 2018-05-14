/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Basilio Fraguela
                  Jose Renau
                  James Tuck
                  Smruti Sarangi

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef MEMORYSYSTEM_H
#define MEMORYSYSTEM_H

#include "GMemorySystem.h"
#include "nanassert.h"
/* }}} */

class MemObj;

class MemorySystem : public GMemorySystem {
private:
protected:
  virtual MemObj *buildMemoryObj(const char *type, const char *section, const char *name);

public:
  MemorySystem(int32_t processorId);
};

#endif
