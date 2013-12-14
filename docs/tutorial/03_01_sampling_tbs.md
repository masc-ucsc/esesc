

## Single-threaded with Time Based Sampling (TBS):

###Assumptions:

Have ESESC built and complete steps from previous demo.

###Description:
For this demo you will run the single-threaded benchmark crafty.

###Steps:

#. Modify `esesc.conf` file in the run directory to select the sampler mode.
    #. Open `esesc.conf` in a text editor.  
    #. Locate the line that says:

        `samplerSel = . . .`

    #. If necessary modify this line to say

        `samplerSel = "TASS"`

    #. Save `esesc.conf` and exit

#. Run ESESC
    #. From the run directory execute the command: 

        `../main/esesc`

    #. View results with report.pl
    #. From the run directory execute the command: 

        `~/projs/esesc/conf/scripts/report.pl –last`

#. Look at the execution time and instruction breakdown in the report output.

