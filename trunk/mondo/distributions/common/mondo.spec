#
# $Id$
#
%define is_mandriva %(test -e /etc/mandriva-release && echo 1 || echo 0)
%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)
%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define is_redhat %(test -e /etc/redhat-release && echo 1 || echo 0)

%define name	mondo
%define version	VVV
%define mrel	RRR
# if mandriva official build (rpm --with is_official)
%{?is_official:%define rel %{mkrel} %{mrel}}%{!?is_official:%define rel %{mrel}}
%define	src		%{name}-%{version}.tgz
%define grp		Archiving/Backup
%define addreqb	mindi >= 1.05, bzip2 >= 0.9, afio, mkisofs, binutils, newt >= 0.50, slang >= 1.4.1

%if %is_redhat
%define	grp		Applications/Archiving
%define addreq	%{addreqb}
Autoreq:		0
%endif

%if %is_mandrake
%define src		%{name}-%{version}.tar.bz2
%define addreq	%{addreqb}
Autoreqprov:	no
%endif

%if %is_mandriva
%define src		%{name}-%{version}.tar.bz2
%define addreq	%{addreqb}
Autoreqprov:	no
%endif

%if %is_suse
%define addreq	%{addreqb}
%endif

Summary:	A program which a Linux user can utilize to create a rescue/restore CD/tape
Summary(fr):	Un programme pour les utilisateurs de Linux pour crï¿½r un CD/tape de sauvegarde/restauration
Summary(it):	Un programma per utenti Linux per creare un CD/tape di rescue
Summary(sp):	Un programa para los usuarios de Linux por crear una CD/cinta de restoracion/rescate

Name:		%{name}
Version:	%{version}
Release:	%{mrel}
License:	GPL
Group:		%{grp}
Url:		http://mondorescue.berlios.de
Source:		%{src}
BuildRoot:	%{_tmppath}/%{name}-%{version}
BuildRequires:	newt-devel >= 0.50, slang-devel >= 1.4.1, gcc
%ifarch ia64
Requires:	%{addreq}, elilo, parted
%else
Requires:	%{addreq}, syslinux >= 1.52
%endif

#%package xmondo
#Summary:	A QT based graphical front end for %{name}
#Group:		Applications/Archiving
#Requires:	%{name} = %{version}-${release}, qt, kdelibs
#
#%package %{name}-devel
#Summary:	Header files for building against Mondo
#%if %is_mandrake
#Group:		Development/Libraries
#%endif
#if %is_redhat
#Group:		Development/Other
#%endif
#Provides: libmondo-devel mondo-devel
#Obsoletes: libmondo-devel
#
#%description -n %{name}-devel
#Mondo libraries and includes

%description
Mondo is a GPL disaster recovery solution to create backup media (CD, DVD, tape, network images) that can be used to redeploy the damaged system, as well as deploy similar or less similar systems.

%description -l fr
Objectif
""""""""
Mondo est une solution GPL de sauvegarde en cas de désastre pour créer des médias (CD, DVD, bande, images réseau) qui peuvent être utilisés pour redéployer le système endomangé, aussi bien que des systèmes similaires, ou moins similaires.

%description -l it
Scopo
"""""
Mondo e' un programma che permette a qualsiasi utente Linux 
di creare un cd di rescue/restore (o piu' cd qualora l'installazione 
dovesse occupare piu' di 2Gb circa). Funziona con gli azionamenti di
nastro, ed il NFS, anche.

%description -l sp
Objectivo
"""""""""
Mondo es un programa que permite cualquier usuario de Linux a crear una CD
de restoracion/rescate (o CDs, si su instalacion es >2GO aprox.).  Funciona 
con cintas y NFS, tambien.

#%description xmondo
#Xmondo is a QT based graphical frontend to mondoarchive.  It can help you 
#set up a backup by following onscreen prompts.

%prep
%setup -q -n %name-%{version}

%build
%configure %{!?_without_xmondo:--with-x11}
%{__make} VERSION=%{version} CFLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT"

%install
%{__rm} -rf $RPM_BUILD_ROOT
%makeinstall
%if %is_suse
%{__rm} -rf $RPM_BUILD_ROOT/%{_datadir}/doc/%name-%{version}
%endif

