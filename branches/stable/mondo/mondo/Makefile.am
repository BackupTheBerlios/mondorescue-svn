SUBDIRS        = common mondoarchive mondorestore restore-scripts post-nuke.sample
DIST_SUBDIRS   = ${SUBDIRS}

pkgdata_DATA   = do-not-compress-these
scriptdir      = $(pkgdatadir)
script_DATA    = autorun

install-data-hook:
	@for prog in $(script_DATA); do \
	  chmod +x $(scriptdir)/$$prog; \
	done
