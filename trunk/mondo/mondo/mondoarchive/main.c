/*
 * $Id$
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
 * The main file for mondoarchive.
 */

/************************* #include statements *************************/
#ifndef S_SPLINT_S
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#include "mondo-cli-EXT.h"

// for CVS
//static char cvsid[] = "$Id$";

/************************* external variables *************************/
extern void set_signals(int);
extern int g_current_media_number;
extern void register_pid(pid_t, char *);
extern int g_currentY;
extern bool g_text_mode;
extern bool g_remount_cdrom_at_end, g_remount_floppy_at_end;
extern char *g_mondo_home;
extern char *g_tmpfs_mountpt;
extern char *g_erase_tmpdir_and_scratchdir;
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
			PACKAGE_VERSION);
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
	mount_boot_if_necessary();	// for Gentoo users with non-mounted /boot partitions
	clean_up_KDE_desktop_if_necessary();	// delete various misc ~/.* files that get in the way
}


/**
 * Undo whatever was done by distro_specific_kludges_at_start_of_mondoarchive().
 */
void distro_specific_kludges_at_end_of_mondoarchive()
{
	log_msg(2, "Restarting magicdev if necessary");
	sync();
	restart_magicdev_if_necessary();	// for RH+Gnome users

	log_msg(2, "Restarting supermounts if necessary");
	sync();
	remount_supermounts_if_necessary();	// for Mandrake users

	log_msg(2, "Unmounting /boot if necessary");
	sync();
	unmount_boot_if_necessary();	// for Gentoo users
}


/**
 * Backup/verify the user's data.
 * What did you think it did, anyway? :-)
 */
