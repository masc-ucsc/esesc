My ESESC: Syscall sched_setaffinity implemented in this version for Linux
Hi all,

As you know, Syscall sched_setaffinity() is not implemented in original version of ESESC. I means, the simulator doesn't migrate threads in original version (The sched_setaffinity() would be called natively (QEMU) and it will have no impact on ESESC). So, I have modified ESESC where simulator is affected by syscall sched_setaffinity(). In fact, user is able to migrate threads to each simulated core in ESESC, and simulator considers this migration in this version, too.
Users are able to bind threads to the simulated cores by using sched_setaffinity, pthread_setaffinity_np, and any binding commands which are based on sched_setaffinity.
Mechanism: Suppose sched_setaffinity(pid_t tid, size_t cpusetsize, cpu_set_t *mask) intend to bind a thread tid to a core which is determined in mask. The binding is done as following in this fork:
1- If at least one of the CPUs, which is determined by a CPU affinity mask, has a free FID, thread tid will be migrated to the FID.
2- Else, the thread which is running on first suitable FID is migrated to another one, and thread tid will be migrated to it.

I hope this modification helps.

