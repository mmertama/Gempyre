![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Gempyre
=====
UI Framework
-------------

Gempyre is a C++ UI framework. It is a UI framework without widgets - instead  the UI is composed using common web tools and frameworks.  Therefore Gempyre is small, easy to learn and quick to take in use.

The application engine is implemented using C++, the UI is constructed using  Javascript, CSS and HTML like any front end; all common frameworks from the vast pool web technologies shall be available as long as the UI elements can be refrerred using ids. Gempyre library provides a  C++ interface to interact with the UI - the API is only a few dozen calls. Gempyre is intended for applications that has a solid C++ core (or C) and takes benefit of rapid UI development without extra hassle with complex/expensive/platform specific/quirky UI development. Gempyre combines power of C++ with vast options of front end development tools, sources, documents frameworks that are only available on for Web Developers.

Gempyre is multiplatform, its core is written using C++17  (tested OSX (CLang), Ubuntu (gcc) and Windows 10 (MSVC) ).   The Gempyre framework can be downloaded at Github under MIT license.

When started Gempyre application run on system default browser, but that can be defined otherwise. Especially in is "Gempyre Client" application written using embedded Chromium browser, the application is written on [Qt](https://www.qt.io/). When that is used, the Gempyre application looks like any other application window. The Gempyre Client sources are located in "affiliates" folder.

Gempyre API has few headers

* [Gempyre core in _gempyre.h_](gempyre.md), everything that basic application needs.
* [Gempyre Utils in _gempyre_utils.h_](gempyre_utils.md), miscellaneous collection or function that helps writing applications.
* [Gempyre Graphics in _gempyre_graphics.h_](gempyre_graphics.md),  helps developing graphics intensive and games.
 

Copyright
Markus Mertama 2020

