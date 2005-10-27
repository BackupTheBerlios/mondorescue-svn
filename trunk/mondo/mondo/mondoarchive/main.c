/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Fri Apr 19 16:40:35 EDT 2002
    copyright            : (C) 2002 by Stan Benoit
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

/** change log ****** MONDO-DEVEL


12/10
- disable stopping/starting of autofs

10/01
- update g_erase_tmpdir_and_scratchdir to delete user-specified tmpdir, scratchdir

06/19
- added AUX_VER

06/14/2004
- use mondorescue.iso, not mindi.iso

02/10/2004
- tell users where BusyBox's sources are

11/14/2003
- cleaned up logging at end#

10/23
- don't try to test-read tape ... That's already
  handled by post_param_configuration()
  
10/19
- if your PATH var is too long, abort

09/23
- added some comments
- malloc/free global strings in new subroutines - malloc_libmondo_global_strings()
  and free_libmondo_global_strings() - which are in libmondo-tools.c
- better magicdev support


09/16
- delete /var/log/partimagehack-debug.log at start of main()

09/15
- added askbootloader

09/09
- if your tape is weird, I'll pause between backup and verify
- fixed silly bug in main() - re: say_at_end

01/01 - 08/31
- call 'dmesg -n1' at start, to shut the kernel logger up
- moved g_erase_tmpdir_and_scratchdir to common/newt-specific.c
- added 'don't panic' msg to start of logfile
- added 'nice(20)' to main()
- added lots of assert()'s and log_OS_error()'s
- clean-up (Hugo)
- make post_param_configuration() setup g_erase_tmpdir_and_scratchdir
- if --version then print & exit quickly
- re-run g_erase_tmpdir_and_scratchdir via system() at very end

Year: 2002
- if user goes root with 'su' instead of 'su -' then
  workaround it by setting PATH correctly
- wipe mondoarchive.log at very beginning
- cleaned up code
- if changed.files.N exists then copy to changes.files for display
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_
- added popup list of changed files
- removed 'beta-quality' warnings
- if kernel not found and mondo in graphics mode then popup and ask
  for kernel path+filename
- fixed tmp[] 'too small' bug
- unmount and eject CD at end of verify cycle
- moved interactively_obtain...() to libmondo-stream.c
- wrote stuff to autodetect tape+cdrw+etc.
- renamed from main.c to mondo-archive.c
- fore+after warnings that this code is beta-quality
- abort if running from ramdisk
- remount floppy at end & unmount at start if Mandrake
- took out #debug stuff
- add 2> /dev/null to 'find' command
- add support for bkpinfo->nonbootable_backup
- add main function begin comment and debug conditional
  compilation - Stan Benoit
- add debug statements to build a run tree. Stan Benoit
**** end change log **********/


/**
 * @file
 * The main file for mondoarchive.
 */

/************************* #include statements *************************/
#include <pthread.h>
//#include <config.h>
//#include "../../config.h"
#include <stdio.h>
#include <stdlib.h>
#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#include "mondo-cli-EXT.h"

#ifndef VERSION
#define VERSION AUX_VER
#endif


char g_version[] = VERSION;


// for CVS
//static char cvsid[] = "$Id$";

/************************* external variables *************************/
extern void set_signals(int);
extern int g_current_media_number;
extern void register_pid(pid_t, char *);
extern int g_currentY;
extern bool g_text_mode;
extern char *g_boot_mountpt;
extern bool g_remount_cdrom_at_end, g_remount_floppy_at_end;
extern char *g_mondo_home;
extern char *g_tmpfs_mountpt;
extern char *g_erase_tmpdir_and_scratchdir;
extern char *g_cdrw_drive_is_here;
static char *g_cdrom_drive_is_here = NULL;
static char *g_dvd_drive_is_here = NULL;
extern double g_kernel_version;

/***************** global vars, used only by main.c ******************/
bool g_skip_floppies;
long diffs;

extern t_bkptype g_backup_media_type;
extern int g_loglevel;

/****************** subroutines used only by main.c ******************/


/**
 * Print a "don't panic" message to the log and a message about the logfile to the screen.
 */
