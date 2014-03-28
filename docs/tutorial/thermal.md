## Thermal Run Output Files 

###Description:
For this demo you will do a full performance-power-thermal run with Crafty benchmark and check
the generated thermal-related output files. 

###Assumptions:
Complete steps from demo 1 and have the config files and executable in run directory.

###Steps:

1. Modify `esesc.conf` file in the run directory to enable power and thermal. 
    1. Open `esesc.conf`.  

    2. Locate the following line.

        `enablePower = false` 

    3. Modify this line to say.

        `enablePower = true`

    4. Locate the following line.

        `enableTherm = false` 

    5. Enable thermal stage by changing false to true.

        `enableTherm = true`

    6. Select the sampler.
    
         `samplerSel = "TBS"`    

    7. Choose `Crafty` benchmark.

         `benchName = "launcher -- stdin crafty.input -- crafty"`

    8. Save `esesc.conf` and exit.

2. Run ESESC, from the run directory execute:

         ../main/esesc

3. Extract thermal metrics and statistics.    
     1. Execute the following command in run directory. 

         `~/projs/esesc/conf/scripts/report.pl -last` 

     2. Locate `Thermal Metrics` table in the report.pl output. 
        
        * `maxT(K)`:         Maximum temperature
        * `gradT`:           Gradient temperature across the chip
        * `Reliability`:     Measures mean time to failure based on EM, NBTI, Throttle cycle...
        * `ChipLeak (W)`:    Total chip leakage
        * `ChipPower (W)`:   Total chip power
        * `Energy (J)`:      Total chip energy
        * `ThrotCycles`:     Thermal throttling cycles
        * etc. 
 
     3. Check the other thermal related statistics in `esesc_microdemo.?????`

4. Plot time vs. total power vs. maximum temperature from the same thermal run that just finished.   
     
     1. Open thermal trace file.
        * `temp_esesc_microdemo.????? report` 
        * The first column is the time at which temperature is recorded. 
        * The other columns each belong to a particular block as indicated in the top row and contain temperature changes over time. 
        * Notice the highest temperature belongs to column 9, Register File.
     
     2. Exit.
        
     3. Open total power trace.
        * `totalpTh_esesc_microdemo.?????` 
        * First column is the time. 
        * Second column is the total power (dynamic + scaled leakage power based on temperature) consumed by the chip. 
     4. Exit.
     
     5. Temperature and total power traces both record the same timestamp. They can be plotted on the same graph. 

     6. In your `run` directory, launch gnuplot command mode.

          `gnuplot`

     7. In gnuplot environment, enter the following commands. 

         1. Set time on the x axis.    
 
              `set xrange [0: 0.18]`
              `set xlabel "Time"`
 
         2. Set temperature on the y1 axis. The temperature belongs to the hottest architectural block, Register File.     
 
              `set yrange [300:380]`
              `set ytics nomirror`
              `set ylabel "Temperature"`
              
         3. Set power on the y2 axis.       
 
              `set y2range [0:5]`
              `set y2tics`
              `set y2label "Total Power"`
              
         4. Plot data. 
 
              `plot "temp_esesc_microdemo.?????" u 2:9 w lines t "Temperature" axis x1y1, "totalpTh_esesc_microdemo.?????" u 1:2 w lines t "Total Power" axis x1y2`
              
         5. Observe the changes in total power versus temperature.         

         6. Return to run directory.
                  
              `exit`

You have now finished a complete thermal run and checked the relevant report files.    
## Generating a New Floorplan 

###Description:
In this demo, you will generate a floorplan for a dual core chip. 

###Assumptions:
Complete steps from demo 1 and have the config files and executable in run directory.

###Steps:

1. Modify `esesc.conf` file in the run directory to configure multiple CPU cores. 
    1. Open `esesc.conf`.  

    2. Locate the lines that say:

        `cpuemul[0]  = 'QEMUSectionCPU'` 
        `cpusimu[0]  = "$(coreType)"`

    3. Modify these lines to specify additional CPU cores by changing the [0] to [0:1].  This will specify 2 homogeneous cores.  After the changes the file should contain the lines:

        `cpuemul[0:1]  = 'QEMUSectionCPU'` 
        `cpusimu[0:1]  = "$(coreType)"`

    4. Change the benchmark for multicore to multithreaded. 

         `benchName = "launcher -- fft -p2 -m20"`

    5. Save and exit.     

