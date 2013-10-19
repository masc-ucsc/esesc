/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Linux VT.
  Comments:
  Helper to find a free VT. That's 100% Linux specific.@p
  The code comes from "lconsole.c" from Allegro project and was originally
created by Marek Habersack and then modified by George Foot. I addapted it
to my needs and changed license from giftware to GPL.@p
  
***************************************************************************/

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __APPLE__
#include <util.h>
#endif /* __APPLE__ */

#include "mi_gdb.h"

#if !defined(__linux__)

int mi_look_for_free_vt()
{
 return -1;
}

mi_aux_term *gmi_look_for_free_vt()
{
 return NULL;
}

#else

#include <linux/vt.h>

/**[txh]********************************************************************

  Description:
  Look for a free and usable Linux VT. Low level, use
@x{gmi_look_for_free_vt}.
  
  Return: The VT number or <0 on error.
  
***************************************************************************/

int mi_look_for_free_vt()
{/* Code from Allegro. */
 int tty, console_fd, fd;
 unsigned short mask;
 char tty_name[16];
 struct vt_stat vts; 

 /* Now we need to find a VT we can use.  It must be readable and
  * writable by us, if we're not setuid root.  VT_OPENQRY itself
  * isn't too useful because it'll only ever come up with one 
  * suggestion, with no guarrantee that we actually have access 
  * to it.
  *
  * At some stage I think this is a candidate for config
  * file overriding, but for now we'll stat the first N consoles
  * to see which ones we can write to (hopefully at least one!),
  * so that we can use that one to do ioctls.  We used to use 
  * /dev/console for that purpose but it looks like it's not 
  * always writable by enough people.
  *
  * Having found and opened a writable device, we query the state
  * of the first sixteen (fifteen really) consoles, and try 
  * opening each unused one in turn.
  */

 console_fd=open("/dev/console",O_WRONLY);
 if (console_fd<0)
   {
    int n;
    /* Try some ttys instead... */
    for (n=1; n<=24; n++)
       {
        snprintf(tty_name,sizeof(tty_name),"/dev/tty%d",n);
        console_fd=open(tty_name,O_WRONLY);
        if (console_fd>=0)
           break;
       }
    if (n>24)
       return -1;
   }

 /* Get the state of the console -- in particular, the free VT field */
 if (ioctl(console_fd,VT_GETSTATE,&vts))
    return -2;
 close(console_fd);

 /* We attempt to set our euid to 0; if we were run with euid 0 to
  * start with, we'll be able to do this now.  Otherwise, we'll just
  * ignore the error returned since it might not be a problem if the
  * ttys we look at are owned by the user running the program. */
 seteuid(0);

 /* tty0 is not really a console, so start counting at 2. */
 fd=-1;
 for (tty=1, mask=2; mask; tty++, mask<<=1)
     if (!(vts.v_state & mask))
       {
        snprintf(tty_name,sizeof(tty_name),"/dev/tty%d",tty);
        fd=open(tty_name,O_RDWR);
        if (fd!=-1)
          {
           close(fd);
           break;
          }
       }

 seteuid(getuid());

 if (!mask)
    return -3;

 return tty;
}

/**[txh]********************************************************************

  Description:
  Look for a free and usable Linux VT to be used by the child.
  
  Return: A new mi_aux_term structure, you can use @x{gmi_end_aux_term} to
release it.
  
***************************************************************************/

mi_aux_term *gmi_look_for_free_vt()
{
 int vt=mi_look_for_free_vt();
 mi_aux_term *res;

 if (vt<0)
    return NULL;
 res=(mi_aux_term *)malloc(sizeof(mi_aux_term));
 if (!res)
    return NULL;
 res->pid=-1;
 asprintf(&res->tty,"/dev/tty%d",vt);
 return res;
}

#endif

