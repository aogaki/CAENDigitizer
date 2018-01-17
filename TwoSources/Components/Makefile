# -*- Makefile -*-
#
# @file  Makefile
# @brief makefile for DaqComponents
# @date  $Date: 2009/06/12 10:42:55 $
#

all:
	make -f Makefile.PSD
	make -f Makefile.PHA
	make -f Makefile.Dispatcher
	make -f Makefile.Logger
	make -f Makefile.Monitor

clean:
	rm ../../src/*.o
	make -f Makefile.PSD clean
	make -f Makefile.PHA clean
	make -f Makefile.Dispatcher clean
	make -f Makefile.Logger clean
	make -f Makefile.Monitor clean
