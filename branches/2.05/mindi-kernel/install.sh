#!/bin/sh

if [ ! -f "install.sh" ] ; then
    echo "Please 'cd' to the directory you have just untarred." >> /dev/stderr
    exit 1
fi

if [ "_$PREFIX" != "_" ]; then
        local=$PREFIX/usr
        if [ -f /usr/local/sbin/mindi ]; then
                echo "WARNING: /usr/local/sbin/mindi exists. You should probably
 remove it !"
        fi
        echo $PATH | grep /usr/sbin > /dev/null || echo "Warning - your PATH env
ironmental variable is BROKEN. Please add /usr/sbin to your PATH."
else
        local=/usr/local
        if [ -f /usr/sbin/mindi ]; then
                echo "WARNING: /usr/sbin/mindi exists. You should probably remov
e the mindi package !"
        fi
        echo $PATH | grep $local/sbin > /dev/null || echo "Warning - your PATH e
nvironmental variable is BROKEN. Please add $local/sbin to your PATH."

fi

echo "mindi-failsafe-kernel will be installed under $local"

echo "Creating target directories ..."
mkdir -p $local/lib/mindi

echo "Copying files ..."
cp -a lib.tar.bz2 vmlinuz $local/lib/mindi/
