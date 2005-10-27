/***************************************************************************
mondo-cli.c
-------------------
begin                : Fri Apr 19 16:40:35 EDT 2002
copyright	     : (C) 2002 Mondo  Hugo Rabson
email                : Hugo Rabson <hugorabson@msn.com>
edited by            : by Stan Benoit 4/2002
email                : troff@nakedsoul.org
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
UPDATE LOG


08/04
- if user specifies a dumb -T value, abort

07/22
- handle -Q sensibly (don't abort if only -Q supplied)

07/17
- better checking of NFS dir's validity

06/19
- added AUX_VER

04/17
- added '-b' support
04/03
- added star support

02/06
- fixed "Please give /dev entry" error msg

01/20/2004
- added 2.6.x-kernel "device = /dev/...." support to param reader
- better handling of SCSI and /dev entries
- touch /boot/boot.b if necessary

09/26/2003
- typo in command-line handling of 'r'

09/25
- added DVD write support

09/24
- if tape backup (-t) but user doesn't specify tape drive then
  make an educated guess using find_tape_device_and_size()

09/18
- insist on partimagehack being present if -x found

09/16
- added support for multiple -I's and -E's in call to mondoarchive
- the '-D' flag doesn't take a value now

07/14
- fatal error if -E too short

05/14
- if 'n' and user doesn't have rights to write to output dir
  then error out

05/05
- added Joshua Oreman's FreeBSD patches

05/04
- added Herman Kuster's multi-level bkp patch

05/02
- write errors to screen, not just to log

04/25
- added lots of assert()'s and log_OS_error()'s

04/21
- line 570 --- resolve boot device if softlink

04/08
- changed a bunch of fprintf(stderr,...)'s to log_it()'s
- fixed final / removal in retrieve_switches_from_command_line()

04/04
- added -e

03/15
- reject relative paths if -d flag (Alessandro Polverini)

01/02/2003
- added -J flag (to let user specify incoming filelist)

12/13/2002
- various strcpy() calls replaced with strncpy() calls

12/10
- added g_loglevel

11/25
- line 614 --- if commmand-line param ends in '/' then drop that '/'

11/19
- rewrote finish() to kill processes more softly
- if user calls with '-l RAW' then 

09/01 - 09/30
- added -N to let user exclude all NFS-related mounts and devices
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_

08/01 - 08/31
- if no device specified when backing up to tape streamer then assume /dev/st0
- abort if tape user specifies tape size (just for testing porpoises)
- tape users no longer need to specify tape size
- added some comments
- added bkpinfo->backup_media_type
- renamed from mondo-archive.c to mondo-cli.c

07/01 - 07/31
- better check for mkfs.vfat
- more sanity-checking for -d flag
- if isodir does not exist then abort
- removed "the rest are..." comment from log
- do not permit '-H' and '-t' in the same command line
- if not '-o' then insist on vfat-friendly kernel

06/01 - 06/30
- added signal-trapping
- added '-W' (won't make bootable CD or floppies) flag
- added missing 'j++' to -s switch's code
- expanded -s switch
- added -u switch
- if dvdrecord exists and cdrecord doesn't then use
  dvdrecord instead of cdrecord

04/01 - 04/30
- if CD-ROM is supermounted then unmount, run, and remount
- replace MONDO_VERSION #define with VERSION from ../config.h
- if CD-ROM is mounted at start then abort
- improved homedir-locating code
- improved "backup will occupy N CD's" calculation a bit
- added -m (manual CD tray) flag


*******************************************************************/

/**
 * @file
 * Functions for handling command-line arguments passed to mondoarchive.
 */

/** @def BOOT_LOADER_CHARS The characters allowed for boot loader on this platform. */

#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "mondo-cli-EXT.h"
#include "../common/libmondo.h"
#include <pthread.h>

#ifndef VERSION
#define VERSION AUX_VER
#endif


//static char cvsid[] = "$Id$";

extern int g_loglevel;
extern bool g_text_mode;
extern bool g_skip_floppies;	///< Whether to skip the creation of boot disks
extern char g_startdir[MAX_STR_LEN];	///< ????? @bug ?????
extern char g_erase_tmpdir_and_scratchdir[MAX_STR_LEN];
extern char g_tmpfs_mountpt[MAX_STR_LEN];
extern bool g_sigpipe;

/*@ file pointer **************************************************/
extern FILE *g_tape_stream;

/*@ long long *****************************************************/
extern long long g_tape_posK;

/*@ long **********************************************************/
extern long g_noof_sets;

/*@ bool******** **************************************************/
bool g_debugging = FALSE;		///< ????? @bug ????? @ingroup globalGroup
bool g_running_live = FALSE;	///< ????? @bug ????? @ingroup globalGroup
extern bool g_cd_recovery;

