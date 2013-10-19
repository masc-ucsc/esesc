/*
 ESESC: Enhanced Super ESCalar simulator
 Copyright (C) 2012 University of California, Santa Cruz.

 Contributed by  Ian Lee

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
#pragma once
#ifndef MEGAOPCONFIG_H
#define MEGAOPCONFIG_H

namespace MegaOpConfig {
  const unsigned int maxRegs = 8;     // Max # Src or Dst Regs allowed
  const unsigned int maxPorts = 8;    // Max # unique Regs which can be in the megaOp
  const unsigned int maxInstrs = 16;  // Max # instrs in the megaOp
  const unsigned int maxAge = 16;     // Max Age the megaOp can linger for
};

enum MEGAOPERROR {
  NO_ERROR = 0,
  OPCODE_MISMATCH = -1,
  ALREADY_READY = -2,
  MAXSIZE_EXCEEDED = -3,

};

#endif // MEGAOPCONFIG_H



