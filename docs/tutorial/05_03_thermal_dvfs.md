## Full Thermal Run with Multicore Floorplan and Thermal Management Policy 

###Description:
For this demo you will run the multi-threaded benchmark FFT. You will enable and use two different thermal management policies: DVFS and thermal throttling and compare the results. You will also learn to enable and use
graphics feature for floorplan thermal map.  

###Assumptions:
Complete steps from demo 1 and have the config files and executable in run directory. Complete demo 6 to setup dual core and generate the corresponding floorplan.  

###Steps:

#. Complete steps from demo 6.

#. Set the thermal management policy to thermal throttling. 
    #. Open `esesc.conf`.  

    #. Set thermal throttling temperature.

         `thermTT = 373.15`

    #. Save `esesc.conf` and exit.  

    #. Open `pwth.conf` 

    #. Disable turbo mode.

         `enableTurbo = false`

    #. Locate the following line.
     
         `enableGraphics = false` 
              
    #. To dump temperature map related files, modify it to say.

         `enableGraphics = true`      
              
    #. Save and exit.      

#. Run ESESC

     `../main/esesc`

#. Run report.pl to extract results from the last ESESC report generated. 

     `~/projs/esesc/conf/report.pl –last`

#. Look at the `Thermal Metrics` section of the `report.pl` output. 

#. Create a short video from the thermal map `svg` snapshots in your run directory. Use the following command as an example. 

    `convert -delay 0.3 lcomp-NORM_layer-2_smpltype-CUR_0.*` fft_tt.gif

#. View `fft_tt.gif` using a browser and observe the changes in temperature in various architectural blocks of each core. 

#. Now change the thermal management policy to `dvfs_t` and re-run `esesc`. 

     #. Open `pwth.conf` 

     #. Enable turbo mode.

          `enableTurbo = true`

     #. Enable DVFS based on temperature.     

          `turboMode = dvfs_t`

     #. Save and exit.      
     
     #. Open `esesc.conf`     

     #. Set thermal throttling temperature.

          `thermTT = 373.15`
          
    #. Save and exit.      

#. Run ESESC

     `../main/esesc`

#. Run report.pl to extract results. 

     `~/projs/esesc/conf/report.pl –last`

#. Look at the `Thermal Metrics` section of the `report.pl` output. 

#. Create a short video from the thermal map `svg` snapshots in your run directory. Use the following command as an example. 

    `convert -delay 0.3 lcomp-NORM_layer-2_smpltype-CUR_0.*` fft_dvfs.gif

#. View `fft_dvfs.gif` using a browser and observe the changes in temperature in various architectural blocks of each core. 