/**
 * Whether we're restoring from ISOs. Obviously not, since this is the
 * backup program.
 * @note You @b MUST declare this variable somewhere in your program if
 * you use libmondo. Otherwise the link will fail.
 * @ingroup globalGroup
 */
bool g_ISO_restore_mode = FALSE;


extern double g_kernel_version;

extern int g_current_media_number;



extern pid_t g_main_pid;




extern char *resolve_softlinks_to_get_to_actual_device_file(char *);



/**
 * @addtogroup cliGroup
 * @{
 */
/**
 * Populate @p bkpinfo from the command-line parameters stored in @p argc and @p argv.
 * @param argc The argument count, including the program name; @p argc passed to main().
 * @param argv The argument vector; @p argv passed to main().
 * @param bkpinfo The backup information structure to populate.
 * @return The number of problems with the command line (0 for success).
 */
int
handle_incoming_parameters(int argc, char *argv[],
						   struct s_bkpinfo *bkpinfo)
{
	/*@ int *** */
	int res = 0;
	int retval = 0;
	int i = 0, j;

	/*@ buffers *************** */
	char *tmp;
	char flag_val[128][MAX_STR_LEN];
	bool flag_set[128];

	malloc_string(tmp);
	sensibly_set_tmpdir_and_scratchdir(bkpinfo);
	for (i = 0; i < 128; i++) {
		flag_val[i][0] = '\0';
		flag_set[i] = FALSE;
	}
	//  strcpy (bkpinfo->tmpdir, "/root/images/mondo");
	//  strcpy (bkpinfo->scratchdir, "/home");
	for (j = 1; j <= MAX_NOOF_MEDIA; j++) {
		bkpinfo->media_size[j] = 650;
	}							/* default */
	res =
		retrieve_switches_from_command_line(argc, argv, flag_val,
											flag_set);
	log_it("value: %s", flag_val['s']);
	retval += res;
	if (!retval) {
		res = process_switches(bkpinfo, flag_val, flag_set);
		retval += res;
	}
/*
  if (!retval)
    {
*/
	log_msg(3, "Switches:-");
	for (i = 0; i < 128; i++) {
		if (flag_set[i]) {
			sprintf(tmp, "-%c %s", i, flag_val[i]);
			log_msg(3, tmp);
		}
	}
//    }
	sprintf(tmp, "rm -Rf %s/tmp.mondo.*", bkpinfo->tmpdir);
	paranoid_system(tmp);
	sprintf(tmp, "rm -Rf %s/mondo.scratch.*", bkpinfo->scratchdir);
	paranoid_system(tmp);
	sprintf(bkpinfo->tmpdir + strlen(bkpinfo->tmpdir), "/tmp.mondo.%ld",
			random() % 32767);
	sprintf(bkpinfo->scratchdir + strlen(bkpinfo->scratchdir),
			"/mondo.scratch.%ld", random() % 32767);
	sprintf(tmp, "mkdir -p %s/tmpfs", bkpinfo->tmpdir);
	paranoid_system(tmp);
	sprintf(tmp, "mkdir -p %s", bkpinfo->scratchdir);
	paranoid_system(tmp);
	if (bkpinfo->nfs_mount[0] != '\0') {
		store_nfs_config(bkpinfo);
	}
	paranoid_free(tmp);
	return (retval);
}




/**
 * Store the sizespec(s) stored in @p value into @p bkpinfo.
 * @param bkpinfo The backup information structure; the @c bkpinfo->media_size field will be populated.
 * @param value The sizespec (e.g. "2g", "40m").
 * @return 0, always.
 * @bug Return code not needed.
 */
int process_the_s_switch(struct s_bkpinfo *bkpinfo, char *value)
{
	int j;
	char tmp[MAX_STR_LEN], *p, comment[MAX_STR_LEN];

	assert(bkpinfo != NULL);
	assert(value != NULL);

	bkpinfo->media_size[0] = -1;	/* dummy value */
	for (j = 1, p = value; j < MAX_NOOF_MEDIA && strchr(p, ',');
		 j++, p = strchr(p, ',') + 1) {
		strncpy(tmp, p, MAX_STR_LEN);
		*(strchr(tmp, ',')) = '\0';
		bkpinfo->media_size[j] = friendly_sizestr_to_sizelong(tmp);
		sprintf(comment, "media_size[%d] = %ld", j,
				bkpinfo->media_size[j]);
		log_msg(3, comment);
	}
	for (; j <= MAX_NOOF_MEDIA; j++) {
		bkpinfo->media_size[j] = friendly_sizestr_to_sizelong(p);
	}
//      bkpinfo->media_size[0] = bkpinfo->media_size[MAX_NOOF_MEDIA];
	for (j = 1; j <= MAX_NOOF_MEDIA; j++) {
		if (bkpinfo->media_size[j] <= 0) {
			log_msg(1, "You gave media #%d an invalid size\n", j);
			return (-1);
		}
	}
	return (0);
}



