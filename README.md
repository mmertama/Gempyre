![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Gempyre
=====
UI Framework
-------------

Gempyre is a UI multiplatform framework. Supporting Windows, Mac OSX, Linux, Raspberry OS and Android. Gempyre is minimalistic and simple; It is a UI framework without widgets - instead, the UI is composed using common web tools and frameworks. Therefore Gempyre is small, easy to learn and quick to take in use.
 
For the application, its engine is implemented using C++ (or Python), the UI is constructed using Javascript, CSS and HTML like any front end. All common tools from the vast pool web technologies shall be available. Gempyre library provides a C++ interface to interact with the UI - the whole API is only a few dozen calls.

Gempyre is intended for applications that has a solid C++ core (or C), and takes benefit of rapid UI development without extra hassle with complex/expensive/platform specific/quirky UI development. Gempyre combines power of C++ with vast options of front end development tools, sources, documents frameworks that are only available on for Web Developers.

Gempyre is multiplatform, its core is written using C++17  (tested OSX (CLang), Ubuntu (gcc), Raspberry OS (gcc) and Windows 10 (MSVC and MinGW) ). The Gempyre framework can be downloaded at Github under MIT license.

Gempyre itself does not contain an application window. The UI is drawn using external application. So OSes uses native browser help, some Python webview. However that is fully configurable per application. 

Gempyre is a library that is linked with the application, except for Android, see [Gempyre-Android](https://github.com/mmertama/Gempyre-Android). For Python, install [Gempyre-Python](https://github.com/mmertama/Gempyre-Python) on top of Gempyre library.


### Gempyre API has few headers

* [Gempyre core in _gempyre.h_](gempyre.md), everything that basic application needs.
* [Gempyre Utils in _gempyre_utils.h_](gempyre_utils.md), miscellaneous collection or function that helps writing applications. 
* [Gempyre Graphics in _gempyre_graphics.h_](gempyre_graphics.md),  helps developing graphics intensive applications and games.
* gempyre_client.h, provides file dialogs for Gempyre application. 


### Linux

* Run
  ```bash
  linux_install.sh
  ```
  
### Mac OSX

* Run
  ```bash
   osx_install.sh
  ```
 
### Windows

* Install git bash from https://gitforwindows.org/
* Run git clone https://github.com/mmertama/Gempyre.git on git bash console.

#### MSVC

* Install cmake https://cmake.org/download/ (let it to be added in path)
* Install Visual Studio https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2019, and pick Desktop development with C++
* Install Python (maybe 3.9, yet tested mostly with 3.8, but 3.6 >= shall be ok) https://www.python.org/downloads/windows/
* From Windows menu, Visual Stuudio: Open "x64 Native Tools Command Prompt for VS 2019"
* Run 
   ```bat
    msvc_install.bat
   ```

#### MinGW

* Make sure you are using the right MinGW shell (Msys minGW 64-bit - one with blue icon (Not brown or cyan :-))
* See Instructions https://www.devdungeon.com/content/install-gcc-compiler-windows-msys2-cc
* Ensure Ninja is installed "packman -s base-devel gcc vim cmake ninja"
* Run
  ```bash
  mingw_install.sh
  ```
 
### Raspberry OS

 * Requires Rasberry OS Bullseye (older is ok, but you need more recent gcc in order to build C++17). Tested Raspberry Pi 3 and Raspberry Pi 4.  
 * Quite late Cmake is required, here are [snap instructions](https://snapcraft.io/install/cmake/raspbian#install).
 * Run
  ```bash
        pi@raspberrypi:~/Development/Gempyre ./raspberry_install.sh
   ```
 * When building on Raspberry, please pass -DRASPBERRY=1 for your cmake call. E.g. 
  ```bash
         pi@raspberrypi:~/Development/Tilze/build $ cmake .. -DRASPBERRY=1
   ```     

 ### Some example projects using Gempyre
 
 * [Examples](https://github.com/mmertama/Gempyre/tree/raspberry/examples)
 * [mandelbrot-Gempyre](https://github.com/mmertama/mandelbrot-Gempyre)
 * [treeview-Gempyre](https://github.com/mmertama/treeview-Gempyre)
 * [hexview-Gempyre](https://github.com/mmertama/hexview-Gempyre)
 * [calc-Gempyre](https://github.com/mmertama/calc-Gempyre)

 
 
 Some future development directions
---------------------

* Update / improve documentation (C++ and Python)  
* Binary releases (Maybe installer / some packet manager support / pip)
* Support for secure web socket (nice for remote UIs)
* Flatbuffer/protobuf instead of JSON (perf up)
* Using WASM instean of JS (perf up)
* Testing coverage and perf measurements.
* JS testing  
* Cleaning code and refactoring (API behind PIMPL?, string_views instead of strings when possible, meaningless std::any for code encapsulation)
* Supress subsystem warnings (gempyre itself has not warnings, but some libraries built in are leaking warnings).
* Reconsider libwebsockets instead of uWebsockets.
* POC of Gempyre-Android style architecture also in core.
* camelStyle or snake_style? (Gempyre-Python already uses snake, and Im warmin up for that :-)



Late updates
----------------

### 2021 1

* Proper build and install with cmake
* Use GTest for API testing
* Rewrote timers + other smaller fixes

### 2021 2

* Lot of fixes and some new utils
* Native application window for OSX, Linux and Windows are using [Hiillos] (https://github.com/mmertama/Hiillos)                        

### 2022 1

* Lot of fixes
* CI using Github actions 

### 2022 2

* Lot of fixes
* Raspberry, and imporoved MinGW support

Copyright Markus Mertama 2020, 2021, 2022

