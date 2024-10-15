# Building the project
* Run `make` from the src file to build all of the object files and snappy-c library. 
* Run `make clean` to clean all of the object files and .so files.

# Running the project
### Running the tinyfile client
Tinyfile client can pass messages to the `tinyfile_service` :D
### Snappyc compression tests
`snappyc_test.c` runs basic snappy-c compression and uncompression on a given input file. After building the project using `make`, you can run the project:
* Single input file
    * `./snappyc_test.o <input_file> <compress_file_path> <uncompress_file_path>`
* All input files in `/bin/input`. There are two commands that accomplish this
    * `./run.sh`
    * `make run`