/**
 * Process mondoarchive's command-line switches.
 * @param bkpinfo The backup information structure to populate.
 * @param flag_val An array of the argument passed to each switch (the letter is the index).
 * If a switch is not set or has no argument, the field in @p flag_val doesn't matter.
 * @param flag_set An array of <tt>bool</tt>s indexed by switch letter: TRUE if it's set,
 * FALSE if it's not.
 * @return The number of problems with the switches, or 0 for success.
 * @bug Maybe include a list of all switches (inc. intentionally undocumented ones not in the manual!) here?
 */
int
process_switches(struct s_bkpinfo *bkpinfo,
				 char flag_val[128][MAX_STR_LEN], bool flag_set[128])
{

	/*@ ints *** */
	int i = 0;
	int retval = 0;
	int percent = 0;

	/*@ buffers ** */
	char *tmp;
	char *psz;

	long itbs;

	malloc_string(tmp);
	malloc_string(psz);

	assert(bkpinfo != NULL);
	assert(flag_val != NULL);
	assert(flag_set != NULL);

	bkpinfo->internal_tape_block_size = DEFAULT_INTERNAL_TAPE_BLOCK_SIZE;

/* compulsory */
	i = flag_set['c'] + flag_set['i'] + flag_set['n'] +
		flag_set['t'] + flag_set['u'] + flag_set['r'] +
		flag_set['w'] + flag_set['C'];
	if (i == 0) {
		retval++;
		log_to_screen("You must specify the media type\n");
	}
	if (i > 1) {
		retval++;
		log_to_screen("Please specify only one media type\n");
	}
	if (flag_set['K']) {
		g_loglevel = atoi(flag_val['K']);
		if (g_loglevel < 3) {
			g_loglevel = 3;
		}
	}
	if (flag_set['L'] && flag_set['0']) {
		retval++;
		log_to_screen("You cannot have 'no compression' _and_ LZOP.\n");
	}
	bkpinfo->backup_data = flag_set['O'];
	bkpinfo->verify_data = flag_set['V'];
	if (flag_set['I'] && !bkpinfo->backup_data) {
		log_to_screen("-I switch is ignored if just verifying");
	}
	if (flag_set['E'] && !bkpinfo->backup_data) {
		log_to_screen("-E switch is ignored if just verifying");
	}

	if (!find_home_of_exe("afio")) {
		if (find_home_of_exe("star")) {
			flag_set['R'] = TRUE;
			log_msg(1, "Using star instead of afio");
		} else {
			fatal_error
				("Neither afio nor star is installed. Please install at least one.");
		}
	}

	if (flag_set['R']) {
		bkpinfo->use_star = TRUE;
		if (flag_set['L']) {
			fatal_error("You may not use star and lzop at the same time.");
		}
		if (!find_home_of_exe("star")) {
			fatal_error
				("Please install 'star' RPM or tarball if you are going to use -R. Thanks.");
		}
	}
	if (flag_set['W']) {
		bkpinfo->nonbootable_backup = TRUE;
		log_to_screen("Warning - you have opted for non-bootable backup");
		if (flag_set['f'] || flag_set['l']) {
			log_to_screen
				("You don't need to specify bootloader or bootdevice");
		}
	}
	if (flag_set['t'] && flag_set['H']) {
		fatal_error
			("Sorry, you may not nuke w/o warning from tape. Drop -H, please.");
	}
	if (flag_set['I']) {
		if (!strcmp(bkpinfo->include_paths, "/")) {
			log_msg(2, "'/' is pleonastic.");
			bkpinfo->include_paths[0] = '\0';
		}
		if (bkpinfo->include_paths[0]) {
			strcat(bkpinfo->include_paths, " ");
		}
		strncpy(bkpinfo->include_paths + strlen(bkpinfo->include_paths),
				flag_val['I'],
				MAX_STR_LEN - strlen(bkpinfo->include_paths));
		log_msg(1, "include_paths is now '%s'", bkpinfo->include_paths);
		if (bkpinfo->include_paths[0] == '-') {
			retval++;
			log_to_screen("Please supply a sensible value with '-I'\n");
		}
	}

	if (g_kernel_version >= 2.6 && !flag_set['d']
		&& (flag_set['c'] || flag_set['w'])) {
		fatal_error
			("If you are using the 2.6.x kernel, please specify the CD-R(W) device.");
	}


	if (flag_set['J']) {
		if (flag_set['I']) {
			retval++;
			log_to_screen
				("Please do not use -J in combination with -I. If you want to make a list of files to backup, that's fine, use -J <filename> but please don't muddy the waters by combining -J with -I. Thanks. :-)");
		}
		bkpinfo->make_filelist = FALSE;
		strcpy(bkpinfo->include_paths, flag_val['J']);
	}
	if (flag_set['c'] || flag_set['w'] || flag_set['C'] || flag_set['r']) {
		if (!flag_set['r'] && g_kernel_version <= 2.5
			&& strstr(flag_val['d'], "/dev/")) {
			fatal_error
				("Please don't give a /dev entry. Give a SCSI node for the parameter of the -d flag.");
		}
		if (flag_set['r'] && g_kernel_version <= 2.5
			&& !strstr(flag_val['d'], "/dev/")) {
			fatal_error
				("Please give a /dev entry, not a SCSI node, as the parameter of the -d flag.");
		}
		if (g_kernel_version >= 2.6 && !strstr(flag_val['d'], "/dev/")) {
			log_to_screen
				("Linus says 2.6 has a broken ide-scsi module. Proceed at your own risk...");
		}

		if (system("which cdrecord > /dev/null 2> /dev/null")
			&& system("which dvdrecord > /dev/null 2> /dev/null")) {
			fatal_error
				("Please install dvdrecord/cdrecord and try again.");
		}
		if (flag_set['C']) {
			bkpinfo->cdrw_speed = 2;
			if (!flag_set['L']) {
				log_to_screen
					("You must use -L with -C. Therefore I am setting it for you.");
				flag_set['L'] = 1;
				flag_val['L'][0] = '\0';
			}
		} else {
			log_msg(3, "flag_val['c'] = %s", flag_val['c']);
			log_msg(3, "flag_val['w'] = %s", flag_val['w']);
//    log_msg(3, "flag_set['r'] = %i", flag_set['r'] );
			if (flag_set['c']) {
				bkpinfo->cdrw_speed = atoi(flag_val['c']);
			} else if (flag_set['w']) {
				bkpinfo->cdrw_speed = atoi(flag_val['w']);
			} else if (flag_set['r']) {
				bkpinfo->cdrw_speed = 1;	/*atoi(flag_val['r']); */
			}

			if (bkpinfo->cdrw_speed < 1) {
				fatal_error
					("You specified a silly speed for a CD-R[W] drive");
			}
		}
	}
	if (flag_set['t'] && !flag_set['d']) {
		log_it("Hmm! No tape drive specified. Let's see what we can do.");
		if (find_tape_device_and_size(flag_val['d'], tmp)) {
			fatal_error
				("Tape device not specified. I couldn't find it either.");
		}
		flag_set['d'] = TRUE;
		sprintf(tmp,
				"You didn't specify a tape streamer device. I'm assuming %s",
				flag_val['d']);
		log_to_screen(tmp);
		percent = 0;
	}

	if (flag_set['r'])			// DVD
	{
		if (flag_set['m']) {
			fatal_error
				("Manual CD tray (-m) not yet supported in conjunction w/ DVD drives. Drop -m.");
		}
		if (!flag_set['d']) {
			if (!find_dvd_device(flag_val['d'], FALSE)) {
				flag_set['d'] = TRUE;
				log_to_screen("I guess DVD drive is at %s", flag_val['d']);
			}
		}
		if (!find_home_of_exe("growisofs")) {
			fatal_error
				("Please install growisofs (probably part of dvd+rw-tools). If you want DVD support, you need it.");
		}
		if (!find_home_of_exe("dvd+rw-format")) {
			fatal_error
				("Please install dvd+rw-format (probably part of dvd+rw-tools). If you want DVD support, you need it.");
		}
		if (strchr(flag_val['d'], ',')) {
			fatal_error
				("Please don't give a SCSI node. Give a _device_, preferably a /dev entry, for the parameter of the -d flag.");
		}
		if (!flag_set['s']) {
			sprintf(flag_val['s'], "%d", DEFAULT_DVD_DISK_SIZE);	// 4.7 salesman's GB = 4.482 real GB = 4582 MB
			strcat(flag_val['s'], "m");
			log_to_screen
				("You did not specify a size (-s) for DVD. I'm guessing %s.",
				 flag_val['s']);
			flag_set['s'] = 1;
		}
/*
      if (flag_set['Z']) {
	  bkpinfo->blank_dvd_first = TRUE;
      }
*/
	}

	if (flag_set['t'] || flag_set['u']) {	/* tape size */
		if (strchr(flag_val['d'], ',')) {
			fatal_error
				("Please don't give a SCSI node. Give a _device_, preferably a /dev entry, for the parameter of the -d flag.");
		}
		if (flag_set['O']) {
			if (flag_set['s']) {
				if (flag_set['t']) {
					fatal_error
						("For the moment, please don't specify a tape size. Mondo should handle end-of-tape gracefully anyway.");
				}
				if (process_the_s_switch(bkpinfo, flag_val['s'])) {
					fatal_error("Bad -s switch");
				}
			} else if (flag_set['u'] || flag_set['t']) {
				for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
					bkpinfo->media_size[i] = 0;
				}
			} else {
				retval++;
				log_to_screen("Tape size not specified.\n");
			}
		}
	} else {					/* CD size */
		if (flag_set['s']) {
			if (process_the_s_switch(bkpinfo, flag_val['s'])) {
				fatal_error("Bad -s switch");
			}
		}
		if (flag_set['w']) {
			bkpinfo->wipe_media_first = TRUE;
		}						/* CD-RW */
	}
	if (flag_set['n']) {
		strncpy(bkpinfo->nfs_mount, flag_val['n'], MAX_STR_LEN);
		if (!flag_set['d']) {
			strncpy(bkpinfo->nfs_remote_dir, "/", MAX_STR_LEN);
		}
		sprintf(tmp, "mount | grep -x \"%s .*\" | cut -d' ' -f3",
				bkpinfo->nfs_mount);
		strncpy(bkpinfo->isodir,
				call_program_and_get_last_line_of_output(tmp),
				MAX_STR_LEN / 4);
		if (strlen(bkpinfo->isodir) < 3) {
			retval++;
			log_to_screen("NFS share is not mounted. Please mount it.\n");
		}
		log_msg(3, "mount = %s", bkpinfo->nfs_mount);
		log_msg(3, "isodir= %s", bkpinfo->isodir);
	}
	if (flag_set['c']) {
		bkpinfo->backup_media_type = cdr;
	}
	if (flag_set['C']) {
		bkpinfo->backup_media_type = cdstream;
	}
	if (flag_set['i']) {
		bkpinfo->backup_media_type = iso;
	}
	if (flag_set['n']) {
		bkpinfo->backup_media_type = nfs;
	}
	if (flag_set['r']) {
		bkpinfo->backup_media_type = dvd;
	}
	if (flag_set['t']) {
		bkpinfo->backup_media_type = tape;
	}
	if (flag_set['u']) {
		bkpinfo->backup_media_type = udev;
	}
	if (flag_set['w']) {
		bkpinfo->backup_media_type = cdrw;
	}

