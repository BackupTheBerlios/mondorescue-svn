/* $Id$

  subroutines which do display-type things
  and use the newt library to do them
*/


/**
 * @file
 * Functions for doing display-type things with the Newt library.
 */

#define MAX_NEWT_COMMENT_LEN 200

#include <unistd.h>

#include "my-stuff.h"
#include "mondostructures.h"
#include "newt-specific.h"
#include "libmondo-string-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-devices-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-fork-EXT.h"
#include "newt-specific-EXT.h"

/*@unused@*/
//static char cvsid[] = "$Id$";

	extern pid_t g_mastermind_pid;
	extern char *g_tmpfs_mountpt;
	extern char *g_mondo_home;

	extern void set_signals(int);

/**
 * @addtogroup globalGroup
 * @{
 */
/**
 * Whether we are currently in a nested call of fatal_error().
 */
	bool g_exiting = FALSE;

	newtComponent g_timeline = NULL,	///< The line of the progress form that shows the time elapsed/remaining
		g_percentline = NULL,	///< The line of the progress form that shows the percent completed/remaining
		g_scale = NULL,			///< The progress bar component in the progress form
		g_progressForm = NULL,	///< The progress form component itself
		g_blurb1 = NULL,		///< The component for line 1 of the blurb in the progress form
		g_blurb2 = NULL,		///< The component for line 2 of the blurb in the progress form
		g_blurb3 = NULL,		///< The component for line 3 (updated continuously) of the blurb in the progress form
		g_label = NULL;			///< ????? @bug ?????

/**
 * Padding above the Newt components, to overcome bugs in Newt.
 */
	char **err_log_lines = NULL,	///< The list of log lines to show on the screen.
		*g_blurb_str_1,			///< The string for line 1 of the blurb in the progress form
		*g_blurb_str_2,			///< The string for line 2 of the blurb in the progress form
		*g_blurb_str_3;			///< The string for line 3 (updated continuously) of the blurb in the progress form
	newtComponent g_isoform_main = NULL,	///< The evalcall form component itself
		g_isoform_header = NULL,	///< The component for the evalcall form title
		g_isoform_scale = NULL,	///< The progress bar component in the evalcall form
		g_isoform_timeline = NULL,	///< The line of the evalcall form that shows the time elapsed/remaining
		g_isoform_pcline = NULL;	///< The line of the evalcall form that shows the percent completed/remaining
	long g_isoform_starttime;	///< The time (in seconds since the epoch) that the evalcall form was opened.
	int g_isoform_old_progress = -1;	///< The most recent progress update of the evalcall form (percent).
	char *g_isoform_header_str;	///< The string for the evalcall form title.
	int g_mysterious_dot_counter;	///< The counter for the twirling baton (/ | \\ - ...) on percentage less than 3
	int g_noof_log_lines = 6;	///< The number of lines to show in the log at the bottom of the screen.
	int g_noof_rows = 25;		///< The number of rows on the screen.

	int g_currentY = 3;			///< The row to write background progress messages to. Incremented each time a message is written.
	extern int g_current_media_number;
	pid_t g_main_pid = 0;		///< The PID of the main Mondo process.
	long g_maximum_progress = 999;	///< The maximum amount of progress (100%) for the currently opened progress form.
	long g_current_progress = -999;	///< The current amount of progress (filelist #, etc.) for the currently opened progress form.
	long g_start_time = 0;		///< The time (in seconds since the epoch) that the progress form was opened.
	bool g_text_mode = TRUE;	///< If FALSE, use a newt interface; if TRUE, use an ugly (but more compatible) dumb terminal interface.
	bool g_called_by_xmondo = FALSE;	///< @bug Unneeded w/current XMondo.
	char *g_erase_tmpdir_and_scratchdir;	///< The command to run to erase the tmpdir and scratchdir at the end of Mondo.
	char *g_selfmounted_isodir;	///< Holds the NFS mountpoint if mounted via mondoarchive.

/* @} - end of globalGroup */

//int g_fd_in=-1, g_fd_out=-1;

	void popup_and_OK(char *);


/**
 * @addtogroup guiGroup
 * @{
 */
/**
 * Ask the user a yes/no question.
 * @param prompt The question to ask the user.
 * @return TRUE for yes; FALSE for no.
 */
	bool ask_me_yes_or_no(char *prompt) {

		/*@ buffers ********************************************************** */
		char *tmp = NULL;
		int i;
		size_t n = 0;

		 assert_string_is_neither_NULL_nor_zerolength(prompt);

		if (g_text_mode) {
			while (1) {
				sync();
				printf
					("---promptdialogYN---1--- %s\r\n---promptdialogYN---Q--- [yes] [no] ---\r\n--> ",
					 prompt);
				(void) getline(&tmp, &n, stdin);
				if (tmp[strlen(tmp) - 1] == '\n')
					tmp[strlen(tmp) - 1] = '\0';

				i = (int) strlen(tmp);
				if (i > 0 && tmp[i - 1] < 32) {
					tmp[i - 1] = '\0';
				}
				if (strstr(_("yesYES"), tmp)) {
					paranoid_free(tmp);
					return (TRUE);
				} else if (strstr(_("NOno"), tmp)) {
					paranoid_free(tmp);
					return (FALSE);
				} else {
					sync();
					printf
						(_("Please enter either YES or NO (or yes or no, or y or n, or...)\n"));
				}
			}
		} else {
			return (popup_with_buttons(prompt, _("Yes"), _("No")));
		}
	}


/**
 * Give the user the opportunity to continue the current operation (OK)
 * or cancel it (Cancel).
 * @param prompt The string to be displayed.
 * @return TRUE for OK, FALSE for Cancel.
 */
	bool ask_me_OK_or_cancel(char *prompt) {

		/*@ buffer *********************************************************** */
		char *tmp = NULL;
		int i;
		size_t n = 0;

		assert_string_is_neither_NULL_nor_zerolength(prompt);
		if (g_text_mode) {
			sync();
			printf
				("---promptdialogOKC---1--- %s\r\n---promptdialogOKC---Q--- [OK] [Cancel] ---\r\n--> ",
				 prompt);
			(void) getline(&tmp, &n, stdin);
			if (tmp[strlen(tmp) - 1] == '\n')
				tmp[strlen(tmp) - 1] = '\0';

			i = (int) strlen(tmp);
			if (i > 0 && tmp[i - 1] < 32) {
				tmp[i - 1] = '\0';
			}
			if (strstr(_("okOKOkYESyes"), tmp)) {
				paranoid_free(tmp);
				return (TRUE);
			} else {
				paranoid_free(tmp);
				return (FALSE);
			}
		} else {
			return (popup_with_buttons(prompt, _(" Okay "), _("Cancel")));
		}
	}


