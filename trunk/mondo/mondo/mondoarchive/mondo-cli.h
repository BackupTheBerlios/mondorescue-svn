/* mondo-cli.h
 * $Id: mondo-cli.h,v 1.2 2004/06/10 15:29:13 hugo Exp $
 */

int handle_incoming_parameters (int argc, char *argv[], struct s_bkpinfo *bkpinfo);
int process_the_s_switch(struct s_bkpinfo *bkpinfo, char *value);
int process_switches (struct s_bkpinfo *bkpinfo, char flag_val[128][MAX_STR_LEN],
		  bool flag_set[128]);
int retrieve_switches_from_command_line (int argc, char *argv[],
				     char flag_val[128][MAX_STR_LEN],
				     bool flag_set[128]);
void help_screen ();
void terminate_daemon(int sig);
void set_signals(int on);
void termination_in_progress(int sig);


