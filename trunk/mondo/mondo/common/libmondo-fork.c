/* libmondo-fork.c
   $Id$

- subroutines for handling forking/pthreads/etc.


01/20/2006
- replaced partimagehack with ntfsclone

06/20/2004
- create fifo /var/log/partimagehack-debug.log and empty it
  to keep ramdisk from filling up

04/13/2004
- >= should be <= g_loglevel

11/15/2003
- changed a few []s to char*s
  
10/12
- rewrote partimagehack handling (multiple fifos, chunks, etc.)

10/11
- partimagehack now has debug level of N (set in my-stuff.h)

10/08
- call to partimagehack when restoring will now log errors to /var/log/....log

10/06
- cleaned up logging a bit

09/30
- line 735 - missing char* cmd in sprintf()

09/28
- added run_external_binary_with_percentage_indicator()
- rewritten eval_call_to_make_ISO()

09/18
- call mkstemp instead of mktemp

09/13
- major NTFS hackage

09/12
- paranoid_system("rm -f /tmp/ *PARTIMAGE*") before calling partimagehack

09/11
- forward-ported unbroken feed_*_partimage() subroutines
  from early August 2003

09/08
- detect & use partimagehack if it exists

09/05
- finally finished partimagehack hack :)

07/04
- added subroutines to wrap around partimagehack

04/27
- don't echo (...res=%d...) at end of log_it()
  unnecessarily
- replace newtFinished() and newtInit() with
  newtSuspend() and newtResume()

04/24
- added some assert()'s and log_OS_error()'s

04/09
- cleaned up run_program_and_log_output()

04/07
- cleaned up code a bit
- let run_program_and_log_output() accept -1 (only log if _no error_)

01/02/2003
- in eval_call_to_make_ISO(), append output to MONDO_LOGFILE
  instead of a temporary stderr text file

12/10
- patch by Heiko Schlittermann to handle % chars in issue.net

11/18
- if mkisofs in eval_call_to_make_ISO() returns an error then return it,
  whether ISO was created or not

10/30
- if mkisofs in eval_call_to_make_ISO() returns an error then find out if
  the output (ISO) file has been created; if it has then return 0 anyway

08/01 - 09/30
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_
- system() now includes 2>/dev/null
- enlarged some tmp[]'s
- added run_program_and_log_to_screen() and run_program_and_log_output()

07/24
- created
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

	size_t n = 0;

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
	size_t n = 0;
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
	/* This works on Newt, and it gives quicker updates. */
	for (; does_file_exist(lockfile); sleep(1)) {
		log_file_end_to_screen(MONDO_LOGFILE, "");
		update_evalcall_form(1);
	}
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
 * Apparently used. @bug This has a purpose, but what?
 */
#define PIMP_START_SZ "STARTSTARTSTART9ff3kff9a82gv34r7fghbkaBBC2T231hc81h42vws8"
#define PIMP_END_SZ "ENDENDEND0xBBC10xBBC2T231hc81h42vws89ff3kff9a82gv34r7fghbka"




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
 * Feed @p input_device through ntfsclone to @p output_fname.
 * @param input_device The device to image.
 * @param output_fname The file to write.
 * @return 0 for success, nonzero for failure.
 */
int feed_into_ntfsprog(char *input_device, char *output_fname)
{
// BACKUP
	int res = -1;
	char*command;

	if (!does_file_exist(input_device)) {
		fatal_error("input device does not exist");
	}
	if ( !find_home_of_exe("ntfsclone")) {
		fatal_error("ntfsclone not found");
	}
	malloc_string(command);
	sprintf(command, "ntfsclone --force --save-image --overwrite %s %s", output_fname, input_device);
	res = run_program_and_log_output(command, 5);
	paranoid_free(command);
	unlink(output_fname);
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


/**
 * Feed @p input_fifo through ntfsclone (restore) to @p output_device.
 * @param input_fifo The ntfsclone file to read.
 * @param output_device Where to put the output.
 * @return The return value of ntfsclone (0 for success).
 */
int feed_outfrom_ntfsprog(char *output_device, char *input_fifo)
{
// RESTORE
	int res = -1;
	char *command;

	if ( !find_home_of_exe("ntfsclone")) {
		fatal_error("ntfsclone not found");
	}
	asprintf(&command, "ntfsclone --force --restore-image --overwrite %s %s", output_device, input_fifo);
	res = run_program_and_log_output(command, 5);
	paranoid_free(command);
	return (res);
}
