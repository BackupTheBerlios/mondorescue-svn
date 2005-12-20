#
# $Id$
#
%define is_mandriva %(test -e /etc/mandriva-release && echo 1 || echo 0)
%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)
%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define is_redhat %(test -e /etc/redhat-release && echo 1 || echo 0)

%define name	mindi
%define version	VVV
%define mrel	1
%define docname		%{name}-%{version}
%define	src		%{name}-%{version}.tgz

%if %is_redhat
Group:			Applications/Archiving
Autoreq:	0
%endif

%if %is_mandrake
%define	src		%{name}-%{version}.tar.bz2
Group:			Archiving/Backup
Autoreqprov: no
%endif

%if %is_mandriva
%define	src		%{name}-%{version}.tar.bz2
Group:			Archiving/Backup
Autoreqprov: no
%endif

%if %is_suse
Group:			Archiving/Backup
%define docname		%{name}
%endif

Summary:	Mindi creates emergency boot disks/CDs using your kernel, tools and modules
Name:		%name
Version:	%version
Release:	%mrel
License:	GPL
Group:		System/Kernel and hardware
Url:		http://mondorescue.berlios.de
Source:		%{src}
BuildRoot:	%{_tmppath}/%{name}-%{version}
Requires:	bzip2 >= 0.9, mkisofs, ncurses, binutils, gawk, dosfstools, afio, which
# Not on all systems
#Conflicts:	bonnie++

%description
Mindi takes your kernel, modules, tools and libraries, and puts them on N
bootable disks (or 1 bootable CD image). You may then boot from the disks/CD
and do system maintenance - e.g. format partitions, backup/restore data,
verify packages, etc.

%prep
%{__rm}  -rf $RPM_BUILD_ROOT
%setup -n %name-%{version}

%build
%ifarch ia64
%{__make} -f Makefile.parted2fdisk clean
%{__make} -f Makefile.parted2fdisk
%endif

%install
export DONT_RELINK=1

%{__rm} -rf $RPM_BUILD_ROOT
export PREFIX=${RPM_BUILD_ROOT}%{_exec_prefix}
export CONFDIR=${RPM_BUILD_ROOT}%{_sysconfdir}
export RPMBUILDMINDI="true"
export DOCDIR=${RPM_BUILD_ROOT}${RPM_DOC_DIR}/%{docname}

