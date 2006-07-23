SUBDIRS        = common mondoarchive mondorestore restore-scripts post-nuke.sample
DIST_SUBDIRS   = ${SUBDIRS}

pkgdata_DATA   = do-not-compress-these autorun

all:	all-am chmod

chmod:	${DESTDIR}/${pkgdatadir}/autorun
	chmod 755 ${DESTDIR}/${pkgdatadir}/autorun
