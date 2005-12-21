/***************************************************************************
mondo-rstr-tools.c  -  description
-----------------

begin: Sun Sep 21 16:40:35 EDT 2003
copyright : (C) 2002 Mondo  Hugo Rabson
email	  : Hugo Rabson <hugorabson@msn.com>
edited by : by Stan Benoit ?/2003
email     : troff@nakedsoul.org
cvsid     : $Id: mondo-rstr-tools.c
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* mondo-rstr-tools.c               Hugo Rabson


07/27
- if the user is foolish enough to use /dev/md0 as boot device,
  call lilo -M /dev/hda to make sure lilo does its job properly
- better NFS+nuke support

07/20
- use backup's i-want-my-lvm file
- be sure to use archives' raidtab when restoring

07/18
- use /tmp/isodir for NFS if DR mode
- better support of users who boot from LVM CD and nuke-restore non-LVM backups

07/12
- bugfix to allow user to burn ISOs to CDs and restore from CDs (not original src)

06/29
- mount ext3 partitions as ext2, just in case :)

06/26
- delete make_relevant_partition_bootable()

06/19
- futzed with the call to mount floppy, to stop it from locking up on my AMD64 system

06/14
- shell out to /mnt/RESTORING chroot in order to let user install GRUB
  manually if automatic GRUB installation fails

06/15
- Added check for different 'isodir' chosen by user than stored in the archive
  Conor Daly <conor.daly@met.ie>

04/17
- replaced INTERNAL_TAPE_BLK_SIZE with bkpinfo->internal_tape_block_size

04/09
- don't try to mount CD if tape bkp

04/03
- trying to copy tmp/mondo-restore.cfg to itself - silly! - fixed

04/02
- when extracting cfg file and mountlist from all.tar.gz (tape copy),
  use block size of INTERNAL_TAPE_BLK_SIZE, not TAPE_BLOCK_SIZE

02/21
- don't use 'mv -v' cos Busybox won't support it

02/09
- make hole for cfg file before moving it (line 2094 or so)

02/03
- changed a couple of refs to filelist.full, to filelist.full.gz

01/16/2004
- instead of copying filelist, use 'ln -sf' to link to original;
  saves space

11/20/2003
- also retrieve /tmp/mountlist.txt if user wants

11/16
- fixed NFS path bug affecting the extractions of filelist/biggielist
  during selective restore

11/02
- fixed mount_cdrom() to run properly w/ nfs restores
- mount_device() returns 0 if swap mount fails cos swap isn't crucial

10/17
- run_grub() uses MNT_RESTORING instead of "/mnt/RESTORING"

10/26
- cleaned up run_grub()

10/25
- fixed mount_cdrom() to run properly w/ nfs restores

10/21
- mount_device() returns 0 if swap mount fails cos swap isn't crucial

10/15
- run_grub() now uses its initiative instead
  of calling grub-install

10/10
- don't leave copies of filelist.full lying around, clogging up
  the ramdisk, there's a good fellow :-)

10/02
- added 'dvd' to the range of media types I'll understand
- fixed iso->cdr problem (thanks, Stan Benoit & Fred Beondo)

09/24
- try lots of tape devs if /dev/st0 fails

09/23/2003
- first incarnation
*/


#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#include "mr-externs.h"
//#include "mondo-restore.h"
//#include "mondo-rstr-compare-EXT.h"
#include "mondo-rstr-tools.h"
#ifndef S_SPLINT_S
#include <pthread.h>
#endif

extern bool g_sigpipe_caught;
extern bool g_ISO_restore_mode;	/* are we in Iso Mode? */
extern bool g_I_have_just_nuked;
extern char *g_tmpfs_mountpt;
extern char *g_isodir_device;
extern char *g_isodir_format;
extern long g_current_progress, g_maximum_progress;
extern char *g_biggielist_txt;	// where 'biggielist.txt' is stored, on ramdisk / tempdir;
						  // biggielist.txt is the list of big files stored on the
						  // backup media set in question
extern char *g_filelist_full;	// filelist.full.gz is the list of all regular files
						  // (excluding big files) stored on the backup media set
extern char *g_biggielist_pot;	// list of big files which _could_ be restored, if the
						  // user chooses them
extern char *g_filelist_imagedevs;	// list of devices (e.g. /dev/hda1, /dev/sda5) which
							 // were archived as images, not just /dev entries
							 // ... e.g. NTFS, BeOS partitions
extern char *g_imagedevs_restthese;	// of the imagedevs listed in FILELIST_IMAGEDEVS,
							  // restore only these
extern char *g_mondo_cfg_file;	// where m*ndo-restore.cfg (the config file) is stored
extern char *g_mountlist_fname;	// where mountlist.txt (the mountlist file) is stored
extern char *g_mondo_home;		// homedir of Mondo; usually /usr/local/share/mondo
extern struct s_bkpinfo *g_bkpinfo_DONTUSETHIS;

extern t_bkptype g_backup_media_type;

extern int g_partition_table_locked_up;

/**
 * @addtogroup restoreUtilityGroup
 * @{
 */
/**
 * Free the malloc()s for the filename variables.
 */
void free_MR_global_filenames()
{
	paranoid_free(g_biggielist_txt);
	paranoid_free(g_filelist_full);
	paranoid_free(g_filelist_imagedevs);
//  paranoid_free (g_imagedevs_pot );
	paranoid_free(g_imagedevs_restthese);
	paranoid_free(g_mondo_cfg_file);
	paranoid_free(g_mountlist_fname);
	paranoid_free(g_mondo_home);
	paranoid_free(g_tmpfs_mountpt);
	paranoid_free(g_isodir_device);
	paranoid_free(g_isodir_format);

}



/**
 * Ask the user which imagedevs from the list contained in @p infname should
 * actually be restored.
 * @param infname The file containing a list of all imagedevs.
 * @param outfname The location of the output file containing the imagedevs the user wanted to restore.
 * @ingroup restoreUtilityGroup
 */
void ask_about_these_imagedevs(char *infname, char *outfname)
{
	FILE *fin;
	FILE *fout;
  /************************************************************************
   * allocate memory regions. test and set  -sab 16 feb 2003              *
   ************************************************************************/
	char *incoming;
	char *question;

	size_t n = 0;

	assert_string_is_neither_NULL_nor_zerolength(infname);
	assert_string_is_neither_NULL_nor_zerolength(outfname);

	if (!(fin = fopen(infname, "r"))) {
		fatal_error("Cannot openin infname");
	}
	if (!(fout = fopen(outfname, "w"))) {
		fatal_error("Cannot openin outfname");
	}
	for (getline(&incoming, &n, fin);
		 !feof(fin); getline(&incoming, &n, fin)) {
		strip_spaces(incoming);

		if (incoming[0] == '\0') {
			continue;
		}

		asprintf(&question,
				"Should I restore the image of %s ?", incoming);

		if (ask_me_yes_or_no(question)) {
			fprintf(fout, "%s\n", incoming);
		}
	}

  /*** free memory ***********/
	paranoid_free(incoming);
	paranoid_free(question);


	paranoid_fclose(fout);
	paranoid_fclose(fin);
}

/**************************************************************************
 *ASK_ABOUT_THESE_IMAGEDEVS                                               *
 **************************************************************************/








/**
 * Extract @c mondo-restore.cfg and @c mountlist.txt from @p ramdisk_fname.
 * @param bkpinfo The backup information structure. @c tmpdir is the only field used.
 * @param ramdisk_fname The filename of the @b compressed ramdisk to look in.
 * @param output_cfg_file Where to put the configuration file extracted.
 * @param output_mountlist_file Where to put the mountlist file extracted.
 * @return 0 for success, nonzero for failure.
 * @ingroup restoreUtilityGroup
 */
int
extract_config_file_from_ramdisk(struct s_bkpinfo *bkpinfo,
								 char *ramdisk_fname,
								 char *output_cfg_file,
								 char *output_mountlist_file)
{
	char *mountpt;
	char *command;
	char *orig_fname;
	int retval = 0;

	assert(bkpinfo != NULL);
	malloc_string(mountpt);
	malloc_string(command);
	malloc_string(orig_fname);
	assert_string_is_neither_NULL_nor_zerolength(ramdisk_fname);
	assert_string_is_neither_NULL_nor_zerolength(output_cfg_file);
	assert_string_is_neither_NULL_nor_zerolength(output_mountlist_file);
	sprintf(mountpt, "%s/mount.bootdisk", bkpinfo->tmpdir);
	sprintf(command, "mkdir -p %s", mountpt);
	run_program_and_log_output(command, FALSE);
	sprintf(command, "gzip -dc %s > %s/mindi.rd 2> /dev/null",
			ramdisk_fname, bkpinfo->tmpdir);

	run_program_and_log_output(command, FALSE);
	sprintf(command, "umount %s", mountpt);

	run_program_and_log_output(command, FALSE);

	sprintf(command, "mount -o loop %s/mindi.rd -t ext2 %s",
			bkpinfo->tmpdir, mountpt);

	run_program_and_log_output(command, FALSE);

	sprintf(command, "mkdir -p %s/tmp", bkpinfo->tmpdir);

	run_program_and_log_output(command, FALSE);

	sprintf(command, "cp -f %s/%s %s",	// %s/%s becomes {mountpt}/tmp/m*ndo-restore.cfg
			mountpt, g_mondo_cfg_file, output_cfg_file);
	run_program_and_log_output(command, FALSE);

	sprintf(orig_fname, "%s/%s", mountpt, g_mountlist_fname);
	if (does_file_exist(orig_fname)) {
		sprintf(command, "cp -f %s %s", orig_fname, output_mountlist_file);
		run_program_and_log_output(command, FALSE);
	}
	sprintf(command, "umount %s", mountpt);
	run_program_and_log_output(command, FALSE);
	if (!does_file_exist(output_cfg_file)
		|| (!does_file_exist(output_mountlist_file)
			&& does_file_exist(orig_fname))) {
		log_msg(2, "Failed to extract %s and/or %s from ramdisk",
				output_cfg_file, output_mountlist_file);
		retval = 1;
	} else {
		retval = 0;
	}
	paranoid_free(mountpt);
	paranoid_free(command);
	paranoid_free(orig_fname);
	return (retval);

}




/**
 * Keep trying to get mondo-restore.cfg from the archive, until the user gives up.
 * @param bkpinfo The backup information structure.
 */
void get_cfg_file_from_archive_or_bust(struct s_bkpinfo *bkpinfo)
{
	while (get_cfg_file_from_archive(bkpinfo)) {
		if (!ask_me_yes_or_no
			("Failed to find config file/archives. Choose another source?"))
		{
			fatal_error("Could not find config file/archives. Aborting.");
		}
		interactively_obtain_media_parameters_from_user(bkpinfo, FALSE);
	}
}


/**
 * Determine whether @p list_fname contains a line containing @p f.
 * @param f The line to search for.
 * @param list_fname The file to search in.
 * @param preamble Ignore this beginning part of @p f ("" to disable).
 * @return TRUE if it's in the list, FALSE if it's not.
 */
bool is_file_in_list(char *f, char *list_fname, char *preamble)
{

  /** needs malloc **/
	char *command;
	char *file;
	char *tmp;
	int res;

	malloc_string(command);
	malloc_string(file);
	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(f);
	assert_string_is_neither_NULL_nor_zerolength(list_fname);
	assert(preamble != NULL);

	if (strncmp(preamble, f, strlen(preamble)) == 0) {
		strcpy(file, f + strlen(preamble));
	} else {
		strcpy(file, f);
	}
	if (file[0] == '/' && file[1] == '/') {
		strcpy(tmp, file);
		strcpy(file, tmp + 1);
	}
	sprintf(tmp,
			"Checking to see if f=%s, file=%s, is in the list of biggiefiles",
			f, file);
	log_msg(2, tmp);
	sprintf(command, "cat %s | grep -x \"%s\"", list_fname, file);
	res = run_program_and_log_output(command, FALSE);
	paranoid_free(command);
	paranoid_free(file);
	paranoid_free(tmp);
	if (res) {
		return (FALSE);
	} else {
		return (TRUE);
	}
}

