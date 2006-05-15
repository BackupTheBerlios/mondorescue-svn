# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Id$

DESCRIPTION="A program that creates emergency boot disks/CDs using your kernel, tools and modules."
HOMEPAGE="http://www.mondorescue.org"
SRC_URI="ftp://ftp.mondorescue.org/src/${P}.tgz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~x86 -*"
IUSE=""

DEPEND="virtual/libc"
RDEPEND=">=app-arch/bzip2-0.9
		>=sys-apps/mindi-kernel-1.0-r1
		sys-libs/ncurses
		sys-devel/binutils
		sys-fs/dosfstools
		sys-apps/gawk"

src_unpack() {
	unpack ${A} || die "Failed to unpack ${A}"
}

src_install() {
	export PREFIX="/usr"
	export CONFDIR="/etc"
	export RPMBUILDMINDI="true"
	./install.sh
}

pkg_postinst() {
	einfo "${P} was successfully installed."
	einfo "Please read the associated docs for help."
	einfo "Or visit the website @ ${HOMEPAGE}"
	echo
	ewarn "This package is still in unstable."
	ewarn "Please report bugs to http://bugs.gentoo.org/"
	ewarn "However, please do an advanced query to search for bugs"
	ewarn "before reporting. This will keep down on duplicates."
	echo
}
