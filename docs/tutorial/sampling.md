## Single-threaded with Time Based Sampling (TBS):

###Assumptions:

Have ESESC built and complete steps from previous demo.

###Description:
For this demo you will run the single-threaded benchmark crafty.

###Steps:

1. Modify `esesc.conf` file in the run directory to select the sampler mode.
    1. Open `esesc.conf` in a text editor.  
    2. Locate the line that says:

        `samplerSel = . . .`

    3. If necessary modify this line to say

        `samplerSel = "TASS"`

    4. Save `esesc.conf` and exit

2. Run ESESC
    1. From the run directory execute the command: 

        `../main/esesc`

    2. View results with report.pl
    3. From the run directory execute the command: 

        `~/projs/esesc/conf/scripts/report.pl –last`

4. Look at the execution time and instruction breakdown in the report output.

## Multi-threaded with Time Based Sampling (TBS):

###Description:
For this demo you will run the multi-threaded benchmark `blackscholes`

###Assumptions:
Complete steps from previous demo and have config files and executable in run directory.

###Steps:

1. Modify `esesc.conf` file in the run directory to configure multiple CPU cores
    1. Open `esesc.conf` in a text editor.  
    2. Locate the lines that say:
          
          `cpuemul[0]  = 'QEMUSectionCPU'`\
          `cpusimu[0]  = "$(coreType)"`\

    3. Modify these lines to specify additional CPU cores by changing the `[0]` to `[0:3]`.  This will specify 4 homogeneous cores.  After the changes the file should contain the lines:

        `cpuemul[0:3]  = 'QEMUSectionCPU'`\
        `cpusimu[0:3]  = "$(coreType)"`\

2. Change sampling mode to Time Based Sampling (TBS).
    1. In `esesc.conf` locate the line that says:
        `samplerSel = "TASS"`

    2. Modify it to say
        `samplerSel = "TBS"`

3. Change the TBS parameters to skip instructions at the start of execution:
    1. In `esesc.conf` locate the section `[TBS]`
    2. Modify the `nInstSkip` and to skip 1 billion instructions so that the lines read as follows:

         `nInstSkip = 1e9`

4. Change the benchmark to blackscholes:
    1. In `esesc.conf` locate the line that says:

        `benchName = . . . `

    2. Modify it to say:

        `benchName = "launcher -- blackscholes 2 blackscholes.input blackscholes.output"` 

    3. Save `esesc.conf` and exit
 
5. Run ESESC
    1. From the run directory execute the command: 

        `../main/esesc`

6. View results with report.pl
    1. From the run directory execute the command: 

        `~/projs/esesc/conf/scripts/report.pl –last`

7. Look at the execution time and instruction breakdown in the report output.

## Adding a new counter for statistics 

###Description:
This demo shows how to add a new counter to keep statistics.  Many of the most useful counters are already included in ESESC, but it is likely that you will need to add new counters during your research.  For this demo we will add a counter that counts the number of consecutive stores that are retired in an out-of-order processor.

###Steps:

1. The first thing that needs to be done is to declare the counter in the appropriate class.
    1. Open `~/projs/esesc/simu/libcore/OoOProcessor.h`
    2. Locate the `OoOProcessor` class declaration.  We will add a `GStatsCntr` (see `misc/libsuc/GStats.h` for more information) named `cons_st_ret` to the class.  Since it is only used in `OoOProcessor` we can add it as the last private member.  Add the following line before `protected:` and then save and close the file.

        `GStatsCntr cons_st_ret;`

2. Next we need to add the counter to the constructor for the class.
    1. Open `~/projs/esesc/simu/libcore/OoOProcessor.cpp`
    2. Locate the code for the `OoOProcessor` constructor.  The `cons_st_ret` GStat will need to be initalized by calling its constructor and passing it a string that is the name of the new GStat.  Usually it is a good idea to include the variable name with the GStat name.  Processor stats can be instantiated as many times as there are cores, so they are prefaced with `P(%d)_` where `%d` is the processor number.  Insert the following line before `,cluserManager`:

        `,cons_st_ret("P(%d)_cons_st_ret",i)`

3. Next we need to add logic which determines when to increment the counter. We will do this in the `OoOProcessor::retire()` method. The method includes the statement `nCommitted.inc(dinst->getStatsFlag());` which is used to increment the total number of committed instructions.  We will declare a bool to see if the previous instruction was a store.  Then if it was we will increment our counter.

    1. First define a bool outside of the ROB for loop (existing loop shown below)
  
          `bool prev_st = false;`\
          `for(uint16_t i=0 ; i<RetireWidth && !rROB.empty() ; i++)`

    2. Then inside the loop add the following code after the `nCommitted.inc(dinst->getStatsFlag());` statement.  Then save and exit the file.

    ```
          if(dinst->getInst()->isStore()) {                                                                                                                                                
            if(prev_st) {                                                                                                                                                                  
              cons_st_ret.inc(dinst->getStatsFlag());                                                                                                                                      
            }                                                                                                                                                                              
            prev_st = true;                                                                                                                                                                
          } else {                                                                                                                                                                         
            prev_st = false;                                                                                                                                                               
          }  
    ```

4. Rebuild **debug** build of ESESC by executing `make` in the `~/projs/build/debug` directory.

5. Modify `esesc.conf` to use `crafty.armel` as the benchmark, and get rid of instruction skip at the start of execution.
    
    1. Edit `esesc.conf` and locate `benchName` and change it to:

        `benchName - "crafty.armel"`

    2. Locate `nInstSkip` in `[TBS]` and set it to `1`.

        `nInstSkip = 1`

6. Run ESESC by executing `../main/esesc < crafty.input` in the run directory.

7. Use `report.pl -last` to find the filename of the last report file that was generated.  Then execute `grep "P(.)_cons_st_ret" <filename>` to see the new GStat counter.


