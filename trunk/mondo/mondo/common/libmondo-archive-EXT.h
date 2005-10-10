/* libmondo-archive-EXT.h */


extern int archive_this_fileset(struct s_bkpinfo *bkpinfo, char *filelist,
								char *fname, int setno);
extern int backup_data(struct s_bkpinfo *bkpinfo);
extern int call_mindi_to_supply_boot_disks(struct s_bkpinfo *bkpinfo);
extern bool can_we_fit_these_files_on_media(struct s_bkpinfo *bkpinfo,
											char *files_to_add, ...);
extern int do_that_initial_phase(struct s_bkpinfo *bkpinfo);
extern int do_that_final_phase(struct s_bkpinfo *bkpinfo);
extern int figure_out_kernel_path_interactively_if_necessary(char *kernel);
extern int make_those_slices_phase(struct s_bkpinfo *bkpinfo);
extern int make_those_afios_phase(struct s_bkpinfo *bkpinfo);
extern int make_slices_and_images(struct s_bkpinfo *bkpinfo,
								  char *biggielist_fname);
extern int make_iso_fs(struct s_bkpinfo *bkpinfo, char *destfile);
extern int make_afioballs_and_images(struct s_bkpinfo *bkpinfo);
extern int (*move_files_to_cd) (struct s_bkpinfo *, char *, ...);
extern int _move_files_to_cd(struct s_bkpinfo *bkpinfo, char *, ...);
extern int (*move_files_to_stream) (struct s_bkpinfo *, char *, ...);
extern int _move_files_to_stream(struct s_bkpinfo *bkpinfo,
								 char *files_to_add, ...);
extern int offer_to_write_boot_floppies_to_physical_disks(struct s_bkpinfo
														  *bkpinfo);
extern void pause_and_ask_for_cdr(int, bool *);
extern int slice_up_file_etc(struct s_bkpinfo *bkpinfo,
							 char *biggie_filename,
							 char *partimagehack_fifo,
							 long biggie_file_number,
							 long noof_biggie_files,
							 bool use_partimagehack);
extern int verify_data(struct s_bkpinfo *bkpinfo);
extern void wipe_archives(char *d);
extern int write_iso_and_go_on(struct s_bkpinfo *bkpinfo, bool last_cd);
extern int write_final_iso_if_necessary(struct s_bkpinfo *bkpinfo);
extern int call_growisofs(struct s_bkpinfo *bkpinfo, char *destfile);
extern int make_afioballs_and_images_SINGLETHREAD(struct s_bkpinfo
												  *bkpinfo);
extern int archive_this_fileset_with_star(struct s_bkpinfo *bkpinfo,
										  char *filelist, char *fname,
										  int setno);
