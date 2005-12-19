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

echo "mindi will be installed under $local"

echo "Creating target directories ..."
install -g root -o root -m 755 -d $conf $local/lib/mindi $local/share/man/man8 $local/sbin $local/doc/mindi

echo "Copying files ..."
install -g root -o root -m 644 isolinux.cfg msg-txt sys-disk.raw.gz isolinux-H.cfg syslinux.cfg syslinux-H.cfg dev.tgz $local/lib/mindi
install -g root -o root -m 644 deplist.txt $conf

cp -a rootfs aux-tools Mindi $local/lib/mindi
chmod 755 $local/lib/mindi/rootfs/bin/*
chmod 755 $local/lib/mindi/rootfs/sbin/*
chmod 755 $local/lib/mindi/aux-tools/sbin/*
chown -R root:root $local/lib/mindi

if [ "$RPMBUILDMINDI" = "true" ]; then
	sed -e "s~^MINDI_PREFIX=XXX~MINDI_PREFIX=/usr~" -e "s~^MINDI_CONF=YYY~MINDI_CONF=/etc/mindi~" mindi > $local/sbin/mindi
else
	sed -e "s~^MINDI_PREFIX=XXX~MINDI_PREFIX=$local~" -e "s~^MINDI_CONF=YYY~MINDI_CONF=$conf~" mindi > $local/sbin/mindi
fi
chmod 755 $local/sbin/mindi
chown root:root $local/sbin/mindi
install -g root -o root -m 755 analyze-my-lvm parted2fdisk.pl $local/sbin

install -g root -o root -m 644 mindi.8 $local/share/man/man8
install -g root -o root -m 644 CHANGES COPYING README README.busybox README.ia64 README.pxe TODO INSTALL $local/doc/mindi

ARCH=`/bin/arch`
echo $ARCH | grep -x "i[0-9]86" &> /dev/null && ARCH=i386
# For the moment, we don't build specific x86_64 busybox binaries
echo $ARCH | grep -x "x86_64" &> /dev/null && ARCH=i386
export ARCH

# Managing busybox
if [ -f $local/lib/mindi/rootfs/bin/busybox-$ARCH ]; then
		echo "Installing busybox ..."
		install -s -g root -o root -m 755 $local/lib/mindi/rootfs/bin/busybox-$ARCH $local/lib/mindi/rootfs/bin/busybox
else
		echo "WARNING: no busybox found, mindi will not work on this arch ($ARCH)"
fi
if [ "$ARCH" = "i386" ] ; then
	if [ -f $local/lib/mindi/rootfs/bin/busybox-$ARCH.net ]; then
		echo "Installing busybox.net ..."
		install -s -g root -o root -m 755 $local/lib/mindi/rootfs/bin/busybox-$ARCH.net $local/lib/mindi/rootfs/bin/busybox.net
	else
		echo "WARNING: no busybox.net found, mindi will not work on this arch ($ARCH) with network"
	fi
fi
# Remove left busybox
rm -f $local/lib/mindi/rootfs/bin/busybox-*

# Managing parted2fdisk
if [ "$ARCH" = "ia64" ] ; then
	(cd $local/sbin && ln -sf parted2fdisk.pl parted2fdisk)
	make -f Makefile.parted2fdisk DEST=$local/lib/mindi install
	if [ -f $local/lib/mindi/rootfs/sbin/parted2fdisk-$ARCH ]; then
		echo "Installing parted2fdisk ..."
		install -s -g root -o root -m 755 $local/lib/mindi/rootfs/sbin/parted2fdisk-$ARCH $local/lib/mindi/rootfs/sbin/parted2fdisk
		install -s -g root -o root -m 755 $local/lib/mindi/rootfs/sbin/parted2fdisk-$ARCH $local/sbin/parted2fdisk
	else
		echo "WARNING: no parted2fdisk found, mindi will not work on this arch ($ARCH)"
	fi
else
	# FHS requires fdisk under /sbin
	(cd $local/sbin && ln -sf /sbin/fdisk parted2fdisk)
	echo "Symlinking fdisk to parted2fdisk"
	( cd $local/lib/mindi/rootfs/sbin && ln -sf fdisk parted2fdisk)
fi
# Remove left parted2fdisk
rm -f $local/lib/mindi/rootfs/sbin/parted2fdisk-*

exit 0