/* optional, popular */
	if (flag_set['g']) {
		g_text_mode = FALSE;
	}
	if (flag_set['E']) {
		if (bkpinfo->exclude_paths[0]) {
			strcat(bkpinfo->exclude_paths, " ");
		}
		strncpy(bkpinfo->exclude_paths + strlen(bkpinfo->exclude_paths),
				flag_val['E'],
				MAX_STR_LEN - strlen(bkpinfo->exclude_paths));
	}
	if (flag_set['e']) {
		bkpinfo->please_dont_eject = TRUE;
	}
	if (flag_set['N'])			// exclude NFS mounts & devices
	{
//      strncpy(psz, list_of_NFS_devices_and_mounts(), MAX_STR_LEN);
		strncpy(psz, list_of_NFS_mounts_only(), MAX_STR_LEN);
		if (bkpinfo->exclude_paths[0]) {
			strncat(bkpinfo->exclude_paths, " ", MAX_STR_LEN);
		}
		strncat(bkpinfo->exclude_paths, psz, MAX_STR_LEN);
		log_msg(3, "-N means we're now excluding %s",
				bkpinfo->exclude_paths);
	}
	if (strlen(bkpinfo->exclude_paths) >= MAX_STR_LEN) {
		fatal_error
			("Your '-E' parameter is too long. Please use '-J'. (See manual.)");
	}
	if (flag_set['b']) {
		strcpy(psz, flag_val['b']);
		log_msg(1, "psz = '%s'", psz);
		if (psz[strlen(psz) - 1] == 'k') {
			psz[strlen(psz) - 1] = '\0';
			itbs = atol(psz) * 1024L;
		} else {
			itbs = atol(psz);
		}
		log_msg(1, "'%s' --> %ld", flag_val['b'], itbs);
		log_msg(1, "Internal tape block size is now %ld bytes", itbs);
		if (itbs % 512 != 0 || itbs < 256 || itbs > 1024L * 1024) {
			fatal_error
				("Are you nuts? Silly, your internal tape block size is. Abort, I shall.");
		}
		bkpinfo->internal_tape_block_size = itbs;
	}
	if (flag_set['D']) {
		bkpinfo->differential = 1;
//      bkpinfo->differential = atoi (flag_val['D']);
		if ((bkpinfo->differential < 1) || (bkpinfo->differential > 9)) {
			fatal_error
				("The D option should be between 1 and 9 inclusive");
		}
	}
	if (flag_set['x']) {
		strncpy(bkpinfo->image_devs, flag_val['x'], MAX_STR_LEN / 4);
		if (run_program_and_log_output("which partimagehack", 2)) {
			fatal_error("Please install partimagehack RPM/tarball.");
		}
	}
	if (flag_set['m']) {
		bkpinfo->manual_cd_tray = TRUE;
	}
	if (flag_set['k']) {
		strncpy(bkpinfo->kernel_path, flag_val['k'], MAX_STR_LEN);
		if (!strcmp(bkpinfo->kernel_path, "failsafe")) {
			strcpy(bkpinfo->kernel_path, "FAILSAFE");
		}
		if (strcmp(bkpinfo->kernel_path, "FAILSAFE")
			&& !does_file_exist(bkpinfo->kernel_path)) {
			retval++;
			sprintf(tmp,
					"You specified kernel '%s', which does not exist\n",
					bkpinfo->kernel_path);
			log_to_screen(tmp);
		}
	}
	if (flag_set['p']) {
		strncpy(bkpinfo->prefix, flag_val['p'], MAX_STR_LEN / 4);
	}


	if (flag_set['d']) {		/* backup directory (if ISO/NFS) */
		if (flag_set['i']) {
			strncpy(bkpinfo->isodir, flag_val['d'], MAX_STR_LEN / 4);
			sprintf(tmp, "ls -l %s", bkpinfo->isodir);
			if (run_program_and_log_output(tmp, FALSE)) {
				fatal_error
					("output folder does not exist - please create it");
			}
		} else if (flag_set['n']) {
			strncpy(bkpinfo->nfs_remote_dir, flag_val['d'], MAX_STR_LEN);
		} else {				/* backup device (if tape/CD-R/CD-RW) */

			strncpy(bkpinfo->media_device, flag_val['d'], MAX_STR_LEN / 4);
		}
	}

	if (flag_set['n']) {
		sprintf(tmp, "echo hi > %s/%s/.dummy.txt", bkpinfo->isodir,
				bkpinfo->nfs_remote_dir);
		if (run_program_and_log_output(tmp, FALSE)) {
			retval++;
			sprintf(tmp,
					"Are you sure directory '%s' exists in remote dir '%s'?\nIf so, do you have rights to write to it?\n",
					bkpinfo->nfs_remote_dir, bkpinfo->nfs_mount);
			log_to_screen(tmp);
		}
	}

	if (!flag_set['d']
		&& (flag_set['c'] || flag_set['w'] || flag_set['C'])) {
		if (g_kernel_version >= 2.6) {
			if (popup_and_get_string
				("Device", "Please specify the device",
				 bkpinfo->media_device, MAX_STR_LEN / 4)) {
				retval++;
				log_to_screen("User opted to cancel.");
			}
		} else if (find_cdrw_device(bkpinfo->media_device)) {
			retval++;
			log_to_screen
				("Tried and failed to find CD-R[W] drive automatically.\n");
		} else {
			flag_set['d'] = TRUE;
			strncpy(flag_val['d'], bkpinfo->media_device, MAX_STR_LEN / 4);
		}
	}

	if (!flag_set['d'] && !flag_set['n'] && !flag_set['C']) {
		retval++;
		log_to_screen("Please specify the backup device/directory.\n");
		fatal_error
			("You didn't use -d to specify the backup device/directory.");
	}
