# Makefile using Automake + Autoconf
# ----------------------------------
# $Id$

# To create statically linked executables (see configure.ac)
CXXLD=$(STATICTOOL) $(CXX)

bin_PROGRAMS=$(X11PROG)
EXTRA_PROGRAMS=FreeFem++-x11

# FFCS:visualization stream redirection
FreeFem___x11_SOURCES=../Graphics/Xrgraph.cpp  ../mpi/parallelempi-empty.cpp ../fflib/ffapi.cpp
FreeFem___x11_DEPENDENCIES=../fflib/libff.a ../lglib/liblg.a
LDADD=../lglib/liblg.a ../fflib/libff.a @UMFPACKLIBS@ @ARPACKLIBS@ @BLASLIBS@ @X11_LIBS@
AM_CPPFLAGS=-I$(srcdir)/../fflib -I$(srcdir)/../Graphics -I$(srcdir)/../.. -I$(srcdir)/../femlib @X_CFLAGS@ 
