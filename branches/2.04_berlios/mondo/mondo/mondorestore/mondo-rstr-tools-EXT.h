/* mondo-rstr-tools-EXT.h
 * $Id: mondo-rstr-tools-EXT.h
 */

extern void free_MR_global_filenames();
extern void get_cfg_file_from_archive_or_bust(struct s_bkpinfo*);
extern bool is_file_in_list(char*, char*, char*); /* needle, haystack, preamble */
extern int iso_fiddly_bits(struct s_bkpinfo *bkpinfo, bool nuke_me_please);
extern void kill_petris(void);
extern int modify_rclocal_one_time( char *path );
extern int mount_cdrom(struct s_bkpinfo *bkpinfo);
extern int mount_device(char*,char*,char*,bool);
extern int mount_all_devices(struct mountlist_itself*, bool);
extern void protect_against_braindead_sysadmins(void);
extern int read_cfg_file_into_bkpinfo( char* cfg_file, struct s_bkpinfo *bkpinfo);
struct s_node *process_filelist_and_biggielist(struct s_bkpinfo *);
extern int backup_crucial_file(char *path_root, char*filename);
extern int run_boot_loader(bool);
extern int run_grub(bool, char*);
extern int run_lilo(bool);
extern int run_elilo(bool);
extern int run_raw_mbr(bool offer_to_hack_scripts, char *bd);
extern char *find_my_editor(void);

extern void streamline_changes_file(char*,char*);
extern void  set_signals( int on );
extern void setup_MR_global_filenames(struct s_bkpinfo *bkpinfo);
//extern void setup_signals(int);
extern void terminate_daemon(int);
extern void termination_in_progress(int);
extern int unmount_all_devices(struct mountlist_itself*);
extern int get_cfg_file_from_archive(struct s_bkpinfo *bkpinfo);
extern int 
extract_config_file_from_ramdisk( struct s_bkpinfo *bkpinfo, 
				  char *ramdisk_fname, 
				  char *output_cfg_file, 
				  char *output_mountlist_file);

extern void ask_about_these_imagedevs( char *infname, char *outfname );

extern void wait_until_software_raids_are_prepped(char*mdstat_file, int wait_for_percentage);
