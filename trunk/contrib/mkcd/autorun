#!/bin/sh
#
# Autorun script for mondo installer CD
# - Hugo Rabson, 2003/04/28
#
# Based on autorun script for the Ark Development Suite
# Copyright (c) 2002 Bernhard Rosenkraenzer <bero@arklinux.org>
#




InBkgd() {
   cd /
   sleep 1
   umount $1 &> /dev/null
   eject $2 &> /dev/null
}


run_command_as_root() {
    local command fnam subcom

    subcom="$1"
    command="xterm -e $subcom"
    if which kdesux &> /dev/null ; then
        kdesu +sb -c "$command"
    else
        fnam=/tmp/$$.$RANDOM.sh
        echo -en "echo -en \"\
To install Mondo, you need to be root.\n\
Please enter root's password now to proceed,\n\
or press <Ctrl>-C to abort installation.\n\"\n\
su - -c $subcom\n" > $fnam
        chmod +x $fnam
        xterm -e "$fnam"
        rm -f $fnam
    fi
}


# ------------------------- main ------------------------- 

if [ -d "`dirname $0`" ]; then
    cd "`dirname $0`"
fi

q=`pwd`
r=`mount | grep $q | tr -s ' ' '\t' | cut -f1`

ps ax | grep mondoarchive | grep -v grep && exit 0
ps ax | grep mondorestore | grep -v grep && exit 0

mr=/setup
#mr=`which mondorestore`
#if [ "$mr" ] && [ -x "$mr" ] ; then
#    kdesu +sb -c "xterm -e $mr"
#else
#fi

subcom=`pwd`$mr
run_command_as_root "$subcom"
InBkgd $q $r &
exit $?
