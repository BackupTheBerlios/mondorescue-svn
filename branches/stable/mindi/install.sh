#!/bin/bash

if [ ! -f "mindi" ] ; then
    echo "Please 'cd' to the directory you have just untarred." >> /dev/stderr
    exit 1
fi

if [ "_$PREFIX" != "_" ]; then
	local=$PREFIX
	if [ -f /usr/local/sbin/mindi ]; then
		echo "WARNING: /usr/local/sbin/mindi exists. You should probably remove it !"
	fi
	conf=$CONFDIR/mindi
	echo $PATH | grep /usr/sbin > /dev/null || echo "Warning - your PATH environmental variable is BROKEN. Please add /usr/sbin to your PATH."
else
	local=/usr/local
	if [ -f /usr/sbin/mindi ]; then
		echo "WARNING: /usr/sbin/mindi exists. You should probably remove the mindi package !"
	fi
	conf=$local/etc/mindi
	echo $PATH | grep $local/sbin > /dev/null || echo "Warning - your PATH environmental variable is BROKEN. Please add $local/sbin to your PATH."
fi

if uname -a | grep Knoppix > /dev/null || [ -e "/ramdisk/usr" ] ; then
    local=/ramdisk/usr
    export PATH=/ramdisk/usr/sbin:/ramdisk/usr/bin:/$PATH
fi

MINDIVER=`cat VERSION`
MINDIREV=`cat REVISION`
echo "mindi ${MINDIVER}-r${MINDIREV} will be installed under $local"

DOCDIR=$local/share/doc/mindi-$MINDIVER
MANDIR=$local/share/man/man8

ARCH=`/bin/arch`
echo $ARCH | grep -x "i[0-9]86" &> /dev/null && ARCH=i386 && locallib=$local/lib
# For the moment, we don't build specific x86_64 busybox binaries
echo $ARCH | grep -x "x86_64" &> /dev/null && ARCH=i386 && locallib=$local/lib64
export ARCH

echo "Creating target directories ..."
install -m 755 -d $conf $locallib/mindi $MANDIR $local/sbin $DOCDIR

echo "Copying files ..."
install -m 644 isolinux.cfg msg-txt sys-disk.raw.gz isolinux-H.cfg syslinux.cfg syslinux-H.cfg dev.tgz $locallib/mindi
install -m 644 deplist.txt $conf

cp -af rootfs aux-tools $locallib/mindi
chmod 755 $locallib/mindi/rootfs/bin/*
chmod 755 $locallib/mindi/rootfs/sbin/*
chmod 755 $locallib/mindi/aux-tools/sbin/*

if [ "$RPMBUILDMINDI" = "true" ]; then
	sed -e "s~^MINDI_PREFIX=XXX~MINDI_PREFIX=/usr~" -e "s~^MINDI_CONF=YYY~MINDI_CONF=/etc/mindi~" -e "s~^MINDI_VER=VVV~MINDI_VER=$MINDIVER~" -e "s~^MINDI_REV=RRR~MINDI_REV=$MINDIREV~" mindi > $local/sbin/mindi
else
	sed -e "s~^MINDI_PREFIX=XXX~MINDI_PREFIX=$local~" -e "s~^MINDI_CONF=YYY~MINDI_CONF=$conf~" -e "s~^MINDI_VER=VVV~MINDI_VER=$MINDIVER~" -e "s~^MINDI_REV=RRR~MINDI_REV=$MINDIREV~" mindi > $local/sbin/mindi
fi
chmod 755 $local/sbin/mindi
install -m 755 analyze-my-lvm parted2fdisk.pl $local/sbin

install -m 644 mindi.8 $MANDIR
install -m 644 ChangeLog COPYING README README.busybox README.ia64 README.pxe TODO INSTALL svn.log $DOCDIR

# Managing busybox
if [ -f $locallib/mindi/rootfs/bin/busybox-$ARCH ]; then
		echo "Installing busybox ..."
		install -s -m 755 $locallib/mindi/rootfs/bin/busybox-$ARCH $locallib/mindi/rootfs/bin/busybox
else
		echo "WARNING: no busybox found, mindi will not work on this arch ($ARCH)"
fi
if [ "$ARCH" = "i386" ] ; then
	if [ -f $locallib/mindi/rootfs/bin/busybox-$ARCH.net ]; then
		echo "Installing busybox.net ..."
		install -s -m 755 $locallib/mindi/rootfs/bin/busybox-$ARCH.net $locallib/mindi/rootfs/bin/busybox.net
	else
		echo "WARNING: no busybox.net found, mindi will not work on this arch ($ARCH) with network"
	fi
fi
# Remove left busybox
rm -f $locallib/mindi/rootfs/bin/busybox-*

# Managing parted2fdisk
if [ "$ARCH" = "ia64" ] ; then
	(cd $local/sbin && ln -sf parted2fdisk.pl parted2fdisk)
	make -f Makefile.parted2fdisk DEST=$locallib/mindi install
	if [ -f $locallib/mindi/rootfs/sbin/parted2fdisk-$ARCH ]; then
		echo "Installing parted2fdisk ..."
		install -s -m 755 $locallib/mindi/rootfs/sbin/parted2fdisk-$ARCH $locallib/mindi/rootfs/sbin/parted2fdisk
		install -s -m 755 $locallib/mindi/rootfs/sbin/parted2fdisk-$ARCH $local/sbin/parted2fdisk
	else
		echo "WARNING: no parted2fdisk found, mindi will not work on this arch ($ARCH)"
	fi
else
	# FHS requires fdisk under /sbin
	(cd $local/sbin && ln -sf /sbin/fdisk parted2fdisk)
	echo "Symlinking fdisk to parted2fdisk"
	( cd $locallib/mindi/rootfs/sbin && ln -sf fdisk parted2fdisk)
fi
# Remove left parted2fdisk
rm -f $locallib/mindi/rootfs/sbin/parted2fdisk-*

if [ "$RPMBUILDMINDI" != "true" ]; then
	chown -R root:root $locallib/mindi $conf $DOCDIR
	chown root:root $local/sbin/mindi $MANDIR/mindi.8 $local/sbin/analyze-my-lvm $local/sbin/parted2fdisk.pl 
	if [ "$ARCH" = "ia64" ] ; then
		chown root:root $local/sbin/parted2fdisk
	fi
fi

# Special case for SuSE family where doc is put elsewhere in the RPM
if [ _"$dfam" = _"suse" ]; then
	rm -rf $DOCDIR
fi

exit 0
