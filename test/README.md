Bifrost USD Unit Tests
======================

Unit tests for Bifrost USD.


## Guidelines

* Unit test cases are grouped together in independent test files. ctest will execute sequentially the test cases in a given test file, but all tests in a given file can be executed in parallel with test cases from another test file.
* If a test file needs to create output files, it must create them into its own unique output folder, avoiding completely the possibility that two concurrent tests would attempt to write to the same file. These unique output folders should be deleted as a first phase in each test file, allowing the test cases that follow to assume and/or check that an output file is not already on disk before it is written to.
* When testing Bifrost USD export features (e.g. export_layer_to_file), a test must not export an initial file first, then replace its content and re-export the file almost immediately. Doing so, it has been observed on the Windows platform that Pixar USD can occasionally report an "access denied" error when it attempts to replace the initial file on disk. To avoid this potential issue, it is better to build the stage or layer content in memory, then export it to the disk only once.
