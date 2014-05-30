# Unit Testing for ESESC

This is a unit testing framework for ESESC that uses Google Test. Test binaries are compiled with the rest of the project and run manually afterwords. Currently, the only tests implemented so far are for OoOProccessor and BPred. 

## Usage

After compiling ESESC (see `doc/Usage.md` for details), change into the `t/` directory inside your build folder. Run the test suite with the following command:

    ctest -V

## Notes



## Author

David Goodman <dagoodman@soe.ucsc.edu>
