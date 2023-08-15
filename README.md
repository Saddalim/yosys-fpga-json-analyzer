A cross-platform command line tool to analyze the networks by Yosys syntheser. Work in progress. Aims to identify frequency bottlenecks in synthesed FPGA combinational networks.

Links for further reading about Yosys
- https://github.com/YosysHQ/yosys
- https://yosyshq.net/yosys/
- https://www.yosyshq.com/

## Usage

To run, specify the JSON to be parsed via the first argument

    $ fpga-json-analyzer ~/top.json

## Build

Visual Studio Code with C++ and CMake extensions installed will do the rest of the work for you. On Windows, you'll need to set up a C++ builder toolchain - see below. For console monkeys, steps are as follows:

### Linux

Install toolchain

    $ sudo apt update
    $ sudo apt install gcc g++ cmake git

Make sure you got at least g++11. Version 13 is recommended, if available on your distro.

    $ g++ --version

Clone/pull the repository and its submodules as well  

    $ git pull
    $ cd yosys-fpga-json-analyzer
    $ git submodule update --init --recursive

Configure project

    $ cmake .

Build project

    $ make

### Windows

Install CMake and a C++ compiler of your choice. This will typically be a full Visual Studio with MSVC compiler included, or the free Visual Studio Code with a g++ environment. I recommend MSYS2 (https://www.msys2.org/) for the ease of use. For detailed steps on how to set up MSYS2/MinGW for Visual Studio Code, see the description in the docs (https://code.visualstudio.com/docs/cpp/config-mingw).

Once you installed these, use VSCode to build, run and debug (<kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>P</kbd>, type cmake, and run Configure, Build, Run in this order), as CLI debugging on Windows / MinGW is not for the faint hearted.