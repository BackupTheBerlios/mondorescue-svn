/***************************************************************************
       mondo-restore.c  -  restores mondoarchive data
                             -------------------
    begin                : Fri May 19 2000
    copyright            : (C) 2000 by Hugo Rabson
    email                : Hugo Rabson <hugorabson@msn.com>
    cvsid                : $Id$
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *                         Change Log                                      *
 ***************************************************************************
.


07/26
- workaround - if not /dev then don't call partimagehack

07/21
- if switch to Interactive Mode during nuke then don't nuke twice :) just once

07/18
- better support of users who boot from LVM CD and nuke-restore non-LVM backups

07/10
- when using 'star', exclude all 'not removed.' messages from log (misleading, they are)
- try w/ ACLs, then w/o ACLs if star fails w/ ACLs
- added ACL, xattr support for afio

06/26
- remove make_relevant_partition_bootable(); roll into mondo-prep.c
- changed various newtDrawRootText() calls to use g_noof_rows instead of
  integers

06/19
- don't try to access biggiestruct before it is populated w/ data relating
  to biggiefile; instead, use control char to find out if biggiefile is
  to be restored w/ partimagehack or not
- added AUX_VER

06/15
- read_cfg_file_into_bkpinfo() --- use user-supplied 'isodir' instead of
  archive's 'isodir' if different
  Conor Daly <conor.daly@met.ie>

06/17
- restore with partimagehack if NTFS device; dd, if non-NTFS device

06/14
- unmount all; remount, run grub-mr; unmount again

04/09
- fixed subset restoration bug introduced when I added 'star' support

04/04
- cleaned up restore_a_tarball_from_stream()

04/03
- added star support
- pause for 3s between partitioning and formatting (if in Nuke Mode)

03/28
- check that g_mountlist_fname was correcfly set; abort if it wasn't

03/25
- removed dev_null code

03/22/04
- added mode_of_file()
- added code to check for unauthorised modification of /dev/null
  by afio (for instance)

12/27/03
- check /tmp/cmdline instead of /proc/cmdline if on FreeBSD

11/15
- added g_fprep
- "Switch to interactive mode?" msg is now more informative

11/05
- after 'Are you sure?' when user specifies / as restore path, set
  restore_path[] to "" so that files are restored to [blank]/file/name :)

10/29
- moved "No restoring or comparing will take place today" block
  up to before iso_fiddly_bits (if iso) is called - fix bug
  if you're in ISO Mode and you say "exit to shell"

10/22
- swapped calls to chmod() and chown() after restoration
  of big files (Jens)

10/21
- changed "/mnt/cdrom" to MNT_CDROM

10/19
- restore biggiefiles selectively from CD properly
- use find_my_editor() to find pico/vi/whatever
- better use of call_program_and_get_last_line_of_output() to
  scan /proc/cmdline

10/18
- don't report 'missing compressor' if no compressor used at all

10/14
- log afio's error messages to /var/log/mondo-archive.log
  when restoring :)
- call vi if pico is not available

10/09
- better logging if fatal error (cannot openout bigfile)
- better GUI feedback when restoring big files
- restore_everything() now uses s_node* instead of char*
- ditto restore_all_*_from_*()

10/02
- succinct msg instead of pop-ups, if used -H

09/27
- tweaked restore-time gui

09/26
- proper reporting of DVD/CDR/etc. type in displayed dialogs

09/23
- malloc/free global strings in new subroutines - malloc_libmondo_global_strings()
  and free_libmondo_global_strings() - which are in libmondo-tools.c

09/21
- trying to fix "mondorestore <no params>" segfault

09/18
- better remounting of /
- cleaned up run_grub()
- sensible command-line handling in Live Mode

09/17
- cleaned up GRUB installer script a bit

09/15
- remount / as r/w if in disaster recovery mode;
  helps for b0rken distros

09/13
- major NTFS hackage

09/12
- changed some in-sub var decl'ns to malloc()'s

09/05
- don't let me run unless I'm root
- finished NTFS/partimagehack support (CD only); working on tape now

09/01
- fixed cosmetic bug in biggiefile restore window

06/01 - 08/31
- added code to main() to make sure NFS
  (backup) share is mounted in Nuke and Compare Modes
- added code to run_grub() to mount /boot before running grub-install
- fixed some stray assert()'s in restore_a_biggiefile_from_stream()
- fixed bugs in extract_config_file_from_ramdisk()
  and get_cfg_file_from_archive() which
  stopped fape+floppy users from being able to
  boot from floppy and leave floppy in drive :)
- added hooks to partimage for doing imagedevs
- fixed calls to popup_and_get_string()

05/01 - 05/31
- fixed biggiefile atime/utime dates restoration bug, I think
- added misc clean-up (Steve Hindle)
- fixed support for subdir-within-NFS-mount
- if nuke mode fails & user reverts to interactive mode _and succeeds_,
  don't claim nuke mode aborted :)
- unmount_all_devices() uses mountlist->el[lino].mountpt
  instead of mountlist->el[lino].device where possible
- added Joshua Oreman's FreeBSD patches
- copied missing paragraph from 1.6x's read_cfg_file_into_bkpinfo()
  to 1.7x's; affected tape streamer users (badly!)
- cleaned up some paranoid assert()'s
- use which("post-nuke") instead of find_home_of_exe("post-nuke")
- fixed "Don't eject while restoring" mode
- get_cfg_file_from_archive() --- also recovers mountlist.txt now :)
- don't eject if 'donteject' is in kernel's command line
- added 'don't panic' msg to start of log

04/01 - 04/30
- added text mode (cat /proc/cmdline; if textonly then text mode is on)
- delete /var/lock/subsys/ * when nuking
- don't resize mountlist if "noresize" present in /proc/cmdline
- changed from chmod -R 1777 tmp to chmod 1777 tmp
- replace newtFinished() and newtInit() with
  newtSuspend() and newtResume()
- get_cfg_file_from_archive() returns 1 instead of aborting now
- read_cfg_file_into_bkpinfo) --- if autorun CD but its config
  file indicates a non-CD backup media then ask, just in case
- sped up restore_a_tarball_from_CD() a bit
- line 4469 --- if post-nuke not found then don't run it :)
- replaced "/mnt/RESTORING" with MNT_RESTORING (#define'd)
- moved compare_*() into mondorestore/mondo-rstr-compare.c
- moved some RAID subroutines into common/libmondo-raid.c
- fixed some iso live compare problems
- replaced FILELIST_FULL with g_filelist_full and FILELIST_FULL_STUB;
  g_filelist_full being the full path of the filelist.full text file and
  FILELIST_FULL_STUB being "tmp/filelist.full" (relative path);
- ditto BIGGIELIST_TXT, MONDO_CFG_FILE
- added lots of assert()'s and log_OS_error()'s
- in Nuke Mode, check mountlist's sanity before doing anything else;
  if it fails sanity test, offer to revert to Interactive Mode (or abort)
- copy log to /mnt/RESTORING/root at end
- read_cfg_file_into_bkpinfo() --- read iso-dev and isodir if bkptype==iso
- line 1701 --- delete ramdisk file after extracting config info
- moved call to make_relevant_partitions_bootable() from
  within run_boot_loader() to within interactive_mode() and
  nuke_mode(), after unmounting disks
- if editing fstab or *.conf, try to use pico if available
- better calling of make-me-bootable
- don't sort mountlist anywhere anymore except _locally_ in
  mount_all_devices() and unmount_all_devices()
- edit fstab, grub.conf _after_ stabgrub-me if it fails
- run_boot_loader() --- backup all crucial files to /etc/ *.pristine first
- added iso_fiddly_bits()
- fixed ISO mode support
- mount_cdrom() only searches for device if NOT in disaster recovery mode
- changed lost of system()'s into run_program_and_log_output()'s
- don't eject if bkpinfo->please_dont_eject_when_restoring
- cleaned up post-nuke handling code
- always eject CD at end, unless bkpinfo->please_dont_...
- misc clean-up (Tom Mortell)
- afio uses -c (1024L*1024L)/TAPE_BLOCK_SIZE now
  instead of -c 1024

01/01 - 03/31/2003
- commented out sort_... line (BB)
- clean-up (Stan Benoit)
- added code for LVM and SW Raid (Brian Borgeson)
- line 814 - added -p to 'mkdir -p tmp'
- mount_cdrom() - calls find_cdrom_device() if
  bkpinfo->media_device is blank (to fill it)

11/01 - 12/31/2002
- mount_cdrom() better at handling multiple CD drives
- minor clean-up in restore_a_tarball_from_CD()
- if --live-from-cd then assume restoring live from CD
- tweaked it to run better w/ ArkLinux
- create /mnt/RESTORING/mnt/.boot.d for Gentoo users
  after restoring
- cleaned up iso_mode(); no longer asks for NFS info 3 times
- mount_cdrom() was trying to mount /mnt/isodir/%s/%d.iso;
  is now just %s/%d.iso
- mount/unmount /boot if necessary (Gentoo)
- added RAW MBR support; added run_raw_mbr() for the purpose
- unmount & remount supermounts at start/end of live restore
- copy /tmp/mountlist.txt to /tmp/mountlist.txt.orig at start
- cleaned up string-handling in get_cfg_info_from_archives()
- fixed run_grub() to call new stabgrub-me script
- popup list of changed files after Compare Mode
- permit mondorestore --edit-mountlist even if live mode
- create a repaired copy of grub-install which is RAID-friendly;
  use it when initializing boot sector with run_grub()
- use grub-MR instead of grub-install
- fixed read_cfg_file_into_bkpinfo() to ignore cfg file's backup_media_type
  if user has already specified a backup media type interactively

10/01 - 10/31
- run_grub() will let you specify the boot device as well as edit the system
  files, if grub-install fails
- fixed bug in fwrite() call in restore_biggiefile_from_CD()
- fixed bug affecting restoration of bigfiles from CD's w/0 compression
- run_grub() will run 'grub-install {boot device}' instead of
  'grub-install (hd0)'

09/01 - 09/30
- use /tmp/tmpfs/mondo.tmp instead of /tmp/mondo.tmp
- initialize MOUNTLIST_FNAME at start of main()
- differential-related cleanup
- better handling of CD-ROM drives which aren't /dev/cdrom :)
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_
- always load config file from archive before operating on it
- moved some subroutines around; now closer to alphabetical order
- changed mount.mindi to mount.bootisk
- mount disks readonly if in Compare Mode
- set /dev/null's permissions to 777, just in case it somehow gets mangled
  ...which apparently happen with some devfs-based Linux distributions
- remove /var/run/ *.pid after restoring
- move spurious lockfiles from /home/ * to /home/ * /.disabled
- if Interactive Mode then ask user which media, etc. (i.e. catchall mode
  is now same as Interactive Mode)

08/01 - 08/30
- use data structure to store the fname, checksum, mods & perms of each bigfile
  ... biggiestruct :)
- if a filelist is tiny (2 bytes or less) then ignore it
- insist_on_this_cd_number() --- now takes 2 params, not 1
- re-enabled 'g_current_media_number = 1' in restore_everything()
- added same to compare_mode()
- replaced lots of global char[]'s with malloc()'s
- if differential backup then don't permit formatting or fdisking,
  whether Interactive or Nuke mode
- added call to register_pid() at start of main()
- if Nuke Mode & it succeeds then ask user if they have contributed yet
- changed tape-size to media-size (config file)
- changed using_* to backup_media_type
- changed *_from_tape to *_from_stream

07/01 - 07/31
- added find_and_mount_actual_cdrom()
- temp dir is always random
- skip tarballs if they don't contain files we're looking for
  (used to read the whole thing & _then_ skip)
- use media_size[1] instead of media_size[0]
- fixed serious bug in line 1546 - should have been !=, not ==; stopped
  mondorestore from correctly restoring big files
- bigfile piping enhancements (Philippe de Muyter)
- unmount CD-ROM after restoring from live filesystem
- TAPE_BLOCK_SIZE treated as %ld, not %d

06/01 - 06/30
- added signal-trapping
- disabled 'nr-failed-disks' flag
- fixed problem w/selective restore
- don't change /tmp's permissions unless it doesn't exist & must be created
- fixed bug in --mbr
- is_file_in_list() enhanced to exclude /mnt/RESTORING or whatever
- added support for uncompressed archives
- --monitas-live now accepts path-to-restore_to_, not just path to restore
- added some debugging/tracking code to the NFS section
- various monitas-related enhancements
- added --isonuke and --mbr switches
- better logging in run_grub()
- improved --monitas-live
- mkdir -p /mnt/RESTORING/var/run/console just in case user excludes it
- afio now uses 16MB buffer instead of 8MB
- always use bkpinfo->media_size[0], now that -s has been expanded
- popup and ask where to restore data, if restoring selectively

05/01 - 05/31
- add '--monitas' flag
- don't run chmod -R 1777 /mnt/RESTORING/tmp before unmounting unless
  restoring at the time...

04/01 - 04/30
- delete old /tmp/filelist.full,biggielist.txt if found when restoring to
  live filesystem
- replace MONDO_VERSION #define with VERSION from ../config.h
- write fname of bigfile to screen when having trouble reading/comparing it
- if restoring to live filesystem then wipe /tmp/tmpfs/ * afterwards
- removed spurious finish(0) from main()

03/01 - 03/31/2002
- if /tmp/m*ndo-restore.cfg not found then assume live restore; restore
  to / instead of /mnt/RESTORING
- clean up is_file_in_list() to deal with the /mnt/RESTORING/ prefix
- exclude leading '/' from filelist.restore-these
- if /tmp/fstab.new exists then use _it_ instead of /tmp/fstab to label
  ext2 or ext3 partitions
- improved logging

[...]

07/10/2001 --- first incarnation
*/


/**
 * @file
 * The main file for mondorestore.
 */

/**************************************************************************
 * #include statements                                                    *
 **************************************************************************/
#include <pthread.h>
#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#include "mr-externs.h"
#include "mondo-restore.h"
#include "mondo-rstr-compare-EXT.h"
#include "mondo-rstr-tools-EXT.h"

extern void success_message(void);
extern void twenty_seconds_til_yikes(void);


/* For use in other programs (ex. XMondo) */
#ifdef MONDORESTORE_MODULE
#define main __mondorestore_main
#define g_ISO_restore_mode __mondorestore_g_ISO_restore_mode
#endif

//static char cvsid[] = "$Id$";

/**************************************************************************
 * Globals                                                                *
 **************************************************************************/
extern char *g_tmpfs_mountpt;	// declared in libmondo-tools.c
extern struct s_bkpinfo *g_bkpinfo_DONTUSETHIS;	// used by finish() to free
												// up global bkpinfo struct
extern bool g_text_mode;
extern FILE *g_fprep;
extern double g_kernel_version;
extern int g_partition_table_locked_up;
extern int g_noof_rows;

extern int partition_everything(struct mountlist_itself *mountlist);


/**
 * @name Restore-Time Globals
 * @ingroup globalGroup
 * @{
 */
/**
 * If TRUE, then SIGPIPE was just caught.
 * Set by the signal handler; cleared after it's handled.
 */
bool g_sigpipe_caught = FALSE;

/**
 * If TRUE, then we're restoring from ISOs or an NFS server.
 * If FALSE, then we're restoring from some kind of real media (tape, CD, etc.)
 */
bool g_ISO_restore_mode = FALSE;	/* are we in Iso Mode? */

/**
 * If TRUE, then we have had a successful "nuke" restore.
 */
bool g_I_have_just_nuked = FALSE;

/**
 * The device to mount to get at the ISO images. Ignored unless @p g_ISO_restore_mode.
 */
char *g_isodir_device;

/**
 * The format of @p g_isodir_device. Ignored unless @p g_ISO_restore_mode.
 */
char *g_isodir_format;

