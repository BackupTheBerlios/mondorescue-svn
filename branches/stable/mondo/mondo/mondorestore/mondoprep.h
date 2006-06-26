/***************************************************************************
                          mondoprep.h  -  description
                             -------------------
    begin                : Sat Apr 20 2002
    copyright            : (C) 2002 by Stan Benoit
    email                : troff@nakedsoul.org
    cvsid                : $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/** Externals **************************************************************/

extern long g_maximum_progress, g_current_progress, g_start_time;
extern int g_currentY, g_current_cd_number;
extern char *g_tape_device;
extern void finish(int);
extern void setup_newt_stuff(void);
extern char which_restore_mode(void);
extern bool ask_me_yes_or_no(char *);
extern long get_phys_size_of_drive(char *);
//extern void log_to_screen (char *);
extern void update_progress_form(char *);
extern void open_progress_form(char *, char *, char *, char *, long);
extern void close_progress_form(void);
extern void popup_and_OK(char *);
extern bool popup_and_get_string(char *, char *, char *, int);
extern long get_time(void);
extern bool is_this_device_mounted(char *);
extern int does_partition_exist(const char *device, int partno);
extern int strcmp_inc_numbers(char *, char *);
extern long count_lines_in_file(char *);
extern off_t length_of_file(char *);
extern long noof_lines_that_match_wildcard(char *, char *);
//extern char *slice_fname (long, long, bool, char *);
extern char *last_line_of_file(char *);
extern void log_file_end_to_screen(char *, char *);
extern int zero_out_a_device(char *);
extern void mvaddstr_and_log_it(int, int, char *);
extern bool does_file_exist(char *);


/** locals **********************************************************/
int extrapolate_mountlist_to_include_raid_partitions(struct mountlist_itself
													 *, struct mountlist_itself
													 *);
bool mountlist_contains_raid_devices(struct mountlist_itself *);
int start_raid_device(char *);
int stop_raid_device(char *);
int start_all_raid_devices(struct mountlist_itself *);
int stop_all_raid_devices(struct mountlist_itself *);
int format_everything(struct mountlist_itself *, bool, struct raidlist_itself *);
int partition_device(FILE *, const char *, int, int, const char *,
					 long long);
int partition_device_with_parted(FILE *, const char *, int, int,
								 const char *, long long);
int partition_device_with_fdisk(FILE *, const char *, int, int,
								const char *, long long);
int format_device(char *, char *, struct raidlist_itself *);
int partition_drive(struct mountlist_itself *, char *);
int partition_everything(struct mountlist_itself *);
int do_my_funky_lvm_stuff(bool, bool);
int which_format_command_do_i_need(char *, char *);
int make_dummy_partitions(FILE *, char *, int);
int make_list_of_drives(struct mountlist_itself *,
						char drivelist[ARBITRARY_MAXIMUM][MAX_STR_LEN]);
int set_partition_type(FILE *, const char *, int, const char *, long long);
void resize_drive_proportionately_to_suit_new_drives(struct mountlist_itself
													 *mountlist,
													 char *drive_name);
void resize_mountlist_proportionately_to_suit_new_drives(struct
														 mountlist_itself
														 *mountlist);


char *truncate_to_drive_name(char *partition);
void create_mountlist_for_drive(struct mountlist_itself *mountlist,
								char *drive_name,
								struct mountlist_reference *drivemntlist);
