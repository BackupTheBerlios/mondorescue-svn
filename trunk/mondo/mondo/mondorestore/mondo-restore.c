/***************************************************************************
 * $Id$
 */

/**
 * @file
 * The main file for mondorestore.
 */

/**************************************************************************
 * #include statements                                                    *
 **************************************************************************/
#include <unistd.h>

#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#include "mr-externs.h"
#include "mondo-restore.h"
#include "mondo-rstr-compare-EXT.h"
#include "mondo-rstr-tools-EXT.h"
#ifndef S_SPLINT_S
#include <pthread.h>
#endif

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
char *g_isodir_device = NULL;

/**
 * The format of @p g_isodir_device. Ignored unless @p g_ISO_restore_mode.
 */
char *g_isodir_format = NULL;

/**
 * The location of 'biggielist.txt', containing the biggiefiles on the current archive set.
 */
char *g_biggielist_txt = NULL;

/**
 * The location of 'filelist.full', containing all files (<em>including biggiefiles</em>) on
 * the current archive set.
 */
char *g_filelist_full = NULL;

/**
 * The location of a file containing a list of the devices that were archived
 * as images, not as individual files.
 */
char *g_filelist_imagedevs = NULL;

/**
 * The location of a file containing a list of imagedevs to actually restore.
 * @see g_filelist_imagedevs
 */
char *g_imagedevs_restthese = NULL;

/**
 * The location of 'mondo-restore.cfg', containing the metadata
 * information for this backup.
 */
char *g_mondo_cfg_file = NULL;

/**
 * The location of 'mountlist.txt', containing the information on the
 * user's partitions and hard drives.
 */
char *g_mountlist_fname = NULL;

/**
 * Mondo's home directory during backup. Unused in mondo-restore; included
 * to avoid link errors.
 */
char *g_mondo_home = NULL;

/* @} - end of "Restore-Time Globals" in globalGroup */

extern int copy_from_src_to_dest(FILE * f_orig, FILE * f_archived,
								 char direction);

/**************************************************************************
 * COMPAQ PROLIANT Stuff:  needs some special help                        *
**************************************************************************/

/**
 * The message to display if we detect that the user is using a Compaq Proliant.
 */
#define COMPAQ_PROLIANTS_SUCK _("Partition and format your disk using Compaq's disaster recovery CD. After you've done that, please reboot with your Mondo CD/floppy in Interactive Mode.")


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
		log_to_screen(_("does not exist"));
		return (1);
	}

	retval = load_mountlist(mountlist, g_mountlist_fname);
	load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
	if (retval) {
		log_to_screen
			(_("Warning - load_raidtab_into_raidlist returned an error"));
	}
	res = edit_mountlist(g_mountlist_fname, mountlist, raidlist);
	if (res) {
		return (1);
	}

	save_mountlist_to_disk(mountlist, g_mountlist_fname);
	save_raidlist_to_raidtab(raidlist, RAIDTAB_FNAME);

	log_to_screen(_("I have finished editing the mountlist for you."));

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
		(_
		 ("Would you like to reboot and use your Compaq CD to prep your hard drive?")))
	{
		fatal_error(_
					("Aborting. Please reboot and prep your hard drive with your Compaq CD."));
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
	iamhere("pre wrm");
	c = which_restore_mode();
	iamhere("post wrm");
	if (c == 'I' || c == 'N' || c == 'C') {
		interactively_obtain_media_parameters_from_user(bkpinfo, FALSE);
	} else {
		popup_and_OK(_
					 ("No restoring or comparing will take place today."));
		if (is_this_device_mounted("/mnt/cdrom")) {
			run_program_and_log_output("umount /mnt/cdrom", FALSE);
		}
		if (g_ISO_restore_mode) {
			asprintf(&tmp, "umount %s", bkpinfo->isodir);
			run_program_and_log_output(tmp, FALSE);
			paranoid_free(tmp);
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

	char *tmp = NULL;
	char *tmp1 = NULL;
	char *fstab_fname = NULL;
	char *old_restpath = NULL;

	struct s_node *filelist = NULL;

	/* try to partition and format */

	log_msg(2, "interactive_mode --- starting (great, assertions OK)");

	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);

	log_msg(2, "interactive_mode --- assertions OK");

	if (g_text_mode) {
		if (!ask_me_yes_or_no
			(_
			 ("Interactive Mode + textonly = experimental! Proceed anyway?")))
		{
			fatal_error("Wise move.");
		}
	}

	iamhere("About to load config file");
	get_cfg_file_from_archive_or_bust(bkpinfo);
	read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
	iamhere("Done loading config file; resizing ML");
#ifdef __FreeBSD__
	tmp = call_program_and_get_last_line_of_output("cat /tmp/cmdline");
#else
	tmp = call_program_and_get_last_line_of_output("cat /proc/cmdline");
#endif
	if (strstr(tmp,"noresize")) {
		log_msg(1, "Not resizing mountlist.");
	} else {
		resize_mountlist_proportionately_to_suit_new_drives(mountlist);
	}
	for (done = FALSE; !done;) {
		iamhere("About to edit mountlist");
		if (g_text_mode) {
			save_mountlist_to_disk(mountlist, g_mountlist_fname);
			tmp1 = find_my_editor();
			asprintf(&tmp, "%s %s", tmp1, g_mountlist_fname);
			paranoid_free(tmp1);

			res = system(tmp);
			paranoid_free(tmp);
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
		mvaddstr_and_log_it(1, 30, _("Restoring Interactively"));
		if (bkpinfo->differential) {
			log_to_screen(_
						  ("Because this is a differential backup, disk"));
			log_to_screen(_
						  (" partitioning and formatting will not take place."));
			done = TRUE;
		} else {
			if (ask_me_yes_or_no
				(_
				 ("Do you want to erase and partition your hard drives?")))
			{
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
							(_
							 ("Warning. Errors occurred during disk partitioning."));
					}

					fmt_errs =
						format_everything(mountlist, FALSE, raidlist);
					if (!fmt_errs) {
						log_to_screen
							(_
							 ("Errors during disk partitioning were handled OK."));
						log_to_screen(_
									  ("Partitions were formatted OK despite those errors."));
						ptn_errs = 0;
					}
					if (!ptn_errs && !fmt_errs) {
						done = TRUE;
					}
				}
				paranoid_fclose(g_fprep);
			} else {
				mvaddstr_and_log_it(g_currentY++, 0,
									_
									("User opted not to partition the devices"));
				if (ask_me_yes_or_no
					(_("Do you want to format your hard drives?"))) {
					fmt_errs =
						format_everything(mountlist, TRUE, raidlist);
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
									_
									("Errors occurred. Please repartition and format drives manually."));
				done = FALSE;
			}
			if (ptn_errs & !fmt_errs) {
				mvaddstr_and_log_it(g_currentY++,
									0,
									_
									("Errors occurred during partitioning. Formatting, however, went OK."));
				done = TRUE;
			}
			if (!done) {
				if (!ask_me_yes_or_no(_("Re-edit the mountlist?"))) {
					retval++;
					iamhere("Leaving interactive_mode()");
					return (retval);
				}
			}
		}
	}

	/* mount */
	if (mount_all_devices(mountlist, TRUE)) {
		unmount_all_devices(mountlist);
		retval++;
		iamhere("Leaving interactive_mode()");
		return (retval);
	}
	/* restore */
	if ((restore_all =
		 ask_me_yes_or_no(_
						  ("Do you want me to restore all of your data?"))))
	{
		log_msg(1, "Restoring all data");
		retval += restore_everything(bkpinfo, NULL);
	} else if ((restore_all =
			 ask_me_yes_or_no
			 (_("Do you want me to restore _some_ of your data?")))) {
		old_restpath = bkpinfo->restore_path;
		for (done = FALSE; !done;) {
			unlink("/tmp/filelist.full");
			filelist = process_filelist_and_biggielist(bkpinfo);
			/* Now you have /tmp/tmpfs/filelist.restore-these and /tmp/tmpfs/biggielist.restore-these;
			   the former is a list of regular files; the latter, biggiefiles and imagedevs.
			 */
			if (filelist) {
			  gotos_suck:
// (NB: %s is where your filesystem is mounted now, by default)", MNT_RESTORING);
				if (popup_and_get_string
					(_("Restore path"), _("Restore files to where?"), bkpinfo->restore_path)) {
					if (!strcmp(bkpinfo->restore_path, "/")) {
						if (!ask_me_yes_or_no(_("Are you sure?"))) {
							paranoid_free(bkpinfo->restore_path);
							bkpinfo->restore_path = old_restpath;
							goto gotos_suck;
						}
						paranoid_alloc(bkpinfo->restore_path, "");	// so we restore to [blank]/file/name :)
					}
					log_msg(1, "Restoring subset");
					retval += restore_everything(bkpinfo, filelist);
					free_filelist(filelist);
				} else {
					bkpinfo->restore_path = old_restpath;
					free_filelist(filelist);
				}
				if (!ask_me_yes_or_no
					(_("Restore another subset of your backup?"))) {
					done = TRUE;
				}
			} else {
				done = TRUE;
			}
		}
	} else {
		mvaddstr_and_log_it(g_currentY++,
							0,
							_
							("User opted not to restore any data.                                  "));
	}
	if (retval) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							_
							("Errors occurred during the restore phase.            "));
	}

	if (ask_me_yes_or_no(_("Initialize the boot loader?"))) {
		run_boot_loader(TRUE);
	} else {
		mvaddstr_and_log_it(g_currentY++,
							0,
							_
							("User opted not to initialize the boot loader."));
	}

