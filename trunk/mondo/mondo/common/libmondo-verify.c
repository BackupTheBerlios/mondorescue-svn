/* $Id$ */

/**
 * @file
 * Functions for verifying backups (booted from hard drive, not CD).
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-verify.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-stream-EXT.h"
#include "libmondo-string-EXT.h"
#include "libmondo-devices-EXT.h"
#include "libmondo-tools-EXT.h"
#include "lib-common-externs.h"

/*@unused@*/
//static char cvsid[] = "$Id$";

char *vfy_tball_fname(struct s_bkpinfo *, char *, int);


/**
 * The number of the most recently verified afioball.
 * @ingroup globalGroup
 */
int g_last_afioball_number = -1;


/**
 * Generate a list of the files that have changed, based on @c afio @c -r
 * messages.
 * @param changedfiles_fname Filename of the place to put a list of afio's reported changed.
 * @param ignorefiles_fname Filename of a list of files to ignore (use "" if none).
 * @param stderr_fname File containing afio's stderr output.
 * @return The number of differences found (0 for a perfect backup).
 * @bug This function seems orphaned.
 * @ingroup utilityGroup
 */
long
generate_list_of_changed_files(char *changedfiles_fname,
							   char *ignorefiles_fname, char *stderr_fname)
{
	/*@ buffer ********************************************************** */
	char *command;
	char *afio_found_changes;

	/*@ int ************************************************************* */
	int res = 0;

	/*@ long ************************************************************ */
	long afio_diffs = 0;

	assert_string_is_neither_NULL_nor_zerolength(changedfiles_fname);
	assert_string_is_neither_NULL_nor_zerolength(ignorefiles_fname);
	assert_string_is_neither_NULL_nor_zerolength(stderr_fname);

	asprintf(&afio_found_changes, "%s.afio", ignorefiles_fname);
	paranoid_system("sync");

/*  sprintf (command,
	   "cat %s | grep \"afio: \" | awk '{j=substr($0,8); i=index(j,\": \");printf \"/%%s\\n\",substr(j,1,i-2);}' | sort | uniq | grep -v \"incheckentry.*xwait\" | grep -vx \"/afio:.*\" | grep -vx \"/dev/.*\" > %s",
	   stderr_fname, afio_found_changes);
*/

	log_msg(1, "Now scanning log file for 'afio: ' stuff");
	asprintf(&command,
			 "cat %s | grep \"afio: \" | sed 's/afio: //' | grep -vx \"/dev/.*\" >> %s",
			 stderr_fname, afio_found_changes);
	log_msg(2, command);
	res = system(command);
	paranoid_free(command);
	if (res) {
		log_msg(2, "Warning - failed to think");
	}

	log_msg(1, "Now scanning log file for 'star: ' stuff");
	asprintf(&command,
			 "cat %s | grep \"star: \" | sed 's/star: //' | grep -vx \"/dev/.*\" >> %s",
			 stderr_fname, afio_found_changes);
	log_msg(2, command);
	res = system(command);
	paranoid_free(command);
	if (res) {
		log_msg(2, "Warning - failed to think");
	}
//  exclude_nonexistent_files (afio_found_changes);
	afio_diffs = count_lines_in_file(afio_found_changes);
	asprintf(&command,
			 "cat %s %s %s | sort | uniq -c | awk '{ if ($1==\"2\") {print $2;};}' | grep -v \"incheckentry xwait()\" > %s",
			 ignorefiles_fname, afio_found_changes, afio_found_changes,
			 changedfiles_fname);
	log_msg(2, command);
	paranoid_system(command);
	paranoid_free(command);
	paranoid_free(afio_found_changes);
	return (afio_diffs);
}


/**
 * @addtogroup LLverifyGroup
 * @{
 */
/**
 * Verify all afioballs stored on the inserted CD (or an ISO image).
 * @param bkpinfo The backup information structure. @c bkpinfo->backup_media_type
 * is used in this function, and the structure is also passed to verify_an_afioball_from_CD().
 * @param mountpoint The location the CD/DVD/ISO is mounted on.
 * @return The number of sets containing differences (0 for success).
 */
