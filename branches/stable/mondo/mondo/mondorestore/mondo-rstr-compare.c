/***************************************************************************
       mondo-compare.c  -  compares mondoarchive data
                             -------------------
    begin                : Fri Apr 25 2003
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



10/21/2003
- changed "/mnt/cdrom" to MNT_CDROM

10/18
- don't say unknown compressor if no compressor at all

09/17
- cleaned up logging & conversion-to-changed.txt
- cleaned up compare_mode()

09/16
- fixed bad malloc(),free() pairs in compare_a_biggiefile()

09/14
- compare_mode() --- a couple of strings were the wrong way round,
  e.g. changed.txt and changed.files

05/05
- exclude /dev/ * from list of changed files

04/30
- added textonly mode

04/27
- improved compare_mode() to allow for ISO/cd/crazy people

04/25
- first incarnation
*/


#include <pthread.h>
#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
//#include "../../config.h"
#include "mr-externs.h"
#include "mondo-rstr-compare.h"
#include "mondo-restore-EXT.h"
#include "mondo-rstr-tools-EXT.h"

//static char cvsid[] = "$Id$";

void popup_changelist_from_file(char *);


/**
 * @addtogroup LLcompareGroup
 * @{
 */
/**
 * Compare biggiefile number @p bigfileno with the filesystem mounted on @p MNT_RESTORING.
 * @param bkpinfo The backup information structure. Only used in insist_on_this_cd_number().
 * @param bigfileno The biggiefile number (starting from 0) to compare.
 * @note This function uses an MD5 checksum.
 */