/* optional, obscure */
	for (i = '0'; i <= '9'; i++) {
		if (flag_set[i]) {
			bkpinfo->compression_level = i - '0';
		}						/* not '\0' but '0' */
	}
	if (flag_set['S']) {
		sprintf(bkpinfo->scratchdir, "%s/mondo.scratch.%ld", flag_val['S'],
				random() % 32768);
	}
	if (flag_set['T']) {
		sprintf(bkpinfo->tmpdir, "%s/tmp.mondo.%ld", flag_val['T'],
				random() % 32768);
		sprintf(tmp, "touch %s/.foo.dat", flag_val['T']);
		if (run_program_and_log_output(tmp, 1)) {
			retval++;
			log_to_screen
				("Please specify a tempdir which I can write to. :)");
			fatal_error("I cannot write to the tempdir you specified.");
		}
		sprintf(tmp, "ln -sf %s/.foo.dat %s/.bar.dat", flag_val['T'],
				flag_val['T']);
		if (run_program_and_log_output(tmp, 1)) {
			retval++;
			log_to_screen
				("Please don't specify a SAMBA or VFAT or NFS tmpdir.");
			fatal_error("I cannot write to the tempdir you specified.");
		}
	}
	if (flag_set['A']) {
		strncpy(bkpinfo->call_after_iso, flag_val['A'], MAX_STR_LEN);
	}
	if (flag_set['B']) {
		strncpy(bkpinfo->call_before_iso, flag_val['B'], MAX_STR_LEN);
	}
	if (flag_set['F']) {
		g_skip_floppies = TRUE;
	}
	if (flag_set['H']) {
		g_cd_recovery = TRUE;
	}
	if (flag_set['l']) {
#ifdef __FreeBSD__
#  define BOOT_LOADER_CHARS "GLBMR"
#else
#  ifdef __IA64__
#    define BOOT_LOADER_CHARS "GER"
#  else
#    define BOOT_LOADER_CHARS "GLR"
#  endif
#endif
		if (!strchr
			(BOOT_LOADER_CHARS,
			 (bkpinfo->boot_loader = flag_val['l'][0]))) {
			log_msg(1, "%c? WTF is %c? I need G, L, E or R.",
					bkpinfo->boot_loader, bkpinfo->boot_loader);
			fatal_error
				("Please specify GRUB, LILO, ELILO  or RAW with the -l switch");
		}
#undef BOOT_LOADER_CHARS
	}
	if (flag_set['f']) {
		strncpy(bkpinfo->boot_device,
				resolve_softlinks_to_get_to_actual_device_file(flag_val
															   ['f']),
				MAX_STR_LEN / 4);
	}
	if (flag_set['P']) {
		strncpy(bkpinfo->postnuke_tarball, flag_val['P'], MAX_STR_LEN);
	}
	if (flag_set['Q']) {
		i = which_boot_loader(tmp);
		log_msg(3, "boot loader is %c, residing at %s", i, tmp);
		printf("boot loader is %c, residing at %s\n", i, tmp);
		finish(0);
	}
	if (flag_set['L']) {
		bkpinfo->use_lzo = TRUE;
		if (run_program_and_log_output("which lzop", FALSE)) {
			retval++;
			log_to_screen
				("Please install LZOP. You can't use '-L' until you do.\n");
		}
	}

	if (!flag_set['o']
		&&
		!run_program_and_log_output
		("cat /etc/issue.net | grep -i suse | grep 64", TRUE)) {
		bkpinfo->make_cd_use_lilo = TRUE;
		log_to_screen
			("Forcing you to use LILO. SuSE 9.0 (64-bit) has a broken mkfs.vfat binary.");
	}
	if (flag_set['o']) {
		bkpinfo->make_cd_use_lilo = TRUE;
	}