/**
 * Close the currently opened evalcall form.
 */
	void
	 close_evalcall_form(void) {
		if (g_text_mode) {
			return;
		}
		if (g_isoform_main == NULL) {
			return;
		}
		update_evalcall_form(100);
		usleep(500000);
		if (g_text_mode) {
			log_msg(2, "Closing evalcall form");
			return;
		}
		newtPopHelpLine();
		newtFormDestroy(g_isoform_main);
		newtPopWindow();
		g_isoform_main = NULL;
		g_isoform_old_progress = -1;
	}


/**
 * Close the currently opened progress form.
 */
	void
	 close_progress_form() {
		if (g_text_mode) {
			return;
		}
		if (g_current_progress == -999) {
			log_msg(2,
					"Trying to close the progress form when it ain't open!");
			return;
		}
		g_current_progress = g_maximum_progress;
		update_progress_form("Complete");
		sleep(1);
		if (g_text_mode) {
			log_msg(2, "Closing progress form");
			return;
		}
		newtPopHelpLine();
		newtFormDestroy(g_progressForm);
		newtPopWindow();
		g_progressForm = NULL;
		g_current_progress = -999;
	}


/**
 * Exit Mondo with a fatal error.
 * @param error_string The error message to present to the user before exiting.
 * @note This function never returns.
 */
	void
	 fatal_error(char *error_string) {
		/*@ buffers ***************************************************** */
		char *fatalstr;
		char *tmp;
		char *command;
		static bool already_exiting = FALSE;
		int i;

		/*@ end vars **************************************************** */

		asprintf(&fatalstr, "-------FATAL ERROR---------");
		set_signals(FALSE);		// link to external func
		g_exiting = TRUE;
		log_msg(1, "%s - '%s'", fatalstr, error_string);
		printf("%s - %s\n", fatalstr, error_string);
		if (getpid() == g_mastermind_pid) {
			log_msg(2, "mastermind %d is exiting", (int) getpid());
			kill(g_main_pid, SIGTERM);
			finish(1);
		}

		if (getpid() != g_main_pid) {
			if (g_mastermind_pid != 0 && getpid() != g_mastermind_pid) {
				log_msg(2, "non-m/m %d is exiting", (int) getpid());
				kill(g_main_pid, SIGTERM);
				finish(1);
			}
		}

		log_msg(3, "OK, I think I'm the main PID.");
		if (already_exiting) {
			log_msg(3, "...I'm already exiting. Give me time, Julian!");
			finish(1);
		}

		already_exiting = TRUE;
		log_msg(2, "I'm going to do some cleaning up now.");
		paranoid_system("killall mindi 2> /dev/null");
		kill_anything_like_this("/mondo/do-not");
		kill_anything_like_this("mondo.tmp");
		kill_anything_like_this("ntfsclone");
		sync();
		asprintf(&tmp, "umount %s", g_tmpfs_mountpt);
		chdir("/");
		for (i = 0; i < 10 && run_program_and_log_output(tmp, 5); i++) {
			log_msg(2, "Waiting for child processes to terminate");
			sleep(1);
			run_program_and_log_output(tmp, 5);
		}
		paranoid_free(tmp);

		if (g_erase_tmpdir_and_scratchdir) {
			run_program_and_log_output(g_erase_tmpdir_and_scratchdir, 5);
		}

		if (g_selfmounted_isodir) {
			asprintf(&command, "umount %s", g_selfmounted_isodir);
			run_program_and_log_output(command, 5);
			asprintf(&command, "rmdir %s", g_selfmounted_isodir);
			run_program_and_log_output(command, 5);
		}

		if (!g_text_mode) {
			log_msg(0, fatalstr);
			log_msg(0, error_string);
			//      popup_and_OK (error_string);
			newtFinished();
		}

		system
			("gzip -9c "MONDO_LOGFILE" > /tmp/MA.log.gz 2> /dev/null");
		printf
				(_("If you require technical support, please contact the mailing list.\n"));
		printf(_("See http://www.mondorescue.org for details.\n"));
		printf
				(_("The list's members can help you, if you attach that file to your e-mail.\n"));
		printf(_("Log file: %s\n"), MONDO_LOGFILE);
		if (does_file_exist("/tmp/MA.log.gz")) {
			printf
				(_("FYI, I have gzipped the log and saved it to /tmp/MA.log.gz\n"));
		}
		printf(_("Mondo has aborted.\n"));
		register_pid(0, "mondo");	// finish() does this too, FYI
		if (!g_main_pid) {
			log_msg(3, "FYI - g_main_pid is blank");
		}
		finish(254);
	}



/**
 * Exit Mondo normally.
 * @param signal The exit code (0 indicates a successful backup; 1 for Mondo means the
 * user aborted; 254 means a fatal error occured).
 * @note This function never returns.
 */
	void
	 finish(int signal) {
		char *command = NULL;

		/*  if (signal==0) { popup_and_OK("Please press <enter> to quit."); } */

		/* newtPopHelpLine(); */

		register_pid(0, "mondo");
		chdir("/");
		run_program_and_log_output("umount " MNT_CDROM, FALSE);
		run_program_and_log_output("rm -Rf /mondo.scratch.* /mondo.tmp.*",
								   FALSE);
		if (g_erase_tmpdir_and_scratchdir) {
			run_program_and_log_output(g_erase_tmpdir_and_scratchdir, 1);
		}
		if (g_selfmounted_isodir) {
			asprintf(&command, "umount %s", g_selfmounted_isodir);
			run_program_and_log_output(command, 1);
			asprintf(&command, "rmdir %s", g_selfmounted_isodir);
			run_program_and_log_output(command, 1);
		}
//  iamhere("foo");
		/* system("clear"); */
//  iamhere("About to call newtFinished");
		if (!g_text_mode) {
			if (does_file_exist("/THIS-IS-A-RAMDISK")) {
				log_msg(1, "Calling newtFinished()");
				newtFinished();
			} else {
				log_msg(1, "Calling newtSuspend()");
				newtSuspend();
			}
		}
//  system("clear");
//  iamhere("Finished calling newtFinished");
		printf(_("Execution run ended; result=%d\n"), signal);
		printf(_("Type 'less %s' to see the output log\n"), MONDO_LOGFILE);
		free_libmondo_global_strings();
		exit(signal);
	}





