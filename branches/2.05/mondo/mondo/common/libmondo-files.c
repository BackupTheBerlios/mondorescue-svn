/* libmondo-files.c                                  file manipulation
   $Id$
.


04/16/04
- find_home_of_exe() really does return NULL now if file not found

03/22/04
- added mode_of_file()

10/02/03
- cleaned up grab_percentage_from_last_line_of_file()

09/18
- added int make_grub_install_scriptlet()

09/16
- cleaned up mkisofs feedback window

09/12
- fixed Teuton-hostile bug in size_of_all_biggiefiles_K()

09/05
- added size_of_partition_in_mountlist_K()
- size_of_all_biggiefiles_K() now calls get_phys_size_of_drive(fname)

07/02
- fixed calls to popup_and_get_string()

05/19
- added CP_BIN

05/05
- added Joshua Oreman's FreeBSD patches

05/04
- find_home_of_exe() now returns NULL if file not found

04/26
- if >4 media est'd, say one meeeellion

04/25
- fixed minor bug in find_home_of_exe()

04/24
- added lots of assert()'s and log_OS_error()'s

04/07
- fix find_home_of_exe()
- cleaned up code a bit

03/27
- copy_mondo_and_mindi_stuff --- if _homedir_/payload.tgz exists then untar it to CD

01/14/2003
- if backup media type == nfs then don't estimate no. of media reqd

11/25/2002
- don't log/echo estimated # of media required if >=50

10/01 - 11/09
- chmod uses 0x, not decimal :)
- added is_this_file_compressed()
- replace convoluted grep with wc (KP)

09/01 - 09/30
- only show "number of media" estimate if no -x
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_

08/01 - 08/31
- handle unknown media sizes
- cleaned up some log_it() calls

07/24
- created
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
	static char output[MAX_STR_LEN];
	char command[MAX_STR_LEN * 2];
	char tmp[MAX_STR_LEN];

	/*@ pointers **************************************************** */
	char *p;
	FILE *fin;

	/*@ initialize pointers ***************************************** */

	p = output;

	/*@************************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(filename);
	if (does_file_exist(filename)) {
		sprintf(command, "md5sum \"%s\"", filename);
		fin = popen(command, "r");
		if (fin) {
			(void) fgets(output, MAX_STR_LEN, fin);
			p = strchr(output, ' ');
			paranoid_pclose(fin);
		}
	} else {
		sprintf(tmp, "File '%s' not found; cannot calc checksum",
				filename);
		log_it(tmp);
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
	static char curr_cksum[1000];

	/*@ pointers **************************************************** */

	/*@ structures ************************************************** */
	struct stat buf;

	/*@ initialize data *************************************************** */
	curr_cksum[0] = '\0';

	/*@************************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(curr_fname);
	if (lstat(curr_fname, &buf)) {
		return (curr_cksum);	// empty
	}

	sprintf(curr_cksum, "%ld-%ld-%ld", (long) (buf.st_size),
			(long) (buf.st_mtime), (long) (buf.st_ctime));
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
	char command[MAX_STR_LEN * 2];
	char incoming[MAX_STR_LEN];
	char tmp[MAX_STR_LEN];

	/*@ long ******************************************************** */
	long noof_lines = -1L;

	/*@ pointers **************************************************** */
	FILE *fin;

	/*@ initialize [0] to null ******************************************** */
	incoming[0] = '\0';

	assert_string_is_neither_NULL_nor_zerolength(filename);
	if (!does_file_exist(filename)) {
		sprintf(tmp,
				"%s does not exist, so I cannot found the number of lines in it",
				filename);
		log_it(tmp);
		return (0);
	}
	sprintf(command, "cat %s | wc -l", filename);
	if (!does_file_exist(filename)) {
		return (-1);
	}
	fin = popen(command, "r");
	if (fin) {
		if (feof(fin)) {
			noof_lines = 0;
		} else {
			(void) fgets(incoming, MAX_STR_LEN - 1, fin);
			while (strlen(incoming) > 0
				   && incoming[strlen(incoming) - 1] < 32) {
				incoming[strlen(incoming) - 1] = '\0';
			}
			noof_lines = atol(incoming);
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
	//  assert_string_is_neither_NULL_nor_zerolength(filename);
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
	char infname[MAX_STR_LEN];
	char outfname[MAX_STR_LEN];
	char tmp[MAX_STR_LEN];
	char incoming[MAX_STR_LEN];

	/*@ int ********************************************************* */
	int i;

	/*@ pointers **************************************************** */
	FILE *fin, *fout;


	/*@ end vars *********************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(inout);
	sprintf(infname, "%s.in", inout);
	sprintf(outfname, "%s", inout);
	sprintf(tmp, "cp -f %s %s", inout, infname);
	run_program_and_log_output(tmp, FALSE);
	if (!(fin = fopen(infname, "r"))) {
		log_OS_error("Unable to openin infname");
		return;
	}
	if (!(fout = fopen(outfname, "w"))) {
		log_OS_error("Unable to openout outfname");
		return;
	}
	for (fgets(incoming, MAX_STR_LEN, fin); !feof(fin);
		 fgets(incoming, MAX_STR_LEN, fin)) {
		i = strlen(incoming) - 1;
		if (i >= 0 && incoming[i] < 32) {
			incoming[i] = '\0';
		}
		if (does_file_exist(incoming)) {
			fprintf(fout, "%s\n", incoming);
		} else {
			sprintf(tmp, "Excluding '%s'-nonexistent\n", incoming);
			log_it(tmp);
		}
	}
	paranoid_fclose(fout);
	paranoid_fclose(fin);
	unlink(infname);
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
	char tmp[MAX_STR_LEN];

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
		sprintf(tmp, "User says kernel is at %s", kernel);
		log_it(tmp);
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

	malloc_string(incoming);
	malloc_string(command);
	incoming[0] = '\0';
	/*@******************************* */

	assert_string_is_neither_NULL_nor_zerolength(fname);
	sprintf(command, "which %s 2> /dev/null", fname);
	strcpy(incoming, call_program_and_get_last_line_of_output(command));
	if (incoming[0] == '\0') {
		if (system("which file > /dev/null 2> /dev/null")) {
			paranoid_free(incoming);
			paranoid_free(command);
			output[0] = '\0';
			return (NULL);		// forget it :)
		}
		sprintf(command,
				"file %s 2> /dev/null | cut -d':' -f1 2> /dev/null",
				incoming);
		strcpy(incoming,
			   call_program_and_get_last_line_of_output(command));
	}
	if (incoming[0] == '\0')	// yes, it is == '\0' twice, not once :)
	{
		sprintf(command, "dirname %s 2> /dev/null", incoming);
		strcpy(incoming,
			   call_program_and_get_last_line_of_output(command));
	}
	strcpy(output, incoming);
	if (output[0] != '\0' && does_file_exist(output)) {
		log_msg(4, "find_home_of_exe () --- Found %s at %s", fname,
				incoming);
	} else {
		output[0] = '\0';
		log_msg(4, "find_home_of_exe() --- Could not find %s", fname);
	}
	paranoid_free(incoming);
	paranoid_free(command);
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
	/*
	   sprintf(tmp,"datablock=%s; trackno=%d",datablock+len, trackno);
	   log_it(tmp);
	 */
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
	char tmp[MAX_STR_LEN];
	char lastline[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	/*@ pointers **************************************************** */
	char *p;

	/*@ int's ******************************************************* */
	int i;

	for (i = NOOF_ERR_LINES - 1;
		 i >= 0 && !strstr(err_log_lines[i], "% Done")
		 && !strstr(err_log_lines[i], "% done"); i--);
	if (i < 0) {
		sprintf(command,
				"tail -n3 %s | fgrep -i \"%c\" | tail -n1 | awk '{print $0;}'",
				filename, '%');
		strcpy(lastline,
			   call_program_and_get_last_line_of_output(command));
		if (!lastline[0]) {
			return (0);
		}
	} else {
		strcpy(lastline, err_log_lines[i]);
	}

	p = strrchr(lastline, '%');
	if (p) {
		*p = '\0';
	}
//  log_msg(2, "lastline='%s', ", p, lastline);
	if (!p) {
		return (0);
	}
	*p = '\0';
	for (p--; *p != ' ' && p != lastline; p--);
	if (p != lastline) {
		p++;
	}
	i = atoi(p);

	sprintf(tmp, "'%s' --> %d", p, i);
//     log_to_screen(tmp);

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
	static char command[MAX_STR_LEN * 2];
	static char tmp[MAX_STR_LEN];

	/*@ pointers **************************************************** */
	FILE *fin;

	/*@ end vars **************************************************** */

	if (!does_file_exist(filename)) {
		sprintf(tmp, "Tring to get last line of nonexistent file (%s)",
				filename);
		log_it(tmp);
		output[0] = '\0';
		return (output);
	}
	sprintf(command, "tail -n1 %s", filename);
	fin = popen(command, "r");
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
 * ?????
 * @bug I don't know what this function does. However, it seems orphaned, so it should probably be removed.
 */
int
make_checksum_list_file(char *filelist, char *cksumlist, char *comppath)
{
	/*@ pointers **************************************************** */
	FILE *fin;
	FILE *fout;

	/*@ int   ******************************************************* */
	int percentage;
	int i;
	int counter = 0;

	/*@ buffer ****************************************************** */
	char stub_fname[1000];
	char curr_fname[1000];
	char curr_cksum[1000];
	char tmp[1000];

	/*@ long [long] ************************************************* */
	long long filelist_length;
	long curr_pos;
	long start_time;
	long current_time;
	long time_taken;
	long time_remaining;

	/*@ end vars *************************************************** */

	start_time = get_time();
	filelist_length = length_of_file(filelist);
	sprintf(tmp, "filelist = %s; cksumlist = %s", filelist, cksumlist);
	log_it(tmp);
	fin = fopen(filelist, "r");
	if (fin == NULL) {
		log_OS_error("Unable to fopen-in filelist");
		log_to_screen("Can't open filelist");
		return (1);
	}
	fout = fopen(cksumlist, "w");
	if (fout == NULL) {
		log_OS_error("Unable to openout cksumlist");
		paranoid_fclose(fin);
		log_to_screen("Can't open checksum list");
		return (1);
	}
	for (fgets(stub_fname, 999, fin); !feof(fin);
		 fgets(stub_fname, 999, fin)) {
		if (stub_fname[(i = strlen(stub_fname) - 1)] < 32) {
			stub_fname[i] = '\0';
		}
		sprintf(tmp, "%s%s", comppath, stub_fname);
		strcpy(curr_fname, tmp + 1);
		strcpy(curr_cksum, calc_file_ugly_minichecksum(curr_fname));
		fprintf(fout, "%s\t%s\n", curr_fname, curr_cksum);
		if (counter++ > 12) {
			current_time = get_time();
			counter = 0;
			curr_fname[37] = '\0';
			curr_pos = ftell(fin) / 1024;
			percentage = (int) (curr_pos * 100 / filelist_length);
			time_taken = current_time - start_time;
			if (percentage == 0) {
				/*              printf("%0d%% done      \r",percentage); */
			} else {
				time_remaining =
					time_taken * 100 / (long) (percentage) - time_taken;
				sprintf(tmp,
						"%02d%% done   %02d:%02d taken   %02d:%02d remaining  %-37s\r",
						percentage, (int) (time_taken / 60),
						(int) (time_taken % 60),
						(int) (time_remaining / 60),
						(int) (time_remaining % 60), curr_fname);
				log_to_screen(tmp);
			}
			sync();
		}
	}
	paranoid_fclose(fout);
	paranoid_fclose(fin);
	log_it("Done.");
	return (0);
}


/**
 * Create the directory @p outdir_fname and all parent directories. Equivalent to <tt>mkdir -p</tt>.
 * @param outdir_fname The directory to create.
 * @return The return value of @c mkdir.
 */
int make_hole_for_dir(char *outdir_fname)
{
	char tmp[MAX_STR_LEN * 2];
	int res = 0;

	assert_string_is_neither_NULL_nor_zerolength(outdir_fname);
	sprintf(tmp, "mkdir -p %s", outdir_fname);
	res = system(tmp);
	return (res);
}


/**
 * Create the parent directories of @p outfile_fname.
 * @param outfile_fname The file to make a "hole" for.
 * @return 0, always.
 * @bug Return value unnecessary.
 */
int make_hole_for_file(char *outfile_fname)
{
	/*@ buffer ****************************************************** */
	char command[MAX_STR_LEN * 2];

	/*@ int  ******************************************************** */
	int res = 0;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(outfile_fname);
	assert(!strstr(outfile_fname, MNT_CDROM));
	assert(!strstr(outfile_fname, "/dev/cdrom"));
	sprintf(command, "mkdir -p \"%s\" 2> /dev/null", outfile_fname);
	res += system(command);
	sprintf(command, "rmdir \"%s\" 2> /dev/null", outfile_fname);
	res += system(command);
	sprintf(command, "rm -f \"%s\" 2> /dev/null", outfile_fname);
	res += system(command);
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
	char incoming[MAX_STR_LEN];

	/*@ end vars *************************************************** */


	fin = fopen(filelist_fname, "r");

	if (!fin) {
		log_OS_error("Unable to openin filelist_fname");
		return (0);
	}
	(void) fgets(incoming, MAX_STR_LEN - 1, fin);
	while (!feof(fin)) {
		if (strstr(incoming, wildcard)) {
			matches++;
		}
		(void) fgets(incoming, MAX_STR_LEN - 1, fin);
	}
	paranoid_fclose(fin);
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
	char tmp[MAX_STR_LEN + 1], lockfile_fname[MAX_STR_LEN + 1];
	int res;
	FILE *fin;

	sprintf(lockfile_fname, "/var/run/monitas-%s.pid", name_str);
	if (!pid) {
		log_it("Unregistering PID");
		if (unlink(lockfile_fname)) {
			log_it("Error unregistering PID");
		}
		return;
	}
	if (does_file_exist(lockfile_fname)) {
		tmp[0] = '\0';
		if ((fin = fopen(lockfile_fname, "r"))) {
			(void) fgets(tmp, MAX_STR_LEN, fin);
			paranoid_fclose(fin);
		} else {
			log_OS_error("Unable to openin lockfile_fname");
		}
		pid = (pid_t) atol(tmp);
		sprintf(tmp, "ps %ld > /dev/null 2> /dev/null", (long int) pid);
		res = system(tmp);
		if (!res) {
			log_it
				("I believe the daemon is already running. If it isn't, please delete %s and try again.",
				 lockfile_fname);
		}
	}
	sprintf(tmp, "echo %ld > %s 2> /dev/null", (long int) getpid(),
			lockfile_fname);
	if (system(tmp)) {
		fatal_error("Cannot register PID");
	}
}



/**
 * Determine the size (in KB) of @p dev in the mountlist in <tt>tmpdir</tt>/mountlist.txt.
 * @param tmpdir The tempdir where the mountlist is stored.
 * @param dev The device to search for.
 * @return The size of the partition in KB.
 */
long size_of_partition_in_mountlist_K(char *tmpdir, char *dev)
{
	char command[MAX_STR_LEN];
	char mountlist[MAX_STR_LEN];
	char sz_res[MAX_STR_LEN];
	long file_len_K;

	sprintf(mountlist, "%s/mountlist.txt", tmpdir);
	sprintf(command,
			"cat %s/mountlist.txt | grep \"%s \" | head -n1 | awk '{print $4;}'",
			tmpdir, dev);
	log_it(command);
	strcpy(sz_res, call_program_and_get_last_line_of_output(command));
	file_len_K = atol(sz_res);
	log_msg(4, "%s --> %s --> %ld", command, sz_res, file_len_K);
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

	/*@ end vars *************************************************** */

	malloc_string(fname);
	malloc_string(biggielist);
	malloc_string(comment);
	log_it("Calculating size of all biggiefiles (in total)");
	sprintf(biggielist, "%s/biggielist.txt", bkpinfo->tmpdir);
	log_it("biggielist = %s", biggielist);
	if (!(fin = fopen(biggielist, "r"))) {
		log_OS_error
			("Cannot open biggielist. OK, so estimate is based on filesets only.");
	} else {
		log_msg(4, "Reading it...");
		for (fgets(fname, MAX_STR_LEN, fin); !feof(fin);
			 fgets(fname, MAX_STR_LEN, fin)) {
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
			sprintf(comment,
					"After adding %s, scratchL+%ld now equals %ld", fname,
					file_len_K, scratchL);
			log_msg(4, comment);
			if (feof(fin)) {
				break;
			}
		}
	}
	log_it("Closing...");
	paranoid_fclose(fin);
	log_it("Finished calculating total size of all biggiefiles");
	paranoid_free(fname);
	paranoid_free(biggielist);
	paranoid_free(comment);
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
	char tmp[MAX_STR_LEN];
	char command[MAX_STR_LEN * 2];
	long long llres;
	/*@ pointers **************************************************** */
	char *p;
	FILE *fin;

	/*@ end vars *************************************************** */

	sprintf(command, "du -sk %s", mountpt);
	fin = popen(command, "r");
	(void) fgets(tmp, MAX_STR_LEN, fin);
	paranoid_pclose(fin);
	p = strchr(tmp, '\t');
	if (p) {
		*p = '\0';
	}
	for (p = tmp, llres = 0; *p != '\0'; p++) {
		llres *= 10;
		llres += (int) (*p - '0');
	}
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
	char command[MAX_STR_LEN * 2];
	char errorstr[MAX_STR_LEN];


	sprintf(command, "which %s > /dev/null 2> /dev/null", fname);
	sprintf(errorstr,
			"Please install '%s'. I cannot find it on your system.",
			fname);
	if (system(command)) {
		log_to_screen(errorstr);
		log_to_screen
			("There may be hyperlink at http://www.mondorescue.com which");
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
	char command[MAX_STR_LEN * 2];
	char tmp[MAX_STR_LEN];
	char old_pwd[MAX_STR_LEN];

	mvaddstr_and_log_it(g_currentY, 0,
						"Copying Mondo's core files to the scratch directory");

	log_msg(4, "g_mondo_home='%s'", g_mondo_home);
	if (strlen(g_mondo_home) < 2) {
		find_and_store_mondoarchives_home(g_mondo_home);
	}
	sprintf(command, CP_BIN " --parents -pRdf %s %s", g_mondo_home,
			bkpinfo->scratchdir);

	log_msg(4, "command = %s", command);
	if (run_program_and_log_output(command, 1)) {
		fatal_error("Failed to copy Mondo's stuff to scratchdir");
	}

	sprintf(tmp, "%s/payload.tgz", g_mondo_home);
	if (does_file_exist(tmp)) {
		log_it("Untarring payload %s to scratchdir %s", tmp,
			   bkpinfo->scratchdir);
		(void) getcwd(old_pwd, MAX_STR_LEN - 1);
		chdir(bkpinfo->scratchdir);
		sprintf(command, "tar -zxvf %s", tmp);
		if (run_program_and_log_output(command, FALSE)) {
			fatal_error("Failed to untar payload");
		}
		chdir(old_pwd);
	}

	sprintf(command, "cp -f %s/LAST-FILELIST-NUMBER %s", bkpinfo->tmpdir,
			bkpinfo->scratchdir);

	if (run_program_and_log_output(command, FALSE)) {
		fatal_error("Failed to copy LAST-FILELIST-NUMBER to scratchdir");
	}

	strcpy(tmp,
		   call_program_and_get_last_line_of_output("which mondorestore"));
	if (!tmp[0]) {
		fatal_error
			("'which mondorestore' returned null. Where's your mondorestore? `which` can't find it. That's odd. Did you install mondorestore?");
	}
	sprintf(command, "cp -f %s %s", tmp, bkpinfo->tmpdir);
	if (run_program_and_log_output(command, FALSE)) {
		fatal_error("Failed to copy mondorestore to tmpdir");
	}

	sprintf(command, "hostname > %s/HOSTNAME", bkpinfo->scratchdir);
	paranoid_system(command);

	if (bkpinfo->postnuke_tarball[0]) {
		sprintf(command, "cp -f %s %s/post-nuke.tgz",
				bkpinfo->postnuke_tarball, bkpinfo->tmpdir);
		if (run_program_and_log_output(command, FALSE)) {
			fatal_error("Unable to copy post-nuke tarball to tmpdir");
		}
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
	char outfile[MAX_STR_LEN];
	char nfs_dev[MAX_STR_LEN];
	char nfs_mount[MAX_STR_LEN];
	char nfs_client_ipaddr[MAX_STR_LEN];
	char nfs_client_netmask[MAX_STR_LEN];
	char nfs_client_broadcast[MAX_STR_LEN];
	char nfs_client_defgw[MAX_STR_LEN];
	char nfs_server_ipaddr[MAX_STR_LEN];
	char tmp[MAX_STR_LEN];
	char command[MAX_STR_LEN * 2];

	/*@ pointers ***** */
	char *p;
	FILE *fout;



	log_it("Storing NFS configuration");
	strcpy(tmp, bkpinfo->nfs_mount);
	p = strchr(tmp, ':');
	if (!p) {
		fatal_error
			("NFS mount doesn't have a colon in it, e.g. 192.168.1.4:/home/nfs");
	}
	*(p++) = '\0';
	strcpy(nfs_server_ipaddr, tmp);
	strcpy(nfs_mount, p);
	sprintf(command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\n' | head -n1 | cut -d' ' -f1");
	strcpy(nfs_dev, call_program_and_get_last_line_of_output(command));
	sprintf(command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\\n' | head -n1 | tr -s '\t' ' ' | cut -d' ' -f7 | cut -d':' -f2");
	strcpy(nfs_client_ipaddr,
		   call_program_and_get_last_line_of_output(command));
	sprintf(command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\\n' | head -n1 | tr -s '\t' ' ' | cut -d' ' -f9 | cut -d':' -f2");
	strcpy(nfs_client_netmask,
		   call_program_and_get_last_line_of_output(command));
	sprintf(command,
			"ifconfig | tr '\n' '#' | sed s/##// | tr '#' ' ' | tr '' '\\n' | head -n1 | tr -s '\t' ' ' | cut -d' ' -f8 | cut -d':' -f2");
	strcpy(nfs_client_broadcast,
		   call_program_and_get_last_line_of_output(command));
	sprintf(command,
			"route -n | grep '^0.0.0.0' | awk '{printf $2}'");
	strcpy(nfs_client_defgw,
		   call_program_and_get_last_line_of_output(command));
	sprintf(tmp,
			"nfs_client_ipaddr=%s; nfs_server_ipaddr=%s; nfs_mount=%s",
			nfs_client_ipaddr, nfs_server_ipaddr, nfs_mount);
	if (strlen(nfs_dev) < 2) {
		fatal_error
			("Unable to find ethN (eth0, eth1, ...) adapter via NFS mount you specified.");
	}
	sprintf(outfile, "%s/start-nfs", bkpinfo->tmpdir);
	sprintf(tmp, "outfile = %s", outfile);
	log_it(tmp);
	if (!(fout = fopen(outfile, "w"))) {
		fatal_error("Cannot store NFS config");
	}
	fprintf(fout, "ifconfig lo 127.0.0.1  # config loopback\n");
	fprintf(fout, "ifconfig %s %s netmask %s broadcast %s	# config client\n", nfs_dev,
			nfs_client_ipaddr, nfs_client_netmask, nfs_client_broadcast);
	fprintf(fout, "route add default gw %s  # default route\n", nfs_client_defgw);
	fprintf(fout, "ping -c 1 %s	# ping server\n", nfs_server_ipaddr);
	fprintf(fout, "mount -t nfs -o nolock %s /tmp/isodir\n",
			bkpinfo->nfs_mount);
	fprintf(fout, "exit 0\n");
	paranoid_fclose(fout);
	chmod(outfile, 0777);
	make_hole_for_dir("/var/cache/mondo-archive");

//  paranoid_system ("mkdir -p /var/cache/mondo-archive 2> /dev/null");

	sprintf(tmp, "cp -f %s /var/cache/mondo-archive", outfile);
	run_program_and_log_output(tmp, FALSE);

	sprintf(tmp, "%s/NFS-DEV", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_dev);

	sprintf(tmp, "%s/NFS-CLIENT-IPADDR", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_ipaddr);
	sprintf(tmp, "%s/NFS-CLIENT-NETMASK", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_netmask);
	sprintf(tmp, "%s/NFS-CLIENT-BROADCAST", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_broadcast);
	sprintf(tmp, "%s/NFS-CLIENT-DEFGW", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_client_defgw);
	sprintf(tmp, "%s/NFS-SERVER-IPADDR", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, nfs_server_ipaddr);
	sprintf(tmp, "%s/NFS-SERVER-MOUNT", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, bkpinfo->nfs_mount);
	sprintf(tmp, "%s/NFS-SERVER-PATH", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, bkpinfo->nfs_remote_dir);
	sprintf(tmp, "%s/ISO-PREFIX", bkpinfo->tmpdir);
	write_one_liner_data_file(tmp, bkpinfo->prefix);
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
	char tmp[MAX_STR_LEN];

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
		sprintf(tmp,
				"Your backup will probably occupy a single %s. Maybe two.",
				media_descriptor_string(bkpinfo->backup_media_type));
	} else if (scratchLL > 4) {
		sprintf(tmp,
				"Your backup will occupy one meeeeellion media! (maybe %s)",
				number_to_text((int) (scratchLL + 1)));
	} else {
		sprintf(tmp, "Your backup will occupy approximately %s media.",
				number_to_text((int) (scratchLL + 1)));
	}
	if (!bkpinfo->image_devs[0] && (scratchLL < 50)) {
		log_to_screen(tmp);
	}
}


/**
 * Get the last suffix of @p instr.
 * If @p instr was "httpd.log.gz", we would return "gz".
 * @param instr The filename to get the suffix of.
 * @return The suffix (without a dot), or "" if none.
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *sz_last_suffix(char *instr)
{
	static char outstr[MAX_STR_LEN];
	char *p;

	p = strrchr(instr, '.');
	if (!p) {
		outstr[0] = '\0';
	} else {
		strcpy(outstr, p);
	}
	return (outstr);
}


/**
 * Determine whether a file is compressed. This is done
 * by reading through the "do-not-compress-these" file distributed with Mondo.
 * @param filename The file to check.
 * @return TRUE if it's compressed, FALSE if not.
 */
bool is_this_file_compressed(char *filename)
{
	char do_not_compress_these[MAX_STR_LEN];
	char tmp[MAX_STR_LEN];
	char *p;

	sprintf(tmp, "%s/do-not-compress-these", g_mondo_home);
	if (!does_file_exist(tmp)) {
		return (FALSE);
	}
	strcpy(do_not_compress_these, last_line_of_file(tmp));
	for (p = do_not_compress_these; p != NULL; p++) {
		strcpy(tmp, p);
		if (strchr(tmp, ' ')) {
			*(strchr(tmp, ' ')) = '\0';
		}
		if (!strcmp(sz_last_suffix(filename), tmp)) {	/*printf("MATCH\n"); */
			return (TRUE);
		}
		if (!(p = strchr(p, ' '))) {
			break;
		}
	}
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

	malloc_string(tmp);
	if ((fout = fopen(outfile, "w"))) {
		fprintf(fout,
				"#!/bin/sh\n\nmount /boot > /dev/null 2> /dev/null\ngrub-install $@\nres=$?\nsync;sync;sync\nexit $res\n");
		paranoid_fclose(fout);
		log_msg(2, "Created %s", outfile);
		sprintf(tmp, "chmod +x %s", outfile);
		paranoid_system(tmp);
		retval = 0;
	} else {
		retval = 1;
	}
	paranoid_free(tmp);
	return (retval);
}

/* @} - end fileGroup */
