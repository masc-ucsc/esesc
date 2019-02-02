# Environment 
ESESC runs on Linux.  It has been tested on x86-64 and ARM platforms. The x86-64 system is the default, but ESESC also compiles and runs on ARM.

Previous version of ESESC had ARMv7, currently it only has MIPSR6 and RISCV. The default flow is RISCV.


# Requirements 
ESESC is currently tested with Arch Linux and with Ubunut with CI. We use this docker images for testing: https://github.com/masc-ucsc/docker-images


Check the dockerfiles to see the required packages.


# Steps to compile and build eSESC

# Compile ESESC to run on an x86_64 system:

These steps assume that the ESESC repo is checked out in the `~/projs/esesc` directory. And also that you are building in the `~/build/debug` or `~/build/release` directory. Change them for your own configuration as needed.

## Debug
    mkdir -p ~/build/debug
    cd ~/build/debug
    cmake -DCMAKE_BUILD_TYPE=Debug ~/projs/esesc
    make VERBOSE=1

# Release
    mkdir ~/build_release
    cd ~/build_release
    cmake ~/projs/esesc
    make

# Track Load data
    cmake -DESESC_TRACE_DATA=1 ~/projs/esesc

# MIPSR6
    cmake -DESESC_MIPSR6=1 ~/projs/esesc

# Release and System
    mkdir ~/build_system
    cd ~/build_release
    cmake -DESESC_SYSTEM=1 ~/projs/esesc
    make

# Compile with QEMU in a arm machine (arch linux chromebook)

cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_HOST_ARCH=armel ~/projs/esesc

# Compile ESESC with DEBUG and clang compiler

CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug ~/projs/esesc

# Compile ESESC with DEBUG and emscripten compiler

CC=emcc CXX=em++ cmake -D_CMAKE_TOOLCHAIN_PREFIX=em -DCMAKE_BUILD_TYPE=Debug ~/projs/esesc

ESESC generates a dot file named memory-arch.dot, which is a description of the memory hierarchy as eSesc sees, after parsing shared.conf.
To view this in as a png, run the following command. 

    dot memory-arch.dot -Tpng -o memory-arch.png

--------------------------------------------------------
#Power

To enable power, set `enablePower = true` in `esesc.conf`

#Temperature

To enable temperature modeling, both the following should be set: 
    enablePower = true 
    doTherm = true  # in therm.conf

Temperature modeling needs a floorplan as well. There are several floorplan for
different configurations already in flp.conf. To use a floorplan, update
the corresponding parameters in therm.conf. For example:

    floorplan[0] = 'floorplan_4c'
    layoutDescr[0] = 'layoutDescr_4c'

both floorplan_4c and layoutDescr_4c are sections defined in flp.conf.
Each floorplan is usually specified with a name mangle, _4c in this example.


To generate a new floorplan, use floorplan.rb script:

    esesc/conf/floorplan.rb BuildDirPath SrcDirPath RunDirPath nameMingle

BuildDirPath: Path to the esesc build directory
SrcDirPath: Path to the esesc source directory
RunPath: Path to the directory that configuration files reside for run. This is the same as build directory if you are running esesc from build directory.
nameMangle: A string to attach to `floorplan` and `layoutDescr` sections when saved in `flp.conf`.

This will generate the floorplan information and power breakdown of blocks,
and run the floorplanning tool to generate a new floorplan, save in in `flp.conf`, and 
update `therm.conf` to point to it.

