# Built on Raspberry
### Raspberry 3 with Rasbian 


** still in early phase, performance and stability are under construction **


1) Quite late Cmake is required, here are [snap instructions](https://snapcraft.io/install/cmake/raspbian#install).

2) Gcc with C++17 has few options, you may try to build it yourself https://forums.raspberrypi.com/viewtopic.php?t=294165

or use some precompiled version:
	see https://gist.github.com/sol-prog/95e4e7e3674ac819179acf33172de8a9
or
	https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/latest/download
	
3) optionally - however those were missing gdb, therefore for development build gdb:

	3.1)
	
	Install late GMP 
	
	```bash
	sudo apt-get install  libgmp-dev
	```
	
	3.2) build from sources:
	
	```bash
	wget https://ftp.gnu.org/gnu/gdb/gdb-11.2.tar.gz 
	tar vxzf ~/Downloads/gdb-7.12.tar.gz 
	cd gdb-11.2
	./configure
	make
	sudo make install
	```
	
	3.3) See gdb --version what version you have, for me this 11.2 version
	get installed into /usr/local/bin/gdb.

4) run
    ```bash
    $ ./raspberry_install.sh
    ```

This is Gempyre core, apps get open into browser window. (look Examples)

For windowed apps a [Hiillos](https://github.com/mmertama/Hiillos) is needed. 
