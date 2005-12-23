/* libmondo-fork-EXT.h */

extern char *call_program_and_get_last_line_of_output(char *call);
extern int run_program_and_log_to_screen(char *basic_call,
										 char *what_i_am_doing);
extern int run_program_and_log_output(char *program, int);
extern int eval_call_to_make_ISO(struct s_bkpinfo *bkpinfo,
								 char *basic_call, char *isofile,
								 int cd_no, char *logstub,
								 char *what_i_am_doing);
extern int run_external_binary_with_percentage_indicator_OLD(char *tt,
															 char *cmd);
extern int run_external_binary_with_percentage_indicator_NEW(char *tt,
															 char *cmd);
extern int copy_from_src_to_dest(FILE *, FILE *, char);
extern int feed_into_partimage(char *input_device, char *output_fname);
extern int feed_outfrom_partimage(char *output_device, char *input_fifo);
