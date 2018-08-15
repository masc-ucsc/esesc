/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by David Munday

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

#include "StoreSet.h"
#include "SescConf.h"

/* }}} */

StoreSet::StoreSet(const int32_t id)
    /* constructor {{{1 */
    : StoreSetSize(SescConf->getInt("cpusimu", "StoreSetSize", id))
#ifdef STORESET_CLEARING
    , clearStoreSetsTimerCB(this)
#endif
{

  SescConf->isPower2("cpusimu", "StoreSetSize", id);

  SSIT.resize(StoreSetSize);
  LFST.resize(StoreSetSize);
  clear_SSIT();
  clear_LFST();

#ifdef STORESET_CLEARING
  clear_interval = CLR_INTRVL;
  Time_t when    = clear_interval + globalClock;
  if(when >= (globalClock * 2))
    when = globalClock * 2 - 1; // To avoid assertion about possible bug. Long enough anyway
  clearStoreSetsTimerCB.scheduleAbs(when);
#endif
}
/* }}} */

SSID_t StoreSet::create_id() {
  static SSID_t rnd = 0;
  SSID_t        SSID;
#if 1
  if(rnd < StoreSetSize) {
    SSID = rnd;
    rnd++;
  } else {
    rnd  = 0;
    SSID = rnd;
  }
#else
  SSID = (PC % 32671) & (StoreSetSize - 1);
  // rnd += 1021; // prime number
#endif

  return SSID;
}
SSID_t StoreSet::create_set(AddrType PC)
/* create_set {{{1 */
{
  SSID_t SSID = create_id();

  ID(SSID_t oldSSID = get_SSID(PC));
  // I(LFST[SSID]==0);

  set_SSID(PC, SSID);
  LFST[SSID] = NULL;

  I(SSID < StoreSetSize);
  IS(if(isValidSSID(oldSSID)) { LFST[oldSSID] = NULL; }); // debug only

  return SSID;
}
/* }}} */
#ifdef STORESET_MERGING
void StoreSet::merge_sets(DInst *m_dinst, DInst *d_dinst)
/* merge two loads into the src LD's set {{{1 */
{
  AddrType merge_this_set_pc   = m_dinst->getPC(); // <<1 + m_dinst->getUopOffset();
  AddrType destroy_this_set_pc = d_dinst->getPC(); // <<1 + d_dinst->getUopOffset();
  SSID_t   merge_SSID          = get_SSID(merge_this_set_pc);
  if(!isValidSSID(merge_SSID))
    merge_SSID = create_set(merge_this_set_pc);

  set_SSID(destroy_this_set_pc, merge_SSID);
}
/* }}} */
#endif
void StoreSet::clear_SSIT()
/* Clear all the SSIT entries {{{1 */
{
  for(int i = 0; i < StoreSetSize; i++) {
    SSIT[i] = -1;
  }
}
/* }}} */

void StoreSet::clear_LFST(void)
/* clear all the LFST entries {{{1 */
{
  for(size_t i = 0; i < LFST.size(); i++)
    LFST[i] = NULL;
}
/* }}} */

#ifdef STORESET_CLEARING
void StoreSet::clearStoreSetsTimer()
/* periodic cleanup of the LSFT and SSIT {{{1 */
{
  // MSG("------------- CLEAR -----------------");
  clear_SSIT();
  clear_LFST();
  Time_t when = clear_interval + globalClock;
  if(when >= (globalClock * 2))
    when = globalClock * 2 - 1; // To avoid assertion about possible bug. Long enough anyway
  clearStoreSetsTimerCB.scheduleAbs(when);
}
/* }}} */
#endif

bool StoreSet::insert(DInst *dinst)
/* insert a store/load in the store set {{{1 */
{
  AddrType inst_pc   = dinst->getPC(); // <<1+dinst->getUopOffset();
  SSID_t   inst_SSID = get_SSID(inst_pc);

  if(!isValidSSID(inst_SSID)) {
    dinst->setSSID(-1);
    return true; // instruction does not belong to an existing set.
  }

  const Instruction *inst = dinst->getInst();

  if(inst->isStoreAddress()) {
    printf("Store Address passed to StoreSet insert. exit\n");
    exit(1);
  }
  I(!dinst->isExecuted());
  dinst->setSSID(inst_SSID);

  DInst *lfs_dinst = get_LFS(inst_SSID);
  set_LFS(inst_SSID,
          dinst); // make this instruction the Last Fetched Store(Should be renamed to instruction since loads are included).

  if(lfs_dinst == 0)
    return true;

  I(!lfs_dinst->isExecuted());
  I(!dinst->isExecuted());
  lfs_dinst->addSrc3(dinst);
  MSG("addSST %8ld->%8lld %lld", (long long)lfs_dinst->getID(), (long long)dinst->getID(), (long long)globalClock);

  return true;
}
/* }}} */

