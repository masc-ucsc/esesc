## Thermal Run Output Files 

###Description:
For this demo you will do a full performance-power-thermal run with Crafty benchmark and check
the generated thermal-related output files. 

###Assumptions:
Complete steps from demo 1 and have the config files and executable in run directory.

###Steps:

#. Modify `esesc.conf` file in the run directory to enable power and thermal. 
    #. Open `esesc.conf`.  

    #. Locate the following line.

        `enablePower = false` 

    #. Modify this line to say.

        `enablePower = true`

    #. Locate the following line.

        `enableTherm = false` 

    #. Enable thermal stage by changing false to true.

        `enableTherm = true`

    #. Select the sampler.
    
         `samplerSel = "TBS"`    

    #. Choose `Crafty` benchmark.

         `benchName = "launcher -- stdin crafty.input -- crafty"`

    #. Save `esesc.conf` and exit.

#. Run ESESC, from the run directory execute:

         ../main/esesc

#. Extract thermal metrics and statistics.    
     #. Execute the following command in run directory. 

         `~/projs/esesc/conf/scripts/report.pl -last` 

     #. Locate `Thermal Metrics` table in the report.pl output. 
        
        * `maxT(K)`:         Maximum temperature
        * `gradT`:           Gradient temperature across the chip
        * `Reliability`:     Measures mean time to failure based on EM, NBTI, Throttle cycle...
        * `ChipLeak (W)`:    Total chip leakage
        * `ChipPower (W)`:   Total chip power
        * `Energy (J)`:      Total chip energy
        * `ThrotCycles`:     Thermal throttling cycles
        * etc. 
 
     #. Check the other thermal related statistics in `esesc_microdemo.?????`

#. Plot time vs. total power vs. maximum temperature from the same thermal run that just finished.   
     
     #. Open thermal trace file.
        * `temp_esesc_microdemo.????? report` 
        * The first column is the time at which temperature is recorded. 
        * The other columns each belong to a particular block as indicated in the top row and contain temperature changes over time. 
        * Notice the highest temperature belongs to column 9, Register File.
     
     #. Exit.
        
     #. Open total power trace.
        * `totalpTh_esesc_microdemo.?????` 
        * First column is the time. 
        * Second column is the total power (dynamic + scaled leakage power based on temperature) consumed by the chip. 
     #. Exit.
     
     #. Temperature and total power traces both record the same timestamp. They can be plotted on the same graph. 

     #. In your `run` directory, launch gnuplot command mode.

          `gnuplot`

     #. In gnuplot environment, enter the following commands. 

         #. Set time on the x axis.    
 
              `set xrange [0: 0.18]`
 
              `set xlabel "Time"`
 
         #. Set temperature on the y1 axis. The temperature belongs to the hottest architectural block, Register File.     
 
              `set yrange [300:380]`
 
              `set ytics nomirror`
 
              `set ylabel "Temperature"`
              
         #. Set power on the y2 axis.       
 
              `set y2range [0:5]`
 
              `set y2tics`
 
              `set y2label "Total Power"`
              
         #. Plot data. 
 
              `plot "temp_esesc_microdemo.?????" u 2:9 w lines t "Temperature" axis x1y1, "totalpTh_esesc_microdemo.?????" u 1:2 w lines t "Total Power" axis x1y2`
              
         #. Observe the changes in total power versus temperature.         

         #. Return to run directory.
                  
              `exit`

You have now finished a complete thermal run and checked the relevant report files.    
