/* libmondo-files-EXT.h */

#include <sys/types.h>
#include <unistd.h>

extern unsigned int updcrc(unsigned int crc, unsigned int c);
extern unsigned int updcrcr(unsigned int crc, unsigned int c);
extern char *calc_checksum_of_file(char *filename);
extern char *calc_file_ugly_minichecksum(char *curr_fname);
extern long count_lines_in_file(char *filename);
extern bool does_file_exist(char *filename);
extern void exclude_nonexistent_files(char *inout);
extern int figure_out_kernel_path_interactively_if_necessary(char *kernel);
extern char *find_home_of_exe(char *fname);
extern int get_trackno_from_logfile(char *logfile);
extern int grab_percentage_from_last_line_of_file(char *filename);
extern char *last_line_of_file(char *filename);
extern off_t length_of_file(char *filename);
extern int make_hole_for_file(char *outfile_fname);
extern void make_list_of_files_to_ignore(char *ignorefiles_fname,
										 char *filelist_fname,
										 char *cklist_fname);
extern long noof_lines_that_match_wildcard(char *filelist_fname,
										   char *wildcard);
extern void register_pid(pid_t pid, char *name_str);
extern long long space_occupied_by_cd(char *mountpt);
extern int whine_if_not_found(char *fname);
extern int write_one_liner_data_file(char *fname, char *contents);



extern long size_of_all_biggiefiles_K(struct s_bkpinfo *bkpinfo);
extern void copy_mondo_and_mindi_stuff_to_scratchdir(struct s_bkpinfo
													 *bkpinfo);
extern void store_nfs_config(struct s_bkpinfo *bkpinfo);


extern void estimate_noof_media_required(struct s_bkpinfo *bkpinfo, long);

extern bool is_this_file_compressed(char *);



extern int make_hole_for_dir(char *outdir_fname);

extern long size_of_partition_in_mountlist_K(char *tmpdir, char *dev);

extern int make_grub_install_scriptlet(char *outfile);
extern int read_one_liner_data_file(char *fname, char *contents);
extern int mode_of_file(char *fname);
extern void paranoid_alloc(char *alloc, char *orig);
