##
## Process with Automake to generate Makefile.in
##

## Headers
noinst_HEADERS       = mondo-cli-EXT.h mondo-cli.h

## The program
sbin_PROGRAMS        = mondoarchive
mondoarchive_SOURCES = main.c mondo-cli.c
mondoarchive_LDADD   = ${top_builddir}/mondo/common/libmondo.a
