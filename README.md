![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Gempyre
=====
UI Framework
-------------

Gempyre is a C++ UI framework. It is a UI framework without widgets - instead  the UI is composed using common web tools and frameworks.  Therefore Gempyre is small, easy to learn and quick to take in use.

For Android support, visit also [Gempyre-Android](https://github.com/mmertama/Gempyre-Android)

If you prefer Python, please try [Gempyre-Python](https://github.com/mmertama/Gempyre-Python) 

The application engine is implemented using C++, the UI is constructed using  Javascript, CSS and HTML like any front end; all common frameworks from the vast pool web technologies shall be available as long as the UI elements can be refrerred using ids. Gempyre library provides a  C++ interface to interact with the UI - the API is only a few dozen calls. Gempyre is intended for applications that has a solid C++ core (or C) and takes benefit of rapid UI development without extra hassle with complex/expensive/platform specific/quirky UI development. Gempyre combines power of C++ with vast options of front end development tools, sources, documents frameworks that are only available on for Web Developers.

Gempyre is multiplatform, its core is written using C++17  (tested OSX (CLang), Ubuntu (gcc) and Windows 10 (MSVC) ). The Gempyre framework can be downloaded at Github under MIT license.

By default a Gempyre application is excecuted on system browser window. For native application outlook it possible to use __client application__ Client  application is given to Gempyre::Ui constructor as an alternative browser. Affiliates folder have Python and Qt clients. For Qt, [Qt](https://www.qt.io/) SDK must be installed and pointed with *QT_DIR*. When that is used, the Gempyre application looks like any other application window. 

Gempyre API has few headers

* [Gempyre core in _gempyre.h_](gempyre.md), everything that basic application needs.
* [Gempyre Utils in _gempyre_utils.h_](gempyre_utils.md), miscellaneous collection or function that helps writing applications.
* [Gempyre Graphics in _gempyre_graphics.h_](gempyre_graphics.md),  helps developing graphics intensive and games.
* gempyre_client.h, provides file dialogs when started using client application. 

How to build on Linux
* Run git clone https://github.com/mmertama/Gempyre.git.
* cmake CMakeLists.txt
* make 

How to build on MacOS
* Run git clone https://github.com/mmertama/Gempyre.git .
* cmake CMakeLists.txt
* make

How to build on Windows 10
* Install git bash from https://gitforwindows.org/
* Run git clone https://github.com/mmertama/Gempyre.git on git bash console.
* Install cmake https://cmake.org/download/ (let it to be added in path)
* Install Visual Studio https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2019, and pick Desktop development with C++
* Install Python 3.8 (3.6 >= shall be ok) https://www.python.org/downloads/windows/
* From Windows menu, Visual Stuudio: Open "x64 Native Tools Command Prompt for VS 2019"
* Run "msvc_build.bat" at Gempyre folder
 
 Projects using Gempyre:
 * [mandelbrot-Gempyre](https://github.com/mmertama/mandelbrot-Gempyre)
 * [treeview-Gempyre](https://github.com/mmertama/treeview-Gempyre)
 * [hexview-Gempyre](https://github.com/mmertama/hexview-Gempyre)
 * [calc-Gempyre](https://github.com/mmertama/calc-Gempyre)
 
 
 Things in pipeline
--------------------
* Update / improve documentation (C++ and Python)  
* make install for system install
* Binary releases (Maybe installer / some packet manager support / pip)
* Use GTest for API testing
* Raspberry build (remote and local UI)
* Native application window for OSX, Linux and Windows as default
* Support for secure web socket (nice for remote UIs)
* Flatbuffer instread of JSON 


Copyright
Markus Mertama 2020

