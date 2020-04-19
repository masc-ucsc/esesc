ESESC
=====

[![Build Status](https://travis-ci.org/masc-ucsc/esesc.png)](https://travis-ci.org/masc-ucsc/esesc)

[![CodeFactor](https://www.codefactor.io/repository/github/masc-ucsc/esesc/badge)](https://www.codefactor.io/repository/github/masc-ucsc/esesc)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/4cae3de3de714e13b6003002f74b7375)](https://www.codacy.com/app/renau/lgraph?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=masc-ucsc/lgraph&amp;utm_campaign=Badge_Grade)

* [ESESC Documentation](./docs)

To discuss about ESESC, there is a gitter channel (the mailing list are being deprecated)

[![Gitter](https://badges.gitter.im/esesc/community.svg)](https://gitter.im/esesc/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

ESESC: A Fast Multicore Simulator
---------------------------------

What is ESESC?

ESESC is a fast multiprocessor simulator with detailed power, thermal, and performance models for modern out-of-order multicores. ESESC is an evolution of the popular [SESC](http://sesc.sourceforge.net/) simulator (Enhanced SESC) that provides many new features.

The main ESESC characteristics are the following:

* It is very fast (over 40MIPS with sampling)
* Uses QEMU and supports user mode RISCV and MIPS64r6 ISA
* Models OoO and InOrder cores in detail (ROB, Instruction Window, etc)
* Supports configurable memory hierarchy, and on-chip memory controller
* Supports multicore, homogeneous and heterogeneous configurations
* Simulates multithreaded and multiprogram applications
* Models power and temperature in addition to performance, and their interactions

ESESC is a significant evolution/improvement over [SESC](http://sesc.sourceforge.net/):

* ESESC has RISCV and MIPS64r6 ISA, sesc had MIPS ISA.
* ESESC can run unmodified Linux RISCV and MIPS binaries, sesc required a custom toolchain.
* ESESC uses QEMU for emulation, sesc had a custom emulator.
* ESESC is integrated with McPat, sesc had an older Wattch model.
* ESESC has a brand new memory hierarchy, sesc had a more complex coherence.
* ESESC has improved thermal modeling, sesc had HotSpot
* ESESC has many types of sampling (statistical, smarts, simpoint), sesc had none.
* ESESC is actively maintained, sesc is no longer mantained.
* ESESC has many bugs solved.

For more information on running ESESC see the [docs](./docs) directory. Use the gitter discussion group for questions.


If you publish research using ESESC please cite the paper [ESESC: A Fast Multicore Simulator Using Time-Based Sampling][3] from HPCA 2013.

    @INPROCEEDINGS{esesc,
      author = {K. Ardestani, E. and Renau, J.},
      title = { {ESESC: A Fast Multicore Simulator Using Time-Based Sampling} },
      booktitle = {International Symposium on High Performance Computer Architecture},
      series = {HPCA'19},
      year = {2013}
    }


[3]:http://masc.soe.ucsc.edu/docs/hpca13.pdf

