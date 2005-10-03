sbin_PROGRAMS   = xmondo
xmondo_SOURCES  = xmondobackup.cpp xmondorestore.cpp xmondo.cpp main.cpp xmondo.h xmondobackup.h xmondorestore.h \
                  xmondo.moc.cpp xmondobackup.moc.cpp xmondorestore.moc.cpp ../mondorestore/mondo-restore.c \
                  ../mondorestore/mondo-rstr-newt.c ../mondorestore/mondo-rstr-tools.c ../mondorestore/mondo-rstr-compare.c \
                  ../mondorestore/mondo-prep.c
AM_CPPFLAGS     = $(all_includes) @my_qt_includes@ @my_kde_includes@ -I../common -UDEBUG -Ubool -DMONDORESTORE_MODULE
xmondo_LDADD    = ../common/libXmondo.la $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET) -L/usr/local/lib -L/usr/X11R6/lib
xmondo_LDFLAGS  = $(all_libraries) @my_qt_libraries@ @my_kde_libraries@ $(KDE_RPATH)
pkgdata_DATA    = mondo.png

xmondobackup.moc.cpp: xmondobackup.h
	@MOC@ ./xmondobackup.h -o xmondobackup.moc.cpp

xmondo.moc.cpp: xmondo.h
	@MOC@ ./xmondo.h -o xmondo.moc.cpp

xmondorestore.moc.cpp: xmondorestore.h
	@MOC@ ./xmondorestore.h -o xmondorestore.moc.cpp

messages: 
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/xmondo.pot; \
	fi