//  run_program_and_log_output("cp -af /etc/lvm " MNT_RESTORING "/etc/", 1);
	protect_against_braindead_sysadmins();
	retval += unmount_all_devices(mountlist);
	/*  if (restore_some || restore_all || */
	if (ask_me_yes_or_no
		(_("Label your ext2 and ext3 partitions if necessary?"))) {
		mvaddstr_and_log_it(g_currentY, 0,
							_
							("Using e2label to label your ext2,3 partitions"));
		if (does_file_exist("/tmp/fstab.new")) {
			asprintf(&fstab_fname, "/tmp/fstab.new");
		} else {
			asprintf(&fstab_fname, "/tmp/fstab");
		}
		asprintf(&tmp,
				"label-partitions-as-necessary %s < %s >> %s 2>> %s",
				g_mountlist_fname, fstab_fname, MONDO_LOGFILE,
				MONDO_LOGFILE);
		paranoid_free(fstab_fname);

		res = system(tmp);
		paranoid_free(tmp);
		if (res) {
			log_to_screen
				(_("label-partitions-as-necessary returned an error"));
			mvaddstr_and_log_it(g_currentY++, 74, _("Failed."));
		} else {
			mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		}
		retval += res;
	}

	iamhere("About to leave interactive_mode()");
	if (retval) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							_
							("Warning - errors occurred during the restore phase."));
	}
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
			log_to_screen(_("OK, I shan't restore/compare any files."));
		}
	}
	if (is_this_device_mounted(MNT_CDROM)) {
		paranoid_system("umount " MNT_CDROM);
	}
//  if (! already_mounted)
//    {
	if (system("umount /tmp/isodir 2> /dev/null")) {
		log_to_screen
			(_
			 ("WARNING - unable to unmount device where the ISO files are stored."));
	}
//    }
	return (retval);
}

/**************************************************************************
 *END_ISO_MODE                                                            *
 **************************************************************************/


static void call_me_after_the_nuke(int retval) {

	char *tmp = NULL;
	char *tmp1 = NULL;

	if (retval) {
		log_to_screen(_("Errors occurred during the nuke phase."));
		log_to_screen(_("Please visit our website at http://www.mondorescue.org for more information."));
	} else {
#ifdef __FreeBSD__
		tmp1 = call_program_and_get_last_line_of_output("cat /tmp/cmdline");
#else
		tmp1 = call_program_and_get_last_line_of_output("cat /proc/cmdline");
#endif
		if ((strstr(tmp1,"restore") == NULL) ||
			(strstr(tmp1,"RESTORE") == NULL)) {
				/* -H option */
				asprintf(&tmp,
			   		_
			   		(" Mondo has restored your system. Please remove the backup media and reboot.\n\nPlease visit our website at http://www.mondorescue.org for more information."));
				popup_and_OK(tmp);
				paranoid_free(tmp);
		}
		paranoid_free(tmp1);

		log_to_screen(_
			 ("Mondo has restored your system. Please remove the backup media and reboot."));
		log_to_screen(_
			 ("Thank you for using Mondo Rescue."));
		log_to_screen(_
			 ("Please visit our website at http://www.mondorescue.org for more information."));
	}
	g_I_have_just_nuked = TRUE;
	return;
}


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
	char *tmp = NULL;
   	char tmpA[MAX_STR_LEN];
   	char tmpB[MAX_STR_LEN];
	char tmpC[MAX_STR_LEN];

	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);

	log_msg(2, "nuke_mode --- starting");

	get_cfg_file_from_archive_or_bust(bkpinfo);
	load_mountlist(mountlist, g_mountlist_fname);	// in case read_cfg_file_into_bkpinfo updated the mountlist
#ifdef __FreeBSD__
	tmp = call_program_and_get_last_line_of_output("cat /tmp/cmdline");
#else
	tmp = call_program_and_get_last_line_of_output("cat /proc/cmdline");
