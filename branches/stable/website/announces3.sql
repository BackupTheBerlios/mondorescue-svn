SQLite format 3   @                                                                             � �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              r�3tableannouncesannouncesCREATE TABLE announces (id INTEGER PRIMARY KEY, date DATE, announce VARCHAR[65535])   �    �����                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       	      	       p>Changes are:<br>v2.05 (Bruno Cornec <bcornec_at_users.berlios.de)<br>- -p options works better for NFS cases<br>- ia64 is now working for rhel3<br>- delivery process to BerliOS improved<br>- Add -p option to generate ISO images file names with prefix. The new<br>default name for ISO images is mondorescue-1.iso, ... For PXE environment,<br>you have to use the prefix option on the command line (read README.pxe)<br>- Mandrake 2005 support<br>- NFS patches (Yann Aubert <technique_at_alixen.fr>)<br>- mondorestore shouldn't now ask final questions with -H<br>(this is an unattended mode)<br><br>1.05 (2005-11-19)<br>- Bug fix for ldd output incorrectly handled, leading to 'grep not found' error<br>(Andree Leidenfrost)<br>- NFS now works in interactive mode, and nolock problems are solve<br>(Andree Leidenfrost)<br>- IA-64 support is now working for rhel 3<br>(Bruno Cornec)<br>- add MINDI_CONF to the mindi LOGFILE<br>(Philippe De Muyter)<br>- Speed up fdisk'ing dev/ida raid devices<br>(Philippe De Muyter)<br></p>   ondo-2.06-266.rhel4.i386.rpm<br>
mandriva/2006.0/mindi-1.06-266.2006.0mdk.i586.rpm<br>
mandriva/2006.0/mondo-2.06-266.2006.0mdk.i586.rpm<br>
fedora/4/mindi-1.06-266.fc4.i386.rpm<br>
fedora/4/mondo-2.06-266.fc4.i386.rpm<br>
redhat/73/mindi-1.06-266.rh73.i386.rpm<br>
redhat/73/mondo-2.06-266.rh73.i386.rpm<br>
sles/9/mindi-1.06-266.sles9.i386.rpm<br>
sles/9/mondo-2.06-266.sles9.i386.rpm<br>
<br>

So more distros supported and built.<br>
I've used them for a restore of a failed server recently (PXE mode) so
they should be working correctly for most of the cases (including now
people not using RPMs)
</p><p>
I'll off till begining of next year, so I wish you all the best for this
new year 2006 coming, and hope you'll find mondorescue as useful as
usual !
</p><p>
Changelogs are following:

</p><p>
MONDO CHANGES<br>
<br>
v2.06 (2005-12-23)<br>
- better error handling of failed commands/mindi (Andree Leidenfrost)<br>
- fix compiler warnings (Andree Leidenfrost)<br>
- -p improvements for NFS/PXE/ISO modes (Bruno Co   rnec)<br>
- support of default route and netmask for PXE/NFS (Bruno Cornec)<br>
- fix for restoring mondo backups on md-raid systems (Philippe De<br>
Muyter)<br>
- remove excessive 'cat' commands (Philippe De Muyter)<br>
- fix to force growisofs to use speed=1 for DVD burning (Philippe De<br>
Muyter)<br>
- now handles cifs correctly (Bruno Cornec)<br>
- fix issue where mondoarchive ejects CD/DVD despite writing iso images<br>
(Andree Leidenfrost)<br>
- Add -P option to df calls (Andree Leidenfrost/Chuan-kai Lin)<br>
- fix usage of joint -B and -m options (Andree Leidenfrost/Efraim<br>
Feinstein)<br>
- Quadrupled ARBITRARY_MAXIMUM from 500 to 2000 for mondorestore's<br>
filebrowser (Andree Leidenfrost)<br>
- remove the renice of mondoarchive (Hugo Rabson)<br>
- relocate what was under /usr/share to /usr/lib (FHS compliance)<br>
(Bruno Cornec/Andree Leidenfrost)<br>
- manage non ambiguous delivery under /usr (packages) or /usr/local (tar<br>
ball) (Bruno Cornec)<br>
- disable x11 build by default (Bruno Cor   nec)<br>
- remove sbminst (Bruno Cornec/Andree Leidenfrost)<br>
- use parted2fdisk everywhere (Bruno Cornec)<br>
- exports MONDO_LIB (Bruno Cornec)<br>
- RPM build for fedora core 4, sles9, redhat 7.3, rhel 3/4, mandriva<br>
2006.0, mandrake 10.2/10.1 (Bruno Cornec/Gary Granger)<br>
- interactive mode now asks for image size and prefix in NFS mode<br>
(Gallig Renaud/Bruno Cornec)<br>
- iso-prefix should be read in iso mode even when -H not given (Stan<br>
Benoit)<br>
- VERSION/RELEASE Tag added (Bruno Cornec)<br>
- many code cleanup, small fixes, PXE/NFS code improvements<br>
(S�bastien Aperghis-Tramoni/Bruno Cornec)<br>
<br>
MINDI CHANGES<br>
<br>
1.06 (2005-12-23)<br>
- mindi manpage added (Andree Leidenfrost)<br>
- clean up remaining mount points, mindi.err at the end (Wolfgang<br>
Rosenauer)<br>
- fix bugs for SuSE distro around tar, tr and find arguments order<br>
(Wolfgang Rosenauer)<br>
- new busybox.net version used for better PXE support (Bruno Cornec)<br>
- USB keyboard support (Bruno Cornec)<br    >
- -p should now work with ISO/PXE/NFS modes (Bruno Cornec)<br>
- relocate what was under /usr/share to /usr/lib (FHS compliance)<br>
(Bruno Cornec/Andree Leidenfrost)<br>
- manage non ambiguous delivery under /usr (packages) or /usr/local (tar<br>
ball) (Bruno Cornec)<br>
- install script rewritten and used for RPM build, with new layout<br>
(S�bastien Aperghis-Tramoni/Bruno Cornec)<br>
- use parted2fdisk everywhere (Bruno Cornec)<br>
- use MONDO_LIB exported by mondoarchive instead of MONDO_HOME guessed<br>
(Bruno Cornec)<br>
- RPM build for fedora core 4, sles9, redhat 7.3, rhel 3/4, mandriva<br>
2006.0, mandrake 10.2/10.1 (Bruno Cornec/Gary Granger)<br>
- VERSION/RELEASE Tag added (Bruno Cornec)<br>
- VMPlayer support<br>
- Code cleanup, small fixes, PXE/NFS code improvements<br>
(Wolfgang Rosenauer/S�bastien Aperghis-Tramoni/Bruno Cornec<br>
- New switches for PXE mode (ping & ipconf, Cf README.pxe)<br>
(S�bastien Aperghis-Tramoni/Bruno Cornec)<br>
- mindi-kernel added to SVN (Bruno Cornec)<br>
</p>   S [S                                                                                                                                                                                                                                                                                                                                       �} !�i2005-11-22Following recent discussion here, it has been decided to increase version numbers of mondo and mindi to avoid confusion.</p><p>So I can now announce the availability of the latest mondo and mindi which can be downloadable from http://mondorescue.berlios.de</p><p>Look also there at the Wiki and the docs. Fill bugs and feature requests. We are working with Hugo on the migration of the rest of the content and the old Website, domain name in parallel, but it wil take some time to be finished.</p><   �" !�32005-09-28Creation of the Berlios SVN repository to take over the non-working one missing since more than one year. Integration of the first patches waiting.    Y �< Y                                                                           �` !�/2006-02-02Mondo Rescue rsync server available</p>
<p>In our always increasing set of services :-) I'd like to announce the
availability of an rsync mirror service for mondo-rescue.

Try rsync rsync.mondorescue.org::</p>�b !�32006-02-01Mondo Rescue ftp server available</p>
<p>Things are progressing. I've gathered all the p   
�? !�m2005-12-24I think that it's time to promote our revisions to a newer version, as a
proof of increased stability, and result of lots of cleanup and
improvements, even if some small known bugs remain (and other could
always appear). So here are mindi 1.06 / mondo 2.06 aka Christmas
Release :-)
</p><p>
Now available at ftp://ftp.mondorescue.org :<br>
<br>
mandrake/10.1/mindi-1.06-266.10.1mdk.i386.rpm<br>
mandrake/10.1/mondo-2.06-266.10.1mdk.i386.rpm<br>
rhel/3/mindi-1.06-266.rhel3.i386.rpm<br>
rhel/3/mondo-2.06-266.rhel3.i386.rpm<br>
rhel/4/mindi-1.06-266.rhel4.i386.rpm<br>
rhel/4/m       ackages that were originally on <a href="http://www.mondorescue.org">http://www.mondorescue.org</a> and also added Mike's SuSE RPMs
+ all the latest packages I've produced and put them on a new ftp server
accessible at <a href="ftp://ftp.mondorescue.org">ftp://ftp.mondorescue.org</a>
</p><p>
You may have issues to access to it yet, as I've done a DNS refresh this
night and propagation could not be made everywhere now.

