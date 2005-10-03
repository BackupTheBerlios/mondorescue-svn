/* X-specific-EXT.h */


extern int  ask_me_yes_or_no (char *prompt);
extern int  ask_me_OK_or_cancel (char *prompt);
extern void close_evalcall_form (void);
extern void close_progress_form ();
extern void fatal_error (char *error_string);
extern void finish (int signal);
extern void mvaddstr_and_log_it (int y, int x, char *output);
extern void open_evalcall_form (char *title);
extern void open_progress_form (char *title, char *b1, char *b2, char *b3, long max_val);
extern void log_file_end_to_screen (char *filename, char *grep_for_me);
extern void log_to_screen (const char *op, ...);
extern void popup_and_OK (char *prompt);
extern int  popup_and_get_string (char *title, char *b, char *output, int maxsize);
extern int  popup_with_buttons (char *p, char *button1, char *button2);
extern void refresh_log_screen ();
extern void setup_newt_stuff ();
extern void update_evalcall_form_ratio (int num, int denom);
extern void update_evalcall_form (int curr);
extern void update_progress_form (char *blurb3);
extern void update_progress_form_full (char *blurb1, char *blurb2, char *blurb3);





extern t_bkptype which_backup_media_type (bool);
extern int which_compression_level ();


extern void popup_changelist_from_file(char*source_file);


