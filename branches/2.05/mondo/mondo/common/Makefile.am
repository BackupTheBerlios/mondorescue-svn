##
## Process this file with Automake to generate `Makefile.in'
##

## Headers
noinst_HEADERS       = libmondo-archive.h libmondo-devices.h \
                       libmondo-filelist.h libmondo-files.h libmondo-fork.h \
                       libmondo-gui.h libmondo-mountlist.h libmondo-raid.h \
                       libmondo-stream.h libmondo-string.h libmondo-tools.h \
                       libmondo-verify.h libmondo-fifo.h \
                       libmondo-archive-EXT.h my-stuff.h mondostructures.h \
                       libmondo-devices-EXT.h libmondo-fifo-EXT.h libmondo-filelist-EXT.h \
                       libmondo-files-EXT.h libmondo-fork-EXT.h libmondo-gui-EXT.h \
                       libmondo-mountlist-EXT.h libmondo-raid-EXT.h libmondo-stream-EXT.h \
                       libmondo-string-EXT.h libmondo-tools-EXT.h libmondo-verify-EXT.h \
                       X-specific.h newt-specific.h

## Libraries
if XWIN
 XMONDOLIB           = libXmondo.la
endif XWIN
lib_LTLIBRARIES      = libmondo.la libmondo-newt.la $(XMONDOLIB)

## libmondo
libmondo_la_SOURCES  = libmondo-archive.c libmondo-devices.c libmondo-fifo.c \
                       libmondo-filelist.c libmondo-files.c libmondo-fork.c \
                       libmondo-mountlist.c libmondo-raid.c \
                       libmondo-stream.c libmondo-string.c libmondo-tools.c \
                       libmondo-verify.c 
libmondo_la_LDFLAGS  = -version-info 2:3:0 -shrext .so
libmondo_la_LIBADD   = @LIBS@

## libmondo-newt
libmondo_newt_la_SOURCES = newt-specific.c
libmondo_newt_la_LDFLAGS = -version-info 1:0:0 -shrext .so
libmondo_newt_la_LIBADD  = -lnewt

## libXmondo
libXmondo_la_SOURCES = libmondo-archive.c libmondo-devices.c libmondo-fifo.c \
                       libmondo-filelist.c libmondo-files.c libmondo-fork.c \
                       X-specific.cpp libmondo-mountlist.c libmondo-raid.c \
                       libmondo-stream.c libmondo-string.c libmondo-tools.c \
                       libmondo-verify.c 
libXmondo_la_LDFLAGS = -version-info 1:0:0 -shrext .so
libXmondo_la_LIBADD  = -L/usr/X11R6/lib -L/usr/local/lib @QT_LIBRARY@ @LIBS@ @my_qt_libraries@ @my_kde_libraries@
libXmondo_la_CFLAGS  = -D_XWIN
libXmondo_la_CXXFLAGS= -D_XWIN @my_qt_includes@ @my_kde_includes@