void StoreSet::remove(DInst *dinst)
/* remove a store from store sets {{{1 */
{
  I(!dinst->getInst()->isStoreAddress());

  SSID_t inst_SSID = dinst->getSSID();

  if(!isValidSSID(inst_SSID))
    return;

  DInst *lfs_dinst = get_LFS(inst_SSID);

  if(dinst == lfs_dinst) {
    I(isValidSSID(inst_SSID));
    set_LFS(inst_SSID, 0);
  }
}
/* }}} */

void StoreSet::stldViolation(DInst *ld_dinst, AddrType st_pc)
/* add a new st/ld violation {{{1 */
{
  return; // FIXME: no store set
  I(ld_dinst->getInst()->isLoad());

  AddrType ld_pc   = ld_dinst->getPC();
  SSID_t   ld_SSID = ld_dinst->getSSID();
  if(!isValidSSID(ld_SSID)) {
    ld_SSID = create_set(ld_pc);
  }
  set_SSID(st_pc, ld_SSID);
}
/* }}} */

void StoreSet::stldViolation(DInst *ld_dinst, DInst *st_dinst)
/* add a new st/ld violation {{{1 */
{
  stldViolation(ld_dinst, st_dinst->getPC());
}
/* }}} */

void StoreSet::stldViolation_withmerge(DInst *ld_dinst, DInst *st_dinst)
/* add a new st/ld violation {{{1 */
{
  I(st_dinst->getInst()->isStore());
  I(ld_dinst->getInst()->isLoad());

  AddrType ld_pc   = ld_dinst->getPC();   // <<1 + ld_dinst->getUopOffset();
  AddrType st_pc   = st_dinst->getPC();   // <<1 + st_dinst->getUopOffset();
  SSID_t   ld_SSID = ld_dinst->getSSID(); // get_SSID(ld_pc);
  SSID_t   st_SSID = st_dinst->getSSID(); // get_SSID(ld_pc);
  if(!isValidSSID(ld_SSID)) {
    ld_SSID = create_set(ld_pc);
  }
  if(isValidSSID(st_SSID)) {
    if(st_SSID != ld_SSID) {
      for(int i = 0; i < StoreSetSize; i++) { // iterate SSIT, move all PCs from old set to THIS LD's set.
        if(SSIT[i] == st_SSID)
          SSIT[i] = ld_SSID;
      }
      LFST[st_SSID] = NULL; // Wipe out any pending LFST for the destroyed set.
    }
  }
  set_SSID(st_pc, ld_SSID);
}
/* }}} */

SSID_t StoreSet::mergeset(SSID_t id1, SSID_t id2)
/* add a new st/ld violation {{{1 */
{
  if(id1 == id2)
    return id1;

  // SSID_t newid = create_id();
  SSID_t newid;
  if(id1 < id2)
    newid = id1;
  else
    newid = id2;

  I(id1 != -1);
  I(id2 != -1);

  set_LFS(id1, 0);
  set_LFS(id2, 0);

  for(int i = 0; i < StoreSetSize; i++) { // iterate SSIT, move all PCs from old set to THIS LD's set.
    if(SSIT[i] == id1 || SSIT[i] == id2)
      SSIT[i] = newid;
  }

  return newid;
}
/* }}} */

void StoreSet::VPC_misspredict(DInst *ld_dinst, AddrType st_pc)
/* add a new st/ld violation {{{1 */
{
  I(st_pc);
  I(ld_dinst->getInst()->isLoad());
  AddrType ld_pc   = ld_dinst->getPC();   // <<1 + ld_dinst->getUopOffset();
  SSID_t   ld_SSID = ld_dinst->getSSID(); // get_SSID(ld_pc);
  if(!isValidSSID(ld_SSID)) {
    ld_SSID = create_set(ld_pc);
  }
  set_SSID(st_pc, ld_SSID);
}
/* }}} */

void StoreSet::assign_SSID(DInst *dinst, SSID_t target_SSID) {
  /* force this dinst to join the specified set (for merging) {{{1 */
  SSID_t   inst_SSID = dinst->getSSID();
  AddrType inst_pc   = dinst->getPC();
  if(isValidSSID(inst_SSID)) {
    DInst *lfs_dinst = get_LFS(inst_SSID);
    if(lfs_dinst != NULL) {
      // if(dinst->getID() == lfs_dinst->getID()){ //is this store or load the most recent from the set
      // if(dinst->getpersistentID() == lfs_dinst->getpersistentID()){ //is this store or load the most recent from the set
      if((lfs_dinst == NULL) || (lfs_dinst == dinst)) {
        if(inst_pc == lfs_dinst->getPC())
          set_LFS(inst_SSID, 0); // remove self from LFS to prevent a deadlock after leaving this set
      }
    }
  }
  set_SSID(inst_pc, target_SSID);
  dinst->setSSID(target_SSID);
}
/* }}} */
