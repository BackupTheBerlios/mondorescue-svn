/*  $Id$
 * file manipulation
*/

/**
 * @file
 * Functions to manipulate files.
 */


#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-files.h"

#include "lib-common-externs.h"

#include "libmondo-tools-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-devices-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-string-EXT.h"

/*@unused@*/
//static char cvsid[] = "$Id$";

extern char err_log_lines[NOOF_ERR_LINES][MAX_STR_LEN];

extern int g_currentY;
extern char *g_mondo_home;

/**
 * @addtogroup fileGroup
 * @{
 */
/**
 * Get an md5 checksum of the specified file.
 * @param filename The file to checksum.
 * @return The 32-character ASCII representation of the 128-bit checksum.
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *calc_checksum_of_file(char *filename)
{
	/*@ buffers ***************************************************** */
	static char *output = NULL;
	char *command;
	char *tmp;
	size_t n = 0;

	/*@ pointers **************************************************** */
	char *p;
	FILE *fin;

	/*@ initialize pointers ***************************************** */

	p = output;

	/*@************************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(filename);

	if (does_file_exist(filename)) {
		asprintf(&command, "md5sum \"%s\"", filename);
		fin = popen(command, "r");
		paranoid_free(command);

		if (fin) {
			(void) getline(&output, &n, fin);
			p = strchr(output, ' ');
			paranoid_pclose(fin);
		}
	} else {
		asprintf(&tmp, "File '%s' not found; cannot calc checksum",
				filename);
		log_it(tmp);
		paranoid_free(tmp);
	}
	if (p) {
		*p = '\0';
	}
	return (output);
}


/**
 * Get a not-quite-unique representation of some of the file's @c stat properties.
 * The returned string has the form <tt>size-mtime-ctime</tt>.
 * @param curr_fname The file to generate the "checksum" for.
 * @return The "checksum".
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *calc_file_ugly_minichecksum(char *curr_fname)
{

	/*@ buffers ***************************************************** */
	char *curr_cksum;

	/*@ pointers **************************************************** */

	/*@ structures ************************************************** */
	struct stat buf;

	/*@************************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(curr_fname);
	if (lstat(curr_fname, &buf)) {
		asprintf(&curr_cksum, "");
	} else {
		asprintf(&curr_cksum, "%ld-%ld-%ld", (long) (buf.st_size),
			(long) (buf.st_mtime), (long) (buf.st_ctime));
	}
	return (curr_cksum);
}


/**
 * Get the number of lines in @p filename.
 * @param filename The file to count lines in.
 * @return The number of lines in @p filename.
 * @bug This function uses the shell and "wc -l"; it should probably be rewritten in C.
 */
long count_lines_in_file(char *filename)
{

	/*@ buffers ***************************************************** */
	char *command;
	char *incoming = NULL;
	char *tmp;

	/*@ long ******************************************************** */
	long noof_lines = -1L;

	/*@ int ******************************************************** */
	size_t n = 0;

	/*@ pointers **************************************************** */
	FILE *fin;

	assert_string_is_neither_NULL_nor_zerolength(filename);
	if (!does_file_exist(filename)) {
		asprintf(&tmp,
				"%s does not exist, so I cannot found the number of lines in it",
				filename);
		log_it(tmp);
		paranoid_free(tmp);
		return (0);
	}
	asprintf(&command, "cat %s | wc -l", filename);
	if (!does_file_exist(filename)) {
		return (-1);
	}
	fin = popen(command, "r");
	paranoid_free(command);

	if (fin) {
		if (feof(fin)) {
			noof_lines = 0;
		} else {
			(void) getline(&incoming, &n, fin);
			while (strlen(incoming) > 0
				   && incoming[strlen(incoming) - 1] < 32) {
				incoming[strlen(incoming) - 1] = '\0';
			}
			noof_lines = atol(incoming);
			paranoid_free(incoming);
		}
		paranoid_pclose(fin);
	}
	return (noof_lines);
}


/**
 * Check for existence of given @p filename.
 * @param filename The file to check for.
 * @return TRUE if it exists, FALSE otherwise.
 */
bool does_file_exist(char *filename)
{

	/*@ structures ************************************************** */
	struct stat buf;

	/*@************************************************************** */

	assert(filename != NULL);

	if (lstat(filename, &buf)) {
		log_msg(20, "%s does not exist", filename);
		return (FALSE);
	} else {
		log_msg(20, "%s exists", filename);
		return (TRUE);
	}
}


/**
 * Modify @p inout (a file containing a list of files) to only contain files
 * that exist.
 * @param inout The filelist to operate on.
 * @note The original file is renamed beforehand, so it will not be accessible
 * while the modification is in progress.
 */
