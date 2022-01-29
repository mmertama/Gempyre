cmake 
	see https://snapcraft.io/install/cmake/raspbian#install
c++17	
	see (out of memory) https://forums.raspberrypi.com/viewtopic.php?t=294165

	see https://gist.github.com/sol-prog/95e4e7e3674ac819179acf33172de8a9

or
	

	https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/latest/download
	(ok, but no gdb)
	
	get gdb
	
	sudo apt-get install  libgmp-dev
	
	build
	
	wget https://ftp.gnu.org/gnu/gdb/gdb-11.2.tar.gz 
	tar vxzf ~/Downloads/gdb-7.12.tar.gz 
	cd gdb-11.2
	./configure
	make
	sudo make install
	#sudo make -j4 -C gdb/ install
	#sudo shutdown -r 0
	gdb --version

or 

	https://github.com/Pro/raspi-toolchain
	
	
	