/**************************************************************************
 *END_IS_FILE_IN_LIST                                                     *
 **************************************************************************/



/**
 * Set up an ISO backup.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->disaster_recovery
 * - @c bkpinfo->isodir
 * @param nuke_me_please If TRUE, we're in nuke mode; if FALSE we're in interactive mode.
 * @return 0 for success, nonzero for failure.
 */
int iso_fiddly_bits(struct s_bkpinfo *bkpinfo, bool nuke_me_please)
{
	char *mount_isodir_command, *tmp, *command;
	int retval = 0, i;
	bool already_mounted = FALSE;

	assert(bkpinfo != NULL);
	malloc_string(mount_isodir_command);
	malloc_string(tmp);
	malloc_string(command);
	g_ISO_restore_mode = TRUE;
	read_cfg_var(g_mondo_cfg_file, "iso-dev", g_isodir_device);
	if (bkpinfo->disaster_recovery) {
/* Patch Conor Daly 26-june-2004 
 * Don't let this clobber an existing bkpinfo->isodir */
		if (!bkpinfo->isodir[0]) {
			strcpy(bkpinfo->isodir, "/tmp/isodir");
		}
/* End patch */
		sprintf(command, "mkdir -p %s", bkpinfo->isodir);
		run_program_and_log_output(command, 5);
		log_msg(2, "Setting isodir to %s", bkpinfo->isodir);
	}

	if (!get_isodir_info
		(g_isodir_device, g_isodir_format, bkpinfo->isodir,
		 nuke_me_please)) {
		return (1);
	}
	paranoid_system("umount " MNT_CDROM " 2> /dev/null");	/* just in case */

	if (is_this_device_mounted(g_isodir_device)) {
		log_to_screen("WARNING - isodir is already mounted");
		already_mounted = TRUE;
	} else {
		sprintf(mount_isodir_command, "mount %s", g_isodir_device);
		if (strlen(g_isodir_format) > 1) {
			sprintf(mount_isodir_command + strlen(mount_isodir_command),
					" -t %s", g_isodir_format);
		}
		strcat(mount_isodir_command, " -o ro ");
		strcat(mount_isodir_command, bkpinfo->isodir);
		run_program_and_log_output("df -m", FALSE);
		sprintf(tmp,
				"The 'mount' command is '%s'. PLEASE report this command to be if you have problems, ok?",
				mount_isodir_command);
		log_msg(1, tmp);
		if (run_program_and_log_output(mount_isodir_command, FALSE)) {
			popup_and_OK
				("Cannot mount the device where the ISO files are stored.");
			return (1);
		}
		log_to_screen
			("I have mounted the device where the ISO files are stored.");
	}
	if (!IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		mount_cdrom(bkpinfo);
	}
	i = what_number_cd_is_this(bkpinfo);	/* has the side-effect of calling mount_cdrom() */
	sprintf(tmp, "%s #%d has been mounted via loopback mount",
			media_descriptor_string(bkpinfo->backup_media_type), i);
	log_msg(1, tmp);
	if (i < 0) {
		popup_and_OK
			("Cannot find ISO images in the directory you specified.");
		retval = 1;
	}
	log_msg(2, "%ld: bkpinfo->isodir is now %s", __LINE__,
			bkpinfo->isodir);
	paranoid_free(mount_isodir_command);
	paranoid_free(tmp);
	paranoid_free(command);
	return (retval);
}




/**
 * Kill all Petris processes.
 */
void kill_petris(void)
{
	char *command;
	malloc_string(command);
	sprintf(command,
			"kill `ps wax 2> /dev/null | grep petris 2> /dev/null | grep -v grep | cut -d' ' -f2` 2> /dev/null");
	paranoid_system(command);
	paranoid_free(command);
}

/**************************************************************************
 *END_KILL_PETRIS                                                         *
 **************************************************************************/


/**
 * (Disabled) Modify rc.local to fix some things on first boot.
 * This function currently doesn't do anything except make sure /tmp has the
 * right permissions.
 * @param path The path to /etc on the user's filesystem.
 * @return 0 for success, nonzero for failure.
 */
int modify_rclocal_one_time(char *path)
{
  /** malloc **/
	char *rclocal_fname;
	char *newfile_fname;
	char *tmp;

	malloc_string(rclocal_fname);
	malloc_string(newfile_fname);
	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(path);

	sprintf(rclocal_fname, "%s/rc.local", path);

//  sprintf(tmp, "chmod 1777 %s/tmp", MNT_RESTORING);
//  run_program_and_log_output( tmp, FALSE );
	return (0);					/* remove this line to open the floodgates... */

	if (!does_file_exist(rclocal_fname)) {
		sprintf(rclocal_fname, "%s/rc.d/rc.local", path);
	}
	if (!does_file_exist(rclocal_fname)) {
		paranoid_free(rclocal_fname);
		paranoid_free(newfile_fname);
		paranoid_free(tmp);
		return (1);
	}
	sprintf(newfile_fname, "%s/rc.local.mondorescue", path);
	sprintf(tmp, "cat %s | grep mondorescue > /dev/null 2> /dev/null",
			rclocal_fname);
	if (system(tmp)) {
		sprintf(tmp, "echo \"[ -e %s ] && %s\n\" >> %s",
				newfile_fname, newfile_fname, rclocal_fname);

		paranoid_system(tmp);
	}
	sprintf(tmp, "echo -en \"#!/bin/sh\
\\n\
\\n\
cat %s | grep -v mondorescue > %s\\n\
rm -f /var/lock/subsys/*xfs*\\n\
rm -f /var/run/xfs.*\\n\
killall xfs\\n\
service xfs start\\n\
yes | rm -f %s\\n\
\" > %s", rclocal_fname, rclocal_fname, newfile_fname, newfile_fname);
	sprintf(tmp, "chmod +x \"%s\"", newfile_fname);
	run_program_and_log_output(tmp, FALSE);
	paranoid_free(rclocal_fname);
	paranoid_free(newfile_fname);
	paranoid_free(tmp);
	return (0);
}

/**************************************************************************
 *END_ MODIFY_RCLOCAL_ONE_TIME                                            *
 **************************************************************************/





/**
 * Mount all devices in @p p_external_copy_of_mountlist on @p MNT_RESTORING.
 * @param p_external_copy_of_mountlist The mountlist containing devices to be mounted.
 * @param writeable If TRUE, then mount read-write; if FALSE mount read-only.
 * @return The number of errors encountered (0 for success).
 */