int verify_afioballs_on_CD(struct s_bkpinfo *bkpinfo, char *mountpoint)
{

	/*@ buffers ********************************************************* */
	char *tmp;

	/*@ int ************************************************************* */
	int set_number = 0;
	int retval = 0;
	int total_sets = 0;
	int percentage = 0;

	assert_string_is_neither_NULL_nor_zerolength(mountpoint);
	assert(bkpinfo != NULL);

	for (set_number = 0;
		 set_number < 9999
		 &&
		 !does_file_exist(vfy_tball_fname
						  (bkpinfo, mountpoint, set_number));
		 set_number++);
	if (!does_file_exist(vfy_tball_fname(bkpinfo, mountpoint, set_number))) {
		return (0);
	}

	if (g_last_afioball_number != set_number - 1) {
		if (set_number == 0) {
			log_msg(1,
					"Weird error in verify_afioballs_on_CD() but it's really a cosmetic error, nothing more");
		} else {
			retval++;
			asprintf(&tmp, "Warning - missing set(s) between %d and %d\n",
					 g_last_afioball_number, set_number - 1);
			log_to_screen(tmp);
			paranoid_free(tmp);
		}
	}
	asprintf(&tmp, "Verifying %s #%d's tarballs",
			 media_descriptor_string(bkpinfo->backup_media_type),
			 g_current_media_number);
	open_evalcall_form(tmp);
	paranoid_free(tmp);

	for (total_sets = set_number;
		 does_file_exist(vfy_tball_fname(bkpinfo, mountpoint, total_sets));
		 total_sets++) {
		log_msg(1, "total_sets = %d", total_sets);
	}
	for (;
		 does_file_exist(vfy_tball_fname(bkpinfo, mountpoint, set_number));
		 set_number++) {
		percentage =
			(set_number - g_last_afioball_number) * 100 / (total_sets -
														   g_last_afioball_number);
		update_evalcall_form(percentage);
		log_msg(1, "set = %d", set_number);
		retval +=
			verify_an_afioball_from_CD(bkpinfo,
									   vfy_tball_fname(bkpinfo, mountpoint,
													   set_number));
	}
	g_last_afioball_number = set_number - 1;
	close_evalcall_form();
	return (retval);
}


/**
 * Verify all slices stored on the inserted CD (or a mounted ISO image).
 * @param bkpinfo The backup information structure. Fields used:
 * - @c compression_level
 * - @c restore_path
 * - @c use_lzo
 * - @c zip_exe
 * - @c zip_suffix
 * @param mtpt The mountpoint the CD/DVD/ISO is mounted on.
 * @return The number of differences (0 for perfect biggiefiles).
 */
int verify_all_slices_on_CD(struct s_bkpinfo *bkpinfo, char *mtpt)
{

	/*@ buffer ********************************************************** */
	char *tmp;
	char *mountpoint;
//  char ca, cb;
	char *command;
	char *sz_exe;
	static char *bufblkA = NULL;
	static char *bufblkB = NULL;
	const long maxbufsize = 65536L;
	long currsizA = 0;
	long currsizB = 0;
	long j;

	/*@ long ************************************************************ */
	long bigfile_num = 0;
	long slice_num = -1;
	int res;

	static FILE *forig = NULL;
	static struct s_filename_and_lstat_info biggiestruct;
	static long last_bigfile_num = -1;
	static long last_slice_num = -1;
	FILE *pin;
	FILE *fin;
	int retval = 0;
//  long long outlen;

	malloc_string(sz_exe);
	if (!bufblkA) {
		if (!(bufblkA = malloc(maxbufsize))) {
			fatal_error("Cannot malloc bufblkA");
		}
	}
	if (!bufblkB) {
		if (!(bufblkB = malloc(maxbufsize))) {
			fatal_error("Cannot malloc bufblkB");
		}
	}

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(mtpt);

	if (bkpinfo->compression_level > 0) {
		if (bkpinfo->use_lzo) {
			asprintf(&sz_exe, "lzop");
		} else {
			asprintf(&sz_exe, "bzip2");
		}
	} else {
		asprintf(&sz_exe, "");
	}

	iamhere("before vsbf");
	asprintf(&tmp, "Verifying %s#%d's big files",
			 media_descriptor_string(bkpinfo->backup_media_type),
			 g_current_media_number);
	open_evalcall_form(tmp);
	paranoid_free(tmp);
	iamhere("after vsbf");
	asprintf(&mountpoint, "%s/archives", mtpt);
	if (last_bigfile_num == -1) {
		bigfile_num = 0;
		slice_num = 0;
	} else if (slice_num == 0) {
		bigfile_num = last_bigfile_num + 1;
		slice_num = 0;
	} else {
		bigfile_num = last_bigfile_num;
		slice_num = last_slice_num + 1;
	}
	while (does_file_exist
		   (slice_fname
			(bigfile_num, slice_num, mountpoint, bkpinfo->zip_suffix))
		   ||
		   does_file_exist(slice_fname
						   (bigfile_num, slice_num, mountpoint, ""))) {
// handle slices until end of CD
		if (slice_num == 0) {
			log_msg(2, "ISO=%d  bigfile=%ld --START--",
					g_current_media_number, bigfile_num);
			if (!
				(fin =
				 fopen(slice_fname(bigfile_num, slice_num, mountpoint, ""),
					   "r"))) {
				log_msg(2, "Cannot open bigfile's info file");
			} else {
				if (fread
					((void *) &biggiestruct, 1, sizeof(biggiestruct),
					 fin) < sizeof(biggiestruct)) {
					log_msg(2, "Unable to get biggiestruct");
				}
				paranoid_fclose(fin);
			}
			asprintf(&tmp, "%s/%s", bkpinfo->restore_path,
					 biggiestruct.filename);
			log_msg(2, "Opening biggiefile #%ld - '%s'", bigfile_num, tmp);
			if (!(forig = fopen(tmp, "r"))) {
				log_msg(2, "Failed to open bigfile. Darn.");
				retval++;
			}
			paranoid_free(tmp);

			slice_num++;
		} else if (does_file_exist
				   (slice_fname(bigfile_num, slice_num, mountpoint, ""))) {
			log_msg(2, "ISO=%d  bigfile=%ld ---END---",
					g_current_media_number, bigfile_num);
			bigfile_num++;
			paranoid_fclose(forig);
			slice_num = 0;
		} else {
			log_msg(2, "ISO=%d  bigfile=%ld  slice=%ld  \r",
					g_current_media_number, bigfile_num, slice_num);
			if (bkpinfo->compression_level > 0) {
				asprintf(&command, "cat %s | %s -dc 2>> %s",
						 slice_fname(bigfile_num, slice_num, mountpoint,
									 bkpinfo->zip_suffix), sz_exe,
						 MONDO_LOGFILE);
			} else {
				asprintf(&command, "cat %s",
						 slice_fname(bigfile_num, slice_num, mountpoint,
									 bkpinfo->zip_suffix));
			}
			if ((pin = popen(command, "r"))) {
				res = 0;
				while (!feof(pin)) {
					currsizA = fread(bufblkA, 1, maxbufsize, pin);
					if (currsizA <= 0) {
						break;
					}
					currsizB = fread(bufblkB, 1, currsizA, forig);
					if (currsizA != currsizB) {
						res++;
					} else {
						for (j = 0;
							 j < currsizA && bufblkA[j] == bufblkB[j];
							 j++);
						if (j < currsizA) {
							res++;
						}
					}
				}
				paranoid_pclose(pin);
				if (res && !strncmp(biggiestruct.filename, " /dev/", 5)) {
					log_msg(3,
							"Ignoring differences between %s and live filesystem because it's a device and therefore the archives are stored via partimagehack, not dd.",
							biggiestruct.filename);
					log_msg(3,
							"If you really want verification for %s, please contact the devteam and offer an incentive.",
							biggiestruct.filename);
					res = 0;
				}
				if (res) {
					log_msg(0,
							"afio: \"%s\": Corrupt biggie file, says libmondo-archive.c",
							biggiestruct.filename);
					retval++;
				}
			}
			paranoid_free(command);
			slice_num++;
		}
	}
	paranoid_free(mountpoint);
	paranoid_free(sz_exe);

	last_bigfile_num = bigfile_num;
	last_slice_num = slice_num - 1;
	if (last_slice_num < 0) {
		last_bigfile_num--;
	}
	close_evalcall_form();
	if (bufblkA) {
		paranoid_free(bufblkA);
	}
	if (bufblkB) {
		paranoid_free(bufblkB);
	}
	return (0);
}


