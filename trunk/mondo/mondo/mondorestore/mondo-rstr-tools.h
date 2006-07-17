/*
 * $Id: mondo-rstr-tools.h
**/

void free_global_filenames();
void get_cfg_file_from_archive_or_bust(struct s_bkpinfo *);
bool is_file_in_list(char *, char *, char *);	/* needle, haystack, preamble */
int iso_fiddly_bits(struct s_bkpinfo *bkpinfo, bool nuke_me_please);
void kill_petris(void);
int mount_cdrom(struct s_bkpinfo *bkpinfo);
int mount_device(char *, char *, char *, bool);
int mount_all_devices(struct mountlist_itself *, bool);
void protect_against_braindead_sysadmins(void);
int read_cfg_file_into_bkpinfo(char *cfg_file, struct s_bkpinfo *bkpinfo);
struct s_node *process_filelist_and_biggielist(struct s_bkpinfo *);
int backup_crucial_file(char *path_root, char *filename);

int run_boot_loader(bool);
int run_grub(bool, char *);
int run_lilo(bool);
int run_elilo(bool);
int run_raw_mbr(bool offer_to_hack_scripts, char *bd);
char *find_my_editor(void);
void streamline_changes_file(char *, char *);
void set_signals(int on);
void setup_global_filenames(struct s_bkpinfo *bkpinfo);
void twenty_seconds_til_yikes(void);
int run_raw_mbr(bool offer_to_hack_scripts, char *bd);
void terminate_daemon(int);
void termination_in_progress(int);
int unmount_all_devices(struct mountlist_itself *);
int get_cfg_file_from_archive(struct s_bkpinfo *bkpinfo);
void ask_about_these_imagedevs(char *infname, char *outfname);
