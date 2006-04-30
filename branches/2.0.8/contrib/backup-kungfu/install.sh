#!/bin/bash

# Install the docs
mkdir -p /usr/share/doc/backup-kungfu-0.1.2
cp ./README COPYRIGHT CHANGELOG /usr/share/doc/backup-kungfu-0.1.2/
chmod 444  /usr/share/doc/backup-kungfu-0.1.2/*

# Install the example conf and the executable
cp ./backup-kungfu.conf /etc/
cp ./backup-kungfu /usr/local/bin
chmod 644 /etc/backup-kungfu.conf
chmod 744 /usr/local/bin/backup-kungfu

echo "Everything installed. Please read /usr/share/doc/backup-kungfu-0.1.2/README before you run the script"