int compare_a_biggiefile(struct s_bkpinfo *bkpinfo, long bigfileno)
{

	FILE *fin;
	FILE *fout;

  /** needs malloc *******/
	char *checksum_ptr;
	char *original_cksum_ptr;
	char *bigfile_fname_ptr;
	char *tmp_ptr;
	char *command_ptr;

	char *checksum, *original_cksum, *bigfile_fname, *tmp, *command;

	char *p;
	int i;
	int retval = 0;

	struct s_filename_and_lstat_info biggiestruct;

	malloc_string(checksum);
	malloc_string(original_cksum);
	malloc_string(bigfile_fname);
	malloc_string(tmp);
	malloc_string(command);
	malloc_string(checksum_ptr);
	malloc_string(original_cksum_ptr);
	malloc_string(bigfile_fname_ptr);
	malloc_string(command_ptr);
	malloc_string(tmp_ptr);

  /*********************************************************************
   * allocate memory clear test                sab 16 feb 2003         *
   *********************************************************************/
	assert(bkpinfo != NULL);
	memset(checksum_ptr, '\0', sizeof(checksum));
	memset(original_cksum_ptr, '\0', sizeof(original_cksum));
	memset(bigfile_fname_ptr, '\0', sizeof(bigfile_fname));
	memset(tmp_ptr, '\0', sizeof(tmp));
	memset(command_ptr, '\0', sizeof(command));
  /** end **/

	if (!does_file_exist(slice_fname(bigfileno, 0, ARCHIVES_PATH, ""))) {
		if (does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST")) {
			insist_on_this_cd_number(bkpinfo, (++g_current_media_number));
		} else {
			sprintf(tmp_ptr,
					"No CD's left. No biggiefiles left. No prob, Bob.");
			log_msg(2, tmp_ptr);
			return (0);
		}
	}
	if (!(fin = fopen(slice_fname(bigfileno, 0, ARCHIVES_PATH, ""), "r"))) {
		sprintf(tmp_ptr,
				_("Cannot open bigfile %ld (%s)'s info file"),
				bigfileno + 1, bigfile_fname_ptr);
		log_to_screen(tmp_ptr);
		return (1);
	}
	fread((void *) &biggiestruct, 1, sizeof(biggiestruct), fin);
	paranoid_fclose(fin);

	strcpy(checksum_ptr, biggiestruct.checksum);
	strcpy(bigfile_fname_ptr, biggiestruct.filename);

	log_msg(2, "biggiestruct.filename = %s", biggiestruct.filename);
	log_msg(2, "biggiestruct.checksum = %s", biggiestruct.checksum);

	sprintf(tmp_ptr, _("Comparing %s"), bigfile_fname_ptr);

	if (!g_text_mode) {
		newtDrawRootText(0, 22, tmp_ptr);
		newtRefresh();
	}
	if (!checksum[0]) {
		log_msg(2, "Warning - %s has no checksum", bigfile_fname_ptr);
	}
	if (!strncmp(bigfile_fname_ptr, "/dev/", 5)) {
		strcpy(original_cksum_ptr, "IGNORE");
	} else {
		sprintf(command_ptr,
				"md5sum \"%s%s\" > /tmp/md5sum.txt 2> /tmp/errors.txt",
				MNT_RESTORING, bigfile_fname_ptr);
	}
	log_msg(2, command_ptr);
	paranoid_system
		("cat /tmp/errors >> /tmp/mondo-restore.log 2> /dev/null");
	if (system(command_ptr)) {
		log_OS_error("Warning - command failed");
		original_cksum[0] = '\0';
		return (1);
	} else {
		if (!(fin = fopen("/tmp/md5sum.txt", "r"))) {
			log_msg(2,
					"Unable to open /tmp/md5sum.txt; can't get live checksum");
			original_cksum[0] = '\0';
			return (1);
		} else {
			fgets(original_cksum_ptr, MAX_STR_LEN - 1, fin);
			paranoid_fclose(fin);
			for (i = strlen(original_cksum_ptr);
				 i > 0 && original_cksum[i - 1] < 32; i--);
			original_cksum[i] = '\0';
			p = (char *) strchr(original_cksum_ptr, ' ');
			if (p) {
				*p = '\0';
			}
		}
	}
	sprintf(tmp_ptr, "bigfile #%ld ('%s') ", bigfileno + 1,
			bigfile_fname_ptr);
	if (!strcmp(checksum_ptr, original_cksum_ptr) != 0) {
		strcat(tmp_ptr, " ... OK");
	} else {
		strcat(tmp_ptr, "... changed");
		retval++;
	}
	log_msg(1, tmp_ptr);
	if (retval) {
		if (!(fout = fopen("/tmp/changed.txt", "a"))) {
			fatal_error("Cannot openout changed.txt");
		}
		fprintf(fout, "%s\n", bigfile_fname_ptr);
		paranoid_fclose(fout);
	}

	paranoid_free(original_cksum_ptr);
	paranoid_free(original_cksum);
	paranoid_free(bigfile_fname_ptr);
	paranoid_free(bigfile_fname);
	paranoid_free(checksum_ptr);
	paranoid_free(checksum);
	paranoid_free(command_ptr);
	paranoid_free(command);
	paranoid_free(tmp_ptr);
	paranoid_free(tmp);

	return (retval);
}

/**************************************************************************
 *END_COMPARE_A_BIGGIEFILE                                                *
 **************************************************************************/


/**
 * Compare all biggiefiles in the backup.
 * @param bkpinfo The backup information structure. Used only in compare_a_biggiefile().
 * @return 0 for success, nonzero for failure.
 */
int compare_all_biggiefiles(struct s_bkpinfo *bkpinfo)
{
	int retval = 0;
	int res;
	long noof_biggiefiles, bigfileno = 0;
	char tmp[MAX_STR_LEN];

	assert(bkpinfo != NULL);
	log_msg(1, "Comparing biggiefiles");

	if (length_of_file(BIGGIELIST) < 6) {
		log_msg(1,
				"OK, really teeny-tiny biggielist; not comparing biggiefiles");
		return (0);
	}
	noof_biggiefiles = count_lines_in_file(BIGGIELIST);
	if (noof_biggiefiles <= 0) {
		log_msg(1, "OK, no biggiefiles; not comparing biggiefiles");
		return (0);
	}
	mvaddstr_and_log_it(g_currentY, 0,
						_("Comparing large files                                                  "));
	open_progress_form(_("Comparing large files"),
					   _("I am now comparing the large files"),
					   _("against the filesystem. Please wait."), "",
					   noof_biggiefiles);
	for (bigfileno = 0; bigfileno < noof_biggiefiles; bigfileno++) {
		sprintf(tmp, "Comparing big file #%ld", bigfileno + 1);
		log_msg(1, tmp);
		update_progress_form(tmp);
		res = compare_a_biggiefile(bkpinfo, bigfileno);
		retval += res;
		g_current_progress++;
	}
	close_progress_form();
	return (0);
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Errors."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
	return (retval);
}