void welcome_to_mondoarchive()
{
	log_msg(0, "Mondo Archive v%s --- http://www.mondorescue.org",
			VERSION);
	log_msg(0, "running on %s architecture", get_architecture());
	log_msg(0,
			"-----------------------------------------------------------");
	log_msg(0,
			"NB: Mondo logs almost everything, so don't panic if you see");
	log_msg(0,
			"some error messages.  Please read them carefully before you");
	log_msg(0,
			"decide to break out in a cold sweat.    Despite (or perhaps");
	log_msg(0,
			"because of) the wealth of messages. some users are inclined");
	log_msg(0,
			"to stop reading this log. If Mondo stopped for some reason,");
	log_msg(0,
			"chances are it's detailed here.  More than likely there's a");
	log_msg(0,
			"message at the very end of this log that will tell you what");
	log_msg(0,
			"is wrong. Please read it!                          -Devteam");
	log_msg(0,
			"-----------------------------------------------------------");

	log_msg(0, "Zero...");
	log_msg(1, "One...");
	log_msg(2, "Two...");
	log_msg(3, "Three...");
	log_msg(4, "Four...");
	log_msg(5, "Five...");
	log_msg(6, "Six...");
	log_msg(7, "Seven...");
	log_msg(8, "Eight...");
	printf("See %s for details of backup run.\n", MONDO_LOGFILE);
}


extern char *g_magicdev_command;

/**
 * Do whatever is necessary to insure a successful backup on the Linux distribution
 * of the day.
 */
void distro_specific_kludges_at_start_of_mondoarchive()
{
	log_msg(2, "Unmounting old ramdisks if necessary");
	stop_magicdev_if_necessary();	// for RH+Gnome users
	run_program_and_log_output
		("umount `mount | grep shm | grep mondo | cut -d' ' -f3`", 2);
	unmount_supermounts_if_necessary();	// for Mandrake users whose CD-ROMs are supermounted
	//  stop_autofs_if_necessary(); // for Xandros users
	mount_boot_if_necessary();	// for Gentoo users with non-mounted /boot partitions
	clean_up_KDE_desktop_if_necessary();	// delete various misc ~/.* files that get in the way
}



/**
 * Undo whatever was done by distro_specific_kludges_at_start_of_mondoarchive().
 */
void distro_specific_kludges_at_end_of_mondoarchive()
{
//  char tmp[500];
	log_msg(2, "Restarting magicdev if necessary");
	sync();
	restart_magicdev_if_necessary();	// for RH+Gnome users

	log_msg(2, "Restarting autofs if necessary");
	sync();
	//  restart_autofs_if_necessary(); // for Xandros users

	log_msg(2, "Restarting supermounts if necessary");
	sync();
	remount_supermounts_if_necessary();	// for Mandrake users

	log_msg(2, "Unmounting /boot if necessary");
	sync();
	unmount_boot_if_necessary();	// for Gentoo users

//  log_msg( 2, "Cleaning up KDE desktop");
//  clean_up_KDE_desktop_if_necessary();
}


/*-----------------------------------------------------------*/



/**
 * Backup/verify the user's data.
 * What did you think it did, anyway? :-)
 */
