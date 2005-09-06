#!/bin/sh

if [ ! -f "mindi" ] ; then
    echo "Please 'cd' to the directory you have just untarred." >> /dev/stderr
    exit 1
fi
local=/usr
# local=/usr/local
if uname -a | grep Knoppix > /dev/null || [ -e "/ramdisk/usr" ] ; then
    local=/ramdisk/usr
    export PATH=/ramdisk/usr/sbin:/ramdisk/usr/bin:/$PATH
fi

mkdir -p $local/share/mindi
mkdir -p $local/sbin

#for i in aux-tools dev rootfs ; do
#    [ -e "$i.tgz" ] && continue
#    cd $i
#    tar -c * | gzip -9 > ../$i.tgz
#    cd ..
#    rm -Rf $i
#done

cp --parents -pRdf * $local/share/mindi/
ln -sf $local/share/mindi/mindi $local/sbin/
chmod +x $local/sbin/mindi
echo $PATH | grep $local/sbin > /dev/null || echo "Warning - your PATH environmental variable is BROKEN. Please add $local/sbin to your PATH."
( cd $local/share/mindi/rootfs && tar -xzf symlinks.tgz )
ARCH=`/bin/arch`
echo $ARCH | grep -x "i[0-9]86" &> /dev/null && ARCH=i386
export ARCH
( cd $local/share/mindi/rootfs && mv bin/busybox-$ARCH bin/busybox)
if [ "$ARCH" = "i386" ] ; then
	( cd $local/share/mindi/rootfs && mv bin/busybox-$ARCH.net bin/busybox.net)
fi
if [ "$ARCH" = "ia64" ] ; then
	make -f Makefile.parted2fdisk
	make -f Makefile.parted2fdisk install
	( cd $local/share/mindi/rootfs && mv sbin/parted2fdisk-ia64 sbin/parted2fdisk)
else
	( cd $local/share/mindi/rootfs/sbin && ln -sf fdisk parted2fdisk)
fi
ls /etc/mindi/* > /dev/null 2>/dev/null
[ "$?" -ne "0" ] && rm -Rf /etc/mindi
exit 0