int main(int argc, char *argv[])
{
	struct s_bkpinfo *bkpinfo;
	char *tmp;
	int res = 0;
	int retval = 0;
	char *say_at_end = NULL;

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	(void) textdomain("mondo");
#endif
/* Make sure I'm root; abort if not */
	if (getuid() != 0) {
		fprintf(stderr, _("Please run as root.\n"));
		exit(127);
	}

/* If -V, -v or --version then echo version no. and quit */
	if (argc == 2
		&& (!strcmp(argv[argc - 1], "-v") || !strcmp(argv[argc - 1], "-V")
			|| !strcmp(argv[argc - 1], "--version"))) {
		printf(_("mondoarchive v%s\nSee man page for help\n"), PACKAGE_VERSION);
		exit(0);
	}

/* Initialize variables */

	malloc_libmondo_global_strings();

	diffs = 0;
	printf(_("Initializing...\n"));
	if (!(bkpinfo = malloc(sizeof(struct s_bkpinfo)))) {
		fatal_error("Cannot malloc bkpinfo");
	}


/* make sure PATH environmental variable allows access to mkfs, fdisk, etc. */
	asprintf(&tmp, "/sbin:/usr/sbin:%s:/usr/local/sbin", getenv("PATH"));
	setenv("PATH", tmp, 1);
	paranoid_free(tmp);

/* Add the ARCH environment variable for ia64 purposes */
	setenv("ARCH", get_architecture(), 1);

	/* Add MONDO_SHARE environment variable for mindi */
	setenv_mondo_share();

	unlink(MONDO_LOGFILE);

/* Configure the bkpinfo structure, global file paths, etc. */
	g_main_pid = getpid();
	log_msg(9, "This");

	register_pid(g_main_pid, "mondo");
	set_signals(TRUE);			// catch SIGTERM, etc.
	run_program_and_log_output("date", 1);
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
			printf(_("Sorry - filelist goes first\n"));
			finish(1);
		} else {
			finish(get_fattr_list(argv[2], argv[3]));
		}
		finish(0);
	}
	if (argc == 4 && !strcmp(argv[1], "setfattr")) {
		g_loglevel = 10;
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
		paranoid_free(tmp);
		finish(1);
	}

	if (argc == 4 && !strcmp(argv[1], "getfacl")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if (!strstr(argv[2], "filelist")) {
			printf(_("Sorry - filelist goes first\n"));
			finish(1);
		} else {
			finish(get_acl_list(argv[2], argv[3]));
		}
		finish(0);
	}
	if (argc == 4 && !strcmp(argv[1], "setfacl")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		finish(set_acl_list(argv[2], argv[3]));
	}

	if (argc > 2 && !strcmp(argv[1], "find-cd")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if ((tmp = find_cdrw_device()) == NULL) {
			printf(_("Failed to find CDR-RW drive\n"));
		} else {
			printf(_("CD-RW is at %s\n"), tmp);
		}
		paranoid_free(tmp);

		if ((tmp = find_cdrom_device(FALSE)) == NULL) {
			printf(_("Failed to find CD-ROM drive\n"));
		} else {
			printf(_("CD-ROM is at %s\n"), tmp);
		}
		paranoid_free(tmp);
		finish(0);
	}

	if (argc > 2 && !strcmp(argv[1], "find-dvd")) {
		g_loglevel = 10;
		g_text_mode = TRUE;
		setup_newt_stuff();
		if ((tmp = find_dvd_device()) == NULL) {
			printf(_("Failed to find DVD drive\n"));
		} else {
			printf(_("DVD is at %s\n"), tmp);
		}
		paranoid_free(tmp);
		finish(0);
	}

	if (argc > 2 && !strcmp(argv[1], "disksize")) {
		printf("%s --> %ld\n", argv[2], get_phys_size_of_drive(argv[2]));
		finish(0);
	}
	if (argc > 2 && !strcmp(argv[1], "test-dev")) {
		if (is_dev_an_NTFS_dev(argv[2])) {
			printf(_("%s is indeed an NTFS dev\n"), argv[2]);
		} else {
			printf(_("%s is _not_ an NTFS dev\n"), argv[2]);
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
				(_("Errors were detected in the command line you supplied.\n"));
			printf(_("Please review the log file - %s \n"),MONDO_LOGFILE);
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
		(_("BusyBox's sources are available from http://www.busybox.net"));

	/* If we're meant to backup then backup */
	if (bkpinfo->backup_data) {
		res = backup_data(bkpinfo);
		retval += res;
		if (res) {
			asprintf(&say_at_end,
				   _("Data archived. Please check the logs, just as a precaution. "));
		} else {
			asprintf(&say_at_end, _("Data archived OK. "));
		}
	}

/* If we're meant to verify then verify */
	if (bkpinfo->verify_data) {
		res = verify_data(bkpinfo);
		if (res < 0) {
			asprintf(&say_at_end, _("%d difference%c found."), -res,
					(-res != 1) ? 's' : ' ');
			res = 0;
		}
		retval += res;
	}

	/* Offer to write floppy disk images to physical disks */
	if (bkpinfo->backup_data && !g_skip_floppies) {
		res = offer_to_write_boot_floppies_to_physical_disks(bkpinfo);
		retval += res;
	}

	/* Report result of entire operation (success? errors?) */
	if (retval == 0) {
		mvaddstr_and_log_it(g_currentY++, 0,
							_("Backup and/or verify ran to completion. Everything appears to be fine."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 0,
							_("Backup and/or verify ran to completion. However, errors did occur."));
	}

	if (does_file_exist("/root/images/mindi/mondorescue.iso")) {
		log_to_screen
			(_("/root/images/mindi/mondorescue.iso, a boot/utility CD, is available if you want it."));
	}


	if (length_of_file("/tmp/changed.files") > 2) {
		if (g_text_mode) {
			log_to_screen
				(_("Type 'less /tmp/changed.files' to see which files don't match the archives"));
		} else {
			log_msg(1,
					_("Type 'less /tmp/changed.files' to see which files don't match the archives"));
			log_msg(2, "Calling popup_changelist_from_file()");
			popup_changelist_from_file("/tmp/changed.files");
			log_msg(2, "Returned from popup_changelist_from_file()");
		}
	} else {
		unlink("/tmp/changed.files");
	}
	log_to_screen(say_at_end);
	paranoid_free(say_at_end);

	asprintf(&tmp, "umount %s/tmpfs", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, TRUE);
	paranoid_free(tmp);

	sprintf(g_erase_tmpdir_and_scratchdir, "rm -Rf %s %s", bkpinfo->tmpdir,
			bkpinfo->scratchdir);
	run_program_and_log_output(g_erase_tmpdir_and_scratchdir, TRUE);

	run_program_and_log_output("mount", 2);

	system("rm -f /var/cache/mondo-archive/last-backup.aborted");
	system("rm -Rf /tmp.mondo.* /mondo.scratch.*");
	if (retval == 0) {
		printf(_("Mondoarchive ran OK.\n"));
	} else {
		printf(_("Errors occurred during backup. Please check logfile.\n"));
	}
	distro_specific_kludges_at_end_of_mondoarchive();
	register_pid(0, "mondo");
	set_signals(FALSE);
	chdir("/tmp");				// just in case there's something wrong with g_erase_tmpdir_and_scratchdir
	system(g_erase_tmpdir_and_scratchdir);
	free_libmondo_global_strings();
	paranoid_free(bkpinfo);

	unlink("/tmp/filelist.full");
	unlink("/tmp/filelist.full.gz");

	run_program_and_log_output("date", 1);

	if (!g_text_mode) {
		popup_and_OK
			(_("Mondo Archive has finished its run. Please press ENTER to return to the shell prompt."));
		log_to_screen(_("See %s for details of backup run."), MONDO_LOGFILE);
		finish(retval);
	} else {
		printf(_("See %s for details of backup run.\n"), MONDO_LOGFILE);
		exit(retval);
	}

	return EXIT_SUCCESS;
}