/**************************************************************************
 *END_COMPARE_ALL_BIGGIEFILES                                             *
 **************************************************************************/


/**
 * Compare afioball @p tarball_fname against the filesystem.
 * You must be chdir()ed to the directory where the filesystem is mounted
 * before you call this function.
 * @param tarball_fname The filename of the tarball to compare.
 * @param current_tarball_number The fileset number contained in @p tarball_fname.
 * @return 0 for success, nonzero for failure.
 */
int compare_a_tarball(char *tarball_fname, int current_tarball_number)
{
	int retval = 0;
	int res;
	long noof_lines;
	long archiver_errors;
	bool use_star;

  /***  needs malloc *********/
	char *command, *tmp, *filelist_name, *logfile, *archiver_exe,
		*compressor_exe;

	malloc_string(command);
	malloc_string(tmp);
	malloc_string(filelist_name);
	malloc_string(logfile);
	malloc_string(archiver_exe);
	malloc_string(compressor_exe);

	use_star = (strstr(tarball_fname, ".star")) ? TRUE : FALSE;
	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);
	sprintf(logfile, "/tmp/afio.log.%d", current_tarball_number);
	sprintf(filelist_name, MNT_CDROM "/archives/filelist.%d",
			current_tarball_number);

	noof_lines = count_lines_in_file(filelist_name);

	if (strstr(tarball_fname, ".bz2")) {
		strcpy(compressor_exe, "bzip2");
	} else if (strstr(tarball_fname, ".lzo")) {
		strcpy(compressor_exe, "lzop");
	} else {
		compressor_exe[0] = '\0';
	}

	if (use_star) {
		strcpy(archiver_exe, "star");
	} else {
		strcpy(archiver_exe, "afio");
	}

	if (compressor_exe[0]) {
		strcpy(tmp, compressor_exe);
		if (!find_home_of_exe(tmp)) {
			fatal_error("(compare_a_tarball) Compression program missing");
		}
		if (use_star)			// star
		{
			if (!strcmp(compressor_exe, "bzip2")) {
				strcat(archiver_exe, " -bz");
			} else {
				fatal_error
					("(compare_a_tarball) Please use only bzip2 with star");
			}
		} else					// afio
		{
			sprintf(compressor_exe, "-P %s -Z", tmp);
		}
	}
// star -diff H=star -bz file=....

#ifdef __FreeBSD__
#define BUFSIZE 512L
#else
#define BUFSIZE (1024L*1024L)/TAPE_BLOCK_SIZE
#endif
	if (use_star)				// doesn't use compressor_exe
	{
		sprintf(command,
				"%s -diff H=star file=%s >> %s 2>> %s",
				archiver_exe, tarball_fname, logfile, logfile);
	} else {
		sprintf(command,
				"%s -r -b %ld -M 16m -c %ld %s %s >> %s 2>> %s",
				archiver_exe,
				TAPE_BLOCK_SIZE,
				BUFSIZE, compressor_exe, tarball_fname, logfile, logfile);
	}