/**
 * Verify one afioball from the CD.
 * You should be changed to the root directory (/) for this to work.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->use_lzo
 * - @c bkpinfo->tmpdir
 * - @c bkpinfo->zip_exe
 * - @c bkpinfo->zip_suffix
 * @param tarball_fname The filename of the afioball to verify.
 * @return 0, always.
 */
int verify_a_tarball(struct s_bkpinfo *bkpinfo, char *tarball_fname)
{
	/*@ buffers ********************************************************* */
	char *command;
	char *outlog;
	char *tmp = NULL;
	//  char *p;

	/*@ pointers ******************************************************* */
	FILE *pin;

	size_t n = 0;

	/*@ long *********************************************************** */
	long diffs = 0;
	/*  getcwd(old_pwd,MAX_STR_LEN-1); */

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);

	log_it("Verifying fileset '%s'", tarball_fname);

	/*  chdir("/"); */
	asprintf(&outlog, "%s/afio.log", bkpinfo->tmpdir);

	/* if programmer forgot to say which compression thingy to use then find out */
	if (strstr(tarball_fname, ".lzo")
		&& strcmp(bkpinfo->zip_suffix, "lzo")) {
		log_msg(2, "OK, I'm going to start using lzop.");
		strcpy(bkpinfo->zip_exe, "lzop");
		strcpy(bkpinfo->zip_suffix, "lzo");
		bkpinfo->use_lzo = TRUE;
	}
	if (strstr(tarball_fname, ".bz2")
		&& strcmp(bkpinfo->zip_suffix, "bz2")) {
		log_msg(2, "OK, I'm going to start using bzip2.");
		strcpy(bkpinfo->zip_exe, "bzip2");
		strcpy(bkpinfo->zip_suffix, "bz2");
		bkpinfo->use_lzo = FALSE;
	}
	unlink(outlog);
	if (strstr(tarball_fname, ".star")) {
		bkpinfo->use_star = TRUE;
		if (strstr(tarball_fname, ".bz2"))
			asprintf(&command,
					 "star -diff diffopts=mode,size,data file=%s %s >> %s 2>> %s",
					 tarball_fname,
					 (strstr(tarball_fname, ".bz2")) ? "-bz" : " ", outlog,
					 outlog);
	} else {
		bkpinfo->use_star = FALSE;
		asprintf(&command, "afio -r -P %s -Z %s >> %s 2>> %s",
				 bkpinfo->zip_exe, tarball_fname, outlog, outlog);
	}
	log_msg(6, "command=%s", command);
	paranoid_system(command);
	paranoid_free(command);

	if (length_of_file(outlog) < 10) {
		asprintf(&command, "cat %s >> %s", outlog, MONDO_LOGFILE);
	} else {
		asprintf(&command, "cat %s | cut -d':' -f%d | sort | uniq", outlog,
				 (bkpinfo->use_star) ? 1 : 2);
		pin = popen(command, "r");
		if (pin) {
			for (getline(&tmp, &n, pin); !feof(pin);
				 getline(&tmp, &n, pin)) {
				if (bkpinfo->use_star) {
					if (!strstr(tmp, "diffopts=")) {
						while (strlen(tmp) > 0
							   && tmp[strlen(tmp) - 1] < 32) {
							tmp[strlen(tmp) - 1] = '\0';
						}
						if (strchr(tmp, '/')) {
							if (!diffs) {
								log_msg(0, "'%s' - differences found",
										tarball_fname);
							}
							log_msg(0, "star: /%s",
									strip_afio_output_line(tmp));
							diffs++;
						}
					}
				} else {
					if (!diffs) {
						log_msg(0, "'%s' - differences found",
								tarball_fname);
					}
					log_msg(0, "afio: /%s", strip_afio_output_line(tmp));
					diffs++;
				}
			}
			paranoid_pclose(pin);
			paranoid_free(tmp);
		} else {
			log_OS_error(command);
		}
	}
	paranoid_free(outlog);
	paranoid_free(command);

	/*  chdir(old_pwd); */
	//  sprintf (tmp, "cat %s | uniq -u >> %s", "/tmp/mondo-verify.err", MONDO_LOGFILE);
	//  paranoid_system (tmp);
	//  unlink ("/tmp/mondo-verify.err");
	return (0);
}


