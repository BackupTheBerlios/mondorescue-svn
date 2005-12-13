#!/bin/bash

if [ ! -f "mindi" ] ; then
    echo "Please 'cd' to the directory you have just untarred." >> /dev/stderr
    exit 1
fi

if [ "_$PREFIX" != "_" ]; then
	local=$PREFIX/usr
	if [ -f /usr/local/sbin/mindi ]; then
		echo "WARNING: /usr/local/sbin/mindi exists. You should probably remove it !"
	fi
	conf=$PREFIX/etc/mindi
else
	local=/usr/local
	if [ -f /usr/sbin/mindi ]; then
		echo "WARNING: /usr/sbin/mindi exists. You should probably remove the mindi package !"
	fi
	conf=$local/etc/mindi
fi

if uname -a | grep Knoppix > /dev/null || [ -e "/ramdisk/usr" ] ; then
    local=/ramdisk/usr
    export PATH=/ramdisk/usr/sbin:/ramdisk/usr/bin:/$PATH
fi

echo "mindi will be installed under $local"

echo "Creating target directories ..."
mkdir -p $local/lib/mindi
mkdir -p $local/share/doc/mindi
mkdir -p $local/share/man/man8
mkdir -p $local/sbin
mkdir -p $conf

echo "Copying files ..."
cp deplist.txt $conf
cp -af rootfs aux-tools isolinux.cfg msg-txt sys-disk.raw.gz isolinux-H.cfg parted2fdisk.pl syslinux.cfg syslinux-H.cfg dev.tgz Mindi $local/lib/mindi
chmod 755 $local/lib/mindi/rootfs/bin/*
chmod 755 $local/lib/mindi/rootfs/sbin/*
chmod 755 $local/lib/mindi/aux-tools/sbin/*

cp -af analyze-my-lvm parted2fdisk.pl $local/sbin
if [ "$RPMBUILDMINDI" = "true" ]; then
	sed -e "s~^MINDI_PREFIX=XXX~MINDI_PREFIX=/usr~" -e "s~^MINDI_CONF=YYY~MINDI_CONF=/etc/mindi~" mindi > $local/sbin/mindi
else
	sed -e "s~^MINDI_PREFIX=XXX~MINDI_PREFIX=$local~" -e "s~^MINDI_CONF=YYY~MINDI_CONF=$conf~" mindi > $local/sbin/mindi
fi
chmod 755 $local/sbin/mindi
chmod 755 $local/sbin/analyze-my-lvm
chmod 755 $local/sbin/parted2fdisk.pl

cp -a mindi.8 $local/share/man/man8
cp -a CHANGES COPYING README README.busybox README.ia64 README.pxe TODO INSTALL $local/share/doc/mindi

echo $PATH | grep $local/sbin > /dev/null || echo "Warning - your PATH environmental variable is BROKEN. Please add $local/sbin to your PATH."

echo "Extracting symlinks ..."
( cd $local/lib/mindi/rootfs && tar -xzf symlinks.tgz )

ARCH=`/bin/arch`
echo $ARCH | grep -x "i[0-9]86" &> /dev/null && ARCH=i386
export ARCH

if [ -f $local/lib/mindi/rootfs/bin/busybox-$ARCH ]; then
		echo "Installing busybox ..."
		mv $local/lib/mindi/rootfs/bin/busybox-$ARCH $local/lib/mindi/rootfs/bin/busybox
else
		echo "WARNING: no busybox found, mindi will not work on this arch ($ARCH)"
fi

if [ "$ARCH" = "i386" ] ; then
	if [ -f $local/lib/mindi/rootfs/bin/busybox-$ARCH.net ]; then
		echo "Installing busybox.net ..."
		mv $local/lib/mindi/rootfs/bin/busybox-$ARCH.net $local/lib/mindi/rootfs/bin/busybox.net
	else
		echo "WARNING: no busybox.net found, mindi will not work on this arch ($ARCH) with network"
	fi
fi

if [ "$ARCH" = "ia64" ] ; then
	make -f Makefile.parted2fdisk DEST=$local/lib/mindi install
	if [ -f $local/lib/mindi/rootfs/sbin/parted2fdisk-$ARCH ]; then
		echo "Installing parted2fdisk ..."
		mv $local/lib/mindi/rootfs/sbin/parted2fdisk-$ARCH $local/lib/mindi/rootfs/sbin/parted2fdisk
	else
		echo "WARNING: no parted2fdisk found, mindi will not work on this arch ($ARCH)"
	fi
else
	echo "Symlinking fdisk to parted2fdisk"
	( cd $local/lib/mindi/rootfs/sbin && ln -sf fdisk parted2fdisk)
fi

exit 0