/**
 * The location of 'biggielist.txt', containing the biggiefiles on the current archive set.
 */
char *g_biggielist_txt;

/**
 * The location of 'filelist.full', containing all files (<em>including biggiefiles</em>) on
 * the current archive set.
 */
char *g_filelist_full;

/**
 * The location of a file containing a list of the devices that were archived
 * as images, not as individual files.
 */
char *g_filelist_imagedevs;

/**
 * The location of a file containing a list of imagedevs to actually restore.
 * @see g_filelist_imagedevs
 */
char *g_imagedevs_restthese;

/**
 * The location of 'mondo-restore.cfg', containing the metadata
 * information for this backup.
 */
char *g_mondo_cfg_file;

/**
 * The location of 'mountlist.txt', containing the information on the
 * user's partitions and hard drives.
 */
char *g_mountlist_fname;

/**
 * Mondo's home directory during backup. Unused in mondo-restore; included
 * to avoid link errors.
 */
char *g_mondo_home;

/* @} - end of "Restore-Time Globals" in globalGroup */



extern int copy_from_src_to_dest(FILE * f_orig, FILE * f_archived,
								 char direction);



/**************************************************************************
 * COMPAQ PROLIANT Stuff:  needs some special help                        *
**************************************************************************/

/**
 * The message to display if we detect that the user is using a Compaq Proliant.
 */
#define COMPAQ_PROLIANTS_SUCK "Partition and format your disk using Compaq's disaster recovery CD. After you've done that, please reboot with your Mondo CD/floppy in Interactive Mode."




/**
 * Allow the user to modify the mountlist before we partition & format their drives.
 * @param bkpinfo The backup information structure. @c disaster_recovery is the only field used.
 * @param mountlist The mountlist to let the user modify.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @return 0 for success, nonzero for failure.
 * @ingroup restoreGuiGroup
 */
int let_user_edit_the_mountlist(struct s_bkpinfo *bkpinfo,
								struct mountlist_itself *mountlist,
								struct raidlist_itself *raidlist)
{
	int retval = 0, res = 0;

	log_msg(2, "let_user_edit_the_mountlist() --- starting");

	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);
	if (!bkpinfo->disaster_recovery) {
		strcpy(g_mountlist_fname, "/tmp/mountlist.txt");
		log_msg(2, "I guess you're testing edit_mountlist()");
	}
	if (!does_file_exist(g_mountlist_fname)) {
		log_to_screen(g_mountlist_fname);
		log_to_screen("does not exist");
		return (1);
	}

	retval = load_mountlist(mountlist, g_mountlist_fname);
	load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
	if (retval) {
		log_to_screen
			("Warning - load_raidtab_into_raidlist returned an error");
	}
	res = edit_mountlist(g_mountlist_fname, mountlist, raidlist);
	if (res) {
		return (1);
	}

	save_mountlist_to_disk(mountlist, g_mountlist_fname);
	save_raidlist_to_raidtab(raidlist, RAIDTAB_FNAME);

	log_to_screen("I have finished editing the mountlist for you.");

	return (retval);
}





/**
 * Determine whether @p mountlist contains a Compaq diagnostic partition.
 * @param mountlist The mountlist to examine.
 * @return TRUE if there's a Compaq diagnostic partition; FALSE if not.
 * @ingroup restoreUtilityGroup
 */
bool
partition_table_contains_Compaq_diagnostic_partition(struct
													 mountlist_itself *
													 mountlist)
{
	int i;

	assert(mountlist != NULL);

	for (i = 0; i < mountlist->entries; i++) {
		if (strstr(mountlist->el[i].format, "ompaq")) {
			log_msg(2, "mountlist[%d] (%s) is %s (Compaq alert!)",
					i, mountlist->el[i].device, mountlist->el[i].format);

			return (TRUE);
		}
	}
	return (FALSE);
}

/**************************************************************************
 *END_PARTITION_TABLE_CONTAINS_COMPAQ_DIAGNOSTIC_PARTITION                *
 **************************************************************************/


/**
 * Allow the user to abort the backup if we find that there is a Compaq diagnostic partition.
 * @note This function does not actually check for the presence of a Compaq partition.
 * @ingroup restoreUtilityGroup
 */
void offer_to_abort_because_Compaq_Proliants_suck(void)
{
	popup_and_OK(COMPAQ_PROLIANTS_SUCK);
	if (ask_me_yes_or_no
		("Would you like to reboot and use your Compaq CD to prep your hard drive?"))
	{
		fatal_error
			("Aborting. Please reboot and prep your hard drive with your Compaq CD.");
	}
}

/**************************************************************************
 *END_OFFER_TO_ABORT_BECAUSE_COMPAQ_PROLIANTS_SUCK                        *
 **************************************************************************/



/**
 * Call interactive_mode(), nuke_mode(), or compare_mode() depending on the user's choice.
 * @param bkpinfo The backup information structure. Most fields are used.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist to go with @p mountlist.
 * @return The return code from the mode function called.
 * @ingroup restoreGroup
 */
int
catchall_mode(struct s_bkpinfo *bkpinfo,
			  struct mountlist_itself *mountlist,
			  struct raidlist_itself *raidlist)
{
	char c, *tmp;
	int retval = 0;

	iamhere("inside catchall");
	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);
	malloc_string(tmp);
	iamhere("pre wrm");
	c = which_restore_mode();
	iamhere("post wrm");
	if (c == 'I' || c == 'N' || c == 'C') {
		interactively_obtain_media_parameters_from_user(bkpinfo, FALSE);
	} else {
		popup_and_OK("No restoring or comparing will take place today.");
		if (is_this_device_mounted("/mnt/cdrom")) {
			run_program_and_log_output("umount /mnt/cdrom", FALSE);
		}
		if (g_ISO_restore_mode) {
			sprintf(tmp, "umount %s", bkpinfo->isodir);
			run_program_and_log_output(tmp, FALSE);
		}
		paranoid_MR_finish(0);
	}

	iamhere("post int");

	if (bkpinfo->backup_media_type == iso) {
		if (iso_fiddly_bits(bkpinfo, (c == 'N') ? TRUE : FALSE)) {
			log_msg(2,
					"catchall_mode --- iso_fiddly_bits returned w/ error");
			return (1);
		} else {
			log_msg(2, "catchall_mode --- iso_fiddly_bits ok");
		}
	}

	if (c == 'I') {
		log_msg(2, "IM selected");
		retval += interactive_mode(bkpinfo, mountlist, raidlist);
	} else if (c == 'N') {
		log_msg(2, "NM selected");
		retval += nuke_mode(bkpinfo, mountlist, raidlist);
	} else if (c == 'C') {
		log_msg(2, "CM selected");
		retval += compare_mode(bkpinfo, mountlist, raidlist);
	}
	paranoid_free(tmp);
	return (retval);
}

/**************************************************************************
 *END_CATCHALL_MODE                                                      *
 **************************************************************************/

/**************************************************************************
 *END_  EXTRACT_CONFIG_FILE_FROM_RAMDISK                                  *
 **************************************************************************/


/**
 * Locate an executable in the directory structure rooted at @p restg.
 * @param out_path Where to put the executable.
 * @param fname The basename of the executable.
 * @param restg The directory structure to look in.
 * @note If it could not be found in @p restg then @p fname is put in @p out_path.
 * @ingroup restoreUtilityGroup
 */
void
find_pathname_of_executable_preferably_in_RESTORING(char *out_path,
													char *fname,
													char *restg)
{
	assert(out_path != NULL);
	assert_string_is_neither_NULL_nor_zerolength(fname);

	sprintf(out_path, "%s/sbin/%s", restg, fname);
	if (does_file_exist(out_path)) {
		sprintf(out_path, "%s/usr/sbin/%s", restg, fname);
		if (does_file_exist(out_path)) {
			sprintf(out_path, "%s/bin/%s", restg, fname);
			if (does_file_exist(out_path)) {
				sprintf(out_path, "%s/usr/bin/%s", restg, fname);
				if (does_file_exist(out_path)) {
					strcpy(out_path, fname);
				}
			}
		}
	}
}

/**************************************************************************
 *END_FIND_PATHNAME_OF_EXECUTABLE_PREFERABLY_IN_RESTORING                 *
 **************************************************************************/




/**
 * @addtogroup restoreGroup
 * @{
 */
/**
 * Restore the user's data, in a disaster recovery situation, prompting the
 * user about whether or not to do every step.
 * The user can edit the mountlist, choose files to restore, etc.
 * @param bkpinfo The backup information structure. Most fields are used.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist to go with @p mountlist.
 * @return 0 for success, or the number of errors encountered.
 */
int
interactive_mode(struct s_bkpinfo *bkpinfo,
				 struct mountlist_itself *mountlist,
				 struct raidlist_itself *raidlist)
{
	int retval = 0;
	int res;
	int ptn_errs = 0;
	int fmt_errs = 0;

	bool done;
	bool restore_all;

  /** needs malloc **********/
	char *tmp;
	char *fstab_fname;
	char *old_restpath;

	struct s_node *filelist;

	/* try to partition and format */

	log_msg(2, "interactive_mode --- starting (great, assertions OK)");

	malloc_string(tmp);
	malloc_string(fstab_fname);
	malloc_string(old_restpath);
	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);

	log_msg(2, "interactive_mode --- assertions OK");

	if (g_text_mode) {
		if (!ask_me_yes_or_no
			("Interactive Mode + textonly = experimental! Proceed anyway?"))
		{
			fatal_error("Wise move.");
		}
	}

	iamhere("About to load config file");
	get_cfg_file_from_archive_or_bust(bkpinfo);
	read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
	iamhere("Done loading config file; resizing ML");
#ifdef __FreeBSD__
	if (strstr
		(call_program_and_get_last_line_of_output("cat /tmp/cmdline"),
		 "noresize"))
#else
	if (strstr
		(call_program_and_get_last_line_of_output("cat /proc/cmdline"),
		 "noresize"))
#endif
	{
		log_msg(1, "Not resizing mountlist.");
	} else {
		resize_mountlist_proportionately_to_suit_new_drives(mountlist);
	}
	for (done = FALSE; !done;) {
		iamhere("About to edit mountlist");
		if (g_text_mode) {
			save_mountlist_to_disk(mountlist, g_mountlist_fname);
			sprintf(tmp, "%s %s", find_my_editor(), g_mountlist_fname);
			res = system(tmp);
			load_mountlist(mountlist, g_mountlist_fname);
		} else {
			res = edit_mountlist(g_mountlist_fname, mountlist, raidlist);
		}
		iamhere("Finished editing mountlist");
		if (res) {
			paranoid_MR_finish(1);
		}
		log_msg(2, "Proceeding...");
		save_mountlist_to_disk(mountlist, g_mountlist_fname);
		save_raidlist_to_raidtab(raidlist, RAIDTAB_FNAME);
		mvaddstr_and_log_it(1, 30, "Restoring Interactively");
		if (bkpinfo->differential) {
			log_to_screen("Because this is a differential backup, disk");
			log_to_screen
				(" partitioning and formatting will not take place.");
			done = TRUE;
		} else {
			if (ask_me_yes_or_no
				("Do you want to erase and partition your hard drives?")) {
				if (partition_table_contains_Compaq_diagnostic_partition
					(mountlist)) {
					offer_to_abort_because_Compaq_Proliants_suck();
					done = TRUE;
				} else {
					twenty_seconds_til_yikes();
					g_fprep = fopen("/tmp/prep.sh", "w");
					ptn_errs = partition_everything(mountlist);
					if (ptn_errs) {
						log_to_screen
							("Warning. Errors occurred during disk partitioning.");
					}

					fmt_errs = format_everything(mountlist, FALSE);
					if (!fmt_errs) {
						log_to_screen
							("Errors during disk partitioning were handled OK.");
						log_to_screen
							("Partitions were formatted OK despite those errors.");
						ptn_errs = 0;
					}
					if (!ptn_errs && !fmt_errs) {
						done = TRUE;
					}
				}
				paranoid_fclose(g_fprep);
			} else {
				mvaddstr_and_log_it(g_currentY++, 0,
									"User opted not to partition the devices");
				if (ask_me_yes_or_no
					("Do you want to format your hard drives?")) {
					fmt_errs = format_everything(mountlist, TRUE);
					if (!fmt_errs) {
						done = TRUE;
					}
				} else {
					ptn_errs = fmt_errs = 0;
					done = TRUE;
				}
			}
			if (fmt_errs) {
				mvaddstr_and_log_it(g_currentY++,
									0,
									"Errors occurred. Please repartition and format drives manually.");
				done = FALSE;
			}
			if (ptn_errs & !fmt_errs) {
				mvaddstr_and_log_it(g_currentY++,
									0,
									"Errors occurred during partitioning. Formatting, however, went OK.");
				done = TRUE;
			}
			if (!done) {
				if (!ask_me_yes_or_no("Re-edit the mountlist?")) {
					retval++;
					goto end_of_func;
				}
			}
		}
	}

	/* mount */
	if (mount_all_devices(mountlist, TRUE)) {
		unmount_all_devices(mountlist);
		retval++;
		goto end_of_func;
	}
	/* restore */
	if ((restore_all =
		 ask_me_yes_or_no("Do you want me to restore all of your data?")))
	{
		log_msg(1, "Restoring all data");
		retval += restore_everything(bkpinfo, NULL);
	} else
		if ((restore_all =
			 ask_me_yes_or_no
			 ("Do you want me to restore _some_ of your data?"))) {
		strcpy(old_restpath, bkpinfo->restore_path);
		for (done = FALSE; !done;) {
			unlink("/tmp/filelist.full");
			filelist = process_filelist_and_biggielist(bkpinfo);
			/* Now you have /tmp/tmpfs/filelist.restore-these and /tmp/tmpfs/biggielist.restore-these;
			   the former is a list of regular files; the latter, biggiefiles and imagedevs.
			 */
			if (filelist) {
			  gotos_suck:
				strcpy(tmp, old_restpath);
// (NB: %s is where your filesystem is mounted now, by default)", MNT_RESTORING);
				if (popup_and_get_string
					("Restore path", "Restore files to where?", tmp,
					 MAX_STR_LEN / 4)) {
					if (!strcmp(tmp, "/")) {
						if (!ask_me_yes_or_no("Are you sure?")) {
							goto gotos_suck;
						}
						tmp[0] = '\0';	// so we restore to [blank]/file/name :)
					}
					strcpy(bkpinfo->restore_path, tmp);
					log_msg(1, "Restoring subset");
					retval += restore_everything(bkpinfo, filelist);
					free_filelist(filelist);
				} else {
					strcpy(bkpinfo->restore_path, old_restpath);
					free_filelist(filelist);
				}
				if (!ask_me_yes_or_no
					("Restore another subset of your backup?")) {
					done = TRUE;
				}
			} else {
				done = TRUE;
			}
		}
		strcpy(old_restpath, bkpinfo->restore_path);
	} else {
		mvaddstr_and_log_it(g_currentY++,
							0,
							"User opted not to restore any data.                                  ");
	}
	if (retval) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							"Errors occurred during the restore phase.            ");
	}

	if (ask_me_yes_or_no("Initialize the boot loader?")) {
		run_boot_loader(TRUE);
	} else {
		mvaddstr_and_log_it(g_currentY++,
							0,
							"User opted not to initialize the boot loader.");
	}

