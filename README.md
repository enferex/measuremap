measuremap: Measure time and space performance of c++ map types.
================================================================
measuremap is a tool to measure the time and space performance of c++ maps. The
initial configuration is to measure the performance of std::map and
llvm::DenseMap.

Usage
-----
./measuremap <number of trials> <number of keys to add to the maps>

The keys are randomly generated and the values are dummy values.

Building
--------
Run `make`

Contact
-------
Matt Davis: https://github.com/enferex
