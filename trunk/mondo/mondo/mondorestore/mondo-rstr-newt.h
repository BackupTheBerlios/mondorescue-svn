/*
 * $Id$
**/

#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "../common/libmondo.h"
#ifdef   __FreeBSD__
#define  raid_device_record             vinum_volume
#endif /*__FreeBSD__*/

#define NO_FLAWS_DETECTED _("No flaws detected in mountlist at this time. Hit 'OK' to proceed.")


/** I found this in the code :) *******************************************/
char g_strings_of_flist_window[ARBITRARY_MAXIMUM][MAX_STR_LEN];
bool g_is_path_selected[ARBITRARY_MAXIMUM];
bool g_is_path_expanded[ARBITRARY_MAXIMUM];
char tmpnopath[MAX_STR_LEN + 2];
char tmpprevpath[MAX_STR_LEN + 2];

/* external subroutines and global vars */

extern long get_time(void);
extern char *last_line_of_file(char *);
extern void center_string(char *, int);
extern bool does_file_exist(char *);
extern void finish(int);
extern long get_phys_size_of_drive(char *);
extern bool is_this_device_mounted(char *);
extern void strip_spaces(char *);
extern void initialize_raidrec(struct raid_device_record *);
extern int make_list_of_drives(struct mountlist_itself *,
							   char[ARBITRARY_MAXIMUM][MAX_STR_LEN]);
char *number_to_text(int);
extern void reload_filelist(struct s_node *);
extern void toggle_all_root_dirs_on(struct s_node *);
extern void toggle_path_expandability(struct s_node *, char *, bool);
extern void toggle_path_selection(struct s_node *, char *, bool);
extern void toggle_node_selection(struct s_node *, bool);
extern struct s_node *find_node_in_filelist(struct s_node *,
											char *filename);
extern int what_number_cd_is_this(struct s_bkpinfo *);
//extern void fatal_error (char *);
extern void sort_mountlist_by_device(struct mountlist_itself *);
extern int load_mountlist(struct mountlist_itself *a, char *b);
extern bool g_text_mode;


/* hacks */
extern int load_raidtab_into_raidlist(struct raidlist_itself *a, char *b);



extern long g_start_time, g_minimum_progress, g_maximum_progress,
	g_current_progress, g_currentY;
extern bool g_ISO_restore_mode;

/* my subroutines */

void add_disklist_entry(struct list_of_disks *, char *,
						struct mountlist_itself *);
void add_mountlist_entry(struct mountlist_itself *,
						 struct raidlist_itself *, newtComponent, int,
						 void *keylist[]);
void add_varslist_entry(struct raid_device_record *);
bool ask_me_yes_or_no(char *);
bool ask_me_OK_or_cancel(char *);
long calculate_raid_device_size(struct mountlist_itself *,
								struct raidlist_itself *, char *);
void choose_raid_level(struct
#ifdef __FreeBSD__
					   vinum_plex
#else
					   raid_device_record
#endif
					   *);
void close_evalcall_form(void);
void close_progress_form(void);
void del_partns_listed_in_disklist(struct mountlist_itself *,
								   struct raidlist_itself *,
								   struct list_of_disks *);
void delete_disklist_entry(struct list_of_disks *, char *, int);
void delete_mountlist_entry(struct mountlist_itself *,
							struct raidlist_itself *, newtComponent, int,
							void *keylist[]);
void delete_raidlist_entry(struct mountlist_itself *,
						   struct raidlist_itself *, char *);
void delete_varslist_entry(struct raid_device_record *, int);
char *disklist_entry_to_string(struct list_of_disks *, int);
int edit_filelist(struct s_node *);
void edit_mountlist_entry(struct mountlist_itself *,
						  struct raidlist_itself *, newtComponent, int,
						  void *keylist[]);
void edit_raidlist_entry(struct mountlist_itself *,
						 struct raidlist_itself *,
						 struct raid_device_record *, int);
void edit_varslist_entry(struct raid_device_record *, int);
int edit_mountlist_in_newt(char *mountlist_fname,
						   struct mountlist_itself *,
						   struct raidlist_itself *);
int edit_mountlist(char *mountlist_fname, struct mountlist_itself *,
				   struct raidlist_itself *);
void edit_raidrec_additional_vars(struct raid_device_record *);
int evaluate_mountlist(struct mountlist_itself *, char *, char *, char *);
int find_device_in_mountlist(struct mountlist_itself *, char *);
int find_next_free_index_in_disklist(struct list_of_disks *);
int find_raid_device_in_raidlist(struct raidlist_itself *, char *);
bool get_isodir_info(char *, char *, char *, bool);
void initiate_new_raidlist_entry(struct raidlist_itself *,
								 struct mountlist_itself *, int, char *);
void insert_essential_additionalvars(struct raid_device_record *);
bool is_this_raid_personality_registered(int);
void log_file_end_to_screen(char *, char *);
void log_to_screen(const char *fmt, ...);
int look_for_duplicate_mountpoints(struct mountlist_itself *, char *);
void make_list_of_unallocated_raid_partitions(struct mountlist_itself *,
											  struct mountlist_itself *,
											  struct raidlist_itself *);
char *mountlist_entry_to_string(struct mountlist_itself *, int);
void mvaddstr_and_log_it(int, int, char *);
void nuke_mode_dummy();
char *number_of_disks_as_string(int, char *);
void open_evalcall_form(char *);
void open_progress_form(char *, char *, char *, char *, long);
void popup_and_OK(char *);
bool popup_and_get_string(char *, char *, char *);
bool popup_with_buttons(char *, char *, char *);
void redraw_disklist(struct list_of_disks *, void *keylist[],
					 newtComponent);
void redraw_mountlist(struct mountlist_itself *, void *keylist[],
					  newtComponent);
void redraw_unallocpartnslist(struct mountlist_itself *, void *keylist[],
							  newtComponent);
void redraw_varslist(struct additional_raid_variables *, void *keylist[],
					 newtComponent);
int read_variableINT_and_remove_from_raidvars(struct raid_device_record *,
											  char *);
void refresh_log_screen(void);
void rejig_partition_name_in_raidlist_if_necessary(struct raidlist_itself
												   *, char *, char *);
void remove_essential_additionalvars(struct raid_device_record *);
void select_raid_disks(struct mountlist_itself *, struct raidlist_itself *,
					   struct raid_device_record *, char *,
					   struct list_of_disks *);
void setup_newt_stuff(void);
long size_of_specific_device(struct mountlist_itself *, char *);
bool spread_flaws_across_three_lines(char *, char *, char *, char *, int);
char *turn_raid_level_number_to_string(int);
void update_evalcall_form(int);
void update_progress_form(char *);
void update_progress_form_full(char *, char *, char *);
char which_restore_mode(void);
void write_variableINT_to_raid_var_line(struct raid_device_record *, int,
										char *, int);
int where_in_drivelist_is_drive(struct list_of_disks *, char *);
char *strip_path(char *);

/* -------------------------------------------------------------------- */


#ifdef __FreeBSD__
#undef raid_device_record
#endif
