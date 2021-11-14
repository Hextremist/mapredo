mapredo
=======

*Mapreducing at the speed of C*

### Overview

Mapredo is C++11 based software inspired by Google's famous MapReduce paper, with focus on small to medium sized data.  Some of the features are:

- Speedy, does word count of the collected works of Shakespeare in ~200ms on a 2010 i5 gen 1 laptop
- Easy to use, can be pipelined and used with command line tools
- Compression support (snappy)
- Runs on modern Linux distros (gcc) and Windows (Visual Studio)
- Does not require any configuration, just install and run
- Ruby wrapper for more complex analyses 

Some of the current limitations are:

- No scale-out to multiple servers
- Can not be natively embedded into other processing frameworks, but pipes may be used
- Windows (MSVC) port not up-to-date
- Not tested on other UNIX like systems, including OS X

### Requirements

    * CMake
    * GTest
    * Doxygen
    * Snappy
    * Tclap

### Installation (linux)

    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make
    sudo make install
    
### Mini-HOWTO

    wget https://www.gutenberg.org/files/100/100-0.txt
    cat 100-0.txt | mapredo wordcount
    cat 100-0.txt | mapredo wordcount | mapredo --sort numvalue
