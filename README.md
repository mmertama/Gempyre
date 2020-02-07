![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Telex
=====
UI Framework
-------------

Telex is a C++ UI framework. It is a UI framework without widgets - instead  the UI is composed using common web tools and frameworks.  Therefore Telex is small, easy to learn and quick to take in use. 

The application engine is implemented using C++, the UI is constructed using  Javascript, CSS and HTML like any front end; all common frameworks from the vast pool web technologies shall be available as long as the UI elements can be refrerred using ids. Telex library provides a  C++ interface to interact with the UI - the API is only a few dozen calls. Telex is intended for applications that has a solid C++ core (or C) and takes benefit of rapid UI development without extra hassle with complex/expensive/platform specific/quirky UI development. Telex combines power of C++ with vast options of front end development tools, sources, documents frameworks that are only available on for Web Developers.

Telex is multiplatform, its core is written using C++17  (tested OSX (CLang), Ubuntu (gcc) and Windows 10 (MSVC) ).   The Telex framework can be downloaded at Github under MIT license. 

When started Telex application run on system default browser, but that can be defined otherwise. Especially in is "Telex Client" application written using embedded Chromium browser, the application is written on [Qt](https://www.qt.io/). When that is used, the Telex application looks like any other application window. The Telex Client sources are located in "affiliates" folder.

Telex API has few headers

* [Telex core in _telex.h_](telex.md), everything that basic application needs.
* [Telex Utils in _telex_utils.h_](telex_utils.md), miscellaneous collection or function that helps writing applications. 
* [Telex Graphics in _telex_graphics.h_](telex_graphics.md),  helps developing graphics intensive and games.
 

Copyright
Markus Mertama 2020