int main(int argc, char *argv[])
{
	struct s_bkpinfo *bkpinfo;
	char *tmp;
	int res, retval;
	char *say_at_end;

/* Make sure I'm root; abort if not */
	if (getuid() != 0) {
		fprintf(stderr, "Please run as root.\r\n");
		exit(127);
	}

/* If -V, -v or --version then echo version no. and quit */
	if (argc == 2
		&& (!strcmp(argv[argc - 1], "-v") || !strcmp(argv[argc - 1], "-V")
			|| !strcmp(argv[argc - 1], "--version"))) {
		printf("mondoarchive v%s\nSee man page for help\n", VERSION);
		exit(0);
	}

/* Initialize variables */

	malloc_libmondo_global_strings();
	malloc_string(tmp);
	malloc_string(say_at_end);

	res = 0;
	retval = 0;
	diffs = 0;
	say_at_end[0] = '\0';
	unlink("/var/log/partimagehack-debug.log");
	printf("Initializing...\n");
	if (!(bkpinfo = malloc(sizeof(struct s_bkpinfo)))) {
		fatal_error("Cannot malloc bkpinfo");
	}


/* make sure PATH environmental variable allows access to mkfs, fdisk, etc. */
	strncpy(tmp, getenv("PATH"), MAX_STR_LEN - 1);
	tmp[MAX_STR_LEN - 1] = '\0';
	if (strlen(tmp) >= MAX_STR_LEN - 33) {
		fatal_error
			("Your PATH environmental variable is too long. Please shorten it.");
	}
	strcat(tmp, ":/sbin:/usr/sbin:/usr/local/sbin");
	setenv("PATH", tmp, 1);

/* Add the ARCH environment variable for ia64 purposes */
	strncpy(tmp, get_architecture(), MAX_STR_LEN - 1);
	tmp[MAX_STR_LEN - 1] = '\0';
	setenv("ARCH", tmp, 1);

	unlink(MONDO_LOGFILE);

/* Configure the bkpinfo structure, global file paths, etc. */
	g_main_pid = getpid();
	log_msg(9, "This");

	register_pid(g_main_pid, "mondo");
	set_signals(TRUE);			// catch SIGTERM, etc.
	nice(10);
	run_program_and_log_output("dmesg -n1", TRUE);

	log_msg(9, "Next");
	welcome_to_mondoarchive();
	distro_specific_kludges_at_start_of_mondoarchive();
	// BERLIOS : too early, bkpinfo is not initialized ??
	//sprintf(g_erase_tmpdir_and_scratchdir, "rm -Rf %s %s", bkpinfo->tmpdir, bkpinfo->scratchdir);
	g_kernel_version = get_kernel_version();

	if (argc == 4 && !strcmp(argv[1], "getfattr")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if (!strstr(argv[2], "filelist")) {
			printf("Sorry - filelist goes first\n");
			finish(1);
		} else {
			finish(get_fattr_list(argv[2], argv[3]));
		}
		finish(0);
	}
	if (argc == 4 && !strcmp(argv[1], "setfattr")) {
		g_loglevel = 10;
//      chdir("/tmp");
		g_text_mode = TRUE;
		setup_newt_stuff();
		finish(set_fattr_list(argv[2], argv[3]));
	}

	if (argc == 3 && !strcmp(argv[1], "wildcards")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		turn_wildcard_chars_into_literal_chars(tmp, argv[2]);
		printf("in=%s; out=%s\n", argv[2], tmp);
		finish(1);
	}

	if (argc == 4 && !strcmp(argv[1], "getfacl")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if (!strstr(argv[2], "filelist")) {
			printf("Sorry - filelist goes first\n");
			finish(1);
		} else {
			finish(get_acl_list(argv[2], argv[3]));
		}
		finish(0);
	}
	if (argc == 4 && !strcmp(argv[1], "setfacl")) {
		g_loglevel = 10;
//      chdir("/tmp");
		g_text_mode = TRUE;
		setup_newt_stuff();
		finish(set_acl_list(argv[2], argv[3]));
	}

	if (argc > 2 && !strcmp(argv[1], "find-cd")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if (find_cdrw_device(tmp)) {
			printf("Failed to find CDR-RW drive\n");
		} else {
			printf("CD-RW is at %s\n", tmp);
		}
		tmp[0] = '\0';
		if (find_cdrom_device(tmp, atoi(argv[2]))) {
			printf("Failed to find CD-ROM drive\n");
		} else {
			printf("CD-ROM is at %s\n", tmp);
		}
		finish(0);
	}

	if (argc > 2 && !strcmp(argv[1], "find-dvd")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if (find_dvd_device(tmp, atoi(argv[2]))) {
			printf("Failed to find DVD drive\n");
		} else {
			printf("DVD is at %s\n", tmp);
		}
		finish(0);
	}

	if (argc > 2 && !strcmp(argv[1], "disksize")) {
		printf("%s --> %ld\n", argv[2], get_phys_size_of_drive(argv[2]));
		finish(0);
	}
	if (argc > 2 && !strcmp(argv[1], "test-dev")) {
		if (is_dev_an_NTFS_dev(argv[2])) {
			printf("%s is indeed an NTFS dev\n", argv[2]);
		} else {
			printf("%s is _not_ an NTFS dev\n", argv[2]);
		}
		finish(0);
	}

	if (pre_param_configuration(bkpinfo)) {
		fatal_error
			("Pre-param initialization phase failed. Please review the error messages above, make the specified changes, then try again. Exiting...");
	}

/* Process command line, if there is one. If not, ask user for info. */
	if (argc == 1) {
		g_text_mode = FALSE;
		setup_newt_stuff();
		res = interactively_obtain_media_parameters_from_user(bkpinfo, TRUE);	/* yes, archiving */
		if (res) {
			fatal_error
				("Syntax error. Please review the parameters you have supplied and try again.");
		}
	} else {
		res = handle_incoming_parameters(argc, argv, bkpinfo);
		if (res) {
			printf
				("Errors were detected in the command line you supplied.\n");
			printf("Please review the log file - " MONDO_LOGFILE "\n");
			log_msg(1, "Mondoarchive will now exit.");
			finish(1);
		}
		setup_newt_stuff();
	}

/* Finish configuring global structures */
	if (post_param_configuration(bkpinfo)) {
		fatal_error
			("Post-param initialization phase failed. Perhaps bad parameters were supplied to mondoarchive? Please review the documentation, error messages and logs. Exiting...");
	}

	log_to_screen
		("BusyBox's sources are available from http://www.busybox.net");
	sprintf(g_erase_tmpdir_and_scratchdir, "rm -Rf %s %s", bkpinfo->tmpdir,
			bkpinfo->scratchdir);

	/* If we're meant to backup then backup */
	if (bkpinfo->backup_data) {
/*
      log_to_screen("INFERNAL PORPOISES");
      res = archive_this_fileset_with_star(bkpinfo, "/tmp/filelist.0", "/tmp/0.star.bz2", 0);
      log_to_screen("atfws returned %d", res);
      finish(0);
*/
		res = backup_data(bkpinfo);
		retval += res;
		if (res) {
			strcat(say_at_end,
				   "Data archived. Please check the logs, just as a precaution. ");
		} else {
			strcat(say_at_end, "Data archived OK. ");
		}
	}

/* If we're meant to verify then verify */
	if (bkpinfo->verify_data) {
		res = verify_data(bkpinfo);
		if (res < 0) {
			sprintf(tmp, "%d difference%c found.", -res,
					(-res != 1) ? 's' : ' ');
			strcat(say_at_end, tmp);
			log_to_screen(tmp);
			res = 0;
		}
		retval += res;
	}

/* Offer to write floppy disk images to physical disks */
	if (bkpinfo->backup_data && !g_skip_floppies) {
		res = offer_to_write_boot_floppies_to_physical_disks(bkpinfo);
		retval += res;
//      res = offer_to_write_boot_ISO_to_physical_CD(bkpinfo);
//      retval += res;
	}

/* Report result of entire operation (success? errors?) */
	if (!retval) {
		mvaddstr_and_log_it(g_currentY++, 0,
							"Backup and/or verify ran to completion. Everything appears to be fine.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 0,
							"Backup and/or verify ran to completion. However, errors did occur.");
	}

	if (does_file_exist("/root/images/mindi/mondorescue.iso")) {
		log_to_screen
			("/root/images/mindi/mondorescue.iso, a boot/utility CD, is available if you want it.");
	}


	if (length_of_file("/tmp/changed.files") > 2) {
		if (g_text_mode) {
			log_to_screen
				("Type 'less /tmp/changed.files' to see which files don't match the archives");
		} else {
			log_msg(1,
					"Type 'less /tmp/changed.files' to see which files don't match the archives");
			log_msg(2, "Calling popup_changelist_from_file()");
			popup_changelist_from_file("/tmp/changed.files");
			log_msg(2, "Returned from popup_changelist_from_file()");
		}
	} else {
		unlink("/tmp/changed.files");
	}
	log_to_screen(say_at_end);
	sprintf(tmp, "umount %s/tmpfs", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, TRUE);
	run_program_and_log_output(g_erase_tmpdir_and_scratchdir, TRUE);

	run_program_and_log_output("mount", 2);

	if (bkpinfo->please_dont_eject) {
		log_msg(5, "Not ejecting at end. Fair enough.");
	} else {
		log_msg(5, "Ejecting at end.");
		if (!find_cdrom_device(tmp, FALSE) || !find_dvd_device(tmp, FALSE)) {
			log_msg(1, "Ejecting %s", tmp);
			eject_device(tmp);
		}
	}

	system("rm -f /var/cache/mondo-archive/last-backup.aborted");
	system("rm -Rf /tmp.mondo.* /mondo.scratch.*");
	if (!retval) {
		printf("Mondoarchive ran OK.\n");
	} else {
		printf("Errors occurred during backup. Please check logfile.\n");
	}
	distro_specific_kludges_at_end_of_mondoarchive();
	register_pid(0, "mondo");
	set_signals(FALSE);
	chdir("/tmp");				// just in case there's something wrong with g_erase_tmpdir_and_scratchdir
	system(g_erase_tmpdir_and_scratchdir);
	free_libmondo_global_strings();
	paranoid_free(say_at_end);
	paranoid_free(tmp);
	paranoid_free(bkpinfo);

	unlink("/tmp/filelist.full");
	unlink("/tmp/filelist.full.gz");

	if (!g_cdrom_drive_is_here) {
		log_msg(10, "FYI, g_cdrom_drive_is_here was never used");
	}
	if (!g_dvd_drive_is_here) {
		log_msg(10, "FYI, g_dvd_drive_is_here was never used");
	}

	if (!g_text_mode) {
		popup_and_OK
			("Mondo Archive has finished its run. Please press ENTER to return to the shell prompt.");
		log_to_screen("See %s for details of backup run.", MONDO_LOGFILE);
		finish(retval);
	} else {
		printf("See %s for details of backup run.\n", MONDO_LOGFILE);
		exit(retval);
	}

	return EXIT_SUCCESS;
}
