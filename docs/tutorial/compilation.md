
## Compile and run a default benchmark

###Assumptions:

 * Running ESESC on an x86-64 system that has either Ubuntu 12.04 or Arch Linux
installed along with all necessary packages for the appropriate OS.

 * The ESESC repository has been cloned from GitHub and is checked out
in `~/projs/esesc`

### Description:
For this demo you will compile ESESC in Release and Debug mode and then 
run a short benchmark and view the statistics that are generated.

### Steps:

1. Make sure you have the latest version of the ESESC code:

    ```
    cd ~/projs/esesc
    git pull
    ```

2. Create the build directory:
  
    ```
    mkdir -p ~/build/debug
    mkdir -p ~/build/release
    ```

3. Compile ESESC:

    ```
    cd ~/build/release
    cmake ~/projs/esesc
    make
    ```

4. For this demo we will use the release build, so create a run directory in the release sub-directory:

    ```
    mkdir -p ~/build/release/run
    ```

5. Create local configuration files and binaries in the run directory:
  
    ```
    cp ~/projs/esesc/conf/*.conf ~/build/release/run
    cp ~/projs/esesc/bins/* ~/build/release/run
    ```

6. Run ESESC and view results:

    ```
    cd ~/build/release/run
    ~/build/release/main/esesc
    ~/projs/esesc/conf/scripts/report.pl -last
    ```

7. Build ESESC in Debug mode before the break (needed for the next demo)

    ```
    cd ~/build/debug
    cmake -DCMAKE_BUILD_TYPE=Debug ~/projs/esesc
    make
    mkdir run
    cd run
    cp ~/projs/esesc/conf/* .
    cp ~/projs/esesc/bins/* .
    ```

### Note:
  View file `~/build/release/run/esesc.conf`, the parameter
  benchname is the benchmark which will be run. By default
  the launcher executable is being used to load the Crafty
  benchmark.

