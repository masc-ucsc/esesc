/*
 *  sesc/scoore interface
 *
 *  Copyright (c) 2006 Jose Renau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef QEMU_SESC_H
#define QEMU_SESC_H

#include "SPARCInstruction.h"
#include <pthread.h>

#define SESC_NCPUS 4
#define QEMU_SESC_INST_SIZE 512

typedef struct qemu_esesc_inst {
  Sparc_Inst_Predecode predec;

  unsigned short cpuid : 16;
  unsigned char  new_bb : 1;
  unsigned char  fake : 1;
  unsigned char  finalize : 1;

  // Values on registers
  unsigned int data_address;
  unsigned int regdest;
} qemu_esesc_inst;

#ifdef __cplusplus
extern "C" {
#endif

extern pthread_mutex_t qemu_sesc_init_lock;
extern pthread_cond_t  qemu_sesc_init_cond;
extern long long       n_inst;
extern long long       n_inst_start;

void sesc_locks_init(void);

void sesc_finish(void);

void sesc_client_init(void);
void sesc_client_finish(void);

void sesc_server_init(void);
void sesc_server_finish(void);

int          sesc_main(int argc, char **argv);
unsigned int sesc_last_pc(short cpuid);

int qemu_loader(int argc, char **argv);
int sesc_cmd_advance_pc(struct qemu_sesc_te *qinst, int ninst, short cpuid);

void activateCPU(int cpuid);

#ifdef CONFIG_SCOORE
void scoore_client_init(void);
void scoore_client_finish(void);
int  scoore_fill_next_icache_line(struct qemu_sesc_te *line, int size);
#endif
#ifdef __cplusplus
}
#endif

#endif
