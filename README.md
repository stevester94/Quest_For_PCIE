- Configuring and Building QEMU
	- git submodule init
	- git submodule update --recursive
	- install the following
		zlib-devel.x86_64  gtk2-devel glib2-devel.x86_64  pixman-devel.x86_64 
	- copy Makefile.objs to hw/misc
	- create build dir in qemu folder
	- run ../configure targets-list=x86_64-softmmu in the build dir
	- link device source files into hw/misc 
	- run make from build dir
- running
	- just use redir_start.bash to start the vm
		- assuming debian8.qcow2 is in the qemu dir
	- ssh 127.0.0.1:2222
	- in the VM, clone the same repo, then build respective driver and insmod