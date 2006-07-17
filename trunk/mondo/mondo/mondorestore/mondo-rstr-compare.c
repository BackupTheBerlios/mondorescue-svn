/***************************************************************************
 * $Id$ - compares mondoarchive data
**/


#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#include "mr-externs.h"
#include "mondo-rstr-compare.h"
#include "mondo-restore-EXT.h"
#include "mondo-rstr-tools-EXT.h"
#ifndef S_SPLINT_S
#include <pthread.h>
#endif

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
	char *checksum = NULL;
	char *original_cksum = NULL;
	char *bigfile_fname = NULL;
	char *tmp = NULL;
	char *tmp1 = NULL;
	char *command = NULL;

	char *p = NULL;
	int i = 0;
	int n = 0;
	int retval = 0;

	struct s_filename_and_lstat_info biggiestruct;

	assert(bkpinfo != NULL);

	tmp1 = slice_fname(bigfileno, 0, ARCHIVES_PATH, "");
	if (!does_file_exist(tmp1)) {
		if (does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST")) {
			insist_on_this_cd_number(bkpinfo, (++g_current_media_number));
		} else {
			log_msg(2, "No CD's left. No biggiefiles left. No problem.");
			return (0);
		}
	}
	if (!(fin = fopen(tmp1, "r"))) {
		asprintf(&tmp, _("Cannot open bigfile %ld (%s)'s info file"),
				bigfileno + 1, tmp);
		log_to_screen(tmp);
		paranoid_free(tmp);
		paranoid_free(tmp1);
		return (1);
	}
	fread((void *) &biggiestruct, 1, sizeof(biggiestruct), fin);
	paranoid_fclose(fin);

	asprintf(&checksum, biggiestruct.checksum);
	asprintf(&bigfile_fname, biggiestruct.filename);

	log_msg(2, "biggiestruct.filename = %s", bigfile_fname);
	log_msg(2, "biggiestruct.checksum = %s", checksum);

	if (!g_text_mode) {
		asprintf(&tmp, _("Comparing %s"), bigfile_fname);
		newtDrawRootText(0, 22, tmp);
		newtRefresh();
		paranoid_free(tmp);
	}
	/* BERLIOS: Useless ?
	if (!checksum[0]) {
		log_msg(2, "Warning - %s has no checksum", bigfile_fname_ptr);
	} */
	if (!strncmp(bigfile_fname, "/dev/", 5)) {
		log_msg(2, _("Ignoring device %s"), bigfile_fname);
		return(0);
	} else {
		asprintf(&command,
				"md5sum \"%s%s\" > /tmp/md5sum.txt 2> /tmp/errors.txt",
				MNT_RESTORING, bigfile_fname);
	}
	log_msg(2, command);
	paranoid_system("cat /tmp/errors >> /tmp/mondo-restore.log 2> /dev/null");
	if (system(command)) {
		log_OS_error("Warning - command failed");
		paranoid_free(command);
		paranoid_free(bigfile_fname);
		return (1);
	} else {
		paranoid_free(command);
		if (!(fin = fopen("/tmp/md5sum.txt", "r"))) {
			log_msg(2, "Unable to open /tmp/md5sum.txt; can't get live checksum");
			paranoid_free(bigfile_fname);
			return (1);
		} else {
			getline(&original_cksum, &n, fin);
			paranoid_fclose(fin);
			for (i = strlen(original_cksum);
				 i > 0 && original_cksum[i - 1] < 32; i--);
			original_cksum[i] = '\0';
			p = (char *) strchr(original_cksum, ' ');
			if (p) {
				*p = '\0';
			}
		}
	}
	if (!strcmp(checksum, original_cksum) != 0) {
		log_msg(1, "bigfile #%ld ('%s') ... OK", bigfileno + 1, bigfile_fname);
	} else {
		log_msg(1, "bigfile #%ld ('%s') ... changed", bigfileno + 1, bigfile_fname);
		retval++;
	}
	paranoid_free(original_cksum);
	paranoid_free(checksum);

	if (retval) {
		if (!(fout = fopen("/tmp/changed.txt", "a"))) {
			fatal_error("Cannot openout changed.txt");
		}
		fprintf(fout, "%s\n", bigfile_fname);
		paranoid_fclose(fout);
	}
	paranoid_free(bigfile_fname);

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
	char *tmp;

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
						_
						("Comparing large files                                                  "));
	open_progress_form(_("Comparing large files"),
					   _("I am now comparing the large files"),
					   _("against the filesystem. Please wait."), "",
					   noof_biggiefiles);
	for (bigfileno = 0; bigfileno < noof_biggiefiles; bigfileno++) {
		asprintf(&tmp, "Comparing big file #%ld", bigfileno + 1);
		log_msg(1, tmp);
		update_progress_form(tmp);
		paranoid_free(tmp);
		res = compare_a_biggiefile(bkpinfo, bigfileno);
		retval += res;
		g_current_progress++;
	}
	close_progress_form();
	/* BERLIOS: useless ?
	return (0);
	*/
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
	char *command = NULL;
	char *tmp = NULL;
	char *filelist_name = NULL;
	char *logfile = NULL;
	char *archiver_exe = NULL;
	char *compressor_exe = NULL;

	use_star = (strstr(tarball_fname, ".star")) ? TRUE : FALSE;
	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);
	asprintf(&filelist_name, MNT_CDROM "/archives/filelist.%d",
			current_tarball_number);

	noof_lines = count_lines_in_file(filelist_name);
	paranoid_free(filelist_name);

	if (strstr(tarball_fname, ".bz2")) {
		asprintf(&compressor_exe, "bzip2");
	} else if (strstr(tarball_fname, ".lzo")) {
		asprintf(&compressor_exe, "lzop");
	} else {
		compressor_exe = NULL;
	}

	if (use_star) {
		asprintf(&archiver_exe, "star -bz");
	} else {
		asprintf(&archiver_exe, "afio");
	}

	if (compressor_exe != NULL) {
		if (!find_home_of_exe(compressor_exe)) {
			fatal_error("(compare_a_tarball) Compression program missing");
		}
		if (use_star) {
			if (strcmp(compressor_exe, "bzip2")) {
				fatal_error
					("(compare_a_tarball) Please use only bzip2 with star");
			}
		} else {
			asprintf(&tmp, compressor_exe);
			sprintf(compressor_exe, "-P %s -Z", tmp);
			paranoid_free(tmp);
		}
	}