/**
 * Verify one afioball from the CD.
 * Checks for existence (calls fatal_error() if it does not exist) and 
 * then calls verify_an_afioball().
 * @param bkpinfo The backup information structure. Passed to verify_an_afioball().
 * @param tarball_fname The filename of the afioball to verify.
 * @return The return value of verify_an_afioball().
 * @see verify_an_afioball
 */
int
verify_an_afioball_from_CD(struct s_bkpinfo *bkpinfo, char *tarball_fname)
{

	/*@ int ************************************************************* */
	int res = 0;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(tarball_fname);

	log_msg(1, "Verifying %s", tarball_fname);
	if (!does_file_exist(tarball_fname)) {
		fatal_error("Cannot verify nonexistent afioball");
	}
	res = verify_a_tarball(bkpinfo, tarball_fname);
	return (res);
}


/**
 * Verify one afioball from the opened tape/CD stream.
 * Copies the file from tape to tmpdir and then calls verify_an_afioball().
 * @param bkpinfo The backup information structure. Passed to verify_an_afioball().
 * @param orig_fname The original filename of the afioball to verify.
 * @param size The size of the afioball to verify.
 * @return The return value of verify_an_afioball().
 * @see verify_an_afioball
 */
int
verify_an_afioball_from_stream(struct s_bkpinfo *bkpinfo, char *orig_fname,
							   long long size)
{

	/*@ int ************************************************************** */
	int retval = 0;
	int res = 0;

	/*@ buffers ********************************************************** */
	char *tmp;
	char *tarball_fname;

	/*@ pointers ********************************************************* */
	char *p;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(orig_fname);

	p = strrchr(orig_fname, '/');
	if (!p) {
		p = orig_fname;
	} else {
		p++;
	}
	asprintf(&tmp, "mkdir -p %s/tmpfs", bkpinfo->tmpdir);
	paranoid_system(tmp);
	paranoid_free(tmp);

	asprintf(&tarball_fname, "%s/tmpfs/temporary-%s", bkpinfo->tmpdir, p);
	/* BERLIOS : useless
	   asprintf(&tmp, "Temporarily copying file from tape to '%s'",
	   tarball_fname);
	   log_it(tmp); 
	   paranoid_free(tmp);
	 */
	read_file_from_stream_to_file(bkpinfo, tarball_fname, size);
	res = verify_a_tarball(bkpinfo, tarball_fname);
	if (res) {
		asprintf(&tmp,
				 "Afioball '%s' no longer matches your live filesystem",
				 p);
		log_msg(0, tmp);
		paranoid_free(tmp);
		retval++;
	}
	unlink(tarball_fname);
	paranoid_free(tarball_fname);
	return (retval);
}