#ifndef __FreeBSD__
	else {
		if (!is_this_a_valid_disk_format("vfat")) {
			bkpinfo->make_cd_use_lilo = TRUE;
			log_to_screen
				("Your kernel appears not to support vfat filesystems. I am therefore");
			log_to_screen
				("using LILO instead of SYSLINUX as the CD/floppy's boot loader.");
		}
		if (run_program_and_log_output("which mkfs.vfat", FALSE)) {
			bkpinfo->make_cd_use_lilo = TRUE;
#ifdef __IA32__
			log_to_screen
				("Your filesystem is missing 'mkfs.vfat', so I cannot use SYSLINUX as");
			log_to_screen
				("your boot loader. I shall therefore use LILO instead.");
#endif
#ifdef __IA64__
			log_to_screen
				("Your filesystem is missing 'mkfs.vfat', so I cannot prepare the EFI");
			log_to_screen("environment correctly. Please install it.");
			fatal_error("Aborting");
#endif
		}
#ifdef __IA64__
		/* We force ELILO usage on IA64 */
		bkpinfo->make_cd_use_lilo = TRUE;
#endif
	}
#endif

	if (bkpinfo->make_cd_use_lilo && !does_file_exist("/boot/boot.b")) {
		paranoid_system("touch /boot/boot.b");
	}

	i = flag_set['O'] + flag_set['V'];
	if (i == 0) {
		retval++;
		log_to_screen("Specify backup (-O), verify (-V) or both (-OV).\n");
	}