int mount_all_devices(struct mountlist_itself
					  *p_external_copy_of_mountlist, bool writeable)
{
	int retval = 0, lino, res;
	char *tmp, *these_failed, *format;
	struct mountlist_itself *mountlist;

	malloc_string(tmp);
	malloc_string(format);
	malloc_string(these_failed);
	assert(p_external_copy_of_mountlist != NULL);
	mountlist = malloc(sizeof(struct mountlist_itself));
	memcpy((void *) mountlist, (void *) p_external_copy_of_mountlist,
		   sizeof(struct mountlist_itself));
	sort_mountlist_by_mountpoint(mountlist, 0);

  /** menset **/
	these_failed[0] = '\0';

	mvaddstr_and_log_it(g_currentY, 0, "Mounting devices         ");
	open_progress_form("Mounting devices",
					   "I am now mounting all the drives.",
					   "This should not take long.",
					   "", mountlist->entries);

	for (lino = 0; lino < mountlist->entries; lino++) {
		if (!strcmp(mountlist->el[lino].device, "/proc")) {
			log_msg(1,
					"Again with the /proc - why is this in your mountlist?");
		} else if (is_this_device_mounted(mountlist->el[lino].device)) {
			sprintf(tmp, "%s is already mounted",
					mountlist->el[lino].device);
			log_to_screen(tmp);
		} else if (strcmp(mountlist->el[lino].mountpoint, "none")
				   && strcmp(mountlist->el[lino].mountpoint, "lvm")
				   && strcmp(mountlist->el[lino].mountpoint, "raid")
				   && strcmp(mountlist->el[lino].mountpoint, "image")) {
			sprintf(tmp, "Mounting %s", mountlist->el[lino].device);
			update_progress_form(tmp);
			strcpy(format, mountlist->el[lino].format);
			if (!strcmp(format, "ext3")) {
				strcpy(format, "ext2");
			}
			res = mount_device(mountlist->el[lino].device,
							   mountlist->el[lino].mountpoint,
							   format, writeable);
			retval += res;
			if (res) {
				strcat(these_failed, mountlist->el[lino].device);
				strcat(these_failed, " ");
			}
		}
		g_current_progress++;
	}
	close_progress_form();
	run_program_and_log_output("df -m", TRUE);
	if (retval) {
		if (g_partition_table_locked_up > 0) {
			log_to_screen
				("fdisk's ictol() call to refresh its copy of the partition table causes the kernel to");
			log_to_screen
				("lock up the partition table. You might have to reboot and use Interactive Mode to");
			log_to_screen
				("format and restore *without* partitioning first. Sorry for the inconvenience.");
		}
		sprintf(tmp, "Could not mount devices %s- shall I abort?",
				these_failed);
		if (!ask_me_yes_or_no(tmp)) {
			retval = 0;
			log_to_screen
				("Continuing, although some devices failed to be mounted");
			mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		} else {
			mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
			log_to_screen
				("Unable to mount some or all of your partitions.");
		}
	} else {
		log_to_screen("All partitions were mounted OK.");
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	run_program_and_log_output("df -m", 3);
	paranoid_free(mountlist);
	paranoid_free(tmp);
	paranoid_free(format);
	paranoid_free(these_failed);
	return (retval);
}

 /**************************************************************************
  *END_MOUNT_ALL_DEVICES                                                   *
  **************************************************************************/


/**
 * Mount the CD-ROM device at /mnt/cdrom.
 * @param bkpinfo The backup information structure. Fields used: 
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->disaster_recovery
 * - @c bkpinfo->isodir
 * - @c bkpinfo->media_device
 * @return 0 for success, nonzero for failure.
 */
int mount_cdrom(struct s_bkpinfo *bkpinfo)
{
	char *mount_cmd, *tmp;
	int i, res;
#ifdef __FreeBSD__
	char mdd[32];
	char *mddev = mdd;
#endif

	malloc_string(mount_cmd);
	assert(bkpinfo != NULL);

	if (bkpinfo->backup_media_type == tape
		|| bkpinfo->backup_media_type == udev) {
		log_msg(8, "Tape/udev. Therefore, no need to mount CDROM.");
		paranoid_free(mount_cmd);
		return 0;
	}

	if (!run_program_and_log_output("mount | fgrep " MNT_CDROM, FALSE)) {
		log_msg(2, "mount_cdrom() - CD already mounted. Fair enough.");
		paranoid_free(mount_cmd);
		return (0);
	}

	if (bkpinfo->backup_media_type == nfs) {
		log_msg(2, "Mounting for NFS thingy");
		log_msg(2, "isodir = %s", bkpinfo->isodir);
		if ((!bkpinfo->isodir[0] || !strcmp(bkpinfo->isodir, "/"))
			&& am_I_in_disaster_recovery_mode()) {
			strcpy(bkpinfo->isodir, "/tmp/isodir");
			log_msg(1, "isodir is being set to %s", bkpinfo->isodir);
		}
#ifdef __FreeBSD__
		sprintf(mount_cmd, "/mnt/isodir/%s/%s/%s-%d.iso", bkpinfo->isodir,
				bkpinfo->nfs_remote_dir, bkpinfo->prefix, g_current_media_number);
		mddev = make_vn(mount_cmd);
		sprintf(mount_cmd, "mount_cd9660 -r %s " MNT_CDROM, mddev);
#else
		sprintf(mount_cmd, "mount %s/%s/%s-%d.iso -t iso9660 -o loop,ro %s",
				bkpinfo->isodir, bkpinfo->nfs_remote_dir,
				bkpinfo->prefix, g_current_media_number, MNT_CDROM);
#endif

	} else if (bkpinfo->backup_media_type == iso) {
#ifdef __FreeBSD__
		sprintf(mount_cmd, "%s/%s-%d.iso", bkpinfo->isodir,
				bkpinfo->prefix, g_current_media_number);
		mddev = make_vn(mount_cmd);
		sprintf(mount_cmd, "mount_cd9660 -r %s %s", mddev, MNT_CDROM);
#else
		sprintf(mount_cmd, "mount %s/%s-%d.iso -t iso9660 -o loop,ro %s",
				bkpinfo->isodir, bkpinfo->prefix, g_current_media_number, MNT_CDROM);
#endif
	} else if (strstr(bkpinfo->media_device, "/dev/"))
#ifdef __FreeBSD__
	{
		sprintf(mount_cmd, "mount_cd9660 -r %s %s", bkpinfo->media_device,
				MNT_CDROM);
	}
#else
	{
		sprintf(mount_cmd, "mount %s -t iso9660 -o ro %s",
				bkpinfo->media_device, MNT_CDROM);
	}
#endif

	else {
		if (bkpinfo->disaster_recovery
			&& does_file_exist("/tmp/CDROM-LIVES-HERE")) {
			paranoid_free(bkpinfo->media_device);
			asprintf(&tmp,
				   last_line_of_file("/tmp/CDROM-LIVES-HERE"));
			bkpinfo->media_device = tmp;
		} else {
			paranoid_free(bkpinfo->media_device);
			bkpinfo->media_device = find_cdrom_device(TRUE);
		}

#ifdef __FreeBSD__
		sprintf(mount_cmd, "mount_cd9660 -r %s %s", bkpinfo->media_device,
				MNT_CDROM);
#else
		sprintf(mount_cmd, "mount %s -t iso9660 -o ro %s",
				bkpinfo->media_device, MNT_CDROM);
#endif

	}
	log_msg(2, "(mount_cdrom) --- command = %s", mount_cmd);
	for (i = 0; i < 2; i++) {
		res = run_program_and_log_output(mount_cmd, FALSE);
		if (!res) {
			break;
		} else {
			log_msg(2, "Failed to mount CD-ROM drive.");
			sleep(5);
			run_program_and_log_output("sync", FALSE);
		}
	}
	if (res) {
		log_msg(2, "Failed, despite %d attempts", i);
	} else {
		log_msg(2, "Mounted CD-ROM drive OK");
	}
	paranoid_free(mount_cmd);
	return (res);
}





/**************************************************************************
 *END_MOUNT_CDROM                                                         *
 **************************************************************************/


/**
 * Mount @p device at @p mpt as @p format.
 * @param device The device (/dev entry) to mount.
 * @param mpt The directory to mount it on.
 * @param format The filesystem type of @p device.
 * @param writeable If TRUE, mount read-write; if FALSE, mount read-only.
 * @return 0 for success, nonzero for failure.
 */
int mount_device(char *device, char *mpt, char *format, bool writeable)
{
	int res = 0;

  /** malloc **/
	char *tmp, *command, *mountdir, *mountpoint, *additional_parameters;

	assert_string_is_neither_NULL_nor_zerolength(device);
	assert_string_is_neither_NULL_nor_zerolength(mpt);
	assert(format != NULL);
	malloc_string(tmp);
	malloc_string(command);
	malloc_string(mountdir);
	malloc_string(mountpoint);
	malloc_string(additional_parameters);

	if (!strcmp(mpt, "/1")) {
		strcpy(mountpoint, "/");
		log_msg(3, "Mommm! SME is being a dildo!");
	} else {
		strcpy(mountpoint, mpt);
	}

	if (!strcmp(mountpoint, "lvm")) {
		return (0);
	}
	if (!strcmp(mountpoint, "image")) {
		return (0);
	}
	sprintf(tmp, "Mounting device %s   ", device);
	log_msg(1, tmp);
	if (writeable) {
		strcpy(additional_parameters, "-o rw");
	} else {
		strcpy(additional_parameters, "-o ro");
	}
	if (find_home_of_exe("setfattr")) {
		strcat(additional_parameters, ",user_xattr");
	}
	if (find_home_of_exe("setfacl")) {
		strcat(additional_parameters, ",acl");
	}

	if (!strcmp(mountpoint, "swap")) {
		sprintf(command, "swapon %s", device);
	} else {
		if (!strcmp(mountpoint, "/")) {
			strcpy(mountdir, MNT_RESTORING);
		} else {
			sprintf(mountdir, "%s%s", MNT_RESTORING, mountpoint);
		}
		sprintf(command, "mkdir -p %s", mountdir);
		run_program_and_log_output(command, FALSE);
		sprintf(command, "mount -t %s %s %s %s 2>> %s", format, device,
				additional_parameters, mountdir, MONDO_LOGFILE);
		log_msg(2, "command='%s'", command);
	}
	res = run_program_and_log_output(command, TRUE);
	if (res && (strstr(command, "xattr") || strstr(command, "acl"))) {
		log_msg(1, "Re-trying without the fancy extra parameters");
		sprintf(command, "mount -t %s %s %s 2>> %s", format, device,
				mountdir, MONDO_LOGFILE);
		res = run_program_and_log_output(command, TRUE);
	}
	if (res) {
		log_msg(1, "Unable to mount device %s (type %s) at %s", device,
				format, mountdir);
		log_msg(1, "command was '%s'", command);
		if (!strcmp(mountpoint, "swap")) {
			log_to_screen(tmp);
		} else {
			log_msg(2, "Retrying w/o the '-t' switch");
			sprintf(command, "mount %s %s 2>> %s", device, mountdir,
					MONDO_LOGFILE);
			log_msg(2, "2nd command = '%s'", command);
			res = run_program_and_log_output(command, TRUE);
			if (res == 0) {
				log_msg(1,
						"That's OK. I called mount w/o a filesystem type and it worked fine in the end.");
			} else {
				log_to_screen(tmp);
			}
		}
	}
	if (res && !strcmp(mountpoint, "swap")) {
		log_msg(2, "That's ok. It's just a swap partition.");
		log_msg(2, "Non-fatal error. Returning 0.");
		res = 0;
	}

	paranoid_free(tmp);
	paranoid_free(command);
	paranoid_free(mountdir);
	paranoid_free(mountpoint);
	paranoid_free(additional_parameters);

	return (res);
}

/**************************************************************************
 *END_MOUNT_DEVICE                                                        *
 **************************************************************************/



/**
 * Fix some miscellaneous things in the filesystem so the system will come
 * up correctly on the first boot.
 */
void protect_against_braindead_sysadmins()
{
	run_program_and_log_output("touch " MNT_RESTORING "/var/log/pacct",
							   FALSE);
	run_program_and_log_output("touch " MNT_RESTORING "/var/account/pacct",
							   FALSE);
	if (run_program_and_log_output("ls " MNT_RESTORING " /tmp", FALSE)) {
		run_program_and_log_output("chmod 1777 " MNT_RESTORING "/tmp",
								   FALSE);
	}
	run_program_and_log_output("mkdir -p " MNT_RESTORING
							   "/var/run/console", FALSE);
	run_program_and_log_output("chmod 777 " MNT_RESTORING "/dev/null",
							   FALSE);
	run_program_and_log_output("cd " MNT_RESTORING
							   "; for i in `ls home/`; do echo \"Moving $i's spurious files to $i/.disabled\"; mkdir $i/.disabled ; mv -f $i/.DCOP* $i/.MCOP* $i/.*authority $i/.kde/tmp* $i/.kde/socket* $i/.disabled/ ; done",
							   TRUE);
	run_program_and_log_output("rm -f " MNT_RESTORING "/var/run/*.pid",
							   TRUE);
	run_program_and_log_output("rm -f " MNT_RESTORING "/var/lock/subsys/*",
							   TRUE);
}

/**************************************************************************
 *END_PROTECT_AGAINST_BRAINDEAD_SYSADMINS                                 *
 **************************************************************************/




/**
 * Fill out @p bkpinfo based on @p cfg_file.
 * @param cfg_file The mondo-restore.cfg file to read into @p bkpinfo.
 * @param bkpinfo The backup information structure to fill out with information
 * from @p cfg_file.
 * @return 0 for success, nonzero for failure.
 */
int read_cfg_file_into_bkpinfo(char *cfgf, struct s_bkpinfo *bkpinfo)
{
  /** add mallocs **/
	char *value;
	char *tmp;
	char *command;
	char *iso_mnt;
	char *iso_path;
	char *old_isodir;
	char cfg_file[100];
	t_bkptype media_specified_by_user;

	malloc_string(command);
	malloc_string(iso_mnt);
	malloc_string(iso_path);
	malloc_string(old_isodir);
	malloc_string(value);
	malloc_string(tmp);
//  assert_string_is_neither_NULL_nor_zerolength(cfg_file);
	assert(bkpinfo != NULL);

	if (!cfgf) {
		strcpy(cfg_file, g_mondo_cfg_file);
	} else {
		strcpy(cfg_file, cfgf);
	}

	media_specified_by_user = bkpinfo->backup_media_type;	// or 'none', if not specified

	if (0 == read_cfg_var(cfg_file, "backup-media-type", value)) {
		if (!strcmp(value, "cdstream")) {
			bkpinfo->backup_media_type = cdstream;
		} else if (!strcmp(value, "cdr")) {
			bkpinfo->backup_media_type = cdr;
		} else if (!strcmp(value, "cdrw")) {
			bkpinfo->backup_media_type = cdrw;
		} else if (!strcmp(value, "dvd")) {
			bkpinfo->backup_media_type = dvd;
		} else if (!strcmp(value, "iso")) {
/*
	  if (am_I_in_disaster_recovery_mode()
	  && !run_program_and_log_output("mount /dev/cdrom "MNT_CDROM, 1)
	  && does_file_exist(MNT_CDROM"/archives/filelist.0"))
*/

// Patch by Conor Daly - 2004/07/12
			bkpinfo->backup_media_type = iso;
			if (am_I_in_disaster_recovery_mode()) {
				/* Check to see if CD is already mounted before mounting it... */
				if (!is_this_device_mounted("/dev/cdrom")) {
					log_msg(2,
							"NB: CDROM device not mounted, mounting...");
					run_program_and_log_output("mount /dev/cdrom "
											   MNT_CDROM, 1);
				}
				if (does_file_exist(MNT_CDROM "/archives/filelist.0")) {
					bkpinfo->backup_media_type = cdr;
					run_program_and_log_output("umount " MNT_CDROM, 1);
					log_it
						("Re-jigging configuration AGAIN. CD-R, not ISO.");
				}
			}
			if (read_cfg_var(cfg_file, "iso-prefix", value) == 0) {
					strcpy(bkpinfo->prefix,value);
			} else {
					strcpy(bkpinfo->prefix,STD_PREFIX);
			}
		} else if (!strcmp(value, "nfs")) {
			bkpinfo->backup_media_type = nfs;
			if (read_cfg_var(cfg_file, "iso-prefix", value) == 0) {
					strcpy(bkpinfo->prefix,value);
			} else {
					strcpy(bkpinfo->prefix,STD_PREFIX);
			}
		} else if (!strcmp(value, "tape")) {
			bkpinfo->backup_media_type = tape;
		} else if (!strcmp(value, "udev")) {
			bkpinfo->backup_media_type = udev;
		} else {
			fatal_error("UNKNOWN bkp-media-type");
		}
	} else {
		fatal_error("backup-media-type not specified!");
	}
	if (bkpinfo->disaster_recovery) {
		if (bkpinfo->backup_media_type == cdstream) {
			paranoid_alloc(bkpinfo->media_device, "/dev/cdrom");
			bkpinfo->media_size[0] = 1999 * 1024;
			bkpinfo->media_size[1] = 650;	/* good guess */
		} else if (bkpinfo->backup_media_type == tape
				   || bkpinfo->backup_media_type == udev) {
			if (read_cfg_var(cfg_file, "media-dev", bkpinfo->media_device)) {
				fatal_error("Cannot get tape device name from cfg file");
			}
			read_cfg_var(cfg_file, "media-size", value);
			bkpinfo->media_size[1] = atol(value);
			sprintf(tmp, "Backup medium is TAPE --- dev=%s",
					bkpinfo->media_device);
			log_msg(2, tmp);
		} else {
			paranoid_alloc(bkpinfo->media_device, "/dev/cdrom");
			bkpinfo->media_size[0] = 1999 * 1024;	/* 650, probably, but we don't need this var anyway */
			bkpinfo->media_size[1] = 1999 * 1024;	/* 650, probably, but we don't need this var anyway */
			log_msg(2, "Backup medium is CD-R[W]");
		}
	} else {
		log_msg(2,
				"Not in Disaster Recovery Mode. No need to derive device name from config file.");
	}

	read_cfg_var(cfg_file, "use-star", value);
	if (strstr(value, "yes")) {
		bkpinfo->use_star = TRUE;
		log_msg(1, "Goody! ... bkpinfo->use_star is now true.");
	}

	if (0 == read_cfg_var(cfg_file, "internal-tape-block-size", value)) {
		bkpinfo->internal_tape_block_size = atol(value);
		log_msg(1, "Internal tape block size has been custom-set to %ld",
				bkpinfo->internal_tape_block_size);
	} else {
		bkpinfo->internal_tape_block_size =
			DEFAULT_INTERNAL_TAPE_BLOCK_SIZE;
		log_msg(1, "Internal tape block size = default (%ld)",
				DEFAULT_INTERNAL_TAPE_BLOCK_SIZE);
	}

	read_cfg_var(cfg_file, "use-lzo", value);
	if (strstr(value, "yes")) {
		bkpinfo->use_lzo = TRUE;
		strcpy(bkpinfo->zip_exe, "lzop");
		strcpy(bkpinfo->zip_suffix, "lzo");
	} else {
		read_cfg_var(cfg_file, "use-comp", value);
		if (strstr(value, "yes")) {
			bkpinfo->use_lzo = FALSE;
			strcpy(bkpinfo->zip_exe, "bzip2");
			strcpy(bkpinfo->zip_suffix, "bz2");
		} else {
			bkpinfo->zip_exe[0] = bkpinfo->zip_suffix[0] = '\0';
		}
	}

	value[0] = '\0';
	read_cfg_var(cfg_file, "differential", value);
	if (!strcmp(value, "yes") || !strcmp(value, "1")) {
		bkpinfo->differential = TRUE;
	}
	log_msg(2, "differential var = '%s'", value);
	if (bkpinfo->differential) {
		log_msg(2, "THIS IS A DIFFERENTIAL BACKUP");
	} else {
		log_msg(2, "This is a regular (full) backup");
	}

	read_cfg_var(g_mondo_cfg_file, "please-dont-eject", tmp);
	if (tmp[0]
		||
		strstr(call_program_and_get_last_line_of_output
			   ("cat /proc/cmdline"), "donteject")) {
		bkpinfo->please_dont_eject = TRUE;
		log_msg(2, "Ok, I shan't eject when restoring! Groovy.");
	}

	if (bkpinfo->backup_media_type == nfs) {
		if (!cfgf) {
			log_msg(2, "nfs_mount remains %s", bkpinfo->nfs_mount);
			log_msg(2, "nfs_remote_dir remains %s",
					bkpinfo->nfs_remote_dir);
			log_msg(2,
					"...cos it wouldn't make sense to abandon the values that GOT ME to this config file in the first place");
		} else {
			read_cfg_var(g_mondo_cfg_file, "nfs-server-mount",
						 bkpinfo->nfs_mount);
			read_cfg_var(g_mondo_cfg_file, "nfs-server-path",
						 bkpinfo->nfs_remote_dir);
			log_msg(2, "nfs_mount is %s", bkpinfo->nfs_mount);
			log_msg(2, "nfs_remote_dir is %s", bkpinfo->nfs_remote_dir);
		}
	} else if (bkpinfo->backup_media_type == iso) {
		/* Patch by Conor Daly 23-june-2004 
		 * to correctly mount iso-dev and set a sensible
		 * isodir in disaster recovery mode
		 */
		strcpy(old_isodir, bkpinfo->isodir);
		read_cfg_var(g_mondo_cfg_file, "iso-mnt", iso_mnt);
		read_cfg_var(g_mondo_cfg_file, "isodir", iso_path);
		sprintf(bkpinfo->isodir, "%s%s", iso_mnt, iso_path);
		if (!bkpinfo->isodir[0]) {
			strcpy(bkpinfo->isodir, old_isodir);
		}
		if (!bkpinfo->disaster_recovery) {
			if (strcmp(old_isodir, bkpinfo->isodir)) {
				log_it
					("user nominated isodir differs from archive, keeping user's choice: %s %s\n",
					 old_isodir, bkpinfo->isodir);
				strcpy(bkpinfo->isodir, old_isodir);
			}
		}
		read_cfg_var(g_mondo_cfg_file, "iso-dev", g_isodir_device);
		log_msg(2, "isodir=%s; iso-dev=%s", bkpinfo->isodir,
				g_isodir_device);
		if (bkpinfo->disaster_recovery) {
			if (is_this_device_mounted(g_isodir_device)) {
				log_msg(2, "NB: isodir is already mounted");
				/* Find out where it's mounted */
				sprintf(command,
						"mount | grep -w %s | tail -n1 | cut -d' ' -f3",
						g_isodir_device);
				log_it("command = %s", command);
				log_it("res of it = %s",
					   call_program_and_get_last_line_of_output(command));
				sprintf(iso_mnt, "%s",
						call_program_and_get_last_line_of_output(command));
			} else {
				sprintf(iso_mnt, "/tmp/isodir");
				sprintf(tmp, "mkdir -p %s", iso_mnt);
				run_program_and_log_output(tmp, 5);
				sprintf(tmp, "mount %s %s", g_isodir_device, iso_mnt);
				if (run_program_and_log_output(tmp, 3)) {
					log_msg(1,
							"Unable to mount isodir. Perhaps this is really a CD backup?");
					bkpinfo->backup_media_type = cdr;
					paranoid_alloc(bkpinfo->media_device, "/dev/cdrom");
					bkpinfo->isodir[0] = iso_mnt[0] = iso_path[0] = '\0';
					if (mount_cdrom(bkpinfo)) {
						fatal_error
							("Unable to mount isodir. Failed to mount CD-ROM as well.");
					} else {
						log_msg(1,
								"You backed up to disk, then burned some CDs. Naughty monkey!");
					}
				}
			}
			/* bkpinfo->isodir should now be the true path to prefix-1.iso etc... */
			if (bkpinfo->backup_media_type == iso) {
				sprintf(bkpinfo->isodir, "%s%s", iso_mnt, iso_path);
			}
		}
	}

	if (media_specified_by_user != none) {
		if (g_restoring_live_from_cd) {
			if (bkpinfo->backup_media_type != media_specified_by_user) {
				log_msg(2,
						"bkpinfo->backup_media_type != media_specified_by_user, so I'd better ask :)");
				interactively_obtain_media_parameters_from_user(bkpinfo,
																FALSE);
				media_specified_by_user = bkpinfo->backup_media_type;
				get_cfg_file_from_archive(bkpinfo);
/*
              if (media_specified_by_user != cdr && media_specified_by_user == cdrw)
                { g_restoring_live_from_cd = FALSE; }
*/
			}
		}
		bkpinfo->backup_media_type = media_specified_by_user;
	}
	g_backup_media_type = bkpinfo->backup_media_type;
	paranoid_free(value);
	paranoid_free(tmp);
	paranoid_free(command);
	paranoid_free(iso_mnt);
	paranoid_free(iso_path);
	paranoid_free(old_isodir);
	return (0);

}

/**************************************************************************
 *END_READ_CFG_FILE_INTO_BKPINFO                                          *
 **************************************************************************/




/**
 * Allow the user to edit the filelist and biggielist.
 * The filelist is unlinked after it is read.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->isodir
 * - @c bkpinfo->media_device
 * - @c bkpinfo->tmpdir
 * @return The filelist structure containing the information read from disk.
 */
struct
s_node *process_filelist_and_biggielist(struct s_bkpinfo *bkpinfo)
{
	struct s_node *filelist;

  /** add mallocs**/
	char *command;
	char *tmp;
	int res = 0;
	pid_t pid;

	assert(bkpinfo != NULL);
	malloc_string(command);
	malloc_string(tmp);

	if (does_file_exist(g_filelist_full)
		&& does_file_exist(g_biggielist_txt)) {
		log_msg(1, "%s exists", g_filelist_full);
		log_msg(1, "%s exists", g_biggielist_txt);
		log_msg(2,
				"Filelist and biggielist already recovered from media. Yay!");
	} else {
		getcwd(tmp, MAX_STR_LEN);
		chdir(bkpinfo->tmpdir);
		log_msg(1, "chdir(%s)", bkpinfo->tmpdir);
		log_to_screen("Extracting filelist and biggielist from media...");
		unlink("/tmp/filelist.full");
		unlink("/" FILELIST_FULL_STUB);
		unlink("/tmp/i-want-my-lvm");
		if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
			sprintf(command,
					"tar -zxf %s %s %s %s %s %s",
					bkpinfo->media_device,
					MOUNTLIST_FNAME_STUB,
					BIGGIELIST_TXT_STUB,
					FILELIST_FULL_STUB,
					"tmp/i-want-my-lvm", MONDO_CFG_FILE_STUB);
			log_msg(1, "tarcommand = %s", command);
			run_program_and_log_output(command, 1);
		} else {
			log_msg(2,
					"Calling insist_on_this_cd_number; bkpinfo->isodir=%s",
					bkpinfo->isodir);
			insist_on_this_cd_number(bkpinfo, 1);
			log_msg(2, "Back from iotcn");
			run_program_and_log_output("mount", 1);
			sprintf(command,
					"tar -zxf %s/images/all.tar.gz %s %s %s %s %s",
					MNT_CDROM,
					MOUNTLIST_FNAME_STUB,
					BIGGIELIST_TXT_STUB,
					FILELIST_FULL_STUB,
					"tmp/i-want-my-lvm", MONDO_CFG_FILE_STUB);

			log_msg(1, "tarcommand = %s", command);
			run_program_and_log_output(command, 1);
//    popup_and_OK("Press ENTER to continue");
			if (!does_file_exist(BIGGIELIST_TXT_STUB)) {
				fatal_error
					("all.tar.gz did not include tmp/biggielist.txt");
			}
			if (!does_file_exist(FILELIST_FULL_STUB)) {
				fatal_error
					("all.tar.gz did not include tmp/filelist.full.gz");
			}
		}
		sprintf(command, "cp -f %s %s", MONDO_CFG_FILE_STUB,
				g_mondo_cfg_file);
		run_program_and_log_output(command, FALSE);

		sprintf(command, "cp -f %s/%s %s", bkpinfo->tmpdir,
				BIGGIELIST_TXT_STUB, g_biggielist_txt);
		log_msg(1, "command = %s", command);
		paranoid_system(command);
		sprintf(command, "ln -sf %s/%s %s", bkpinfo->tmpdir,
				FILELIST_FULL_STUB, g_filelist_full);
		log_msg(1, "command = %s", command);
		paranoid_system(command);
	}

	if (am_I_in_disaster_recovery_mode()
		&&
		ask_me_yes_or_no("Do you want to retrieve the mountlist as well?"))
	{
//      sprintf(command, "cp -f tmp/mountlist.txt /tmp");
		sprintf(command, "ln -sf %s/%s /tmp", MOUNTLIST_FNAME_STUB,
				bkpinfo->tmpdir);
		paranoid_system(command);
	}

	chdir(tmp);

	if (!does_file_exist(g_biggielist_txt)) {
		log_msg(1, "Warning - %s not found", g_biggielist_txt);
	}
	if (!does_file_exist(g_filelist_full)) {
		log_msg(1, "Warning - %s does not exist", g_filelist_full);
	}
//  popup_and_OK("Wonderful.");

	log_msg(2, "Forking");
	pid = fork();
	switch (pid) {
	case -1:
		fatal_error("Forking error");
		break;

	case 0:
		log_to_screen("Pre-processing filelist");
		if (!does_file_exist(g_biggielist_txt)) {
			sprintf(command, "> %s", g_biggielist_txt);
			paranoid_system(command);
		}
		sprintf(command, "cat %s | grep  -x \"/dev/.*\" > %s",
				g_biggielist_txt, g_filelist_imagedevs);
		paranoid_system(command);
		exit(0);
		break;

	default:
		open_evalcall_form("Pre-processing filelist");
		while (!waitpid(pid, (int *) 0, WNOHANG)) {
			usleep(100000);
			update_evalcall_form(0);
		}
	}
	close_evalcall_form();

	log_msg(3, "loading filelist");
	filelist = load_filelist(g_filelist_full);
	log_msg(3, "deleting original filelist");
	unlink(g_filelist_full);
	if (g_text_mode) {
		printf("Restore which directory? --> ");
		fgets(tmp, sizeof(tmp), stdin);
		toggle_path_selection(filelist, tmp, TRUE);
		if (strlen(tmp) == 0) {
			res = 1;
		} else {
			res = 0;
		}
	} else {
		res = edit_filelist(filelist);
	}
	if (res) {
		log_msg(2, "User hit 'cancel'. Freeing filelist and aborting.");
		free_filelist(filelist);
		return (NULL);
	}
	ask_about_these_imagedevs(g_filelist_imagedevs, g_imagedevs_restthese);
	close_evalcall_form();

	// NB: It's not necessary to add g_biggielist_txt to the filelist.full
	// file. The filelist.full file already contains the filename of EVERY
	// file backed up - regular and biggie files.

	// However, we do want to make sure the imagedevs selected by the user
	// are flagged for restoring.
	if (length_of_file(g_imagedevs_restthese) > 2) {
		add_list_of_files_to_filelist(filelist, g_imagedevs_restthese,
									  TRUE);
	}

	paranoid_free(command);
	paranoid_free(tmp);
	return (filelist);
}

