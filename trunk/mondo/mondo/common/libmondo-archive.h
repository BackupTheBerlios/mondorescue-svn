/* libmondo-archive.h
 * $Id$
 */

int archive_this_fileset(struct s_bkpinfo *bkpinfo, char *filelist,
						 char *fname, int setno);
int backup_data(struct s_bkpinfo *bkpinfo);
int call_mindi_to_supply_boot_disks(struct s_bkpinfo *bkpinfo);
bool can_we_fit_these_files_on_media(struct s_bkpinfo *bkpinfo,
									 char *files_to_add, ...);
int do_that_initial_phase(struct s_bkpinfo *bkpinfo);
int do_that_final_phase(struct s_bkpinfo *bkpinfo);
int figure_out_kernel_path_interactively_if_necessary(char *kernel);
bool get_bit_N_of_array(char *array, int N);
int make_those_slices_phase(struct s_bkpinfo *bkpinfo);
int make_those_afios_phase(struct s_bkpinfo *bkpinfo);
int make_slices_and_images(struct s_bkpinfo *bkpinfo,
						   char *biggielist_fname);
int make_iso_fs(struct s_bkpinfo *bkpinfo, struct s_mrconf *mrconf, char *destfile);
int make_afioballs_and_images(struct s_bkpinfo *bkpinfo);
extern int (*move_files_to_cd) (struct s_bkpinfo *, char *, ...);
int _move_files_to_cd(struct s_bkpinfo *bkpinfo, char *files_to_add, ...);
extern int (*move_files_to_stream) (struct s_bkpinfo *, char *, ...);
int _move_files_to_stream(struct s_bkpinfo *bkpinfo, char *files_to_add,
						  ...);
int offer_to_write_boot_floppies_to_physical_disks(struct s_bkpinfo
												   *bkpinfo);
void pause_and_ask_for_cdr(int, bool *);
void set_bit_N_of_array(char *array, int N, bool true_or_false);
int slice_up_file_etc(struct s_bkpinfo *bkpinfo, char *biggie_filename,
					  char *ntfsprog_fifo,
					  long biggie_file_number, long noof_biggie_files,
					  bool use_ntfsprog);
int verify_data(struct s_bkpinfo *bkpinfo);
void wipe_archives(char *d);
int write_image_to_floppy(char *device, char *datafile);
int write_image_to_floppy_SUB(char *device, char *datafile);
int write_iso_and_go_on(struct s_bkpinfo *bkpinfo, bool last_cd);
int write_final_iso_if_necessary(struct s_bkpinfo *bkpinfo);
int call_growisofs(struct s_bkpinfo *bkpinfo, char *destfile);
int make_afioballs_and_images_SINGLETHREAD(struct s_bkpinfo *bkpinfo);
int archive_this_fileset_with_star(struct s_bkpinfo *bkpinfo,
								   char *filelist, char *fname, int setno);
void setenv_mondo_share(void);