%post
/sbin/ldconfig
%{__chmod} 755 %{_libdir}/%{name}/restore-scripts/%{name}/*

%postun
/sbin/ldconfig

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc ChangeLog svn.log
%doc INSTALL COPYING README TODO AUTHORS NEWS

%attr(755,root,root) %{_sbindir}/*
#%attr(755,root,root) %{_libdir}/%{name}/restore-scripts/%{name}/*
%{_libdir}
%{_mandir}

#%{!?_without_xmondo:%files xmondo}
#%{!?_without_xmondo:%{_sbindir}/xmondo}
#%{!?_without_xmondo:%{_libdir}/libXmondo-%{libversion}.so}
#%{!?_without_xmondo:%{_libdir}/libXmondo.so}
#%{!?_without_xmondo:%{_datadir}/mondo/mondo.png}

%changelog
* Fri Nov 05 2005 Bruno Cornec <bcornec@users.berlios.de> 1.05
- ia64 is now working
- -p option related bug fixed
- use libdir instead of datadir

* Tue Sep 06 2005 Bruno Cornec <bcornec@users.berlios.de> 2.04_berlios
- Merge of patches mentionned on mondo ML + ia64 updates
- Add -p option

* Tue May 03 2005 Hugo Rabson <hugorabson@msn.com> 2.04_cvs_20050503
- made mondo more clever about finding its home. Avoids mondo considering
  directories like '/usr/share/doc/momdo' as its home.

* Wed Aug 04 2004 Hugo Rabson <hugorabson@msn.com> 2.03
- test sanity of user-specified tempdir
- better SLES8 support

* Mon Jun 28 2004 Hugo Rabson <hugorabson@msn.com> 2.02
- instead of using 'dd' to erase partition table, delete existing
  partitions w/ the same call to fdisk that is used to create the
  new partitions; this should avoids locking up the partition table
- set bootable partition in the above same call to fdisk, for
  the same reason (avoids locking up the partition table)
- better software RAID support
- mount ext3 partitions as ext2 when restoring - better for Debian
- better star, ACL support
- added ACL, xattr support for afio users

* Fri Jun 26 2004 Hugo Rabson <hugorabson@msn.com> 2.01
- fixed cvs for SuSE systems
- fixed NTFS backup/restore bug relating to partimagehack
  log file overflow and NTFS v non-NTFS differentiation
- more reliable extraction of config info from CDs, floppies
- better support of ISO dirs at restore-time (Conor Daly)
- fixed spec file for SuSE users
- added ldconfig to install section

* Fri Jun 19 2004 Hugo Rabson <hugorabson@msn.com> 2.00
- first 2.0 release
- updated grub-install.patched to support SuSE and Red Hat 
- call 'mt' to set block size to 32K before opening in/out tape
- updated mondo-prep.c to create each disk's partitions all at once
  (one call per drive) instead of one call to fdisk per partition
- when extracting cfg file and mountlist from all.tar.gz (tape copy),
  use block size of INTERNAL_TAPE_BLK_SIZE, not TAPE_BLOCK_SIZE
- added star and rudimentary SELinux support
- fixed lots of bugs
- all logging now goes to /var/log/mondo-archive.log, with symlink
  to /tmp/mondo-restore.log for restore-time log-tracking
- added grub-install.patched
- removed embleer & other binaries
- added '-b' to specify block size
- added '-R' for star support

* Thu Mar 25 2004 Bruno Cornec <Bruno.Cornec@hp.com> 1.7_cvs-20040325
- ia64 fixes

* Fri Nov 07 2003 Joshua Oreman <oremanj@get-linux.org> 1.7_cvs-20031107
- fixed symbolic links for libraries
- added support for boot/root multi floppies

- added kdelibs as xmondo dependency
- added xmondo pixmap installation
- better find_cdrom_device(), to cope w/ multiple CD writers
- fixed -m and -Vc flags
- fixed NTFS support!
- bootable CD uses native, not El Torito, support now
- removed mondo-makefilelist
- added 2.6 kernel support
- if 2.6 kernel, insist that the user specify CD device
- drop Embleer; insist on ms-sys and parted if Windows partition

* Wed Nov 05 2003 Jesse Keating <jkeating@j2solutions.net> 1.7_cvs-20031105.1
- added -devel package

* Tue Nov 04 2003 Jesse Keating <jkeating@j2solutions.net> 1.7_cvs-20031104.1
- made xmondo a second package
- added ability to specify --without xmondo at build time

* Sun Nov 02 2003 Jesse Keating <jkeating@j2solutions.net> 1.7_cvs-20031102.1
- Clean up, added spanish translation
- Set prefix to be /usr
- added/fixed Requires
- remove CVS directories prior to building

* Thu Oct 23 2003 Hugo Rabson <hugorabson@msn.com> 1.75_cvs_20031023
- nothing yet

* Wed Oct 22 2003 Hugo Rabson <hugorabson@msn.com> 1.75
- fixed chmod/chown bug (Jens Richter)
- ask user to confirm NFS mountpoint in Interactive Mode
- rewritten format_everything() to make sure LVMs, RAIDs and
  regular partitions are prepped in the correct order
- better magicdev support
- rewritten external binary caller subroutine
- DVD support added
- better backup-time control gui; offer to exclude nfs if appl.
- fixed multi-tape support
- re-implemented -D and -J
- fixed bug in extract_config_file_from_ramdisk() which
  affected tape+floppy users
- updated is_incoming_block_valid() to make it
  return end-of-tape if >300 flotsam blocks
- unmount CD-ROM before burning (necessary for RH8/9)
- fixed some stray assert()'s
- fixed bug in grub-MR (Christian)
- make user remove floppy/CD before restoring interactively from tape
- fixed bug in am_I_in_disaster_recovery_mode()
- added code to nuke_mode() to make sure NFS
  (backup) share is mounted in Nuke Mode
- improved tape device detection code
- better GRUB support
- better logging of changed bigfiles at compare-time
- better NTFS support, thanks to partimagehack-static
- better logging
- rewrote tape-handling code, breaking compatibility w/ previous versions
- fixed ISO/CD biggiefile verification bug in mondoarchive
- fixed bug which stopped boot/compare-time changelist from popping up
- replaced mondo-makefilelist with C code - faster, cleaner
- tweaked GUI - better feedback

* Wed May 28 2003 Anonymous <root@rohan> 1.74
- misc fixes (Michael Hanscho's friend)
- added rudimentary support for SME
- added better label support
- fixed biggietime atime/ctime restoration bug 73
- fixed 'default boot loader' detection bug (Joshua Oreman)
- use single-threaded make_afioballs_and_images() if FreeBSD
- fixed mondoarchive -Vi multi-CD verify bug (Tom Mortell)
- superior get_phys_size_of_drive() (Joshua Oreman)
- fixed RAID-related bug in where_is_root_mounted()
- ISO tweaks
- fixed silly bug in load_filelist() which stopped
  funny German filenames from being handled properly
- fixed various calls to popup_and_get_string()
- fixed spec file
- reject -E /
- added partimagehack to the mix

* Tue May 20 2003 Anonymous <root@rohan> 1.73
- mark devices as bootable _after_ unmounting them
- resolve boot device (-f) if softlink
- post_param_configuration() --- store iso-dev and isodir
- added post-nuke-sample.tgz to package
- Nuke Mode now checks mountlist against hardware; offer user
  opportunity to edit mountlist if insane; if user declines, abort
- added lots of assert()'s and other checks
- ran code thru Valgrind to catch & fix some memory leaks
- made mondo-restore.c smaller by moving some subroutines to
  common/libmondo-raid.c and mondorestore/mondo-rstr-compare.c
- added '-Q' to let user test mondoarchive's ability to find
  their boot loader and type
- improved which_boot_loader()
- when burning or comparing to a CD, defeat autorun if it is
  running, to avoid confusing mondoarchive and the user
- if original backup media no longer available at boot-time
  then offer user chance to choose another media source
- when booting, type 'nuke noresize' to nuke w/o resizing
  mountlist to fill your drives
- add 'textonly' when booting, to avoid using Newt gui
- run nice(20) to prioritize mondoarchive at start
- don't pause and wait for next blank CD at backup-time
  unless necessary (e.g. previous CD has not been removed)
- get_phys_size_of_drive() --- better support of older drives
- don't eject if "donteject" is in kernel's command line
- cleaned up segfault-handling
- added Conor's strip_path() to improve file list display
- added Herman Kuster's multi-level bkp patch
- better boot-time screen/message
- added Joshua Oreman's FreeBSD patches x3
- fixed interactive/textonly support
- fixed support for subdir-within-NFS-mount
- fixed "Can't backup if ramdisk not mounted" bug
- try to work around eccentricities of multi-CD drive PCs
- misc clean-ups (Steve Hindle)

* Tue Apr 08 2003 Hugo Rabson <hugorabson@msn.com> 1.72
- LVM/RAID bugs fixed (Brian Borgeson)
- major clean-up of code (Stan Benoit)
- make-me-bootable fix (Juraj Ziegler)
- fixed problem w/ multi-ISO verify cycle (Tom Mortell)
- removed duplicate entry from makefile
- if root is /dev/root then assume not a ramdisk
- reject relative paths if -d flag (Alessandro Polverini)
- fixed potentially infinite loop in log_to_screen (Tom Mortell)
- add '/' to custom filelist as workaround for obscure bug
- ask user speed of CDRW if writing to CD
- find_cdrom_device() --- if nonexistent/not found then
  make sure to return '' as dev str and 1 as res
- tweaked restore scripts tgz
- cleaned up find_cdrom_device()
- if user creates /usr/share/mondo/payload.tgz then untar
  payload to CD at backup-time
- fixed insist_on_this_cd_number()
- fixed am_i_in_disaster_recovery_mode()
- misc clean-up (Tom Mortell)
- made code more legible
- fixed post-nuke support
- added -e support
- fixed nfs support
- fixed iso support
- at restore-time, only sort mountlist internally, 
  in mount_all_devices() and unmount_all_devices() 
- fixed cosmetic bug in label-partitions-as-necessary
- updated documentation
- fixed fstab-hacking scripts

* Wed Feb 12 2003 Hugo Rabson <hugorabson@msn.com> 1.71
- log newt, slang, ncurses info
- updated man page
- handle %% chars in issue.net properly (Heiko Schlittermann)
- fixed serious NFS restore bug
- cleaned up spec file; it should cause fewer problems now (Jesse Keating)
- changed various strcpy() calls to strncpy() calls
- added mondo-makefilelist to makefile (Mikael Hultgren)
- mount_cdrom() better at handling multiple CD drives
- exclude /media/cdrom,cdrecorder,floppy
- sensibly_set_tmpdir_and_scratchdir() --- exclude smb and smbfs
- better logging by eval_call_to_make_ISO()
- accept -J <fname> to let user provide their own fs catalog
  instead of -I <paths> to backup
- if dir excluded with -E or included with -I and dir is actually
  a softlink then exclude/include the dir pointed to, as well
- better location for manpage
- adjusted block size of tarball at start of tape, to help
  users w/ broken tape driver firmware
- sort -u fstab after modifying it
- if backup type is nfs then don't estimate noof media
- fixed Makefile (Mikael Hultgren)
- updated manpage

* Mon Dec 07 2002 Hugo Rabson <hugo@firstlinux.net> 1.70
- new devel branch opened

* Mon Dec 02 2002 Hugo Rabson <hugo@firstlinux.net> 1.52
- fixed bug in multithreading
- use new grub-MR instead of grub-install
- wipe only the partition table (not the MBR) when partitioning drives
- ignore lilo.conf.anaconda when looking for lilo.conf file
- accepts '-l RAW' to backup/restore original boot sector instead
  of running grub or lilo to init it after restoring
- fixed&updated stabgrub-me script; software RAID + GRUB work now
- mount/unmount /boot partition for Gentoo 1.2 users
- re-enabled extra tape checksums
- disabled spurious warnings
- unmount/remount supermounts at start/end of live restore, if nec.
- cleaned up mondo's tape block handling (now, TAPE_BLOCK_SIZE=128K
  and I've added INTERNAL_TAPE_BLK_SIZE=32K variable for buffering)
- added Makefile
- added -l RAW, to backup and restore original MBR
- cleaned up iso_mode() and nfs restoring
- create /mnt/RESTORING/mnt/.boot.d for Gentoo users
- made mondorestore CD bootable for ArkLinux users
- if user runs as 'su' not 'su -' then work around

* Sun Nov 17 2002 Hugo Rabson <hugo@firstlinux.net> 1.51
- pop-up list of changed files, at end of verification phase
- better handling of changed.files list at restore-time
- lots of CD-related fixes
- added '-N' flag --- to let user exclude all NFS-related mounts&devices
- better handling of 'kill'
- restructuring of code to ease integration of mondo w/XMondo
- fixed obscure bug in find_and_mount_actual_cd()
- if / or /root has <50MB free then abort & complain
- fixed install.sh
- fixed .spec file
- updated documentation
- commented code
- updated man page
- added -v / --version flag
- replace convoluted grep with wc (KP)
- fixed bug affecting restoration of bigfiles from CD's created w/0 compression
- fixed BurnProof-related bug
- better at figuring out which is the best partition for temp/scratchdir
- added do-not-compress-these (text file) to RPM
- do not compress files of types listed in do-not-compress-these
- dropped -U from call to afio - saves 20-30% runtime (Cosgrove)
- added Cosgrove's do-not-compress-these list
- included various patches from KP
- chmod tmpdir, scratchdir to 700 before using
- restore from specified backup device, even if its own cfg file disagrees
- fixed multi-tape bug
- fixed "Can't find first ISO when verifying nonbootable ISO" bug
- multithreaded make_afioballs_and_images()
- tmpdir and scratchdir are set sensibly whether mondoarchive is called with
  command-line parameters or not
- fixed bug in strip_spaces() which stopped it from handling
  small strings correctly - affected mountlist editor
- create a repaired copy of grub-install which is RAID-friendly;
  use it when initializing boot sector with run_grub()
- fixed bug in mondo-makefilelist

* Sun Sep 08 2002 Hugo Rabson <hugo@firstlinux.net> 1.50
- if restoring, don't try to find SCSI node of CD-ROM drive; find /dev entry
- during selective restore, skip filesets which don't contain relevant archives
- set /dev/null's perms to 777, just in case devfs-enabled kernel mangles it
- remove /var/run/*.pid after restoring
- move spurious lockfiles from /home/* to /home/*/.disabled
- ask user to confirm the tape/CD device name
- lots of multitape-related fixes
- added code to autodetect the hardware of the user, if possible
- if isodir does not exist then abort
- more sanity-checking for -d flag
- doubled 'biggiefile' threshold... to 32MB
- exclude /root/images/mindi
- fixed multi-imagedev bug (Emmanuel Druon)
- unmount/remount /mnt/floppy before/after backing up, if Mandrake
- restructured the source files
- fixed serious bug in line 1546 - should have been !=, not ==; stopped
  mondorestore from correctly restoring big files
