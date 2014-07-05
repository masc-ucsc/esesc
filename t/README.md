# Unit Testing for ESESC

This is a unit testing framework for ESESC that uses Google Test. Test binaries are compiled with the rest of the project and run manually afterwords. Currently, the only tests implemented so far are for OoOProccessor and BPred. 

## Usage

After compiling ESESC (see `doc/Usage.md` for details), run the follow command:

    sudo make test

It is recommended to use `sudo` when running the test suite, because otherwise you may see *Operation not permitted* errors.

You can also run tests with more verbose output by changing into the `t/` directory inside your build folder, and using the following command:

    ctest -V

## Notes

* All `bins/` and `conf/` files are copied into `t/` in the build directory for use in the simulation.

* OoOProcessor unit test
    This test harness uses the BootLoader to start the system and run the default simulation. GStats nCommitted and clockTicks are asserted to be greater than zero after the simulation is run.

* BPred unit test
    This test harness uses the BootLoader to start the system and run the default simulation. GStats nBranches, nTaken, and nMiss are checked. They should all be greater than zero, and nMiss should not be equal to nBranches.

## Author

David Goodman <dagoodman@soe.ucsc.edu>
