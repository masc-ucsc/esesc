// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "Reader.h"
#include "DInst.h"
#include "Instruction.h"
#include "SescConf.h"
#include "ThreadSafeFIFO.h"

ThreadSafeFIFO<RAWDInst> *Reader::tsfifo                   = NULL;
pthread_mutex_t *         Reader::tsfifo_snd_mutex         = NULL;
volatile int *            Reader::tsfifo_snd_mutex_blocked = 0;
pthread_mutex_t           Reader::tsfifo_rcv_mutex;
volatile int              Reader::tsfifo_rcv_mutex_blocked;
EmuDInstQueue *           Reader::ruffer = NULL;

FlowID Reader::nemul = 0;

Reader::Reader(const char *section) {
  if(tsfifo == NULL) {
    // Shared through all the objects, but sized with the # cores

    nemul = SescConf->getRecordSize("", "cpuemul");

    tsfifo = new ThreadSafeFIFO<RAWDInst>[nemul];

    // On thread for each tsfifo sender
    tsfifo_snd_mutex         = new pthread_mutex_t[nemul];
    tsfifo_snd_mutex_blocked = new int[nemul];
    for(int i = 0; i < nemul; i++) {
      pthread_mutex_init(&tsfifo_snd_mutex[i], NULL);
      pthread_mutex_lock(&tsfifo_snd_mutex[i]);
      tsfifo_snd_mutex_blocked[i] = 0;
    }

    // Receive has only 1 thread (ESESC SIMU Thread)
    tsfifo_rcv_mutex_blocked = 0;
    pthread_mutex_init(&tsfifo_rcv_mutex, NULL);
    pthread_mutex_lock(&tsfifo_rcv_mutex);

    ruffer = new EmuDInstQueue[nemul];
  }
}