/**
 * Log the last @p g_noof_log_lines lines of @p filename that match @p
 * grep_for_me to the screen.
 * @param filename The file to give the end of.
 * @param grep_for_me If not "", then only give lines in @p filename that match this regular expression.
 */
	void
	 log_file_end_to_screen(char *filename, char *grep_for_me) {

		/*@ buffers ********************************************************** */
		char *command = NULL;
		char *tmp = NULL;

		/*@ pointers ********************************************************* */
		FILE *fin = NULL;

		/*@ int ************************************************************** */
		int i = 0;
		size_t n = 0;

		assert_string_is_neither_NULL_nor_zerolength(filename);
		assert(grep_for_me != NULL);

		if (!does_file_exist(filename)) {
			return;
		}
		if (grep_for_me[0] != '\0') {
			asprintf(&command, "grep '%s' %s | tail -n%d",
					 grep_for_me, filename, g_noof_log_lines);
		} else {
			asprintf(&command, "tail -n%d %s", g_noof_log_lines, filename);
		}
		fin = popen(command, "r");
		if (!fin) {
			log_OS_error(command);
		} else {
			for (i = 0; i < g_noof_log_lines; i++) {
				for (;
					strlen(err_log_lines[i]) < 2 && !feof(fin);) {
					getline(&(err_log_lines[i]), &n, fin);
					strip_spaces(err_log_lines[i]);
					if (!strncmp(err_log_lines[i], "root:", 5)) {
						asprintf(&tmp, "%s", err_log_lines[i] + 6);
						paranoid_free(err_log_lines[i]);
						err_log_lines[i] = tmp;
					}
					if (feof(fin)) {
						break;
					}
				}
			}
			paranoid_pclose(fin);
		}
		refresh_log_screen();
		paranoid_free(command);
	}


/**
 * Log a message to the screen.
 * @param fmt A printf-style format string to write. The following parameters are its arguments.
 * @note The message is also written to the logfile.
 */
	void
	 log_to_screen(const char *fmt, ...) {

		/*@ int ************************************************************** */
		int i = 0;
		int j = 0;
		va_list args;

		/*@ buffers ********************************************************** */
		char *output = NULL;


		va_start(args, fmt);
		vasprintf(&output, fmt, args);
		log_msg(0, output);
		if (strlen(output) > 80) {
			output[80] = '\0';
		}
		va_end(args);
		i = (int) strlen(output);
		if (i > 0 && output[i - 1] < 32) {
			output[i - 1] = '\0';
		}

		if (err_log_lines) {
			paranoid_free(&err_log_lines[0]);
			for (i = 1; i < g_noof_log_lines; i++) {
				err_log_lines[i - 1] = err_log_lines[i];
			}
		}
		while (strlen(output) > 0 && output[strlen(output) - 1] < 32) {
			output[strlen(output) - 1] = '\0';
		}
		for (j = 0; j < (int) strlen(output); j++) {
			if (output[j] < 32) {
				output[j] = ' ';
			}
		}
		if (err_log_lines)
			err_log_lines[g_noof_log_lines - 1] = output;
		if (g_text_mode) {
			printf("%s\n", output);
		} else {
			refresh_log_screen();
		}
	}


/**
 * Write a string to the root window at (@p x, @p y) and also to the logfile.
 * @param y The row to write the string to.
 * @param x The column to write the string to.
 * @param output The string to write.
 */
	void
	 mvaddstr_and_log_it(int y, int x, char *output) {
		assert_string_is_neither_NULL_nor_zerolength(output);
		log_msg(0, output);
		if (g_text_mode) {
			printf("%s\n", output);
		} else {
			newtDrawRootText(x, y, output);
			newtRefresh();
		}
	}




/**
 * Open an evalcall form with title @p ttl.
 * @param ttl The title to use for the evalcall form.
 */
	void
	 open_evalcall_form(char *ttl) {

		/*@ buffers ********************************************************* */
		char *title;
		char *tmp;

		/*@ initialize ****************************************************** */
		g_isoform_old_progress = -1;
		g_mysterious_dot_counter = 0;

		assert(ttl != NULL);
		asprintf(&title, ttl);
		// BERLIOS: We need to unallocate it somewhere
		asprintf(&g_isoform_header_str, title);
		//  center_string (title, 80);
		if (g_text_mode) {
			log_msg(0, title);
		} else {
			asprintf(&tmp, title);
			/* BERLIOS: center_string is now broken replace it ! */
			//center_string(tmp, 80);
			newtPushHelpLine(tmp);
			paranoid_free(tmp);
		}
		/* BERLIOS: center_string is now broken replace it ! */
		//center_string(g_isoform_header_str, 36);
		g_isoform_starttime = get_time();
		if (g_text_mode) {
			log_msg(0, g_isoform_header_str);
		} else {
			g_isoform_header = newtLabel(1, 1, g_isoform_header_str);
			g_isoform_scale = newtScale(3, 3, 34, 100);
			//      newtOpenWindow (20, 6, 40, 7, title);      // "Please Wait");
			newtCenteredWindow(40, 7, title);
			g_isoform_main = newtForm(NULL, NULL, 0);
			g_isoform_timeline = newtLabel(1, 5, "This is the timeline");
			g_isoform_pcline = newtLabel(1, 6, "This is the pcline");
			newtFormAddComponents(g_isoform_main, g_isoform_timeline,
								  g_isoform_pcline, g_isoform_header,
								  g_isoform_scale, NULL);
			newtRefresh();
		}
		update_evalcall_form(0);
		paranoid_free(title);
	}



