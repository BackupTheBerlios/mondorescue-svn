/*
 * $Id$
 */

extern bool am_I_in_disaster_recovery_mode(void);
extern bool does_device_exist(char *device);
extern int does_partition_exist(const char *drive, int partno);
extern bool does_string_exist_in_boot_block(char *dev, char *str);
extern int find_and_mount_actual_cd(struct s_bkpinfo *bkpinfo,
									char *mountpoint);
extern int find_cdrom_device(char *output, bool try_to_mount);
extern int find_dvd_device(char *output, bool try_to_mount);
extern long get_phys_size_of_drive(char *drive);
extern bool is_this_a_valid_disk_format(char *format);
extern bool is_this_device_mounted(char *device_raw);
extern int find_device_in_mountlist(struct mountlist_itself *mountlist,
									char *device);
extern int mount_CDROM_here(char *device, char *mountpoint);
extern long long size_of_specific_device_in_mountlist(struct
													  mountlist_itself
													  *mountlist,
													  char *device);
extern char *where_is_root_mounted(void);
extern char *make_vn(char *file);
extern int kick_vn(char *vn);
extern char which_boot_loader(char *which_device);



extern int find_cdrw_device(char *cdrw_device);


extern int interactively_obtain_media_parameters_from_user(struct s_bkpinfo
														   *, bool);


extern void make_fifo(char *store_name_here, char *stub);


extern void insist_on_this_cd_number(struct s_bkpinfo *bkpinfo,
									 int cd_number_i_want);


extern int what_number_cd_is_this(struct s_bkpinfo *bkpinfo);


extern int eject_device(char *);

extern char *list_of_NFS_mounts_only(void);

extern void sensibly_set_tmpdir_and_scratchdir(struct s_bkpinfo *bkpinfo);


extern bool set_dev_to_this_if_rx_OK(char *, char *);

extern void retract_CD_tray_and_defeat_autorun(void);

extern bool does_string_exist_in_first_N_blocks(char *dev, char *str,
												int n);

extern int inject_device(char *dev);
extern bool does_nonMS_partition_exist(void);
extern char *resolve_softlinks_to_get_to_actual_device_file(char
															*incoming);

extern void set_g_cdrom_and_g_dvd_to_bkpinfo_value(struct s_bkpinfo
												   *bkpinfo);

extern bool is_dev_an_NTFS_dev(char *bigfile_fname);

extern char *which_partition_format(const char *drive);