void exclude_nonexistent_files(char *inout)
{
	char *infname;
	char *outfname;
	char *tmp;
	char *incoming = NULL;

	/*@ int ********************************************************* */
	int i;
	size_t n = 0;

	/*@ pointers **************************************************** */
	FILE *fin, *fout;


	/*@ end vars *********************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(inout);

	asprintf(&infname, "%s.in", inout);

	asprintf(&tmp, "cp -f %s %s", inout, infname);
	run_program_and_log_output(tmp, FALSE);
	paranoid_free(tmp);

	if (!(fin = fopen(infname, "r"))) {
		log_OS_error("Unable to openin infname");
		paranoid_free(infname);
		return;
	}

	asprintf(&outfname, "%s", inout);
	if (!(fout = fopen(outfname, "w"))) {
		log_OS_error("Unable to openout outfname");
		paranoid_free(outfname);
		return;
	}
	paranoid_free(outfname);

	for (getline(&incoming, &n, fin); !feof(fin);
		 getline(&incoming, &n, fin)) {
		i = strlen(incoming) - 1;
		if (i >= 0 && incoming[i] < 32) {
			incoming[i] = '\0';
		}
		if (does_file_exist(incoming)) {
			fprintf(fout, "%s\n", incoming);
		} else {
			asprintf(&tmp, "Excluding '%s'-nonexistent\n", incoming);
			log_it(tmp);
			paranoid_free(tmp);
		}
	}
	paranoid_free(incoming);
	paranoid_fclose(fout);
	paranoid_fclose(fin);
	unlink(infname);
	paranoid_free(infname);
}


/**
 * Attempt to find the user's kernel by calling Mindi.
 * If Mindi can't find the kernel, ask user. If @p kernel is not empty,
 * don't do anything.
 * @param kernel Where to put the found kernel.
 * @return 0 for success, 1 for failure.
 */
int figure_out_kernel_path_interactively_if_necessary(char *kernel)
{
	char *tmp;

	if (!kernel[0]) {
		strcpy(kernel,
			   call_program_and_get_last_line_of_output
			   ("mindi --findkernel 2> /dev/null"));
	}
	log_it("Calling Mindi with kernel path of '%s'", kernel);
	while (!kernel[0]) {
		if (!ask_me_yes_or_no
			("Kernel not found or invalid. Choose another?")) {
			return (1);
		}
		if (!popup_and_get_string
			("Kernel path",
			 "What is the full path and filename of your kernel, please?",
			 kernel, MAX_STR_LEN / 4)) {
			fatal_error
				("Kernel not found. Please specify with the '-k' flag.");
		}
		asprintf(&tmp, "User says kernel is at %s", kernel);
		log_it(tmp);
		paranoid_free(tmp);
	}
	return (0);
}


/**
 * Find location of specified executable in user's PATH.
 * @param fname The basename of the executable to search for (e.g. @c afio).
 * @return The full path to the executable, or "" if it does not exist, or NULL if @c file could not be found.
 * @note The returned string points to static storage that will be overwritten with each call.
 * @bug The checks with @c file and @c dirname seem pointless. If @c incoming is "", then you're calling
 * <tt>dirname 2\>/dev/null</tt> or <tt>file 2\>/dev/null | cut -d':' -f1 2\>/dev/null</tt>, which basically amounts
 * to nothing.
 */
char *find_home_of_exe(char *fname)
{
	/*@ buffers ********************* */
	static char output[MAX_STR_LEN];
	char *incoming;
	char *command;

	/*@******************************* */

	assert_string_is_neither_NULL_nor_zerolength(fname);
	asprintf(&command, "which %s 2> /dev/null", fname);
	asprintf(&incoming, call_program_and_get_last_line_of_output(command));
	paranoid_free(command);

	if (incoming[0] == '\0') {
		if (system("which file > /dev/null 2> /dev/null")) {
			paranoid_free(incoming);
			return (NULL);		// forget it :)
		}
		asprintf(&command,
				"file %s 2> /dev/null | cut -d':' -f1 2> /dev/null",
				incoming);
		paranoid_free(incoming);

		asprintf(&incoming,
			   call_program_and_get_last_line_of_output(command));
		paranoid_free(command);
	}
	if (incoming[0] == '\0')	// yes, it is == '\0' twice, not once :)
	{
		asprintf(&command, "dirname %s 2> /dev/null", incoming);
		paranoid_free(incoming);

		asprintf(&incoming,
			   call_program_and_get_last_line_of_output(command));
		paranoid_free(command);
	}
	strcpy(output, incoming);
	paranoid_free(incoming);

	if (output[0] != '\0' && does_file_exist(output)) {
		log_msg(4, "find_home_of_exe () --- Found %s at %s", fname,
				output);
	} else {
		output[0] = '\0';
		log_msg(4, "find_home_of_exe() --- Could not find %s", fname);
	}
	if (!output[0]) {
		return (NULL);
	} else {
		return (output);
	}
}


/**
 * Get the last sequence of digits surrounded by non-digits in the first 32k of
 * a file.
 * @param logfile The file to look in.
 * @return The number found, or 0 if none.
 */
int get_trackno_from_logfile(char *logfile)
{

	/*@ pointers ********************************************************* */
	FILE *fin;

	/*@ int ************************************************************** */
	int trackno = 0;
	size_t len = 0;

	/*@ buffer ************************************************************ */
	char datablock[32701];

	assert_string_is_neither_NULL_nor_zerolength(logfile);

	if (!(fin = fopen(logfile, "r"))) {
		log_OS_error("Unable to open logfile");
		fatal_error("Unable to open logfile to read trackno");
	}
	len = fread(datablock, 1, 32700, fin);
	paranoid_fclose(fin);
	if (len <= 0) {
		return (0);
	}
	for (; len > 0 && !isdigit(datablock[len - 1]); len--);
	datablock[len--] = '\0';
	for (; len > 0 && isdigit(datablock[len - 1]); len--);
	trackno = atoi(datablock + len);
	return (trackno);
}


/**
 * Get a percentage from the last line of @p filename. We look for the string
 * "% done" on the last line and, if we find it, grab the number before the last % sign.
 * @param filename The file to get the percentage from.
 * @return The percentage found, or 0 for error.
 */
int grab_percentage_from_last_line_of_file(char *filename)
{

	/*@ buffers ***************************************************** */
	char *lastline;
	char *command;
	/*@ pointers **************************************************** */
	char *p;

	/*@ int's ******************************************************* */
	int i;

	for (i = NOOF_ERR_LINES - 1;
		 i >= 0 && !strstr(err_log_lines[i], "% Done")
		 && !strstr(err_log_lines[i], "% done"); i--);
	if (i < 0) {
		asprintf(&command,
				"tail -n3 %s | fgrep -i \"%c\" | tail -n1 | awk '{print $0;}'",
				filename, '%');
		asprintf(&lastline,
			   call_program_and_get_last_line_of_output(command));
		paranoid_free(command);
		if (!lastline[0]) {
			paranoid_free(lastline);
			return (0);
		}
	} else {
		asprintf(&lastline, err_log_lines[i]);
	}

	p = strrchr(lastline, '%');
	if (p) {
		*p = '\0';
	}
	if (!p) {
		paranoid_free(lastline);
		return (0);
	}
	*p = '\0';
	for (p--; *p != ' ' && p != lastline; p--);
	if (p != lastline) {
		p++;
	}
	i = atoi(p);
	paranoid_free(lastline);

	return (i);
}


/**
 * Return the last line of @p filename.
 * @param filename The file to get the last line of.
 * @return The last line of the file.
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *last_line_of_file(char *filename)
{
	/*@ buffers ***************************************************** */
	static char output[MAX_STR_LEN];
	static char *command;
	static char *tmp;

	/*@ pointers **************************************************** */
	FILE *fin;

	/*@ end vars **************************************************** */

	if (!does_file_exist(filename)) {
		asprintf(&tmp, "Tring to get last line of nonexistent file (%s)",
				filename);
		log_it(tmp);
		paranoid_free(tmp);

		output[0] = '\0';
		return (output);
	}
	asprintf(&command, "tail -n1 %s", filename);
	fin = popen(command, "r");
	paranoid_free(command);

	(void) fgets(output, MAX_STR_LEN, fin);
	paranoid_pclose(fin);
	while (strlen(output) > 0 && output[strlen(output) - 1] < 32) {
		output[strlen(output) - 1] = '\0';
	}
	return (output);
}


