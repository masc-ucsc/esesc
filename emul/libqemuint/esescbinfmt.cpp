/*
 ESESC: Enhanced Super ESCalar simulator
 Copyright (C) 2010 University of California, Santa Cruz.

 Contributed by Jose Renau

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <regex.h>
#include <sys/types.h>

#include <sys/time.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "Config.h"

std::vector<regex_t> model;

Config *conf = 0;

Config *loadConf()
/* load esescbinfmt configuration {{{1 */
{
  const char *home = getenv("HOME");
  if(home == 0) {
    fprintf(stderr, "ERROR: could not get your home directory\n");
    exit(-2);
  }

  char cadena[1024];
  sprintf(cadena, "%s/.esescbinfmt/config", home);

  Config *conf = new Config(cadena, "ESESCBINFMT");
  if(conf == 0) {
    fprintf(stderr, "ERROR: could not open esescbinfmt configuration file [%s]\n", cadena);
    exit(-1);
  }

  ssize_t min = conf->getRecordMin("", "model");
  ssize_t max = conf->getRecordMax("", "model");

  model.resize(max - min + 1);
  for(size_t i = 0; i < model.size(); i++) {
    const char *regexp = conf->getCharPtr("", "model", min + i);
    int         res    = regcomp(&model[i], regexp, REG_ICASE | REG_EXTENDED);
    if(res != 0) {
      char err_msg[2048];
      regerror(res, &model[i], err_msg, 2048);
      fprintf(stderr, "ERROR: could not compile the regular expression [%s] : %s\n", regexp, err_msg);
      exit(-2);
    }
  }

  return conf;
}
/* }}} */

void createLogEntry(const char *binary, char *argv[])
/* create Log entry {{{1 */
{
  char cadena[1024];

  time_t start_time;
  time(&start_time);

  const char *user = getenv("USER");
  const char *home = getenv("HOME");

  ctime_r(&start_time, cadena);
  cadena[strlen(cadena) - 1] = 0; // remove the \n

  size_t cadena2_max = 4096 + 256 * 32;
  char   cadena2[cadena2_max + 256];
  sprintf(cadena2, "%s:%s:%s: %s ", cadena, user, home, binary);
  for(int i = 0; argv[i] != 0; i++) {
    if(strlen(cadena2) + strlen(argv[i]) > cadena2_max) {
      fprintf(stderr, "ERROR: too many arguments, over %zd bytes\n", cadena2_max);
      exit(-1);
    }
    strcat(cadena2, " ");
    strcat(cadena2, argv[i]);
  }
  strcat(cadena2, "\n");

  int cadena2_len = strlen(cadena2);

  char logname[1024];
  sprintf(logname, "%s/.esescbinfmt/log", home);

  // Open with append permission and close fast because NSF does not guarantee being race free
  int fd = open(logname, O_CREAT | O_WRONLY | O_APPEND, S_IWUSR | S_IRUSR);
  if(fd < 0) {
    fprintf(stderr, "ERROR: Impossible to create %s\n", logname);
    exit(-1);
  }
  int res = write(fd, cadena2, cadena2_len);
  close(fd);
  if(res != cadena2_len) {
    fprintf(stderr, "ERROR: Incorrect loggin?? [%d vs %d]\n", res, cadena2_len);
    exit(-1);
  }
}
/* }}} */

void runapp(const char *newroot, const char *esescbin, const char *qemubin, char *newargv[])
/* run the app if it is in the list of apps {{{1 */
{
  bool domodel = false;

  for(size_t i = 0; i < model.size(); i++) {
    if(regexec(&model[i], newargv[1], 0, 0, 0) == 0) {
      domodel = true;
      break;
    }
  }
  if(domodel)
    createLogEntry(esescbin, newargv);
  else {
    createLogEntry(qemubin, newargv);
  }

  if(fork() != 0) {
    int stat;
    wait(&stat);
    exit(0);
  }

  if(newroot) {
    int res = chroot(newroot);
    if(res < 0) {
      fprintf(stderr, "ERROR: could not chroot to %s\n", newroot);
    }
  }

  if(domodel)
    execvp(esescbin, newargv);
  else {
    execvp(qemubin, newargv);
  }
}
/* }}} */

void setupQEMU(int argc, const char *argv[])
/* setup qemu binary and path {{{1 */
{
  const char *qsection = conf->getCharPtr("", "qemu");
  if(qsection == 0) {
    fprintf(stderr, "ERROR: esescbinfmt does not have an qemu section\n");
    exit(-2);
  }

  const char *esescconf = getenv("ESESCCONF");
  if(esescconf == 0) {
    esescconf = conf->getCharPtr(qsection, "esescconf");
  }
  if(esescconf == 0) {
    fprintf(stderr, "ERROR: esescbinfmt needs esescconf to run your benchmark\n");
    exit(-2);
  }

  const char *newroot = 0;
  if(conf->checkCharPtr(qsection, "chroot"))
    newroot = conf->getCharPtr(qsection, "chroot");

  const char *esescbin = conf->getCharPtr(qsection, "esescbin");
  if(esescbin == 0) {
    fprintf(stderr, "ERROR: esescbinfmt does not have an esescbin field for the simulation\n");
    exit(-2);
  }
  const char *qemubin = 0;
  if(strstr(argv[0], "arm"))
    qemubin = "/usr/local/esesc/qemu-arm";
  else if(strstr(argv[0], "sparc"))
    qemubin = "/usr/local/esesc/qemu-sparc";
  else
    qemubin = "/usr/bin/time";

  if(!conf->lock()) {
    fprintf(stderr, "ERROR: there were errors loading the esescbinfmt configuration\n");
    exit(-2);
  }

  char *newargv[argc + 4];
  for(int i = 0; i < argc; i++) {
    newargv[i] = (char *)alloca(strlen(argv[i]) + 4);
    strcpy(newargv[i], argv[i]);
  }

  newargv[argc] = 0;

  runapp(newroot, esescbin, qemubin, newargv);
}
/* }}} */

int main(int argc, const char *argv[]) {
  conf = loadConf();

  setupQEMU(argc, argv);

  return 0;
}