#undef BUFSIZE

	res = system(command);
	retval += res;
	if (res) {
		log_OS_error(command);
		sprintf(tmp, "Warning - afio returned error = %d", res);
	}
	if (length_of_file(logfile) > 5) {
		sprintf(command,
				"sed s/': \\\"'/\\|/ %s | sed s/'\\\": '/\\|/ | cut -d'|' -f2 | sort -u | grep -vx \"dev/.*\" >> /tmp/changed.txt",
				logfile);
		system(command);
		archiver_errors = count_lines_in_file(logfile);
	} else {
		archiver_errors = 0;
	}
	sprintf(tmp, "%ld difference%c in fileset #%d          ",
			archiver_errors, (archiver_errors != 1) ? 's' : ' ',
			current_tarball_number);
	if (archiver_errors) {
		sprintf(tmp,
				"Differences found while processing fileset #%d       ",
				current_tarball_number);
		log_msg(1, tmp);
	}
	unlink(logfile);
	paranoid_free(command);
	paranoid_free(tmp);
	paranoid_free(filelist_name);
	paranoid_free(logfile);
	malloc_string(archiver_exe);
	malloc_string(compressor_exe);
	return (retval);
}

/**************************************************************************
 *END_COMPARE_A_TARBALL                                                   *
 **************************************************************************/


/**
 * Compare all afioballs in this backup.
 * @param bkpinfo The backup media structure. Passed to other functions.
 * @return 0 for success, nonzero for failure.
 */
int compare_all_tarballs(struct s_bkpinfo *bkpinfo)
{
	int retval = 0;
	int res;
	int current_tarball_number = 0;

  /**  needs malloc **********/

	char *tarball_fname, *progress_str, *tmp;
	long max_val;

	malloc_string(tarball_fname);
	malloc_string(progress_str);
	malloc_string(tmp);

	assert(bkpinfo != NULL);
	mvaddstr_and_log_it(g_currentY, 0, _("Comparing archives"));
	read_cfg_var(g_mondo_cfg_file, "last-filelist-number", tmp);

	max_val = atol(tmp);
	sprintf(progress_str, _("Comparing with %s #%d "),
			media_descriptor_string(bkpinfo->backup_media_type),
			g_current_media_number);

	open_progress_form(_("Comparing files"),
					   _("Comparing tarballs against filesystem."),
					   _("Please wait. This may take some time."),
					   progress_str, max_val);

	log_to_screen(progress_str);

	for (;;) {
		insist_on_this_cd_number(bkpinfo, g_current_media_number);
		update_progress_form(progress_str);
		sprintf(tarball_fname,
				MNT_CDROM "/archives/%d.afio.bz2", current_tarball_number);

		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%d.afio.lzo",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%d.afio.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%d.star.bz2",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			sprintf(tarball_fname, MNT_CDROM "/archives/%d.star.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			if (!does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST") ||
				system("find " MNT_CDROM
					   "/archives/slice* > /dev/null 2> /dev/null")
				== 0) {
				log_msg(2, "OK, I think I'm done with tarballs...");
				break;
			}
			log_msg(2, "OK, I think it's time for another CD...");
			g_current_media_number++;
			sprintf(progress_str, _("Comparing with %s #%d "),
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_to_screen(progress_str);
		} else {
			res = compare_a_tarball(tarball_fname, current_tarball_number);

			g_current_progress++;
			current_tarball_number++;
		}
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Errors."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
	paranoid_free(tarball_fname);
	paranoid_free(progress_str);
	paranoid_free(tmp);
	return (retval);
}

/**************************************************************************
 *END_COMPARE_ALL_TARBALLS                                                *
 **************************************************************************/

/* @} - end LLcompareGroup */


/**
 * @addtogroup compareGroup
 * @{
 */
/**
 * Compare all data on a CD-R/CD-RW/DVD/ISO/NFS-based backup.
 * @param bkpinfo The backup information structure. Passed to other functions.
 * @return 0 for success, nonzero for failure.
 */
