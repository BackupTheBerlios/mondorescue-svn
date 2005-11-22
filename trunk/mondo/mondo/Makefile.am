SUBDIRS        = common mondoarchive mondorestore docs restore-scripts

pkgdata_DATA   = do-not-compress-these

install-data-hook: post-nuke.sample
	cp -R post-nuke.sample $(pkgdatadir)/
