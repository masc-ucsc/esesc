

## Multi-threaded with Time Based Sampling (TBS):

###Description:
For this demo you will run the multi-threaded benchmark `blackscholes`

###Assumptions:
Complete steps from previous demo and have config files and executable in run directory.

###Steps:

#. Modify `esesc.conf` file in the run directory to configure multiple CPU cores
    #. Open `esesc.conf` in a text editor.  
    #. Locate the lines that say:
          
          `cpuemul[0]  = 'QEMUSectionCPU'`\
          `cpusimu[0]  = "$(coreType)"`\

    #. Modify these lines to specify additional CPU cores by changing the `[0]` to `[0:3]`.  This will specify 4 homogeneous cores.  After the changes the file should contain the lines:

        `cpuemul[0:3]  = 'QEMUSectionCPU'`\
        `cpusimu[0:3]  = "$(coreType)"`\

#. Change sampling mode to Time Based Sampling (TBS).
    #. In `esesc.conf` locate the line that says:
        `samplerSel = "TASS"`

    #. Modify it to say
        `samplerSel = "TBS"`

#. Change the TBS parameters to skip instructions at the start of execution:
    #. In `esesc.conf` locate the section `[TBS]`
    #. Modify the `nInstSkip` and to skip 1 billion instructions so that the lines read as follows:

         `nInstSkip = 1e9`

#. Change the benchmark to blackscholes:
    #. In `esesc.conf` locate the line that says:

        `benchName = . . . `

    #. Modify it to say:

        `benchName = "launcher -- blackscholes 2 blackscholes.input blackscholes.output"` 

    #. Save `esesc.conf` and exit
 
#. Run ESESC
    #. From the run directory execute the command: 

        `../main/esesc`

#. View results with report.pl
    #. From the run directory execute the command: 

        `~/projs/esesc/conf/scripts/report.pl –last`

#. Look at the execution time and instruction breakdown in the report output.