#endif
	if (strstr(tmp,"noresize")) {
		log_msg(2, "Not resizing mountlist.");
	} else {
		resize_mountlist_proportionately_to_suit_new_drives(mountlist);
	}
	paranoid_free(tmp);

	if (!evaluate_mountlist(mountlist, tmpA, tmpB, tmpC)) {
		asprintf(&tmp,
				_
				("Mountlist analyzed. Result: \"%s %s %s\" Switch to Interactive Mode?"),
				tmpA, tmpB, tmpC);
		if (ask_me_yes_or_no(tmp)) {
			paranoid_free(tmp);
			retval = interactive_mode(bkpinfo, mountlist, raidlist);
			finish(retval);
		} else {
			paranoid_free(tmp);
			fatal_error("Nuke Mode aborted. ");
		}
	}
	save_mountlist_to_disk(mountlist, g_mountlist_fname);
	mvaddstr_and_log_it(1, 30, _("Restoring Automatically"));
	if (bkpinfo->differential) {
		log_to_screen(_("Because this is a differential backup, disk"));
		log_to_screen(_
					  ("partitioning and formatting will not take place."));
		res = 0;
	} else {
		if (partition_table_contains_Compaq_diagnostic_partition
			(mountlist)) {
			offer_to_abort_because_Compaq_Proliants_suck();
		} else {
			twenty_seconds_til_yikes();
			g_fprep = fopen("/tmp/prep.sh", "w");
#ifdef __FreeBSD__
			tmp = call_program_and_get_last_line_of_output("cat /tmp/cmdline");
#else
			tmp = call_program_and_get_last_line_of_output("cat /proc/cmdline");
#endif
			if (strstr(tmp,,"nopart")) {
				log_msg(2,
						"Not partitioning drives due to 'nopart' option.");
				res = 0;
			} else {
				res = partition_everything(mountlist);
				if (res) {
					log_to_screen
						(_
						 ("Warning. Errors occurred during partitioning."));
					res = 0;
				}
			}
			paranoid_free(tmp);

			retval += res;
			if (!res) {
				log_to_screen(_("Preparing to format your disk(s)"));
				sleep(1);
				sync();
				log_to_screen(_
							  ("Please wait. This may take a few minutes."));
				res += format_everything(mountlist, FALSE, raidlist);
			}
			paranoid_fclose(g_fprep);
		}
	}
	retval += res;
	if (res) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							_
							("Failed to partition and/or format your hard drives."));

		if (ask_me_yes_or_no(_("Try in interactive mode instead?"))) {
			retval = interactive_mode(bkpinfo, mountlist, raidlist);
			call_me_after_the_nuke(retval);
		} else
			if (!ask_me_yes_or_no
				(_("Would you like to try to proceed anyway?"))) {
		}
		return(retval);
	}
	retval = mount_all_devices(mountlist, TRUE);
	if (retval) {
		unmount_all_devices(mountlist);
		log_to_screen
			(_
			 ("Unable to mount all partitions. Sorry, I cannot proceed."));
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
	retval += unmount_all_devices(mountlist);
	mvaddstr_and_log_it(g_currentY,
						0,
						_
						("Using e2label to label your ext2,3 partitions"));

	asprintf(&tmp, "label-partitions-as-necessary %s < /tmp/fstab",
			g_mountlist_fname);
	res = run_program_and_log_output(tmp, TRUE);
	paranoid_free(tmp);

	if (res) {
		log_to_screen(_
					  ("label-partitions-as-necessary returned an error"));
		mvaddstr_and_log_it(g_currentY++, 74, _("Failed."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
	retval += res;
	call_me_after_the_nuke(retval);
	return(retval);
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

	char *old_restpath = NULL;

	struct mountlist_itself *mountlist = NULL;
	struct raidlist_itself *raidlist = NULL;
	struct s_node *filelist = NULL;

	log_msg(1, "restore_to_live_filesystem() - starting");
	assert(bkpinfo != NULL);

	mountlist = malloc(sizeof(struct mountlist_itself));
	raidlist = malloc(sizeof(struct raidlist_itself));

	if (!mountlist || !raidlist) {
		fatal_error("Cannot malloc() mountlist and/or raidlist");
	}

	paranoid_alloc(bkpinfo->restore_path, "/");
	if (!g_restoring_live_from_cd) {
		popup_and_OK
			(_
			 ("Please insert tape/CD/boot floppy, then hit 'OK' to continue."));
		sleep(1);
	}
	interactively_obtain_media_parameters_from_user(bkpinfo, FALSE);
	if (bkpinfo->media_device == NULL) {
		log_msg(2, "Warning - failed to find media dev");
	} else {
		log_msg(2, "bkpinfo->media_device = %s", bkpinfo->media_device);
	}


	log_msg(2, "bkpinfo->isodir = %s", bkpinfo->isodir);

	open_evalcall_form(_("Thinking..."));

	get_cfg_file_from_archive_or_bust(bkpinfo);
	read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
	load_mountlist(mountlist, g_mountlist_fname);	// in case read_cfg_file_into_bkpinfo 

	close_evalcall_form();
	retval = load_mountlist(mountlist, g_mountlist_fname);
	load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
	filelist = process_filelist_and_biggielist(bkpinfo);
	if (filelist) {
		save_filelist(filelist, "/tmp/selected-files.txt");
		old_restpath = bkpinfo->restore_path;
		if (popup_and_get_string(_("Restore path"),
								 _("Restore files to where? )"),
								 bkpinfo->restore_path)) {
			iamhere("Restoring everything");
			retval += restore_everything(bkpinfo, filelist);
		}
		free_filelist(filelist);
		bkpinfo->restore_path = old_restpath;
	}
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		log_msg(2,
				"I probably don't need to unmount or eject the CD-ROM but I'm doing it anyway.");
	}
	run_program_and_log_output("umount " MNT_CDROM, FALSE);
	if ((!bkpinfo->please_dont_eject) && (bkpinfo->media_device != NULL)) {
		eject_device(bkpinfo->media_device);
	}
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
char *
restore_a_biggiefile_from_CD(struct s_bkpinfo *bkpinfo,
							 long bigfileno,
							 struct s_node *filelist)
{
	FILE *fin = NULL;
	FILE *fout = NULL;
	FILE *fbzip2 = NULL;
	char *checksum = NULL;
   	char *outfile_fname = NULL;
   	char *tmp = NULL;
   	char *tmp1 = NULL;
   	char *tmp2 = NULL;
   	char *tmp3 = NULL;
   	char *bzip2_command = NULL;
	char *bigblk = NULL;
	char *pathname_of_last_file_restored = NULL;
	int retval = 0;
	int finished = FALSE;
	long sliceno;
	long siz;
	long siz1;
	char *ntfsprog_fifo = NULL;
	char *file_to_openout = NULL;
	struct s_filename_and_lstat_info biggiestruct;
	struct utimbuf the_utime_buf, *ubuf = NULL;
	bool use_ntfsprog_hack = FALSE;
	pid_t pid;
	int res = 0;
	int old_loglevel;
	char *sz_msg;
	struct s_node *node = NULL;

	old_loglevel = g_loglevel;
	ubuf = &the_utime_buf;
	assert(bkpinfo != NULL);

	if (!(bigblk = malloc(TAPE_BLOCK_SIZE))) {
		fatal_error("Cannot malloc bigblk");
	}

	tmp = slice_fname(bigfileno, 0, ARCHIVES_PATH, "");
	if (!(fin = fopen(tmp,"r"))) {
		log_to_screen(_("Cannot even open bigfile's info file"));
		paranoid_free(tmp);
		return (pathname_of_last_file_restored);
	}
	paranoid_free(tmp);

	memset((void *) &biggiestruct, 0, sizeof(biggiestruct));
	if (fread((void *) &biggiestruct, 1, sizeof(biggiestruct), fin) <
		sizeof(biggiestruct)) {
		log_msg(2, "Warning - unable to get biggiestruct of bigfile #%d",
				bigfileno + 1);
	}
	paranoid_fclose(fin);

	asprintf(&checksum, biggiestruct.checksum);

	if (!checksum[0]) {
		asprintf(&tmp, "Warning - bigfile %ld does not have a checksum",
				bigfileno + 1);
		log_msg(3, tmp);
		paranoid_free(tmp);
		/* BERLIOS : Useless ???
		p = checksum;
		*/
	}
	paranoid_free(checksum);

	if (!strncmp(biggiestruct.filename, "/dev/", 5))	// Whether NTFS or not :)
	{
		asprintf(&outfile_fname, biggiestruct.filename);
	} else {
		asprintf(&outfile_fname, "%s/%s", bkpinfo->restore_path,
				biggiestruct.filename);
	}

	/* skip file if we have a selective restore subset & it doesn't match */
	if (filelist != NULL) {
		node = find_string_at_node(filelist, biggiestruct.filename);
		if (!node) {
			log_msg(0, "Skipping %s (name isn't in filelist)",
					biggiestruct.filename);
			return (pathname_of_last_file_restored);
		} else if (!(node->selected)) {
			log_msg(1, "Skipping %s (name isn't in biggielist subset)",
					biggiestruct.filename);
			return (pathname_of_last_file_restored);
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
		asprintf(&ntfsprog_fifo, "/tmp/%d.%d.000", (int) (random() % 32768),
				(int) (random() % 32768));
		mkfifo(ntfsprog_fifo, 0x770);

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
		// BERLIOS: Is it the right place ??
		paranoid_free(ntfsprog_fifo);
	} else {
		use_ntfsprog_hack = FALSE;
		file_to_openout = outfile_fname;
		if (!does_file_exist(outfile_fname))	// yes, it looks weird with the '!' but it's correct that way
		{
			make_hole_for_file(outfile_fname);
		}
	}

	asprintf(&tmp, "Reassembling big file %ld (%s)", bigfileno + 1,
			outfile_fname);
	log_msg(2, tmp);
	paranoid_free(tmp);

	/*
	   last slice is zero-length and uncompressed; when we find it, we stop.
	   We DON'T wait until there are no more slices; if we did that,
	   We might stop at end of CD, not at last slice (which is 0-len and uncompd)
	 */

	asprintf(&pathname_of_last_file_restored, biggiestruct.filename);

	log_msg(3, "file_to_openout = %s", file_to_openout);
	if (!(fout = fopen(file_to_openout, "w"))) {
		log_to_screen(_("Cannot openout outfile_fname - hard disk full?"));
		return (pathname_of_last_file_restored);
	}
	log_msg(3, "Opened out to %s", outfile_fname);	// CD/DVD --> mondorestore --> ntfsclone --> hard disk itself

	for (sliceno = 1, finished = FALSE; !finished;) {
		tmp = slice_fname(bigfileno, sliceno, ARCHIVES_PATH, "");
		tmp1 = slice_fname(bigfileno, sliceno, ARCHIVES_PATH, "lzo");
		tmp2 = slice_fname(bigfileno, sliceno, ARCHIVES_PATH, "bz2");
		if (!does_file_exist(tmp) && !does_file_exist(tmp1) &&
			!does_file_exist(tmp2)) {
			log_msg(3,
					"Cannot find a data slice or terminator slice on CD %d",
					g_current_media_number);
			g_current_media_number++;
			asprintf(&tmp3,
					"Asking for %s #%d so that I may read slice #%ld\n",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number, sliceno);
			log_msg(2, tmp3);
			paranoid_free(tmp3);

			asprintf(&tmp3, _("Restoring from %s #%d"),
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_to_screen(tmp3);
			paranoid_free(tmp3);

			insist_on_this_cd_number(bkpinfo, g_current_media_number);
			log_to_screen(_("Continuing to restore."));
		} else {
			if (does_file_exist(tmp) && length_of_file(tmp) == 0) {
				log_msg(2,
						"End of bigfile # %ld (slice %ld is the terminator)",
						bigfileno + 1, sliceno);
				finished = TRUE;
				continue;
			} else {
				if (does_file_exist(tmp1)) {
					asprintf(&bzip2_command, "lzop -dc %s 2>> %s",tmp1, MONDO_LOGFILE);
				} else if (does_file_exist(tmp2)) {
						asprintf(&bzip2_command, "bzip2 -dc %s 2>> %s",tmp2, MONDO_LOGFILE);
				} else if (does_file_exist(tmp)) {
						asprintf(&bzip2_command, "");
				} else {
					log_to_screen(_("OK, that's pretty fsck0red..."));
					return (pathname_of_last_file_restored);
				}
			}

			if (bzip2_command == NULL) {
				asprintf(&bzip2_command, "cat %s 2>> %s", tmp, MONDO_LOGFILE);
			}
			asprintf(&tmp3, "Working on %s #%d, file #%ld, slice #%ld ",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number, bigfileno + 1, sliceno);
			log_msg(2, tmp3);
			if (!g_text_mode) {
				newtDrawRootText(0, g_noof_rows - 2, tmp3);
				newtRefresh();
				update_progress_form(tmp3);
			}
			paranoid_free(tmp3);

			if (!(fbzip2 = popen(bzip2_command, "r"))) {
				fatal_error("Can't run popen command");
			}
			paranoid_free(bzip2_command);

			while (!feof(fbzip2)) {
				siz = fread(bigblk, 1, TAPE_BLOCK_SIZE, fbzip2);
				if (siz > 0) {
					siz1 = fwrite(bigblk, 1, siz, fout);
					asprintf(&sz_msg, "Read %ld from fbzip2; written %ld to fout", siz, siz1);
					log_it(sz_msg);
					paranoid_free(sz_msg);
				}
			}
			paranoid_pclose(fbzip2);


			sliceno++;
			g_current_progress++;
		}
		paranoid_free(tmp);
		paranoid_free(tmp1);
		paranoid_free(tmp2);
	}
	paranoid_fclose(fout);
	g_loglevel = old_loglevel;

	if (use_ntfsprog_hack) {
		log_msg(3, "Waiting for ntfsclone to finish");
		asprintf(&tmp,
				" ps | grep \" ntfsclone \" | grep -v grep > /dev/null 2> /dev/null");
		while (system(tmp) == 0) {
			sleep(1);
		}
		paranoid_free(tmp);
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
	paranoid_free(outfile_fname);
	paranoid_free(bigblk);

	return (pathname_of_last_file_restored);
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
 * @param filelist The node structure containing the list of files to be restored.
 * If @p orig_bf_fname is not in the list, it will be ignored.
 * @return 0 for success (or skip), nonzero for failure.
 */
char *restore_a_biggiefile_from_stream(struct s_bkpinfo *bkpinfo, char *orig_bf_fname, long biggiefile_number,
									 struct s_node *filelist,
									 int use_ntfsprog)
{
	FILE *pout;
	FILE *fin;

  /** mallocs ********/
	char *tmp = NULL;
	char *tmp1 = NULL;
	char *command = NULL;
	char *outfile_fname = NULL;
	char *ntfsprog_command = NULL;
	char *ntfsprog_fifo = NULL;
	char *file_to_openout = NULL;
	char *pathname_of_last_file_restored = NULL;

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

	old_loglevel = g_loglevel;
	assert(bkpinfo != NULL);
	assert(orig_bf_fname != NULL);

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

	asprintf(&pathname_of_last_file_restored, orig_bf_fname);

	/* open out to biggiefile to be restored (or /dev/null if biggiefile is not to be restored) */

	if (filelist != NULL) {
		node = find_string_at_node(filelist, orig_bf_fname);
		if (!node) {
			dummy_restore = TRUE;
			log_msg(1,
					"Skipping big file %ld (%s) - not in biggielist subset",
					biggiefile_number + 1, orig_bf_fname);
			pathname_of_last_file_restored = NULL;
		} else if (!(node->selected)) {
			dummy_restore = TRUE;
			log_msg(1, "Skipping %s (name isn't in biggielist subset)",
					orig_bf_fname);
			pathname_of_last_file_restored = NULL;
		}
	}

	if (use_ntfsprog) {
		if (strncmp(orig_bf_fname, "/dev/", 5)) {
			log_msg(1, "I was in error when I set use_ntfsprog to TRUE.");
			log_msg(1, "%s isn't even in /dev", orig_bf_fname);
			use_ntfsprog = FALSE;
		}
	}

	if (use_ntfsprog) {
		g_loglevel = 4;
		asprintf(&outfile_fname, orig_bf_fname);
		use_ntfsprog_hack = TRUE;
		log_msg(2,
				"Calling ntfsclone in background because %s is a /dev entry",
				outfile_fname);
		asprintf(&ntfsprog_fifo, "/tmp/%d.%d.000", (int) (random() % 32768),
				(int) (random() % 32768));
		mkfifo(ntfsprog_fifo, 0x770);

		file_to_openout = ntfsprog_fifo;
		switch (pid = fork()) {
		case -1:
			fatal_error("Fork failure");
		case 0:
			log_msg(3,
					"CHILD - fip - calling feed_outfrom_ntfsprog(%s, %s)",
					outfile_fname, ntfsprog_fifo);
			res = feed_outfrom_ntfsprog(outfile_fname, ntfsprog_fifo);
//          log_msg(3, "CHILD - fip - exiting");
			exit(res);
			break;
		default:
			log_msg(3,
					"feed_into_ntfsprog() called in background --- pid=%ld",
					(long int) (pid));
		}
		paranoid_free(ntfsprog_fifo);
	} else {
		if (!strncmp(orig_bf_fname, "/dev/", 5))	// non-NTFS partition
		{
			asprintf(&outfile_fname, orig_bf_fname);
		} else					// biggiefile
		{
			asprintf(&outfile_fname, "%s/%s", bkpinfo->restore_path,
					orig_bf_fname);
		}
		use_ntfsprog_hack = FALSE;
		file_to_openout = outfile_fname;
		if (!does_file_exist(outfile_fname))	// yes, it looks weird with the '!' but it's correct that way
		{
			make_hole_for_file(outfile_fname);
		}
		asprintf(&tmp1, "Reassembling big file %ld (%s)",
				biggiefile_number + 1, orig_bf_fname);
		log_msg(2, tmp1);
		paranoid_free(tmp1);
	}

	if (dummy_restore) {
		paranoid_free(outfile_fname);
		asprintf(&outfile_fname, "/dev/null");
	}

	if (!bkpinfo->zip_exe[0]) {
		asprintf(&command, "cat > \"%s\"", file_to_openout);
	} else {
		asprintf(&command, "%s -dc > \"%s\" 2>> %s", bkpinfo->zip_exe,
				file_to_openout, MONDO_LOGFILE);
	}
	asprintf(&tmp1, "Pipe command = '%s'", command);
	log_msg(3, tmp1);
	paranoid_free(tmp1);

	/* restore biggiefile, one slice at a time */
	if (!(pout = popen(command, "w"))) {
		fatal_error("Cannot pipe out");
	}
	paranoid_free(command);

	for (res = read_header_block_from_stream(&slice_siz, tmp, &ctrl_chr);
		 ctrl_chr != BLK_STOP_A_BIGGIE;
		 res = read_header_block_from_stream(&slice_siz, tmp, &ctrl_chr)) {
		if (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		asprintf(&tmp1, "Working on file #%ld, slice #%ld    ",
				biggiefile_number + 1, current_slice_number);
		log_msg(2, tmp1);

		if (!g_text_mode) {
			newtDrawRootText(0, g_noof_rows - 2, tmp1);
			newtRefresh();
		}
		strip_spaces(tmp1);
		update_progress_form(tmp1);
		paranoid_free(tmp1);

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
	paranoid_free(tmp);
	paranoid_pclose(pout);

	log_msg(1, "pathname_of_last_file_restored is now %s",
			pathname_of_last_file_restored);

	if (use_ntfsprog_hack) {
		log_msg(3, "Waiting for ntfsclone to finish");
		asprintf(&tmp,
				" ps | grep \" ntfsclone \" | grep -v grep > /dev/null 2> /dev/null");
		while (system(tmp) == 0) {
			sleep(1);
		}	
		paranoid_free(tmp);
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

	paranoid_free(outfile_fname);
	g_loglevel = old_loglevel;
	return (pathname_of_last_file_restored);
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
	char *tmp = NULL;
	char *filelist_name;
	char *filelist_subset_fname = NULL;
	char *executable = NULL;
	char *temp_log = NULL;
	long matches = 0;
	bool use_star;
	char *xattr_fname = NULL;
	char *acl_fname = NULL;

	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);

	log_msg(5, "Entering");
	use_star = (strstr(tarball_fname, ".star")) ? TRUE : FALSE;
	asprintf(&command, "mkdir -p %s/tmp", MNT_RESTORING);
	run_program_and_log_output(command, 9);
	paranoid_free(command);

	asprintf(&filelist_name, MNT_CDROM "/archives/filelist.%ld",
			current_tarball_number);
	if (length_of_file(filelist_name) <= 2) {
		log_msg(2, "There are _zero_ files in filelist '%s'",
				filelist_name);
		log_msg(2,
				"This is a bit silly (ask dev-team to fix mondo_makefilelist, please)");
		log_msg(2,
				"but it's non-critical. It's cosmetic. Don't worry about it.");
		retval = 0;
		log_msg(5, "Leaving");
		return (retval);
	}
	if (count_lines_in_file(filelist_name) <= 0
		|| length_of_file(tarball_fname) <= 0) {
		log_msg(3, "length_of_file(%s) = %llu", tarball_fname,
				length_of_file(tarball_fname));
		asprintf(&tmp, "Unable to restore fileset #%ld (CD I/O error)",
				current_tarball_number);
		log_to_screen(tmp);
		paranoid_free(tmp);
		retval = 1;
		log_msg(5, "Leaving");
		return (retval);
	}

	if (filelist) {
		asprintf(&filelist_subset_fname, "/tmp/filelist-subset-%ld.tmp",
				current_tarball_number);
		if ((matches =
			 save_filelist_entries_in_common(filelist_name, filelist,
											 filelist_subset_fname,
											 use_star))
			<= 0) {
			asprintf(&tmp, "Skipping fileset %ld", current_tarball_number);
			log_msg(1, tmp);
			paranoid_free(tmp);
		} else {
			log_msg(3, "Saved fileset %ld's subset to %s",
					current_tarball_number, filelist_subset_fname);
		}
		asprintf(&tmp, "Tarball #%ld --- %ld matches",
				current_tarball_number, matches);
		log_to_screen(tmp);
		paranoid_free(tmp);
	} else {
		filelist_subset_fname = NULL;
	}
	paranoid_free(filelist_name);

	if (filelist == NULL || matches > 0) {
		asprintf(&xattr_fname, XATTR_LIST_FNAME_RAW_SZ,
				MNT_CDROM "/archives", current_tarball_number);
		asprintf(&acl_fname, ACL_LIST_FNAME_RAW_SZ, MNT_CDROM "/archives",
				current_tarball_number);
		if (strstr(tarball_fname, ".bz2")) {
			asprintf(&executable, "bzip2");
		} else if (strstr(tarball_fname, ".lzo")) {
			asprintf(&executable, "lzop");
		} else {
			executable = NULL;
		}

		if (executable == NULL) {
			asprintf(&tmp, "which %s > /dev/null 2> /dev/null", executable);
			if (run_program_and_log_output(tmp, FALSE)) {
				log_to_screen
					(_
					 ("(compare_a_tarball) Compression program not found - oh no!"));
				paranoid_MR_finish(1);
			}
			paranoid_free(tmp);

			asprintf(&tmp, executable);
			asprintf(&executable, "-P %s -Z", tmp);
			paranoid_free(tmp);
		}
#ifdef __FreeBSD__
#define BUFSIZE 512
#else
#define BUFSIZE (1024L*1024L)/TAPE_BLOCK_SIZE
#endif

		asprintf(&temp_log, "/tmp/%d.%d", (int) (random() % 32768),
			(int) (random() % 32768));

		if (use_star) {
			if (strstr(tarball_fname, ".bz2")) {
				asprintf(&tmp, " -bz");
			} else {
				asprintf(&tmp, "");
			}
			asprintf(&command,
					"star -x -force-remove -U " STAR_ACL_SZ
					" errctl= file=%s %s 2>> %s >> %s", tarball_fname, tmp, temp_log, temp_log);
			paranoid_free(tmp);
		} else {
			if (filelist_subset_fname != NULL) {
				asprintf(&command,
						"afio -i -M 8m -b %ld -c %ld %s -w '%s' %s 2>> %s >> %s",
						TAPE_BLOCK_SIZE,
						BUFSIZE, executable, filelist_subset_fname,
//             files_to_restore_this_time_fname,
						tarball_fname, temp_log, temp_log);
			} else {
				asprintf(&command,
						"afio -i -b %ld -c %ld -M 8m %s %s 2>> %s >> %s",
						TAPE_BLOCK_SIZE,
						BUFSIZE, executable, tarball_fname, temp_log, temp_log);
			}
		}
		paranoid_free(executable);

#undef BUFSIZE
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
		paranoid_free(command);

		if (res && length_of_file(temp_log) < 5) {
			res = 0;
		}

		log_msg(1, "Setting fattr list %s", xattr_fname);
		if (length_of_file(xattr_fname) > 0) {
			res = set_fattr_list(filelist_subset_fname, xattr_fname);
			if (res) {
				log_to_screen
					(_
					 ("Errors occurred while setting extended attributes"));
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
					(_
					 ("Errors occurred while setting access control lists"));
			} else {
				log_msg(1, "I set ACL OK");
			}
			retval += res;
		}
		if (retval) {
			asprintf(&command, "cat %s >> %s", temp_log, MONDO_LOGFILE);
			system(command);
			paranoid_free(command);
			log_msg(2, "Errors occurred while processing fileset #%d",
					current_tarball_number);
		} else {
			log_msg(2, "Fileset #%d processed OK", current_tarball_number);
		}
		unlink(xattr_fname);
		paranoid_free(xattr_fname);
	}
	if (does_file_exist("/PAUSE")) {
		popup_and_OK
			(_
			 ("Press ENTER to go on. Delete /PAUSE to stop these pauses."));
	}
	unlink(filelist_subset_fname);
	unlink(acl_fname);
	unlink(temp_log);

	paranoid_free(filelist_subset_fname);
	paranoid_free(acl_fname);
	paranoid_free(temp_log);

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
	char *tmp = NULL;
	char *command = NULL;
	char *afio_fname = NULL;
	char *filelist_fname = NULL;
	char *filelist_subset_fname = NULL;
	char *executable = NULL;
	long matches = 0;
	bool restore_this_fileset = FALSE;
	bool use_star;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);
	/* to do it with a file... */
	use_star = (strstr(tarball_fname, ".star")) ? TRUE : FALSE;
	asprintf(&tmp,
			"Restoring from fileset #%ld (%ld KB) on %s #%d",
			current_tarball_number, (long) size >> 10,
			media_descriptor_string(bkpinfo->backup_media_type),
			g_current_media_number);
	log_msg(2, tmp);
	paranoid_free(tmp);
	run_program_and_log_output("mkdir -p " MNT_RESTORING "/tmp", FALSE);

  /****************************************************************************
   * Use RAMDISK's /tmp; saves time; oh wait, it's too small                  *
   * Well, pipe from tape to afio, then; oh wait, can't do that either: bug   *
   * in afio or someting; oh darn.. OK, use tmpfs :-)                         *
   ****************************************************************************/
	asprintf(&afio_fname, "/tmp/tmpfs/archive.tmp.%ld",
			current_tarball_number);
	asprintf(&filelist_fname, "%s/filelist.%ld", bkpinfo->tmpdir,
			current_tarball_number);
	asprintf(&filelist_subset_fname, "%s/filelist-subset-%ld.tmp",
			bkpinfo->tmpdir, current_tarball_number);
	res = read_file_from_stream_to_file(bkpinfo, afio_fname, size);
	if (strstr(tarball_fname, ".star")) {
		bkpinfo->use_star = TRUE;
	}
	if (res) {
		log_msg(1, "Warning - error reading afioball from tape");
	}
	if (bkpinfo->compression_level != 0) {
		if (bkpinfo->use_star) {
			asprintf(&executable, " -bz");
		} else {
			asprintf(&executable, "-P %s -Z", bkpinfo->zip_exe);
		}
	}

	if (!filelist)				// if unconditional restore then restore entire fileset
	{
		restore_this_fileset = TRUE;
	} else						// If restoring selectively then get TOC from tarball
	{
		if (strstr(tarball_fname, ".star.")) {
			use_star = TRUE;
			asprintf(&command, "star -t file=%s %s > %s 2>> %s", afio_fname, executable, filelist_fname, MONDO_LOGFILE);
		} else {
			use_star = FALSE;
			asprintf(&command, "afio -t -M 8m -b %ld %s %s > %s 2>> %s", TAPE_BLOCK_SIZE,
					executable, afio_fname, filelist_fname, MONDO_LOGFILE);
		}
		log_msg(1, "command = %s", command);
		if (system(command)) {
			log_msg(4, "Warning - error occurred while retrieving TOC");
		}
		paranoid_free(command);
		if ((matches =
			 save_filelist_entries_in_common(filelist_fname, filelist,
											 filelist_subset_fname,
											 use_star))
			<= 0 || length_of_file(filelist_subset_fname) < 2) {
			if (length_of_file(filelist_subset_fname) < 2) {
				log_msg(1, "No matches found in fileset %ld",
						current_tarball_number);
			}
			asprintf(&tmp, "Skipping fileset %ld", current_tarball_number);
			log_msg(2, tmp);
			paranoid_free(tmp);
			restore_this_fileset = FALSE;
		} else {
			log_msg(5, "%ld matches. Saved fileset %ld's subset to %s",
					matches, current_tarball_number,
					filelist_subset_fname);
			restore_this_fileset = TRUE;
		}
	}
	unlink(filelist_fname);
	paranoid_free(filelist_fname);

// Concoct the call to star/afio to restore files
	if (strstr(tarball_fname, ".star.")) {
		// star
		if (filelist) {
			asprintf(&command, "star -x file=%s %s list=%s 2>> %s", afio_fname, executable
					filelist_subset_fname,MONDO_LOGFILE);
		} else {
			asprintf(&command,"star -x file=%s %s 2>> %s", afio_fname, executable,MONDO_LOGFILE);
		}
	} else {
		// afio
		if (filelist) {
			asprintf(&command, "afio -i -M 8m -b %ld %s -w %s %s 2>> %s", TAPE_BLOCK_SIZE, executable, filelist_subset_fname,afio_fname,MONDO_LOGFILE);
		} else {
			asprintf(&command, "afio -i -M 8m -b %ld %s %s 2>> %s", TAPE_BLOCK_SIZE, executable,afio_fname,MONDO_LOGFILE);
		}
	}
	paranoid_free(executable);

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
	paranoid_free(command);

	if (does_file_exist("/PAUSE") && current_tarball_number >= 50) {
		log_to_screen(_("Paused after set %ld"), current_tarball_number);
		popup_and_OK(_("Pausing. Press ENTER to continue."));
	}

	unlink(filelist_subset_fname);
	unlink(afio_fname);

	paranoid_free(filelist_subset_fname);
	paranoid_free(afio_fname);
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
	int res = 0;
	long noof_biggiefiles = 0L, bigfileno = 0L, total_slices = 0L;
	char *tmp = NULL;
	bool just_changed_cds = FALSE, finished = FALSE;
	char *xattr_fname = NULL;
	char *acl_fname = NULL;
	char *biggies_whose_EXATs_we_should_set = NULL;	// EXtended ATtributes
	char *pathname_of_last_biggie_restored = NULL;
	FILE *fbw = NULL;

	assert(bkpinfo != NULL);

	asprintf(&biggies_whose_EXATs_we_should_set,
			"%s/biggies-whose-EXATs-we-should-set", bkpinfo->tmpdir);
	if (!(fbw = fopen(biggies_whose_EXATs_we_should_set, "w"))) {
		log_msg(1, "Warning - cannot openout %s",
				biggies_whose_EXATs_we_should_set);
	}

	read_cfg_var(g_mondo_cfg_file, "total-slices", tmp);
	total_slices = atol(tmp);
	paranoid_free(tmp);

	asprintf(&tmp, _("Reassembling large files      "));
	mvaddstr_and_log_it(g_currentY, 0, tmp);
	paranoid_free(tmp);

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
	asprintf(&tmp, "OK, there are %ld biggiefiles in the archives",
			noof_biggiefiles);
	log_msg(2, tmp);
	paranoid_free(tmp);

	open_progress_form(_("Reassembling large files"),
					   _("I am now reassembling all the large files."),
					   _("Please wait. This may take some time."),
					   "", total_slices);
	for (bigfileno = 0, finished = FALSE; !finished;) {
		log_msg(2, "Thinking about restoring bigfile %ld", bigfileno + 1);
		tmp = slice_fname(bigfileno, 0, ARCHIVES_PATH, "");
		if (!does_file_exist(tmp)) {
			log_msg(3,
					"...but its first slice isn't on this CD. Perhaps this was a selective restore?");
			log_msg(3, "Cannot find bigfile #%ld 's first slice on %s #%d",
					bigfileno + 1,
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			tmp1 = slice_fname(bigfileno + 1, 0, ARCHIVES_PATH, "");
			log_msg(3, "Slicename would have been %s", tmp1);
			paranoid_free(tmp1);

			// I'm not positive 'just_changed_cds' is even necessary...
			if (just_changed_cds) {
				just_changed_cds = FALSE;
				log_msg(3,
						"I'll continue to scan this CD for bigfiles to be restored.");
			} else if (does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST")) {
				insist_on_this_cd_number(bkpinfo,
										 ++g_current_media_number);
				asprintf(&tmp, _("Restoring from %s #%d"),
						media_descriptor_string(bkpinfo->
												backup_media_type),
						g_current_media_number);
				log_to_screen(tmp);
				paranoid_free(tmp);

				just_changed_cds = TRUE;
			} else {
				log_msg(2, "There was no bigfile #%ld. That's OK.",
						bigfileno + 1);
				log_msg(2, "I'm going to stop restoring bigfiles now.");
				finished = TRUE;
			}
		} else {
			just_changed_cds = FALSE;
			asprintf(&tmp, _("Restoring big file %ld"), bigfileno + 1);
			update_progress_form(tmp);
			paranoid_free(tmp);

			pathname_of_last_biggie_restored =
				restore_a_biggiefile_from_CD(bkpinfo, bigfileno, filelist);
			iamhere(pathname_of_last_biggie_restored);
			if (fbw && pathname_of_last_biggie_restored[0]) {
				fprintf(fbw, "%s\n", pathname_of_last_biggie_restored);
			}
			paranoid_free(pathname_of_last_biggie_restored);
			retval += res;
			bigfileno++;

		}
		paranoid_free(tmp);
	}

	if (fbw) {
		fclose(fbw);
		asprintf(&acl_fname, ACL_BIGGLST_FNAME_RAW_SZ, ARCHIVES_PATH);
		asprintf(&xattr_fname, XATTR_BIGGLST_FNAME_RAW_SZ, ARCHIVES_PATH);
		tmp = find_home_of_exe("setfacl");
		if (length_of_file(acl_fname) > 0 && tmp) {
			set_acl_list(biggies_whose_EXATs_we_should_set, acl_fname);
		}
		paranoid_free(tmp);

		tmp = find_home_of_exe("setfattr");
		if (length_of_file(xattr_fname) > 0 && tmp) {
			set_fattr_list(biggies_whose_EXATs_we_should_set, xattr_fname);
		}
		paranoid_free(tmp);
		paranoid_free(acl_fname);
		paranoid_free(xattr_fname);
	}
	paranoid_free(biggies_whose_EXATs_we_should_set);

	if (does_file_exist("/PAUSE")) {
		popup_and_OK
			(_
			 ("Press ENTER to go on. Delete /PAUSE to stop these pauses."));
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Errors."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
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
	char *tmp1;
	char *tarball_fname;
	char *progress_str;
	char *comment;

	assert(bkpinfo != NULL);

	mvaddstr_and_log_it(g_currentY, 0, _("Restoring from archives"));
	log_msg(2,
			"Insisting on 1st CD, so that I can have a look at LAST-FILELIST-NUMBER");
	if (g_current_media_number != 1) {
		log_msg(3, "OK, that's jacked up.");
		g_current_media_number = 1;
	}
	insist_on_this_cd_number(bkpinfo, g_current_media_number);
	read_cfg_var(g_mondo_cfg_file, "last-filelist-number", tmp);
	max_val = atol(tmp) + 1;
	paranoid_free(tmp);

	asprintf(&progress_str, _("Restoring from %s #%d"),
			media_descriptor_string(bkpinfo->backup_media_type),
			g_current_media_number);
	log_to_screen(progress_str);
	open_progress_form(_("Restoring from archives"),
					   _("Restoring data from the archives."),
					   _("Please wait. This may take some time."),
					   progress_str, max_val);
	for (;;) {
		insist_on_this_cd_number(bkpinfo, g_current_media_number);
		update_progress_form(progress_str);

		asprintf(&tarball_fname, MNT_CDROM "/archives/%ld.afio.bz2",
				current_tarball_number);
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%ld.afio.lzo",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%ld.afio.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%ld.star.bz2",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%ld.star.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			if (current_tarball_number == 0) {
				log_to_screen
					(_
					 ("No tarballs. Strange. Maybe you only backed up freakin' big files?"));
				return (0);
			}
			if (!does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST")
				|| system("find " MNT_CDROM
						  "/archives/slice* > /dev/null 2> /dev/null") ==
				0) {
				break;
			}
			g_current_media_number++;
			paranoid_free(progress_str);
			asprintf(&progress_str, _("Restoring from %s #%d"),
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_to_screen(progress_str);
		} else {
			paranoid_free(progress_str);
			asprintf(&progress_str,
					_("Restoring from fileset #%ld on %s #%d"),
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
			if (res) {
				asprintf(&tmp1, _("reported errors"));
			} else {
				asprintf(&tmp1, _("succeeded"));
			}
			asprintf(&tmp, _("%s #%d, fileset #%ld - restore %s"),
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number, current_tarball_number,tmp1);
			paranoid_free(tmp1);
			
			if (attempts > 1) {
				asprintf(&tmp1, _(" (%d attempts) - review logs"), attempts);
			}
			asprintf(&comment, "%s%s", tmp, tmp1);
			paranoid_free(tmp);
			paranoid_free(tmp1);
			if (attempts > 1) {
				log_to_screen(comment);
			}
			paranoid_free(comment);

			retval += res;
			current_tarball_number++;
			g_current_progress++;
		}
		paranoid_free(tarball_fname);
	}
	paranoid_free(progress_str);
	close_progress_form();

	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Errors."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}

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
	char *tmp = NULL;
	char *biggie_fname = NULL;
	char *xattr_fname = NULL;
	char *acl_fname = NULL;
	char *p = NULL;
	char *pathname_of_last_biggie_restored = NULL;
	char *biggies_whose_EXATs_we_should_set = NULL;	// EXtended ATtributes
	long long biggie_size = NULL;
	FILE *fbw = NULL;

	assert(bkpinfo != NULL);

	read_cfg_var(g_mondo_cfg_file, "total-slices", tmp);
	total_slices = atol(tmp);
	paranoid_free(tmp);

	asprintf(&tmp, "Reassembling large files      ");
	asprintf(&xattr_fname, XATTR_BIGGLST_FNAME_RAW_SZ, bkpinfo->tmpdir);
	asprintf(&acl_fname, ACL_BIGGLST_FNAME_RAW_SZ, bkpinfo->tmpdir);
	mvaddstr_and_log_it(g_currentY, 0, tmp);
	paranoid_free(tmp);

	asprintf(&biggies_whose_EXATs_we_should_set,
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
	asprintf(&tmp, "OK, there are %ld biggiefiles in the archives",
			noof_biggiefiles);
	log_msg(2, tmp);
	paranoid_free(tmp);

	open_progress_form(_("Reassembling large files"),
					   _("I am now reassembling all the large files."),
					   _("Please wait. This may take some time."),
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
		/* BERLIOS: useless
		p = strrchr(biggie_fname, '/');
		if (!p) {
			p = biggie_fname;
		} else {
			p++;
		}
		*/
		asprintf(&tmp, _("Restoring big file %ld (%lld K)"),
				current_bigfile_number + 1, biggie_size / 1024);
		update_progress_form(tmp);
		paranoid_free(tmp);

		pathname_of_last_biggie_restored = restore_a_biggiefile_from_stream(bkpinfo, biggie_fname,
											   current_bigfile_number,
											   filelist, ctrl_chr);
		log_msg(1, "I believe I have restored %s",
				pathname_of_last_biggie_restored);
		if (fbw && pathname_of_last_biggie_restored[0]) {
			fprintf(fbw, "%s\n", pathname_of_last_biggie_restored);
		}
		paranoid_free(pathname_of_last_biggie_restored);

		retval += res;
		current_bigfile_number++;

	}
	if (current_bigfile_number != noof_biggiefiles
		&& noof_biggiefiles != 0) {
		asprintf(&tmp, "Warning - bigfileno=%ld but noof_biggiefiles=%ld\n",
				current_bigfile_number, noof_biggiefiles);
	} else {
		asprintf(&tmp,
				"%ld biggiefiles in biggielist.txt; %ld biggiefiles processed today.",
				noof_biggiefiles, current_bigfile_number);
	}
	log_msg(1, tmp);
	paranoid_free(tmp);

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
	paranoid_free(xattr_fname);
	paranoid_free(acl_fname);
	paranoid_free(biggies_whose_EXATs_we_should_set);

	if (does_file_exist("/PAUSE")) {
		popup_and_OK
			(_
			 ("Press ENTER to go on. Delete /PAUSE to stop these pauses."));
	}

	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Errors."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
	paranoid_free(biggie_fname);
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
	char *tmp = NULL;
	char *progress_str = NULL;
	char *tmp_fname = NULL;
	char *xattr_fname = NULL;
	char *acl_fname = NULL;

	long long tmp_size;

	assert(bkpinfo != NULL);
	mvaddstr_and_log_it(g_currentY, 0, _("Restoring from archives"));
	read_cfg_var(g_mondo_cfg_file, "last-filelist-number", tmp);
	max_val = atol(tmp) + 1;
	paranoid_free(tmp);

	chdir(bkpinfo->restore_path);	/* I don't know why this is needed _here_ but it seems to be. -HR, 02/04/2002 */

	run_program_and_log_output("pwd", 5);

	asprintf(&progress_str, _("Restoring from media #%d"),
			g_current_media_number);
	log_to_screen(progress_str);
	open_progress_form(_("Restoring from archives"),
					   _("Restoring data from the archives."),
					   _("Please wait. This may take some time."),
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
		asprintf(&xattr_fname, "%s/xattr-subset-%ld.tmp", bkpinfo->tmpdir,
				current_afioball_number);
		asprintf(&acl_fname, "%s/acl-subset-%ld.tmp", bkpinfo->tmpdir,
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
		/* BERLIOS: useless ?
		asprintf(&tmp,
				_("Restoring from fileset #%ld (name=%s, size=%ld K)"),
				current_afioball_number, tmp_fname, (long) tmp_size >> 10);
				*/
		res =
			restore_a_tarball_from_stream(bkpinfo, tmp_fname,
										  current_afioball_number,
										  filelist, tmp_size, xattr_fname,
										  acl_fname);
		retval += res;
		if (res) {
			asprintf(&tmp, _("Fileset %ld - errors occurred"),
					current_afioball_number);
			log_to_screen(tmp);
			paranoid_free(tmp);
		}
		res =
			read_header_block_from_stream(&tmp_size, tmp_fname, &ctrl_chr);
		if (ctrl_chr != BLK_STOP_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_STOP_AN_AFIO_OR_SLICE, ctrl_chr);
		}

		current_afioball_number++;
		g_current_progress++;

		paranoid_free(progress_str);
		asprintf(&progress_str, _("Restoring from fileset #%ld on %s #%d"),
				current_afioball_number,
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
		res =
			read_header_block_from_stream(&tmp_size, tmp_fname, &ctrl_chr);
		unlink(xattr_fname);
		unlink(acl_fname);
		paranoid_free(xattr_fname);
		paranoid_free(acl_fname);
	}							// next
	paranoid_free(progress_str);
	paranoid_free(tmp_fname);

	log_msg(1, "All done with afioballs");
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Errors."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
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
	char *cwd = NULL;
	char *newpath = NULL;
	char *tmp = NULL;
	assert(bkpinfo != NULL);

	malloc_string(cwd);
	malloc_string(newpath);
	log_msg(2, "restore_everything() --- starting");
	g_current_media_number = 1;
	/* BERLIOS: should test return value, or better change the function */
	getcwd(cwd, MAX_STR_LEN - 1);
	asprintf(&tmp, "mkdir -p %s", bkpinfo->restore_path);
	run_program_and_log_output(tmp, FALSE);
	paranoid_free(tmp);

	log_msg(1, "Changing dir to %s", bkpinfo->restore_path);
	chdir(bkpinfo->restore_path);
	/* BERLIOS: should test return value, or better change the function */
	getcwd(newpath, MAX_STR_LEN - 1);
	log_msg(1, "path is now %s", newpath);
	log_msg(1, "restoring everything");
	tmp = find_home_of_exe("petris");
	if (!tmp && !g_text_mode) {
		newtDrawRootText(0, g_noof_rows - 2,
						 _
						 ("Press ALT-<left cursor> twice to play Petris :-) "));
		newtRefresh();
	}
	paranoid_free(tmp);

	mvaddstr_and_log_it(g_currentY, 0,
						_("Preparing to read your archives"));
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		mount_cdrom(bkpinfo);
		mvaddstr_and_log_it(g_currentY++, 0,
							_
							("Restoring OS and data from streaming media"));
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
							_("Restoring OS and data from CD       "));
		mount_cdrom(bkpinfo);
		resA = restore_all_tarballs_from_CD(bkpinfo, filelist);
		resB = restore_all_biggiefiles_from_CD(bkpinfo, filelist);
	}
	chdir(cwd);
	if (resA + resB) {
		log_to_screen(_("Errors occurred while data was being restored."));
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
	return (resA + resB);
}

/**************************************************************************
 *END_RESTORE_EVERYTHING                                                  *
 **************************************************************************/


extern void wait_until_software_raids_are_prepped(char *, int);

char which_restore_mode(void);


/**
 * Log a "don't panic" message to the logfile.
 */
void welcome_to_mondorestore()
{
	log_msg(0, "-------------- Mondo Restore v%s -------------",
			PACKAGE_VERSION);
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
			"support. Without it, we can't help you.            - DevTeam");
	log_msg(0,
			"------------------------------------------------------------");
	log_msg(0,
			"BTW, despite (or perhaps because of) the wealth of messages,");
	log_msg(0,
			"some users are inclined to stop reading this log.  If Mondo ");
	log_msg(0,
			"stopped for some reason, chances are it's detailed here.    ");
	log_msg(0,
			"More than likely there's a message near the end of this     ");
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
	FILE *fin = NULL;
	FILE *fout = NULL;
	int retval = 0;
	int res = 0;
	char *tmp = NULL;

	struct mountlist_itself *mountlist = NULL;
	struct raidlist_itself *raidlist = NULL;
	struct s_bkpinfo *bkpinfo = NULL;
	struct s_node *filelist = NULL;
	char *a = NULL, *b = NULL;

  /**************************************************************************
   * hugo-                                                                  *
   * busy stuff here - it needs some comments -stan                           *
   *                                                                        *
   **************************************************************************/

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	(void) textdomain("mondo");
#endif

	if (getuid() != 0) {
		fprintf(stderr, _("Please run as root.\n"));
		exit(127);
	}

	g_loglevel = DEFAULT_MR_LOGLEVEL;

/* Configure global variables */
#ifdef __FreeBSD__
	tmp = call_program_and_get_last_line_of_output("cat /tmp/cmdline");
#else
	tmp = call_program_and_get_last_line_of_output("cat /proc/cmdline");
#endif
	if (strstr(tmp,"textonly")) {
		g_text_mode = TRUE;
		log_msg(1, "TEXTONLY MODE");
	} else {
		g_text_mode = FALSE;
	}							// newt :-)
	paranoid_free(tmp);

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

	g_mondo_home = call_program_and_get_last_line_of_output("which mondorestore"));
	sprintf(g_tmpfs_mountpt, "/tmp/tmpfs");
	make_hole_for_dir(g_tmpfs_mountpt);
	g_current_media_number = 1;	// precaution

	run_program_and_log_output("mkdir -p " MNT_CDROM, FALSE);
	run_program_and_log_output("mkdir -p /mnt/floppy", FALSE);

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
	asprintf(&tmp, "%s.orig", g_mountlist_fname);
	if (!does_file_exist(g_mountlist_fname)) {
		log_msg(2,
				"%ld: Warning - g_mountlist_fname (%s) does not exist yet",
				__LINE__, g_mountlist_fname);
	} else if (!does_file_exist(tmp)) {
		paranoid_free(tmp);
		asprintf(&tmp, "cp -f %s %s.orig", g_mountlist_fname,
				g_mountlist_fname);
		run_program_and_log_output(tmp, FALSE);
	}
	paranoid_free(tmp);

/* Init directories */
	make_hole_for_dir(bkpinfo->tmpdir);
	asprintf(&tmp, "mkdir -p %s", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, FALSE);
	paranoid_free(tmp);

	make_hole_for_dir("/var/log");
	make_hole_for_dir("/tmp/tmpfs");	/* just in case... */
	run_program_and_log_output("umount " MNT_CDROM, FALSE);
	run_program_and_log_output
		("ln -sf " MONDO_LOGFILE " /tmp/mondo-restore.log", FALSE);

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
		open_progress_form(_("Reassembling /dev/hda1"),
						   _("Shark is a bit of a silly person."),
						   _("Please wait. This may take some time."),
						   "", 1999);
		system("rm -Rf /tmp/*pih*");

		(void)restore_a_biggiefile_from_CD(bkpinfo, 42, NULL);
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
		asprintf(&a, argv[3]);
		asprintf(&b, argv[4]);

		res = save_filelist_entries_in_common(a, filelist, b, FALSE);
		free_filelist(filelist);
		paranoid_free(a);
		paranoid_free(b);
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

	if (argc == 3 && strcmp(argv[1], "--mdconv") == 0) {
		finish(create_raidtab_from_mdstat(argv[2]));
	}


	if (argc == 2 && strcmp(argv[1], "--live-grub") == 0) {
		retval = run_grub(FALSE, "/dev/hda");
		if (retval) {
			log_to_screen(_("Failed to write Master Boot Record"));
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
				(_
				 ("Live mode doesn't support command-line parameters yet."));
			paranoid_MR_finish(1);
//    return(1);
		}
		log_msg(1, "I am in normal, live mode.");
		log_msg(2, "FYI, MOUNTLIST_FNAME = %s", g_mountlist_fname);
		mount_boot_if_necessary();	/* for Gentoo users */
		log_msg(2, "Still here.");
		if (argc > 1 && strcmp(argv[argc - 1], "--live-from-cd") == 0) {
			g_restoring_live_from_cd = TRUE;
		} else {
			log_msg(2, "Calling restore_to_live_filesystem()");
			retval = restore_to_live_filesystem(bkpinfo);
		}
		log_msg(2, "Still here. Yay.");
		if (strlen(bkpinfo->tmpdir) > 0) {
			asprintf(&tmp, "rm -Rf %s/*", bkpinfo->tmpdir);
			run_program_and_log_output(tmp, FALSE);
			paranoid_free(tmp);
		}
		unmount_boot_if_necessary();	/* for Gentoo users */
		paranoid_MR_finish(retval);
	} else {
/* Disaster recovery mode (must be) */
		log_msg(1, "I must be in disaster recovery mode.");
		log_msg(2, "FYI, MOUNTLIST_FNAME = %s ", g_mountlist_fname);
		if (argc == 3 && strcmp(argv[1], "--monitas-memorex") == 0) {
			log_to_screen(_("Uh, that hasn't been implemented yet."));
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
				asprintf(&tmp, "mount %s -t nfs -o nolock /tmp/isodir",
						bkpinfo->nfs_mount);
				run_program_and_log_output(tmp, 1);
				paranoid_free(tmp);
			}
		}


		if (retval) {
			log_to_screen
				(_
				 ("Warning - load_raidtab_into_raidlist returned an error"));
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
			res = format_everything(mountlist, FALSE, raidlist);
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
				log_to_screen(_("Failed to write Master Boot Record"));
			}
		} else if (argc == 2 && strcmp(argv[1], "--isonuke") == 0) {
			iamhere("isonuke");
			retval = iso_mode(bkpinfo, mountlist, raidlist, TRUE);
		} else if (argc != 1) {
			log_to_screen(_("Invalid paremeters"));
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
				(_
				 ("See /tmp/changed.files for list of files that have changed."));
		}
		mvaddstr_and_log_it(g_currentY++,
							0,
							_
							("Run complete. Errors were reported. Please review the logfile."));
	} else {
		if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
			mvaddstr_and_log_it(g_currentY++,
								0,
								_
								("Run complete. Please remove floppy/CD/media and reboot."));
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
								_
								("Run complete. Please remove media and reboot."));
		}
	}

// g_I_have_just_nuked is set true by nuke_mode() just before it returns
	if (g_I_have_just_nuked || does_file_exist("/POST-NUKE-ANYWAY")) {
		if (!system("which post-nuke > /dev/null 2> /dev/null")) {
			log_msg(1, "post-nuke found; running...");
			if (mount_all_devices(mountlist, TRUE)) {
				log_to_screen
					(_
					 ("Unable to re-mount partitions for post-nuke stuff"));
			} else {
				log_msg(1, "Re-mounted partitions for post-nuke stuff");
				asprintf(&tmp, "post-nuke %s %d", bkpinfo->restore_path,
						retval);
				if (!g_text_mode) {
					newtSuspend();
				}
				log_msg(2, "Calling '%s'", tmp);
				if ((res = system(tmp))) {
					log_OS_error(tmp);
				}
				paranoid_free(tmp);
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
	asprintf(&tmp, "rm -Rf %s", bkpinfo->tmpdir);
	run_program_and_log_output(tmp, FALSE);
	paranoid_free(tmp);

	log_to_screen
		(_
		 ("Restore log copied to /tmp/mondo-restore.log on your hard disk"));
	asprintf(&tmp,
			_
			("Mondo-restore is exiting (retval=%d)                                      "),
			retval);
	log_to_screen(tmp);
	paranoid_free(tmp);

	asprintf(&tmp, "umount %s", bkpinfo->isodir);
	run_program_and_log_output(tmp, 5);
	paranoid_free(tmp);

	paranoid_free(mountlist);
	paranoid_free(raidlist);
	if (am_I_in_disaster_recovery_mode()) {
		run_program_and_log_output("mount / -o remount,rw", 2);
	}							// for b0rken distros
	paranoid_MR_finish(retval);	// frees global stuff plus bkpinfo
	free_libmondo_global_strings();	// it's fine to have this here :) really :)

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