/**
 * Get the length of @p filename in bytes.
 * @param filename The file to get the length of.
 * @return The length of the file, or -1 for error.
 */
long long length_of_file(char *filename)
{
	/*@ pointers *************************************************** */
	FILE *fin;

	/*@ long long ************************************************* */
	long long length;

	fin = fopen(filename, "r");
	if (!fin) {
		log_it("filename=%s", filename);
		log_OS_error("Unable to openin filename");
		return (-1);
	}
	fseek(fin, 0, SEEK_END);
	length = ftell(fin);
	paranoid_fclose(fin);
	return (length);
}


/**
 * Create the directory @p outdir_fname and all parent directories. Equivalent to <tt>mkdir -p</tt>.
 * @param outdir_fname The directory to create.
 * @return The return value of @c mkdir.
 */
/* BERLIOS: This function shouldn't call system at all */
int make_hole_for_dir(char *outdir_fname)
{
	char *tmp;
	int res = 0;

	assert_string_is_neither_NULL_nor_zerolength(outdir_fname);
	asprintf(&tmp, "mkdir -p %s", outdir_fname);
	res = system(tmp);
	paranoid_free(tmp);
	return (res);
}


/**
 * Create the parent directories of @p outfile_fname.
 * @param outfile_fname The file to make a "hole" for.
 * @return 0, always.
 * @bug Return value unnecessary.
 */