/**
 * Open a progress form with title @p title.
 * @param title The title to use for the progress form (will be put in the title bar on Newt).
 * @param b1 The first line of the blurb; generally static.
 * @param b2 The second line of the blurb; generally static.
 * @param b3 The third line of the blurb; generally dynamic (it is passed
 * to update_evalcall_form() every time).
 * @param max_val The maximum amount of progress (number of filesets, etc.)
 */
	void
	 open_progress_form(char *title, char *b1, char *b2, char *b3,
						long max_val) {

		/*@ buffers ********************************************************* */
		char *b1c;
		char *blurb1;
		char *blurb2;
		char *blurb3;

		/*@ initialize ****************************************************** */
		g_mysterious_dot_counter = 0;

		assert(title != NULL);
		assert(b1 != NULL);
		assert(b2 != NULL);
		assert(b3 != NULL);

		asprintf(&blurb1, b1);
		asprintf(&blurb2, b2);
		asprintf(&blurb3, b3);
		asprintf(&b1c, b1);
		/* BERLIOS: center_string is now broken replace it ! */
		//center_string(b1c, 80);
		if (max_val <= 0) {
			max_val = 1;
		}

		g_start_time = get_time();
		g_maximum_progress = max_val;
		g_current_progress = 0;
		// BERLIOS: We need to unallocate them
		asprintf(&g_blurb_str_1, blurb1);
		asprintf(&g_blurb_str_2, blurb3);
		asprintf(&g_blurb_str_3, blurb2);
		if (g_text_mode) {
			log_msg(0, blurb1);
			log_msg(0, blurb2);
			log_msg(0, blurb3);
		} else {
			g_blurb1 = newtLabel(2, 1, blurb1);
			g_blurb2 = newtLabel(2, 2, blurb3);
			g_blurb3 = newtLabel(2, 4, blurb2);
			//      newtOpenWindow (10, 4, 60, 11, title);
			newtCenteredWindow(60, 11, title);
			g_scale = newtScale(3, 6, 54, g_maximum_progress);
			g_progressForm = newtForm(NULL, NULL, 0);
			g_percentline = newtLabel(10, 9, "This is the percentline");
			g_timeline = newtLabel(10, 8, "This is the timeline");
			newtFormAddComponents(g_progressForm, g_percentline,
								  g_timeline, g_scale, g_blurb1, g_blurb3,
								  g_blurb2, NULL);
			newtPushHelpLine(b1c);
			newtRefresh();
		}
		update_progress_form_full(blurb1, blurb2, blurb3);
		paranoid_free(b1c);
		paranoid_free(blurb1);
		paranoid_free(blurb2);
		paranoid_free(blurb3);
	}

/**
 * Give a message to the user in the form of a dialog box (under Newt).
 * @param prompt The message.
 */
	void
	 popup_and_OK(char *prompt) {
		char ch;

		assert_string_is_neither_NULL_nor_zerolength(prompt);

		log_msg(0, prompt);
		if (g_text_mode) {
			printf
				("---promptpopup---1--- %s\r\n---promptpopup---Q--- [OK] ---\r\n--> ",
				 prompt);
			while (((ch = getchar()) != '\n') && (ch != EOF));
		} else {
			(void) popup_with_buttons(prompt, _(" OK "), "");
		}
	}

/**
 * Ask the user to enter a value.
 * @param title The title of the dialog box.
 * @param b The blurb (e.g. what you want the user to enter).
 * @param output The string to put the user's answer in. It has to be freed by the caller
 * @return TRUE if the user pressed OK, FALSE if they pressed Cancel.
 */
	bool popup_and_get_string(char *title, char *b, char *output) {

		/*@ newt ************************************************************ */
		newtComponent myForm;
		newtComponent b_1;
		newtComponent b_2;
		newtComponent b_res;
		newtComponent text;
		newtComponent type_here;

		/*@ pointers ********************************************************* */
		char *entry_value = NULL;

		/*@ buffers ********************************************************** */
		char *blurb = NULL;
		size_t n = 0;
		bool ret = TRUE;

		assert_string_is_neither_NULL_nor_zerolength(title);
		assert(b != NULL);

		if (g_text_mode) {
			printf
				("---promptstring---1--- %s\r\n---promptstring---2--- %s\r\n---promptstring---Q---\r\n-->  ",
				 title, b);
			paranoid_free(output);
			(void) getline(&output, &n, stdin);
			if (output[strlen(output) - 1] == '\n')
				output[strlen(output) - 1] = '\0';
			return (ret);
		}
		asprintf(&blurb, b);
		text = newtTextboxReflowed(2, 1, blurb, 48, 5, 5, 0);

		type_here =
			newtEntry(2, newtTextboxGetNumLines(text) + 2,
					  output, 50,
					  &entry_value, NEWT_FLAG_RETURNEXIT
			);
		b_1 = newtButton(6, newtTextboxGetNumLines(text) + 4, _("  OK  "));
		b_2 = newtButton(18, newtTextboxGetNumLines(text) + 4, _("Cancel"));
		newtCenteredWindow(54, newtTextboxGetNumLines(text) + 9, title);
		myForm = newtForm(NULL, NULL, 0);
		newtFormAddComponents(myForm, text, type_here, b_1, b_2, NULL);
		/* BERLIOS: center_string is now broken replace it ! */
		//center_string(blurb, 80);
		newtPushHelpLine(blurb);
		paranoid_free(blurb);

		b_res = newtRunForm(myForm);
		newtPopHelpLine();
		if (b_res == b_2) {
			ret = FALSE;
		} else {
			// Copy entry_value before destroying the form
			// clearing potentially output before
			paranoid_alloc(output,entry_value);
		}
		newtFormDestroy(myForm);
		newtPopWindow();
		return(ret);
	}


/**
 * Pop up a dialog box with user-defined buttons.
 * @param p The text to put in the dialog box.
 * @param button1 The label on the first button.
 * @param button2 The label on the second button, or "" if you only want one button.
 * @return TRUE if @p button1 was pushed, FALSE otherwise.
 */
	bool popup_with_buttons(char *p, char *button1, char *button2) {

		/*@ buffers *********************************************************** */
		char *prompt;
		char *tmp = NULL;
		size_t n = 0;

		/*@ newt ************************************************************** */
		newtComponent myForm;
		newtComponent b_1;
		newtComponent b_2;
		newtComponent b_res;
		newtComponent text;

		assert_string_is_neither_NULL_nor_zerolength(p);
		assert(button1 != NULL);
		assert(button2 != NULL);
		if (g_text_mode) {
			if (strlen(button2) == 0) {
				printf("%s (%s) --> ", p, button1);
			} else {
				printf("%s (%s or %s) --> ", p, button1, button2);
			}
			for (asprintf(&tmp, " ");
				 strcmp(tmp, button1) && (strlen(button2) == 0
										  || strcmp(tmp, button2));) {
				printf("--> ");
				paranoid_free(tmp);
				(void) getline(&tmp, &n, stdin);
			}
			if (!strcmp(tmp, button1)) {
				paranoid_free(tmp);
				return (TRUE);
			} else {
				paranoid_free(tmp);
				return (FALSE);
			}
		}

		asprintf(&prompt, p);
		text = newtTextboxReflowed(1, 1, prompt, 40, 5, 5, 0);
		b_1 =
			newtButton(20 -
					   ((button2[0] !=
						 '\0') ? strlen(button1) +
						2 : strlen(button1) / 2),
					   newtTextboxGetNumLines(text) + 3, button1);
		if (button2[0] != '\0') {
			b_2 =
				newtButton(24, newtTextboxGetNumLines(text) + 3, button2);
		} else {
			b_2 = NULL;
		}
		//  newtOpenWindow (25, 5, 46, newtTextboxGetNumLines (text) + 7, "Alert");
		newtCenteredWindow(46, newtTextboxGetNumLines(text) + 7, _("Alert"));
		myForm = newtForm(NULL, NULL, 0);
		newtFormAddComponents(myForm, text, b_1, b_2, NULL);
		/* BERLIOS: center_string is now broken replace it ! */
		//center_string(prompt, 80);
		newtPushHelpLine(prompt);
		paranoid_free(prompt);
		b_res = newtRunForm(myForm);
		newtPopHelpLine();
		newtFormDestroy(myForm);
		newtPopWindow();
		if (b_res == b_1) {
			return (TRUE);
		} else {
			return (FALSE);
		}
	}




