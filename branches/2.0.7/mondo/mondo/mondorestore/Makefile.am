##
## Process with Automake to generate Makefile.in
##

## Headers
noinst_HEADERS        = mondo-restore.h mondo-rstr-compare.h mondo-rstr-newt.h mondo-rstr-tools.h \
                        mondo-restore-EXT.h mr-externs.h \
                        mondo-rstr-compare-EXT.h mondo-rstr-tools-EXT.h mondoprep.h

## The program
sbin_PROGRAMS         = mondorestore
mondorestore_SOURCES  = mondo-prep.c mondo-restore.c mondo-rstr-newt.c \
                        mondo-rstr-compare.c mondo-rstr-tools.c
mondorestore_LDADD    = ${top_builddir}/mondo/common/libmondo.a @MONDORESTORE_STATIC@
