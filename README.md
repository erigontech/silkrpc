# SilkRPC Daemon

C++ implementation of the daemon component exposing the [Ethereum JSON RPC protocol](https://eth.wiki/json-rpc/API) within the [Erigon](https://github.com/ledgerwatch/erigon) architecture.

[![CircleCI](https://circleci.com/gh/torquem-ch/silkrpc.svg?style=shield)](https://circleci.com/gh/torquem-ch/silkrpc)
[![Codecov master](https://img.shields.io/codecov/c/github/torquem-ch/silkrpc/master.svg?style=shield&logo=codecov&logoColor=white)](https://codecov.io/gh/torquem-ch/silkrpc)
![version](https://img.shields.io/github/v/release/torquem-ch/silkrpc?sort=semver&color=normal)

[![Total Alerts](https://img.shields.io/lgtm/alerts/g/torquem-ch/silkrpc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/torquem-ch/silkrpc/alerts)
[![Code Quality: C/C++](https://img.shields.io/lgtm/grade/cpp/g/torquem-ch/silkrpc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/torquem-ch/silkrpc/context:cpp)
[![Code Quality: Python](https://img.shields.io/lgtm/grade/python/g/torquem-ch/silkrpc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/torquem-ch/silkrpc/context:python)

[![GitHub](https://img.shields.io/github/license/torquem-ch/silkrpc.svg)](https://github.com/torquem-ch/silkrpc/blob/master/LICENSE)
![semver](https://img.shields.io/badge/semver-2.0.0-blue)

<br>

# Building the source

## Clone the repository

```
git clone --recurse-submodules https://github.com/torquem-ch/silkrpc.git
```

To update the submodules later on run 
```
git submodule update --init --recursive
```

## Linux & MacOS
Building SilkRPC daemon requires
* C++20 compiler: [GCC](https://www.gnu.org/software/gcc/) >= 10.2.0 or [Clang](https://clang.llvm.org/) >= 10.0.0
* Build system: [CMake](http://cmake.org) >= 3.18.4
* GNU Multiple Precision arithmetic library: [GMP](http://gmplib.org) >= 6.2.0
    * Linux: `sudo apt-get install libgmp3-dev`; `sudo apt-get install -y m4 texinfo bison`  
    * MacOS: `brew install gmp`
* Microsoft mimalloc library: [mimalloc](https://github.com/microsoft/mimalloc) >= 2.0.0
   * Linux:
    ```
    git clone https://github.com/microsoft/mimalloc
    cd mimalloc
    mkdir -p out/release
    cd out/release
    cmake ../..
    make
    sudo make install
    ```
   * MacOS: `brew install mimalloc`
* [Python 3.x](https://www.python.org/downloads/) interpreter >= 3.8.2
    * `sudo apt-get install python3` or `brew install python3`
* some additional Python modules
    * `pip3 install -r requirements.txt` from project folder

Please make your [GCC](https://www.gnu.org/software/gcc/) or [Clang](https://clang.llvm.org/) compiler available as `gcc` and `g++` or `clang` and `clang++` at the command line prompt.

Once the prerequisites are installed, there are convenience [bash](https://www.gnu.org/software/bash/) scripts for complete rebuild and incremental build both in debug and release configurations:

- [GCC](https://www.gnu.org/software/gcc/) compiler
```
# Complete rebuild
./rebuild_gcc_debug.sh
./rebuild_gcc_release.sh
```

```
# Incremental build
./build_gcc_debug.sh
./build_gcc_release.sh
```

- [Clang](https://clang.llvm.org/) compiler
```
# Complete rebuild
./rebuild_clang_debug.sh
./rebuild_clang_release.sh
```

```
# Incremental build
./build_clang_debug.sh
./build_clang_release.sh
```
The resulting build folders are `build_[gcc, clang]_[debug, release]` according to your choice.

To enable parallel compilation, set the `CMAKE_BUILD_PARALLEL_LEVEL` environment variable to the desired value as described [here](https://cmake.org/cmake/help/latest/manual/cmake.1.html#build-a-project). 

For example, in order to have 4 concurrent compile processes, insert in `.bashrc` file the following line
```
export CMAKE_BUILD_PARALLEL_LEVEL=4
```

You can also perform the build step-by-step manually: just bootstrap cmake by running
```
mkdir build_gcc_release
cd build_gcc_release
cmake ..
```
(you have to run `cmake ..` just the first time), then run the build itself
```
cmake --build .
```

Now you can run the unit tests
```
cmd/unit_test
```

and check the code style running
```
./run_linter.sh
```

You can clean the build using
```
cmake --build . --target clean
```

## Windows
* Install [Visual Studio](https://www.visualstudio.com/downloads) 2019. Community edition is fine.
* Make sure your setup includes CMake support and Windows 10 SDK.
* Install [vcpkg](https://github.com/microsoft/vcpkg#quick-start-windows).
* `.\vcpkg\vcpkg install mpir:x64-windows`
* Add <VCPKG_ROOT>\installed\x64-windows\include to your `INCLUDE` environment variable.
* Add <VCPKG_ROOT>\installed\x64-windows\bin to your `PATH` environment variable.
* Open Visual Studio and select File -> CMake...
* Browse the folder where you have cloned this repository and select the file CMakeLists.txt
* Let CMake cache generation complete (it may take several minutes)
* Solution explorer shows the project tree.
* To build simply `CTRL+Shift+B`
* Binaries are written to `%USERPROFILE%\CMakeBuilds\silkrpc\build` If you want to change this path simply edit `CMakeSettings.json` file.

# Code style

We use the standard C++20 programming language. We follow the [Google's C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with the following differences:

* `snake_case` for function names.
* .cpp & .hpp file extensions are used for C++; .cc are used just for [gRPC](https://grpc.io/) generated C++; .c & .h are reserved for C.
* `using namespace foo` is allowed inside .cpp files, but not inside headers.
* Exceptions are allowed.
* User-defined literals are allowed.
* Maximum line length is 190, indentation is 4 spaces – see `.clang-format`.

# Activation

From the build folder (`build_[gcc, clang]_[debug, release]` according to your choice) you typically activate Silkrpc using:

```
$ cmd/silkrpcdaemon --target <core_service_host_address>:9090
```

where `<core_service_host_address>` is the hostname or IP address of the Core services to connect to.

## Command-line parameters

You can check all command-line parameters supported by Silkrpc using:

```
$ cmd/silkrpcdaemon --help
silkrpcdaemon: C++ implementation of ETH JSON Remote Procedure Call (RPC) daemon

  Flags from silkrpc_daemon.cpp:
    --http_port (Ethereum JSON RPC API local binding as string <address>:<port>); default: "localhost:8545";
    --log_verbosity (logging verbosity level); default: c;
    --num_contexts (number of running I/O contexts as integer); default: number of hardware thread contexts / 3;
    --num_workers (number of worker threads as integer); default: 16;
    --target (Core gRPC service location as string <address>:<port>); default: "localhost:9090";
    --wait_mode (I/O scheduler wait mode); default: blocking;
```

You can also check the Silkrpc executable version by:

```
$ cmd/silkrpcdaemon --version
silkrpcdaemon version: 0.0.7-109+commit.bfe634dd
```

## Running Silkrpc with Erigon

Currently Silkrpc is _compatible only with Erigon2 [`2022.09.01-alpha`](https://github.com/ledgerwatch/erigon/releases/tag/v2022.09.01)_ version: last integration and performance test sessions has been performed using Erigon1 at [4067b7c](https://github.com/ledgerwatch/erigon/commit/4067b7c4da6c5d741d3027d95ae2afdf6b7a943a). In order to run Silkrpc with Erigon2, you must install and build Erigon2 following the usage instructions [here](https://github.com/ledgerwatch/erigon/tree/stable#usage).

Please note the following caveats:
- Erigon2 by default starts an internal RPCDaemon so you need to activate Erigon2 using `--http=false` option to avoid conflicts on JSON RPC HTTP port
- if you launch Silkrpc and Erigon2 using the default settings on the same machine, you won't need to customize gRPC client/server addresses/ports; otherwise, you need to make sure that Silkrpc `target` setting matches Erigon2 `private.api.addr` setting
- just the flags described above in the [Command-line parameters](#command-line-parameters) section are actually supported by Silkrpc (other flags may be present in the help description but should be ignored)