/* and finally... */

	paranoid_free(tmp);
	paranoid_free(psz);
	return (retval);
}



/**
 * Get the switches from @p argc and @p argv using getopt() and place them in
 * @p flag_set and @p flag_val.
 * @param argc The argument count (@p argc passed to main()).
 * @param argv The argument vector (@p argv passed to main()).
 * @param flag_val An array indexed by switch letter - if a switch is set and
 * has an argument then set flag_val[switch] to that argument.
 * @param flag_set An array indexed by switch letter - if a switch is set then
 * set flag_set[switch] to TRUE, else set it to FALSE.
 * @return The number of problems with the command line (0 for success).
 */
int
retrieve_switches_from_command_line(int argc, char *argv[],
									char flag_val[128][MAX_STR_LEN],
									bool flag_set[128])
{
	/*@ ints ** */
	int opt = 0;
	char tmp[MAX_STR_LEN];
	int i = 0;
	int len;

	/*@ bools *** */
	bool bad_switches = FALSE;

	assert(flag_val != NULL);
	assert(flag_set != NULL);

	for (i = 0; i < 128; i++) {
		flag_val[i][0] = '\0';
		flag_set[i] = FALSE;
	}
	while ((opt =
			getopt(argc, argv,
				   "0123456789A:B:CDE:FHI:J:K:LNOP:QRS:T:VWb:c:d:ef:gik:l:mn:op:rs:tuw:x:"))
		   != -1) {
		if (opt == '?') {
			bad_switches = TRUE;
			/*log_it("Invalid option: %c\n",optopt); */
		} else {
			if (flag_set[optopt]) {
				bad_switches = TRUE;
				sprintf(tmp, "Switch -%c previously defined as %s\n", opt,
						flag_val[i]);
				log_to_screen(tmp);
			} else {
				flag_set[opt] = TRUE;
				if (optarg) {
					len = strlen(optarg);
					if (optarg[0] != '/' && optarg[len - 1] == '/') {
						optarg[--len] = '\0';
						log_to_screen
							("Warning - param '%s' should not have trailing slash!",
							 optarg);
					}
					if (opt == 'd') {
						if (strchr(flag_val[opt], '/')
							&& flag_val[opt][0] != '/') {
							sprintf(tmp,
									"-%c flag --- must be absolute path --- '%s' isn't absolute",
									opt, flag_val[opt]);
							log_to_screen(tmp);
							bad_switches = TRUE;
						}
					}
					strcpy(flag_val[opt], optarg);
				}
			}
		}
	}
	for (i = optind; i < argc; i++) {
		bad_switches = TRUE;
		sprintf(tmp, "Invalid arg -- %s\n", argv[i]);
		log_to_screen(tmp);
	}
	return (bad_switches);
}