2. Run the following commands in build directory to generate the floorplanning tool executable.

     `make floorplan`

3. In run directory, run the floorplan script to generate a new floorplan for the dual core config.

    `~/projs/esesc/conf/scripts/floorplan.rb BuildDir_Path SrcDir_Path RunDir_Path NameMangle`

    * `BuildDir_Path`:   Path to the ESESC build directory
    * `SrcDir_Path`:     Path to the ESESC source directory
    * `RunDir_Path`:     Path to the configuration files in run directory
    * `NameMangle`:      A string to attach to 'floorplan' and 'layoutDescr' sections
              
4. Once floorplan script finishes running, the links to the floorplan in `pwth.conf` are updated.
    
     `floorplan[0]   = ‘floorplan_NameMangle’`
     `layoutDescr[0] = ‘layoutDescr_NameMangle’`

5. New layout and floorplan definitions in `flp.conf` have been created under the following sections.

     `[layoutDescr_NameMangle]`
     `[floorplan_NameMangle]`

6. A copy of layout and floorplan description is also saved in `new.flp` in run directory.
   
You have now finished generating the new floorplan and can continue running ESESC in thermal mode for a suitable benchmark.  

## Full Thermal Run with Multicore Floorplan and Thermal Management Policy 

###Description:
For this demo you will run the multi-threaded benchmark FFT. You will enable and use two different thermal management policies: DVFS and thermal throttling and compare the results. You will also learn to enable and use
graphics feature for floorplan thermal map.  

###Assumptions:
Complete steps from demo 1 and have the config files and executable in run directory. Complete demo 6 to setup dual core and generate the corresponding floorplan.  

###Steps:

1. Complete steps from demo 6.

2. Set the thermal management policy to thermal throttling. 
    1. Open `esesc.conf`.  

    2. Set thermal throttling temperature.

         `thermTT = 373.15`

    3. Save `esesc.conf` and exit.  

    4. Open `pwth.conf` 

    5. Disable turbo mode.

         `enableTurbo = false`

    6. Locate the following line.
     
         `enableGraphics = false` 
              
    7. To dump temperature map related files, modify it to say.

         `enableGraphics = true`      
              
    8. Save and exit.      

3. Run ESESC

     `../main/esesc`

4. Run report.pl to extract results from the last ESESC report generated. 

     `~/projs/esesc/conf/report.pl –last`

5. Look at the `Thermal Metrics` section of the `report.pl` output. 

6. Create a short video from the thermal map `svg` snapshots in your run directory. Use the following command as an example. 

    `convert -delay 0.3 lcomp-NORM_layer-2_smpltype-CUR_0.*` fft_tt.gif

7. View `fft_tt.gif` using a browser and observe the changes in temperature in various architectural blocks of each core. 

8. Now change the thermal management policy to `dvfs_t` and re-run `esesc`. 

     1. Open `pwth.conf` 

     2. Enable turbo mode.

          `enableTurbo = true`

     3. Enable DVFS based on temperature.     

          `turboMode = dvfs_t`

     4. Save and exit.      
    
     5. Open `esesc.conf`     

     6. Set thermal throttling temperature.

          `thermTT = 373.15`
          
     7. Save and exit.      

9. Run ESESC

     `../main/esesc`

10. Run report.pl to extract results. 

     `~/projs/esesc/conf/report.pl –last`

11. Look at the `Thermal Metrics` section of the `report.pl` output. 

12. Create a short video from the thermal map `svg` snapshots in your run directory. Use the following command as an example. 

    `convert -delay 0.3 lcomp-NORM_layer-2_smpltype-CUR_0.*` fft_dvfs.gif

13. View `fft_dvfs.gif` using a browser and observe the changes in temperature in various architectural blocks of each core. 
