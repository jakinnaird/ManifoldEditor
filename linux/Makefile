# Manifold Editor
# Makefile for Linux
#
# To use, run:
#
# make
#
# This will compile Irrlicht as a shared library, run cmake to prepare
# Manifold Editor to compile, and build the application
#
# All artifacts are dropped in the bin/ folder
#

all: irrlicht editor
clean: irrlicht-clean editor-clean

irrlicht:
	cd ../thirdparty/irrlicht-1.8.5/source/Irrlicht && make sharedlib NDEBUG=1
	mv ../thirdparty/irrlicht-1.8.5/lib/Linux/libIrrlicht.so.1.8.5 ../bin/libIrrlicht.so.1.8

irrlicht-clean:
	cd ../thirdparty/irrlicht-1.8.5/source/Irrlicht && make clean
	rm ../bin/libIrrlicht.so.1.8 ../thirdparty/irrlicht-1.8.5/source/Irrlicht/libIrrlicht.so.1.8.5

editor: irrlicht
	cd .cmake && cmake .
	cd .cmake && make
	mv .cmake/ManifoldEditor ../bin

editor-clean:
	cd .cmake && make clean
	rm ../bin/ManifoldEditor