// star -diff H=star -bz file=....

#ifdef __FreeBSD__
#define BUFSIZE 512L
#else
#define BUFSIZE (1024L*1024L)/TAPE_BLOCK_SIZE
#endif

	asprintf(&logfile, "/tmp/afio.log.%d", current_tarball_number);
	if (use_star)				// doesn't use compressor_exe
	{
		asprintf(&command,
				"%s -diff H=star file=%s >> %s 2>> %s",
				archiver_exe, tarball_fname, logfile, logfile);
	} else {
		asprintf(&command,
				"%s -r -b %ld -M 16m -c %ld %s %s >> %s 2>> %s",
				archiver_exe,
				TAPE_BLOCK_SIZE,
				BUFSIZE, compressor_exe, tarball_fname, logfile, logfile);
	}
#undef BUFSIZE
	paranoid_free(archiver_exe);
	paranoid_free(compressor_exe);

	res = system(command);
	retval += res;
	if (res) {
		log_OS_error(command);
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
	paranoid_free(command);

	if (archiver_errors) {
		asprintf(&tmp,
				"Differences found while processing fileset #%d       ",
				current_tarball_number);
		log_msg(1, tmp);
		paranoid_free(tmp);
	}
	unlink(logfile);
	paranoid_free(logfile);
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

	char *tarball_fname = NULL;
	char *progress_str = NULL;
	char *tmp = NULL;
	long max_val;

	assert(bkpinfo != NULL);
	mvaddstr_and_log_it(g_currentY, 0, _("Comparing archives"));
	read_cfg_var(g_mondo_cfg_file, "last-filelist-number", tmp);

	max_val = atol(tmp);
	paranoid_free(tmp);

	asprintf(&progress_str, _("Comparing with %s #%d "),
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
		asprintf(&tarball_fname,
				MNT_CDROM "/archives/%d.afio.bz2", current_tarball_number);

		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%d.afio.lzo",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%d.afio.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%d.star.bz2",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			paranoid_free(tarball_fname);
			asprintf(&tarball_fname, MNT_CDROM "/archives/%d.star.",
					current_tarball_number);
		}
		if (!does_file_exist(tarball_fname)) {
			if (!does_file_exist(MNT_CDROM "/archives/NOT-THE-LAST") ||
				system("find " MNT_CDROM
					   "/archives/slice* > /dev/null 2> /dev/null")
				== 0) {
				log_msg(2, "OK, I think I'm done with tarballs...");
				paranoid_free(tarball_fname);
				break;
			}
			log_msg(2, "OK, I think it's time for another CD...");
			g_current_media_number++;
			paranoid_free(progress_str);
			asprintf(&progress_str, _("Comparing with %s #%d "),
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			log_to_screen(progress_str);
		} else {
			res = compare_a_tarball(tarball_fname, current_tarball_number);
			paranoid_free(tarball_fname);

			g_current_progress++;
			current_tarball_number++;
		}
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
	char *tmp = NULL;
	char *cwd = NULL;
	char *new = NULL;
	char *command = NULL;
	int resA = 0;
	int resB = 0;
	long noof_changed_files;

	malloc_string(cwd);
	malloc_string(new);

	assert(bkpinfo != NULL);

	getcwd(cwd, MAX_STR_LEN - 1);
	chdir(bkpinfo->restore_path);
	getcwd(new, MAX_STR_LEN - 1);
	insist_on_this_cd_number(bkpinfo, g_current_media_number);
	unlink("/tmp/changed.txt");

	resA = compare_all_tarballs(bkpinfo);
	resB = compare_all_biggiefiles(bkpinfo);
	chdir(cwd);
	noof_changed_files = count_lines_in_file("/tmp/changed.txt");
	if (noof_changed_files) {
		asprintf(&tmp, _("%ld files do not match the backup            "),
				noof_changed_files);
		//      mvaddstr_and_log_it( g_currentY++, 0, tmp );
		log_to_screen(tmp);
		paranoid_free(tmp);

		asprintf(&command, "cat /tmp/changed.txt >> %s", MONDO_LOGFILE);
		paranoid_system(command);
		paranoid_free(command);
	} else {
		asprintf(&tmp, _("All files match the backup                     "));
		mvaddstr_and_log_it(g_currentY++, 0, tmp);
		log_to_screen(tmp);
		paranoid_free(tmp);
	}

	paranoid_free(cwd);
	paranoid_free(new);

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

  /**************************************************************************
   * also deletes tmp/filelist.full & tmp/biggielist.txt _and_ tries to     *
   * restore them from start of tape, if available                          *
   **************************************************************************/
	assert(bkpinfo != NULL);
	assert(mountlist != NULL);
	assert(raidlist != NULL);

	while (get_cfg_file_from_archive(bkpinfo)) {
		if (!ask_me_yes_or_no
			(_
			 ("Failed to find config file/archives. Choose another source?")))
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
							_
							("Warning - differences found during the compare phase"));
	}

	retval += unmount_all_devices(mountlist);

	if (count_lines_in_file("/tmp/changed.txt") > 0) {
		mvaddstr_and_log_it(g_currentY++, 0,
							_
							("Differences found while files were being compared."));
		streamline_changes_file("/tmp/changed.files", "/tmp/changed.txt");
		if (count_lines_in_file("/tmp/changed.files") <= 0) {
			mvaddstr_and_log_it(g_currentY++, 0,
								_
								("...but they were logfiles and temporary files. Your archives are fine."));
			log_to_screen(_
						  ("The differences were logfiles and temporary files. Your archives are fine."));
		} else {
			q = count_lines_in_file("/tmp/changed.files");
			asprintf(&tmp, _("%ld significant difference%s found."), q,
					(q != 1) ? "s" : "");
			mvaddstr_and_log_it(g_currentY++, 0, tmp);
			log_to_screen(tmp);
			paranoid_free(tmp);

			asprintf(&tmp,
				   _("Type 'less /tmp/changed.files' for a list of non-matching files"));
			mvaddstr_and_log_it(g_currentY++, 0, tmp);
			log_to_screen(tmp);
			paranoid_free(tmp);

			log_msg(2, "calling popup_changelist_from_file()");
			popup_changelist_from_file("/tmp/changed.files");
			log_msg(2, "Returning from popup_changelist_from_file()");
		}
	} else {
		log_to_screen
			(_
			 ("No significant differences were found. Your backup is perfect."));
	}
	kill_petris();
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

	char *dir = NULL;
	char *command = NULL;

	assert(bkpinfo != NULL);
  /** needs malloc **/
	malloc_string(dir);
	getcwd(dir, MAX_STR_LEN);
	chdir(bkpinfo->restore_path);

	asprintf(&command, "cp -f /tmp/LAST-FILELIST-NUMBER %s/tmp",
			bkpinfo->restore_path);
	run_program_and_log_output(command, FALSE);
	paranoid_free(command);
	mvaddstr_and_log_it(g_currentY,
						0, _("Verifying archives against filesystem"));

	if (bkpinfo->disaster_recovery
		&& does_file_exist("/tmp/CDROM-LIVES-HERE")) {
		paranoid_free(bkpinfo->media_device);
		// last_line_of_file allocates the string
		bkpinfo->media_device = last_line_of_file("/tmp/CDROM-LIVES-HERE");
	} else {
		paranoid_free(bkpinfo->media_device);
		// find_cdrom_device allocates the string
		bkpinfo->media_device = find_cdrom_device(FALSE);
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
	char *dir = NULL;
	char *command = NULL;

	assert(bkpinfo != NULL);
	malloc_string(dir);

	getcwd(dir, MAX_STR_LEN);
	chdir(bkpinfo->restore_path);
	asprintf(&command, "cp -f /tmp/LAST-FILELIST-NUMBER %s/tmp",
			bkpinfo->restore_path);
	run_program_and_log_output(command, FALSE);
	paranoid_free(command);

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
	return (res);
}

/**************************************************************************
 *END_COMPARE_TO_TAPE                                                     *
 **************************************************************************/

/* @} - end compareGroup */
