/* libmondo-fifo.c
   $Id$


04/17
- replaced INTERNAL_TAPE_BLK_SIZE with bkpinfo->internal_tape_block_size

04/04/2004
- ps wax is now ps wwax

09/10/2003
- p75 is now p80 in outopening tape call
- negotiate largest buffer possible when opening tape drive
- fall back to dd if buffer fails

09/09
- better logging for tape users
- working on try_hard_to_fwrite(), try_hard_to_fread()
- replaced internal w/ EXTERNAL 'buffer' exe.

08/30
- tweaked error msgs in try_hard_to_fwrite()

08/02
- updated is_incoming_block_valid() to make it
  return end-of-tape if >300 flotsam blocks

05/02
- when logging tape errors, don't repeat self

04/24
- added lots of log_OS_error()'s and assert()'s

04/22
- copy_file_from_here_to_there() --- added a bit of fault tolerance;
  if write fails, retry a few times before reporting error

04/07/2003
- line 866 --- set block_size used by internal buffer to 32768;
  was INTERNAL_TAPE_BLK_SIZE/2

10/01 - 11/30/2002
- is_incoming_block_valid() --- always make
  checksums %65536, just in case int size is
  odd (GRRR, ArkLinux)
- disabled rotor-related superfluous warnings
- added INTERNAL_TAPE_BLK_SIZE
- do_sem() now returns int explicitly
- changed internal_block_size
- some irregularities (e.g. bad 'type'-ing) found by Kylix; fixed by Hugo

09/01 - 09/30
- change 64k to TAPE_BLOCK_SIZE
- added internal_block_size; set it to TAPE_BLOCK_SIZE*2
- if data is flowing FROM tape TO hard disk then set the threshold to 10 (not 75)
- lots of multitape-related fixes
- finally caught & fixed the 'won't finish unzipping last bigfile' bug
- added WIFEXITED() after waitpid(), to improve multi-tape support

08/01 - 08/31
- trying to catch & stop 'won't finish unzipping last bigfile' bug by
  changing the copy_file_rom_here_to_there code
- changed % threshold from 95 back to 75
- don't insist on full 256K write of last block to tape
- if >25 secs go by & all data (AFAIK) has been copied thru by FIFO wrapper
  and g_tape_stream is _still_ not closed then shrug shoulders & pthread_exit
  anyway...
- change fprintf()'s to log_it()'s
- added a header+footer to each block as it is read/written to/from tape
  by copy_file_from_here_to_there
- wrote workaround to allow >2GB of archives w/buffering
- changed % threshold from 75 to 95
- added calls to set_signals(); 'buffer' was killing mondoarchive as
  it terminated
- cleaned up struct-passing, to improve reliability and eliminate
  some race conditions
- changed some forks to pthreads
- added some comments
- created libfifo{.c,.h,-EXT.h}
- copied core of 'buffer' here
- added some other, Mondo-specific functions
- hacked 'buffer' into user-friendliness
*/

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
char g_sz_call_to_buffer[MAX_STR_LEN];

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
	char sz_dir[32];
	char keych;
	char *tmp;
	char *command;
	FILE *fres;
	int bufsize;				// in megabytes
	int res;
	int wise_upper_limit;
	int wise_lower_limit;

	malloc_string(tmp);
	malloc_string(command);
	assert_string_is_neither_NULL_nor_zerolength(device);
	assert(direction == 'w' || direction == 'r');
	sprintf(sz_dir, "%c", direction);
	wise_upper_limit = (am_I_in_disaster_recovery_mode()? 8 : 32);
	wise_lower_limit = 1;		// wise_upper_limit/2 + 1;
	paranoid_system("sync");
	for (bufsize = wise_upper_limit, res = -1;
		 res != 0 && bufsize >= wise_lower_limit; bufsize--) {
		sprintf(tmp,
				"dd if=/dev/zero bs=1024 count=16k 2> /dev/null | buffer -o /dev/null -s %ld -m %d%c",
				internal_tape_block_size, bufsize, 'm');
		res = run_program_and_log_output(tmp, 2);
	}
	if (!res) {
		bufsize++;
		sprintf(tmp, _("Negotiated max buffer of %d MB "), bufsize);
		log_to_screen(tmp);
	} else {
		bufsize = 0;
		res = 0;
		log_to_screen
			(_("Cannot negotiate a buffer of ANY size. Using dd instead."));
	}
	if (direction == 'r') {
		keych = 'i';
	} else {
		keych = 'o';
	}
	if (bufsize) {
		sprintf(g_sz_call_to_buffer,
				"buffer -m %d%c -p%d -B -s%ld -%c %s 2>> %s", bufsize, 'm',
				(direction == 'r') ? 20 : 75, internal_tape_block_size,
				keych, device, MONDO_LOGFILE);
	} else {
		sprintf(g_sz_call_to_buffer, "dd bs=%ld %cf=%s",
				internal_tape_block_size, keych, device);
	}
	log_msg(2, "Calling buffer --- command = '%s'", g_sz_call_to_buffer);
	fres = popen(g_sz_call_to_buffer, sz_dir);
	if (fres) {
		log_msg(2, "Successfully opened ('%c') tape device %s", direction,
				device);
	} else {
		log_msg(2, "Failed to open ('%c') tape device %s", direction,
				device);
	}
	sleep(2);
	sprintf(tmp, "ps wwax | grep \"%s\"", g_sz_call_to_buffer);
	if (run_program_and_log_output(tmp, 2)) {
		log_msg(2, "Warning - I think I failed to open tape, actually.");
	}
	g_tape_buffer_size_MB = bufsize;
	strcmp(tmp, g_sz_call_to_buffer);
	tmp[30] = '\0';
	sprintf(command, "ps wwax | grep buffer | grep -v grep");
	if (run_program_and_log_output(command, 1)) {
		fres = NULL;
		log_to_screen(_("Failed to open tape streamer. Buffer error."));
	} else {
		log_to_screen(_("Buffer successfully started."));
	}

	paranoid_free(command);
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

	malloc_string(tmp);
	malloc_string(command);
	paranoid_system("sync");
	sprintf(command,
			"ps wwax | fgrep \"%s\" | fgrep -v grep | awk '{print $1;}' | grep -v PID | tr -s '\n' ' ' | awk '{ print $1; }'",
			g_sz_call_to_buffer);
	log_msg(2, "kill_buffer() --- command = %s", command);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	sprintf(command, "kill %s", tmp);
	log_msg(2, "kill_buffer() --- command = %s", command);
	if (strlen(tmp) > 0) {
		run_program_and_log_output(command, TRUE);
	}
	paranoid_free(tmp);
	paranoid_free(command);
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

int
extract_config_file_from_ramdisk(struct s_bkpinfo *bkpinfo,
								 char *ramdisk_fname,
								 char *output_cfg_file,
								 char *output_mountlist_file);
