![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Gempyre
=====
UI Framework
-------------

Gempyre is a UI framework. It is a UI framework without widgets - instead  the UI is composed using common web tools and frameworks.  Therefore Gempyre is small, easy to learn and quick to take in use.

For Android support, visit also [Gempyre-Android](https://github.com/mmertama/Gempyre-Android)

If you prefer Python, please try [Gempyre-Python](https://github.com/mmertama/Gempyre-Python) 

The application engine is implemented using C++, the UI is constructed using  Javascript, CSS and HTML like any front end; all common frameworks from the vast pool web technologies shall be available as long as the UI elements can be refrerred using ids. Gempyre library provides a  C++ interface to interact with the UI - the API is only a few dozen calls. Gempyre is intended for applications that has a solid C++ core (or C) and takes benefit of rapid UI development without extra hassle with complex/expensive/platform specific/quirky UI development. Gempyre combines power of C++ with vast options of front end development tools, sources, documents frameworks that are only available on for Web Developers.

Gempyre is multiplatform, its core is written using C++17  (tested OSX (CLang), Ubuntu (gcc) and Windows 10 (MSVC) ). The Gempyre framework can be downloaded at Github under MIT license.

Gempyre itself does not contain application window and bare Gempyre applications are excecuted on system browser window. However for native application a __client application__ is used. Native application window for OSX, Linux and Windows are using [Hiillos] (https://github.com/mmertama/Hiillos). However Gempyre::Ui constructor can support other client applications as well - for example [Gempyre-Python](https://github.com/mmertama/Gempyre-Python) uses a Python client and [Gempyre-Android](https://github.com/mmertama/Gempyre-Android) is a special native application. 


Gempyre API has few headers

* [Gempyre core in _gempyre.h_](gempyre.md), everything that basic application needs.
* [Gempyre Utils in _gempyre_utils.h_](gempyre_utils.md), miscellaneous collection or function that helps writing applications.
* [Gempyre Graphics in _gempyre_graphics.h_](gempyre_graphics.md),  helps developing graphics intensive and games.
* gempyre_client.h, provides file dialogs when started using client application. 

How to build on Linux and MacOS
* Run git clone https://github.com/mmertama/Gempyre.git.
* md build
* cd build
* cmake ..
* cmake --build .
* sudo cmake --install .

How to build on Windows 10
* Install git bash from https://gitforwindows.org/
* Run git clone https://github.com/mmertama/Gempyre.git on git bash console.
* Install cmake https://cmake.org/download/ (let it to be added in path)
* MSVC:
    * Install Visual Studio https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2019, and pick Desktop development with C++
    * Install Python 3.8 (3.6 >= shall be ok) https://www.python.org/downloads/windows/
    * From Windows menu, Visual Stuudio: Open "x64 Native Tools Command Prompt for VS 2019"
    * Run "msvc_install.bat" at Gempyre folder.
* MinGW
    * See Instrictions https://www.devdungeon.com/content/install-gcc-compiler-windows-msys2-cc
    * Ensure Ninja is installed "packman -s base-devel gcc vim cmake ninja"
    * Add C:\msys64\mingw64\bin and C:\msys64\usr\bin to your path
    * Then you can execute "mingw_install.bat" Windows Command promt at Gempyre folder.
 
 Projects using Gempyre:
 * [mandelbrot-Gempyre](https://github.com/mmertama/mandelbrot-Gempyre)
 * [treeview-Gempyre](https://github.com/mmertama/treeview-Gempyre)
 * [hexview-Gempyre](https://github.com/mmertama/hexview-Gempyre)
 * [calc-Gempyre](https://github.com/mmertama/calc-Gempyre)
 
 
 Things to do and in pipeline
---------------------
* Update / improve documentation (C++ and Python)  
* Binary releases (Maybe installer / some packet manager support / pip)
* Raspberry build (remote and local UI)
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
* Rust wrapper


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

Copyright
Markus Mertama 2020, 2021

