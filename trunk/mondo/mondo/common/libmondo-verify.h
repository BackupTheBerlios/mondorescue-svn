/***************************************************************************
                          mondoverify.h  -  description
                             -------------------
    begin                : Mon Apr 22 2002
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




 /** externals *************************************************************/
int close_tape(struct s_bkpinfo *);
extern void close_progress_form(void);
extern long count_lines_in_file(char *);
extern bool does_file_exist(char *);
extern void exclude_nonexistent_files(char *);
extern void fatal_error(char *);
extern int find_and_mount_actual_cd(struct s_bkpinfo *, char *);
extern char *find_cdrom_device(bool);
extern void finish(int);
extern int get_last_filelist_number(struct s_bkpinfo *);
extern long get_time(void);
extern int grab_percentage_from_last_line_of_file(char *);
extern char *last_line_of_file(char *);
extern long long length_of_file(char *);
extern void log_file_end_to_screen(char *, char *);
extern void log_tape_pos(void);
extern char *marker_to_string(int);
extern void open_evalcall_form(char *);
extern void open_progress_form(char *, char *, char *, char *, long);
extern int openin_tape(struct s_bkpinfo *);
extern void popup_and_OK(char *);
extern bool popup_and_get_string(char *, char *, char *, int);
extern int read_file_from_tape_to_file(struct s_bkpinfo *, char *,
									   long long);
extern int read_header_block_from_tape(long long *, char *, int *);
extern void setup_newt_stuff(void);
extern char *slice_fname(long, long, char *, char *);
extern long long space_occupied_by_cd(char *);
extern int strcmp_inc_numbers(char *, char *);
extern char *strip_afio_output_line(char *);
extern char *trim_empty_quotes(char *);
extern void update_evalcall_form(int);
extern void update_progress_form(char *);
extern int write_data_disks_to_tape(char *);
extern int write_header_block_to_tape(long long, char *, int);
extern void wrong_marker(int, int);


/** Locals *****************************************************************/


int verify_cd_image(struct s_bkpinfo *);
int verify_a_tarball(struct s_bkpinfo *, char *);
int verify_an_afioball_from_CD(struct s_bkpinfo *, char *);
int verify_an_afioball_from_tape(struct s_bkpinfo *, char *, long long);
int verify_a_biggiefile_from_tape(struct s_bkpinfo *, char *, long long);
int verify_afioballs_from_CD(struct s_bkpinfo *);
int verify_afioballs_from_tape(struct s_bkpinfo *);
int verify_biggiefiles_from_tape(struct s_bkpinfo *);
int verify_tape_backups(struct s_bkpinfo *);
char *vfy_tball_fname(struct s_bkpinfo *, char *, int);




/*---------------------------------------------------------------------*/



extern FILE *g_tape_stream;
extern long g_start_time, g_minimum_progress, g_maximum_progress,
	g_current_progress, g_currentY;
extern char err_log_lines[NOOF_ERR_LINES][MAX_STR_LEN];
extern int g_current_media_number;

extern void mvaddstr_and_log_it(int, int, char *);

extern bool ask_me_yes_or_no(char *);
extern char *calc_checksum_of_file(char *filename);
extern void center_string(char *, int);
extern void close_evalcall_form(void);
extern int closein_tape(struct s_bkpinfo *);
