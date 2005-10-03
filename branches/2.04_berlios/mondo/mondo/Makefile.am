# Note that configure will define @XMONDO@ to be "xmondo" if X support
# is enabled, "" if not
SUBDIRS        = common mondoarchive mondorestore @XMONDO@ docs restore-scripts
DIST_SUBDIRS   = common mondoarchive mondorestore xmondo docs restore-scripts

pkgdata_DATA   = do-not-compress-these

install-data-hook: post-nuke.sample
	cp -R post-nuke.sample $(pkgdatadir)/