./install.sh

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%post
for i in %{_libdir}/mindi/aux-tools/sbin/* %{_libdir}/mindi/rootfs/bin/* %{_libdir}/mindi/rootfs/sbin/* ; do
	if [ ! -h $i ]; then
		%{__chmod} 755 $i
	fi
done
if [ -f /usr/local/sbin/mindi ]; then
	echo "WARNING: /usr/local/sbin/mindi exists. You should probably remove your manual mindi installation !"
fi

%files
%defattr(644,root,root,755)
%config(noreplace) %{_sysconfdir}/mindi/deplist.txt
%doc CHANGES INSTALL COPYING README TODO README.ia64 README.pxe README.busybox
%{_mandir}
%{_libdir}/mindi
%attr(755,root,root) %{_sbindir}/*

%changelog
* Fri Nov 05 2005 Bruno Cornec <bcornec@users.berlios.de> 1.05
- ia64 is now working
- use install.sh script
- use libdir for binary data

* Tue Sep 06 2005 Bruno Cornec <bcornec@users.berlios.de> 1.04_berlios
- Merge of patches mentionned on mondo ML + ia64 updates
- Fix bugs when called alone

* Tue May 03 2005 Hugo Rabson <hugorabson@msn.com> 1.04_cvs_20050503
- supports exec-shield

* Wed Aug 04 2004 Hugo Rabson <hugorabson@msn.com> 1.03
- better support of SLES 8

* Mon Jun 28 2004 Hugo Rabson <hugorabson@msn.com> 1.02
- better kernel-level logging
- added ACL, xattr binaries to deplist.txt
- fixed obscure bug which occasionally stopped mindi from correctly finding
  and documenting all LVM2 LVM-on-RAID volumes

* Fri Jun 25 2004 Hugo Rabson <hugorabson@msn.com> 1.01
- added ide_tape and other modules to mindi's config detection
- unmount errant ramdisk ($mtpt) if fail to create boot floppy
- better support of ISO dirs at restore-time (Conor Daly)

* Fri Jun 18 2004 Hugo Rabson <hugorabson@msn.com> 1.00
- first 1.0x release
- busybox now static
- trap Ctrl-Alt-Del; trigger soft reset
- better SuSE 9.1 support
- added parted2fdisk.pl (IA64) (Bruno Cornec)
- add LVM2 support for kernel 2.6 (Takeru Komoriya)
- better detection of multiple Mindis (Martin Fürstenau)
- don't complain if just a Mindi boot CD & not a platform for Mondo
- updated busybox to 1.0.0pre10
- removed uClibc
- add memtest support
- 2.6 kernel support
- removed Embleer files (Andree Leidenfrost)
- added kernel-only floppy support, to accommodate really big kernels
- updated+rebuilt busybox
- added star support
- mount /sys at boot-time
- better 64-bit and 2.6 kernel support
- better LVM, failsafe kernel support (Jim Richard)
- use LILO, not raw kernel, on 1.4MB boot floppy 
- record names of unsaved modules, for future reference
- enlarged ramdisk by 8MB

* Wed Oct 22 2003 Hugo Rabson <hugorabson@msn.com> 0.95
- changed some '==' to '=' --- now more RH6-friendly
- allow absolute pathnames again in deplist
- disable multifunc cd thing
- better Gentoo support (Bill)
- better OnStream support
- better Slackware support (Laurenz)
- added partimagehack-static to deplist.txt
- recompiled Busybox - now 10k smaller; better stack-handling
- fixed boot screen typo
- added support for 'auto' fs format
- better devfs support for Mandrake users
- better Debian+LVM support (Ralph Grewe)
- updated analyze-my-lvm to handle floating-point gigabyte -L values

* Thu Jul 24 2003 Anonymous <root@rohan> 0.94
- altered rootfs's /dev entry to stop cvs from becoming confused
- tweaked MAX_COMPRESSED_SIZE
- added multi-function CD support to mindi and sbin/post-init
- re-mount root as rw just in case
- ask user to remove last data (floppy) disk if nec. (Tom Mortell)
- added support for 5th column in mountlist.txt for labels
- added symlinks.tgz
- suppress erroneous error msgs re: failsafe kernel
 
* Sun May 18 2003 Hugo Rabson <hugorabson@msn.com> 0.93
- added cciss.o to SCSI_MODS
- if format type is (e.g.) ext3,ext2 then use 1st entry
- re-enabled fsck*
- if cciss in use the enable it at boot-time
- added /dev/ataraid/* to boot disk via ataraid.tgz (Luc S.)
- better ISO support
- tweaked Mindi to use 10-15% fewer floppies
- added RUN_AFTER_INITIAL_BOOT_PHASE to let user specify a command
  to be run by Mindi before it bootstraps to aux data disks;
  e.g. 'echo engage > /proc/scsi/something'
- added RUN_AFTER_BOOT_PHASE_COMPLETE to let user specify a command
  to be run by Mindi after it boots but before it runs mondorestore
- allow for Debian & other Stiefkinder that use 'none', not 'swap'
  as the mountpoint of the swap partition
- removed afio dependency
- re-worked install.sh and tarball not to use tgz's
- cleared up the boot msg
- updated busybox to 0.60.5; updated uClibc to 0.9.19
- add #!/bin/sh to start of insert-all-my-modules
- fixed obscure bug in install-additional-tools
- changed grep -m to grep | head -n1 for Debian users
- moved vmlinuz, lib.tar.bz2 to mindi-kernel tarball/rpm

* Wed Apr 09 2003 Hugo Rabson <hugorabson@msn.com> 0.92
- fixed LVM/RAID bugs (Brian Borgeson)
- if bad lilo, give more verbose error before quitting
- added mt and perl to deplist.txt
- insist on gawk being present
- insmod ide-cd, cdrom, isofs, just in case
- boot-time tmpfs ramdisk is now 40m (was 34m)
- cleaned up logs
- iso mode now calls Interactive
- make SizeOfPartition() more Debian-friendly (Andree Leidenfrost)
- clean up some calls to grep, esp. partition_mountpt=...
- corrected some bashisms, to suit Debian ped- er, users
- made first line refer to bash, not sh, to make sure 
  the Debian people know Mindi requires bash, not hs
- changed grep -v "#" and grep -vx "#.*" to grep -vx " *#.*"
- disabled code which would have made Mindi use sfdisk instead of fdisk
  if it looked as if Debian's fdisk would misbehave

* Wed Feb 12 2003 Hugo Rabson <hugorabson@msn.com> 0.91
- new devel branch
- EXTRA_SPACE=16384
- updated ResolveSoftlinks() to work better with b0rkn Gentoo devfs /dev
- patched analyze-my-lvm (Benjamin Mampaey)
- detect built-in boot.b files in lilo
- cleaned up spec file a bit, more use of macros (Jesse Keating)
- return w/err, don't abort, if Matt Nelson's RH8 system is farked
- better at finding isolinux.bin
- better at analyzing dependencies when running on broken distributions
- try harder to boot from CD, even if tape fails
- added ADDITIONAL_BOOT_PARAMS to be sent to kernel
- better resolution of relative softlinks, leading to fewer
  duplicates on data disks and therefore fewer data disks
- detect Debian+devfs; use sfdisk instead of fdisk in that
  eventuality, to work around yet another of Debian's warts
- removed /lib/libuuid.* from rootfs.tgz
- fixed vi
- mindi now uses gawk --traditional (making gawk behave in a functionally
  identical way to awk) - PASS; great, so now Debian needs to fix its awk :)
- removed / from list of dirs accessed when trying to resolve deplist entry
- spinner bugfix; handle odd LABELs properly (Tom Mortell)
- fixed LVM/RAID bugs (Brian Borgeson)

* Mon Dec 02 2002 Hugo Rabson <hugo@firstlinux.net> 0.72_20021202
- misc code clean-up
- save boot device's boot sector
- detect and beware Compaq diagnostic partitions
- better handling of devfs V non-devfs kernels and boot devs
- trimmed busybox 0.60.3 binary on boot disk
- mindi now resides in /usr/local/bin
- removed lilo from dependencies
- more Gentoo-friendly

* Sun Nov 18 2002 Hugo Rabson <hugo@firstlinux.net> 0.71
- if your kernel's builddate doesn't match any of the kernels in 
  your /boot directory then fudge the issue & find the closest 
  match (*grumble* Debian)
- scan tape & CD at start, to force inclusion of modules on boot device
- double EXTRA_SPACE if >7 disks
- slimmer, more lithesome logfile
- now accepts --findkernel
- line 1982 - dd count=24000 should be count=$ramdisksize (Andras Korn)
- better handling of non-Linux partitions on DevFS-enabled kernels
- misc clean-ups
- fixed Debian/ramdisksize/bloat problem (Johannes Franken)
- fixed 2.4.20/fdisk eccentricity (Alistair Stevens)
- updated kernel to 2.4.18-mdk6
- generate mountlist in dev-abetical order ;)
- fixed minor bug in .spec file
- better at finding /boot/boot.b if your distro breaks the de facto standard
- fixed minor bug in MakeMountlist
- fixed install.sh
- cleaned up deplist.txt
- added rudimentary -v / --version flag
- line 1180 or so --- duplicate mindi.iso entries --- fixed
- added /bin/[ - a softlink to /bin/sh
- ListAllPartitions() - sanity fix (KP)
- fixed .spec bug
- better feedback
- added Markus's RAID patch
- ListAllPartitions() - include /mnt/win* (Hugo)

* Sun Sep 08 2002 Hugo Rabson <hugo@firstlinux.net> 0.70
- better logging by dependency-calculating code
- better handling of dependencies, specifically softlinks
- if called by mondo then use mondo's temp dir as our temp dir too
- changed gawk to $AWK in a few places, to allow for Debian
- added host* config files to deplist
- added tftp, ifconfig to busybox
- added RPC support to uClibc
- cleaned up creation of Mondo-Mindi configuration file
- fixed bug in TryToFindKernelPath which stopped it from handling
  multiple, same-version kernels gracefully
- cleaned up deplist.txt
- receive 'DIFFERENTIAL' variable from Mondo
- better Debian compatibility, esp. w/detection of ver# (Hector Alvarez)
- better devfs support; call fgrep in places instead of grep (Andrew Korn)
- fixed analyze-my-lvm (Ralph Gruwe)
- moved 50K of stuff from rootfs.tgz to aux-tools.tgz
- added x11-tools.tgz option
- updated /dev/console and /dev/tty0 (Paul Stevens)
- cleaned up /mnt/groovy-stuff V /tmp/tmpfs code
- catch sigint, sighup, etc.
- fixed LILO-related message.txt mistake
- .spec file clean-up; automation; config file (Carl Wilhem Soderstrom)
- better devfs support (Andrew Korn)

* Sun Jul 14 2002 Hugo Rabson <hugo@firstlinux.net> 0.64-1
- fixed RH7.3 readonly bug
- improved nfs config file creation
- search more locations for isolinux.bin
- added isonuke option
- faster data disk creation
- better DevFS support (Hector Alvarez, DuckX)
- nfs-related fix (Hans Lie)
- abort if vfat filesystem present but mkfs.vfat missing
- removed softlink to pico
- allow Mondo to say no compression will be used
- added [ to ramdisk
- tar data disks with -b [block size] of 32k
- don't autoboot to 'RESTORE' screen - it's scary!
- let user choose lilo or syslinux as boot loader
- added syslinux support
- cleaned up message screens