/**************************************************************************
 *END_ PROCESS_FILELIST_AND_BIGGIELIST                                    *
 **************************************************************************/




/**
 * Make a backup copy of <tt>path_root</tt>/<tt>filename</tt>.
 * The backup filename is the filename of the original with ".pristine" added.
 * @param path_root The place where the filesystem is mounted (e.g. MNT_RESTORING).
 * @param filename The filename (absolute path) within @p path_root.
 * @return 0 for success, nonzero for failure.
 */
int backup_crucial_file(char *path_root, char *filename)
{
	char *tmp;
	char *command;
	int res;

	malloc_string(tmp);
	malloc_string(command);
	assert(path_root != NULL);
	assert_string_is_neither_NULL_nor_zerolength(filename);

	sprintf(tmp, "%s/%s", path_root, filename);
	sprintf(command, "cp -f %s %s.pristine", tmp, tmp);

	res = run_program_and_log_output(command, 5);
	paranoid_free(tmp);
	paranoid_free(command);
	return (res);
}


/**
 * Install the user's boot loader in the MBR.
 * Currently LILO, ELILO, GRUB, RAW (dd of MBR), and the FreeBSD bootloader are supported.
 * @param offer_to_hack_scripts If TRUE, then offer to hack the user's fstab for them.
 * @return 0 for success, nonzero for failure.
 */
