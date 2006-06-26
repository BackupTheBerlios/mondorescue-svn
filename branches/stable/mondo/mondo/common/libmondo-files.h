/* libmondo-files.h
 * $Id$
 */

#include "crcttt.h"


unsigned int updcrc(unsigned int crc, unsigned int c);
unsigned int updcrcr(unsigned int crc, unsigned int c);
char *calc_checksum_of_file(char *filename);
char *calc_file_ugly_minichecksum(char *curr_fname);
char *calc_file_ugly_minichecksum(char *curr_fname);
long count_lines_in_file(char *filename);
bool does_file_exist(char *filename);
void exclude_nonexistent_files(char *inout);
int figure_out_kernel_path_interactively_if_necessary(char *kernel);
char *find_home_of_exe(char *fname);
int get_trackno_from_logfile(char *logfile);
int grab_percentage_from_last_line_of_file(char *filename);
char *last_line_of_file(char *filename);
off_t length_of_file(char *filename);
int make_checksum_list_file(char *filelist, char *cksumlist,
							char *comppath);
int make_hole_for_file(char *outfile_fname);
void make_list_of_files_to_ignore(char *ignorefiles_fname,
								  char *filelist_fname,
								  char *cklist_fname);
long noof_lines_that_match_wildcard(char *filelist_fname, char *wildcard);
void register_pid(pid_t pid, char *name_str);
long size_of_all_biggiefiles_K(struct s_bkpinfo *bkpinfo);
long long space_occupied_by_cd(char *mountpt);
int whine_if_not_found(char *fname);
int write_one_liner_data_file(char *fname, char *contents);

void copy_mondo_and_mindi_stuff_to_scratchdir(struct s_bkpinfo *bkpinfo);
void store_nfs_config(struct s_bkpinfo *bkpinfo);
void estimate_noof_media_required(struct s_bkpinfo *bkpinfo, long);
bool is_this_file_compressed(char *);








int make_hole_for_dir(char *outdir_fname);
long size_of_partition_in_mountlist_K(char *tmpdir, char *dev);
int make_grub_install_scriptlet(char *outfile);

int read_one_liner_data_file(char *fname, char *contents);
int mode_of_file(char *fname);
