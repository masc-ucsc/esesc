    arm-shell# ls MacroLibrary.py MacroParser.py generateRIT.py 
    generateRIT.py  MacroLibrary.py  MacroParser.py
    arm-shell# ./generateRIT.py 

    Usage:   generateRIT.py out_RIT_file

    arm-shell# ./generateRIT.py test_rit.s

Alternatively, here is a Makefile which generates a RIT file 
and compiles it to make a binary (Run this makefile on an 
ARM computer to generate ARM binary).

    arm-shell# more Makefile 
    rit.s: rit
            objdump -d rit > rit.s

    rit: test_rit.s
            gcc -g -static test_rit.s -o rit

    test_rit.s:
            ./generateRIT.py test_rit.s

    clean:
            rm test_rit.s rit rit.s

    arm-shell# make clean
    rm test_rit.s rit rit.s
    arm-shell# make
    ./generateRIT.py test_rit.s
    gcc -g -static test_rit.s -o rit
    objdump -d rit > rit.s
    arm-shell# 

    arm-shell# gdb rit
    (gdb) run
    Starting program: /root/kernels/rit 
    [Inferior 1 (process 28075) exited normally]
    (gdb) q
    [root@alarm kernels]# 