/**
 * Verify one biggiefile form the opened tape/CD stream.
 * @param bkpinfo The backup information structure. @c bkpinfo->tmpdir is the only field used.
 * @param biggie_fname The filename of the biggiefile to verify.
 * @param size The size in bytes of said biggiefile.
 * @return 0 for success (even if the file doesn't match); nonzero for a tape error.
 */
int
verify_a_biggiefile_from_stream(struct s_bkpinfo *bkpinfo,
								char *biggie_fname, long long size)
{

	/*@ int ************************************************************* */
	int retval = 0;
	int res = 0;
	int current_slice_number = 0;
	int ctrl_chr = '\0';

	/*@ char ************************************************************ */
	char *test_file;
	char *biggie_cksum;
	char *orig_cksum;
	char *tmp;
	char *slice_fnam;

	/*@ pointers ******************************************************** */
	char *p;

	/*@ long long ******************************************************* */
	long long slice_siz;

	malloc_string(slice_fnam);
	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(biggie_fname);

	p = strrchr(biggie_fname, '/');
	if (!p) {
		p = biggie_fname;
	} else {
		p++;
	}
	asprintf(&test_file, "%s/temporary-%s", bkpinfo->tmpdir, p);
	/* BERLIOS: useless
	   asprintf(&tmp,
	   "Temporarily copying biggiefile %s's slices from tape to '%s'",
	   p, test_file);
	   log_it(tmp); 
	   paranoid_free(tmp);
	 */
	for (res =
		 read_header_block_from_stream(&slice_siz, slice_fnam, &ctrl_chr);
		 ctrl_chr != BLK_STOP_A_BIGGIE;
		 res =
		 read_header_block_from_stream(&slice_siz, slice_fnam,
									   &ctrl_chr)) {
		if (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		res = read_file_from_stream_to_file(bkpinfo, test_file, slice_siz);
		unlink(test_file);
		res =
			read_header_block_from_stream(&slice_siz, slice_fnam,
										  &ctrl_chr);
		if (ctrl_chr != BLK_STOP_AN_AFIO_OR_SLICE) {
			log_msg(2, "test_file = %s", test_file);
			wrong_marker(BLK_STOP_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		current_slice_number++;
		retval += res;
	}
	paranoid_free(test_file);

	asprintf(&biggie_cksum, slice_fnam);
	paranoid_free(slice_fnam);

	if (biggie_cksum[0] != '\0') {
		asprintf(&orig_cksum, calc_checksum_of_file(biggie_fname));
		if (strcmp(biggie_cksum, orig_cksum)) {
			asprintf(&tmp, "orig cksum=%s; curr cksum=%s", biggie_cksum,
					 orig_cksum);
			log_msg(2, tmp);
			paranoid_free(tmp);

			asprintf(&tmp, "%s has changed on live filesystem",
					 biggie_fname);
			log_to_screen(tmp);
			paranoid_free(tmp);

			asprintf(&tmp, "echo \"%s\" >> /tmp/biggies.changed",
					 biggie_fname);
			system(tmp);
			paranoid_free(tmp);
		}
		paranoid_free(orig_cksum);
	}
	paranoid_free(biggie_cksum);

	return (retval);
}


/**
 * Verify all afioballs from the opened tape/CD stream.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->restore_path
 * - @c bkpinfo->tmpdir
 *
 * @return 0 for success (even if there are differences); nonzero for a tape error.
 */
int verify_afioballs_from_stream(struct s_bkpinfo *bkpinfo)
{
	/*@ int ********************************************************** */
	int retval = 0;
	int res = 0;
	long current_afioball_number = 0;
	int ctrl_chr = 0;
	int total_afioballs = 0;

	/*@ buffers ***************************************************** */
	char *tmp;
	char *fname;
	char *curr_xattr_list_fname;
	char *curr_acl_list_fname;

	/*@ long long *************************************************** */
	long long size = 0;

	assert(bkpinfo != NULL);
	malloc_string(fname);

	asprintf(&curr_xattr_list_fname, XATTR_BIGGLST_FNAME_RAW_SZ,
			 bkpinfo->tmpdir);
	asprintf(&curr_acl_list_fname, ACL_BIGGLST_FNAME_RAW_SZ,
			 bkpinfo->tmpdir);
	log_to_screen("Verifying regular archives on tape");
	total_afioballs = get_last_filelist_number(bkpinfo) + 1;
	open_progress_form("Verifying filesystem",
					   "I am verifying archives against your live filesystem now.",
					   "Please wait. This may take a couple of hours.", "",
					   total_afioballs);
	res = read_header_block_from_stream(&size, fname, &ctrl_chr);
	if (ctrl_chr != BLK_START_AFIOBALLS) {
		iamhere("YOU SHOULD NOT GET HERE");
		iamhere("Grabbing the EXAT files");
		if (ctrl_chr == BLK_START_EXTENDED_ATTRIBUTES) {
			res =
				read_EXAT_files_from_tape(bkpinfo, &size, fname, &ctrl_chr,
										  curr_xattr_list_fname,
										  curr_acl_list_fname);
		}
	}
	if (ctrl_chr != BLK_START_AFIOBALLS) {
		wrong_marker(BLK_START_AFIOBALLS, ctrl_chr);
	}
	paranoid_free(curr_xattr_list_fname);
	paranoid_free(curr_acl_list_fname);

	for (res = read_header_block_from_stream(&size, fname, &ctrl_chr);
		 ctrl_chr != BLK_STOP_AFIOBALLS;
		 res = read_header_block_from_stream(&size, fname, &ctrl_chr)) {
		asprintf(&curr_xattr_list_fname, XATTR_LIST_FNAME_RAW_SZ,
				 bkpinfo->tmpdir, current_afioball_number);
		asprintf(&curr_acl_list_fname, ACL_LIST_FNAME_RAW_SZ,
				 bkpinfo->tmpdir, current_afioball_number);
		if (ctrl_chr == BLK_START_EXTENDED_ATTRIBUTES) {
			iamhere("Reading EXAT files from tape");
			res =
				read_EXAT_files_from_tape(bkpinfo, &size, fname, &ctrl_chr,
										  curr_xattr_list_fname,
										  curr_acl_list_fname);
		}
		paranoid_free(curr_xattr_list_fname);
		paranoid_free(curr_acl_list_fname);

		if (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		asprintf(&tmp, "Verifying fileset #%ld", current_afioball_number);
		/*log_it(tmp); */
		update_progress_form(tmp);
		paranoid_free(tmp);

		res = verify_an_afioball_from_stream(bkpinfo, fname, size);
		if (res) {
			asprintf(&tmp, "Afioball %ld differs from live filesystem",
					 current_afioball_number);
			log_to_screen(tmp);
			paranoid_free(tmp);
		}
		retval += res;
		current_afioball_number++;
		g_current_progress++;
		res = read_header_block_from_stream(&size, fname, &ctrl_chr);
		if (ctrl_chr != BLK_STOP_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_STOP_AN_AFIO_OR_SLICE, ctrl_chr);
		}
	}
	log_msg(1, "All done with afioballs");
	close_progress_form();
	paranoid_free(fname);
	return (retval);
}


/**
 * Verify all biggiefiles on the opened CD/tape stream.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->restore_path
 * - @c bkpinfo->tmpdir
 *
 * @return 0 for success (even if there are differences); nonzero for a tape error.
 */
int verify_biggiefiles_from_stream(struct s_bkpinfo *bkpinfo)
{

	/*@ int ************************************************************ */
	int retval = 0;
	int res = 0;
	int ctrl_chr = 0;

	/*@ long *********************************************************** */
	long noof_biggiefiles = 0;
	long current_biggiefile_number = 0;

	/*@ buffers ******************************************************** */
	char *orig_fname, *logical_fname;
	char *comment;
	char *curr_xattr_list_fname;
	char *curr_acl_list_fname;
	/*@ pointers ******************************************************* */
	char *p;

	/*@ long long size ************************************************* */
	long long size = 0;

	assert(bkpinfo != NULL);
	malloc_string(orig_fname);

	asprintf(&curr_xattr_list_fname, XATTR_BIGGLST_FNAME_RAW_SZ,
			 bkpinfo->tmpdir);
	asprintf(&curr_acl_list_fname, ACL_BIGGLST_FNAME_RAW_SZ,
			 bkpinfo->tmpdir);
	asprintf(&comment, "Verifying all bigfiles.");
	log_to_screen(comment);
	/*
	   asprintf(&tmp, "%s/biggielist.txt", bkpinfo->tmpdir);
	   noof_biggiefiles = count_lines_in_file (tmp); // pointless
	   paranoid_free(tmp);
	 */
	res = read_header_block_from_stream(&size, orig_fname, &ctrl_chr);
	if (ctrl_chr != BLK_START_BIGGIEFILES) {
		if (ctrl_chr == BLK_START_EXTENDED_ATTRIBUTES) {
			iamhere("Grabbing the EXAT biggiefiles");
			res =
				read_EXAT_files_from_tape(bkpinfo, &size, orig_fname,
										  &ctrl_chr, curr_xattr_list_fname,
										  curr_acl_list_fname);
		}
	}
	paranoid_free(curr_xattr_list_fname);
	paranoid_free(curr_acl_list_fname);

	if (ctrl_chr != BLK_START_BIGGIEFILES) {
		wrong_marker(BLK_START_BIGGIEFILES, ctrl_chr);
	}
	noof_biggiefiles = (long) size;
	log_msg(1, "noof_biggiefiles = %ld", noof_biggiefiles);
	open_progress_form("Verifying big files", comment,
					   "Please wait. This may take some time.", "",
					   noof_biggiefiles);
	paranoid_free(comment);

	for (res = read_header_block_from_stream(&size, orig_fname, &ctrl_chr);
		 ctrl_chr != BLK_STOP_BIGGIEFILES;
		 res = read_header_block_from_stream(&size, orig_fname, &ctrl_chr))
	{
		if (ctrl_chr != BLK_START_A_NORMBIGGIE
			&& ctrl_chr != BLK_START_A_PIHBIGGIE) {
			wrong_marker(BLK_START_A_NORMBIGGIE, ctrl_chr);
		}
		p = strrchr(orig_fname, '/');
		if (!p) {
			p = orig_fname;
		} else {
			p++;
		}
		asprintf(&comment, "Verifying bigfile #%ld (%ld K)",
				 current_biggiefile_number, (long) size >> 10);
		update_progress_form(comment);
		paranoid_free(comment);

		asprintf(&logical_fname, "%s/%s", bkpinfo->restore_path,
				 orig_fname);
		res =
			verify_a_biggiefile_from_stream(bkpinfo, logical_fname, size);
		paranoid_free(logical_fname);
		retval += res;
		current_biggiefile_number++;
		g_current_progress++;
	}
	close_progress_form();
	paranoid_free(orig_fname);
	return (retval);
}

/* @} - end of LLverifyGroup */


/**
 * Verify the CD indicated by @c g_current_media_number.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->isodir
 * - @c bkpinfo->prefix
 * - @c bkpinfo->manual_cd_tray
 * - @c bkpinfo->media_device
 * - @c bkpinfo->nfs_remote_dir
 * - @c bkpinfo->tmpdir
 * - @c bkpinfo->verify_data
 *
 * @return 0 for success (even if differences are found), nonzero for failure.
 * @ingroup verifyGroup
 */
int verify_cd_image(struct s_bkpinfo *bkpinfo)
{

	/*@ int ************************************************************ */
	int retval = 0;

	/*@ buffers ******************************************************** */
	char *mountpoint;
	char *command;
	char *tmp;
	char *fname;
#ifdef __FreeBSD__
	char mdd[32];
	char *mddevice = mdd;
	int ret = 0;
	int vndev = 2;
#else
//skip
#endif

	assert(bkpinfo != NULL);

	asprintf(&mountpoint, "%s/cdrom", bkpinfo->tmpdir);
	asprintf(&fname, "%s/%s/%s-%d.iso", bkpinfo->isodir, bkpinfo->prefix,
			 bkpinfo->nfs_remote_dir, g_current_media_number);

	mkdir(mountpoint, 1777);
	sync();
	if (!does_file_exist(fname)) {
		asprintf(&tmp,
				 "%s not found; assuming you backed up to CD; verifying CD...",
				 fname);
		log_msg(2, tmp);
		paranoid_free(tmp);

		if (bkpinfo->manual_cd_tray) {
			popup_and_OK("Please push CD tray closed.");
		}
		if (find_and_mount_actual_cd(bkpinfo, mountpoint)) {
			log_to_screen("failed to mount actual CD");
			return (1);
		}
	} else {
		asprintf(&tmp, "%s found; verifying ISO...", fname);
		log_to_screen(tmp);
		paranoid_free(tmp);
#ifdef __FreeBSD__
		ret = 0;
		vndev = 2;
		mddevice = make_vn(fname);
		if (ret) {
			asprintf(&tmp, "make_vn of %s failed; unable to verify ISO\n",
					 fname);
			log_to_screen(tmp);
			paranoid_free(tmp);
			return (1);
		}
		asprintf(&command, "mount_cd9660 %s %s", mddevice, mountpoint);
#else
		asprintf(&command, "mount -o loop,ro -t iso9660 %s %s", fname,
				 mountpoint);
#endif
		if (run_program_and_log_output(command, FALSE)) {
			asprintf(&tmp, "%s failed; unable to mount ISO image\n",
					 command);
			log_to_screen(tmp);
			paranoid_free(tmp);
			return (1);
		}
		paranoid_free(command);
	}
	log_msg(2, "OK, I've mounted the ISO/CD\n");
	asprintf(&tmp, "%s/archives/NOT-THE-LAST", mountpoint);
	if (!does_file_exist(tmp)) {
		log_msg
			(2,
			 "This is the last CD. I am therefore setting bkpinfo->verify_data to FALSE.");
		bkpinfo->verify_data = FALSE;
/*
   (a) It's an easy way to tell the calling subroutine that we've finished &
   there are no more CD's to be verified; (b) It stops the post-backup verifier
   from running after the per-CD verifier has run too.
*/
	}
	paranoid_free(tmp);

	verify_afioballs_on_CD(bkpinfo, mountpoint);
	iamhere("before verify_all_slices");
	verify_all_slices_on_CD(bkpinfo, mountpoint);

#ifdef __FreeBSD__
	ret = 0;
	asprintf(&command, "umount %s", mountpoint);
	ret += system(command);

	ret += kick_vn(mddevice);
	if (ret)
#else
	asprintf(&command, "umount %s", mountpoint);

	if (system(command))
#endif
	{
		asprintf(&tmp, "%s failed; unable to unmount ISO image\n",
				 command);
		log_to_screen(tmp);
		paranoid_free(tmp);
		retval++;
	} else {
		log_msg(2, "OK, I've unmounted the ISO file\n");
	}
	paranoid_free(command);
	paranoid_free(mountpoint);

	if (!does_file_exist(fname)) {
		asprintf(&command, "umount %s", bkpinfo->media_device);
		run_program_and_log_output(command, 2);
		paranoid_free(command);

		if (!bkpinfo->please_dont_eject
			&& eject_device(bkpinfo->media_device)) {
			log_msg(2, "Failed to eject CD-ROM drive");
		}
	}
	paranoid_free(fname);
	return (retval);
}


/**
 * Verify all backups on tape.
 * This should be done after the backup process has already closed the tape.
 * @param bkpinfo The backup information structure. Passed to various helper functions.
 * @return 0 for success (even if thee were differences), nonzero for failure.
 * @ingroup verifyGroup
 */
int verify_tape_backups(struct s_bkpinfo *bkpinfo)
{

	/*@ int ************************************************************ */
	int retval = 0;

	/*@ buffers ******************************************************** */
	char *tmp;
	char *changed_files_fname;

	/*@ long *********************************************************** */
	long diffs = 0;

	assert(bkpinfo != NULL);

	log_msg(3, "verify_tape_backups --- starting");
	log_to_screen("Verifying backups");
	openin_tape(bkpinfo);

	/* verify archives themselves */
	retval += verify_afioballs_from_stream(bkpinfo);
	retval += verify_biggiefiles_from_stream(bkpinfo);

	/* find the final blocks */
	paranoid_system("sync");
	sleep(2);
	closein_tape(bkpinfo);

	/* close tape; exit */
	//  fclose(g_tape_stream); <-- not needed; is handled by closein_tape()
	paranoid_system
		("rm -f /tmp/biggies.changed /tmp/changed.files.[0-9]* 2> /dev/null");
	asprintf(&changed_files_fname, "/tmp/changed.files.%d",
			 (int) (random() % 32767));
	asprintf(&tmp,
			 "cat %s | grep -x \"%s:.*\" | cut -d'\"' -f2 | sort -u | awk '{print \"/\"$0;};' | tr -s '/' '/' | grep -v \"(total of\" | grep -v \"incheckentry.*xwait\" | grep -vx \"/afio:.*\" | grep -vx \"dev/.*\"  > %s",
			 MONDO_LOGFILE, (bkpinfo->use_star) ? "star" : "afio",
			 changed_files_fname);
	log_msg(2, "Running command to derive list of changed files");
	log_msg(2, tmp);
	if (system(tmp)) {
		if (does_file_exist(changed_files_fname)
			&& length_of_file(changed_files_fname) > 2) {
			log_to_screen
				("Warning - unable to check logfile to derive list of changed files");
		} else {
			log_to_screen
				("No differences found. Therefore, no 'changed.files' text file.");
		}
	}
	paranoid_free(tmp);

	asprintf(&tmp, "cat /tmp/biggies.changed >> %s", changed_files_fname);
	paranoid_system(tmp);
	paranoid_free(tmp);

	diffs = count_lines_in_file(changed_files_fname);
	if (diffs > 0) {
		asprintf(&tmp, "cp -f %s %s", changed_files_fname,
				 "/tmp/changed.files");
		run_program_and_log_output(tmp, FALSE);
		paranoid_free(tmp);

		asprintf(&tmp,
				 "%ld files differed from live filesystem; type less %s or less %s to see",
				 diffs, changed_files_fname, "/tmp/changed.files");
		log_msg(0, tmp);
		paranoid_free(tmp);

		log_to_screen
			("See /tmp/changed.files for a list of nonmatching files.");
		log_to_screen
			("The files probably changed on filesystem, not on backup media.");
		//      retval++;
	}
	paranoid_free(changed_files_fname);
	return (retval);
}


/**
 * Generate the filename of a tarball to verify.
 * @param bkpinfo The backup information structure. @c bkpinfo->zip_suffix is the only field used.
 * @param mountpoint The directory where the CD/DVD/ISO is mounted.
 * @param setno The afioball number to get the location of.
 * @return The absolute path to the afioball.
 * @note The returned string points to static data that will be overwritten with each call.
 * @ingroup stringGroup
 */
char *vfy_tball_fname(struct s_bkpinfo *bkpinfo, char *mountpoint,
					  int setno)
{
	/*@ buffers ******************************************************* */
	static char *output;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(mountpoint);
	asprintf(&output, "%s/archives/%d.star.%s", mountpoint, setno,
			 bkpinfo->zip_suffix);
	if (!does_file_exist(output)) {
		paranoid_free(output);
		asprintf(&output, "%s/archives/%d.afio.%s", mountpoint, setno,
				 bkpinfo->zip_suffix);
	}
	return (output);
}