/* BERLIOS: This function shouldn't call system at all */
int make_hole_for_file(char *outfile_fname)
{
	/*@ buffer ****************************************************** */
	char *command;

	/*@ int  ******************************************************** */
	int res = 0;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(outfile_fname);
	assert(!strstr(outfile_fname, MNT_CDROM));
	assert(!strstr(outfile_fname, "/dev/cdrom"));

	asprintf(&command, "mkdir -p \"%s\" 2> /dev/null", outfile_fname);
	res += system(command);
	paranoid_free(command);

	asprintf(&command, "rmdir \"%s\" 2> /dev/null", outfile_fname);
	res += system(command);
	paranoid_free(command);

	asprintf(&command, "rm -f \"%s\" 2> /dev/null", outfile_fname);
	res += system(command);
	paranoid_free(command);
	unlink(outfile_fname);
	return (0);
}


/**
 * Get the number of lines in @p filelist_fname that contain the string @p wildcard.
 * @param filelist_fname The file to search through.
 * @param wildcard The string to search for. This is @e not a shell glob or a regular expression.
 * @return The number of lines matched.
 */
long noof_lines_that_match_wildcard(char *filelist_fname, char *wildcard)
{
	/*@ long ******************************************************* */
	long matches = 0;

	/*@ pointers *************************************************** */
	FILE *fin;

	/*@ buffers **************************************************** */
	char *incoming = NULL;

	size_t n = 0;
	/*@ end vars *************************************************** */


	fin = fopen(filelist_fname, "r");

	if (!fin) {
		log_OS_error("Unable to openin filelist_fname");
		return (0);
	}
	(void) getline(&incoming, &n, fin);
	while (!feof(fin)) {
		if (strstr(incoming, wildcard)) {
			matches++;
		}
		(void) getline(&incoming, &n, fin);
	}
	paranoid_fclose(fin);
	paranoid_free(incoming);
	return (matches);
}


/**
 * Register our PID in a file in /var/run.
 * The PID will be put in /var/run/monitas-<tt>name_str</tt>.pid.
 * @param pid 0 to remove file, anything else to create it.
 * @param name_str The basename of the PID file (e.g. "mondo" or "server")
 * @note This function does not provide support against multiple instances, unless you check for that yourself.
 */
void register_pid(pid_t pid, char *name_str)
{
	char *tmp;
    char *lockfile_fname;
	int res;
	size_t n = 0;
	FILE *fin;

	asprintf(&lockfile_fname, "/var/run/monitas-%s.pid", name_str);
	if (!pid) {
		log_it("Unregistering PID");
		if (unlink(lockfile_fname)) {
			log_it("Error unregistering PID");
		}
		paranoid_free(lockfile_fname);
		return;
	}
	if (does_file_exist(lockfile_fname)) {
		if ((fin = fopen(lockfile_fname, "r"))) {
			(void) getline(&tmp, &n, fin);
			paranoid_fclose(fin);
		} else {
			log_OS_error("Unable to openin lockfile_fname");
		}
		pid = (pid_t) atol(tmp);
		paranoid_free(tmp);

		asprintf(&tmp, "ps %ld > /dev/null 2> /dev/null", (long int) pid);
		res = system(tmp);
		paranoid_free(tmp);
		if (!res) {
			log_it
				("I believe the daemon is already running. If it isn't, please delete %s and try again.",
				 lockfile_fname);
		}
	}
	asprintf(&tmp, "echo %ld > %s 2> /dev/null", (long int) getpid(),
			lockfile_fname);
	paranoid_free(lockfile_fname);

	if (system(tmp)) {
		fatal_error("Cannot register PID");
	}
	paranoid_free(tmp);
	return;
}


/**
 * Determine the size (in KB) of @p dev in the mountlist in <tt>tmpdir</tt>/mountlist.txt.
 * @param tmpdir The tempdir where the mountlist is stored.
 * @param dev The device to search for.
 * @return The size of the partition in KB.
 */
long size_of_partition_in_mountlist_K(char *tmpdir, char *dev)
{
	char *command;
	char *sz_res;
	long file_len_K;

	asprintf(&command,
			"grep '%s ' %s/mountlist.txt | head -n1 | awk '{print $4;}'",
			dev, tmpdir);
	log_it(command);
	asprintf(&sz_res, call_program_and_get_last_line_of_output(command));
	file_len_K = atol(sz_res);
	log_msg(4, "%s --> %s --> %ld", command, sz_res, file_len_K);
	paranoid_free(command);
	paranoid_free(sz_res);
	return (file_len_K);
}


/**
 * Calculate the total size (in KB) of all the biggiefiles in this backup.
 * @param bkpinfo The backup information structure. Only the @c bkpinfo->tmpdir field is used.
 * @return The total size of all biggiefiles in KB.
 */
