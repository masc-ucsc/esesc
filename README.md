ESESC
=====

* [ESESC Forum][1]
* [ESESC Blog][2]
* [ESESC Documentation](./docs)

ESESC: A Fast Multicore Simulator
---------------------------------

What is ESESC?

ESESC is a fast multiprocessor simulator with detailed power, thermal, and performance models for modern out-of-order multicores. ESESC is an evolution of the popular [SESC](http://sesc.sourceforge.net/) simulator (Enhanced SESC) that provides many new features.

The main ESESC characteristics are the following:

* It is very fast (over 20MIPS with sampling)
* Uses QEMU and supports user mode MIPS64r6 ISA
* Models OoO and InOrder cores in detail (ROB, Instruction Window, etc)
* Supports configurable memory hierarchy, and on-chip memory controller
* Supports multicore, homogeneous and heterogeneous configurations
* Simulates multithreaded and multiprogram applications
* Models power and temperature in addition to performance, and their interactions

ESESC is a significant evolution/improvement over [SESC](http://sesc.sourceforge.net/):

* ESESC has MIPS64r6 ISA, sesc had MIPS ISA.
* ESESC can run unmodified Linux MIPS binaries, sesc required a custom toolchain.
* ESESC uses QEMU for emulation, sesc had a custom emulator.
* ESESC is integrated with McPat, sesc had an older Wattch model.
* ESESC has a brand new memory hierarchy, sesc had a more complex coherence.
* ESESC has improved thermal modeling, sesc had HotSpot
* ESESC has many types of sampling (statistical, smarts, simpoint), sesc had none.
* ESESC is actively maintained, sesc is no longer mantained.
* ESESC has many bugs solved.

For more information on running ESESC see the [docs](./docs) directory.  There is also an [ESESC blog][1] where updates, announcements, and uses cases are posted.  And a [forum][2] to ask questions and discuss using ESESC.

If you publish research using ESESC please cite the paper [ESESC: A Fast Multicore Simulator Using Time-Based Sampling][3] from HPCA 2013.

    @INPROCEEDINGS{esesc,
      author = {K. Ardestani, E. and Renau, J.},
      title = { {ESESC: A Fast Multicore Simulator Using Time-Based Sampling} },
      booktitle = {International Symposium on High Performance Computer Architecture},
      series = {HPCA'19},
      year = {2013}
    }

[1]:https://groups.google.com/forum/#!forum/esesc 
[2]:http://masc.cse.ucsc.edu/esesc
[3]:http://masc.soe.ucsc.edu/docs/hpca13.pdf

