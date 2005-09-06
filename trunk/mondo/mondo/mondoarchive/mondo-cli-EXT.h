/* mondo-cli-EXT.h */

extern int handle_incoming_parameters (int argc, char *argv[], struct s_bkpinfo *bkpinfo);
extern int process_the_s_switch(struct s_bkpinfo *bkpinfo, char *value);
extern int process_switches (struct s_bkpinfo *bkpinfo, char flag_val[128][MAX_STR_LEN],
		  bool flag_set[128]);
extern int retrieve_switches_from_command_line (int argc, char *argv[],
				     char flag_val[128][MAX_STR_LEN],
				     bool flag_set[128]);
extern void help_screen ();
extern void terminate_daemon(int sig);
extern void set_signals(int on);
extern void termination_in_progress(int sig);