int run_boot_loader(bool offer_to_hack_scripts)
{
	int res;
	int retval = 0;

  /** malloc *******/
	char *device;
	char *tmp;
	char *name;

	malloc_string(device);
	malloc_string(tmp);
	malloc_string(name);
	backup_crucial_file(MNT_RESTORING, "/etc/fstab");
	backup_crucial_file(MNT_RESTORING, "/etc/grub.conf");
	backup_crucial_file(MNT_RESTORING, "/etc/lilo.conf");
	backup_crucial_file(MNT_RESTORING, "/etc/elilo.conf");
	read_cfg_var(g_mondo_cfg_file, "bootloader.device", device);
	read_cfg_var(g_mondo_cfg_file, "bootloader.name", name);
	sprintf(tmp, "run_boot_loader: device='%s', name='%s'", device, name);
	log_msg(2, tmp);
	system("sync");
	if (!strcmp(name, "LILO")) {
		res = run_lilo(offer_to_hack_scripts);
	} else if (!strcmp(name, "ELILO")) {
		res = run_elilo(offer_to_hack_scripts);
	} else if (!strcmp(name, "GRUB")) {
//      if ( does_file_exist(DO_MBR_PLEASE) || (offer_to_hack_scripts && ask_me_yes_or_no("Because of bugs in GRUB, you're much better off running mondorestore --mbr after this program terminates. Are you sure you want to install GRUB right now?")))
//        {
		res = run_grub(offer_to_hack_scripts, device);
//    unlink(DO_MBR_PLEASE);
//  }
//      else
//        {
//    log_msg(1, "Not running run_grub(). Was a bad idea anyway.");
//    res = 1;
//  }
	} else if (!strcmp(name, "RAW")) {
		res = run_raw_mbr(offer_to_hack_scripts, device);
	}
#ifdef __FreeBSD__
	else if (!strcmp(name, "BOOT0")) {
		sprintf(tmp, "boot0cfg -B %s", device);
		res = run_program_and_log_output(tmp, FALSE);
	} else {
		sprintf(tmp, "ls /dev | grep -xq %ss[1-4].*", device);
		if (!system(tmp)) {
			sprintf(tmp, MNT_RESTORING "/sbin/fdisk -B %s", device);
			res = run_program_and_log_output(tmp, 3);
		} else {
			log_msg(1,
					"I'm not running any boot loader. You have a DD boot drive. It's already loaded up.");
		}
	}
#else
	else {
		log_to_screen
			("Unable to determine type of boot loader. Defaulting to LILO.");
		res = run_lilo(offer_to_hack_scripts);
	}
#endif
	retval += res;
	if (res) {
		log_to_screen("Your boot loader returned an error");
	} else {
		log_to_screen("Your boot loader ran OK");
	}
	paranoid_free(device);
	paranoid_free(tmp);
	paranoid_free(name);
	return (retval);
}

/**************************************************************************
 *END_ RUN_BOOT_LOADER                                                    *
 **************************************************************************/



/**
 * Attempt to find the user's editor.
 * @return The editor found ("vi" if none could be found).
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *find_my_editor(void)
{
	static char output[MAX_STR_LEN];
	if (find_home_of_exe("pico")) {
		strcpy(output, "pico");
	} else if (find_home_of_exe("nano")) {
		strcpy(output, "nano");
	} else if (find_home_of_exe("e3em")) {
		strcpy(output, "e3em");
	} else if (find_home_of_exe("e3vi")) {
		strcpy(output, "e3vi");
	} else {
		strcpy(output, "vi");
	}
	if (!find_home_of_exe(output)) {
		log_msg(2, " (find_my_editor) --- warning - %s not found", output);
	}
	return (output);
}


/**
 * Install GRUB on @p bd.
 * @param offer_to_run_stabgrub If TRUE, then offer to hack the user's fstab for them.
 * @param bd The boot device where GRUB is installed.
 * @return 0 for success, nonzero for failure.
 */