//  run_program_and_log_output("cp -af /etc/lvm " MNT_RESTORING "/etc/", 1);
	protect_against_braindead_sysadmins();
	//  modify_rclocal_one_time( MNT_RESTORING "/etc" );
	retval += unmount_all_devices(mountlist);
	/*  if (restore_some || restore_all || */
	if (ask_me_yes_or_no
		("Label your ext2 and ext3 partitions if necessary?")) {
		mvaddstr_and_log_it(g_currentY, 0,
							"Using e2label to label your ext2,3 partitions");
		if (does_file_exist("/tmp/fstab.new")) {
			strcpy(fstab_fname, "/tmp/fstab.new");
		} else {
			strcpy(fstab_fname, "/tmp/fstab");
		}
		sprintf(tmp,
				"label-partitions-as-necessary %s < %s >> %s 2>> %s",
				g_mountlist_fname, fstab_fname, MONDO_LOGFILE,
				MONDO_LOGFILE);
		res = system(tmp);
		if (res) {
			log_to_screen
				("label-partitions-as-necessary returned an error");
			mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
		} else {
			mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		}
		retval += res;
	}

	iamhere("About to leave interactive_mode()");
	if (retval) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							"Warning - errors occurred during the restore phase.");
	}
  end_of_func:
	paranoid_free(tmp);
	paranoid_free(fstab_fname);
	paranoid_free(old_restpath);
	iamhere("Leaving interactive_mode()");
	return (retval);
}

/**************************************************************************
 *END_INTERACTIVE_MODE                                                    *
 **************************************************************************/



/**
 * Run an arbitrary restore mode (prompt the user), but from ISO images
 * instead of real media.
 * @param bkpinfo The backup information structure. Most fields are used.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @param nuke_me_please If TRUE, we plan to run Nuke Mode.
 * @return 0 for success, or the number of errors encountered.
 */
int
iso_mode(struct s_bkpinfo *bkpinfo,
		 struct mountlist_itself *mountlist,
		 struct raidlist_itself *raidlist, bool nuke_me_please)
{
	char c;
	int retval = 0;

	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);
	if (iso_fiddly_bits(bkpinfo, nuke_me_please)) {
		log_msg(1, "iso_mode --- returning w/ error");
		return (1);
	} else {
		c = which_restore_mode();
		if (c == 'I' || c == 'N' || c == 'C') {
			interactively_obtain_media_parameters_from_user(bkpinfo,
															FALSE);
		}
		if (c == 'I') {
			retval += interactive_mode(bkpinfo, mountlist, raidlist);
		} else if (c == 'N') {
			retval += nuke_mode(bkpinfo, mountlist, raidlist);
		} else if (c == 'C') {
			retval += compare_mode(bkpinfo, mountlist, raidlist);
		} else {
			log_to_screen("OK, I shan't restore/compare any files.");
		}
	}
	if (is_this_device_mounted(MNT_CDROM)) {
		paranoid_system("umount " MNT_CDROM);
	}
//  if (! already_mounted)
//    {
	if (system("umount /tmp/isodir 2> /dev/null")) {
		log_to_screen
			("WARNING - unable to unmount device where the ISO files are stored.");
	}
//    }
	return (retval);
}

/**************************************************************************
 *END_ISO_MODE                                                            *
 **************************************************************************/


/*            MONDO - saving your a$$ since Feb 18th, 2000            */




/**
 * Restore the user's data automatically (no prompts), after a twenty-second
 * warning period.
 * @param bkpinfo The backup information structure. Most fields are used.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @return 0 for success, or the number of errors encountered.
 * @warning <b><i>THIS WILL ERASE ALL EXISTING DATA!</i></b>
 */
int
nuke_mode(struct s_bkpinfo *bkpinfo,
		  struct mountlist_itself *mountlist,
		  struct raidlist_itself *raidlist)
{
	int retval = 0;
	int res = 0;
	bool boot_loader_installed = FALSE;
  /** malloc **/
	char tmp[MAX_STR_LEN], tmpA[MAX_STR_LEN], tmpB[MAX_STR_LEN],
		tmpC[MAX_STR_LEN];

	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);

	log_msg(2, "nuke_mode --- starting");

	get_cfg_file_from_archive_or_bust(bkpinfo);
	load_mountlist(mountlist, g_mountlist_fname);	// in case read_cfg_file_into_bkpinfo updated the mountlist
#ifdef __FreeBSD__
	if (strstr
		(call_program_and_get_last_line_of_output("cat /tmp/cmdline"),
		 "noresize"))
#else
	if (strstr
		(call_program_and_get_last_line_of_output("cat /proc/cmdline"),
		 "noresize"))
#endif
	{
		log_msg(2, "Not resizing mountlist.");
	} else {
		resize_mountlist_proportionately_to_suit_new_drives(mountlist);
	}
	if (!evaluate_mountlist(mountlist, tmpA, tmpB, tmpC)) {
		sprintf(tmp,
				"Mountlist analyzed. Result: \"%s %s %s\" Switch to Interactive Mode?",
				tmpA, tmpB, tmpC);
		if (ask_me_yes_or_no(tmp)) {
			retval = interactive_mode(bkpinfo, mountlist, raidlist);
			finish(retval);
		} else {
			fatal_error("Nuke Mode aborted. ");
		}
	}
	save_mountlist_to_disk(mountlist, g_mountlist_fname);
	mvaddstr_and_log_it(1, 30, "Restoring Automatically");
	if (bkpinfo->differential) {
		log_to_screen("Because this is a differential backup, disk");
		log_to_screen("partitioning and formatting will not take place.");
		res = 0;
	} else {
		if (partition_table_contains_Compaq_diagnostic_partition
			(mountlist)) {
			offer_to_abort_because_Compaq_Proliants_suck();
		} else {
			twenty_seconds_til_yikes();
			g_fprep = fopen("/tmp/prep.sh", "w");
#ifdef __FreeBSD__
			if (strstr
				(call_program_and_get_last_line_of_output
				 ("cat /tmp/cmdline"), "nopart"))
#else
			if (strstr
				(call_program_and_get_last_line_of_output
				 ("cat /proc/cmdline"), "nopart"))
#endif
			{
				log_msg(2,
						"Not partitioning drives due to 'nopart' option.");
				res = 0;
			} else {
				res = partition_everything(mountlist);
				if (res) {
					log_to_screen
						("Warning. Errors occurred during partitioning.");
					res = 0;
				}
			}
			retval += res;
			if (!res) {
				log_to_screen("Preparing to format your disk(s)");
				sleep(1);
				system("sync");
				log_to_screen("Please wait. This may take a few minutes.");
				res += format_everything(mountlist, FALSE);
			}
			paranoid_fclose(g_fprep);
		}
	}
	retval += res;
	if (res) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							"Failed to partition and/or format your hard drives.");

		if (ask_me_yes_or_no("Try in interactive mode instead?")) {
			retval = interactive_mode(bkpinfo, mountlist, raidlist);
			goto after_the_nuke;
		} else
			if (!ask_me_yes_or_no
				("Would you like to try to proceed anyway?")) {
			return (retval);
		}
	}
	retval = mount_all_devices(mountlist, TRUE);
	if (retval) {
		unmount_all_devices(mountlist);
		log_to_screen
			("Unable to mount all partitions. Sorry, I cannot proceed.");
		return (retval);
	}
	iamhere("Restoring everything");
	retval += restore_everything(bkpinfo, NULL);
	if (!run_boot_loader(FALSE)) {
		log_msg(1,
				"Great! Boot loader was installed. No need for msg at end.");
		boot_loader_installed = TRUE;
	}
	protect_against_braindead_sysadmins();
//  run_program_and_log_output("cp -af /etc/lvm " MNT_RESTORING "/etc/", 1);
	//  modify_rclocal_one_time( MNT_RESTORING "/etc" );
	retval += unmount_all_devices(mountlist);
	mvaddstr_and_log_it(g_currentY,
						0,
						"Using e2label to label your ext2,3 partitions");

	sprintf(tmp, "label-partitions-as-necessary %s < /tmp/fstab",
			g_mountlist_fname);
	res = run_program_and_log_output(tmp, TRUE);
	if (res) {
		log_to_screen("label-partitions-as-necessary returned an error");
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	retval += res;

  after_the_nuke:
	if (retval) {
		log_to_screen("Errors occurred during the nuke phase.");
	} else if (strstr(call_program_and_get_last_line_of_output("cat /proc/cmdline"), "RESTORE"))	// Bruno's thing
	{
		log_to_screen
			("PC was restored successfully. Thank you for using Mondo Rescue.");
		log_to_screen
			("Please visit http://www.mondorescue.org and thank the dev team.");
	} else {
#ifdef FREELOADER
		success_message();
#else
		log_to_screen("PC was restored successfully!");
#endif
	}
	g_I_have_just_nuked = TRUE;
/*
  if (!boot_loader_installed && !does_file_exist(DO_MBR_PLEASE))
    {
      log_to_screen("PLEASE RUN 'mondorestore --mbr' NOW TO INITIALIZE YOUR BOOT SECTOR");
      write_one_liner_data_file(DO_MBR_PLEASE, "mondorestore --mbr");
    }
*/
	return (retval);
}

/**************************************************************************
 *END_NUKE_MODE                                                           *
 **************************************************************************/



/**
 * Restore the user's data (or a subset of it) to the live filesystem.
 * This should not be called if we're booted from CD!
 * @param bkpinfo The backup information structure. Most fields are used.
 * @return 0 for success, or the number of errors encountered.
 */
int restore_to_live_filesystem(struct s_bkpinfo *bkpinfo)
{
	int retval = 0;

  /** malloc **/
	char *old_restpath;

	struct mountlist_itself *mountlist;
//  static
	struct raidlist_itself *raidlist;
	struct s_node *filelist;

	log_msg(1, "restore_to_live_filesystem() - starting");
	assert(bkpinfo != NULL);
	malloc_string(old_restpath);
	mountlist = malloc(sizeof(struct mountlist_itself));
	raidlist = malloc(sizeof(struct raidlist_itself));
	if (!mountlist || !raidlist) {
		fatal_error("Cannot malloc() mountlist and/or raidlist");
	}

	strcpy(bkpinfo->restore_path, "/");
	if (!g_restoring_live_from_cd) {
		popup_and_OK
			("Please insert tape/CD/boot floppy, then hit 'OK' to continue.");
		sleep(1);
	}
	interactively_obtain_media_parameters_from_user(bkpinfo, FALSE);
	log_msg(2, "bkpinfo->media_device = %s", bkpinfo->media_device);
	if (!bkpinfo->media_device[0]) {
		log_msg(2, "Warning - failed to find media dev");
	}


	log_msg(2, "bkpinfo->isodir = %s", bkpinfo->isodir);

	open_evalcall_form("Thinking...");

	get_cfg_file_from_archive_or_bust(bkpinfo);
	read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
	load_mountlist(mountlist, g_mountlist_fname);	// in case read_cfg_file_into_bkpinfo 

	close_evalcall_form();
	retval = load_mountlist(mountlist, g_mountlist_fname);
	load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
	filelist = process_filelist_and_biggielist(bkpinfo);
	if (filelist) {
		save_filelist(filelist, "/tmp/selected-files.txt");
		strcpy(old_restpath, bkpinfo->restore_path);
		if (popup_and_get_string("Restore path",
								 "Restore files to where? )",
								 bkpinfo->restore_path, MAX_STR_LEN / 4)) {
			iamhere("Restoring everything");
			retval += restore_everything(bkpinfo, filelist);
			free_filelist(filelist);
			strcpy(bkpinfo->restore_path, old_restpath);
		} else {
			free_filelist(filelist);
		}
		strcpy(bkpinfo->restore_path, old_restpath);
	}
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		log_msg(2,
				"I probably don't need to unmount or eject the CD-ROM but I'm doing it anyway.");
	}
	run_program_and_log_output("umount " MNT_CDROM, FALSE);
	if (!bkpinfo->please_dont_eject) {
		eject_device(bkpinfo->media_device);
	}
	paranoid_free(old_restpath);
	free(mountlist);
	free(raidlist);
	return (retval);
}

/**************************************************************************
 *END_RESTORE_TO_LIVE_FILESYSTEM                                          *
 **************************************************************************/

/* @} - end of restoreGroup */


#include <utime.h>
/**
 * @addtogroup LLrestoreGroup
 * @{
 */
/**
 * Restore biggiefile @p bigfileno from the currently mounted CD.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->restore_path
 * @param bigfileno The biggiefile number (starting from 0) to restore.
 * @param filelist The node structure containing the list of files to restore.
 * If the biggiefile is not in this list, it will be skipped (return value will
 * still indicate success).
 * @return 0 for success (or skip), nonzero for failure.
 */