long size_of_all_biggiefiles_K(struct s_bkpinfo *bkpinfo)
{
	/*@ buffers ***************************************************** */
	char *fname;
	char *biggielist;
	char *comment;

	/*@ long ******************************************************** */
	long scratchL = 0;
	long file_len_K;

	/*@ pointers *************************************************** */
	FILE *fin = NULL;
	size_t n = 0;

	/*@ end vars *************************************************** */

	log_it("Calculating size of all biggiefiles (in total)");
	asprintf(&biggielist, "%s/biggielist.txt", bkpinfo->tmpdir);
	log_it("biggielist = %s", biggielist);
	if (!(fin = fopen(biggielist, "r"))) {
		log_OS_error
			("Cannot open biggielist. OK, so estimate is based on filesets only.");
	} else {
		log_msg(4, "Reading it...");
		for (getline(&fname, &n, fin); !feof(fin);
			 getline(&fname, &n, fin)) {
			if (fname[strlen(fname) - 1] <= 32) {
				fname[strlen(fname) - 1] = '\0';
			}
			if (0 == strncmp(fname, "/dev/", 5)) {
				file_len_K = get_phys_size_of_drive(fname) * 1024L;
			} else {
				file_len_K = (long) (length_of_file(fname) / 1024);
			}
			if (file_len_K > 0) {
				scratchL += file_len_K;
				log_msg(4, "%s --> %ld K", fname, file_len_K);
			}
			asprintf(&comment,
					"After adding %s, scratchL+%ld now equals %ld", fname,
					file_len_K, scratchL);
			log_msg(4, comment);
			paranoid_free(comment);

			if (feof(fin)) {
				break;
			}
		}
		paranoid_free(fname);
	}
	paranoid_free(biggielist);

	log_it("Closing...");
	paranoid_fclose(fin);
	log_it("Finished calculating total size of all biggiefiles");
	return (scratchL);
}


/**
 * Determine the amount of space (in KB) occupied by a mounted CD.
 * This can also be used to find the space used for other directories.
 * @param mountpt The mountpoint/directory to check.
 * @return The amount of space occupied in KB.
 */
long long space_occupied_by_cd(char *mountpt)
{
	/*@ buffer ****************************************************** */
	char *tmp = NULL;
	char *command;
	long long llres;
	size_t n = 0;
	/*@ pointers **************************************************** */
	char *p;
	FILE *fin;

	/*@ end vars *************************************************** */

	asprintf(&command, "du -sk %s", mountpt);
	fin = popen(command, "r");
	paranoid_free(command);

	(void) getline(&tmp, &n, fin);
	paranoid_pclose(fin);
	p = strchr(tmp, '\t');
	if (p) {
		*p = '\0';
	}
	for (p = tmp, llres = 0; *p != '\0'; p++) {
		llres *= 10;
		llres += (int) (*p - '0');
	}
	paranoid_free(tmp);
	return (llres);
}


/**
 * Update a CRC checksum to include another character.
 * @param crc The original CRC checksum.
 * @param c The character to add.
 * @return The new CRC checksum.
 * @ingroup utilityGroup
 */
unsigned int updcrc(unsigned int crc, unsigned int c)
{
	unsigned int tmp;
	tmp = (crc >> 8) ^ c;
	crc = (crc << 8) ^ crctttab[tmp & 255];
	return crc;
}


/**
 * Update a reverse CRC checksum to include another character.
 * @param crc The original CRC checksum.
 * @param c The character to add.
 * @return The new CRC checksum.
 * @ingroup utilityGroup
 */
unsigned int updcrcr(unsigned int crc, unsigned int c)
{
	unsigned int tmp;
	tmp = crc ^ c;
	crc = (crc >> 8) ^ crc16tab[tmp & 0xff];
	return crc;
}


/**
 * Check for an executable on the user's system; write a message to the
 * screen and the log if we can't find it.
 * @param fname The executable basename to look for.
 * @return 0 if it's found, nonzero if not.
 */
int whine_if_not_found(char *fname)
{
	/*@ buffers *** */
	char *command;
	char *errorstr;
	int res = 0;


	asprintf(&command, "which %s > /dev/null 2> /dev/null", fname);
	res = system(command);
	paranoid_free(command);

	if (res) {
		asprintf(&errorstr,
			"Please install '%s'. I cannot find it on your system.",
			fname);
		log_to_screen(errorstr);
		paranoid_free(errorstr);
		log_to_screen
			("There may be an hyperlink at http://www.mondorescue.org which");
		log_to_screen("will take you to the relevant (missing) package.");
		return (1);
	} else {
		return (0);
	}
}


/**
 * Create a data file at @p fname containing @p contents.
 * The data actually can be multiple lines, despite the name.
 * @param fname The file to create.
 * @param contents The data to put in it.
 * @return 0 for success, 1 for failure.
 */
