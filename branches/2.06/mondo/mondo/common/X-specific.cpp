/* X-specific.c


- subroutines which do display-type things
  and use the Qt library to do them

10/04
- took out the newt-related subroutines

09/12
- created
*/

#include <config.h>

#if !WITH_X
#warning "*** You are compiling X-specific.cpp without X support!"
#warning "*** Compiling newt-specific.c instead."
#include "newt-specific.c"
#else

#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qmultilineedit.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <kapp.h>
#include <sys/time.h>

extern "C" {
	#include <unistd.h>
	#include "my-stuff.h"
	#include "mondostructures.h"
	#include "X-specific.h"
	#include "libmondo-string-EXT.h"
	#include "libmondo-files-EXT.h"
	#include "libmondo-tools-EXT.h"
	#include "libmondo-fork-EXT.h"
	#include "libmondo-gui-EXT.h"
	#include "lib-common-externs.h"
}

extern QProgressBar            *XMondoProgress;
extern QLabel                  *XMondoProgressWhat, *XMondoProgressWhat2, *XMondoProgressWhat3;
extern QMultiLineEdit          *XMondoLog;
extern QLabel                  *XMondoStatus;
extern QLabel                  *XMondoTimeTaken, *XMondoTimeToGo, *XMondoProgressPercent;
extern XMEventHolder            events;
extern int                      g_operation_in_progress;

#include <pthread.h>

void *run_evalcall_updater_thread (void *wasted_electrons) {
    while (1) {
	usleep (500000);
	update_evalcall_form (0);
    }
    return wasted_electrons;
}