/**
 * Synchronize the log messages stored in @p err_log_lines with those shown
 * on the screen.
 */
	void
	 refresh_log_screen() {

		/*@ int *********************************************************** */
		int i = 0;


		if (g_text_mode || !err_log_lines) {
			return;
		}
		for (i = g_noof_log_lines - 1; i >= 0; i--) {
			newtDrawRootText(0, i + g_noof_rows - 1 - g_noof_log_lines,
							 "                                                                                ");
		}
		newtRefresh();
		for (i = g_noof_log_lines - 1; i >= 0; i--) {
			//BERLIOS : removed for now, Think it's useless : err_log_lines[i][79] = '\0';
			newtDrawRootText(0, i + g_noof_rows - 1 - g_noof_log_lines,
							 err_log_lines[i]);
		}
		newtRefresh();
	}


/**
 * Set up the Newt graphical environment. If @p g_text_mode is TRUE, then
 * only allocate some memory.
 */
	void
	 setup_newt_stuff() {

		/*@ int *********************************************************** */
		int i = 0;
		int cols;

		if (!g_text_mode) {
			newtInit();
			newtCls();
			newtPushHelpLine
				(_("Welcome to Mondo Rescue, by Dev Team and the Internet. All rights reversed."));
			/*  newtDrawRootText(28,0,"Welcome to Mondo Rescue"); */
			newtDrawRootText(18, 0, WELCOME_STRING);
			newtRefresh();
			newtGetScreenSize(&cols, &g_noof_rows);
			g_noof_log_lines = (g_noof_rows / 5) + 1;
		}

		err_log_lines =
			(char **) malloc(sizeof(char *) * g_noof_log_lines);
		if (!err_log_lines) {
			fatal_error("Out of memory");
		}

		for (i = 0; i < g_noof_log_lines; i++) {
			err_log_lines[i] = NULL;
		}
	}


/**
 * Update the evalcall form to show <tt>num</tt> %.
 * @param num The numerator of the ratio.
 */
	void
	 update_evalcall_form(int num) {

		/*@ long ************************************************************ */
		long current_time = 0;
		long time_taken = 0;
		long time_total_est = 0;
		long time_remaining = 0;

		/*@ buffers ********************************************************** */
		char *timeline_str;
		char *pcline_str;
		char *taskprogress;
		char *tmp1;
		char *tmp2;
		char *p;

		/*@ int ************************************************************** */
		int percentage = 0;
		int i = 0;
		int j = 0;

		//log_it("update_eval_call_form called");
		if (num < 1) {
			percentage = 1;
		} else {
			percentage = (int) trunc(num);
		}

		current_time = get_time();
		time_taken = current_time - g_isoform_starttime;
		if (num) {
			time_total_est = time_taken * 100 / num;
			time_remaining = time_total_est - time_taken;
		} else {
			time_remaining = 0;
		}
		if (!g_text_mode) {
			newtLabelSetText(g_isoform_header, g_isoform_header_str);
		}
		/* BERLIOS: 27 should be a parameter */
		g_mysterious_dot_counter = (g_mysterious_dot_counter + 1) % 27;
		if ((percentage < 3 && g_isoform_old_progress < 3)
			|| percentage > g_isoform_old_progress) {
			g_isoform_old_progress = percentage;
			asprintf(&timeline_str,
					 _("%2ld:%02ld taken            %2ld:%02ld remaining"),
					 time_taken / 60, time_taken % 60, time_remaining / 60,
					 time_remaining % 60);
			if (percentage < 3) {
				tmp1 =
					(char *) malloc(g_mysterious_dot_counter *
									sizeof(char));
				for (i = 0, p = tmp1; i < g_mysterious_dot_counter - 1;
					 i++, p++) {
					*p = '.';
				}
				*p = '\0';

				/* BERLIOS: 27 should be a parameter */
				tmp2 =
					(char *) malloc(27 -
									g_mysterious_dot_counter *
									sizeof(char));
				for (i = 0, p = tmp2;
					 i < 27 - g_mysterious_dot_counter - 1; i++, p++) {
					*p = ' ';
				}
				*p = '\0';

				asprintf(&pcline_str, " Working%s%s %c", tmp1, tmp2,
						 special_dot_char(g_mysterious_dot_counter));
				paranoid_free(tmp1);
				paranoid_free(tmp2);
			} else {
				asprintf(&pcline_str,
						 _(" %3d%% done              %3d%% to go"),
						 percentage, 100 - percentage);
			}
			if (g_text_mode) {
				j = trunc(percentage / 5);
				tmp1 = (char *) malloc((j + 1) * sizeof(char));
				for (i = 0, p = tmp1; i < j; i++, p++) {
					*p = '*';
				}
				*p = '\0';

				tmp2 = (char *) malloc((20 - j + 1) * sizeof(char));
				for (i = 0, p = tmp2; i < 20 - j; i++, p++) {
					*p = '.';
				}
				*p = '\0';

				if (percentage >= 3) {
					asprintf(&taskprogress,
							 "TASK:  [%s%s] %3d%% done; %2ld:%02ld to go",
							 tmp1, tmp2, percentage, time_remaining / 60,
							 time_remaining % 60);
					printf("---evalcall---1--- %s\r\n",
						   g_isoform_header_str);
					printf("---evalcall---2--- %s\r\n", taskprogress);
					printf("---evalcall---E---\r\n");
					paranoid_free(taskprogress);
				}
			} else {
				newtScaleSet(g_isoform_scale,
							 (unsigned long long) percentage);
				newtLabelSetText(g_isoform_pcline, pcline_str);
				if (percentage >= 3) {
					newtLabelSetText(g_isoform_timeline, timeline_str);
				}
			}
			paranoid_free(timeline_str);
			paranoid_free(pcline_str);
		}
		if (!g_text_mode) {
//      log_it("refreshing");
			newtRefresh();
		}
	}


