/* 
 * Function to handle mondorescue messages
 *
 * $Id$
 *
 */

#include "my-stuff.h"

extern int g_main_pid;
extern int g_buffer_pid;

/**
 * The default maximum level to log messages at or below.
 */
int g_loglevel = DEFAULT_DEBUG_LEVEL;

/**
 * The standard log_debug_msg() (log_msg() also due to a macro). Writes some describing
 * information to the logfile.
 */
void log_debug_msg(int debug_level, const char *szFile,
							const char *szFunction, int nLine,
							const char *fmt, ...)
{
	va_list args;
	int i;
	static int depth = 0;
	FILE *fout;

	if (depth > 5) {
		depth--;
		return;
	}
	depth++;

	if (debug_level <= g_loglevel) {
		va_start(args, fmt);
		if (!(fout = fopen(MONDO_LOGFILE, "a"))) {
			return;
		}						// fatal_error("Failed to openout to logfile - sheesh..."); }

		// add tabs to distinguish log levels
		if (debug_level > 0) {
			for (i = 1; i < debug_level; i++)
				fprintf(fout, "\t");
			if (getpid() == g_main_pid)
				fprintf(fout, "[Main] %s->%s#%d: ", szFile, szFunction,
						nLine);
			else if (getpid() == g_buffer_pid && g_buffer_pid > 0)
				fprintf(fout, "[Buff] %s->%s#%d: ", szFile, szFunction,
						nLine);
			else
				fprintf(fout, "[TH=%d] %s->%s#%d: ", getpid(), szFile,
						szFunction, nLine);
		}
		vfprintf(fout, fmt, args);

		// do not slow down the progran if standard debug level
		// must be enabled: if no flush, the log won't be up-to-date if there
		// is a segfault
		//if (g_dwDebugLevel != 1)

		va_end(args);
		fprintf(fout, "\n");
		paranoid_fclose(fout);
	}
	depth--;
}
