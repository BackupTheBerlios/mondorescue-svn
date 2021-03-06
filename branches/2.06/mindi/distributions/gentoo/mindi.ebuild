# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /var/cvsroot/gentoo-x86/sys-apps/mindi/mindi-1.04.ebuild,v 1.1 2005/01/22 10:29:25 wschlich Exp $

DESCRIPTION="A program that creates emergency boot disks/CDs using your kernel, tools and modules."
HOMEPAGE="http://www.mondorescue.org"
SRC_URI="ftp://ftp.berlios.de/pub/mondorescue/src/${P}.tgz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~x86 -*"
IUSE=""

DEPEND="virtual/libc"
RDEPEND=">=app-arch/bzip2-0.9
		>=sys-apps/mindi-kernel-1.0-r1
		app-cdr/cdrtools
		sys-libs/ncurses
		sys-devel/binutils
		sys-fs/dosfstools
		sys-apps/gawk"

src_unpack() {
	unpack ${A} || die "Failed to unpack ${A}"
	cd ${S}/rootfs || die
	tar xzf symlinks.tgz || die "Failed to unpack symlinks.tgz"

	# This will need to change when IA64 is tested. Obviously.
	rm -f bin/busybox-ia64 sbin/parted2fdisk-ia64
	mv bin/busybox-i386 bin/busybox
}

src_install() {
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
