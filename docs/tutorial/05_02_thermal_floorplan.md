## Generating a New Floorplan 

###Description:
In this demo, you will generate a floorplan for a dual core chip. 

###Assumptions:
Complete steps from demo 1 and have the config files and executable in run directory.

###Steps:

#. Modify `esesc.conf` file in the run directory to configure multiple CPU cores. 
    #. Open `esesc.conf`.  

    #. Locate the lines that say:

        `cpuemul[0]  = 'QEMUSectionCPU'` 
        
        `cpusimu[0]  = "$(coreType)"`

    #. Modify these lines to specify additional CPU cores by changing the [0] to [0:1].  This will specify 2 homogeneous cores.  After the changes the file should contain the lines:

        `cpuemul[0:1]  = 'QEMUSectionCPU'` 

        `cpusimu[0:1]  = "$(coreType)"`

    #. Change the benchmark for multicore to multithreaded. 

         `benchName = "launcher -- fft -p2 -m20"`

    #. Save and exit.     

#. Run the following commands in build directory to generate the floorplanning tool executable.

     `make floorplan`

#. In run directory, run the floorplan script to generate a new floorplan for the dual core config.

    `~/projs/esesc/conf/scripts/floorplan.rb BuildDir_Path SrcDir_Path RunDir_Path NameMangle`

    * `BuildDir_Path`:   Path to the ESESC build directory

    * `SrcDir_Path`:     Path to the ESESC source directory

    * `RunDir_Path`:     Path to the configuration files in run directory

    * `NameMangle`:      A string to attach to 'floorplan' and 'layoutDescr' sections
              
#. Once floorplan script finishes running, the links to the floorplan in `pwth.conf` are updated.
    
     `floorplan[0]   = ‘floorplan_NameMangle’`

     `layoutDescr[0] = ‘layoutDescr_NameMangle’`

#. New layout and floorplan definitions in `flp.conf` have been created under the following sections.

     `[layoutDescr_NameMangle]`

     `[floorplan_NameMangle]`

#. A copy of layout and floorplan description is also saved in `new.flp` in run directory.
   
You have now finished generating the new floorplan and can continue running ESESC in thermal mode for a suitable benchmark.  
