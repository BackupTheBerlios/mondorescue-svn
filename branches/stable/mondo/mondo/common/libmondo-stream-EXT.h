/* libmondo-stream-EXT.h */



extern int closein_tape(struct s_bkpinfo *bkpinfo);
extern int closeout_tape(struct s_bkpinfo *bkpinfo);
extern int find_tape_device_and_size(char *dev, char *siz);
extern void insist_on_this_tape_number(int tapeno);
extern void log_tape_pos(void);
extern int maintain_collection_of_recent_archives(char *td,
												  char *latest_fname);
extern int openin_cdstream(struct s_bkpinfo *bkpinfo);
extern int openin_tape(struct s_bkpinfo *bkpinfo);
extern int openout_cdstream(char *cddev, int speed);
extern int openout_tape(char *tapedev, long internal_tape_block_size);
extern int read_file_from_stream_to_file(struct s_bkpinfo *bkpinfo,
										 char *outfile, long long size);
extern int read_file_from_stream_to_stream(struct s_bkpinfo *bkpinfo,
										   FILE * fout, long long size);
extern int read_file_from_stream_FULL(struct s_bkpinfo *bkpinfo,
									  char *outfname, FILE * foutstream,
									  long long orig_size);
extern int read_header_block_from_stream(long long *plen, char *filename,
										 int *pcontrol_char);
extern int register_in_tape_catalog(t_archtype type, int number, long aux,
									char *fn);
extern bool should_we_write_to_next_tape(long mediasize,
										 off_t
										 length_of_incoming_file);
extern int skip_incoming_files_until_we_find_this_one(char
													  *the_file_I_was_reading);
extern int start_to_read_from_next_tape(struct s_bkpinfo *bkpinfo);
extern int start_to_write_to_next_tape(struct s_bkpinfo *bkpinfo);
extern int write_backcatalog_to_tape(struct s_bkpinfo *bkpinfo);
extern int write_data_disks_to_stream(char *fname);
extern int write_file_to_stream_from_file(struct s_bkpinfo *bkpinfo,
										  char *infile);
extern int write_header_block_to_stream(off_t length_of_incoming_file,
										char *filename, int control_char);
extern void wrong_marker(int should_be, int it_is);
extern int closein_cdstream(struct s_bkpinfo *bkpinfo);
extern int read_EXAT_files_from_tape(struct s_bkpinfo *bkpinfo,
									 long long *ptmp_size, char *tmp_fname,
									 int *pctrl_chr, char *xattr_fname,
									 char *acl_fname);
extern int write_EXAT_files_to_tape(struct s_bkpinfo *bkpinfo,
									char *xattr_fname, char *acl_fname);