/**
 * Print a not-so-helpful help message and exit.
 */
void help_screen()
{
	log_msg(1, "Type 'man mondo-archive' for more information\n");
	exit(1);
}


/**
 * Terminate Mondo in response to a signal.
 * @param sig The signal number received.
 */
void terminate_daemon(int sig)
{
	char tmp[64];
	char tmp2[MAX_STR_LEN];
	//  char command[512];
	//  pid_t pid;

	switch (sig) {
	case SIGINT:
		sprintf(tmp, "SIGINT");
		strcpy(tmp2, "You interrupted me :-)");
		break;
	case SIGKILL:
		sprintf(tmp, "SIGKILL");
		strcpy(tmp2,
			   "I seriously have no clue how this signal even got to me. Something's wrong with your system.");
		break;
	case SIGTERM:
		sprintf(tmp, "SIGTERM");
		strcpy(tmp2, "Got terminate signal");
		break;
	case SIGHUP:
		sprintf(tmp, "SIGHUP");
		strcpy(tmp2, "Hangup on line");
		break;
	case SIGSEGV:
		sprintf(tmp, "SIGSEGV");
		strcpy(tmp2,
			   "Internal programming error. Please send a backtrace as well as your log.");
		break;
	case SIGPIPE:
		sprintf(tmp, "SIGPIPE");
		strcpy(tmp2, "Pipe was broken");
		break;
	case SIGABRT:
		sprintf(tmp, "SIGABRT");
		sprintf(tmp2,
				"Abort - probably failed assertion. I'm sleeping for a few seconds so you can read the message.");
		break;
	default:
		sprintf(tmp, "(Unknown)");
	}

	strcat(tmp, " signal received from OS");
	log_to_screen(tmp);
	log_to_screen(tmp2);
	if (sig == SIGABRT) {
		sleep(10);
	}
	kill_buffer();
	fatal_error
		("Mondoarchive is terminating in response to a signal from the OS");
	finish(254);				// just in case
}




/**
 * Turn signal-trapping on or off.
 * @param on If TRUE, turn it on; if FALSE, turn it off (we still trap it, just don't do as much).
 */
void set_signals(int on)
{
	int signals[] =
		{ SIGTERM, SIGHUP, SIGTRAP, SIGABRT, SIGINT, SIGKILL, SIGSTOP, 0 };
	int i;

	signal(SIGPIPE, sigpipe_occurred);
	for (i = 0; signals[i]; i++) {
		if (on) {
			signal(signals[i], terminate_daemon);
		} else {
			signal(signals[i], termination_in_progress);
		}
	}
}




/**
 * Exit immediately without cleaning up.
 * @param sig The signal we are exiting due to.
 */
void termination_in_progress(int sig)
{
	log_msg(1, "Termination in progress");
	usleep(1000);
	pthread_exit(0);
}

/* @} - end of cliGroup */
