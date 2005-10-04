/* $Id$ */

/**
 * @file
 * Functions to handle buffering of tape archives as they are read/written.
 * This used the external program @c buffer mostly.
 */

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <pthread.h>

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo.h"

/**
 * @addtogroup globalGroup
 * @{
 */
/**
 * The SIGPIPE handler sets this to TRUE.
 */
bool g_sigpipe = FALSE;

/**
 * PID of the "main" process.
 */
pid_t g_mastermind_pid = 0;

/**
 * Command line with which @c buffer was invoked.
 */
char *g_sz_call_to_buffer;

/**
 * Size of the buffer used with @c buffer.
 */
int g_tape_buffer_size_MB = 0;

/* @} - end of globalGroup */


/**
 * @addtogroup fifoGroup
 * @{
 */
/**
 * Open a pipe to/from @c buffer.
 * If buffer does not work at all, we use `dd'.
 * @param device The device to read from/write to.
 * @param direction @c 'r' (reading) or @c 'w' (writing).
 * @return A file pointer to/from the @c buffer process.
 */
FILE *open_device_via_buffer(char *device, char direction,
							 long internal_tape_block_size)
{
	char *sz_dir;
	char keych;
	char *tmp;
	FILE *fres;
	int bufsize;				// in megabytes
	int res;
	int wise_upper_limit;
	int wise_lower_limit;

	assert_string_is_neither_NULL_nor_zerolength(device);
	assert(direction == 'w' || direction == 'r');
	asprintf(&sz_dir, "%c", direction);
	wise_upper_limit = (am_I_in_disaster_recovery_mode()? 8 : 32);
	wise_lower_limit = 1;		// wise_upper_limit/2 + 1;
	paranoid_system("sync");
	for (bufsize = wise_upper_limit, res = -1;
		 res != 0 && bufsize >= wise_lower_limit; bufsize--) {
		asprintf(&tmp,
				 "dd if=/dev/zero bs=1024 count=16k 2> /dev/null | buffer -o /dev/null -s %ld -m %d%c",
				 internal_tape_block_size, bufsize, 'm');
		res = run_program_and_log_output(tmp, 2);
		paranoid_free(tmp);
	}
	if (!res) {
		bufsize++;
		asprintf(&tmp, "Negotiated max buffer of %d MB ", bufsize);
		log_to_screen(tmp);
		paranoid_free(tmp);
	} else {
		bufsize = 0;
		res = 0;
		log_to_screen
			("Cannot negotiate a buffer of ANY size. Using dd instead.");
	}
	if (direction == 'r') {
		keych = 'i';
	} else {
		keych = 'o';
	}
	if (bufsize) {
		asprintf(&g_sz_call_to_buffer,
				 "buffer -m %d%c -p%d -B -s%ld -%c %s 2>> %s", bufsize,
				 'm', (direction == 'r') ? 20 : 75,
				 internal_tape_block_size, keych, device, MONDO_LOGFILE);
	} else {
		asprintf(&g_sz_call_to_buffer, "dd bs=%ld %cf=%s",
				 internal_tape_block_size, keych, device);
	}
	log_msg(2, "Calling buffer --- command = '%s'", g_sz_call_to_buffer);
	fres = popen(g_sz_call_to_buffer, sz_dir);
	paranoid_free(sz_dir);
	if (fres) {
		log_msg(2, "Successfully opened ('%c') tape device %s", direction,
				device);
	} else {
		log_msg(2, "Failed to open ('%c') tape device %s", direction,
				device);
	}
	sleep(2);
	asprintf(&tmp, "ps wwax | grep \"%s\"", g_sz_call_to_buffer);
	if (run_program_and_log_output(tmp, 2)) {
		log_msg(2, "Warning - I think I failed to open tape, actually.");
	}
	paranoid_free(tmp);
	g_tape_buffer_size_MB = bufsize;
	/* BERLIOS: usless ?
	   strcmp(tmp, g_sz_call_to_buffer);
	   tmp[30] = '\0';
	 */
	asprintf(&tmp, "ps wwax | grep buffer | grep -v grep");
	if (run_program_and_log_output(tmp, 1)) {
		fres = NULL;
		log_to_screen("Failed to open tape streamer. Buffer error.");
	} else {
		log_to_screen("Buffer successfully started.");
	}
	paranoid_free(tmp);
	return (fres);
}


/**
 * Kill @c buffer processes.
 */
void kill_buffer()
{
	char *tmp;
	char *command;

	paranoid_system("sync");
	asprintf(&command,
			 "ps wwax | fgrep \"%s\" | fgrep -v grep | awk '{print $1;}' | grep -v PID | tr -s '\n' ' ' | awk '{ print $1; }'",
			 g_sz_call_to_buffer);
	paranoid_free(g_sz_call_to_buffer);
	log_msg(2, "kill_buffer() --- command = %s", command);

	asprintf(&tmp, "%s",
			 call_program_and_get_last_line_of_output(command));
	paranoid_free(command);

	asprintf(&command, "kill %s", tmp);
	log_msg(2, "kill_buffer() --- command = %s", command);

	if (strlen(tmp) > 0) {
		run_program_and_log_output(command, TRUE);
	}
	paranoid_free(command);
	paranoid_free(tmp);
}


/**
 * Handler for SIGPIPE.
 * @param sig The signal that occurred (hopefully SIGPIPE).
 */
void sigpipe_occurred(int sig)
{
	g_sigpipe = TRUE;
}

/* @} - end of fifoGroup */

/* BERLIOS: useless ?
int 
extract_config_file_from_ramdisk( struct s_bkpinfo *bkpinfo, 
				  char *ramdisk_fname, 
				  char *output_cfg_file, 
				  char *output_mountlist_file);
*/
