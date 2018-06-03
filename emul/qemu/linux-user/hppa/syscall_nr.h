/*
 * This file contains the system call numbers.
 */

#define TARGET_NR_restart_syscall 0
#define TARGET_NR_exit            1
#define TARGET_NR_fork            2
#define TARGET_NR_read            3
#define TARGET_NR_write           4
#define TARGET_NR_open            5
#define TARGET_NR_close           6
#define TARGET_NR_waitpid         7
#define TARGET_NR_creat           8
#define TARGET_NR_link            9
#define TARGET_NR_unlink         10
#define TARGET_NR_execve         11
#define TARGET_NR_chdir          12
#define TARGET_NR_time           13
#define TARGET_NR_mknod          14
#define TARGET_NR_chmod          15
#define TARGET_NR_lchown         16
#define TARGET_NR_socket         17
#define TARGET_NR_stat           18
#define TARGET_NR_lseek          19
#define TARGET_NR_getpid         20
#define TARGET_NR_mount          21
#define TARGET_NR_bind           22
#define TARGET_NR_setuid         23
#define TARGET_NR_getuid         24
#define TARGET_NR_stime          25
#define TARGET_NR_ptrace         26
#define TARGET_NR_alarm          27
#define TARGET_NR_fstat          28
#define TARGET_NR_pause          29
#define TARGET_NR_utime          30
#define TARGET_NR_connect        31
#define TARGET_NR_listen         32
#define TARGET_NR_access         33
#define TARGET_NR_nice           34
#define TARGET_NR_accept         35
#define TARGET_NR_sync           36
#define TARGET_NR_kill           37
#define TARGET_NR_rename         38
#define TARGET_NR_mkdir          39
#define TARGET_NR_rmdir          40
#define TARGET_NR_dup            41
#define TARGET_NR_pipe           42
#define TARGET_NR_times          43
#define TARGET_NR_getsockname    44
#define TARGET_NR_brk            45
#define TARGET_NR_setgid         46
#define TARGET_NR_getgid         47
#define TARGET_NR_signal         48
#define TARGET_NR_geteuid        49
#define TARGET_NR_getegid        50
#define TARGET_NR_acct           51
#define TARGET_NR_umount2        52
#define TARGET_NR_getpeername    53
#define TARGET_NR_ioctl          54
#define TARGET_NR_fcntl          55
#define TARGET_NR_socketpair     56
#define TARGET_NR_setpgid        57
#define TARGET_NR_send           58
#define TARGET_NR_uname          59
#define TARGET_NR_umask          60
#define TARGET_NR_chroot         61
#define TARGET_NR_ustat          62
#define TARGET_NR_dup2           63
#define TARGET_NR_getppid        64
#define TARGET_NR_getpgrp        65
#define TARGET_NR_setsid         66
#define TARGET_NR_pivot_root     67
#define TARGET_NR_sgetmask       68
#define TARGET_NR_ssetmask       69
#define TARGET_NR_setreuid       70
#define TARGET_NR_setregid       71
#define TARGET_NR_mincore        72
#define TARGET_NR_sigpending     73
#define TARGET_NR_sethostname    74
#define TARGET_NR_setrlimit      75
#define TARGET_NR_getrlimit      76
#define TARGET_NR_getrusage      77
#define TARGET_NR_gettimeofday   78
#define TARGET_NR_settimeofday   79
#define TARGET_NR_getgroups      80
#define TARGET_NR_setgroups      81
#define TARGET_NR_sendto         82
#define TARGET_NR_symlink        83
#define TARGET_NR_lstat          84
#define TARGET_NR_readlink       85
#define TARGET_NR_uselib         86
#define TARGET_NR_swapon         87
#define TARGET_NR_reboot         88
#define TARGET_NR_mmap2          89
#define TARGET_NR_mmap           90
#define TARGET_NR_munmap         91
#define TARGET_NR_truncate       92
#define TARGET_NR_ftruncate      93
#define TARGET_NR_fchmod         94
#define TARGET_NR_fchown         95
#define TARGET_NR_getpriority    96
#define TARGET_NR_setpriority    97
#define TARGET_NR_recv           98
#define TARGET_NR_statfs         99
#define TARGET_NR_fstatfs       100
#define TARGET_NR_stat64        101
#define TARGET_NR_socketcall    102
#define TARGET_NR_syslog        103
#define TARGET_NR_setitimer     104
#define TARGET_NR_getitimer     105
#define TARGET_NR_capget        106
#define TARGET_NR_capset        107
#define TARGET_NR_pread64       108
#define TARGET_NR_pwrite64      109
#define TARGET_NR_getcwd        110
#define TARGET_NR_vhangup       111
#define TARGET_NR_fstat64       112
#define TARGET_NR_vfork         113
#define TARGET_NR_wait4         114
#define TARGET_NR_swapoff       115
#define TARGET_NR_sysinfo       116
#define TARGET_NR_shutdown      117
#define TARGET_NR_fsync         118
#define TARGET_NR_madvise       119
#define TARGET_NR_clone         120
#define TARGET_NR_setdomainname 121
#define TARGET_NR_sendfile      122
#define TARGET_NR_recvfrom      123
#define TARGET_NR_adjtimex      124
#define TARGET_NR_mprotect      125
#define TARGET_NR_sigprocmask   126
#define TARGET_NR_create_module 127
#define TARGET_NR_init_module   128
#define TARGET_NR_delete_module 129
#define TARGET_NR_get_kernel_syms 130
#define TARGET_NR_quotactl      131
#define TARGET_NR_getpgid       132
#define TARGET_NR_fchdir        133
#define TARGET_NR_bdflush       134
#define TARGET_NR_sysfs         135
#define TARGET_NR_personality   136
#define TARGET_NR_afs_syscall   137 /* Syscall for Andrew File System */
#define TARGET_NR_setfsuid      138
#define TARGET_NR_setfsgid      139
#define TARGET_NR__llseek       140
#define TARGET_NR_getdents      141
#define TARGET_NR__newselect    142
#define TARGET_NR_flock         143
#define TARGET_NR_msync         144
#define TARGET_NR_readv         145
#define TARGET_NR_writev        146
#define TARGET_NR_getsid        147
#define TARGET_NR_fdatasync     148
#define TARGET_NR__sysctl       149
#define TARGET_NR_mlock         150
#define TARGET_NR_munlock       151
#define TARGET_NR_mlockall      152
#define TARGET_NR_munlockall    153
#define TARGET_NR_sched_setparam          154
#define TARGET_NR_sched_getparam          155
#define TARGET_NR_sched_setscheduler      156
#define TARGET_NR_sched_getscheduler      157
#define TARGET_NR_sched_yield             158
#define TARGET_NR_sched_get_priority_max  159
#define TARGET_NR_sched_get_priority_min  160
#define TARGET_NR_sched_rr_get_interval   161
#define TARGET_NR_nanosleep     162
#define TARGET_NR_mremap        163
#define TARGET_NR_setresuid     164
#define TARGET_NR_getresuid     165
#define TARGET_NR_sigaltstack   166
#define TARGET_NR_query_module  167
#define TARGET_NR_poll          168
#define TARGET_NR_nfsservctl    169
#define TARGET_NR_setresgid     170
#define TARGET_NR_getresgid     171
#define TARGET_NR_prctl         172
#define TARGET_NR_rt_sigreturn          173
#define TARGET_NR_rt_sigaction          174
#define TARGET_NR_rt_sigprocmask        175
#define TARGET_NR_rt_sigpending         176
#define TARGET_NR_rt_sigtimedwait       177
#define TARGET_NR_rt_sigqueueinfo       178
#define TARGET_NR_rt_sigsuspend         179
#define TARGET_NR_chown         180
#define TARGET_NR_setsockopt    181
#define TARGET_NR_getsockopt    182
#define TARGET_NR_sendmsg       183
#define TARGET_NR_recvmsg       184
#define TARGET_NR_semop         185
#define TARGET_NR_semget        186
#define TARGET_NR_semctl        187
#define TARGET_NR_msgsnd        188
#define TARGET_NR_msgrcv        189
#define TARGET_NR_msgget        190
#define TARGET_NR_msgctl        191
#define TARGET_NR_shmat         192
#define TARGET_NR_shmdt         193
#define TARGET_NR_shmget        194
#define TARGET_NR_shmctl        195
#define TARGET_NR_getpmsg       196
#define TARGET_NR_putpmsg       197
#define TARGET_NR_lstat64       198
#define TARGET_NR_truncate64    199
#define TARGET_NR_ftruncate64   200
#define TARGET_NR_getdents64    201
#define TARGET_NR_fcntl64       202
#define TARGET_NR_attrctl       203
#define TARGET_NR_acl_get       204
#define TARGET_NR_acl_set       205
#define TARGET_NR_gettid        206
#define TARGET_NR_readahead     207
#define TARGET_NR_tkill         208
#define TARGET_NR_sendfile64    209
#define TARGET_NR_futex         210
#define TARGET_NR_sched_setaffinity 211
#define TARGET_NR_sched_getaffinity 212
#define TARGET_NR_set_thread_area   213
#define TARGET_NR_get_thread_area   214
#define TARGET_NR_io_setup          215
#define TARGET_NR_io_destroy        216
#define TARGET_NR_io_getevents      217
#define TARGET_NR_io_submit         218
#define TARGET_NR_io_cancel         219
#define TARGET_NR_alloc_hugepages   220
#define TARGET_NR_free_hugepages    221
#define TARGET_NR_exit_group        222
#define TARGET_NR_lookup_dcookie    223
#define TARGET_NR_epoll_create      224
#define TARGET_NR_epoll_ctl         225
#define TARGET_NR_epoll_wait        226
#define TARGET_NR_remap_file_pages  227
#define TARGET_NR_semtimedop        228
#define TARGET_NR_mq_open           229
#define TARGET_NR_mq_unlink         230
#define TARGET_NR_mq_timedsend      231
#define TARGET_NR_mq_timedreceive   232
#define TARGET_NR_mq_notify         233
#define TARGET_NR_mq_getsetattr     234
#define TARGET_NR_waitid            235
#define TARGET_NR_fadvise64_64      236
#define TARGET_NR_set_tid_address   237
#define TARGET_NR_setxattr          238
#define TARGET_NR_lsetxattr         239
#define TARGET_NR_fsetxattr         240
#define TARGET_NR_getxattr          241
#define TARGET_NR_lgetxattr         242
#define TARGET_NR_fgetxattr         243
#define TARGET_NR_listxattr         244
#define TARGET_NR_llistxattr        245
#define TARGET_NR_flistxattr        246
#define TARGET_NR_removexattr       247
#define TARGET_NR_lremovexattr      248
#define TARGET_NR_fremovexattr      249
#define TARGET_NR_timer_create      250
#define TARGET_NR_timer_settime     251
#define TARGET_NR_timer_gettime     252
#define TARGET_NR_timer_getoverrun  253
#define TARGET_NR_timer_delete      254
#define TARGET_NR_clock_settime     255
#define TARGET_NR_clock_gettime     256
#define TARGET_NR_clock_getres      257
#define TARGET_NR_clock_nanosleep   258
#define TARGET_NR_tgkill            259
#define TARGET_NR_mbind             260
#define TARGET_NR_get_mempolicy     261
#define TARGET_NR_set_mempolicy     262
#define TARGET_NR_vserver           263
#define TARGET_NR_add_key           264
#define TARGET_NR_request_key       265
#define TARGET_NR_keyctl            266
#define TARGET_NR_ioprio_set        267
#define TARGET_NR_ioprio_get        268
#define TARGET_NR_inotify_init      269
#define TARGET_NR_inotify_add_watch 270
#define TARGET_NR_inotify_rm_watch  271
#define TARGET_NR_migrate_pages     272
#define TARGET_NR_pselect6          273
#define TARGET_NR_ppoll             274
#define TARGET_NR_openat            275
#define TARGET_NR_mkdirat           276
#define TARGET_NR_mknotat           277
#define TARGET_NR_fchownat          278
#define TARGET_NR_futimesat         279
#define TARGET_NR_fstatat64         280
#define TARGET_NR_unlinkat          281
#define TARGET_NR_renameat          282
#define TARGET_NR_linkat            283
#define TARGET_NR_symlinkat         284
#define TARGET_NR_readlinkat        285
#define TARGET_NR_fchmodat          286
#define TARGET_NR_faccessat         287
#define TARGET_NR_unshare           288
#define TARGET_NR_set_robust_list   289
#define TARGET_NR_get_robust_list   290
#define TARGET_NR_splice            291
#define TARGET_NR_sync_file_range   292
#define TARGET_NR_tee               293
#define TARGET_NR_vmsplice          294
#define TARGET_NR_move_pages        295
#define TARGET_NR_getcpu            296
#define TARGET_NR_epoll_pwait       297
#define TARGET_NR_statfs64          298
#define TARGET_NR_fstatfs64         299
#define TARGET_NR_kexec_load        300
#define TARGET_NR_utimensat         301
#define TARGET_NR_signalfd          302
#define TARGET_NR_timerfd           303
#define TARGET_NR_eventfd           304
#define TARGET_NR_fallocate         305
#define TARGET_NR_timerfd_create    306
#define TARGET_NR_timerfd_settime   307
#define TARGET_NR_timerfd_gettime   308
#define TARGET_NR_signalfd4         309
#define TARGET_NR_eventfd2          310
#define TARGET_NR_epoll_create1     311
#define TARGET_NR_dup3              312
#define TARGET_NR_pipe2             313
#define TARGET_NR_inotify_init1     314
#define TARGET_NR_preadv            315
#define TARGET_NR_pwritev           316
#define TARGET_NR_rt_tgsigqueueinfo 317
#define TARGET_NR_perf_event_open   318
#define TARGET_NR_recvmmsg          319
#define TARGET_NR_accept4           320
#define TARGET_NR_prlimit64         321
#define TARGET_NR_fanotify_init     322
#define TARGET_NR_fanotify_mark     323
#define TARGET_NR_clock_adjtime     324
#define TARGET_NR_name_to_handle_at 325
#define TARGET_NR_open_by_handle_at 326
#define TARGET_NR_syncfs            327
#define TARGET_NR_setns             328
#define TARGET_NR_sendmmsg          329
#define TARGET_NR_process_vm_readv  330
#define TARGET_NR_process_vm_writev 331
#define TARGET_NR_kcmp              332
#define TARGET_NR_finit_module      333
#define TARGET_NR_sched_setattr     334
#define TARGET_NR_sched_getattr     335
#define TARGET_NR_utimes            336
#define TARGET_NR_renameat2         337
#define TARGET_NR_seccomp           338
#define TARGET_NR_getrandom         339
#define TARGET_NR_memfd_create      340
#define TARGET_NR_bpf               341
#define TARGET_NR_execveat          342
#define TARGET_NR_membarrier        343
#define TARGET_NR_userfaultfd       344
#define TARGET_NR_mlock2            345
#define TARGET_NR_copy_file_range   346
#define TARGET_NR_preadv2           347
#define TARGET_NR_pwritev2          348