int run_grub(bool offer_to_run_stabgrub, char *bd)
{
  /** malloc **/
	char *command;
	char *boot_device;
	char *rootdev;
	char *rootdrive;
	char *conffile;
	char *tmp;
	char *editor;

	int res;
	int done;

	malloc_string(command);
	malloc_string(boot_device);
	malloc_string(tmp);
	malloc_string(editor);
	malloc_string(rootdev);
	malloc_string(rootdrive);
	malloc_string(conffile);
	assert_string_is_neither_NULL_nor_zerolength(bd);
	strcpy(editor, "vi");		// find_my_editor() );
	strcpy(boot_device, bd);

	if (!run_program_and_log_output("which grub-MR", FALSE)) {
		log_msg(1, "Yay! grub-MR found...");
		sprintf(command, "grub-MR %s /tmp/mountlist.txt", boot_device);
		log_msg(1, "command = %s", command);
	} else {
		sprintf(command, "chroot " MNT_RESTORING " grub-install %s",
				boot_device);
		log_msg(1, "WARNING - grub-MR not found; using grub-install");
	}
	if (offer_to_run_stabgrub
		&& ask_me_yes_or_no("Did you change the mountlist?"))
		/* interactive mode */
	{
		mvaddstr_and_log_it(g_currentY,
							0,
							"Modifying fstab and grub.conf, and running GRUB...                             ");
		for (done = FALSE; !done;) {
			popup_and_get_string("Boot device",
								 "Please confirm/enter the boot device. If in doubt, try /dev/hda",
								 boot_device, MAX_STR_LEN / 4);
			sprintf(command, "stabgrub-me %s", boot_device);
			res = run_program_and_log_output(command, 1);
			if (res) {
				popup_and_OK
					("GRUB installation failed. Please install manually using 'grub-install' or similar command. You are now chroot()'ed to your restored system. Please type 'exit' when you are done.");
				newtSuspend();
				system("chroot " MNT_RESTORING);
				newtResume();
				popup_and_OK("Thank you.");
			} else {
				done = TRUE;
			}
			popup_and_OK("You will now edit fstab and grub.conf");
			if (!g_text_mode) {
				newtSuspend();
			}
			sprintf(tmp, "%s " MNT_RESTORING "/etc/fstab", editor);
			paranoid_system(tmp);
			sprintf(tmp, "%s " MNT_RESTORING "/etc/grub.conf", editor);
			paranoid_system(tmp);
			if (!g_text_mode) {
				newtResume();
			}
		}
	} else
		/* nuke mode */
	{
		mvaddstr_and_log_it(g_currentY,
							0,
							"Running GRUB...                                                 ");
		iamhere(command);
		res = run_program_and_log_output(command, 1);
		if (res) {
			popup_and_OK
				("Because of bugs in GRUB's own installer, GRUB was not installed properly. Please install the boot loader manually now, using this chroot()'ed shell prompt. Type 'exit' when you have finished.");
			newtSuspend();
			system("chroot " MNT_RESTORING);
			newtResume();
			popup_and_OK("Thank you.");
		}
	}
	if (res) {
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
		log_to_screen
			("GRUB ran w/error(s). See /tmp/mondo-restore.log for more info.");
		log_msg(1, "Type:-");
		log_msg(1, "    mount-me");
		log_msg(1, "    chroot " MNT_RESTORING);
		log_msg(1, "    mount /boot");
		log_msg(1, "    grub-install '(hd0)'");
		log_msg(1, "    exit");
		log_msg(1, "    unmount-me");
		log_msg(1,
				"If you're really stuck, please e-mail the mailing list.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(rootdev);
	paranoid_free(rootdrive);
	paranoid_free(conffile);
	paranoid_free(command);
	paranoid_free(boot_device);
	paranoid_free(tmp);
	paranoid_free(editor);

	return (res);
}

/**************************************************************************
 *END_RUN_GRUB                                                            *
 **************************************************************************/


/**
 * Install ELILO on the user's boot drive (determined by elilo.conf).
 * @param offer_to_run_stabelilo If TRUE, then offer to hack the user's fstab for them.
 * @return 0 for success, nonzero for failure.
 */
int run_elilo(bool offer_to_run_stabelilo)
{
  /** malloc **/
	char *command;
	char *tmp;
	char *editor;

	int res;
	int done;

	malloc_string(command);
	malloc_string(tmp);
	malloc_string(editor);
	strcpy(editor, find_my_editor());
	if (offer_to_run_stabelilo
		&& ask_me_yes_or_no("Did you change the mountlist?"))

		/* interactive mode */
	{
		mvaddstr_and_log_it(g_currentY,
							0,
							"Modifying fstab and elilo.conf...                             ");
		sprintf(command, "stabelilo-me");
		res = run_program_and_log_output(command, 3);
		if (res) {
			popup_and_OK
				("You will now edit fstab and elilo.conf, to make sure they match your new mountlist.");
			for (done = FALSE; !done;) {
				if (!g_text_mode) {
					newtSuspend();
				}
				sprintf(tmp, "%s " MNT_RESTORING "/etc/fstab", editor);
				paranoid_system(tmp);
				sprintf(tmp, "%s " MNT_RESTORING "/etc/elilo.conf",
						editor);
				paranoid_system(tmp);
				if (!g_text_mode) {
					newtResume();
				}
//              newtCls();
				if (ask_me_yes_or_no("Edit them again?")) {
					continue;
				}
				done = TRUE;
			}
		} else {
			log_to_screen("elilo.conf and fstab were modified OK");
		}
	} else
		/* nuke mode */
	{
		res = TRUE;
	}
	paranoid_free(command);
	paranoid_free(tmp);
	paranoid_free(editor);
	return (res);
}

/**************************************************************************
 *END_RUN_ELILO                                                            *
 **************************************************************************/


/**
 * Install LILO on the user's boot drive (determined by /etc/lilo.conf).
 * @param offer_to_run_stablilo If TRUE, then offer to hack the user's fstab for them.
 * @return 0 for success, nonzero for failure.
 */
int run_lilo(bool offer_to_run_stablilo)
{
  /** malloc **/
	char *command;
	char *tmp;
	char *editor;

	int res;
	int done;
	bool run_lilo_M = FALSE;
	malloc_string(command);
	malloc_string(tmp);
	malloc_string(editor);

	if (!run_program_and_log_output
		("grep \"boot.*=.*/dev/md\" " MNT_RESTORING "/etc/lilo.conf", 1)) {
		run_lilo_M = TRUE;
	}

	strcpy(editor, find_my_editor());
	if (offer_to_run_stablilo
		&& ask_me_yes_or_no("Did you change the mountlist?"))

		/* interactive mode */
	{
		mvaddstr_and_log_it(g_currentY,
							0,
							"Modifying fstab and lilo.conf, and running LILO...                             ");
		sprintf(command, "stablilo-me");
		res = run_program_and_log_output(command, 3);
		if (res) {
			popup_and_OK
				("You will now edit fstab and lilo.conf, to make sure they match your new mountlist.");
			for (done = FALSE; !done;) {
				if (!g_text_mode) {
					newtSuspend();
				}
				sprintf(tmp, "%s " MNT_RESTORING "/etc/fstab", editor);
				paranoid_system(tmp);
				sprintf(tmp, "%s " MNT_RESTORING "/etc/lilo.conf", editor);
				paranoid_system(tmp);
				if (!g_text_mode) {
					newtResume();
				}
//              newtCls();
				if (ask_me_yes_or_no("Edit them again?")) {
					continue;
				}
				res =
					run_program_and_log_output("chroot " MNT_RESTORING
											   " lilo -L", 3);
				if (res) {
					res =
						run_program_and_log_output("chroot " MNT_RESTORING
												   " lilo", 3);
				}
				if (res) {
					done =
						ask_me_yes_or_no
						("LILO failed. Re-edit system files?");
				} else {
					done = TRUE;
				}
			}
		} else {
			log_to_screen("lilo.conf and fstab were modified OK");
		}
	} else
		/* nuke mode */
	{
		mvaddstr_and_log_it(g_currentY,
							0,
							"Running LILO...                                                 ");
		res =
			run_program_and_log_output("chroot " MNT_RESTORING " lilo -L",
									   3);
		if (res) {
			res =
				run_program_and_log_output("chroot " MNT_RESTORING " lilo",
										   3);
		}
		if (res) {
			mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
			log_to_screen
				("Failed to re-jig fstab and/or lilo. Edit/run manually, please.");
		} else {
			mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		}
	}
	if (run_lilo_M) {
		run_program_and_log_output("chroot " MNT_RESTORING
								   " lilo -M /dev/hda", 3);
		run_program_and_log_output("chroot " MNT_RESTORING
								   " lilo -M /dev/sda", 3);
	}
	paranoid_free(command);
	paranoid_free(tmp);
	paranoid_free(editor);
	return (res);
}

/**************************************************************************
 *END_RUN_LILO                                                            *
 **************************************************************************/


/**
 * Install a raw MBR onto @p bd.
 * @param offer_to_hack_scripts If TRUE, then offer to hack the user's fstab for them.
 * @param bd The device to copy the stored MBR to.
 * @return 0 for success, nonzero for failure.
 */
int run_raw_mbr(bool offer_to_hack_scripts, char *bd)
{
  /** malloc **/
	char *command;
	char *boot_device;
	char *tmp;
	char *editor;
	int res;
	int done;

	malloc_string(command);
	malloc_string(boot_device);
	malloc_string(tmp);
	malloc_string(editor);
	assert_string_is_neither_NULL_nor_zerolength(bd);

	strcpy(editor, find_my_editor());
	strcpy(boot_device, bd);
	sprintf(command, "raw-MR %s /tmp/mountlist.txt", boot_device);
	log_msg(2, "run_raw_mbr() --- command='%s'", command);

	if (offer_to_hack_scripts
		&& ask_me_yes_or_no("Did you change the mountlist?"))
		/* interactive mode */
	{
		mvaddstr_and_log_it(g_currentY, 0,
							"Modifying fstab and restoring MBR...                           ");
		for (done = FALSE; !done;) {
			if (!run_program_and_log_output("which vi", FALSE)) {
				popup_and_OK("You will now edit fstab");
				if (!g_text_mode) {
					newtSuspend();
				}
				sprintf(tmp, "%s " MNT_RESTORING "/etc/fstab", editor);
				paranoid_system(tmp);
				if (!g_text_mode) {
					newtResume();
				}
//              newtCls();
			}
			popup_and_get_string("Boot device",
								 "Please confirm/enter the boot device. If in doubt, try /dev/hda",
								 boot_device, MAX_STR_LEN / 4);
			sprintf(command, "stabraw-me %s", boot_device);
			res = run_program_and_log_output(command, 3);
			if (res) {
				done = ask_me_yes_or_no("Modifications failed. Re-try?");
			} else {
				done = TRUE;
			}
		}
	} else
		/* nuke mode */
	{
		mvaddstr_and_log_it(g_currentY, 0,
							"Restoring MBR...                                               ");
		res = run_program_and_log_output(command, 3);
	}
	if (res) {
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
		log_to_screen
			("MBR+fstab processed w/error(s). See /tmp/mondo-restore.log for more info.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(command);
	paranoid_free(boot_device);
	paranoid_free(tmp);
	paranoid_free(editor);
	return (res);
}

/**************************************************************************
 *END_RUN_RAW_MBR                                                         *
 **************************************************************************/





/**
 * Turn signal trapping on or off.
 * @param on If TRUE, then do full cleanup when we receive a signal; if FALSE, then 
 * print a message and exit immediately.
 */
void set_signals(int on)
{
	int signals[] =
		{ SIGKILL, SIGPIPE, SIGTERM, SIGHUP, SIGTRAP, SIGABRT, SIGINT,
		SIGSTOP, 0
	};
	int i;
	for (i = 0; signals[i]; i++) {
		if (on) {
			signal(signals[i], terminate_daemon);
		} else {
			signal(signals[i], termination_in_progress);
		}
	}
}

/**************************************************************************
 *END_SET_SIGNALS                                                         *
 **************************************************************************/


/**
 * malloc() and set sensible defaults for the mondorestore filename variables.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->tmpdir
 * - @c bkpinfo->disaster_recovery
 */
void setup_MR_global_filenames(struct s_bkpinfo *bkpinfo)
{
	char *temppath;

	assert(bkpinfo != NULL);

	malloc_string(g_biggielist_txt);
	malloc_string(g_filelist_full);
	malloc_string(g_filelist_imagedevs);
	malloc_string(g_imagedevs_restthese);
	malloc_string(g_mondo_cfg_file);
	malloc_string(g_mountlist_fname);
	malloc_string(g_tmpfs_mountpt);
	malloc_string(g_isodir_device);
	malloc_string(g_isodir_format);

	temppath = bkpinfo->tmpdir;

	sprintf(g_biggielist_txt, "%s/%s", temppath, BIGGIELIST_TXT_STUB);
	sprintf(g_filelist_full, "%s/%s", temppath, FILELIST_FULL_STUB);
	sprintf(g_filelist_imagedevs, "%s/tmp/filelist.imagedevs", temppath);
//  sprintf(g_imagedevs_pot, "%s/tmp/imagedevs.pot", temppath);
	sprintf(g_imagedevs_restthese, "%s/tmp/imagedevs.restore-these",
			temppath);
	if (bkpinfo->disaster_recovery) {
		sprintf(g_mondo_cfg_file, "/%s", MONDO_CFG_FILE_STUB);
		sprintf(g_mountlist_fname, "/%s", MOUNTLIST_FNAME_STUB);
	} else {
		sprintf(g_mondo_cfg_file, "%s/%s", temppath, MONDO_CFG_FILE_STUB);
		sprintf(g_mountlist_fname, "%s/%s", temppath,
				MOUNTLIST_FNAME_STUB);
	}
}

/**************************************************************************
 *END_SET_GLOBAL_FILENAME                                                 *
 **************************************************************************/


/**
 * Copy @p input_file (containing the result of a compare) to @p output_file,
 * deleting spurious "changes" along the way.
 * @param output_file The output file to write with spurious changes removed.
 * @param input_file The input file, a list of changed files created by a compare.
 */
void streamline_changes_file(char *output_file, char *input_file)
{
	FILE *fin;
	FILE *fout;
  /** malloc **/
	char *incoming;

	assert_string_is_neither_NULL_nor_zerolength(output_file);
	assert_string_is_neither_NULL_nor_zerolength(input_file);
	malloc_string(incoming);

	if (!(fin = fopen(input_file, "r"))) {
		log_OS_error(input_file);
		return;
	}
	if (!(fout = fopen(output_file, "w"))) {
		fatal_error("cannot open output_file");
	}
	for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin);
		 fgets(incoming, MAX_STR_LEN - 1, fin)) {
		if (strncmp(incoming, "etc/adjtime", 11)
			&& strncmp(incoming, "etc/mtab", 8)
			&& strncmp(incoming, "tmp/", 4)
			&& strncmp(incoming, "boot/map", 8)
			&& !strstr(incoming, "incheckentry")
			&& strncmp(incoming, "etc/mail/statistics", 19)
			&& strncmp(incoming, "var/", 4))
			fprintf(fout, "%s", incoming);	/* don't need \n here, for some reason.. */
	}
	paranoid_fclose(fout);
	paranoid_fclose(fin);
	paranoid_free(incoming);
}

/**************************************************************************
 *END_STREAMLINE_CHANGES_FILE                                             *
 **************************************************************************/






/**
 * Prompt the user to support the Mondo project.
 */
void success_message(void)
{
	int i;
	/* malloc and ptr */
	char *tmp;

	malloc_string(tmp);
	if (strstr
		(call_program_and_get_last_line_of_output("cat /proc/cmdline"),
		 "restore") == NULL) {
		if (ask_me_yes_or_no
			("Have you contributed to the Mondo project financially or in some other way, yet?"))
		{
			log_to_screen
				("Thank you for supporting Mondo. It goes from strength to strength,");
			log_to_screen("thanks to the support of users like you.");
		} else {
			if (ask_me_yes_or_no
				("Are you or your company willing to consider contributing to Mondo in some way?"))
			{
				popup_and_OK
					("To support the project which has just performed a valuable service for you, please visit http://www.mondorescue.com; click on 'Download' and then 'PayPal'.");
			} else {
				log_to_screen
					("Free Software, like freedom itself, must be supported or it will be lost.");
				log_msg(1,
						"To your credit, you were honest: you said no, you wouldn't be contributing");
				log_msg(1,
						"to this project, ever. However, that makes you a freeloader. I bet you're");
				log_msg(1,
						"the sort of person who likes to sneak into movie theatres...");
				popup_and_OK
					("If you ever change your mind, you may support this product by going to http://www.mondrescue.com and clicking on 'Download', followed by 'PalPal'.");
			}
		}
	}
#ifdef FREELOADER
	i = (int) (random()) % 32;
#else
	i = 1;
#endif

	if (i != 25) {
		strcpy(tmp,
			   "Mondo has restored your system. Please remove the backup media and reboot.");
	} else {
		strcpy(tmp,
			   "M0nd0 h45 r3570r3d j00r 5y573m. P13453 r3m0v3 7h3 b4ckup m3d14 4nd r3b007.");
	}
	if (strstr
		(call_program_and_get_last_line_of_output("cat /proc/cmdline"),
		 "restore") == NULL) {
		popup_and_OK(tmp);
	}
	log_to_screen(tmp);
	paranoid_free(tmp);
}

/**************************************************************************
 *END_SUCCESS_MESSAGE                                                     *
 **************************************************************************/



/**
 * Exit due to a signal (normal cleanup).
 * @param sig The signal we're exiting due to.
 */
void terminate_daemon(int sig)
{
	log_to_screen
		("Mondorestore is terminating in response to a signal from the OS");
	paranoid_MR_finish(254);
}

/**************************************************************************
 *END_TERMINATE_DAEMON                                                    *
 **************************************************************************/


/**
 * Give the user twenty seconds to press Ctrl-Alt-Del before we nuke their drives.
 */
void twenty_seconds_til_yikes()
{
	int i;
	/* MALLOC * */
	char *tmp;

	malloc_string(tmp);
	if (does_file_exist("/tmp/NOPAUSE")) {
		return;
	}
	open_progress_form("CAUTION",
					   "Be advised: I am about to ERASE your hard disk(s)!",
					   "You may press Ctrl+Alt+Del to abort safely.",
					   "", 20);
	for (i = 0; i < 20; i++) {
		g_current_progress = i;
		sprintf(tmp, "You have %d seconds left to abort.", 20 - i);
		update_progress_form(tmp);
		sleep(1);
	}
	close_progress_form();
	paranoid_free(tmp);
}

/**************************************************************************
 *END_TWENTY_SECONDS_TIL_YIKES                                            *
 **************************************************************************/





/**
 * Exit due to a signal (no cleanup).
 * @param sig The signal we're exiting due to.
 */
void termination_in_progress(int sig)
{
	log_msg(1, "Termination in progress");
	usleep(1000);
	pthread_exit(0);
}

/**************************************************************************
 *END_TERMINATION_IN_PROGRESS                                             *
 **************************************************************************/



/**
 * Unmount all devices in @p p_external_copy_of_mountlist.
 * @param p_external_copy_of_mountlist The mountlist to guide the devices to unmount.
 * @return 0 for success, nonzero for failure.
 */
int unmount_all_devices(struct mountlist_itself
						*p_external_copy_of_mountlist)
{
	struct mountlist_itself *mountlist;
	int retval = 0, lino, res = 0, i;
	char *command;
	char *tmp;

	malloc_string(command);
	malloc_string(tmp);
	assert(p_external_copy_of_mountlist != NULL);

	mountlist = malloc(sizeof(struct mountlist_itself));
	memcpy((void *) mountlist, (void *) p_external_copy_of_mountlist,
		   sizeof(struct mountlist_itself));
	sort_mountlist_by_mountpoint(mountlist, 0);

	run_program_and_log_output("df -m", 3);
	mvaddstr_and_log_it(g_currentY, 0, "Unmounting devices      ");
	open_progress_form("Unmounting devices",
					   "Unmounting all devices that were mounted,",
					   "in preparation for the post-restoration reboot.",
					   "", mountlist->entries);
	chdir("/");
	for (i = 0;
		 i < 10
		 &&
		 run_program_and_log_output
		 ("ps wax | grep buffer | grep -v \"grep buffer\"", TRUE) == 0;
		 i++) {
		sleep(1);
		log_msg(2, "Waiting for buffer() to finish");
	}

	paranoid_system("sync");

	if (run_program_and_log_output
		("cp -f /tmp/mondo-restore.log " MNT_RESTORING "/tmp/", FALSE)) {
		log_msg(1,
				"Error. Failed to copy log to PC's /tmp dir. (Mounted read-only?)");
	}
	if (run_program_and_log_output
		("cp -f /tmp/mondo-restore.log " MNT_RESTORING "/root/", FALSE)) {
		log_msg(1,
				"Error. Failed to copy log to PC's /root dir. (Mounted read-only?)");
	}
	if (does_file_exist("/tmp/DUMBASS-GENTOO")) {
		run_program_and_log_output("mkdir -p " MNT_RESTORING
								   "/mnt/.boot.d", 5);
	}
	for (lino = mountlist->entries - 1; lino >= 0; lino--) {
		if (!strcmp(mountlist->el[lino].mountpoint, "lvm")) {
			continue;
		}
		sprintf(tmp, "Unmounting device %s  ", mountlist->el[lino].device);

		update_progress_form(tmp);
		if (is_this_device_mounted(mountlist->el[lino].device)) {
			if (!strcmp(mountlist->el[lino].mountpoint, "swap")) {
				sprintf(command, "swapoff %s", mountlist->el[lino].device);
			} else {
				if (!strcmp(mountlist->el[lino].mountpoint, "/1")) {
					sprintf(command, "umount %s/", MNT_RESTORING);
					log_msg(3,
							"Well, I know a certain kitty-kitty who'll be sleeping with Mommy tonight...");
				} else {
					sprintf(command, "umount " MNT_RESTORING "%s",
							mountlist->el[lino].mountpoint);
				}
			}
			log_msg(10, "The 'umount' command is '%s'", command);
			res = run_program_and_log_output(command, 3);
		} else {
			strcat(tmp, "...not mounted anyway :-) OK");
			res = 0;
		}
		g_current_progress++;
		if (res) {
			strcat(tmp, "...Failed");
			retval++;
			log_to_screen(tmp);
		} else {
			log_msg(2, tmp);
		}
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	if (retval) {
		log_to_screen("Unable to unmount some of your partitions.");
	} else {
		log_to_screen("All partitions were unmounted OK.");
	}
	free(mountlist);
	paranoid_free(command);
	paranoid_free(tmp);
	return (retval);
}

/**************************************************************************
 *END_UNMOUNT_ALL_DEVICES                                                 *
 **************************************************************************/



/**
 * Extract mondo-restore.cfg and the mountlist from the tape inserted
 * to the ./tmp/ directory.
 * @param dev The tape device to read from.
 * @return 0 for success, nonzero for failure.
 */
int extract_cfg_file_and_mountlist_from_tape_dev(char *dev)
{
	char *command;
	int res = 0;
	// BCO: below 32KB seems to block at least on RHAS 2.1 and MDK 10.0
	long tape_block_size = 32768;

	malloc_string(command);

	// tar -zxvf-
	sprintf(command,
			"dd if=%s bs=%ld count=%ld 2> /dev/null | tar -zx %s %s %s %s %s",
			dev,
			tape_block_size,
			1024L * 1024 * 32 / tape_block_size,
			MOUNTLIST_FNAME_STUB, MONDO_CFG_FILE_STUB,
			BIGGIELIST_TXT_STUB, FILELIST_FULL_STUB, "tmp/i-want-my-lvm");
	log_msg(2, "command = '%s'", command);
	res = run_program_and_log_output(command, -1);
	if (res != 0 && does_file_exist(MONDO_CFG_FILE_STUB)) {
		res = 0;
	}
	paranoid_free(command);
	return (res);
}



/**
 * Get the configuration file from the floppy, tape, or CD.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->media_device
 * - @c bkpinfo->tmpdir
 * @return 0 for success, nonzero for failure.
 */
int get_cfg_file_from_archive(struct s_bkpinfo *bkpinfo)
{
	int retval = 0;

   /** malloc *****/
	char *device;
	char *command;
	char *cfg_file;
	char *mounted_cfgf_path;
	char *tmp;
	char *sav;
	char *mountpt;
	char *ramdisk_fname;
	char *mountlist_file;
	int res;

	bool try_plan_B;

	assert(bkpinfo != NULL);
	malloc_string(cfg_file);
	malloc_string(mounted_cfgf_path);
	malloc_string(mountpt);
	malloc_string(ramdisk_fname);
	malloc_string(mountlist_file);
	malloc_string(device);
	malloc_string(command);
	malloc_string(tmp);
	log_msg(2, "gcffa --- starting");
	log_to_screen("I'm thinking...");
	sprintf(mountpt, "%s/mount.bootdisk", bkpinfo->tmpdir);
	device[0] = '\0';
	chdir(bkpinfo->tmpdir);
	strcpy(cfg_file, MONDO_CFG_FILE_STUB);
	unlink(cfg_file);			// cfg_file[] is missing the '/' at the start, FYI, by intent
	unlink(FILELIST_FULL_STUB);
	unlink(BIGGIELIST_TXT_STUB);
	unlink("tmp/i-want-my-lvm");
	sprintf(command, "mkdir -p %s", mountpt);
	run_program_and_log_output(command, FALSE);

//   unlink( "tmp/mondo-restore.cfg" ); // superfluous, surely?

	sprintf(cfg_file, "%s/%s", bkpinfo->tmpdir, MONDO_CFG_FILE_STUB);
	sprintf(mountlist_file, "%s/%s", bkpinfo->tmpdir,
			MOUNTLIST_FNAME_STUB);
	//   make_hole_for_file( cfg_file );
	//   make_hole_for_file( mountlist_file);
	log_msg(2, "mountpt = %s; cfg_file=%s", mountpt, cfg_file);

	/* Floppy? */
	sprintf(tmp, "mkdir -p %s", mountpt);
	run_program_and_log_output(tmp, FALSE);
	sprintf(tmp, "mkdir -p %s/tmp", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, FALSE);

	sprintf(command, "mount /dev/fd0u1722 %s", mountpt);
	sprintf(tmp,
			"(sleep 15; kill `ps ax | grep \"%s\" | cut -d' ' -f1` 2> /dev/null) &",
			command);
	log_msg(1, "tmp = '%s'", tmp);
	system(tmp);
	res = run_program_and_log_output(command, FALSE);
	if (res) {
		sprintf(command, "mount /dev/fd0H1440 %s", mountpt);
		res = run_program_and_log_output(command, FALSE);
	}
	if (res) {
		try_plan_B = TRUE;
	} else {
		try_plan_B = TRUE;
		log_msg(2,
				"Mounted floppy OK but I don't trust it because the archives might contain more up-to-date config file than the floppy does.");
// NB: If busybox does not support 'mount -o loop' then Plan A WILL NOT WORK.
		log_msg(2, "Processing floppy (plan A?)");
		sprintf(ramdisk_fname, "%s/mindi.rdz", mountpt);
		if (!does_file_exist(ramdisk_fname)) {
			sprintf(ramdisk_fname, "%s/initrd.img", mountpt);
		}
		if (!does_file_exist(ramdisk_fname)) {
			log_msg(2,
					"Cannot find ramdisk file on mountpoint. Are you sure that's a boot disk in the drive?");
		}
		if (extract_config_file_from_ramdisk
			(bkpinfo, ramdisk_fname, cfg_file, mountlist_file)) {
			log_msg(2,
					"Warning - failed to extract config file from ramdisk. I think this boot disk is mangled.");
		}
		sprintf(command, "umount %s", mountpt);
		run_program_and_log_output(command, 5);
		unlink(ramdisk_fname);
	}
	if (!does_file_exist(cfg_file)) {
		log_msg(2, "gcffa --- we don't have cfg file yet.");
		if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
			try_plan_B = TRUE;
		} else {
			log_msg(2, "gcffa --- calling mount_cdrom now :)");
			if (!mount_cdrom(bkpinfo)) {
				log_msg(2,
						"gcffa --- managed to mount CD; so, no need for Plan B");
				try_plan_B = FALSE;
			} else {
				try_plan_B = TRUE;
			}
			if (what_number_cd_is_this(bkpinfo) > 1) {
				insist_on_this_cd_number(bkpinfo,
										 (g_current_media_number = 1));
			}
		}
		if (try_plan_B) {
			log_msg(2, "gcffa --- OK, switching to Plan B");
			chdir(bkpinfo->tmpdir);
			run_program_and_log_output("mkdir -p tmp", FALSE);

			if (strlen(bkpinfo->media_device) == 0) {
				paranoid_alloc(bkpinfo->media_device, "/dev/st0");
				log_msg(2, "media_device is blank; assuming %s", bkpinfo->media_device);
			}
			asprintf(&sav,bkpinfo->media_device);
			if (extract_cfg_file_and_mountlist_from_tape_dev
				(bkpinfo->media_device)) {
				paranoid_alloc(bkpinfo->media_device, "/dev/st0");
				if (extract_cfg_file_and_mountlist_from_tape_dev
					(bkpinfo->media_device)) {
					paranoid_alloc(bkpinfo->media_device, "/dev/osst0");
					if (extract_cfg_file_and_mountlist_from_tape_dev
						(bkpinfo->media_device)) {
						paranoid_alloc(bkpinfo->media_device, "/dev/ht0");
						if (extract_cfg_file_and_mountlist_from_tape_dev
							(bkpinfo->media_device)) {
							log_msg(3,
									"I tried lots of devices but none worked.");
							paranoid_alloc(bkpinfo->media_device, sav);
						}
					}
				}
			}
			paranoid_free(sav);

			if (!does_file_exist("tmp/mondo-restore.cfg")) {
				log_to_screen("Cannot find config info on tape/CD/floppy");
				return (1);
			}
		} else {
			log_msg(2,
					"gcffa --- looking at mounted CD for mindi-boot.2880.img");
			sprintf(command,
					"mount " MNT_CDROM
					"/images/mindi-boot.2880.img -o loop %s", mountpt);
			sprintf(mounted_cfgf_path, "%s/%s", mountpt, cfg_file);
			if (!does_file_exist(mounted_cfgf_path)) {
				log_msg(2,
						"gcffa --- Plan C, a.k.a. untarring some file from all.tar.gz");
				sprintf(command, "tar -zxvf " MNT_CDROM "/images/all.tar.gz %s %s %s %s %s", MOUNTLIST_FNAME_STUB, MONDO_CFG_FILE_STUB, BIGGIELIST_TXT_STUB, FILELIST_FULL_STUB, "tmp/i-want-my-lvm");	// add -b TAPE_BLOCK_SIZE if you _really_ think it's necessary
				run_program_and_log_output(command, TRUE);
				if (!does_file_exist(MONDO_CFG_FILE_STUB)) {
					fatal_error
						("Please reinsert the disk/CD and try again.");
				}
			}
		}
	}
	if (does_file_exist(MONDO_CFG_FILE_STUB)) {
		log_msg(1, "gcffa --- great! We've got the config file");
		sprintf(tmp, "%s/%s",
				call_program_and_get_last_line_of_output("pwd"),
				MONDO_CFG_FILE_STUB);
		sprintf(command, "cp -f %s %s", tmp, cfg_file);
		iamhere(command);
		if (strcmp(tmp, cfg_file)
			&& run_program_and_log_output(command, 1)) {
			log_msg(1,
					"... but an error occurred when I tried to move it to %s",
					cfg_file);
		} else {
			log_msg(1, "... and I moved it successfully to %s", cfg_file);
		}
		sprintf(command, "cp -f %s/%s %s",
				call_program_and_get_last_line_of_output("pwd"),
				MOUNTLIST_FNAME_STUB, mountlist_file);
		iamhere(command);
		if (strcmp(tmp, cfg_file)
			&& run_program_and_log_output(command, 1)) {
			log_msg(1, "Failed to get mountlist");
		} else {
			log_msg(1, "Got mountlist too");
			sprintf(command, "cp -f %s %s", mountlist_file,
					g_mountlist_fname);
			if (run_program_and_log_output(command, 1)) {
				log_msg(1, "Failed to copy mountlist to /tmp");
			} else {
				log_msg(1, "Copied mountlist to /tmp as well OK");
				sprintf(command, "cp -f tmp/i-want-my-lvm /tmp/");
				run_program_and_log_output(command, 1);
/*	      sprintf(command, "grep \" lvm \" %s", g_mountlist_fname);
	      if (!run_program_and_log_output(command, 5) && !does_file_exist("/tmp/i-want-my-lvm"))
	        {
		  log_msg(1, "Warning. You want LVM but I don't have i-want-my-lvm. FIXME.");
		}
              else if (run_program_and_log_output(command,5) && does_file_exist("/tmp/i-want-my-lvm"))
                {
		  log_msg(1, "Warning. You don't want LVM but i-want-my-lvm exists. I'll delete it. Cool.");
	          do_my_funky_lvm_stuff(TRUE, FALSE); // ...after I stop any LVMs :)
		  stop_raid_device("/dev/md0");
		  stop_raid_device("/dev/md1");
		  stop_raid_device("/dev/md2");
		  unlink("/tmp/i-want-my-lvm");
		}
 	      else if (!run_program_and_log_output(command,5) && does_file_exist("/tmp/i-want-my-lvm"))
	        {
		  log_msg(1, "You had better pray that i-want-my-lvm patches your mountlist. FIXME.");
		}
*/
			}
		}
	}
	run_program_and_log_output("umount " MNT_CDROM, FALSE);
	if (!does_file_exist(cfg_file)) {
		iamhere(cfg_file);
		log_msg(1, "%s not found", cfg_file);
		log_to_screen
			("Oh dear. Unable to recover configuration file from boot disk");
		return (1);
	}

	log_to_screen("Recovered mondo-restore.cfg");
	if (!does_file_exist(MOUNTLIST_FNAME_STUB)) {
		log_to_screen("...but not mountlist.txt - a pity, really...");
	}
/* start SAH */
	else {
		sprintf(command, "cp -f %s %s/%s", MOUNTLIST_FNAME_STUB,
				bkpinfo->tmpdir, MOUNTLIST_FNAME_STUB);
		run_program_and_log_output(command, FALSE);
	}
/*--commented out by SAH
  sprintf( command, "cp -f %s %s/%s", cfg_file, bkpinfo->tmpdir, MONDO_CFG_FILE_STUB );
  run_program_and_log_output( command, FALSE );
  sprintf( command, "cp -f %s %s/%s", mountlist_file, bkpinfo->tmpdir, MOUNTLIST_FNAME_STUB );
  run_program_and_log_output( command, FALSE );
*/
/* end SAH */

	sprintf(command, "cp -f %s /%s", cfg_file, MONDO_CFG_FILE_STUB);
	run_program_and_log_output(command, FALSE);
	sprintf(command, "cp -f %s /%s", mountlist_file, MOUNTLIST_FNAME_STUB);
	run_program_and_log_output(command, FALSE);
	sprintf(command, "cp -f etc/raidtab /etc/");
	run_program_and_log_output(command, FALSE);
	sprintf(command, "cp -f tmp/i-want-my-lvm /tmp/");
	run_program_and_log_output(command, FALSE);
	g_backup_media_type = bkpinfo->backup_media_type;
	paranoid_free(device);
	paranoid_free(command);
	paranoid_free(tmp);
	paranoid_free(cfg_file);
	paranoid_free(mounted_cfgf_path);
	paranoid_free(mountpt);
	paranoid_free(ramdisk_fname);
	paranoid_free(mountlist_file);
	return (retval);
}

/**************************************************************************
 *END_GET_CFG_FILE_FROM_ARCHIVE                                           *
 **************************************************************************/

/* @} - end restoreUtilityGroup */


/***************************************************************************
 * F@                                                                      *
 * () -- Hugo Rabson                                  *
 *                                                                         *
 * Purpose:                                                                *
 *                                                                         *
 * Called by:                                                              *
 * Params:    -                      -                                     *
 * Returns:   0=success; nonzero=failure                                   *
 ***************************************************************************/



void wait_until_software_raids_are_prepped(char *mdstat_file,
										   int wait_for_percentage)
{
	struct s_mdstat *mdstat;
	int unfinished_mdstat_devices = 9999, i;
	char *screen_message;

	malloc_string(screen_message);
	mdstat = malloc(sizeof(struct s_mdstat));

	assert(wait_for_percentage <= 100);
	iamhere("Help, my boat is sync'ing. (Get it? Urp! Urp!)");
	while (unfinished_mdstat_devices > 0) {
		if (read_mdstat(mdstat, mdstat_file)) {
			log_to_screen("Sorry, cannot read %s", mdstat_file);
			return;
		}
		for (unfinished_mdstat_devices = i = 0; i < mdstat->entries; i++) {
			if (mdstat->el[i].progress < wait_for_percentage) {
				unfinished_mdstat_devices++;
				sprintf(screen_message, "Sync'ing /dev/md%d",
						mdstat->el[i].md);
				open_evalcall_form(screen_message);
				if (mdstat->el[i].progress == -1)	// delayed while another partition inits
				{
					continue;
				}
				while (mdstat->el[i].progress < wait_for_percentage) {
					update_evalcall_form(mdstat->el[i].progress);
					sleep(2);
					if (read_mdstat(mdstat, mdstat_file)) {
						break;
					}
				}
				close_evalcall_form();
			}
		}
	}
	paranoid_free(screen_message);
	paranoid_free(mdstat);
}
