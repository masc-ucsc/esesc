## Adding a new counter for statistics 

###Description:
This demo shows how to add a new counter to keep statistics.  Many of the most useful counters are already included in ESESC, but it is likely that you will need to add new counters during your research.  For this demo we will add a counter that counts the number of consecutive stores that are retired in an out-of-order processor.

###Steps:

#. The first thing that needs to be done is to declare the counter in the appropriate class.
    #. Open `~/projs/esesc/simu/libcore/OoOProcessor.h`
    #. Locate the `OoOProcessor` class declaration.  We will add a `GStatsCntr` (see `misc/libsuc/GStats.h` for more information) named `cons_st_ret` to the class.  Since it is only used in `OoOProcessor` we can add it as the last private member.  Add the following line before `protected:` and then save and close the file.

        `GStatsCntr cons_st_ret;`

#. Next we need to add the counter to the constructor for the class.
    #. Open `~/projs/esesc/simu/libcore/OoOProcessor.cpp`
    #. Locate the code for the `OoOProcessor` constructor.  The `cons_st_ret` GStat will need to be initalized by calling its constructor and passing it a string that is the name of the new GStat.  Usually it is a good idea to include the variable name with the GStat name.  Processor stats can be instantiated as many times as there are cores, so they are prefaced with `P(%d)_` where `%d` is the processor number.  Insert the following line before `,cluserManager`:

        `,cons_st_ret("P(%d)_cons_st_ret",i)`

#. Next we need to add logic which determines when to increment the counter. We will do this in the `OoOProcessor::retire()` method. The method includes the statement `nCommitted.inc(dinst->getStatsFlag());` which is used to increment the total number of committed instructions.  We will declare a bool to see if the previous instruction was a store.  Then if it was we will increment our counter.

    #. First define a bool outside of the ROB for loop (existing loop shown below)
  
          `bool prev_st = false;`\
          `for(uint16_t i=0 ; i<RetireWidth && !rROB.empty() ; i++)`

    #. Then inside the loop add the following code after the `nCommitted.inc(dinst->getStatsFlag());` statement.  Then save and exit the file.

~~~~~
          if(dinst->getInst()->isStore()) {                                                                                                                                                
            if(prev_st) {                                                                                                                                                                  
              cons_st_ret.inc(dinst->getStatsFlag());                                                                                                                                      
            }                                                                                                                                                                              
            prev_st = true;                                                                                                                                                                
          } else {                                                                                                                                                                         
            prev_st = false;                                                                                                                                                               
          }  
~~~~~~          

4. Rebuild **debug** build of ESESC by executing `make` in the `~/projs/build/debug` directory.

5. Modify `esesc.conf` to use `crafty.armel` as the benchmark, and get rid of instruction skip at the start of execution.
    
    #. Edit `esesc.conf` and locate `benchName` and change it to:

        `benchName - "crafty.armel"`

    #. Locate `nInstSkip` in `[TBS]` and set it to `1`.

        `nInstSkip = 1`

6. Run ESESC by executing `../main/esesc < crafty.input` in the run directory.

7. Use `report.pl -last` to find the filename of the last report file that was generated.  Then execute `grep "P(.)_cons_st_ret" <filename>` to see the new GStat counter.