/**
 * Update the progress form to show @p blurb3 and the current value of
 * @p g_maximum_progress.
 * @param blurb3 The new third line of the blurb; use @p g_blurb_str_2 (no, that's not a typo) to keep it the same.
 */
	void
	 update_progress_form(char *blurb3) {
		/*  log_it("update_progress_form --- called"); */
		if (g_current_progress == -999) {
			/* log_it("You're trying to update progress form when it ain't open. Aww, that's OK. I'll let it go. It's a bit naughty but it's a nonfatal error. No prob, Bob."); */
			return;
		}
		paranoid_free(g_blurb_str_2);
		asprintf(&g_blurb_str_2, blurb3);
		update_progress_form_full(g_blurb_str_1, g_blurb_str_2,
								  g_blurb_str_3);
	}


/**
 * Update the progress form's complete blurb and show @p g_current_progress.
 * @param blurb1 The first line of the blurb. Use @p g_blurb_str_1 to keep it unchanged.
 * @param blurb2 The second line of the blurb. Use @p g_blurb_str_3 (no, that's not a typo) to keep it the same.
 * @param blurb3 The third line of the blurb. Use @p g_blurb_str_2 (no, that's not a typo either) to keep it the same.
 */
	void
	 update_progress_form_full(char *blurb1, char *blurb2, char *blurb3) {
		/*@ long ***************************************************** */
		long current_time = 0;
		long time_taken = 0;
		long time_remaining = 0;
		long time_total_est = 0;

		/*@ int ******************************************************* */
		int percentage = 0;
		int i = 0;
		int j = 0;

		/*@ buffers *************************************************** */
		char *percentline_str;
		char *timeline_str;
		char *taskprogress;
		char *tmp;
		char *tmp1;
		char *tmp2;
		char *p;

//  log_msg(1, "'%s' '%s' '%s'", blurb1, blurb2, blurb3);
		if (!g_text_mode) {
			assert(blurb1 != NULL);
			assert(blurb2 != NULL);
			assert(blurb3 != NULL);
			assert(g_timeline != NULL);
		}

		current_time = get_time();
		time_taken = current_time - g_start_time;
		if (g_maximum_progress == 0) {
			percentage = 0;
		} else {
			if (g_current_progress > g_maximum_progress) {
				asprintf(&tmp,
						 "update_progress_form_full(%s,%s,%s) --- g_current_progress=%ld; g_maximum_progress=%ld",
						 blurb1, blurb2, blurb3, g_current_progress,
						 g_maximum_progress);
				log_msg(0, tmp);
				paranoid_free(tmp);
				g_current_progress = g_maximum_progress;
			}
			percentage =
				(int) ((g_current_progress * 100L) / g_maximum_progress);
		}
		if (percentage < 1) {
			percentage = 1;
		}
		if (percentage > 100) {
			percentage = 100;
		}
		if (g_current_progress) {
			time_total_est =
				time_taken * (long) g_maximum_progress /
				(long) (g_current_progress);
			time_remaining = time_total_est - time_taken;
		} else {
			time_remaining = 0;
		}
		/* BERLIOS/ Is it useful here ? */
		//g_mysterious_dot_counter = (g_mysterious_dot_counter + 1) % 27;
		asprintf(&timeline_str,
				 "%2ld:%02ld taken               %2ld:%02ld remaining  ",
				 time_taken / 60, time_taken % 60, time_remaining / 60,
				 time_remaining % 60);
		asprintf(&percentline_str,
				 " %3d%% done                 %3d%% to go", percentage,
				 100 - percentage);

		if (g_text_mode) {
			printf(_("---progress-form---1--- %s%s"), blurb1, "\r\n");
			printf(_("---progress-form---2--- %s%s"), blurb2, "\r\n");
			printf(_("---progress-form---3--- %s%s"), blurb3, "\r\n");
			printf(_("---progress-form---E---\n"));

			j = trunc(percentage / 5);
			tmp1 = (char *) malloc((j + 1) * sizeof(char));
			for (i = 0, p = tmp1; i < j; i++, p++) {
				*p = '*';
			}
			*p = '\0';

			tmp2 = (char *) malloc((20 - j + 1) * sizeof(char));
			for (i = 0, p = tmp2; i < 20 - j; i++, p++) {
				*p = '.';
			}
			*p = '\0';

			if (percentage > 100) {
				log_msg(2, _("percentage = %d"), percentage);
			}
			asprintf(&taskprogress,
					 _("TASK:  [%s%s] %3d%% done; %2ld:%02ld to go"), tmp1,
					 tmp2, percentage, time_remaining / 60,
					 time_remaining % 60);

			printf(_("---progress-form---4--- %s\r\n"), taskprogress);
			paranoid_free(taskprogress);
		} else {
			/* BERLIOS: center_string is now broken replace it ! */
			//center_string(blurb1, 54);
			/* BERLIOS: center_string is now broken replace it ! */
			//center_string(blurb2, 54);
			/* BERLIOS: center_string is now broken replace it ! */
			//center_string(blurb3, 54);
			newtLabelSetText(g_blurb1, blurb1);
			newtLabelSetText(g_blurb2, blurb3);
			newtLabelSetText(g_blurb3, blurb2);
			newtScaleSet(g_scale, (unsigned long long) g_current_progress);
			if (percentage >= 2) {
				newtLabelSetText(g_timeline, timeline_str);
			}
			newtLabelSetText(g_percentline, percentline_str);
			newtRefresh();
		}
		paranoid_free(percentline_str);
		paranoid_free(timeline_str);
	}