int
restore_a_biggiefile_from_CD(struct s_bkpinfo *bkpinfo,
							 long bigfileno,
							 struct s_node *filelist,
							 char *pathname_of_last_file_restored)
{
	FILE *fin;
	FILE *fout;
	FILE *fbzip2;

  /** malloc ***/
	char *checksum, *outfile_fname, *tmp, *bzip2_command,
		*ntfsprog_command, *suffix, *sz_devfile;
	char *bigblk;
	char *p;
	int retval = 0;
	int finished = FALSE;
	long sliceno;
	long siz;
	char ntfsprog_fifo[MAX_STR_LEN];
	char *file_to_openout = NULL;
	struct s_filename_and_lstat_info biggiestruct;
	struct utimbuf the_utime_buf, *ubuf;
	bool use_ntfsprog_hack = FALSE;
	pid_t pid;
	int res = 0;
	int old_loglevel;
	char sz_msg[MAX_STR_LEN];
	struct s_node *node;

	old_loglevel = g_loglevel;
	ubuf = &the_utime_buf;
	assert(bkpinfo != NULL);

	malloc_string(checksum);
	malloc_string(outfile_fname);
	malloc_string(tmp);
	malloc_string(bzip2_command);
	malloc_string(ntfsprog_command);
	malloc_string(suffix);
	malloc_string(sz_devfile);

	pathname_of_last_file_restored[0] = '\0';
	if (!(bigblk = malloc(TAPE_BLOCK_SIZE))) {
		fatal_error("Cannot malloc bigblk");
	}

	if (!(fin = fopen(slice_fname(bigfileno, 0, ARCHIVES_PATH, ""), "r"))) {
		log_to_screen("Cannot even open bigfile's info file");
		return (1);
	}

	memset((void *) &biggiestruct, 0, sizeof(biggiestruct));
	if (fread((void *) &biggiestruct, 1, sizeof(biggiestruct), fin) <
		sizeof(biggiestruct)) {
		log_msg(2, "Warning - unable to get biggiestruct of bigfile #%d",
				bigfileno + 1);
	}
	paranoid_fclose(fin);

	strcpy(checksum, biggiestruct.checksum);

	if (!checksum[0]) {
		sprintf(tmp, "Warning - bigfile %ld does not have a checksum",
				bigfileno + 1);
		log_msg(3, tmp);
		p = checksum;
	}

	if (!strncmp(biggiestruct.filename, "/dev/", 5))	// Whether NTFS or not :)
	{
		strcpy(outfile_fname, biggiestruct.filename);
	} else {
		sprintf(outfile_fname, "%s/%s", bkpinfo->restore_path,
				biggiestruct.filename);
	}

	/* skip file if we have a selective restore subset & it doesn't match */
	if (filelist != NULL) {
		node = find_string_at_node(filelist, biggiestruct.filename);
		if (!node) {
			log_msg(0, "Skipping %s (name isn't in filelist)",
					biggiestruct.filename);
			pathname_of_last_file_restored[0] = '\0';
			return (0);
		} else if (!(node->selected)) {
			log_msg(1, "Skipping %s (name isn't in biggielist subset)",
					biggiestruct.filename);
			pathname_of_last_file_restored[0] = '\0';
			return (0);
		}
	}
	/* otherwise, continue */

	log_msg(1, "DEFINITELY restoring %s", biggiestruct.filename);
	if (biggiestruct.use_ntfsprog) {
		if (strncmp(biggiestruct.filename, "/dev/", 5)) {
			log_msg(1,
					"I was in error when I set biggiestruct.use_ntfsprog to TRUE.");
			log_msg(1, "%s isn't even in /dev", biggiestruct.filename);
			biggiestruct.use_ntfsprog = FALSE;
		}
	}

	if (biggiestruct.use_ntfsprog)	// if it's an NTFS device
//  if (!strncmp ( biggiestruct.filename, "/dev/", 5))
	{
		g_loglevel = 4;
		use_ntfsprog_hack = TRUE;
		log_msg(2,
				"Calling ntfsclone in background because %s is an NTFS /dev entry",
				outfile_fname);
		sprintf(sz_devfile, "/tmp/%d.%d.000", (int) (random() % 32768),
				(int) (random() % 32768));
		mkfifo(sz_devfile, 0x770);
		strcpy(ntfsprog_fifo, sz_devfile);
		file_to_openout = ntfsprog_fifo;
		switch (pid = fork()) {
		case -1:
			fatal_error("Fork failure");
		case 0:
			log_msg(3,
					"CHILD - fip - calling feed_outfrom_ntfsprog(%s, %s)",
					biggiestruct.filename, ntfsprog_fifo);
			res =
				feed_outfrom_ntfsprog(biggiestruct.filename,
									   ntfsprog_fifo);
//          log_msg(3, "CHILD - fip - exiting");
			exit(res);
			break;
		default:
			log_msg(3,
					"feed_into_ntfsprog() called in background --- pid=%ld",
					(long int) (pid));
		}
	} else {
		use_ntfsprog_hack = FALSE;
		ntfsprog_fifo[0] = '\0';
		file_to_openout = outfile_fname;
		if (!does_file_exist(outfile_fname))	// yes, it looks weird with the '!' but it's correct that way
		{
			make_hole_for_file(outfile_fname);
		}
	}

	sprintf(tmp, "Reassembling big file %ld (%s)", bigfileno + 1,
			outfile_fname);
	log_msg(2, tmp);

	/*
	   last slice is zero-length and uncompressed; when we find it, we stop.
	   We DON'T wait until there are no more slices; if we did that,
	   We might stop at end of CD, not at last slice (which is 0-len and uncompd)
	 */

	strncpy(pathname_of_last_file_restored, biggiestruct.filename,
			MAX_STR_LEN - 1);
	pathname_of_last_file_restored[MAX_STR_LEN - 1] = '\0';

	log_msg(3, "file_to_openout = %s", file_to_openout);
	if (!(fout = fopen(file_to_openout, "w"))) {
		log_to_screen("Cannot openout outfile_fname - hard disk full?");
		return (1);
	}
	log_msg(3, "Opened out to %s", outfile_fname);	// CD/DVD --> mondorestore --> ntfsclone --> hard disk itself

	for (sliceno = 1, finished = FALSE; !finished;) {
		if (!does_file_exist
			(slice_fname(bigfileno, sliceno, ARCHIVES_PATH, ""))
			&&
			!does_file_exist(slice_fname
							 (bigfileno, sliceno, ARCHIVES_PATH, "lzo"))
			&&
			!does_file_exist(slice_fname
							 (bigfileno, sliceno, ARCHIVES_PATH, "bz2"))) {
			log_msg(3,
					"Cannot find a data slice or terminator slice on CD %d",
					g_current_media_number);
			g_current_media_number++;
			sprintf(tmp,
					"Asking for %s #%d so that I may read slice #%ld\n",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number, sliceno);
			log_msg(2, tmp);
			sprintf(tmp, "Restoring from %s #%d",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_to_screen(tmp);
			insist_on_this_cd_number(bkpinfo, g_current_media_number);
			log_to_screen("Continuing to restore.");
		} else {
			strcpy(tmp,
				   slice_fname(bigfileno, sliceno, ARCHIVES_PATH, ""));
			if (does_file_exist(tmp) && length_of_file(tmp) == 0) {
				log_msg(2,
						"End of bigfile # %ld (slice %ld is the terminator)",
						bigfileno + 1, sliceno);
				finished = TRUE;
				continue;
			} else {
				if (does_file_exist
					(slice_fname
					 (bigfileno, sliceno, ARCHIVES_PATH, "lzo"))) {
					strcpy(bzip2_command, "lzop");
					strcpy(suffix, "lzo");
				} else
					if (does_file_exist
						(slice_fname
						 (bigfileno, sliceno, ARCHIVES_PATH, "bz2"))) {
					strcpy(bzip2_command, "bzip2");
					strcpy(suffix, "bz2");
				} else
					if (does_file_exist
						(slice_fname
						 (bigfileno, sliceno, ARCHIVES_PATH, ""))) {
					strcpy(bzip2_command, "");
					strcpy(suffix, "");
				} else {
					log_to_screen("OK, that's pretty fsck0red...");
					return (1);
				}
			}
			if (bzip2_command[0] != '\0') {
				sprintf(bzip2_command + strlen(bzip2_command),
						" -dc %s 2>> %s",
						slice_fname(bigfileno, sliceno, ARCHIVES_PATH,
									suffix), MONDO_LOGFILE);
			} else {
				sprintf(bzip2_command, "cat %s 2>> %s",
						slice_fname(bigfileno, sliceno, ARCHIVES_PATH,
									suffix), MONDO_LOGFILE);
			}
			sprintf(tmp, "Working on %s #%d, file #%ld, slice #%ld    ",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number, bigfileno + 1, sliceno);
			log_msg(2, tmp);

			if (!g_text_mode) {
				newtDrawRootText(0, g_noof_rows - 2, tmp);
				newtRefresh();
				strip_spaces(tmp);
				update_progress_form(tmp);
			}
			if (!(fbzip2 = popen(bzip2_command, "r"))) {
				fatal_error("Can't run popen command");
			}
			while (!feof(fbzip2)) {
				siz = fread(bigblk, 1, TAPE_BLOCK_SIZE, fbzip2);
				if (siz > 0) {
					sprintf(sz_msg, "Read %ld from fbzip2", siz);
					siz = fwrite(bigblk, 1, siz, fout);
					sprintf(sz_msg + strlen(sz_msg),
							"; written %ld to fout", siz);
//        log_msg(2. sz_msg);
				}
			}
			paranoid_pclose(fbzip2);


			sliceno++;
			g_current_progress++;
		}
	}
/*
  memset(bigblk, TAPE_BLOCK_SIZE, 1); // This all looks very fishy...
  fwrite( bigblk, 1, TAPE_BLOCK_SIZE, fout);
  fwrite( bigblk, 1, TAPE_BLOCK_SIZE, fout);
  fwrite( bigblk, 1, TAPE_BLOCK_SIZE, fout);
  fwrite( bigblk, 1, TAPE_BLOCK_SIZE, fout);
*/
	paranoid_fclose(fout);
	g_loglevel = old_loglevel;

	if (use_ntfsprog_hack) {
		log_msg(3, "Waiting for ntfsclone to finish");
		sprintf(tmp,
				" ps ax | grep \" ntfsclone \" | grep -v grep > /dev/null 2> /dev/null");
		while (system(tmp) == 0) {
			sleep(1);
		}
		log_it("OK, ntfsclone has really finished");
	}

	if (strcmp(outfile_fname, "/dev/null")) {
		chown(outfile_fname, biggiestruct.properties.st_uid,
			  biggiestruct.properties.st_gid);
		chmod(outfile_fname, biggiestruct.properties.st_mode);
		ubuf->actime = biggiestruct.properties.st_atime;
		ubuf->modtime = biggiestruct.properties.st_mtime;
		utime(outfile_fname, ubuf);
	}
	paranoid_free(bigblk);
	paranoid_free(checksum);
	paranoid_free(outfile_fname);
	paranoid_free(tmp);
	paranoid_free(bzip2_command);
	paranoid_free(ntfsprog_command);
	paranoid_free(suffix);
	paranoid_free(sz_devfile);

	return (retval);
}

/**************************************************************************
 *END_ RESTORE_A_BIGGIEFILE_FROM_CD                                       *
 **************************************************************************/



/**
 * Restore a biggiefile from the currently opened stream.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->restore_path
 * - @c bkpinfo->zip_exe
 * @param orig_bf_fname The original filename of the biggiefile.
 * @param biggiefile_number The number of the biggiefile (starting from 0).
 * @param orig_checksum Unused.
 * @param biggiefile_size Unused.
 * @param filelist The node structure containing the list of files to be restored.
 * If @p orig_bf_fname is not in the list, it will be ignored.
 * @return 0 for success (or skip), nonzero for failure.
 * @bug orig_checksum and biggiefile_size are unused (except to check that they are non-NULL).
 */
int restore_a_biggiefile_from_stream(struct s_bkpinfo *bkpinfo, char *orig_bf_fname, long biggiefile_number, char *orig_checksum,	//UNUSED
									 long long biggiefile_size,	//UNUSED
									 struct s_node *filelist,
									 int use_ntfsprog,
									 char *pathname_of_last_file_restored)
{
	FILE *pout;
	FILE *fin;

  /** mallocs ********/
	char *tmp;
	char *command;
	char *outfile_fname;
	char *ntfsprog_command;
	char *sz_devfile;
	char *ntfsprog_fifo;
	char *file_to_openout = NULL;

	struct s_node *node;

	int old_loglevel;
	long current_slice_number = 0;
	int retval = 0;
	int res = 0;
	int ctrl_chr = '\0';
	long long slice_siz;
	bool dummy_restore = FALSE;
	bool use_ntfsprog_hack = FALSE;
	pid_t pid;
	struct s_filename_and_lstat_info biggiestruct;
	struct utimbuf the_utime_buf, *ubuf;
	ubuf = &the_utime_buf;

	malloc_string(tmp);
	malloc_string(ntfsprog_fifo);
	malloc_string(outfile_fname);
	malloc_string(command);
	malloc_string(sz_devfile);
	malloc_string(ntfsprog_command);
	old_loglevel = g_loglevel;
	assert(bkpinfo != NULL);
	assert(orig_bf_fname != NULL);
	assert(orig_checksum != NULL);

	pathname_of_last_file_restored[0] = '\0';
	if (use_ntfsprog == BLK_START_A_PIHBIGGIE) {
		use_ntfsprog = 1;
		log_msg(1, "%s --- pih=YES", orig_bf_fname);
	} else if (use_ntfsprog == BLK_START_A_NORMBIGGIE) {
		use_ntfsprog = 0;
		log_msg(1, "%s --- pih=NO", orig_bf_fname);
	} else {
		use_ntfsprog = 0;
		log_msg(1, "%s --- pih=NO (weird marker though)", orig_bf_fname);
	}

	strncpy(pathname_of_last_file_restored, orig_bf_fname,
			MAX_STR_LEN - 1);
	pathname_of_last_file_restored[MAX_STR_LEN - 1] = '\0';

	/* open out to biggiefile to be restored (or /dev/null if biggiefile is not to be restored) */

	if (filelist != NULL) {
		node = find_string_at_node(filelist, orig_bf_fname);
		if (!node) {
			dummy_restore = TRUE;
			log_msg(1,
					"Skipping big file %ld (%s) - not in biggielist subset",
					biggiefile_number + 1, orig_bf_fname);
			pathname_of_last_file_restored[0] = '\0';
		} else if (!(node->selected)) {
			dummy_restore = TRUE;
			log_msg(1, "Skipping %s (name isn't in biggielist subset)",
					orig_bf_fname);
			pathname_of_last_file_restored[0] = '\0';
		}
	}

	if (use_ntfsprog) {
		if (strncmp(orig_bf_fname, "/dev/", 5)) {
			log_msg(1,
					"I was in error when I set use_ntfsprog to TRUE.");
			log_msg(1, "%s isn't even in /dev", orig_bf_fname);
			use_ntfsprog = FALSE;
		}
	}

	if (use_ntfsprog) {
		g_loglevel = 4;
		strcpy(outfile_fname, orig_bf_fname);
		use_ntfsprog_hack = TRUE;
		log_msg(2,
				"Calling ntfsclone in background because %s is a /dev entry",
				outfile_fname);
		sprintf(sz_devfile, "/tmp/%d.%d.000", (int) (random() % 32768),
				(int) (random() % 32768));
		mkfifo(sz_devfile, 0x770);
		strcpy(ntfsprog_fifo, sz_devfile);
		file_to_openout = ntfsprog_fifo;
		switch (pid = fork()) {
		case -1:
			fatal_error("Fork failure");
		case 0:
			log_msg(3,
					"CHILD - fip - calling feed_outfrom_ntfsprog(%s, %s)",
					outfile_fname, ntfsprog_fifo);
			res =
				feed_outfrom_ntfsprog(outfile_fname, ntfsprog_fifo);
//          log_msg(3, "CHILD - fip - exiting");
			exit(res);
			break;
		default:
			log_msg(3,
					"feed_into_ntfsprog() called in background --- pid=%ld",
					(long int) (pid));
		}
	} else {
		if (!strncmp(orig_bf_fname, "/dev/", 5))	// non-NTFS partition
		{
			strcpy(outfile_fname, orig_bf_fname);
		} else					// biggiefile
		{
			sprintf(outfile_fname, "%s/%s", bkpinfo->restore_path,
					orig_bf_fname);
		}
		use_ntfsprog_hack = FALSE;
		ntfsprog_fifo[0] = '\0';
		file_to_openout = outfile_fname;
		if (!does_file_exist(outfile_fname))	// yes, it looks weird with the '!' but it's correct that way
		{
			make_hole_for_file(outfile_fname);
		}
		sprintf(tmp, "Reassembling big file %ld (%s)",
				biggiefile_number + 1, orig_bf_fname);
		log_msg(2, tmp);
	}

	if (dummy_restore) {
		sprintf(outfile_fname, "/dev/null");
	}

	if (!bkpinfo->zip_exe[0]) {
		sprintf(command, "cat > \"%s\"", file_to_openout);
	} else {
		sprintf(command, "%s -dc > \"%s\" 2>> %s", bkpinfo->zip_exe,
				file_to_openout, MONDO_LOGFILE);
	}
	sprintf(tmp, "Pipe command = '%s'", command);
	log_msg(3, tmp);

	/* restore biggiefile, one slice at a time */
	if (!(pout = popen(command, "w"))) {
		fatal_error("Cannot pipe out");
	}
	for (res = read_header_block_from_stream(&slice_siz, tmp, &ctrl_chr);
		 ctrl_chr != BLK_STOP_A_BIGGIE;
		 res = read_header_block_from_stream(&slice_siz, tmp, &ctrl_chr)) {
		if (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		sprintf(tmp, "Working on file #%ld, slice #%ld    ",
				biggiefile_number + 1, current_slice_number);
		log_msg(2, tmp);
		if (!g_text_mode) {
			newtDrawRootText(0, g_noof_rows - 2, tmp);
			newtRefresh();
		}
		strip_spaces(tmp);
		update_progress_form(tmp);
		if (current_slice_number == 0) {
			res =
				read_file_from_stream_to_file(bkpinfo,
											  "/tmp/biggie-blah.txt",
											  slice_siz);
			if (!(fin = fopen("/tmp/biggie-blah.txt", "r"))) {
				log_OS_error("blah blah");
			} else {
				if (fread
					((void *) &biggiestruct, 1, sizeof(biggiestruct),
					 fin) < sizeof(biggiestruct)) {
					log_msg(2,
							"Warning - unable to get biggiestruct of bigfile #%d",
							biggiefile_number + 1);
				}
				paranoid_fclose(fin);
			}
		} else {
			res =
				read_file_from_stream_to_stream(bkpinfo, pout, slice_siz);
		}
		retval += res;
		res = read_header_block_from_stream(&slice_siz, tmp, &ctrl_chr);
		if (ctrl_chr != BLK_STOP_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_STOP_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		current_slice_number++;
		g_current_progress++;
	}
	paranoid_pclose(pout);

	log_msg(1, "pathname_of_last_file_restored is now %s",
			pathname_of_last_file_restored);

	if (use_ntfsprog_hack) {
		log_msg(3, "Waiting for ntfsclone to finish");
		sprintf(tmp,
				" ps ax | grep \" ntfsclone \" | grep -v grep > /dev/null 2> /dev/null");
		while (system(tmp) == 0) {
			sleep(1);
		}
		log_msg(3, "OK, ntfsclone has really finished");
	}

	log_msg(3, "biggiestruct.filename = %s", biggiestruct.filename);
	log_msg(3, "biggiestruct.checksum = %s", biggiestruct.checksum);
	if (strcmp(outfile_fname, "/dev/null")) {
		chmod(outfile_fname, biggiestruct.properties.st_mode);
		chown(outfile_fname, biggiestruct.properties.st_uid,
			  biggiestruct.properties.st_gid);
		ubuf->actime = biggiestruct.properties.st_atime;
		ubuf->modtime = biggiestruct.properties.st_mtime;
		utime(outfile_fname, ubuf);
	}

	paranoid_free(tmp);
	paranoid_free(outfile_fname);
	paranoid_free(command);
	paranoid_free(ntfsprog_command);
	paranoid_free(sz_devfile);
	paranoid_free(ntfsprog_fifo);
	g_loglevel = old_loglevel;
	return (retval);
}

/**************************************************************************
 *END_RESTORE_A_BIGGIEFILE_FROM_STREAM                                    *
 **************************************************************************/



/**
 * Restore @p tarball_fname from CD.
 * @param tarball_fname The filename of the tarball to restore (in /mnt/cdrom).
 * This will be used unmodified.
 * @param current_tarball_number The number (starting from 0) of the fileset
 * we're restoring now.
 * @param filelist The node structure containing the list of files to be
 * restored. If no file in the afioball is in this list, afio will still be
 * called, but nothing will be written.
 * @return 0 for success, nonzero for failure.
 */
int
restore_a_tarball_from_CD(char *tarball_fname,
						  long current_tarball_number,
						  struct s_node *filelist)
{
	int retval = 0;
	int res;
	char *p;

  /** malloc **/
	char *command;
	char *tmp;
	char *filelist_name;
	char *filelist_subset_fname;
	char *executable;
	char *temp_log;
	char screen_message[100];
	long matches = 0;
	bool use_star;
	char *xattr_fname;
	char *acl_fname;
//  char files_to_restore_this_time_fname[MAX_STR_LEN];

	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);
	malloc_string(command);
	malloc_string(tmp);
	malloc_string(filelist_name);
	malloc_string(filelist_subset_fname);
	malloc_string(executable);
	malloc_string(temp_log);
	malloc_string(xattr_fname);
	malloc_string(acl_fname);

	log_msg(5, "Entering");
	filelist_subset_fname[0] = '\0';
	use_star = (strstr(tarball_fname, ".star")) ? TRUE : FALSE;
//  sprintf(files_to_restore_this_time_fname, "/tmp/ftrttf.%d.%d", (int)getpid(), (int)random());
	sprintf(command, "mkdir -p %s/tmp", MNT_RESTORING);
	run_program_and_log_output(command, 9);
	sprintf(temp_log, "/tmp/%d.%d", (int) (random() % 32768),
			(int) (random() % 32768));

	sprintf(filelist_name, MNT_CDROM "/archives/filelist.%ld",
			current_tarball_number);
	if (length_of_file(filelist_name) <= 2) {
		log_msg(2, "There are _zero_ files in filelist '%s'",
				filelist_name);
		log_msg(2,
				"This is a bit silly (ask Hugo to fix mondo-makefilelist, please)");
		log_msg(2,
				"but it's non-critical. It's cosmetic. Don't worry about it.");
		retval = 0;
		goto leave_sub;
	}
	if (count_lines_in_file(filelist_name) <= 0
		|| length_of_file(tarball_fname) <= 0) {
		log_msg(3, "length_of_file(%s) = %ld", tarball_fname,
				length_of_file(tarball_fname));
		sprintf(tmp, "Unable to restore fileset #%ld (CD I/O error)",
				current_tarball_number);
		log_to_screen(tmp);
		retval = 1;
		goto leave_sub;
	}

	if (filelist) {
		sprintf(filelist_subset_fname, "/tmp/filelist-subset-%ld.tmp",
				current_tarball_number);
		if ((matches =
			 save_filelist_entries_in_common(filelist_name, filelist,
											 filelist_subset_fname,
											 use_star))
			<= 0) {
			sprintf(tmp, "Skipping fileset %ld", current_tarball_number);
			log_msg(1, tmp);
		} else {
			log_msg(3, "Saved fileset %ld's subset to %s",
					current_tarball_number, filelist_subset_fname);
		}
		sprintf(screen_message, "Tarball #%ld --- %ld matches",
				current_tarball_number, matches);
		log_to_screen(screen_message);
	} else {
		filelist_subset_fname[0] = '\0';
	}

	if (filelist == NULL || matches > 0) {
		sprintf(xattr_fname, XATTR_LIST_FNAME_RAW_SZ,
				MNT_CDROM "/archives", current_tarball_number);
		sprintf(acl_fname, ACL_LIST_FNAME_RAW_SZ, MNT_CDROM "/archives",
				current_tarball_number);
		if (strstr(tarball_fname, ".bz2")) {
			strcpy(executable, "bzip2");
		} else if (strstr(tarball_fname, ".lzo")) {
			strcpy(executable, "lzop");
		} else {
			executable[0] = '\0';
		}
		if (executable[0]) {
			sprintf(tmp, "which %s > /dev/null 2> /dev/null", executable);
			if (run_program_and_log_output(tmp, FALSE)) {
				log_to_screen
					("(compare_a_tarball) Compression program not found - oh no!");
				paranoid_MR_finish(1);
			}
			strcpy(tmp, executable);
			sprintf(executable, "-P %s -Z", tmp);
		}
#ifdef __FreeBSD__
#define BUFSIZE 512
#else
#define BUFSIZE (1024L*1024L)/TAPE_BLOCK_SIZE
#endif

//      if (strstr(tarball_fname, ".star."))
		if (use_star) {
			sprintf(command,
					"star -x -force-remove -U " STAR_ACL_SZ
					" errctl= file=%s", tarball_fname);
			if (strstr(tarball_fname, ".bz2")) {
				strcat(command, " -bz");
			}
		} else {
			if (filelist_subset_fname[0] != '\0') {
				sprintf(command,
						"afio -i -M 8m -b %ld -c %ld %s -w %s %s",
						TAPE_BLOCK_SIZE,
						BUFSIZE, executable, filelist_subset_fname,
//             files_to_restore_this_time_fname,
						tarball_fname);
			} else {
				sprintf(command,
						"afio -i -b %ld -c %ld -M 8m %s %s",
						TAPE_BLOCK_SIZE,
						BUFSIZE, executable, tarball_fname);
			}
		}
#undef BUFSIZE
		sprintf(command + strlen(command), " 2>> %s >> %s", temp_log,
				temp_log);
		log_msg(1, "command = '%s'", command);
		unlink(temp_log);
		res = system(command);
		if (res) {
			p = strstr(command, "-acl ");
			if (p) {
				p[0] = p[1] = p[2] = p[3] = ' ';
				log_msg(1, "new command = '%s'", command);
				res = system(command);
			}
		}
		if (res && length_of_file(temp_log) < 5) {
			res = 0;
		}

		log_msg(1, "Setting fattr list %s", xattr_fname);
		if (length_of_file(xattr_fname) > 0) {
			res = set_fattr_list(filelist_subset_fname, xattr_fname);
			if (res) {
				log_to_screen
					("Errors occurred while setting extended attributes");
			} else {
				log_msg(1, "I set xattr OK");
			}
			retval += res;
		}
		if (length_of_file(acl_fname) > 0) {
			log_msg(1, "Setting acl list %s", acl_fname);
			res = set_acl_list(filelist_subset_fname, acl_fname);
			if (res) {
				log_to_screen
					("Errors occurred while setting access control lists");
			} else {
				log_msg(1, "I set ACL OK");
			}
			retval += res;
		}
		if (retval) {
			sprintf(command, "cat %s >> %s", temp_log, MONDO_LOGFILE);
			system(command);
			log_msg(2, "Errors occurred while processing fileset #%d",
					current_tarball_number);
		} else {
			log_msg(2, "Fileset #%d processed OK", current_tarball_number);
		}
	}
	if (does_file_exist("/PAUSE")) {
		popup_and_OK
			("Press ENTER to go on. Delete /PAUSE to stop these pauses.");
	}
	unlink(filelist_subset_fname);
	unlink(xattr_fname);
	unlink(acl_fname);
	unlink(temp_log);

  leave_sub:
	paranoid_free(command);
	paranoid_free(tmp);
	paranoid_free(filelist_name);
	paranoid_free(filelist_subset_fname);
	paranoid_free(executable);
	paranoid_free(temp_log);
	paranoid_free(xattr_fname);
	paranoid_free(acl_fname);
	log_msg(5, "Leaving");
	return (retval);
}

/**************************************************************************
 *END_RESTORE_A_TARBALL_FROM_CD                                           *
 **************************************************************************/


/**
 * Restore a tarball from the currently opened stream.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->media_device
 * - @c bkpinfo->zip_exe
 * @param tarball_fname The filename of the afioball to restore.
 * @param current_tarball_number The number (starting from 0) of the fileset
 * we're restoring now.
 * @param filelist The node structure containing the list of files to be
 * restored. If no file in the afioball is in this list, afio will still be
 * called, but nothing will be written.
 * @param size The size (in @b bytes) of the afioball.
 * @return 0 for success, nonzero for failure.
 */
int
restore_a_tarball_from_stream(struct s_bkpinfo *bkpinfo,
							  char *tarball_fname,
							  long current_tarball_number,
							  struct s_node *filelist,
							  long long size, char *xattr_fname,
							  char *acl_fname)
{
	int retval = 0;
	int res = 0;

  /** malloc add ***/
	char *tmp;
	char *command;
	char *afio_fname;
	char *filelist_fname;
	char *filelist_subset_fname;
	char *executable;
	long matches = 0;
	bool restore_this_fileset = FALSE;
	bool use_star;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);
	malloc_string(filelist_subset_fname);
	malloc_string(filelist_fname);
	malloc_string(afio_fname);
	malloc_string(executable);
	malloc_string(command);
	malloc_string(tmp);
	filelist_subset_fname[0] = '\0';
	/* to do it with a file... */
	use_star = (strstr(tarball_fname, ".star")) ? TRUE : FALSE;
	sprintf(tmp,
			"Restoring from fileset #%ld (%ld KB) on %s #%d",
			current_tarball_number, (long) size >> 10,
			media_descriptor_string(bkpinfo->backup_media_type),
			g_current_media_number);
	log_msg(2, tmp);
	run_program_and_log_output("mkdir -p " MNT_RESTORING "/tmp", FALSE);

  /****************************************************************************
   * Use RAMDISK's /tmp; saves time; oh wait, it's too small                  *
   * Well, pipe from tape to afio, then; oh wait, can't do that either: bug   *
   * in afio or someting; oh darn.. OK, use tmpfs :-)                         *
   ****************************************************************************/
	filelist_fname[0] = filelist_subset_fname[0] = '\0';
	sprintf(afio_fname, "/tmp/tmpfs/archive.tmp.%ld",
			current_tarball_number);
	sprintf(filelist_fname, "%s/filelist.%ld", bkpinfo->tmpdir,
			current_tarball_number);
	sprintf(filelist_subset_fname, "%s/filelist-subset-%ld.tmp",
			bkpinfo->tmpdir, current_tarball_number);
//  sprintf(filelist_fname, "/tmp/tmpfs/temp-filelist.%ld", current_tarball_number);
	res = read_file_from_stream_to_file(bkpinfo, afio_fname, size);
	if (strstr(tarball_fname, ".star")) {
		bkpinfo->use_star = TRUE;
	}
	if (res) {
		log_msg(1, "Warning - error reading afioball from tape");
	}
	if (bkpinfo->compression_level == 0) {
		executable[0] = '\0';
	} else {
		if (bkpinfo->use_star) {
			strcpy(executable, " -bz");
		} else {
			sprintf(executable, "-P %s -Z", bkpinfo->zip_exe);
		}
	}

	if (!filelist)				// if unconditional restore then restore entire fileset
	{
		restore_this_fileset = TRUE;
	} else						// If restoring selectively then get TOC from tarball
	{
		if (strstr(tarball_fname, ".star.")) {
			use_star = TRUE;
			sprintf(command, "star -t file=%s %s", afio_fname, executable);
		} else {
			use_star = FALSE;
			sprintf(command, "afio -t -M 8m -b %ld %s %s", TAPE_BLOCK_SIZE,
					executable, afio_fname);
		}
		sprintf(command + strlen(command), " > %s 2>> %s", filelist_fname,
				MONDO_LOGFILE);
		log_msg(1, "command = %s", command);
		if (system(command)) {
			log_msg(4, "Warning - error occurred while retrieving TOC");
		}
		if ((matches =
			 save_filelist_entries_in_common(filelist_fname, filelist,
											 filelist_subset_fname,
											 use_star))
			<= 0 || length_of_file(filelist_subset_fname) < 2) {
			if (length_of_file(filelist_subset_fname) < 2) {
				log_msg(1, "No matches found in fileset %ld",
						current_tarball_number);
			}
			sprintf(tmp, "Skipping fileset %ld", current_tarball_number);
			log_msg(2, tmp);
			restore_this_fileset = FALSE;
		} else {
			log_msg(5, "%ld matches. Saved fileset %ld's subset to %s",
					matches, current_tarball_number,
					filelist_subset_fname);
			restore_this_fileset = TRUE;
		}
	}

// Concoct the call to star/afio to restore files
	if (strstr(tarball_fname, ".star."))	// star
	{
		sprintf(command, "star -x file=%s %s", afio_fname, executable);
		if (filelist) {
			sprintf(command + strlen(command), " list=%s",
					filelist_subset_fname);
		}
	} else						// afio
	{
		sprintf(command, "afio -i -M 8m -b %ld %s", TAPE_BLOCK_SIZE,
				executable);
		if (filelist) {
			sprintf(command + strlen(command), " -w %s",
					filelist_subset_fname);
		}
		sprintf(command + strlen(command), " %s", afio_fname);
	}
	sprintf(command + strlen(command), " 2>> %s", MONDO_LOGFILE);

// Call if IF there are files to restore (selectively/unconditionally)
	if (restore_this_fileset) {
		log_msg(1, "Calling command='%s'", command);
		paranoid_system(command);

		iamhere("Restoring xattr, acl stuff");
		res = set_fattr_list(filelist_subset_fname, xattr_fname);
		if (res) {
			log_msg(1, "Errors occurred while setting xattr");
		} else {
			log_msg(1, "I set xattr OK");
		}
		retval += res;

		res = set_acl_list(filelist_subset_fname, acl_fname);
		if (res) {
			log_msg(1, "Errors occurred while setting ACL");
		} else {
			log_msg(1, "I set ACL OK");
		}
		retval += res;

	} else {
		log_msg(1, "NOT CALLING '%s'", command);
	}

	if (does_file_exist("/PAUSE") && current_tarball_number >= 50) {
		log_to_screen("Paused after set %ld", current_tarball_number);
		popup_and_OK("Pausing. Press ENTER to continue.");
	}

	unlink(filelist_subset_fname);
	unlink(filelist_fname);
	unlink(afio_fname);

	paranoid_free(filelist_subset_fname);
	paranoid_free(filelist_fname);
	paranoid_free(afio_fname);
	paranoid_free(command);
	paranoid_free(tmp);
	return (retval);
}

/**************************************************************************
 *END_RESTORE_A_TARBALL_FROM_STREAM                                       *
 **************************************************************************/




/**
 * Restore all biggiefiles from all media in this CD backup.
 * The CD with the last afioball should be currently mounted.
 * @param bkpinfo The backup information structure. @c backup_media_type is the
 * only field used in this function.
 * @param filelist The node structure containing the list of files to be
 * restored. If a prospective biggiefile is not in this list, it will be ignored.
 * @return 0 for success, nonzero for failure.
 */
int
restore_all_biggiefiles_from_CD(struct s_bkpinfo *bkpinfo,
								struct s_node *filelist)
{
	int retval = 0;
	int res;
	long noof_biggiefiles, bigfileno = 0, total_slices;
  /** malloc **/
	char *tmp;
	bool just_changed_cds = FALSE, finished;
	char *xattr_fname;
	char *acl_fname;
	char *biggies_whose_EXATs_we_should_set;	// EXtended ATtributes
	char *pathname_of_last_biggie_restored;
	FILE *fbw = NULL;

	malloc_string(xattr_fname);
	malloc_string(acl_fname);
	malloc_string(tmp);
	malloc_string(biggies_whose_EXATs_we_should_set);
	malloc_string(pathname_of_last_biggie_restored);
	assert(bkpinfo != NULL);

	sprintf(biggies_whose_EXATs_we_should_set,
			"%s/biggies-whose-EXATs-we-should-set", bkpinfo->tmpdir);
	if (!(fbw = fopen(biggies_whose_EXATs_we_should_set, "w"))) {
		log_msg(1, "Warning - cannot openout %s",
				biggies_whose_EXATs_we_should_set);
	}

	read_cfg_var(g_mondo_cfg_file, "total-slices", tmp);
	total_slices = atol(tmp);
	sprintf(tmp, "Reassembling large files      ");
	mvaddstr_and_log_it(g_currentY, 0, tmp);
	if (length_of_file(BIGGIELIST) < 6) {
		log_msg(1, "OK, no biggielist; not restoring biggiefiles");
		return (0);
	}
	noof_biggiefiles = count_lines_in_file(BIGGIELIST);
	if (noof_biggiefiles <= 0) {
		log_msg(2,
				"OK, no biggiefiles in biggielist; not restoring biggiefiles");
		return (0);
	}
	sprintf(tmp, "OK, there are %ld biggiefiles in the archives",
			noof_biggiefiles);
	log_msg(2, tmp);

	open_progress_form("Reassembling large files",
					   "I am now reassembling all the large files.",
					   "Please wait. This may take some time.",
					   "", total_slices);
	for (bigfileno = 0, finished = FALSE; !finished;) {
		log_msg(2, "Thinking about restoring bigfile %ld", bigfileno + 1);
		if (!does_file_exist(slice_fname(bigfileno, 0, ARCHIVES_PATH, ""))) {
			log_msg(3,
					"...but its first slice isn't on this CD. Perhaps this was a selective restore?");
			log_msg(3, "Cannot find bigfile #%ld 's first slice on %s #%d",
					bigfileno + 1,
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_msg(3, "Slicename would have been %s",
					slice_fname(bigfileno + 1, 0, ARCHIVES_PATH, ""));
			// I'm not positive 'just_changed_cds' is even necessary...
			if (just_changed_cds) {
				just_changed_cds = FALSE;
				log_msg(3,
						"I'll continue to scan this CD for bigfiles to be restored.");
			} else if (does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST")) {
				insist_on_this_cd_number(bkpinfo,
										 ++g_current_media_number);
				sprintf(tmp, "Restoring from %s #%d",
						media_descriptor_string(bkpinfo->
												backup_media_type),
						g_current_media_number);
				log_to_screen(tmp);
				just_changed_cds = TRUE;
			} else {
				log_msg(2, "There was no bigfile #%ld. That's OK.",
						bigfileno + 1);
				log_msg(2, "I'm going to stop restoring bigfiles now.");
				finished = TRUE;
			}
		} else {
			just_changed_cds = FALSE;
			sprintf(tmp, "Restoring big file %ld", bigfileno + 1);
			update_progress_form(tmp);
			res =
				restore_a_biggiefile_from_CD(bkpinfo, bigfileno, filelist,
											 pathname_of_last_biggie_restored);
			iamhere(pathname_of_last_biggie_restored);
			if (fbw && pathname_of_last_biggie_restored[0]) {
				fprintf(fbw, "%s\n", pathname_of_last_biggie_restored);
			}
			retval += res;
			bigfileno++;

		}
	}

	if (fbw) {
		fclose(fbw);
		sprintf(acl_fname, ACL_BIGGLST_FNAME_RAW_SZ, ARCHIVES_PATH);
		sprintf(xattr_fname, XATTR_BIGGLST_FNAME_RAW_SZ, ARCHIVES_PATH);
		if (length_of_file(acl_fname) > 0 && find_home_of_exe("setfacl")) {
			set_acl_list(biggies_whose_EXATs_we_should_set, acl_fname);
		}
		if (length_of_file(xattr_fname) > 0
			&& find_home_of_exe("setfattr")) {
			set_fattr_list(biggies_whose_EXATs_we_should_set, xattr_fname);
		}
	}
	if (does_file_exist("/PAUSE")) {
		popup_and_OK
			("Press ENTER to go on. Delete /PAUSE to stop these pauses.");
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(xattr_fname);
	paranoid_free(acl_fname);
	paranoid_free(tmp);
	paranoid_free(biggies_whose_EXATs_we_should_set);
	paranoid_free(pathname_of_last_biggie_restored);
	return (retval);
}

/**************************************************************************
 *END_RESTORE_ALL_BIGGIFILES_FROM_CD                                      *
 **************************************************************************/



/**
 * Restore all afioballs from all CDs in the backup.
 * The first CD should be inserted (if not, it will be asked for).
 * @param bkpinfo The backup information structure. @c backup_media_type is the
 * only field used in @e this function.
 * @param filelist The node structure containing the list of files to be
 * restored. If no file in some particular afioball is in this list, afio will
 * still be called for that fileset, but nothing will be written.
 * @return 0 for success, or the number of filesets that failed.
 */
int
restore_all_tarballs_from_CD(struct s_bkpinfo *bkpinfo,
							 struct s_node *filelist)
{
	int retval = 0;
	int res;
	int attempts;
	long current_tarball_number = 0;
	long max_val;
  /**malloc ***/
	char *tmp;
	char *tarball_fname;
	char *progress_str;
	char *comment;

	malloc_string(tmp);
	malloc_string(tarball_fname);
	malloc_string(progress_str);
	malloc_string(comment);

	assert(bkpinfo != NULL);

	mvaddstr_and_log_it(g_currentY, 0, "Restoring from archives");
	log_msg(2,
			"Insisting on 1st CD, so that I can have a look at LAST-FILELIST-NUMBER");
	if (g_current_media_number != 1) {
		log_msg(3, "OK, that's jacked up.");
		g_current_media_number = 1;
	}
	insist_on_this_cd_number(bkpinfo, g_current_media_number);
	read_cfg_var(g_mondo_cfg_file, "last-filelist-number", tmp);
	max_val = atol(tmp) + 1;
	sprintf(progress_str, "Restoring from %s #%d",
			media_descriptor_string(bkpinfo->backup_media_type),
			g_current_media_number);
	log_to_screen(progress_str);
	open_progress_form("Restoring from archives",
					   "Restoring data from the archives.",
					   "Please wait. This may take some time.",
					   progress_str, max_val);
	for (;;) {
		insist_on_this_cd_number(bkpinfo, g_current_media_number);
		update_progress_form(progress_str);
		sprintf(tarball_fname, MNT_CDROM "/archives/%ld.afio.bz2",
				current_tarball_number);
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%ld.afio.lzo",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%ld.afio.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%ld.star.bz2",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%ld.star.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			if (current_tarball_number == 0) {
				log_to_screen
					("No tarballs. Strange. Maybe you only backed up freakin' big files?");
				return (0);
			}
			if (!does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST")
				|| system("find " MNT_CDROM
						  "/archives/slice* > /dev/null 2> /dev/null") ==
				0) {
				break;
			}
			g_current_media_number++;
			sprintf(progress_str, "Restoring from %s #%d",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_to_screen(progress_str);
		} else {
			sprintf(progress_str, "Restoring from fileset #%ld on %s #%d",
					current_tarball_number,
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
//    log_msg(3, "progress_str = %s", progress_str);
			for (res = 999, attempts = 0; attempts < 3 && res != 0;
				 attempts++) {
				res =
					restore_a_tarball_from_CD(tarball_fname,
											  current_tarball_number,
											  filelist);
			}
			sprintf(tmp, "%s #%d, fileset #%ld - restore ",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number, current_tarball_number);
			if (res) {
				strcat(tmp, "reported errors");
			} else if (attempts > 1) {
				strcat(tmp, "succeeded");
			} else {
				strcat(tmp, "succeeded");
			}
			if (attempts > 1) {
				sprintf(tmp + strlen(tmp), " (%d attempts) - review logs",
						attempts);
			}
			strcpy(comment, tmp);
			if (attempts > 1) {
				log_to_screen(comment);
			}

			retval += res;
			current_tarball_number++;
			g_current_progress++;
		}
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(tmp);
	paranoid_free(tarball_fname);
	paranoid_free(progress_str);
	paranoid_free(comment);

	return (retval);
}

/**************************************************************************
 *END_RESTORE_ALL_TARBALLS_FROM_CD                                        *
 **************************************************************************/



/**
 * Restore all biggiefiles from the currently opened stream.
 * @param bkpinfo The backup information structure. Passed to other functions.
 * @param filelist The node structure containing the list of files to be
 * restored. If a prospective biggiefile is not in the list, it will be ignored.
 * @return 0 for success, or the number of biggiefiles that failed.
 */
int
restore_all_biggiefiles_from_stream(struct s_bkpinfo *bkpinfo,
									struct s_node *filelist)
{
	long noof_biggiefiles;
	long current_bigfile_number = 0;
	long total_slices;

	int retval = 0;
	int res = 0;
	int ctrl_chr;

  /** malloc add ****/
	char *tmp;
	char *biggie_fname;
	char *biggie_cksum;
	char *xattr_fname;
	char *acl_fname;
	char *p;
	char *pathname_of_last_biggie_restored;
	char *biggies_whose_EXATs_we_should_set;	// EXtended ATtributes
	long long biggie_size;
	FILE *fbw = NULL;

	malloc_string(tmp);
	malloc_string(biggie_fname);
	malloc_string(biggie_cksum);
	malloc_string(xattr_fname);
	malloc_string(acl_fname);
	malloc_string(biggies_whose_EXATs_we_should_set);
	malloc_string(pathname_of_last_biggie_restored);
	assert(bkpinfo != NULL);

	read_cfg_var(g_mondo_cfg_file, "total-slices", tmp);

	total_slices = atol(tmp);
	sprintf(tmp, "Reassembling large files      ");
	sprintf(xattr_fname, XATTR_BIGGLST_FNAME_RAW_SZ, bkpinfo->tmpdir);
	sprintf(acl_fname, ACL_BIGGLST_FNAME_RAW_SZ, bkpinfo->tmpdir);
	mvaddstr_and_log_it(g_currentY, 0, tmp);
	sprintf(biggies_whose_EXATs_we_should_set,
			"%s/biggies-whose-EXATs-we-should-set", bkpinfo->tmpdir);
	if (!(fbw = fopen(biggies_whose_EXATs_we_should_set, "w"))) {
		log_msg(1, "Warning - cannot openout %s",
				biggies_whose_EXATs_we_should_set);
	}
// get xattr and acl files if they're there
	res =
		read_header_block_from_stream(&biggie_size, biggie_fname,
									  &ctrl_chr);
	if (ctrl_chr == BLK_START_EXTENDED_ATTRIBUTES) {
		res =
			read_EXAT_files_from_tape(bkpinfo, &biggie_size, biggie_fname,
									  &ctrl_chr, xattr_fname, acl_fname);
	}

	noof_biggiefiles = atol(biggie_fname);
	sprintf(tmp, "OK, there are %ld biggiefiles in the archives",
			noof_biggiefiles);
	log_msg(2, tmp);
	open_progress_form("Reassembling large files",
					   "I am now reassembling all the large files.",
					   "Please wait. This may take some time.",
					   "", total_slices);

	for (res =
		 read_header_block_from_stream(&biggie_size, biggie_fname,
									   &ctrl_chr);
		 ctrl_chr != BLK_STOP_BIGGIEFILES;
		 res =
		 read_header_block_from_stream(&biggie_size, biggie_fname,
									   &ctrl_chr)) {
		if (ctrl_chr != BLK_START_A_NORMBIGGIE
			&& ctrl_chr != BLK_START_A_PIHBIGGIE) {
			wrong_marker(BLK_START_A_NORMBIGGIE, ctrl_chr);
		}
		p = strrchr(biggie_fname, '/');
		if (!p) {
			p = biggie_fname;
		} else {
			p++;
		}
		sprintf(tmp, "Restoring big file %ld (%lld K)",
				current_bigfile_number + 1, biggie_size / 1024);
		update_progress_form(tmp);
		res = restore_a_biggiefile_from_stream(bkpinfo, biggie_fname,
											   current_bigfile_number,
											   biggie_cksum,
											   biggie_size,
											   filelist, ctrl_chr,
											   pathname_of_last_biggie_restored);
		log_msg(1, "I believe I have restored %s",
				pathname_of_last_biggie_restored);
		if (fbw && pathname_of_last_biggie_restored[0]) {
			fprintf(fbw, "%s\n", pathname_of_last_biggie_restored);
		}
		retval += res;
		current_bigfile_number++;

	}
	if (current_bigfile_number != noof_biggiefiles
		&& noof_biggiefiles != 0) {
		sprintf(tmp, "Warning - bigfileno=%ld but noof_biggiefiles=%ld\n",
				current_bigfile_number, noof_biggiefiles);
	} else {
		sprintf(tmp,
				"%ld biggiefiles in biggielist.txt; %ld biggiefiles processed today.",
				noof_biggiefiles, current_bigfile_number);
	}
	log_msg(1, tmp);

	if (fbw) {
		fclose(fbw);
		if (length_of_file(biggies_whose_EXATs_we_should_set) > 2) {
			iamhere("Setting biggie-EXATs");
			if (length_of_file(acl_fname) > 0) {
				log_msg(1, "set_acl_list(%s,%s)",
						biggies_whose_EXATs_we_should_set, acl_fname);
				set_acl_list(biggies_whose_EXATs_we_should_set, acl_fname);
			}
			if (length_of_file(xattr_fname) > 0) {
				log_msg(1, "set_fattr_List(%s,%s)",
						biggies_whose_EXATs_we_should_set, xattr_fname);
				set_fattr_list(biggies_whose_EXATs_we_should_set,
							   xattr_fname);
			}
		} else {
			iamhere
				("No biggiefiles selected. So, no biggie-EXATs to set.");
		}
	}
	if (does_file_exist("/PAUSE")) {
		popup_and_OK
			("Press ENTER to go on. Delete /PAUSE to stop these pauses.");
	}

	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(biggies_whose_EXATs_we_should_set);
	paranoid_free(pathname_of_last_biggie_restored);
	paranoid_free(biggie_fname);
	paranoid_free(biggie_cksum);
	paranoid_free(xattr_fname);
	paranoid_free(acl_fname);
	paranoid_free(tmp);
	return (retval);
}

/**************************************************************************
 *END_RESTORE_ALL_BIGGIEFILES_FROM_STREAM                                 *
 **************************************************************************/






/**
 * Restore all afioballs from the currently opened tape stream.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->restore_path
 * @param filelist The node structure containing the list of files to be
 * restored. If no file in an afioball is in this list, afio will still be
 * called for that fileset, but nothing will be written.
 * @return 0 for success, or the number of filesets that failed.
 */
int
restore_all_tarballs_from_stream(struct s_bkpinfo *bkpinfo,
								 struct s_node *filelist)
{
	int retval = 0;
	int res;
	long current_afioball_number = 0;
	int ctrl_chr;
	long max_val /*, total_noof_files */ ;

  /** malloc **/
	char *tmp;
	char *progress_str;
	char *tmp_fname;
	char *xattr_fname;
	char *acl_fname;

	long long tmp_size;

	malloc_string(tmp);
	malloc_string(progress_str);
	malloc_string(tmp_fname);
	assert(bkpinfo != NULL);
	malloc_string(xattr_fname);
	malloc_string(acl_fname);
	mvaddstr_and_log_it(g_currentY, 0, "Restoring from archives");
	read_cfg_var(g_mondo_cfg_file, "last-filelist-number", tmp);
	max_val = atol(tmp) + 1;

	chdir(bkpinfo->restore_path);	/* I don't know why this is needed _here_ but it seems to be. -HR, 02/04/2002 */

	run_program_and_log_output("pwd", 5);

	sprintf(progress_str, "Restoring from media #%d",
			g_current_media_number);
	log_to_screen(progress_str);
	open_progress_form("Restoring from archives",
					   "Restoring data from the archives.",
					   "Please wait. This may take some time.",
					   progress_str, max_val);

	log_msg(3, "hey");

	res = read_header_block_from_stream(&tmp_size, tmp_fname, &ctrl_chr);
	if (res) {
		log_msg(2, "Warning - error reading afioball from tape");
	}
	retval += res;
	if (ctrl_chr != BLK_START_AFIOBALLS) {
		wrong_marker(BLK_START_AFIOBALLS, ctrl_chr);
	}
	log_msg(2, "ho");
	res = read_header_block_from_stream(&tmp_size, tmp_fname, &ctrl_chr);
	while (ctrl_chr != BLK_STOP_AFIOBALLS) {
		update_progress_form(progress_str);
		sprintf(xattr_fname, "%s/xattr-subset-%ld.tmp", bkpinfo->tmpdir,
				current_afioball_number);
		sprintf(acl_fname, "%s/acl-subset-%ld.tmp", bkpinfo->tmpdir,
				current_afioball_number);
		unlink(xattr_fname);
		unlink(acl_fname);
		if (ctrl_chr == BLK_START_EXTENDED_ATTRIBUTES) {
			iamhere("Reading EXAT files from tape");
			res =
				read_EXAT_files_from_tape(bkpinfo, &tmp_size, tmp_fname,
										  &ctrl_chr, xattr_fname,
										  acl_fname);
		}
		if (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		sprintf(tmp,
				"Restoring from fileset #%ld (name=%s, size=%ld K)",
				current_afioball_number, tmp_fname, (long) tmp_size >> 10);
		res =
			restore_a_tarball_from_stream(bkpinfo, tmp_fname,
										  current_afioball_number,
										  filelist, tmp_size, xattr_fname,
										  acl_fname);
		retval += res;
		if (res) {
			sprintf(tmp, "Fileset %ld - errors occurred",
					current_afioball_number);
			log_to_screen(tmp);
		}
		res =
			read_header_block_from_stream(&tmp_size, tmp_fname, &ctrl_chr);
		if (ctrl_chr != BLK_STOP_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_STOP_AN_AFIO_OR_SLICE, ctrl_chr);
		}

		current_afioball_number++;
		g_current_progress++;
		sprintf(progress_str, "Restoring from fileset #%ld on %s #%d",
				current_afioball_number,
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
		res =
			read_header_block_from_stream(&tmp_size, tmp_fname, &ctrl_chr);
		unlink(xattr_fname);
		unlink(acl_fname);
	}							// next
	log_msg(1, "All done with afioballs");
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(tmp);
	paranoid_free(progress_str);
	paranoid_free(tmp_fname);
	paranoid_free(xattr_fname);
	paranoid_free(acl_fname);
	return (retval);
}

/**************************************************************************
 *END_ RESTORE_ALL_TARBALLS_FROM_STREAM                                   *
 **************************************************************************/

/* @} - end of LLrestoreGroup */


/**
 * Restore all files in @p filelist.
 * @param bkpinfo The backup information structure. Most fields are used.
 * @param filelist The node structure containing the list of files to be
 * restored.
 * @return 0 for success, or the number of afioballs and biggiefiles that failed.
 * @ingroup restoreGroup
 */
int restore_everything(struct s_bkpinfo *bkpinfo, struct s_node *filelist)
{
	int resA;
	int resB;

  /** mallco ***/
	char *cwd;
	char *newpath;
	char *tmp;
	assert(bkpinfo != NULL);

	malloc_string(cwd);
	malloc_string(newpath);
	malloc_string(tmp);
	log_msg(2, "restore_everything() --- starting");
	g_current_media_number = 1;
	getcwd(cwd, MAX_STR_LEN - 1);
	sprintf(tmp, "mkdir -p %s", bkpinfo->restore_path);
	run_program_and_log_output(tmp, FALSE);
	log_msg(1, "Changing dir to %s", bkpinfo->restore_path);
	chdir(bkpinfo->restore_path);
	getcwd(newpath, MAX_STR_LEN - 1);
	log_msg(1, "path is now %s", newpath);
	log_msg(1, "restoring everything");
	if (!find_home_of_exe("petris") && !g_text_mode) {
		newtDrawRootText(0, g_noof_rows - 2,
						 "Press ALT-<left cursor> twice to play Petris :-) ");
		newtRefresh();
	}
	mvaddstr_and_log_it(g_currentY, 0, "Preparing to read your archives");
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		mount_cdrom(bkpinfo);
		mvaddstr_and_log_it(g_currentY++, 0,
							"Restoring OS and data from streaming media");
		if (bkpinfo->backup_media_type == cdstream) {
			openin_cdstream(bkpinfo);
		} else {
			assert_string_is_neither_NULL_nor_zerolength(bkpinfo->
														 media_device);
			openin_tape(bkpinfo);
		}
		resA = restore_all_tarballs_from_stream(bkpinfo, filelist);
		resB = restore_all_biggiefiles_from_stream(bkpinfo, filelist);
		if (bkpinfo->backup_media_type == cdstream) {
			closein_cdstream(bkpinfo);
		} else {
			closein_tape(bkpinfo);
		}
	} else {
		mvaddstr_and_log_it(g_currentY++, 0,
							"Restoring OS and data from CD       ");
		mount_cdrom(bkpinfo);
		resA = restore_all_tarballs_from_CD(bkpinfo, filelist);
		resB = restore_all_biggiefiles_from_CD(bkpinfo, filelist);
	}
	chdir(cwd);
	if (resA + resB) {
		log_to_screen("Errors occurred while data was being restored.");
	}
	if (length_of_file("/etc/raidtab") > 0) {
		log_msg(2, "Copying local raidtab to restored filesystem");
		run_program_and_log_output("cp -f /etc/raidtab " MNT_RESTORING
								   "/etc/raidtab", FALSE);
	}
	kill_petris();
	log_msg(2, "restore_everything() --- leaving");
	paranoid_free(cwd);
	paranoid_free(newpath);
	paranoid_free(tmp);
	return (resA + resB);
}

/**************************************************************************
 *END_RESTORE_EVERYTHING                                                  *
 **************************************************************************/



/**
 * @brief Haha. You wish! (This function is not implemented :-)
 */
int
restore_live_from_monitas_server(struct s_bkpinfo *bkpinfo,
								 char *monitas_device,
								 char *restore_this_directory,
								 char *restore_here)
	 /* NB: bkpinfo hasn't been populated yet, except for ->tmp which is "/tmp" */
{
	FILE *fout;
	int retval = 0;
	int i;
	int j;
	struct mountlist_itself the_mountlist;
	static struct raidlist_itself the_raidlist;
  /** malloc **/
	char tmp[MAX_STR_LEN + 1];
	char command[MAX_STR_LEN + 1];
	char datablock[256 * 1024];
	char datadisks_fname[MAX_STR_LEN + 1];
	long k;
	long length;
	long long llt;
	struct s_node *filelist = NULL;
	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(monitas_device);
	assert(restore_this_directory != NULL);
	assert(restore_here != NULL);

	sprintf(tmp, "restore_here = '%s'", restore_here);

	log_msg(2, tmp);

	log_msg(2, "restore_live_from_monitas_server() - starting");
	unlink("/tmp/mountlist.txt");
	unlink("/tmp/filelist.full");
	unlink("/tmp/biggielist.txt");
	if (restore_here[0] == '\0') {
		strcpy(bkpinfo->restore_path, MNT_RESTORING);
	} else {
		strcpy(bkpinfo->restore_path, restore_here);
	}
	log_msg(3, "FYI FYI FYI FYI FYI FYI FYI FYI FYI FYI FYI");
	sprintf(tmp, "FYI - data will be restored to %s",
			bkpinfo->restore_path);
	log_msg(3, tmp);
	log_msg(3, "FYI FYI FYI FYI FYI FYI FYI FYI FYI FYI FYI");
	sprintf(datadisks_fname, "/tmp/mondorestore.datadisks.%d",
			(int) (random() % 32768));
	chdir(bkpinfo->tmpdir);

	sprintf(command, "cat %s", monitas_device);
	g_tape_stream = popen(command, "r");	// for compatibility with openin_tape()
	if (!(fout = fopen(datadisks_fname, "w"))) {
		log_OS_error(datadisks_fname);
		return (1);
	}
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 4; j++) {
			for (length = k = 0; length < 256 * 1024; length += k) {
				k = fread(datablock + length, 1, 256 * 1024 - length,
						  g_tape_stream);
			}
			fwrite(datablock, 1, length, fout);
			g_tape_posK += length;
		}
	}
	paranoid_fclose(fout);
	sprintf(command,
			"tar -zxvf %s tmp/mondo-restore.cfg tmp/mountlist.txt tmp/filelist.full tmp/biggielist.txt",
			datadisks_fname);
	run_program_and_log_output(command, 4);
	read_header_block_from_stream(&llt, tmp, &i);
	read_header_block_from_stream(&llt, tmp, &i);

	unlink(datadisks_fname);
	read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
	retval = load_mountlist(&the_mountlist, g_mountlist_fname);	// in case read_cfg_file_into_bkpinfo   strcpy(bkpinfo->media_device, monitas_device);


	load_raidtab_into_raidlist(&the_raidlist, RAIDTAB_FNAME);
	iamhere("FIXME");
	fatal_error("This will fail");
	sprintf(command,
			"grep -x \"%s.*\" %s > %s",
			restore_this_directory, g_filelist_full, g_filelist_full);
	if (system(command)) {
		retval++;
		log_to_screen
			("Error(s) occurred while processing filelist and wildcard");
	}
	iamhere("FIXME");
	fatal_error("This will fail");
	sprintf(command,
			"grep -x \"%s.*\" %s > %s",
			restore_this_directory, g_biggielist_txt, g_biggielist_txt);
	if (system(command)) {
		log_msg(1,
				"Error(s) occurred while processing biggielist and wildcard");
	}
	sprintf(command, "touch %s", g_biggielist_txt);
	run_program_and_log_output(command, FALSE);
//  filelist = load_filelist(g_filelist_restthese);  // FIXME --- this probably doesn't work because it doesn't include the biggiefiles
	retval += restore_everything(bkpinfo, filelist);
	free_filelist(filelist);
	log_msg(2, "--------End of restore_live_from_monitas_server--------");
	return (retval);
}

/**************************************************************************
 *END_RESTORE_LIVE_FROM_MONITAS_SERVER                                    *
 **************************************************************************/




extern void wait_until_software_raids_are_prepped(char *, int);


char which_restore_mode(void);


/**
 * Log a "don't panic" message to the logfile.
 */
void welcome_to_mondorestore()
{
	log_msg(0, "-------------- Mondo Restore v%s -------------", PACKAGE_VERSION);
	log_msg(0,
			"DON'T PANIC! Mondorestore logs almost everything, so please ");
	log_msg(0,
			"don't break out in a cold sweat just because you see a few  ");
	log_msg(0,
			"error messages in the log. Read them; analyze them; see if  ");
	log_msg(0,
			"they are significant; above all, verify your backups! Please");
	log_msg(0,
			"attach a compressed copy of this log to any e-mail you send ");
	log_msg(0,
			"to the Mondo mailing list when you are seeking technical    ");
	log_msg(0,
			"support. Without it, we can't help you.               - Hugo");
	log_msg(0,
			"------------------------------------------------------------");
	log_msg(0,
			"BTW, despite (or perhaps because of) the wealth of messages,");
	log_msg(0,
			"some users are inclined to stop reading this log.  If Mondo ");
	log_msg(0,
			"stopped for some reason, chances are it's detailed here.    ");
	log_msg(0,
			"More than likely there's a message at the very end of this  ");
	log_msg(0,
			"log that will tell you what is wrong.  Please read it!      ");
	log_msg(0,
			"------------------------------------------------------------");
}



/**
 * Restore the user's data.
 * What did you think it did, anyway? :-)
 */
int main(int argc, char *argv[])
{
	FILE *fin;
	FILE *fout;
	int retval = 0;
	int res;
//  int c;
	char *tmp;

	struct mountlist_itself *mountlist;
	struct raidlist_itself *raidlist;
	struct s_bkpinfo *bkpinfo;
	struct s_node *filelist;
	char *a, *b;

  /**************************************************************************
   * hugo-                                                                  *
   * busy stuff here - it needs some comments -stan                           *
   *                                                                        *
   **************************************************************************/
	if (getuid() != 0) {
		fprintf(stderr, "Please run as root.\r\n");
		exit(127);
	}

	g_loglevel = DEFAULT_MR_LOGLEVEL;
	malloc_string(tmp);

/* Configure global variables */
#ifdef __FreeBSD__
	if (strstr
		(call_program_and_get_last_line_of_output("cat /tmp/cmdline"),
		 "textonly"))
#else
	if (strstr
		(call_program_and_get_last_line_of_output("cat /proc/cmdline"),
		 "textonly"))
#endif
	{
		g_text_mode = TRUE;
		log_msg(1, "TEXTONLY MODE");
	} else {
		g_text_mode = FALSE;
	}							// newt :-)
	if (!
		(bkpinfo = g_bkpinfo_DONTUSETHIS =
		 malloc(sizeof(struct s_bkpinfo)))) {
		fatal_error("Cannot malloc bkpinfo");
	}
	if (!(mountlist = malloc(sizeof(struct mountlist_itself)))) {
		fatal_error("Cannot malloc mountlist");
	}
	if (!(raidlist = malloc(sizeof(struct raidlist_itself)))) {
		fatal_error("Cannot malloc raidlist");
	}

	malloc_libmondo_global_strings();

	strcpy(g_mondo_home,
		   call_program_and_get_last_line_of_output("which mondorestore"));
	sprintf(g_tmpfs_mountpt, "/tmp/tmpfs");
	make_hole_for_dir(g_tmpfs_mountpt);
	g_current_media_number = 1;	// precaution

	run_program_and_log_output("mkdir -p " MNT_CDROM, FALSE);
	run_program_and_log_output("mkdir -p /mnt/floppy", FALSE);

	malloc_string(tmp);
	malloc_string(a);
	malloc_string(b);
	setup_MR_global_filenames(bkpinfo);	// malloc() and set globals, using bkpinfo->tmpdir etc.
	reset_bkpinfo(bkpinfo);
	bkpinfo->backup_media_type = none;	// in case boot disk was made for one backup type but user wants to restore from another backup type
	bkpinfo->restore_data = TRUE;	// Well, yeah :-)
	if (am_I_in_disaster_recovery_mode()) {
		run_program_and_log_output("mount / -o remount,rw", 2);
	}							// for b0rken distros
	g_main_pid = getpid();
	srandom((int) (time(NULL)));
	register_pid(getpid(), "mondo");
	set_signals(TRUE);
	g_kernel_version = get_kernel_version();

	log_msg(1, "FYI - g_mountlist_fname = %s", g_mountlist_fname);
	if (strlen(g_mountlist_fname) < 3) {
		fatal_error
			("Serious error in malloc()'ing. Could be a bug in your glibc.");
	}
	mkdir(MNT_CDROM, 0x770);

/* Backup original mountlist.txt */
	sprintf(tmp, "%s.orig", g_mountlist_fname);
	if (!does_file_exist(g_mountlist_fname)) {
		log_msg(2,
				"%ld: Warning - g_mountlist_fname (%s) does not exist yet",
				__LINE__, g_mountlist_fname);
	} else if (!does_file_exist(tmp)) {
		sprintf(tmp, "cp -f %s %s.orig", g_mountlist_fname,
				g_mountlist_fname);
		run_program_and_log_output(tmp, FALSE);
	}

/* Init directories */
	make_hole_for_dir(bkpinfo->tmpdir);
	sprintf(tmp, "mkdir -p %s", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, FALSE);
	make_hole_for_dir("/var/log");
	make_hole_for_dir("/tmp/tmpfs");	/* just in case... */
	run_program_and_log_output("umount " MNT_CDROM, FALSE);
	run_program_and_log_output
		("ln -sf /var/log/mondo-archive.log /tmp/mondo-restore.log",
		 FALSE);

	run_program_and_log_output("rm -Rf /tmp/tmpfs/mondo.tmp.*", FALSE);

/* Init GUI */
	malloc_libmondo_global_strings();
	setup_newt_stuff();			/* call newtInit and setup screen log */
	welcome_to_mondorestore();
	if (bkpinfo->disaster_recovery) {
		log_msg(1, "I am in disaster recovery mode");
	} else {
		log_msg(1, "I am in normal, live mode");
	}

	iamhere("what time is it");

/* Process command-line parameters */
	if (argc == 2 && strcmp(argv[1], "--edit-mountlist") == 0) {
#ifdef __FreeBSD__
		system("mv -f /tmp/raidconf.txt /etc/raidtab");
		if (!does_file_exist("/etc/raidtab"))
			system("vinum printconfig > /etc/raidtab");
#endif
		load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
		if (!does_file_exist(g_mountlist_fname)) {
			strcpy(g_mountlist_fname, "/tmp/mountlist.txt");
		}
		res = let_user_edit_the_mountlist(bkpinfo, mountlist, raidlist);
#ifdef __FreeBSD__
		system("mv -f /etc/raidtab /tmp/raidconf.txt");
#endif
		paranoid_MR_finish(res);
	}

	g_loglevel = DEFAULT_MR_LOGLEVEL;
	if (argc == 3 && strcmp(argv[1], "--echo-to-screen") == 0) {
		fout = fopen("/tmp/out.txt", "w");
		fput_string_one_char_at_a_time(stderr, argv[2]);
		finish(0);
	}

	if (argc == 3 && strcmp(argv[1], "--gendf") == 0) {
		make_grub_install_scriptlet(argv[2]);
		finish(0);
	}

	if (argc >= 2 && strcmp(argv[1], "--pih") == 0) {
		if (system("mount | grep cdrom 2> /dev/null > /dev/null")) {
			system("mount " MNT_CDROM);
		}
		bkpinfo->compression_level = 1;
		g_current_media_number = 2;
		strcpy(bkpinfo->restore_path, "/tmp/TESTING");
		bkpinfo->backup_media_type = dvd;
		open_progress_form("Reassembling /dev/hda1",
						   "Shark is a bit of a silly person.",
						   "Please wait. This may take some time.",
						   "", 1999);
		system("rm -Rf /tmp/*pih*");

		restore_a_biggiefile_from_CD(bkpinfo, 42, NULL, tmp);
	}

	if (argc == 5 && strcmp(argv[1], "--common") == 0) {
		g_loglevel = 6;
		filelist = load_filelist(argv[2]);
		if (!filelist) {
			fatal_error("Failed to load filelist");
		}
		toggle_node_selection(filelist, FALSE);
		toggle_all_root_dirs_on(filelist);
		// BERLIOS: /usr/lib ???
		toggle_path_selection(filelist, "/usr/share", TRUE);
//      show_filelist(filelist);
		save_filelist(filelist, "/tmp/out.txt");
//      finish(0);
//      toggle_path_selection (filelist, "/root/stuff", TRUE);
		strcpy(a, argv[3]);
		strcpy(b, argv[4]);

		res = save_filelist_entries_in_common(a, filelist, b, FALSE);
		free_filelist(filelist);
		printf("res = %d", res);
		finish(0);
	}

	if (argc == 3 && strcmp(argv[1], "--popuplist") == 0) {
		popup_changelist_from_file(argv[2]);
		paranoid_MR_finish(0);
	}

	if (argc == 5 && strcmp(argv[1], "--copy") == 0) {
		log_msg(1, "SCORE");
		g_loglevel = 10;
		if (strstr(argv[2], "save")) {
			log_msg(1, "Saving from %s to %s", argv[3], argv[4]);
			fin = fopen(argv[3], "r");
			fout = fopen(argv[4], "w");
			copy_from_src_to_dest(fin, fout, 'w');
			fclose(fin);
			fin = fopen(argv[3], "r");
			copy_from_src_to_dest(fin, fout, 'w');
			fclose(fout);
			fclose(fin);
		} else if (strstr(argv[2], "restore")) {
			fout = fopen(argv[3], "w");
			fin = fopen(argv[4], "r");
			copy_from_src_to_dest(fout, fin, 'r');
			fclose(fin);
			fin = fopen(argv[4], "r");
			copy_from_src_to_dest(fout, fin, 'r');
			fclose(fout);
			fclose(fin);
		} else {
			fatal_error("Unknown additional param");
		}
		finish(0);
	}

	if (argc == 3 && strcmp(argv[1], "--mdstat") == 0) {
		wait_until_software_raids_are_prepped(argv[2], 100);
		finish(0);
	}

	if (argc == 4 && strcmp(argv[1], "--mdconv") == 0) {
		finish(create_raidtab_from_mdstat(argv[2], argv[3]));
	}


	if (argc == 2 && strcmp(argv[1], "--live-grub") == 0) {
		retval = run_grub(FALSE, "/dev/hda");
		if (retval) {
			log_to_screen("Failed to write Master Boot Record");
		}
		paranoid_MR_finish(0);
	}
	if (argc == 3 && strcmp(argv[1], "--paa") == 0) {
		g_current_media_number = atoi(argv[2]);
		pause_and_ask_for_cdr(5, NULL);
		paranoid_MR_finish(0);
	} else if (!bkpinfo->disaster_recovery) {	// live!
		if (argc != 1) {
			popup_and_OK
				("Live mode doesn't support command-line parameters yet.");
			paranoid_MR_finish(1);
//    return(1);
		}
		log_msg(1, "I am in normal, live mode.");
		log_msg(2, "FYI, MOUNTLIST_FNAME = %s", g_mountlist_fname);
		mount_boot_if_necessary();	/* for Gentoo users */
		log_msg(2, "Still here.");
		if (argc > 1 && strcmp(argv[argc - 1], "--live-from-cd") == 0) {
			g_restoring_live_from_cd = TRUE;
		}
		if (argc == 5 && strcmp(argv[1], "--monitas-live") == 0) {
			retval =
				restore_live_from_monitas_server(bkpinfo,
												 argv[2],
												 argv[3], argv[4]);
		} else {
			log_msg(2, "Calling restore_to_live_filesystem()");
			retval = restore_to_live_filesystem(bkpinfo);
		}
		log_msg(2, "Still here. Yay.");
		if (strlen(bkpinfo->tmpdir) > 0) {
			sprintf(tmp, "rm -Rf %s/*", bkpinfo->tmpdir);
			run_program_and_log_output(tmp, FALSE);
		}
		unmount_boot_if_necessary();	/* for Gentoo users */
		paranoid_MR_finish(retval);
	} else {
/* Disaster recovery mode (must be) */
		log_msg(1, "I must be in disaster recovery mode.");
		log_msg(2, "FYI, MOUNTLIST_FNAME = %s ", g_mountlist_fname);
		if (argc == 3 && strcmp(argv[1], "--monitas-memorex") == 0) {
			log_to_screen("Uh, that hasn't been implemented yet.");
			paranoid_MR_finish(1);
		}

		iamhere("About to call load_mountlist and load_raidtab");
		strcpy(bkpinfo->restore_path, MNT_RESTORING);
		read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
		retval = load_mountlist(mountlist, g_mountlist_fname);
		retval += load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
		iamhere
			("Returned from calling load_mountlist and load_raidtab successfully");

		if (argc > 1
			&& (strcmp(argv[1], "--compare") == 0
				|| strcmp(argv[1], "--nuke") == 0)) {
			if (bkpinfo->backup_media_type == nfs
				&& !is_this_device_mounted(bkpinfo->nfs_mount)) {
				log_msg(1, "Mounting nfs dir");
				sprintf(bkpinfo->isodir, "/tmp/isodir");
				run_program_and_log_output("mkdir -p /tmp/isodir", 5);
				sprintf(tmp, "mount %s -t nfs -o nolock /tmp/isodir",
						bkpinfo->nfs_mount);
				run_program_and_log_output(tmp, 1);
			}
		}


		if (retval) {
			log_to_screen
				("Warning - load_raidtab_into_raidlist returned an error");
		}


		log_msg(1, "Send in the clowns.");

		if (argc == 2 && strcmp(argv[1], "--partition-only") == 0) {
			log_msg(0, "Partitioning only.");
			load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
			strcpy(g_mountlist_fname, "/tmp/mountlist.txt");
			load_mountlist(mountlist, g_mountlist_fname);
			res = partition_everything(mountlist);
			finish(res);
		}

		if (argc == 2 && strcmp(argv[1], "--format-only") == 0) {
			log_msg(0, "Formatting only.");
			load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
			strcpy(g_mountlist_fname, "/tmp/mountlist.txt");
			load_mountlist(mountlist, g_mountlist_fname);
			res = format_everything(mountlist, FALSE);
			finish(res);
		}

		if (argc == 2 && strcmp(argv[1], "--stop-lvm-and-raid") == 0) {
			log_msg(0, "Stopping LVM and RAID");
			load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
			strcpy(g_mountlist_fname, "/tmp/mountlist.txt");
			load_mountlist(mountlist, g_mountlist_fname);
			res = do_my_funky_lvm_stuff(TRUE, FALSE);
			res += stop_all_raid_devices(mountlist);
			finish(res);
		}

		if (argc == 2 && strcmp(argv[1], "--nuke") == 0) {
			iamhere("nuking");
			retval += nuke_mode(bkpinfo, mountlist, raidlist);
		} else if (argc == 2 && strcmp(argv[1], "--interactive") == 0) {
			iamhere("catchall");
			retval += catchall_mode(bkpinfo, mountlist, raidlist);
		} else if (argc == 2 && strcmp(argv[1], "--compare") == 0) {
			iamhere("compare");
			retval += compare_mode(bkpinfo, mountlist, raidlist);
		} else if (argc == 2 && strcmp(argv[1], "--iso") == 0) {
			iamhere("iso");
			retval = iso_mode(bkpinfo, mountlist, raidlist, FALSE);
		} else if (argc == 2 && strcmp(argv[1], "--mbr") == 0) {
			iamhere("mbr");
			retval = mount_all_devices(mountlist, TRUE);
			if (!retval) {
				retval += run_boot_loader(FALSE);
				retval += unmount_all_devices(mountlist);
			}
			if (retval) {
				log_to_screen("Failed to write Master Boot Record");
			}
		} else if (argc == 2 && strcmp(argv[1], "--isonuke") == 0) {
			iamhere("isonuke");
			retval = iso_mode(bkpinfo, mountlist, raidlist, TRUE);
		} else if (argc != 1) {
			log_to_screen("Invalid paremeters");
			paranoid_MR_finish(1);
		} else {
			iamhere("catchall (no mode specified in command-line call");
			retval += catchall_mode(bkpinfo, mountlist, raidlist);
		}
	}

	/* clean up at the end */
	if (retval) {
		if (does_file_exist("/tmp/changed.files")) {
			log_to_screen
				("See /tmp/changed.files for list of files that have changed.");
		}
		mvaddstr_and_log_it(g_currentY++,
							0,
							"Run complete. Errors were reported. Please review the logfile.");
	} else {
		if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
			mvaddstr_and_log_it(g_currentY++,
								0,
								"Run complete. Please remove floppy/CD/media and reboot.");
		} else {
			run_program_and_log_output("sync", FALSE);
			if (is_this_device_mounted(MNT_CDROM)) {
				res =
					run_program_and_log_output("umount " MNT_CDROM, FALSE);
			} else {
				res = 0;
			}

			if (!bkpinfo->please_dont_eject) {
				res = eject_device("/dev/cdrom");
/*
              if (res) 
		{
		  log_to_screen( "WARNING - failed to eject CD-ROM disk" ); 
		}
*/
			}
			mvaddstr_and_log_it(g_currentY++,
								0,
								"Run complete. Please remove media and reboot.");
		}
	}

// g_I_have_just_nuked is set true by nuke_mode() just before it returns
	if (g_I_have_just_nuked || does_file_exist("/POST-NUKE-ANYWAY")) {
		if (!system("which post-nuke > /dev/null 2> /dev/null")) {
			log_msg(1, "post-nuke found; running...");
			if (mount_all_devices(mountlist, TRUE)) {
				log_to_screen
					("Unable to re-mount partitions for post-nuke stuff");
			} else {
				log_msg(1, "Re-mounted partitions for post-nuke stuff");
				sprintf(tmp, "post-nuke %s %d", bkpinfo->restore_path,
						retval);
				if (!g_text_mode) {
					newtSuspend();
				}
				log_msg(2, "Calling '%s'", tmp);
				if ((res = system(tmp))) {
					log_OS_error(tmp);
				}
				if (!g_text_mode) {
					newtResume();
				}
//              newtCls();
				log_msg(1, "post-nuke returned w/ res=%d", res);
			}
			unmount_all_devices(mountlist);
			log_msg(1, "I've finished post-nuking.");
		}
	}
/*  
  log_to_screen("If you are REALLY in a hurry, hit Ctrl-Alt-Del now.");
  log_to_screen("Otherwise, please wait until the RAID disks are done.");
  wait_until_software_raids_are_prepped("/proc/mdstat", 100);
  log_to_screen("Thank you.");
*/
	unlink("/tmp/mondo-run-prog.tmp");
	set_signals(FALSE);
	sprintf(tmp, "rm -Rf %s", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, FALSE);
	log_to_screen
		("Restore log copied to /tmp/mondo-restore.log on your hard disk");
	sprintf(tmp,
			"Mondo-restore is exiting (retval=%d)                                      ",
			retval);
	log_to_screen(tmp);
	sprintf(tmp, "umount %s", bkpinfo->isodir);
	run_program_and_log_output(tmp, 5);
	paranoid_free(mountlist);
	paranoid_free(raidlist);
	if (am_I_in_disaster_recovery_mode()) {
		run_program_and_log_output("mount / -o remount,rw", 2);
	}							// for b0rken distros
	paranoid_MR_finish(retval);	// frees global stuff plus bkpinfo
	free_libmondo_global_strings();	// it's fine to have this here :) really :)
	paranoid_free(a);
	paranoid_free(b);
	paranoid_free(tmp);

	unlink("/tmp/filelist.full");
	unlink("/tmp/filelist.full.gz");

	exit(retval);
}

/**************************************************************************
 *END_MAIN                                                                *
 **************************************************************************/





/**************************************************************************
 *END_MONDO-RESTORE.C                                                     *
 **************************************************************************/
