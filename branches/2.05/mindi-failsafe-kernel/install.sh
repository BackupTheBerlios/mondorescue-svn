#!/bin/sh

if [ ! -f "install.sh" ] ; then
    echo "Please 'cd' to the directory you have just untarred." >> /dev/stderr
    exit 1
fi
local=/usr/local
mkdir -p $local/share/mindi
mkdir -p $local/sbin

cp --parents -pRdf * $local/share/mindi/
exit 0
