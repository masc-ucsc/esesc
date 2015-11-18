
## Enabling the power model and viewing the power numbers.

###Assumptions:

Have ESESC built and complete steps from previous demo.

###Description:
For this demo you will run the single-threaded benchmark crafty

###Steps:

1. Modify `esesc.conf` file in the run directory to select the sampler mode.
    1. Open `esesc.conf` in a text editor.  

    2. Switch back to a single core configuration for the demo. \
        Locate the  lines that say:

        `cpuemul[0:3]  = 'QEMUSectionCPU'`\
        `cpusimu[0:3]  = "$(coreType)"` 

        Modify these to:

        `cpuemul[0]  = 'QEMUSectionCPU'`\
        `cpusimu[0]  = "$(coreType)"`

    3. Choose the right benchmark for this demo.

        `benchName = "launcher -- stdin crafty.input -- crafty"`

    4. Locate the line that says:

        `enablePower = false`

        Modify this line to say:

        `enablePower = true`

    5. Save `esesc.conf` and exit

2. Run ESESC
    1. From the run directory execute the command: 

        `../main/esesc`

    2. View results with report.pl

    3. From the run directory execute the command: 

        `~/projs/esesc/conf/scripts/report.pl -last`

3. Look at the power numbers tabulated per sub-block and memory structure and the total chip power.
