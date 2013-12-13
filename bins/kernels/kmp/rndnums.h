/* rnds.h - Random Numbers
 *
 * Public Domain Software
 * 1991 by Larry Smith
 */

#ifndef RNDNUMS
#define RNDNUMS

#define MILL struct rndmill

MILL   *init_mill(unsigned long seed1,unsigned long seed2,unsigned long seed3);
void    nuke_mill(MILL *src);

unsigned long rndnum(MILL *src);

MILL   *checkpoint_mill(MILL *src);
void    reset_mill(MILL *src, MILL *chkpt);

#endif