int write_one_liner_data_file(char *fname, char *contents)
{
	/*@ pointers *************************************************** */
	FILE *fout;
	int res = 0;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(fname);
	if (!contents) {
		log_it("%d: Warning - writing NULL to %s", __LINE__, fname);
	}
	if (!(fout = fopen(fname, "w"))) {
		log_it("fname=%s");
		log_OS_error("Unable to openout fname");
		return (1);
	}
	fprintf(fout, "%s\n", contents);
	paranoid_fclose(fout);
	return (res);
}


/**
 * Read @p fname into @p contents.
 * @param fname The file to read.
 * @param contents Where to put its contents.
 * @return 0 for success, nonzero for failure.
 */
int read_one_liner_data_file(char *fname, char *contents)
{
	/*@ pointers *************************************************** */
	FILE *fin;
	int res = 0;
	int i;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(fname);
	if (!contents) {
		log_it("%d: Warning - reading NULL from %s", __LINE__, fname);
	}
	if (!(fin = fopen(fname, "r"))) {
		log_it("fname=%s", fname);
		log_OS_error("Unable to openin fname");
		return (1);
	}
	fscanf(fin, "%s\n", contents);
	i = strlen(contents);
	if (i > 0 && contents[i - 1] < 32) {
		contents[i - 1] = '\0';
	}
	paranoid_fclose(fin);
	return (res);
}


/**
 * Copy the files that Mondo/Mindi need to run to the scratchdir or tempdir.
 * Currently this includes: copy Mondo's home directory to scratchdir, untar "mondo_home/payload.tgz"
 * if it exists, copy LAST-FILELIST-NUMBER to scratchdir, copy mondorestore
 * and post-nuke.tgz (if it exists) to tmpdir, and run "hostname > scratchdir/HOSTNAME".
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->postnuke_tarball
 * - @c bkpinfo->scratchdir
 * - @c bkpinfo->tmpdir
 */
void copy_mondo_and_mindi_stuff_to_scratchdir(struct s_bkpinfo *bkpinfo)
{
	/*@ Char buffers ** */
	char *command;
	char *tmp;
	char old_pwd[MAX_STR_LEN];

	mvaddstr_and_log_it(g_currentY, 0,
						"Copying Mondo's core files to the scratch directory");

	/* BERLIOS: Why do we need to do it here as well ? */
	log_msg(4, "g_mondo_home='%s'", g_mondo_home);
	if ((g_mondo_home == NULL) || strlen(g_mondo_home) < 2) {
		paranoid_free(g_mondo_home);
		g_mondo_home = find_and_store_mondoarchives_home();
	}
	asprintf(&command, CP_BIN " --parents -pRdf %s %s", g_mondo_home,
			bkpinfo->scratchdir);

	log_msg(4, "command = %s", command);
	if (run_program_and_log_output(command, 1)) {
		fatal_error("Failed to copy Mondo's stuff to scratchdir");
	}
	paranoid_free(command);

	asprintf(&tmp, "%s/payload.tgz", g_mondo_home);
	if (does_file_exist(tmp)) {
		log_it("Untarring payload %s to scratchdir %s", tmp,
			   bkpinfo->scratchdir);
		(void) getcwd(old_pwd, MAX_STR_LEN - 1);
		chdir(bkpinfo->scratchdir);
		asprintf(&command, "tar -zxvf %s", tmp);
		if (run_program_and_log_output(command, FALSE)) {
			fatal_error("Failed to untar payload");
		}
		paranoid_free(command);
		chdir(old_pwd);
	}
	paranoid_free(tmp);

	asprintf(&command, "cp -f %s/LAST-FILELIST-NUMBER %s", bkpinfo->tmpdir,
			bkpinfo->scratchdir);
	if (run_program_and_log_output(command, FALSE)) {
		fatal_error("Failed to copy LAST-FILELIST-NUMBER to scratchdir");
	}
	paranoid_free(command);

	asprintf(&tmp,
		   call_program_and_get_last_line_of_output("which mondorestore"));
	if (!tmp[0]) {
		fatal_error
			("'which mondorestore' returned null. Where's your mondorestore? `which` can't find it. That's odd. Did you install mondorestore?");
	}
	asprintf(&command, "cp -f %s %s", tmp, bkpinfo->tmpdir);
	paranoid_free(tmp);

	if (run_program_and_log_output(command, FALSE)) {
		fatal_error("Failed to copy mondorestore to tmpdir");
	}
	paranoid_free(command);

	asprintf(&command, "hostname > %s/HOSTNAME", bkpinfo->scratchdir);
	paranoid_system(command);
	paranoid_free(command);

	if (bkpinfo->postnuke_tarball[0]) {
		asprintf(&command, "cp -f %s %s/post-nuke.tgz",
				bkpinfo->postnuke_tarball, bkpinfo->tmpdir);
		if (run_program_and_log_output(command, FALSE)) {
			fatal_error("Unable to copy post-nuke tarball to tmpdir");
		}
		paranoid_free(command);
	}

	mvaddstr_and_log_it(g_currentY++, 74, "Done.");
}


