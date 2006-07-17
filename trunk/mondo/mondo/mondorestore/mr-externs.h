/*
 * $Id$
 *
 * mondo-restore.c's externs
 *
 */

#define SIZE 730000 * 1024		/*  Size for ISO's stops -1 */
#define BIGGIELIST MNT_CDROM"/archives/biggielist.txt"
#define ARCHIVES_PATH MNT_CDROM"/archives"

#ifdef __FreeBSD__
#define raid_device_record vinum_volume
#endif

extern bool ask_me_yes_or_no(char *);
extern char *calc_checksum_of_file(char *);
extern int closein_tape(struct s_bkpinfo *);
extern void close_evalcall_form(void);
extern char *call_program_and_get_last_line_of_output(char *);
extern void close_progress_form(void);
extern long count_lines_in_file(char *);
extern bool does_file_exist(char *);
extern int does_partition_exist(const char *device, int partno);
extern int do_my_funky_lvm_stuff(bool, bool);
extern int edit_filelist(struct s_node *);
extern int edit_mountlist(char *mountlist_fname, struct mountlist_itself *,
						  struct raidlist_itself *);
extern int format_everything(struct mountlist_itself *, bool, struct raidlist_itself *);
extern int format_device(char *, char *, struct raidlist_itself *);
extern void finish(int);
extern void free_filelist(struct s_node *);
extern long get_time(void);
extern bool get_isodir_info(char *, char *, char *, bool);
extern void fatal_error(char *);
extern void initialize_raid_record(struct raid_device_record *);
extern bool is_this_device_mounted(char *);
extern off_t length_of_file(char *);
extern char *last_line_of_file(char *);
extern struct s_node *load_filelist(char *);
extern void log_tape_pos(void);
extern void initialize_raidrec(struct raid_device_record *);
extern void log_file_end_to_screen(char *, char *);
extern void log_to_screen(const char *fmt, ...);
extern void mvaddstr_and_log_it(int, int, char *);
extern int make_dummy_partitions(char *, int);
extern int make_hole_for_file(char *);
extern int make_list_of_drives(struct mountlist_itself *,
							   struct list_of_disks *);
extern bool mountlist_contains_raid_devices(struct mountlist_itself *);
extern void open_evalcall_form(char *);
extern void open_progress_form(char *, char *, char *, char *, long);
extern int openin_cdstream(struct s_bkpinfo *);
extern int openin_tape(struct s_bkpinfo *);
extern int partition_device(char *, int, int, char *, long);
extern int partition_device_with_fdisk(char *, int, int, char *, long);
extern int partition_device_with_parted(char *, int, int, char *, long);
extern int partition_drive(struct mountlist_itself *, char *);
extern int partition_everything(struct mountlist_itself *);
extern void popup_and_OK(char *);
extern bool popup_and_get_string(char *, char *, char *, int);
extern void setup_newt_stuff(void);
extern void reset_bkpinfo(struct s_bkpinfo *);
extern int read_cfg_var(char *, char *, char *);
extern int read_file_from_stream_to_file(struct s_bkpinfo *, char *,
										 long long);
extern int read_file_from_stream_to_stream(struct s_bkpinfo *, FILE *,
										   long long);
extern int read_file_from_stream_FULL(struct s_bkpinfo *, char *, FILE *,
									  long long);
extern int read_header_block_from_stream(long long *, char *, int *);
extern void save_filelist(struct s_node *, char *);
extern void strip_spaces(char *);
extern int strcmp_inc_numbers(char *, char *);
extern char *slice_fname(long, long, char *, char *);
extern int stop_raid_device(char *);
extern int stop_all_raid_devices(struct mountlist_itself *);
extern void update_evalcall_form(int);
extern void update_progress_form(char *);
extern int verify_tape_backups(struct s_bkpinfo *);
extern char which_restore_mode(void);
extern int which_format_command_do_i_need(char *, char *);
extern int write_cfg_var(char *, char *, char *);
extern void wrong_marker(int, int);
extern void resize_drive_proportionately_to_suit_new_drives(struct
															mountlist_itself
															*mountlist, char
															*drive_name);
extern void resize_mountlist_proportionately_to_suit_new_drives(struct
																mountlist_itself
																*mountlist);
extern int get_cfg_file_from_archive(struct s_bkpinfo *);



/**************************************************************************
 * Externals   yummmy!!!                                                  *
 **************************************************************************/
extern long g_maximum_progress;
extern long g_current_progress;
extern long g_start_time;
extern int g_currentY;
extern int g_current_media_number;	/* set to 1 in mondo-tools.c (tape) */
extern long long g_tape_posK;
extern FILE *g_tape_stream;
extern bool g_cd_recovery;
extern bool g_text_mode;
extern bool g_restoring_live_from_cd;
extern int fput_string_one_char_at_a_time(FILE *, char *);





extern int
evaluate_mountlist(struct mountlist_itself *mountlist, char *flaws_str_A,
				   char *flaws_str_B, char *flaws_str_C);



#ifdef __FreeBSD__
#undef raid_device_record
#endif
