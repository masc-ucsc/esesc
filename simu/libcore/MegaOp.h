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

#ifndef MEGAOP_H
#define MEGAOP_H

#include <vector>
#include <algorithm>
#include <set>

#include "DInst.h"
#include "Instruction.h"
#include "GStats.h"

class MegaOp {
  private:

  public:
    bool ready;               // TODO: Remove?
    uint32_t age;
    uint32_t numImm;
    InstOpcode opcode;
    
    MegaOp *parentOp;

    std::list<DInst> instrs;
    std::set<AddrType> pcs;
    std::set<RegType> srcs;
    std::set<RegType> dsts;

    MegaOp(InstOpcode);
    MegaOp(DInst);
    
    int addMegaOp(MegaOp);
    
    bool matchSrcs(std::set<RegType>);
    bool matchDsts(std::set<RegType>);
    
    int countSrcs() const { return srcs.size(); }
    int countDsts() const { return dsts.size(); }
    int countInstrs() const { return instrs.size(); }

    static MegaOp merge(std::list<MegaOp> &list);

    void update();
    void renameSrcs(RegType, RegType);
    void renameDsts(RegType, RegType);
    void pruneTmpRegs();
    void visualize(std::string);
};

#endif // MEGAOP_H