int compare_to_CD(struct s_bkpinfo *bkpinfo)
{
  /** needs malloc *********/
	char *tmp, *cwd, *new, *command;
	int resA = 0;
	int resB = 0;
	long noof_changed_files;

	malloc_string(tmp);
	malloc_string(cwd);
	malloc_string(new);
	malloc_string(command);

	assert(bkpinfo != NULL);

	getcwd(cwd, MAX_STR_LEN - 1);
	chdir(bkpinfo->restore_path);
	getcwd(new, MAX_STR_LEN - 1);
	sprintf(tmp, "new path is %s", new);
	insist_on_this_cd_number(bkpinfo, g_current_media_number);
	unlink("/tmp/changed.txt");

	resA = compare_all_tarballs(bkpinfo);
	resB = compare_all_biggiefiles(bkpinfo);
	chdir(cwd);
	noof_changed_files = count_lines_in_file("/tmp/changed.txt");
	if (noof_changed_files) {
		sprintf(tmp, _("%ld files do not match the backup            "),
				noof_changed_files);
		//      mvaddstr_and_log_it( g_currentY++, 0, tmp );
		log_to_screen(tmp);
		sprintf(command, "cat /tmp/changed.txt >> %s", MONDO_LOGFILE);
		paranoid_system(command);
	} else {
		sprintf(tmp, _("All files match the backup                     "));
		mvaddstr_and_log_it(g_currentY++, 0, tmp);
		log_to_screen(tmp);
	}

	paranoid_free(tmp);
	paranoid_free(cwd);
	paranoid_free(new);
	paranoid_free(command);

	return (resA + resB);
}

/**************************************************************************
 *END_COMPARE_TO_CD                                                       *
 **************************************************************************/




/**
 * Compare all data in the user's backup.
 * This function will mount filesystems, compare afioballs and biggiefiles,
 * and show the user the differences.
 * @param bkpinfo The backup information structure. Passed to other functions.
 * @param mountlist The mountlist containing partitions to mount.
 * @param raidlist The raidlist containing the user's RAID devices.
 * @return The number of errors/differences found.
 */
int
compare_mode(struct s_bkpinfo *bkpinfo,
			 struct mountlist_itself *mountlist,
			 struct raidlist_itself *raidlist)
{
	int retval = 0;
	long q;
	char *tmp;

	malloc_string(tmp);

  /**************************************************************************
   * also deletes tmp/filelist.full & tmp/biggielist.txt _and_ tries to     *
   * restore them from start of tape, if available                          *
   **************************************************************************/
	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);

	while (get_cfg_file_from_archive(bkpinfo)) {
		if (!ask_me_yes_or_no
			(_("Failed to find config file/archives. Choose another source?")))
		{
			fatal_error("Unable to find config file/archives. Aborting.");
		}
		interactively_obtain_media_parameters_from_user(bkpinfo, FALSE);
	}

	read_cfg_file_into_bkpinfo(g_mondo_cfg_file, bkpinfo);
	g_current_media_number = 1;
	mvaddstr_and_log_it(1, 30, _("Comparing Automatically"));
	iamhere("Pre-MAD");
	retval = mount_all_devices(mountlist, FALSE);
	iamhere("Post-MAD");
	if (retval) {
		unmount_all_devices(mountlist);
		return (retval);
	}
	if (bkpinfo->backup_media_type == tape
		|| bkpinfo->backup_media_type == udev) {
		retval += compare_to_tape(bkpinfo);
	} else if (bkpinfo->backup_media_type == cdstream) {
		retval += compare_to_cdstream(bkpinfo);
	} else {
		retval += compare_to_CD(bkpinfo);
	}
	if (retval) {
		mvaddstr_and_log_it(g_currentY++,
							0,
							_("Warning - differences found during the compare phase"));
	}

	retval += unmount_all_devices(mountlist);

	if (count_lines_in_file("/tmp/changed.txt") > 0) {
		mvaddstr_and_log_it(g_currentY++, 0,
							_("Differences found while files were being compared."));
		streamline_changes_file("/tmp/changed.files", "/tmp/changed.txt");
		if (count_lines_in_file("/tmp/changed.files") <= 0) {
			mvaddstr_and_log_it(g_currentY++, 0,
								_("...but they were logfiles and temporary files. Your archives are fine."));
			log_to_screen
				(_("The differences were logfiles and temporary files. Your archives are fine."));
		} else {
			q = count_lines_in_file("/tmp/changed.files");
			sprintf(tmp, _("%ld significant difference%s found."), q,
					(q != 1) ? "s" : "");
			mvaddstr_and_log_it(g_currentY++, 0, tmp);
			log_to_screen(tmp);

			strcpy(tmp,
				   _("Type 'less /tmp/changed.files' for a list of non-matching files"));
			mvaddstr_and_log_it(g_currentY++, 0, tmp);
			log_to_screen(tmp);

			log_msg(2, "calling popup_changelist_from_file()");
			popup_changelist_from_file("/tmp/changed.files");
			log_msg(2, "Returning from popup_changelist_from_file()");
		}
	} else {
		log_to_screen
			(_("No significant differences were found. Your backup is perfect."));
	}
	kill_petris();
	paranoid_free(tmp);
	return (retval);
}

