/* newt-specific.h
 * $Id$
 */


#ifndef H_NEWT
#include <newt.h>
#endif

bool ask_me_yes_or_no(char *prompt);
bool ask_me_OK_or_cancel(char *prompt);
void close_evalcall_form(void);
void close_progress_form();
void fatal_error(char *error_string);
void finish(int signal);
void mvaddstr_and_log_it(int y, int x, char *output);
void log_file_end_to_screen(char *filename, char *grep_for_me);
void log_to_screen(const char *fmt, ...);
void open_evalcall_form(char *title);
void open_progress_form(char *title, char *b1, char *b2, char *b3,
						long max_val);
void popup_and_OK(char *prompt);
bool popup_and_get_string(char *title, char *b, char *output, int maxsize);
bool popup_with_buttons(char *p, char *button1, char *button2);
void refresh_log_screen();
void setup_newt_stuff();
void update_evalcall_form_ratio(int num, int denom);
void update_evalcall_form(int curr);
void update_progress_form(char *blurb3);
void update_progress_form_full(char *blurb1, char *blurb2, char *blurb3);






t_bkptype which_backup_media_type(bool);
int which_compression_level();


void popup_changelist_from_file(char *source_file);