- added '#include <signal.h>' to my-stuff.h
- exclude "incheckentry xwait()" from changed.files
- fixed minor bug in find_cdrom_device()
- fixed bug in friendly_sizestr...
- insist on tape #1 when start verifying
- added internal buffering, replacing the external 'buffer' exe
- if differential backup then don't permit formatting or fdisking,
  whether Interactive or Nuke mode
- if mondorestore is run on live filesystem (or from ramdisk) without
  parameters then mondorestore will ask which backup media (tape, CD, etc.)
  was used; it will read the config file from the media and proceed from there
- if tape streamer is softlink then resolve it first
- incorporate post-nuke tarball
- if user doesn't specify tape size, proceed anyway; behave intelligently
  in the event of end-of-tape
- prefix bkpinfo->restore_path to biggiefile fname before generating
  checksum & comparing to archived biggiefile
- if /etc/lilo.conf not found not /etc/lilo.conf.anaconda found
  then create a softlink from the former to the latter, to work
  around RH7.3's b0rken LILO support
- LFS support (mharris, michele, hugo)
- fixed verify bug --- CD#1 was being verified again & again & ...
- differential mode fixed; supported again
- ask user for boot loader + device if not detectible
- list up to 512 files in file selection window at once (was 128)
- better handling of bigfiles' checksums, perms and owns
- delete final filelist if <=2 bytes long
- if kernel not found and mondo in graphics mode then popup and ask
  for kernel path+filename

* Sun Jul 14 2002 Hugo Rabson <hugo@firstlinux.net> 1.45-1
- 1.5x branch forked off from 1.4x branch