extern "C" {

pthread_t updater;
bool updater_running = false;

extern pid_t g_mastermind_pid;
pid_t g_main_pid;

char err_log_lines[NOOF_ERR_LINES][MAX_STR_LEN], g_blurb_str_1[MAX_STR_LEN] =
  "", g_blurb_str_2[MAX_STR_LEN] = "", g_blurb_str_3[MAX_STR_LEN] = "";

int g_result_of_last_event = -1;
long g_isoform_starttime;
int g_isoform_old_progress = -1;
char g_isoform_header_str[MAX_STR_LEN];
int g_mysterious_dot_counter;
extern FILE *g_f_logfile_out;

int g_currentY = 3;		/* purpose */
extern int g_current_media_number;

long g_maximum_progress = 999;	/* purpose */
long g_current_progress = -999;	/* purpose */
long g_start_time = 0;		/* purpose */
int g_text_mode = TRUE;

int g_exiting = FALSE;
char *g_erase_tmpdir_and_scratchdir;
extern char g_tmpfs_mountpt[];

int ask_me_yes_or_no (char *prompt) { return popup_with_buttons (prompt, "Yes", "No"); }
int ask_me_OK_or_cancel (char *prompt) { return popup_with_buttons (prompt, "OK", "Cancel"); }

int
popup_with_buttons (char *prompt, char *button1, char *button2) 
{
    if (!g_operation_in_progress) return popup_with_buttons_sub (prompt, button1, button2);

    int res;
    events.popupWithButtons (prompt, button1, button2);
    while (g_result_of_last_event == -1);
    res = g_result_of_last_event;
    g_result_of_last_event = -1;
    return res;
}

int
popup_with_buttons_sub (char *prompt, char *button1, char *button2)
{
	switch (QMessageBox::information (0, "XMondo", prompt, button2, button1, 0, 0, 1)) {
		case 0:
			return 1;
			break;
		case 1:
			return 0;
			break;
		default:
			return 2;
			break;
	}
}

void
fatal_error (char *error)
{
    if (!g_operation_in_progress) fatal_error_sub (error);
    events.errorMsg (error);
    while(1);
}

void
fatal_error_sub (char *error)
{
    static bool already_exiting = false;
    char tmp[MAX_STR_LEN];
    g_exiting = TRUE;

    log_it ("Fatal error received - '%s'", error);
    printf ("Fatal error... %s\n", error);
    if (getpid() == g_mastermind_pid)
	{   
	    log_it ("(FE) mastermind %d is exiting", (int)getpid());   
	    kill (g_main_pid, SIGTERM);
	    finish (1);
	}

    if (getpid() != g_main_pid)
	{   
	    if (g_mastermind_pid != 0 && getpid() != g_mastermind_pid)
		{    
		    log_it ("(FE) non-m/m %d is exiting", (int)getpid());   
		    kill (g_main_pid, SIGTERM);
		    finish (1);
		}

	    if (getpid() != g_main_pid)
		{		    
		    log_it ("(FE) aux pid %d is exiting", (int)getpid());   
		    kill (g_main_pid, SIGTERM);
		    finish (1);
		}
	}

    log_it ("OK, I think I'm the main PID.");
    if (already_exiting)
	{
	    log_it ("...I'm already exiting. Give me time, Julian!");   
	    finish (1);
	}

    already_exiting = TRUE;
    log_it ("I'm going to do some cleaning up now.");
    kill_anything_like_this ("mondoarchive");
    kill_anything_like_this ("/mondo/do-not");
    kill_anything_like_this ("tmp.mondo");
    sync();
    
    sprintf (tmp, "umount %s", g_tmpfs_mountpt);
    chdir ("/");
    for(int i=0; i<10 && run_program_and_log_output (tmp, TRUE); i++)
	{
	    log_it ("Waiting for child processes to terminate");   
	    sleep (1);
	    run_program_and_log_output (tmp, TRUE);
	}

    if (g_erase_tmpdir_and_scratchdir[0])
	{
	    run_program_and_log_output (g_erase_tmpdir_and_scratchdir, TRUE);
	}

    QMessageBox::critical (0, "XMondo", QString ("<font color=\"red\"><b>FATAL ERROR</b></font><br>%1").arg (error),
												 "Quit", 0, 0, 0, 0);
    kapp->quit();
    printf ("---FATAL ERROR--- %s\n", error);
    system ("gzip -9c /var/log/mondo-archive.log > /tmp/MA.log.gz 2> /dev/null");
    printf ("If you require technical support, please contact the mailing list.\n");
    printf ("See http://www.mondorescue.org for details.\n");
    printf ("Log file: %s\n", MONDO_LOGFILE);
    
    if (does_file_exist ("/tmp/MA.log.gz"))
	{
	    printf ("FYI, I have gzipped the log and saved it to /tmp/MA.log.gz\n");
	    printf ("The list's members can help you, if you attach that file to your e-mail.\n");
	}
    
    printf ("Mondo has aborted.\n");
    register_pid (0, "mondo");
    
    if (!g_main_pid) {
	log_it ("FYI - g_main_pid is blank");
    }
    
    finish (254);
    /*NOTREACHED*/
}

void
finish (int eval)
{
	register_pid (0, "mondo");
	chdir ("/");
	run_program_and_log_output (static_cast <char*> ("umount /mnt/cdrom"), true);
	printf ("See %s for details of backup run.", MONDO_LOGFILE);
	exit (eval);
	/*NOTREACHED*/
}

void
log_file_end_to_screen (char *filename, char *grep_for_me)
{

	/** buffers ***********************************************************/
  char command[MAX_STR_LEN + 1];
	char tmp[MAX_STR_LEN + 1];

	/** pointers **********************************************************/
  FILE *fin;

	/** int ***************************************************************/
  int i = 0;



  if (!does_file_exist (filename))
    {
      return;
    }
  if (grep_for_me[0] != '\0')
    {
      sprintf (command, "grep \"%s\" %s | tail -n%d", 
	       grep_for_me, filename, NOOF_ERR_LINES);
    }
  else
    {
      sprintf (command, "tail -n%d %s", NOOF_ERR_LINES, filename);
    }
  fin = popen (command, "r");
  if (fin)
    {
      for (i = 0; i < NOOF_ERR_LINES; i++)
	  {
	      char tmp[MAX_STR_LEN];
	      fgets (tmp, MAX_STR_LEN, fin);
	      events.insertLine (XMondoLog, tmp);
	  }
    }
  pclose (fin);
}

void
log_to_screen (const char *line, ...)
{
    char *output = new char [MAX_STR_LEN];
    va_list ap;
    va_start (ap, line);
    vsprintf (output, line, ap);
    va_end (ap);
    standard_log_debug_msg (0, __FILE__, __FUNCTION__, __LINE__, output);
    events.insertLine (XMondoLog, output);
    delete[] output;
}

void
mvaddstr_and_log_it (int y, int x, char *line)
{
    if ((x != 0) && (strcmp (line, "Done.") == 0)) {
	return;
    }

    XMondoStatus->setText (line);
    usleep (250000);
}

void
popup_and_OK (char *msg)
{
    if (!g_operation_in_progress) return popup_and_OK_sub (msg);
    events.infoMsg (msg);
    while (g_result_of_last_event == -1);
    g_result_of_last_event = -1;
}

void
popup_and_OK_sub (char *msg)
{
	QMessageBox::information (0, "XMondo", msg, "OK", 0, 0, 0, 0);
}

int
popup_and_get_string (char *title, char *msg, char *output, int maxlen)
{
    if (!g_operation_in_progress) return popup_and_get_string_sub (title, msg, output, maxlen);

    int res;
    events.getInfo (title, msg, output, maxlen);
    while (g_result_of_last_event == -1);
    res = g_result_of_last_event;
    g_result_of_last_event = -1;
    return res;
}

int
popup_and_get_string_sub (char *title, char *msg, char *output, int maxlen)
{
	bool ok;

	(void) maxlen;
	QString out = QInputDialog::getText (title, msg, QLineEdit::Normal, QString::null, &ok);
	if (ok) {
		strcpy (output, out.ascii());
		return 1;
	}
	return 0;
}

void
refresh_log_screen()
{}

void
setup_newt_stuff()
{}

void
open_evalcall_form (char *ttl)
{
	g_isoform_starttime = get_time();
	events.setTotalSteps (XMondoProgress, 100);
	events.setProgress (XMondoProgress, 0);
	events.show (XMondoProgress);
	events.setText (XMondoProgressWhat, ttl);
	events.show (XMondoProgressWhat);
	events.setText (XMondoTimeTaken, "");
	events.setText (XMondoTimeToGo,  "");
	update_evalcall_form (0);
}

void
open_progress_form (char *title, char *line1, char *line2, char *line3, long maxval)
{
	g_start_time = get_time();
	g_maximum_progress = maxval;
	g_current_progress = 0;
	events.setTotalSteps (XMondoProgress, maxval);
	events.setProgress (XMondoProgress, 0);
	events.show (XMondoProgress);
	events.setText (XMondoProgressWhat, title);
	events.show (XMondoProgressWhat);
	events.setText (XMondoTimeTaken, "");
	events.setText (XMondoTimeToGo, "");
	update_progress_form_full (line1, line2, line3);
}

void
update_evalcall_form_ratio (int num, int denom)
{
	int timeTaken, timeTotal, timeToGo;
	int percent;

	timeTaken = get_time() - g_isoform_starttime;
	
	if (num * 100 / denom <= 1) {
	    struct timeval tv;
	    gettimeofday (&tv, 0);

	        if (!updater_running) {
		    pthread_create (&updater, 0, run_evalcall_updater_thread, 0);
		    updater_running = true;
		}
		QString ttaken;
		ttaken.sprintf ("%2ld:%02ld taken", timeTaken / 60, timeTaken % 60);
		events.setTotalSteps (XMondoProgress, 0);
		events.setProgress (XMondoProgress, (timeTaken * 50) + ((tv.tv_usec / 100000) * 5));
		events.show (XMondoProgress);
		events.show (XMondoTimeTaken);
		events.setText (XMondoTimeTaken, ttaken);
		events.hide (XMondoTimeToGo);
	}
	else {
	        if (updater_running) {
		    pthread_cancel (updater);
		    updater_running = false;
		}
		QString ttaken, ttogo;
		timeTotal = timeTaken * denom / num;
		timeToGo = timeTotal - timeTaken;
		ttaken.sprintf ("%2ld:%02ld taken", timeTaken / 60, timeTaken % 60);
		ttogo.sprintf ("%2ld:%02ld to go", timeToGo / 60, timeToGo % 60);
		percent = (num * 100 + denom / 2) / denom;
		events.setTotalSteps (XMondoProgress, 100);
		events.setProgress (XMondoProgress, percent);
		events.show (XMondoProgress);
		events.setText (XMondoTimeTaken, ttaken);
		events.show (XMondoTimeTaken);
		events.setText (XMondoTimeToGo, ttogo);
		events.show (XMondoTimeToGo);
	}
}

void update_evalcall_form (int percent)
{
	update_evalcall_form_ratio (percent, 100);
}

void
update_progress_form (char *b3)
{
	if (g_current_progress < -900) {
		log_it ("Ignoring update_progress_form (it's not open)");
		return;
	}
	events.setText (XMondoProgressWhat3, b3);
	update_progress_form_full (g_blurb_str_1, g_blurb_str_2, b3);
}

void
update_progress_form_full (char *b1, char *b2, char *b3)
{
    strcpy (g_blurb_str_1, b1);
    strcpy (g_blurb_str_2, b2);
    strcpy (g_blurb_str_3, b3);

    int timeTaken, timeTotal, timeToGo;
    timeTaken = get_time() - g_start_time;
    if (g_current_progress) {
	timeTotal = timeTaken * g_maximum_progress / g_current_progress;
	timeToGo  = timeTotal - timeTaken;
	
	QString ttaken, ttogo;
	ttaken.sprintf ("%2ld:%02ld taken", timeTaken / 60, timeTaken % 60);
	ttogo.sprintf ("%2ld:%02ld to go", timeToGo / 60, timeToGo % 60);
	
	events.setTotalSteps (XMondoProgress, g_maximum_progress);
	events.setProgress (XMondoProgress, g_current_progress);
	events.setText (XMondoTimeTaken, ttaken);
	events.setText (XMondoTimeToGo, ttogo);
	events.setText (XMondoProgressWhat, b1);
	events.setText (XMondoProgressWhat2, b2);
	events.setText (XMondoProgressWhat3, b3);
	events.show (XMondoProgressWhat3);
	events.show (XMondoProgress);
	events.show (XMondoTimeTaken);
	events.show (XMondoTimeToGo);
	events.show (XMondoProgressWhat);
	events.show (XMondoProgressWhat2);
    }
    else {
	events.setTotalSteps (XMondoProgress, 0);
	events.setProgress (XMondoProgress, timeTaken * 100);
	events.show (XMondoProgress);
	events.hide (XMondoTimeTaken);
	events.hide (XMondoTimeToGo);
	events.setText (XMondoProgressWhat, b1);
	events.show (XMondoProgressWhat);
	events.setText (XMondoProgressWhat2, b2);
	events.show (XMondoProgressWhat2);
	events.setText (XMondoProgressWhat3, b3);
	events.show (XMondoProgressWhat3);
    }
}

void
close_evalcall_form()
{
    if (updater_running) {
	pthread_cancel (updater);
	updater_running = false;
    }
	update_evalcall_form (100);
	usleep (500000);
	events.hide (XMondoProgress);
	events.hide (XMondoTimeTaken);
	events.hide (XMondoTimeToGo);
	events.hide (XMondoProgressWhat);
	events.hide (XMondoProgressWhat2);
	events.hide (XMondoProgressWhat3);
}

void
close_progress_form()
{
	update_progress_form ("Complete");
	usleep (1000000);
	events.hide (XMondoProgress);
	events.hide (XMondoTimeTaken);
	events.hide (XMondoTimeToGo);
	events.hide (XMondoProgressWhat);
	events.hide (XMondoProgressWhat2);
	events.hide (XMondoProgressWhat3);
}

t_bkptype which_backup_media_type (bool restoring) {}
int which_compression_level() {}
void popup_changelist_from_file (char *file) {}

} /* extern "C" */
#endif /* WITH_X */