/**
 * Ask the user which backup media type they would like to use.
 * The choices are @p none (exit to shell), @c cdr, @c cdrw, @c dvd,
 * @c tape, @c cdstream, @c udev (only when @p g_text_mode is TRUE), @c nfs,
 * and @c iso.
 * @param restoring TRUE if we're restoring, FALSE if we're backing up.
 * @return The backup type chosen, or @c none if the user chose "Exit to shell".
 */
	t_bkptype which_backup_media_type(bool restoring) {

		/*@ char ************************************************************ */
		t_bkptype output;


		/*@ newt ************************************************************ */
		char *title_sz;
		char *minimsg_sz;
		static t_bkptype possible_bkptypes[] =
			{ none, cdr, cdrw, dvd, tape, cdstream, udev, nfs, iso };
		static char *possible_responses[] =
			{ "none", "cdr", "cdrw", "dvd", "tape", "cdstream", "udev",
			"nfs", "iso", NULL
		};
		char *outstr = NULL;
		t_bkptype backup_type;
		int i;
		size_t n = 0;

		newtComponent b1;
		newtComponent b2;
		newtComponent b3;
		newtComponent b4;
		newtComponent b5;
		newtComponent b6;
		newtComponent b7;
		newtComponent b8;
		newtComponent b_res;
		newtComponent myForm;

		if (g_text_mode) {
			for (backup_type = none; backup_type == none;) {
				printf(_("Backup type ("));
				for (i = 0; possible_responses[i]; i++) {
					printf("%c%s", (i == 0) ? '\0' : ' ',
						   possible_responses[i]);
				}
				printf(")\n--> ");
				(void) getline(&outstr, &n, stdin);
				strip_spaces(outstr);
				for (i = 0; possible_responses[i]; i++) {
					if (!strcmp(possible_responses[i], outstr)) {
						backup_type = possible_bkptypes[i];
					}
				}
			}
			paranoid_free(outstr);
			return (backup_type);
		}
		newtDrawRootText(18, 0, WELCOME_STRING);
		if (restoring) {
			asprintf(&title_sz,
					 _("Please choose the backup media from which you want to read data."));
			asprintf(&minimsg_sz, _("Read from:"));
		} else {
			asprintf(&title_sz,
					 _("Please choose the backup media to which you want to archive data."));
			asprintf(&minimsg_sz, _("Backup to:"));
		}
		newtPushHelpLine(title_sz);
		paranoid_free(title_sz);

		//  newtOpenWindow (23, 3, 34, 17, minimsg_sz);
		newtCenteredWindow(34, 17, minimsg_sz);
		paranoid_free(minimsg_sz);

		b1 = newtButton(1, 1, _("CD-R disks "));
		b2 = newtButton(17, 1, _("CD-RW disks"));
		b3 = newtButton(1, 9, _("Tape drive "));
		b4 = newtButton(17, 5, _("CD streamer"));
		b5 = newtButton(1, 5, _(" DVD disks "));
		b6 = newtButton(17, 9, _(" NFS mount "));
		b7 = newtButton(1, 13, _(" Hard disk "));
		b8 = newtButton(17, 13, _("    Exit   "));
		myForm = newtForm(NULL, NULL, 0);
		newtFormAddComponents(myForm, b1, b5, b3, b7, b2, b4, b6, b8,
							  NULL);
		b_res = newtRunForm(myForm);
		newtFormDestroy(myForm);
		newtPopWindow();
		if (b_res == b1) {
			output = cdr;
		} else if (b_res == b2) {
			output = cdrw;
		} else if (b_res == b3) {
			output = tape;
		} else if (b_res == b4) {
			output = cdstream;
		} else if (b_res == b5) {
			output = dvd;
		} else if (b_res == b6) {
			output = nfs;
		} else if (b_res == b7) {
			output = iso;
		} else {
			output = none;
		}
		newtPopHelpLine();
		return (output);
	}


/**
 * Ask the user how much compression they would like to use.
 * The choices are "None" (0), "Minimum" (1), "Average" (4), and "Maximum" (9).
 * @return The compression level (0-9) chosen, or -1 for "Exit".
 */
	int
	 which_compression_level() {

		/*@ char ************************************************************ */
		int output = none;


		/*@ newt ************************************************************ */

		newtComponent b1;
		newtComponent b2;
		newtComponent b3;
		newtComponent b4;
		newtComponent b5;
		newtComponent b_res;
		newtComponent myForm;

		newtDrawRootText(18, 0, WELCOME_STRING);
		newtPushHelpLine
			(_("   Please specify the level of compression that you want."));
		//  newtOpenWindow (23, 3, 34, 13, "How much compression?");
		newtCenteredWindow(34, 13, _("How much compression?"));
		b1 = newtButton(4, 1, _("Maximum"));
		b2 = newtButton(18, 1, _("Average"));
		b3 = newtButton(4, 5, _("Minimum"));
		b4 = newtButton(18, 5, _(" None  "));
		b5 = newtButton(4, 9, _("         Exit        "));
		myForm = newtForm(NULL, NULL, 0);
		newtFormAddComponents(myForm, b1, b3, b2, b4, b5, NULL);
		b_res = newtRunForm(myForm);
		newtFormDestroy(myForm);
		newtPopWindow();
		if (b_res == b1) {
			output = 9;
		} else if (b_res == b2) {
			output = 4;
		} else if (b_res == b3) {
			output = 1;
		} else if (b_res == b4) {
			output = 0;
		} else if (b_res == b5) {
			output = -1;
		}
		newtPopHelpLine();
		return (output);
	}


