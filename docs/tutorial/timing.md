
## Debugging ESESC with GDB:

### Description:
For this demo you will learn basic debugging techniques for ESESC.

### Steps:

1. Run ESESC on GDB, and add a breakpoint:
    
  * From the run directory execute the command:

    `gdb ../main/esesc`

  * From GDB, type:

    `b OoOProcessor::execute()`

2. Start execution in GDB

  * In GDB, type:

    `r -c esesc.conf`

3. Continue the execution and check status

  * In GDB type:

```
      p this->dumpROB()
      c 1000
      p this->dumpROB()
```

4. Play around with available information 

  * At the breakpoint, type

    `p globalClock`

  * Use the auto-completion to check other possibilities

5. Exit GDB:
    
  * Type:

    `quit`

