/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: pseudo terminal
  Comments:
  Helper to find a free pseudo terminal. Use this if you need to manage
  input *and* output to the target process. If you just need output then
  define a handler for target output stream records (assuming that this
  is working for your particular version of gdb).
  Usage:

        mi_pty *pty = gmi_look_for_free_pty();
        if (pty) gmi_target_terminal(mih, pty->slave);
        ...
        * reading from pty->master will get stdout from target *
        * writing to pty->master will send to target stdin *
        
  Note: Contributed by Greg Watson (gwatson lanl gov)

***************************************************************************/

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "mi_gdb.h"

/**[txh]********************************************************************

  Description:
  Look for a free and usable pseudo terminal. Low level, use
@x{gmi_look_for_free_pty}.
  
  Return: A file descriptor connected to the master pty and the name of the slave device, or <0 on error.
  
***************************************************************************/

#ifdef __APPLE__

#include <util.h>

int mi_look_for_free_pty(int *master, char **slave)
{
 int fdmaster;
 int fdslave;
 static char name[BUFSIZ];

 if (openpty(&fdmaster,&fdslave,name,NULL,NULL)<0)
    return -1;

 (void)close(fdslave); /* this will be reopened by gdb */
 *master=fdmaster;
 *slave =name;

 return 0;
}

#elif defined(__linux__)

int mi_look_for_free_pty(int *master, char **slave)
{
 if ((*master=open("/dev/ptmx",O_RDWR))<0)
    return -1;
 if (grantpt(*master)<0 || unlockpt(*master)<0)
    return -1;
 *slave = ptsname(*master);

 return 0;
}

#else /* undefined o/s */

int mi_look_for_free_pty(int *master, char **slave)
{
 return -1;
}
#endif

/**[txh]********************************************************************

  Description:
  Look for a free and usable pseudo terminal to be used by the child.
  
  Return: A new mi_pty structure, you can use @x{gmi_end_pty} to
release it.
  
***************************************************************************/

mi_pty *gmi_look_for_free_pty()
{
 int master;
 char *slave;
 int pty=mi_look_for_free_pty(&master,&slave);
 mi_pty *res;

 if (pty<0)
    return NULL;
 res=(mi_pty *)malloc(sizeof(mi_pty));
 if (!res)
    return NULL;
 res->slave=strdup(slave);
 res->master=master;
 return res;
}

void mi_free_pty(mi_pty *p)
{
 if (!p)
    return;
 free(p->slave);
 free(p);
}

/**[txh]********************************************************************

  Description:
  Closes the pseudo termial master and releases the allocated memory.
  
***************************************************************************/

void gmi_end_pty(mi_pty *p)
{
 if (!p)
    return;
 close(p->master);
 mi_free_pty(p);
}