/**
 * Load @p source_file (a list of files) into @p filelist. There can be no more than
 * @p ARBITRARY_MAXIMUM entries.
 * @param filelist The filelist structure to load @p source_file into.
 * @param source_file The file containing a list of filenames to load into @p filelist.
 */
	int load_filelist_into_array(struct s_filelist *filelist,
								 char *source_file) {
		int i;
		bool done;
		char *reason = NULL;
		char *tmp = NULL;
		size_t n = 0;
		FILE *fin;
		struct s_filelist_entry dummy_fle;

		assert(filelist != NULL);
		assert_string_is_neither_NULL_nor_zerolength(source_file);

		iamhere("entering");
		if (!(fin = fopen(source_file, "r"))) {
			log_OS_error(source_file);
			log_msg(2, "Can't open %s; therefore, cannot popup list",
					source_file);
			return (1);
		}
		log_msg(2, "Loading %s", source_file);
		for (filelist->entries = 0; filelist->entries <= ARBITRARY_MAXIMUM;
			 filelist->entries++) {
			if (feof(fin)) {
				break;
			}
			(void) getline(&tmp, &n, fin);
			i = (int) strlen(tmp);
			if (i < 2) {
				if (feof(fin)) {
					break;
				}
			}
			if (tmp[i - 1] < 32) {
				tmp[--i] = '\0';
			}
			if (i < 2) {
				if (feof(fin)) {
					break;
				}
			}
			if (!does_file_exist(tmp)) {
				if (feof(fin)) {
					break;
				}
			}
			filelist->el[filelist->entries].severity =
				severity_of_difference(tmp, reason);
			paranoid_free(reason);
			strcpy(filelist->el[filelist->entries].filename, tmp);
			if (feof(fin)) {
				break;
			}
		}
		paranoid_fclose(fin);
		if (filelist->entries >= ARBITRARY_MAXIMUM) {
			log_to_screen(_("Arbitrary limits suck, man!"));
			paranoid_free(tmp);
			return (1);
		}
		paranoid_free(tmp);

		for (done = FALSE; !done;) {
			done = TRUE;
			for (i = 0; i < filelist->entries - 1; i++) {
//          if (strcmp(filelist->el[i].filename, filelist->el[i+1].filename) > 0)
				if (filelist->el[i].severity < filelist->el[i + 1].severity
					|| (filelist->el[i].severity ==
						filelist->el[i + 1].severity
						&& strcmp(filelist->el[i].filename,
								  filelist->el[i + 1].filename) > 0)) {
					memcpy((void *) &dummy_fle,
						   (void *) &(filelist->el[i]),
						   sizeof(struct s_filelist_entry));
					memcpy((void *) &(filelist->el[i]),
						   (void *) &(filelist->el[i + 1]),
						   sizeof(struct s_filelist_entry));
					memcpy((void *) &(filelist->el[i + 1]),
						   (void *) &dummy_fle,
						   sizeof(struct s_filelist_entry));
					log_msg(2, "Swapping %s and %s",
							filelist->el[i].filename,
							filelist->el[i + 1].filename);
					done = FALSE;
				}
			}
		}
		iamhere("leaving");
		return (0);
	}



/**
 * Generate a pretty string based on @p flentry.
 * @param flentry The filelist entry to stringify.
 * @return The string form of @p flentry.
 * @note The returned value points to static storage that will be overwritten with each call.
 */
	char *filelist_entry_to_string(struct s_filelist_entry *flentry) {
		char *comment;

		iamhere("entering");
		assert(flentry != NULL);
		if (flentry->severity == 0) {
			asprintf(&comment, "0     %93s", flentry->filename);
		} else if (flentry->severity == 1) {
			asprintf(&comment, "low   %93s", flentry->filename);
		} else if (flentry->severity == 2) {
			asprintf(&comment, "med   %93s", flentry->filename);
		} else {
			asprintf(&comment, "high  %93s", flentry->filename);
		}
		iamhere("leaving");
		return (comment);
	}





/**
 * Pop up a list containing the filenames in @p source_file and the severity if they have changed since the
 * last backup. There can be no more than @p ARBITRARY_MAXIMUM files in @p source_file.
 * @param source_file The file containing a list of changed files.
 */
	void popup_changelist_from_file(char *source_file) {
		char *reason = NULL;
		newtComponent myForm;
		newtComponent bClose;
		newtComponent bSelect;
		newtComponent b_res;
		newtComponent fileListbox;
		newtComponent headerMsg;

		/*@ ???? ************************************************************ */
		void *curr_choice;
		void *keylist[ARBITRARY_MAXIMUM];

		/*@ int ************************************************************* */
		int i = 0;
		int currline = 0;
		int finished = FALSE;
		long lng = 0;

		/*@ buffers ********************************************************* */
		char *tmp;
		char *differ_sz;

		struct s_filelist *filelist;
		assert_string_is_neither_NULL_nor_zerolength(source_file);
		if (g_text_mode) {
			log_msg(2, "Text mode. Therefore, no popup list.");
			return;
		}
		log_msg(2, "Examining file %s", source_file);

		lng = count_lines_in_file(source_file);
		if (lng < 1) {
			log_msg(2, "No lines in file. Therefore, no popup list.");
			return;
		} else if (lng >= ARBITRARY_MAXIMUM) {
			log_msg(2, "Too many files differ for me to list.");
			return;
		}

		filelist = (struct s_filelist *) malloc(sizeof(struct s_filelist));
		fileListbox =
			newtListbox(2, 2, 12, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
		newtListboxClear(fileListbox);

		if (load_filelist_into_array(filelist, source_file)) {
			log_msg(2, "Can't open %s; therefore, cannot popup list",
					source_file);
			return;
		}
		log_msg(2, "%d files loaded into filelist array",
				filelist->entries);
		for (i = 0; i < filelist->entries; i++) {
			keylist[i] = (void *) i;
			newtListboxAppendEntry(fileListbox,
								   filelist_entry_to_string(&
															(filelist->
															 el[i])),
								   keylist[i]);
		}
		asprintf(&differ_sz,
				 _("  %d files differ. Hit 'Select' to pick a file. Hit 'Close' to quit the list."),
				 i);
		newtPushHelpLine(differ_sz);
		paranoid_free(differ_sz);

		bClose = newtCompactButton(10, 15, _(" Close  "));
		bSelect = newtCompactButton(30, 15, _(" Select "));
		asprintf(&tmp, "%-10s               %-20s", _("Priority"),
				 _("Filename"));
		headerMsg = newtLabel(2, 1, tmp);
		paranoid_free(tmp);

		newtOpenWindow(5, 4, 70, 16, _("Non-matching files"));
		myForm = newtForm(NULL, NULL, 0);
		newtFormAddComponents(myForm, headerMsg, fileListbox, bClose,
							  bSelect, NULL);
		while (!finished) {
			b_res = newtRunForm(myForm);
			if (b_res == bClose) {
				finished = TRUE;
			} else {
				curr_choice = newtListboxGetCurrent(fileListbox);
				for (i = 0;
					 i < filelist->entries && keylist[i] != curr_choice;
					 i++);
				if (i == filelist->entries && filelist->entries > 0) {
					log_to_screen(_("I don't know what that button does!"));
				} else {
					currline = i;
					if (filelist->entries > 0) {
						severity_of_difference(filelist->el[currline].
											   filename, reason);
						asprintf(&tmp, "%s --- %s",
								 filelist->el[currline].filename, reason);
						popup_and_OK(tmp);
						paranoid_free(tmp);
						paranoid_free(reason);
					}
				}
			}
		}
		newtFormDestroy(myForm);
		newtPopWindow();
		newtPopHelpLine();
	}

/* @} - end of guiGroup */


void wait_until_software_raids_are_prepped(char *mdstat_file,
										   int wait_for_percentage);
