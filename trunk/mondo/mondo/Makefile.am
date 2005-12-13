SUBDIRS        = common mondoarchive mondorestore docs restore-scripts

pkglib_DATA   = do-not-compress-these

install-data-hook: post-nuke.sample
	cp -R post-nuke.sample $(pkglibdir)/
