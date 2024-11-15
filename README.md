![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Gempyre
=====
UI Framework
-------------

Gempyre is a UI multiplatform framework. Supporting Windows, Mac OSX, Linux, Raspberry OS and Android. Gempyre is minimalistic and simple; It is a UI framework without widgets - instead, the UI is composed using common web tools and frameworks. Therefore Gempyre is small, easy to learn and quick to take in use.
 
For the application, Gempyre let engine to be implemented using C++ (or Python), the UI can be constructed using CSS and HTML like any front end. All common tools from the web technologies are be available. Gempyre library provides a simple and easy C++ API for a  application development and the whole API is only a few dozen calls.

Gempyre is intended for applications that has a solid C++ core (or C), and allows rapid UI development without extra hassle with platform specific UI development. 

Gempyre is multiplatform, its core is built using C++20  (tested on OSX (CLang), Ubuntu (gcc), Raspberry OS (gcc) and Windows 10 (MSVC and MinGW) ). The Gempyre framework can be downloaded at Github under MIT license.

Gempyre itself does not contain an application window. The UI is drawn using external application. Some OSes defaults to system browser, some Python webview. However that is fully configurable per application. 

Gempyre is a library that is linked with the application, except for Android, see [Gempyre-Android](https://github.com/mmertama/Gempyre-Android). For Python, install [Gempyre-Python](https://github.com/mmertama/Gempyre-Python) on top of Gempyre library.


### Gempyre API 

* gempyre.h, core classes for the application development.
* gempyre_utils.h, miscellaneous collection of helper functions. These are not available for Python as there are plenty of analogous functionality.  
* gempyre_graphics.h, HTML canvas functions for bitmap and vector graphics.
* gempyre_client.h, file dialogs for Gempyre application. Not available when system browser is used as an application window. 

[Gempyre Documentation](https://mmertama.github.io/Gempyre/)

#### Note (11/24)

I have changed default Web socket library and that may arise build issues. If there are issues you may try to set 
`USE_LIBWEBSOCKETS` to OFF. (e.g. cmake -DUSE_LIBWEBSOCKETS=OFF ...).
(I will remove this note later)

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
* Install Python (latest, yet tested mostly with 3.10, but 3.6 >= shall be ok) https://www.python.org/downloads/windows/
* From Windows menu, Visual Studio: Open "x64 Native Tools Command Prompt for VS 2019" (later probably ok)
* Run 
   ```bat
    msvc_install.bat
   ```

#### MinGW

* Make sure you are using the right MinGW shell (Msys minGW 64-bit - one with blue icon (Not brown or cyan :-))
* See Instructions https://www.devdungeon.com/content/install-gcc-compiler-windows-msys2-cc
* Ensure Ninja is installed `pacman -S base-devel gcc vim cmake ninja`
* Run
  ```bash
  mingw_install.sh
  ```
 
### Raspberry OS

 * Requires Rasberry OS Bullseye (older is ok, but you need more recent gcc in order to build C++17). Tested Raspberry Pi 3 and Raspberry Pi 4.  
 * Quite late CMake is required, here are [snap instructions](https://snapcraft.io/install/cmake/raspbian#install).
 * Run
  ```bash
        pi@raspberrypi:~/Development/Gempyre ./raspberry_install.sh
   ```
 * When building on Raspberry, please pass -DRASPBERRY=1 for your cmake call. E.g. 
  ```bash
         pi@raspberrypi:~/Development/Tilze/build $ cmake .. -DRASPBERRY=1
   ```     
## FAQ
Q: After installation you get: "WARNING: webview is not installed -> verification is not completed!", what is that?</br>
A: Most likely python3 webview is not installed. See installation from [pywebview](https://pywebview.flowrl.com/guide/installation.html). Please also make sure websockets python library is installed. 
```bash
$ pip3 install pywebview && pip3 install websockets
```
The error is not fatal, but as a consequence the default UI is not drawn on its own window and it fallbacks to the default browser.

Q: How to use some HTML/JS/CSS feature for GUI from C++ that does not have a API?</br>
A: Try `Ui::eval()`, it let you execute javascript in the gui context: e.g. Here I change a checkbox element so it fires a change event.

```cpp
ui.eval("document.getElementById(\"" + check_box.id() + "\").click();");
```
Q: Why Canvas drawings seems to happen in random order?</br>
A: Canvas drawing is highly asynchronous operation and subsequent `CanvasElement::draw()` operations may not get updated in the order you have called them. The solution is either make a single draw call where all items are updated once, or use `CanvasElement::draw_completed()` to wait until each draw has happen. `CanvasElement::draw_completed()` should be used for animations (or game like frequent updates) instead of timers for updating the graphics. Like this [example](https://github.com/mmertama/Gempyre/blob/6ef49c831c39f8dd67a0cd656f26dae4a2ff46e0/examples/fire/src/main.cpp#L100)

Q: Why `Gempyre::Bitmap` merge is not working?</br>
A: You are likely merge an uninitialized bitmap. `Gempyre::Bitmap(width, height)` or `Gempyre::Bitmap::create(width, height)` does not initialize bitmap data, therefore it's alpha channel can be zero and bitmap
is not drawn. To initialize the bitmap, use `Gempyre::Bitmap::Bitmap(0, 0, width, height, Gempyre::Color::White)`, or draw a rectangle to fill the Bitmap, after the construction. 

Q: How to scale `Gempyre::Bitmap`?</br>
A: You can use browser's bitmap scaling by applying the bitmap as image and then using `Gempyre::FrameComposer` for draw. Look the following snippet:

```cpp

// name the image
const auto image_key = "/my_image.png";

// I define a lambda for drawing to avoid copy-paste. texture_images maps image names and ids.
const auto texture_draw = [&ui, &texture_images, image_key, clip, rect]() {
  Gempyre::CanvasElement compose(ui, COMPOSE_CANVAS);
  Gempyre::FrameComposer fc;
  const auto id = texture_images[image_key];
  fc.draw_image(id, clip, target);
  compose.draw(fc);
};

 // test if image is already loaded
 const auto res = ui.resource(image_key);
    if(!res) {
      const auto png = bmp.png_image();  // bmp is the image we want to scale - but we make a png out of it
      if(ui.add_data(image_key, png)) {  // load the image
        const auto id = compose.add_image(image_key, [texture_draw](auto){
          texture_draw();                // draw when loaded, too early request wont draw anything
        });
        texture_images.emplace(image_key, id);    
      }
    } else {
      assert(ui.available(texture_images[image_key]));
      texture_draw();
    }
```

Q: How to do multiple `Gempyre::FrameComposer` in a single draw? My second draw is not visible</br>
A: Do you call erase? See `CanvasElement::draw_completed above. You may also try to call your drawn in a `Ui::after` timer with 0ms delay, that should ensure the correct drawing order.

Q: Why my dynamic HTML `<select>` does not work with numbers?</br>
A: The UI select uses strings, and hence the values has to be _quoted_ strings.
```html
  <select id="show_tags" disabled></select>  
```

```cpp

   Gempyre::Element show_tags(ui, "show_tags");
   show_tags.remove_attribute("disabled");  // remove disabled
   // dynamical add items numerical keys
   for(const int value : tag_keys) {
        Gempyre::Element opt{ui, "option", show_tags};
        const auto value = std::to_string(value);   // number as a string
        opt.set_attribute("value", GempyreUtils::qq(value)); // The numerical values must be quoted, otherwise JS interpret them as numbers!
        opt.set_attribute("name", "tag_option");
        opt.set_html(value);
    }

  ```
Q: I want to use Python window to have nice window and get file dialogs. But I do not get working in MacOS.
A: The latest MacOS wish your run your Python in virtual environment. Look [venv](https://docs.python.org/3/library/venv.html). As in any environment you may need to install pywebview and websockets. For example in venv shell call: 

`$ pip install pywebview websockets`

However I noted that pywebview may not get successfully installed, and hence a link must be called, for example (please verify your python and pywebview versions): 

`$ ln -s ./venv/lib/python3.12/site-packages/pywebview-5.1.dist-info ./venv/lib/python3.12/site-packages/pywebview`   

## Example

### Hello world
As an example, here is a simple Gempyre application with a single button. The UI is written using HTML; please note the <b> <code> &lt;script type="text/javascript" src="gempyre.js"&gt; &lt;/script&gt; </code> </b> line is required for every Gempyre application. The HTML widgets are accessed by their HTML element ids from the C++.

```cmake
cmake_minimum_required(VERSION 3.26)

project( hello VERSION 1.0 LANGUAGES CXX)

add_executable(${PROJECT_NAME} main.cpp)

# Find Gempyre after installation 
find_package(Gempyre REQUIRED)

# Add gempyre_add_resources
include(gempyre)

gempyre_add_resources(PROJECT ${PROJECT_NAME}
    TARGET include/hello_resources.h 
    SOURCES gui/hello.html)


set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME} PRIVATE . include)
target_link_libraries(${PROJECT_NAME} gempyre::gempyre)

```

```html 

 <!doctype html>

<html lang="en">

    <head>

        <meta charset="utf-8">

        <title>Hello</title>

    </head>

    <body>

        <script type="text/javascript" src="gempyre.js"></script>

        <button id="startbutton">...</button>

        <div id="content"></div>

    </body>

</html>
``` 

And then we have a main.cpp. I discuss here every line by line. At first, *gempyre.h* is included so it can be used.

```cpp 

#include  <gempyre.h>

 ```

Within Gempyre you normally build in the HTML and other resources, to compose a single file executable. It is preferred that Gempyre is statically linked in, thus the application is just a single file. Therefore distributing and executing Gempyre applications shall be very easy: just run it! There are no runtimes to install nor DLLs to be dependent on; just a single binary executable.

The resource composing is done in CMakeLists.txt (single line), yet here the generated header is included, it contains a `Hellohtml`, a `std::string` object that will be passed then to the `Gempyre::Ui` constructor.

```cpp 

#include "hello_resources.h"

int main(int /*argc*/, char** /*argv*/)  {

```
 

In the constructor, you provide a mapping between file names and generated data and the HTML page that will be shown.

 ```cpp

   Gempyre::Ui ui({ {"/hello.html", Hellohtml} }, "hello.html");

``` 

HTML elements are represented by `Gempyre::Element`. The element constructor takes a HTML id to refer to the corresponding element in the UI. Here we refer to the text area and button as defined in the HTML code above. The `Gempyre::Element` represents any of the HTML elements. It's only inherited class is `Gempyre::CanvasElement` that implements specific graphics functionalities.

 ```cpp

  Gempyre::Element text(ui, "content");

  Gempyre::Element button(ui, "startbutton");

```
 

The Gempyre API provides a set of methods to get and set HTML content, values, and attributes. Here we just use setHTML to apply a given string as an element HTML content.

```cpp 

   button.set_html("Hello?");

```

The subscribe method listens for element events, and when the button is "clicked" a given function is executed.

 ```cpp

 button.subscribe("click", [&text]() {

      text.set_html("Hello World!");

    });

```

The `Gempyre::Ui::run()` starts an event loop. In this example, the system default web browser is opened, and the UI is executed on tab (there are more alternatives to have a system looking application) and the application is waiting for the button to be pressed or the browser window to be closed.

```cpp 

  ui.run();

  return 0;

}

```
### Selection list
Dynamic selection list (or combo box) is as easy as adding an empty <i>select</i> element in the Ui HTML code:
```html
<select name="levels" id="level_select">
</select>
```
and then fill it in cpp side:

```cpp
Gempyre::Element select{ui, "level_select"};
for(const std::string& level : wp.levels()) {
   Gempyre::Element opt{ui, "option", select};
   opt.set_attribute("value", level);
   opt.set_html(level);
   }
```

to read a value upon change you subscribe it's change:

```cpp
select.subscribe(Gempyre::Event::CHANGE, [](const auto& e) {
    std::cout << e.properties.at("value") << std::endl;
}, {"value"});
```

Applying an initial value is trivial:

```cpp
select.set_attribute("value", wp.levels[level_index]); // set value to some other than 1st
```

Please note that as each event has a lot of properties, you have to list what you 
need. For a selection change (as well as most of the inputs) a <b>value</b> is used.   

### WebP support
WebP is a image format that can be used to replace PNG and GIF (animations). There is an external project [WebP-Gempyre](https://github.com/mmertama/webp-gempyre) that will help using WebP images with Gempyre.

 ### Some example projects using Gempyre
 
 * [Examples](https://github.com/mmertama/Gempyre/tree/raspberry/examples)
 * [mandelbrot-Gempyre](https://github.com/mmertama/mandelbrot-Gempyre)
 * [treeview-Gempyre](https://github.com/mmertama/treeview-Gempyre)
 * [hexview-Gempyre](https://github.com/mmertama/hexview-Gempyre)
 * [calc-Gempyre](https://github.com/mmertama/calc-Gempyre)

 
 
 Some future development directions
---------------------

* Binary releases (Maybe installer / some packet manager support / pip)
* Support for secure web socket (nice for remote UIs)
* Testing coverage and perf measurements.
* JS testing  

maybe not
* Flatbuffer/protobuf instead of JSON
* Using WASM instead of JS
* POC of Gempyre-Android style architecture also in core.git


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
* Raspberry, and improved MinGW support

### 2023 1
* Lot of fixed
* API changes and harmonize with GemGui-rs
* Testing coverage

### 2023 2
* Python UI fixes
* Performance update
* Refactoring internals
* Ready to change uwebsockets to libwebsockets or websockets++. (however not completed)
* Pedantic and Sanitizers
* Suppress external library warnings (except MSVC not working, it seems :-/ )
* Update / improve documentation 

### 2024
* API change const std::string& --> std::string_view where incompatibility allows
* Some new methods and utilities

Copyright Markus Mertama 2020, 2021, 2022, 2023, 2024