/**************************************************************************
 *END_COMPARE_MODE                                                        *
 **************************************************************************/

/**
 * Compare all data on a cdstream-based backup.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->disaster_recovery
 * - @c bkpinfo->media_device
 * - @c bkpinfo->restore_path
 * @return 0 for success, nonzero for failure.
 */
int compare_to_cdstream(struct s_bkpinfo *bkpinfo)
{
	int res;

  /** needs malloc **/
	char *dir, *command;

	assert(bkpinfo != NULL);
	malloc_string(dir);
	malloc_string(command);
	getcwd(dir, MAX_STR_LEN);
	chdir(bkpinfo->restore_path);

	sprintf(command, "cp -f /tmp/LAST-FILELIST-NUMBER %s/tmp",
			bkpinfo->restore_path);
	run_program_and_log_output(command, FALSE);
	mvaddstr_and_log_it(g_currentY,
						0, _("Verifying archives against filesystem"));

	if (bkpinfo->disaster_recovery
		&& does_file_exist("/tmp/CDROM-LIVES-HERE")) {
		strcpy(bkpinfo->media_device,
			   last_line_of_file("/tmp/CDROM-LIVES-HERE"));
	} else {
		find_cdrom_device(bkpinfo->media_device, FALSE);
	}
	res = verify_tape_backups(bkpinfo);
	chdir(dir);
	if (length_of_file("/tmp/changed.txt") > 2
		&& length_of_file("/tmp/changed.files") > 2) {
		log_msg(0,
				"Type 'less /tmp/changed.files' to see which files don't match the archives");
		log_msg(2, "Calling popup_changelist_from_file()");
		popup_changelist_from_file("/tmp/changed.files");
		log_msg(2, "Returned from popup_changelist_from_file()");
	}

	mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	paranoid_free(dir);
	paranoid_free(command);
	return (res);
}

/**************************************************************************
 *END_COMPARE_CD_STREAM                                                   *
 **************************************************************************/


/**
 * Compare all data on a tape-based backup.
 * @param bkpinfo The backup information structure. Field used: @c bkpinfo->restore_path.
 * @return 0 for success, nonzero for failure.
 */
/**************************************************************************
 * F@COMPARE_TO_TAPE()                                                    *
 * compare_to_tape() -  gots me??                                         *
 *                                                                        *
 * returns: int                                                           *
 **************************************************************************/
int compare_to_tape(struct s_bkpinfo *bkpinfo)
{
	int res;
	char *dir, *command;

	assert(bkpinfo != NULL);
	malloc_string(dir);
	malloc_string(command);

	getcwd(dir, MAX_STR_LEN);
	chdir(bkpinfo->restore_path);
	sprintf(command, "cp -f /tmp/LAST-FILELIST-NUMBER %s/tmp",
			bkpinfo->restore_path);
	run_program_and_log_output(command, FALSE);
	mvaddstr_and_log_it(g_currentY,
						0, _("Verifying archives against filesystem"));
	res = verify_tape_backups(bkpinfo);
	chdir(dir);
	if (res) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Failed."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
	paranoid_free(dir);
	paranoid_free(command);
	return (res);
}

/**************************************************************************
 *END_COMPARE_TO_TAPE                                                     *
 **************************************************************************/

/* @} - end compareGroup */