/**
 * Store the client's NFS configuration in files to be restored at restore-time.
 * Assumes that @c bkpinfo->media_type = nfs, but does not check for this.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c nfs_mount
 * - @c nfs_remote_dir
 * - @c tmpdir
 */
void store_nfs_config(struct s_bkpinfo *bkpinfo)
{

	/*@ buffers ******** */
	char *outfile;
	char *nfs_dev;
	char *nfs_mount;
	char *nfs_client_ipaddr;
	char *nfs_client_netmask;
	char *nfs_client_defgw;
	char *nfs_server_ipaddr;
	char *tmp;
	char *command;

	/*@ pointers ***** */
	char *p;
	FILE *fout;



	log_it("Storing NFS configuration");
	asprintf(&tmp, bkpinfo->nfs_mount);
	p = strchr(tmp, ':');
	if (!p) {
		fatal_error
			("NFS mount doesn't have a colon in it, e.g. 192.168.1.4:/home/nfs");
	}
	*(p++) = '\0';
	asprintf(&nfs_server_ipaddr, tmp);
	paranoid_free(tmp);

	asprintf(&nfs_mount, p);
	asprintf(&command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\n' | head -n1 | cut -d' ' -f1");
	asprintf(&nfs_dev, call_program_and_get_last_line_of_output(command));
	paranoid_free(command);

	asprintf(&command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\\n' | head -n1 | tr -s '\t' ' ' | cut -d' ' -f7 | cut -d':' -f2");
	asprintf(&nfs_client_ipaddr,
		   call_program_and_get_last_line_of_output(command));
	paranoid_free(command);

	asprintf(&command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\\n' | head -n1 | tr -s '\t' ' ' | cut -d' ' -f9 | cut -d':' -f2");
	asprintf(&nfs_client_netmask,
		   call_program_and_get_last_line_of_output(command));
	paranoid_free(command);

	asprintf(&command,
			"route | egrep '^default' | awk '{printf $2}'");
	asprintf(&nfs_client_defgw,
		   call_program_and_get_last_line_of_output(command));
	paranoid_free(command);

	asprintf(&tmp,
			"nfs_client_ipaddr=%s; nfs_client_netmask=%s; nfs_server_ipaddr=%s; nfs_mount=%s; nfs_client_defgw=%s;  ",
			nfs_client_ipaddr, nfs_client_netmask, nfs_server_ipaddr, nfs_mount, nfs_client_defgw);
	paranoid_free(nfs_mount);
	log_it(tmp);
	paranoid_free(tmp);

	if (strlen(nfs_dev) < 2) {
		fatal_error
			("Unable to find ethN (eth0, eth1, ...) adapter via NFS mount you specified.");
	}
	asprintf(&outfile, "%s/start-nfs", bkpinfo->tmpdir);
	asprintf(&tmp, "outfile = %s", outfile);
	log_it(tmp);
	paranoid_free(tmp);

	if (!(fout = fopen(outfile, "w"))) {
		fatal_error("Cannot store NFS config");
	}
	fprintf(fout, "ifconfig lo 127.0.0.1  # config loopback\n");
	fprintf(fout, "ifconfig %s %s netmask %s	# config client\n", nfs_dev,
			nfs_client_ipaddr, nfs_client_netmask);
	fprintf(fout, "route add default gw %s  # default route\n", nfs_client_defgw);
	fprintf(fout, "ping -c 1 %s	# ping server\n", nfs_server_ipaddr);
	fprintf(fout, "mount -t nfs -o nolock %s /tmp/isodir\n",
			bkpinfo->nfs_mount);
	fprintf(fout, "exit 0\n");
	paranoid_fclose(fout);
	chmod(outfile, 0777);
	make_hole_for_dir("/var/cache/mondo-archive");

//  paranoid_system ("mkdir -p /var/cache/mondo-archive 2> /dev/null");

	asprintf(&tmp, "cp -f %s /var/cache/mondo-archive", outfile);
	paranoid_free(outfile);

	run_program_and_log_output(tmp, FALSE);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-DEV", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_dev);
	paranoid_free(nfs_dev);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-CLIENT-IPADDR", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_ipaddr);
	paranoid_free(nfs_client_ipaddr);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-CLIENT-NETMASK", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_netmask);
	paranoid_free(nfs_client_netmask);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-CLIENT-DEFGW", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_defgw);
	paranoid_free(nfs_client_defgw);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-SERVER-IPADDR", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_server_ipaddr);
	paranoid_free(nfs_server_ipaddr);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-SERVER-MOUNT", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, bkpinfo->nfs_mount);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/NFS-SERVER-PATH", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, bkpinfo->nfs_remote_dir);
	paranoid_free(tmp);

	asprintf(&tmp, "%s/ISO-PREFIX", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, bkpinfo->prefix);
	paranoid_free(tmp);

	log_it("Finished storing NFS configuration");
}


/**
 * Determine the approximate number of media that the backup will take up,
 * and tell the user. The uncompressed size is estimated as size_of_all_biggiefiles_K()
 * plus (noof_sets x bkpinfo->optimal_set_size). The compression factor is estimated as
 * 2/3 for LZO and 1/2 for bzip2. The data is not saved anywhere. If there are any
 * "imagedevs", the estimate is not shown as it will be wildly inaccurate.
 * If there are more than 50 media estimated, the estimate will not be shown.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->image_devs
 * - @c bkpinfo->media_size
 * - @c bkpinfo->optimal_set_size
 * - @c bkpinfo->use_lzo
 * @param noof_sets The number of filesets created.
 * @ingroup archiveGroup
 */
void
estimate_noof_media_required(struct s_bkpinfo *bkpinfo, long noof_sets)
{
	/*@ buffers *************** */
	char *tmp;

	/*@ long long ************* */
	long long scratchLL;

	if (bkpinfo->media_size[1] <= 0 || bkpinfo->backup_media_type == nfs) {
		log_to_screen("Number of media required: UNKNOWN");
		return;
	}

	log_it("Estimating number of media required...");
	scratchLL =
		(long long) (noof_sets) * (long long) (bkpinfo->optimal_set_size)
		+ (long long) (size_of_all_biggiefiles_K(bkpinfo));
	scratchLL = (scratchLL / 1024) / bkpinfo->media_size[1];
	scratchLL++;
	if (bkpinfo->use_lzo) {
		scratchLL = (scratchLL * 2) / 3;
	} else {
		scratchLL = scratchLL / 2;
	}
	if (!scratchLL) {
		scratchLL++;
	}
	if (scratchLL <= 1) {
		asprintf(&tmp,
				"Your backup will probably occupy a single %s. Maybe two.",
				media_descriptor_string(bkpinfo->backup_media_type));
	} else {
		asprintf(&tmp, "Your backup will occupy approximately %s media.",
				number_to_text((int) (scratchLL + 1)));
	}
	if (!bkpinfo->image_devs[0] && (scratchLL < 50)) {
		log_to_screen(tmp);
	}
	paranoid_free(tmp);
	return;
}


/**
 * Determine whether a file is compressed. This is done
 * by reading through the "do-not-compress-these" file distributed with Mondo.
 * @param filename The file to check.
 * @return TRUE if it's compressed, FALSE if not.
 */
bool is_this_file_compressed(char *filename)
{
	char *do_not_compress_these;
	char *tmp;
	char *p;

	asprintf(&tmp, "%s/do-not-compress-these", g_mondo_home);
	if (!does_file_exist(tmp)) {
		paranoid_free(tmp);
		return (FALSE);
	}
	paranoid_free(tmp);

	asprintf(&do_not_compress_these, last_line_of_file(tmp));
	for (p = do_not_compress_these; p != NULL; p++) {
		asprintf(&tmp, p);
		if (strchr(tmp, ' ')) {
			*(strchr(tmp, ' ')) = '\0';
		}
		if (!strcmp(strrchr(filename, '.'), tmp)) {
			paranoid_free(do_not_compress_these);
			paranoid_free(tmp);
			return (TRUE);
		}
		paranoid_free(tmp);

		if (!(p = strchr(p, ' '))) {
			break;
		}
	}
	paranoid_free(do_not_compress_these);
	return (FALSE);
}


int mode_of_file(char *fname)
{
	struct stat buf;

	if (lstat(fname, &buf)) {
		return (-1);
	}							// error
	else {
		return (buf.st_mode);
	}
}


/**
 * Create a small script that mounts /boot, calls @c grub-install, and syncs the disks.
 * @param outfile Where to put the script.
 * @return 0 for success, 1 for failure.
 */
int make_grub_install_scriptlet(char *outfile)
{
	FILE *fout;
	char *tmp;
	int retval = 0;

	if ((fout = fopen(outfile, "w"))) {
		fprintf(fout,
				"#!/bin/sh\n\nmount /boot > /dev/null 2> /dev/null\ngrub-install $@\nres=$?\nsync;sync;sync\nexit $res\n");
		paranoid_fclose(fout);
		log_msg(2, "Created %s", outfile);
		asprintf(&tmp, "chmod +x %s", outfile);
		paranoid_system(tmp);
		paranoid_free(tmp);

		retval = 0;
	} else {
		retval = 1;
	}
	return (retval);
}

/* @} - end fileGroup */

void paranoid_alloc(char *alloc, char *orig) 
{
		paranoid_free(alloc); 
		asprintf(&alloc, orig);
}

