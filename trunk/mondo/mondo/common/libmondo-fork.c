/* $Id$
  subroutines for handling forking/pthreads/etc.
*/


#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-fork.h"
#include "libmondo-string-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-tools-EXT.h"
#include "lib-common-externs.h"

/*@unused@*/
//static char cvsid[] = "$Id$";

extern char *g_tmpfs_mountpt;
extern t_bkptype g_backup_media_type;
extern bool g_text_mode;
pid_t g_buffer_pid = 0;


/**
 * Call a program and retrieve its last line of output.
 * @param call The program to run.
 * @return The last line of its output.
 * @note The returned value points to static storage that will be overwritten with each call.
 */
char *call_program_and_get_last_line_of_output(char *call)
{
	/*@ buffers ***************************************************** */
	static char *result = NULL;
	char *tmp = NULL;

	/*@ pointers **************************************************** */
	FILE *fin;

	int n = 0;

	/*@******************************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(call);
	if ((fin = popen(call, "r"))) {
		for (getline(&tmp, &n, fin); !feof(fin);
			 getline(&tmp, &n, fin)) {
			if (strlen(tmp) > 1) {
				if (result != NULL) {
					paranoid_free(result);
				}
				asprintf(&result, tmp);
			}
		}
		paranoid_pclose(fin);
	} else {
		log_OS_error("Unable to popen call");
	}
	strip_spaces(result);
	paranoid_free(tmp);
	return (result);
}

#define MONDO_POPMSG  "Your PC will not retract the CD tray automatically. Please call mondoarchive with the -m (manual CD tray) flag."

/**
 * Call mkisofs to create an ISO image.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->manual_cd_tray
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->please_dont_eject_when_restoring
 * @param basic_call The call to mkisofs. May contain tokens that will be resolved to actual data. The tokens are:
 * - @c _ISO_ will become the ISO file (@p isofile)
 * - @c _CD#_ becomes the CD number (@p cd_no)
 * - @c _ERR_ becomes the logfile (@p g_logfile)
 * @param isofile Replaces @c _ISO_ in @p basic_call. Should probably be the ISO image to create (-o parameter to mkisofs).
 * @param cd_no Replaces @c _CD#_ in @p basic_call. Should probably be the CD number.
 * @param logstub Unused.
 * @param what_i_am_doing The action taking place (e.g. "Making ISO #1"). Used as the title of the progress dialog.
 * @return Exit code of @c mkisofs (0 is success, anything else indicates failure).
 * @bug @p logstub is unused.
 */
int
eval_call_to_make_ISO(struct s_bkpinfo *bkpinfo,
					  char *basic_call, char *isofile,
					  int cd_no, char *logstub, char *what_i_am_doing)
{

	/*@ int's  *** */
	int retval = 0;


	/*@ buffers      *** */
	char *midway_call, *ultimate_call, *tmp, *command,
		*cd_number_str;
	char *p;

/*@***********   End Variables ***************************************/

	log_msg(3, "Starting");
	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(basic_call);
	assert_string_is_neither_NULL_nor_zerolength(isofile);
	assert_string_is_neither_NULL_nor_zerolength(logstub);
	if (!(midway_call = malloc(1200))) {
		fatal_error("Cannot malloc midway_call");
	}
	if (!(ultimate_call = malloc(1200))) {
		fatal_error("Cannot malloc ultimate_call");
	}
	if (!(tmp = malloc(1200))) {
		fatal_error("Cannot malloc tmp");
	}

	asprintf(&cd_number_str, "%d", cd_no);
	resolve_naff_tokens(midway_call, basic_call, isofile, "_ISO_");
	resolve_naff_tokens(tmp, midway_call, cd_number_str, "_CD#_");
	paranoid_free(cd_number_str);

	resolve_naff_tokens(ultimate_call, tmp, MONDO_LOGFILE, "_ERR_");
	log_msg(4, "basic call = '%s'", basic_call);
	log_msg(4, "midway_call = '%s'", midway_call);
	log_msg(4, "tmp = '%s'", tmp);
	log_msg(4, "ultimate call = '%s'", ultimate_call);
	asprintf(&command, "%s >> %s", ultimate_call, MONDO_LOGFILE);

	log_to_screen
		("Please be patient. Do not be alarmed by on-screen inactivity.");
	log_msg(4, "Calling open_evalcall_form() with what_i_am_doing='%s'",
			what_i_am_doing);
	strcpy(tmp, command);
	if (bkpinfo->manual_cd_tray) {
		p = strstr(tmp, "2>>");
		if (p) {
			sprintf(p, "   ");
			while (*p == ' ') {
				p++;
			}
			for (; *p != ' '; p++) {
				*p = ' ';
			}
		}
		paranoid_free(command);
		asprintf(&command, tmp);
#ifndef _XWIN
		if (!g_text_mode) {
			newtSuspend();
		}
#endif
		log_msg(1, "command = '%s'", command);
		retval += system(command);
		if (!g_text_mode) {
			newtResume();
		}
		if (retval) {
			log_msg(2, "Basic call '%s' returned an error.", basic_call);
			popup_and_OK("Press ENTER to continue.");
			popup_and_OK
				("mkisofs and/or cdrecord returned an error. CD was not created");
		}
	}
	/* if text mode then do the above & RETURN; if not text mode, do this... */
	else {
		log_msg(3, "command = '%s'", command);
//      yes_this_is_a_goto:
		retval =
			run_external_binary_with_percentage_indicator_NEW
			(what_i_am_doing, command);
	}
	paranoid_free(command);

	paranoid_free(midway_call);
	paranoid_free(ultimate_call);
	paranoid_free(tmp);
	return (retval);
}


/**
 * Run a program and log its output (stdout and stderr) to the logfile.
 * @param program The program to run. Passed to the shell, so you can use pipes etc.
 * @param debug_level If @p g_loglevel is higher than this, do not log the output.
 * @return The exit code of @p program (depends on the command, but 0 almost always indicates success).
 */
int run_program_and_log_output(char *program, int debug_level)
{
	/*@ buffer ****************************************************** */
	char *callstr;
	char *incoming = NULL;
	char tmp[MAX_STR_LEN * 2];

	/*@ int ********************************************************* */
	int res;
	int i;
	int n = 0;
	int len;
	bool log_if_failure = FALSE;
	bool log_if_success = FALSE;

	/*@ pointers *************************************************** */
	FILE *fin;
	char *p;

	/*@ end vars *************************************************** */

	assert(program != NULL);
	if (!program[0]) {
		log_msg(2, "Warning - asked to run zerolength program");
		return (1);
	}

	if (debug_level <= g_loglevel) {
		log_if_success = TRUE;
		log_if_failure = TRUE;
	}
	asprintf(&callstr,
			"%s > /tmp/mondo-run-prog-thing.tmp 2> /tmp/mondo-run-prog-thing.err",
			program);
	while ((p = strchr(callstr, '\r'))) {
		*p = ' ';
	}
	while ((p = strchr(callstr, '\n'))) {
		*p = ' ';
	}							/* single '=' is intentional */


	len = (int) strlen(program);
	for (i = 0; i < 35 - len / 2; i++) {
		tmp[i] = '-';
	}
	tmp[i] = '\0';
	strcat(tmp, " ");
	strcat(tmp, program);
	strcat(tmp, " ");
	for (i = 0; i < 35 - len / 2; i++) {
		strcat(tmp, "-");
	}
	res = system(callstr);
	if (((res == 0) && log_if_success) || ((res != 0) && log_if_failure)) {
		log_msg(0, "running: %s", callstr);
		log_msg(0,
				"--------------------------------start of output-----------------------------");
	}
	paranoid_free(callstr);

	if (log_if_failure
		&&
		system
		("cat /tmp/mondo-run-prog-thing.err >> /tmp/mondo-run-prog-thing.tmp 2> /dev/null"))
	{
		log_OS_error("Command failed");
	}
	unlink("/tmp/mondo-run-prog-thing.err");
	fin = fopen("/tmp/mondo-run-prog-thing.tmp", "r");
	if (fin) {
		for (getline(&incoming, &n, fin); !feof(fin);
			 getline(&incoming, &n, fin)) {
			/* patch by Heiko Schlittermann */
			p = incoming;
			while (p && *p) {
				if ((p = strchr(p, '%'))) {
					memmove(p, p + 1, strlen(p) + 1);
					p += 2;
				}
			}
			/* end of patch */
			strip_spaces(incoming);
			if ((res == 0 && log_if_success)
				|| (res != 0 && log_if_failure)) {
				log_msg(0, incoming);
			}
		}
		paranoid_free(incoming);
		paranoid_fclose(fin);
	}
	unlink("/tmp/mondo-run-prog-thing.tmp");
	if ((res == 0 && log_if_success) || (res != 0 && log_if_failure)) {
		log_msg(0,
				"--------------------------------end of output------------------------------");
		if (res) {
			log_msg(0, "...ran with res=%d", res);
		} else {
			log_msg(0, "...ran just fine. :-)");
		}
	}
//  else
//    { log_msg (0, "-------------------------------ran w/ res=%d------------------------------", res); }
	return (res);
}


/**
 * Run a program and log its output to the screen.
 * @param basic_call The program to run.
 * @param what_i_am_doing The title of the evalcall form.
 * @return The return value of the command (varies, but 0 almost always means success).
 * @see run_program_and_log_output
 * @see log_to_screen
 */
int run_program_and_log_to_screen(char *basic_call, char *what_i_am_doing)
{
	/*@ int ******************************************************** */
	int retval = 0;
	int res = 0;
	int n = 0;
	int i;

	/*@ pointers **************************************************** */
	FILE *fin;

	/*@ buffers **************************************************** */
	char *tmp;
	char *command;
	char *lockfile;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(basic_call);

	asprintf(&lockfile, "/tmp/mojo-jojo.blah.XXXXXX");
	mkstemp(lockfile);
	asprintf(&command,
			"echo hi > %s ; %s >> %s 2>> %s; res=$?; sleep 1; rm -f %s; exit $res",
			lockfile, basic_call, MONDO_LOGFILE, MONDO_LOGFILE, lockfile);
	open_evalcall_form(what_i_am_doing);
	asprintf(&tmp, "Executing %s", basic_call);
	log_msg(2, tmp);
	paranoid_free(tmp);

	if (!(fin = popen(command, "r"))) {
		log_OS_error("Unable to popen-in command");
		asprintf(&tmp, "Failed utterly to call '%s'", command);
		log_to_screen(tmp);
		paranoid_free(tmp);
		paranoid_free(lockfile);
		paranoid_free(command);
		return (1);
	}
	paranoid_free(command);

	if (!does_file_exist(lockfile)) {
		log_to_screen("Waiting for external binary to start");
		for (i = 0; i < 60 && !does_file_exist(lockfile); sleep(1), i++) {
			log_msg(3, "Waiting for lockfile %s to exist", lockfile);
		}
	}
#ifdef _XWIN
	/* This only can update when newline goes into the file,
	   but it's *much* prettier/faster on Qt. */
	tmp = NULL;
	while (does_file_exist(lockfile)) {
		while (!feof(fin)) {
			if (!getline(&tmp, &n, fin))
				break;
			log_to_screen(tmp);
		}
		usleep(500000);
	}
	paranoid_free(tmp);
#else
	/* This works on Newt, and it gives quicker updates. */
	for (; does_file_exist(lockfile); sleep(1)) {
		log_file_end_to_screen(MONDO_LOGFILE, "");
		update_evalcall_form(1);
	}
#endif
	/* Evaluate the status returned by pclose to get the exit code of the called program. */
	errno = 0;
	res = pclose(fin);
	/* Log actual pclose errors. */
	if (errno)
		log_msg(5, "pclose err: %d", errno);
	/* Check if we have a valid status. If we do, extract the called program's exit code. */
	/* If we don't, highlight this fact by returning -1. */
	if (WIFEXITED(res)) {
		retval = WEXITSTATUS(res);
	} else {
		retval = -1;
	}
	close_evalcall_form();
	unlink(lockfile);
	paranoid_free(lockfile);

	return (retval);
}


/**
 * Thread callback to run a command (partimage) in the background.
 * @param xfb A transfer block of @c char, containing:
 * - xfb:[0] A marker, should be set to 2. Decremented to 1 while the command is running and 0 when it's finished.
 * - xfb:[1] The command's return value, if xfb:[0] is 0.
 * - xfb+2:  A <tt>NULL</tt>-terminated string containing the command to be run.
 * @return NULL to pthread_join.
 */
void *call_partimage_in_bkgd(void *xfb)
{
	char *transfer_block;
	int retval = 0;

	g_buffer_pid = getpid();
	unlink("/tmp/null");
	log_msg(1, "starting");
	transfer_block = (char *) xfb;
	transfer_block[0]--;		// should now be 1
	retval = system(transfer_block + 2);
	if (retval) {
		log_OS_error("partimage returned an error");
	}
	transfer_block[1] = retval;
	transfer_block[0]--;		// should now be 0
	g_buffer_pid = 0;
	log_msg(1, "returning");
	pthread_exit(NULL);
}


/**
 * File to touch if we want partimage to wait for us.
 */
#define PAUSE_PARTIMAGE_FNAME "/tmp/PAUSE-PARTIMAGE-FOR-MONDO"

/**
 * Apparently used. @bug This has a purpose, but what?
 */
#define PIMP_START_SZ "STARTSTARTSTART9ff3kff9a82gv34r7fghbkaBBC2T231hc81h42vws8"
#define PIMP_END_SZ "ENDENDEND0xBBC10xBBC2T231hc81h42vws89ff3kff9a82gv34r7fghbka"

/**
 * Marker to start the next subvolume for Partimage.
 */
#define NEXT_SUBVOL_PLEASE "I-grew-up-on-the-crime-side,-the-New-York-Times-side,-where-staying-alive-was-no-jive"

/**
 * Marker to end the partimage file.
 */
#define NO_MORE_SUBVOLS "On-second-hand,momma-bounced-on-old-man,-and-so-we-moved-to-Shaolin-Land."

int copy_from_src_to_dest(FILE * f_orig, FILE * f_archived, char direction)
{
// if dir=='w' then copy from orig to archived
// if dir=='r' then copy from archived to orig
	char *tmp;
	char *buf;
	long int bytes_to_be_read, bytes_read_in, bytes_written_out =
		0, bufcap, subsliceno = 0;
	int retval = 0;
	FILE *fin;
	FILE *fout;
	FILE *ftmp;

	log_msg(5, "Opening.");
	bufcap = 256L * 1024L;
	if (!(buf = malloc(bufcap))) {
		fatal_error("Failed to malloc() buf");
	}

	if (direction == 'w') {
		fin = f_orig;
		fout = f_archived;
		asprintf(&tmp, "%-64s", PIMP_START_SZ);
		if (fwrite(tmp, 1, 64, fout) != 64) {
			fatal_error("Can't write the introductory block");
		}
		paranoid_free(tmp);

		while (1) {
			bytes_to_be_read = bytes_read_in = fread(buf, 1, bufcap, fin);
			if (bytes_read_in == 0) {
				break;
			}
			asprintf(&tmp, "%-64ld", bytes_read_in);
			if (fwrite(tmp, 1, 64, fout) != 64) {
				fatal_error("Cannot write introductory block");
			}
			paranoid_free(tmp);

			log_msg(7,
					"subslice #%ld --- I have read %ld of %ld bytes in from f_orig",
					subsliceno, bytes_read_in, bytes_to_be_read);
			bytes_written_out += fwrite(buf, 1, bytes_read_in, fout);
			asprintf(&tmp, "%-64ld", subsliceno);
			if (fwrite(tmp, 1, 64, fout) != 64) {
				fatal_error("Cannot write post-thingy block");
			}
			paranoid_free(tmp);

			log_msg(7, "Subslice #%d written OK", subsliceno);
			subsliceno++;
		}
		asprintf(&tmp, "%-64ld", 0L);
		if (fwrite(tmp, 1, 64L, fout) != 64L) {
			fatal_error("Cannot write final introductory block");
		}
	} else {
		fin = f_archived;
		fout = f_orig;
		if (!(tmp = malloc(64L))) {
			fatal_error("Failed to malloc() tmp");
		}
		if (fread(tmp, 1, 64L, fin) != 64L) {
			fatal_error("Cannot read the introductory block");
		}
		log_msg(5, "tmp is %s", tmp);
		if (!strstr(tmp, PIMP_START_SZ)) {
			fatal_error("Can't find intro blk");
		}
		if (fread(tmp, 1, 64L, fin) != 64L) {
			fatal_error("Cannot read introductory blk");
		}
		bytes_to_be_read = atol(tmp);
		while (bytes_to_be_read > 0) {
			log_msg(7, "subslice#%ld, bytes=%ld", subsliceno,
					bytes_to_be_read);
			bytes_read_in = fread(buf, 1, bytes_to_be_read, fin);
			if (bytes_read_in != bytes_to_be_read) {
				fatal_error
					("Danger, WIll Robinson. Failed to read whole subvol from archives.");
			}
			bytes_written_out += fwrite(buf, 1, bytes_read_in, fout);
			if (fread(tmp, 1, 64, fin) != 64) {
				fatal_error("Cannot read post-thingy block");
			}
			if (atol(tmp) != subsliceno) {
				log_msg(1, "Wanted subslice %ld but got %ld ('%s')",
						subsliceno, atol(tmp), tmp);
			}
			log_msg(7, "Subslice #%ld read OK", subsliceno);
			subsliceno++;
			if (fread(tmp, 1, 64, fin) != 64) {
				fatal_error("Cannot read introductory block");
			}
			bytes_to_be_read = atol(tmp);
		}
	}

//  log_msg(4, "Written %ld of %ld bytes", bytes_written_out, bytes_read_in);

	if (direction == 'w') {
		asprintf(&tmp, "%-64s", PIMP_END_SZ);
		if (fwrite(tmp, 1, 64, fout) != 64) {
			fatal_error("Can't write the final block");
		}
		paranoid_free(tmp);
	} else {
		log_msg(1, "tmpA is %s", tmp);
		if (!strstr(tmp, PIMP_END_SZ)) {
			if (fread(tmp, 1, 64, fin) != 64) {
				fatal_error("Can't read the final block");
			}
			log_msg(5, "tmpB is %s", tmp);
			if (!strstr(tmp, PIMP_END_SZ)) {
				ftmp = fopen("/tmp/out.leftover", "w");
				bytes_read_in = fread(tmp, 1, 64, fin);
				log_msg(1, "bytes_read_in = %ld", bytes_read_in);
//      if (bytes_read_in!=128+64) { fatal_error("Can't read the terminating block"); }
				fwrite(tmp, 1, bytes_read_in, ftmp);
				paranoid_free(tmp);
				
				if (!(tmp = malloc(512))) {
					fatal_error("Failed to malloc() tmp");
				}
				fread(tmp, 1, 512, fin);
				log_msg(0, "tmp = '%s'", tmp);
				fwrite(tmp, 1, 512, ftmp);
				fclose(ftmp);
				fatal_error("Missing terminating block");
			}
		}
		paranoid_free(tmp);
	}

	paranoid_free(buf);
	log_msg(3, "Successfully copied %ld bytes", bytes_written_out);
	return (retval);
}


/**
 * Call partimage from @p input_device to @p output_fname.
 * @param input_device The device to read.
 * @param output_fname The file to write.
 * @return 0 for success, nonzero for failure.
 */
int dynamically_create_pipes_and_copy_from_them_to_output_file(char
															   *input_device, char
															   *output_fname)
{
	char *curr_fifo;
	char *prev_fifo = NULL;
	char *next_fifo;
	char *command;
	char *sz_call_to_partimage;
	int fifo_number = 0;
	struct stat buf;
	pthread_t partimage_thread;
	int res = 0;
	char *tmpstub;
	FILE *fout;
	FILE *fin;
	char *tmp;

	log_msg(1, "g_tmpfs_mountpt = %s", g_tmpfs_mountpt);
	if (g_tmpfs_mountpt && g_tmpfs_mountpt[0]
		&& does_file_exist(g_tmpfs_mountpt)) {
		asprintf(&tmpstub, g_tmpfs_mountpt);
	} else {
		asprintf(&tmpstub, "/tmp");
	}
	paranoid_system("rm -f /tmp/*PARTIMAGE*");
	asprintf(&command, "rm -Rf %s/pih-fifo-*", tmpstub);
	paranoid_system(command);
	paranoid_free(command);

	asprintf(&tmp, "%s/pih-fifo-%ld", tmpstub, (long int) random());
	paranoid_free(tmpstub);
	tmpstub = tmp;
	paranoid_free(tmp);

	mkfifo(tmpstub, S_IRWXU | S_IRWXG);	// never used, though...
	asprintf(&curr_fifo, "%s.%03d", tmpstub, fifo_number);
	asprintf(&next_fifo, "%s.%03d", tmpstub, fifo_number + 1);
	mkfifo(curr_fifo, S_IRWXU | S_IRWXG);
	mkfifo(next_fifo, S_IRWXU | S_IRWXG);	// make sure _next_ fifo already exists before we call partimage
	asprintf(&sz_call_to_partimage,
			"%c%cpartimagehack " PARTIMAGE_PARAMS
			" save %s %s > /tmp/stdout 2> /tmp/stderr", 2, 0, input_device,
			tmpstub);
	log_msg(5, "curr_fifo   = %s", curr_fifo);
	log_msg(5, "next_fifo   = %s", next_fifo);
	log_msg(5, "sz_call_to_partimage call is '%s'",
			sz_call_to_partimage + 2);
	if (!lstat(output_fname, &buf) && S_ISREG(buf.st_mode)) {
		log_msg(5, "Deleting %s", output_fname);
		unlink(output_fname);
	}
	if (!(fout = fopen(output_fname, "w"))) {
		fatal_error("Unable to openout to output_fname");
	}
	res =
		pthread_create(&partimage_thread, NULL, call_partimage_in_bkgd,
					   (void *) sz_call_to_partimage);
	if (res) {
		fatal_error("Failed to create thread to call partimage");
	}
	log_msg(1, "Running fore/back at same time");
	log_to_screen("Working with partimagehack...");
	while (sz_call_to_partimage[0] > 0) {
		asprintf(&tmp, "%s\n", NEXT_SUBVOL_PLEASE);
		if (fwrite(tmp, 1, 128, fout) != 128) {
			fatal_error("Cannot write interim block");
		}
		paranoid_free(tmp);

		log_msg(5, "fifo_number=%d", fifo_number);
		log_msg(4, "Cat'ting %s", curr_fifo);
		if (!(fin = fopen(curr_fifo, "r"))) {
			fatal_error("Unable to openin from fifo");
		}
		if (prev_fifo !=  NULL) {
			log_msg(5, "Deleting %s", prev_fifo);
			unlink(prev_fifo);		// just in case
			paranoid_free(prev_fifo);
		}
		copy_from_src_to_dest(fin, fout, 'w');
		paranoid_fclose(fin);
		fifo_number++;

		prev_fifo = curr_fifo;
		curr_fifo = next_fifo;
		log_msg(5, "Creating %s", next_fifo);
		asprintf(&next_fifo, "%s.%03d", tmpstub, fifo_number + 1);
		mkfifo(next_fifo, S_IRWXU | S_IRWXG);	// make sure _next_ fifo exists before we cat this one
		system("sync");
		sleep(5);
	}
	asprintf(&tmp, "%s\n", NO_MORE_SUBVOLS);
	if (fwrite(tmp, 1, 128, fout) != 128) {
		fatal_error("Cannot write interim block");
	}
	if (fwrite(tmp, 1, 128, fout) != 128) {
		fatal_error("Cannot write interim block");
	}
	if (fwrite(tmp, 1, 128, fout) != 128) {
		fatal_error("Cannot write interim block");
	}
	if (fwrite(tmp, 1, 128, fout) != 128) {
		fatal_error("Cannot write interim block");
	}
	paranoid_free(tmp);
	paranoid_fclose(fout);
	log_to_screen("Cleaning up after partimagehack...");
	log_msg(3, "Final fifo_number=%d", fifo_number);
	paranoid_system("sync");
	unlink(next_fifo);
	paranoid_free(next_fifo);

	unlink(curr_fifo);
	paranoid_free(curr_fifo);

	unlink(prev_fifo);
	paranoid_free(prev_fifo);

	log_to_screen("Finished cleaning up.");

//  if (!lstat(sz_wait_for_this_file, &statbuf))
//    { log_msg(3, "WARNING! %s was not processed.", sz_wait_for_this_file); }
	log_msg(2, "Waiting for pthread_join() to join.");
	pthread_join(partimage_thread, NULL);
	res = sz_call_to_partimage[1];
	paranoid_free(sz_call_to_partimage);
	log_msg(2, "pthread_join() joined OK.");
	log_msg(1, "Partimagehack(save) returned %d", res);
	unlink(tmpstub);
	paranoid_free(tmpstub);

	return (res);
}


/**
 * Feed @p input_device through partimage to @p output_fname.
 * @param input_device The device to image.
 * @param output_fname The file to write.
 * @return 0 for success, nonzero for failure.
 */
int feed_into_partimage(char *input_device, char *output_fname)
{
// BACKUP
	int res;

	if (!does_file_exist(input_device)) {
		fatal_error("input device does not exist");
	}
	if (!find_home_of_exe("partimagehack")) {
		fatal_error("partimagehack not found");
	}
	res =
		dynamically_create_pipes_and_copy_from_them_to_output_file
		(input_device, output_fname);
	return (res);
}


int run_external_binary_with_percentage_indicator_OLD(char *tt, char *cmd)
{

	/*@ int *************************************************************** */
	int res = 0;
	int percentage = 0;
	int maxpc = 0;
	int pcno = 0;
	int last_pcno = 0;

	/*@ buffers *********************************************************** */
	char *command;
	char *tempfile;
	/*@ pointers ********************************************************** */
	FILE *pin;

	assert_string_is_neither_NULL_nor_zerolength(cmd);

	asprintf(&tempfile,
		   call_program_and_get_last_line_of_output
		   ("mktemp -q /tmp/mondo.XXXXXXXX"));
	asprintf(&command, "%s >> %s 2>> %s; rm -f %s", cmd, tempfile, tempfile,
			tempfile);
	log_msg(3, command);
	open_evalcall_form(tt);

	if (!(pin = popen(command, "r"))) {
		log_OS_error("fmt err");
		paranoid_free(command);
		paranoid_free(tempfile);
		return (1);
	}
	paranoid_free(command);

	maxpc = 100;
// OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
	for (sleep(1); does_file_exist(tempfile); sleep(1)) {
		pcno = grab_percentage_from_last_line_of_file(MONDO_LOGFILE);
		if (pcno < 0 || pcno > 100) {
			log_msg(5, "Weird pc#");
			continue;
		}
		percentage = pcno * 100 / maxpc;
		if (pcno <= 5 && last_pcno > 40) {
			close_evalcall_form();
			open_evalcall_form("Verifying...");
		}
		last_pcno = pcno;
		update_evalcall_form(percentage);
	}
// OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
	close_evalcall_form();
	if (pclose(pin)) {
		res++;
		log_OS_error("Unable to pclose");
	}
	unlink(tempfile);
	paranoid_free(tempfile);
	return (res);
}


void *run_prog_in_bkgd_then_exit(void *info)
{
	char *sz_command;
	static int res = 4444;

	res = 999;
	sz_command = (char *) info;
	log_msg(4, "sz_command = '%s'", sz_command);
	res = system(sz_command);
	if (res > 256 && res != 4444) {
		res = res / 256;
	}
	log_msg(4, "child res = %d", res);
	sz_command[0] = '\0';
	pthread_exit((void *) (&res));
}


int run_external_binary_with_percentage_indicator_NEW(char *tt, char *cmd)
{

	/*@ int *************************************************************** */
	int res = 0;
	int percentage = 0;
	int maxpc = 100;
	int pcno = 0;
	int last_pcno = 0;
	int counter = 0;

	/*@ buffers *********************************************************** */
	char *command;
	/*@ pointers ********************************************************** */
	static int chldres = 0;
	int *pchild_result;
	pthread_t childthread;

	pchild_result = &chldres;
	assert_string_is_neither_NULL_nor_zerolength(cmd);
	assert_string_is_neither_NULL_nor_zerolength(tt);
	*pchild_result = 999;

	asprintf(&command, "%s 2>> %s", cmd, MONDO_LOGFILE);
	log_msg(3, "command = '%s'", command);
	if ((res =
		 pthread_create(&childthread, NULL, run_prog_in_bkgd_then_exit,
						(void *) command))) {
		fatal_error("Unable to create an archival thread");
	}

	log_msg(8, "Parent running");
	open_evalcall_form(tt);
	for (sleep(1); command[0] != '\0'; sleep(1)) {
		pcno = grab_percentage_from_last_line_of_file(MONDO_LOGFILE);
		if (pcno <= 0 || pcno > 100) {
			log_msg(8, "Weird pc#");
			continue;
		}
		percentage = pcno * 100 / maxpc;
		if (pcno <= 5 && last_pcno >= 40) {
			close_evalcall_form();
			open_evalcall_form("Verifying...");
		}
		if (counter++ >= 5) {
			counter = 0;
			log_file_end_to_screen(MONDO_LOGFILE, "");
		}
		last_pcno = pcno;
		update_evalcall_form(percentage);
	}
	paranoid_free(command);

	log_file_end_to_screen(MONDO_LOGFILE, "");
	close_evalcall_form();
	pthread_join(childthread, (void *) (&pchild_result));
	if (pchild_result) {
		res = *pchild_result;
	} else {
		res = 666;
	}
	log_msg(3, "Parent res = %d", res);
	return (res);
}

#define PIH_LOG "/var/log/partimage-debug.log"

/**
 * Feed @p input_fifo through partimage (restore) to @p output_device.
 * @param input_fifo The partimage file to read.
 * @param output_device Where to put the output.
 * @return The return value of partimagehack (0 for success).
 * @bug Probably unnecessary, as the partimage is just a sparse file. We could use @c dd to restore it.
 */
int feed_outfrom_partimage(char *output_device, char *input_fifo)
{
// RESTORE
	char *tmp;
	char *stuff;
	char *sz_call_to_partimage;
	pthread_t partimage_thread;
	int res;
	char *curr_fifo;
	char *prev_fifo;
	char *oldest_fifo = NULL;
	char *next_fifo;
	char *afternxt_fifo;
	int fifo_number = 0;
	char *tmpstub;
	FILE *fin;
	FILE *fout;

	log_msg(1, "output_device=%s", output_device);
	log_msg(1, "input_fifo=%s", input_fifo);
	asprintf(&tmpstub, "/tmp");

	log_msg(1, "tmpstub was %s", tmpstub);
	asprintf(&stuff, tmpstub);
	paranoid_free(tmpstub);

	asprintf(&tmpstub, "%s/pih-fifo-%ld", stuff, (long int) random());
	paranoid_free(stuff);

	log_msg(1, "tmpstub is now %s", tmpstub);
	unlink("/tmp/PARTIMAGEHACK-POSITION");
	unlink(PAUSE_PARTIMAGE_FNAME);
	paranoid_system("rm -f /tmp/*PARTIMAGE*");
	asprintf(&curr_fifo, "%s.%03d", tmpstub, fifo_number);
	asprintf(&next_fifo, "%s.%03d", tmpstub, fifo_number + 1);
	asprintf(&afternxt_fifo, "%s.%03d", tmpstub, fifo_number + 2);
	mkfifo(PIH_LOG, S_IRWXU | S_IRWXG);
	mkfifo(curr_fifo, S_IRWXU | S_IRWXG);
	mkfifo(next_fifo, S_IRWXU | S_IRWXG);	// make sure _next_ fifo already exists before we call partimage
	mkfifo(afternxt_fifo, S_IRWXU | S_IRWXG);
	system("cat " PIH_LOG " > /dev/null &");
	log_msg(3, "curr_fifo   = %s", curr_fifo);
	log_msg(3, "next_fifo   = %s", next_fifo);
	if (!does_file_exist(input_fifo)) {
		fatal_error("input fifo does not exist");
	}
	if (!(fin = fopen(input_fifo, "r"))) {
		fatal_error("Unable to openin from input_fifo");
	}
	if (!find_home_of_exe("partimagehack")) {
		fatal_error("partimagehack not found");
	}
	asprintf(&sz_call_to_partimage,
			"%c%cpartimagehack " PARTIMAGE_PARAMS
			" restore %s %s > /dev/null 2>> %s", 2, 0, output_device, curr_fifo,
			MONDO_LOGFILE);
	log_msg(1, "output_device = %s", output_device);
	log_msg(1, "curr_fifo = %s", curr_fifo);
	log_msg(1, "sz_call_to_partimage+2 = %s", sz_call_to_partimage + 2);
	res =
		pthread_create(&partimage_thread, NULL, call_partimage_in_bkgd,
					   (void *) sz_call_to_partimage);
	if (res) {
		fatal_error("Failed to create thread to call partimage");
	}
	log_msg(1, "Running fore/back at same time");
	log_msg(2, " Trying to openin %s", input_fifo);
	if (!does_file_exist(input_fifo)) {
		log_msg(2, "Warning - %s does not exist", input_fifo);
	}
	while (!does_file_exist("/tmp/PARTIMAGEHACK-POSITION")) {
		log_msg(6, "Waiting for partimagehack (restore) to start");
		sleep(1);
	}

	if (!(tmp = malloc(128))) {
		fatal_error("Failed to malloc() tmp");
	}
	while (sz_call_to_partimage[0] > 0) {
		if (fread(tmp, 1, 128, fin) != 128) {
			fatal_error("Cannot read introductory block");
		}
		if (strstr(tmp, NEXT_SUBVOL_PLEASE)) {
			log_msg(2, "Great. Next subvol coming up.");
		} else if (strstr(tmp, NO_MORE_SUBVOLS)) {
			log_msg(2, "Great. That was the last subvol.");
			break;
		} else {
			log_msg(2, "WTF is this? '%s'", tmp);
			fatal_error("Unknown interim block");
		}
		if (feof(fin)) {
			log_msg(1, "Eof(fin) detected. Breaking.");
			break;
		}
		log_msg(3, "Processing subvol %d", fifo_number);
		log_msg(5, "fifo_number=%d", fifo_number);
		if (!(fout = fopen(curr_fifo, "w"))) {
			fatal_error("Cannot openout to curr_fifo");
		}
		copy_from_src_to_dest(fout, fin, 'r');
		paranoid_fclose(fout);
		fifo_number++;
		if (oldest_fifo != NULL) {
			log_msg(6, "Deleting %s", oldest_fifo);
			unlink(oldest_fifo);	// just in case
			paranoid_free(oldest_fifo);
		}
		oldest_fifo = prev_fifo;
		prev_fifo = curr_fifo;
		curr_fifo = next_fifo;
		next_fifo = afternxt_fifo;
		asprintf(&afternxt_fifo, "%s.%03d", tmpstub, fifo_number + 2);
		log_msg(6, "Creating %s", afternxt_fifo);
		mkfifo(afternxt_fifo, S_IRWXU | S_IRWXG);	// make sure _next_ fifo already exists before we access current fifo
		fflush(fin);
//      system("sync");
		usleep(1000L * 100L);
	}
	paranoid_free(tmp);
	paranoid_free(tmpstub);

	paranoid_fclose(fin);
	paranoid_system("sync");
	log_msg(1, "Partimagehack has finished. Great. Fin-closing.");
	log_msg(1, "Waiting for pthread_join");
	pthread_join(partimage_thread, NULL);
	res = sz_call_to_partimage[1];
	paranoid_free(sz_call_to_partimage);

	log_msg(1, "Yay. Partimagehack (restore) returned %d", res);
	unlink(prev_fifo);
	paranoid_free(prev_fifo);

	unlink(curr_fifo);
	paranoid_free(curr_fifo);

	unlink(next_fifo);
	paranoid_free(next_fifo);

	unlink(afternxt_fifo);
	paranoid_free(afternxt_fifo);

	unlink(PIH_LOG);
	/* BERLIOS : pas de unlink(oldest_fifo) ??? */
	paranoid_free(oldest_fifo);

	return (res);
}