FYI:

</p><p>
$ host ftp.mondorescue.org<br>
ftp.mondorescue.org has address 213.30.161.23
</p><p>

The tree is:
</p><p>
level one distribution<br>
level two version number<br>
level three the packages

</p><p>
Feel free to send me (by private mail) packages that you have and that
are not in the repository, so that I can add them.

</p><p>
Thanks to HP and especially the HP/Intel Solution Center to help me
hosting the project.

Next step will be the Web site.</p>
                                                                                                                                    sing qemu + more tools
to generate RPMs)
</p>

<p>
The focus on this version has been more around the new website and
documentation, and a bit less on code. So some persistant bugs are still
not corrected (some dvd burning issues and dm/lvm2 support mainly) and
will be hopefully in 2.0.8.
</p>

<p>
MONDO CHANGES
</p>

<p>
v2.07 (2006-02-23)<br>
- useless cat, sort|uniq commands removed (Bruno Cornec/S�bastien Aperghis-Tramoni)<br>
- Doc cleanup (Andree Leidenfrost)<br>
- Add the actual format to messages after calls to function is_this_a_valid_disk_format() about unsupported formats.  (Andree Leidenfrost)<br>
- Abort|Warn when one of the include|exclude dirs (-I|-E) does not exist (Bruno Cornec/Jeffs)<br>
- Replaced partimagehack with ntfsclone from ntfsprogs package. (Andree Leidenfrost)<br>
- use df -P everywhere (Bruno Cornec)<br>
- Paypal incitations removed (Andree Leidenfrost)<br>
- mondo now uses /usr/share for the restore-scripts (Bruno Cornec)<br>
- rpmlint cleanups (Bruno Cornec)<br>
- no shared    librairies and no X11 anymore (were useless) (Bruno Cornec)<br>
- files > 2GB are now really supported (Andree Leidenfrost)<br>
- new SGML based Mondo Rescue documentation + new Web site (Bruno Cornec/Andree Leidenfrost)<br>
- mondoarchive aborts when 'mindi --findkernel' gives a fatal error (See also Debian bug #352323.) (Andree Leidenfrost)<br>
- /tmp not excluded anymore from backup (Bruno Cornec)<br>
- New RPM Build environement (Bruno Cornec)<br>
</p>

<p>
MINDI CHANGES
</p>

<p>
1.07 (2006-02-23)<br>
- stop creating further size of floppy disks if the smaller one succeeds (Bruno Cornec)<br>
- init revamped (removed unnecessary second general module loading phase, start NFS appropriately depending on PXE or simple NFS) (Andree Leidenfrost)<br>
  - Changed module 'nfsacl' to 'nfs_acl' (Andree Leidenfrost)<br>
  - Mindi/DiskSize is gone (Bruno Cornec)<br>
- useless cat, sort|uniq commands removed (Bruno Cornec/S�bastien Aperghis-Tramoni)<br>
- Doc cleanup (Andree Leidenfrost)<br>
- Bug fix for chown i    n install.sh (JeffS)<br>
- CHANGES renamed also in install.sh now (Bruno Cornec)<br>
- rpmlint cleanups<br>
- Get mindi to look for analyze-my-lvm in it's library directory MINDI_LIB (See also Debian bug #351446.)<br>
- mindi only deletes freshly created 1440kb images in case of error (See also Debian Bug #348966.) (Andree Leidenfrost)<br>
- try standard grub-install in grub-MR restore script before trying anything fancy (Andree Leidenfrost)<br>
- busybox mount should be called with -o ro for PXE (Make RHEL 3 works in PXE with a 2.6 failsafe kernel now available) (Bruno Cornec)<br>
- Fix mindi for 2.6 Failsafe support (Bruno Cornec)<br>
- mindi now depends on grep >= 2.5 (for -m option) (Marco Puggelli/Bruno Cornec)<br>
- Fix a bug in LVM context for RHEL4 in GetValueFromField (R�mi Bondoin/Bruno Cornec)<br>
- New RPM Build environement (Bruno Cornec)<br>
- mindi now supports x86_64 natively (Bruno Cornec)<br>
- stop creating further size of floppy disks if the smaller one succeeds (Bruno Cornec)<br>
</p>    �  �                                                                                                                         �j !�C2006-03-20Mondo 2.0.7 / Mindi 1.0.7 are out</p>
<p>
I'm happy to announce the availability of the latest and greatest
version of mondo 2.0.7 / mindi 1.0.7. Version numbers have changed of
format to be more in line with what is done generally (x.y.z)
</p>
<p>
Now available at ftp://ftp.mondorescue.org :
</p>

<p>
mandrake/10.1/mindi-1.0.7-447.101mdk.i386.rpm<br>
mandrake/10.1/mondo-2.0.7-447.101mdk.i386.rpm<br>
mandrake/10.2/mindi-1.0.7-447.102mdk.i386.rpm<br>
mandrake/10.2/mondo-2.0.7-447.102mdk.i386.rpm<br>
mandriva/2006.0/mindi-1.0.7-447.20060mdk.i386.rpm<br>
mandriva/2006.0/mondo-2.0.7-447.20060mdk.i386.rpm<br>
src/mindi-1.0.7-r447.tgz<br>
src/mondo-2.0.7-r447.tgz<br>
</p>

<p>
Other distributions will follow asap (should be able to produce soon
redhat 7.3, fedora core 4, and only after rhel3/4 and sles9) due to the
change in the virtual machines I have to do (now u       1/mindi-1.0.7-460.101mdk.i386.rpm
mandrake/10.1/mondo-2.0.7-460.101mdk.i386.rpm
mandrake/10.2/mindi-1.0.7-460.102mdk.i386.rpm
mandrake/10.2/mondo-2.0.7-460.102mdk.i386.rpm
mandriva/2006.0/mindi-1.0.7-460.20060mdk.i586.rpm
mandriva/2006.0/mondo-2.0.7-460.20060mdk.i586.rpm
redhat/73/mindi-1.0.7-460.rh73.i386.rpm
redhat/73/mondo-2.0.7-460.rh73.i386.rpm
redhat/9/mindi-1.0.7-460.rh9.i386.rpm
redhat/9/mondo-2.0.7-460.rh9.i386.rpm
rhel/3/mindi-1.0.7-460.rhel3.i386.rpm
rhel/3/mondo-2.0.7-460.rhel3.i386.rpm
rhel/4/mindi-1.0.7-460.rhel4.i386.rpm
rhel/4/mondo-2.0.7-460.rhel4.i386.rpm
sles/9/mindi-1.0.7-460.sles9.i386.rpm
sles/9/mondo-2.0.7-460.sles9.i386.rpm
suse/10.0/mindi-1.0.7-464.suse10.0.i386.rpm
suse/10.0/mondo-2.0.7-464.suse10.0.i386.rpm
</p>


<p>
Changes are few:
</p>

<p>
MONDO:
</p>

<p>
Remove speed=1 for growisofs (better version expected for 2.0.8)
Usage of Epoch for RPMs to ease update
</p>

<p>
MINDI:
</p>

<p>
analyze-my-lvm is under $MINDI_SBIN (not _LIB)
Usage of Epoch for RPMs to ease update
</p>     s   �R	 !�2006-05-31Mondo 2.0.8-1 / Mindi 1.0.8-1 available</p>
<p>
I'm happy to announce the availability of a newest version of
mondo 2.0.8-1 / mindi 1.0.8-1. Enjoy it !
</p>

<p>
Now available at <a href="ftp://ftp.mondorescue.org/">ftp://ftp.mondorescue.org/</a>
</p>

<p>
fedora/4/mondo-2.0.8-1.fc4.i386.rpm
fedora/4/mindi-1.0.8-1.fc4.i386.rpm
fedora/5/mindi-1.0.8-1.fc5.i386.rpm
fedora/5/mondo-2.0.8-1.fc5.i386.rpm
mandrake/10.2/mindi-1.0.8-1.102mdk.i386.rpm
mandrake/10.2/mondo-2.0.8-1.102mdk.i386.rpm
mandrake/10.1/mindi-1.0.8-1.101mdk.i386.rpm
mandrake/10.1/mondo-2.0.8-1.101mdk.i386.rpm
mandriva/2006.   � !�s2006-04-08Mondo 2.0.7 / Mindi 1.0.7 updated to r460</p>
<p>
I'm happy to announce the availability of a newest version of
mondo 2.0.7 / mindi 1.0.7 aka r460
</p>

<p>
Now available at ftp://ftp.mondorescue.org/
</p>

<p>
fedora/4/mindi-1.0.7-460.fc4.i386.rpm
fedora/4/mondo-2.0.7-460.fc4.i386.rpm
fedora/5/mindi-1.0.7-460.fc5.i386.rpm
fedora/5/mondo-2.0.7-460.fc5.i386.rpm
mandrake/10.      0/mindi-1.0.8-1.20060mdk.i586.rpm
mandriva/2006.0/mondo-2.0.8-1.20060mdk.i586.rpm
redhat/7.3/mindi-1.0.8-1.rh73.i386.rpm
redhat/7.3/mondo-2.0.8-1.rh73.i386.rpm
redhat/9/mindi-1.0.8-1.rh9.i386.rpm
redhat/9/mondo-2.0.8-1.rh9.i386.rpm
rhel/3/mindi-1.0.8-1.rhel3.i386.rpm
rhel/3/mondo-2.0.8-1.rhel3.i386.rpm
rhel/4/mindi-1.0.8-1.rhel4.i386.rpm
rhel/4/mondo-2.0.8-1.rhel4.i386.rpm
sles/9/mindi-1.0.8-1.sles9.i386.rpm
sles/9/mondo-2.0.8-1.sles9.i386.rpm
suse/10.0/mindi-1.0.8-1.suse10.0.i386.rpm
suse/10.0/mondo-2.0.8-1.suse10.0.i386.rpm
src/mondo-doc-2.0.8-1.tar.gz
src/mindi-1.0.8-1.tar.gz
src/mondo-2.0.8-1.tar.gz
</p>

<p>As usual src.rpm packages are also available in the same directory. Also a new mondo-doc package is produced for most distributions containing all the formats of mondo's documentation. Only there for completion, as mondo's package already contains the required doc files.
</p>

<p>
This version should fix most of latest bugs reported for mindi. the 2.0.x branch should now enter in a bug fix only mo   de. All new features will be introduced in 2.2.x on which we will now work (internationalization, memory management, configuration files, ...)
</p>

<p>
Changes are :
</p>

<p>
MONDO:
</p>

<p>
- new build process (Bruno Cornec)<br>
- Fix a bug in .spec for RPM build (%attr now unused)
(Bruno Cornec)<br>
- Support of dm and LVM v2 (Andree Leidenfrost)<br>
- New mr_strtok functionn added and used for dm support (Andree Leidenfrost)<br>
- Complete doc is now a separate package. mondo still contains the
  man pages and howto in minimal useful formats
  (Bruno Cornec)<br>
  - HOWTO now contains a new chapter on unattended support for mondo<br>
  - Increase size (4 times) of include|exclude variables<br>
  - Fix a bug on -I and -E not working with multiple parameters<br>
- Fix a bug in verify for NFS by swapping nfs_remote_dir and isodir when
  assembling name for image file to verify
  (Andree Leidenfrost)<br>
- Fix mondo when restoring filenames containing blanks
  (still a problem for filenames with ')
  (B    runo Cornec)<br>
- Fix a RPM generation bug for rh7.3 (i386-redhat-linux prefix for binaries)
(Bruno Cornec)<br>
</p>

<p>
MINDI:
</p>

<p>
- new build process (Bruno Cornec)<br>
- Fix a bug when a disk less than 2.8 MB can be built, to
  include enough modules to support SCSI cds
  (Bruno Cornec)<br>
- Fix a bug in .spec for RPM build (%attr now unused)
(Bruno Cornec)<br>
- Add support for LABEL on swap partitions
(Michel Loiseleur + Julien Pinon)<br>
- Attempt to fix bug 6827 (addition of a script for
  busybox udhcpc to support pxe/dhcp restore)
  (Bruno Cornec)<br>
  - support of dm and LVM v2 (Andree Leidenfrost)<br>
  - analyze-my-lvm is under $MINDI_LIB (Andree Leidenfrost)<br>
- Fix a bug introduced by trying to avoid an error
  message when modprobe.d doesn't exist
  (Johannes Franken)<br>
- Fix for Bug #6975 (/net is now excluded from kernel search location)
(Bruno Cornec)<br>
- Allow 5670 MB fllopy disks for lilo as well (Bruno Cornec)<br>
- Add missing net modules (Klaus Ade Johnstad)<br>
</p>    �                                                                                                                                                                                                                                                                                  �V !�2006-06-09MondoRescue 2.0.8-3 is now available</p>

<p>I'm happy to announce the availability of a   �q
 !�Q2006-06-02Mondo 2.0.8-2 / Mindi 1.0.8-2 available</p>
<p>
It turned out that mindi 1.0.8-1 had bugs at least on RHEL 4 and also I wanted a new function for PXE deployment (change of NIC), so here are mondo 2.0.8-2 / mindi 1.0.8-2.
</p>
<p>
Available for the 11 distributions supported as the usual place <a href="ftp://ftp.mondorescue.org/">ftp://ftp.mondorescue.org/</a>
</p>
<p>
<b>CAUTION:</b> The ipconf parameter used for PXE deployment has changed and its syntaxt is NOT compatible with the previous one. You now need to precise first the NIC on which you will deploy. Look at mindi's README.pxe for details.
</p>
    newest version of mondoescue 2.0.8-3. Enjoy it as usual!</p>
<p>
Now available at <a href="ftp://ftp.mondorescue.org/">ftp://ftp.mondorescue.org/
</a>
</p>
<p>
./fedora/4/mindi-1.0.8-3.fc4.i386.rpm
./fedora/4/mondo-2.0.8-3.fc4.i386.rpm
./fedora/5/mindi-1.0.8-3.fc5.i386.rpm
./fedora/5/mondo-2.0.8-3.fc5.i386.rpm
./mandrake/10.2/mindi-1.0.8-3.102mdk.i386.rpm
./mandrake/10.2/mondo-2.0.8-3.102mdk.i386.rpm
./mandrake/10.1/mindi-1.0.8-3.101mdk.i386.rpm
./mandrake/10.1/mondo-2.0.8-3.101mdk.i386.rpm
./mandriva/2006.0/mindi-1.0.8-3.20060mdk.i586.rpm
./mandriva/2006.0/mondo-2.0.8-3.20060mdk.i586.rpm
./redhat/7.3/mindi-1.0.8-3.rh73.i386.rpm
./redhat/7.3/mondo-2.0.8-3.rh73.i386.rpm
./redhat/9/mindi-1.0.8-3.rh9.i386.rpm
./redhat/9/mondo-2.0.8-3.rh9.i386.rpm
./rhel/3/mondo-2.0.8-3.rhel3.i386.rpm
./rhel/3/mindi-1.0.8-3.rhel3.i386.rpm
./rhel/4/mindi-1.0.8-3.rhel4.i386.rpm
./rhel/4/mondo-2.0.8-3.rhel4.i386.rpm
./sles/9/mindi-1.0.8-3.sles9.i386.rpm
./sles/9/mondo-2.0.8-3.sles9.i386.rpm
./src/mindi-1.0.8-3.tar.gz
./src/mond    o-2.0.8-3.tar.gz
./suse/10.0/mindi-1.0.8-3.suse10.0.i386.rpm
./suse/10.0/mondo-2.0.8-3.suse10.0.i386.rpm
./suse/10.1/mindi-1.0.8-3.suse10.1.i586.rpm
./suse/10.1/mondo-2.0.8-3.suse10.1.i586.rpm
</p>

<p>As usual src.rpm packages are also available in the same directory.</p>

<p>
Changes are :
</p>
<p>
MINDI CHANGES<br>
<br>
- exec-shield removed for mindi<br>
  (Bruno Cornec)<br>
- Fix a bug for ia64 build in mindi where locallib was undefined<br>
  (Bruno Cornec)<br>
- Fix a bug for SuSE and Debian where $dfam was used in install.sh<br>
  (Bruno Cornec)<br>
- Make the init script mdadm-aware<br>
  (Andree Leidenfrost)<br>
<br>
</p><p>
MONDO CHANGES<br>
<br>
- Fix a bug in -I and -E handling !!<br>
  (Paolo Bernardoni <bernardoni_at_sysnet.it>/Bruno Cornec)<br>
- Fix permissions for autorun<br>
  (Bruno Cornec)<br>
- Fox delivery problems for tar files with too restrictive umask<br>
  (Bruno Cornec)<br>
- Fix parsing of DHCP information in start-nfs script<br>
  (Andree Leidenfrost)<br>
<br>
</p>
/src/mond   J J                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                �3 !�U2006-07-22Mondorescue at the RMLL</p>

<p>The presentation I made the <a href="http://www.rmll.info/conf_373">7th of July</a> for the <a href="http://www.rmll.info">RMLL</a> (Rencontres Mondiales du Logicial Libre) is now available online.</p>

<p>Around 25 persons attended it</p>

<p>See it at <a href="http://www.mondorescue.org/docs/mondo-presentation-v2.pdf">http://www.mondorescue.org/docs/mondo-presentation-v2.pdf</a></p>
