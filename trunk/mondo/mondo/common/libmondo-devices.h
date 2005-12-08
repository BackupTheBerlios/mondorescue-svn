/*
 * $Id$
 */

bool am_I_in_disaster_recovery_mode(void);
bool does_device_exist(char *device);
int does_partition_exist(const char *drive, int partno);
bool does_string_exist_in_boot_block(char *dev, char *str);
int find_and_mount_actual_cd(struct s_bkpinfo *bkpinfo, char *mountpoint);
char *find_cdrom_device(bool try_to_mount);
char *find_dvd_device(void);
long get_phys_size_of_drive(char *drive);
bool is_this_a_valid_disk_format(char *format);
int find_device_in_mountlist(struct mountlist_itself *mountlist,
							 char *device);
bool is_this_device_mounted(char *device_raw);
char *where_is_root_mounted(void);
char *make_vn(char *file);
int kick_vn(char *vn);
char which_boot_loader(char *which_device);



char *find_cdrw_device(void);

int interactively_obtain_media_parameters_from_user(struct s_bkpinfo *,
													bool);



void make_fifo(char *store_name_here, char *stub);

void insist_on_this_cd_number(struct s_bkpinfo *bkpinfo,
							  int cd_number_i_want);

int what_number_cd_is_this(struct s_bkpinfo *bkpinfo);

int eject_device(char *);

char *list_of_NFS_mounts_only();

void sensibly_set_tmpdir_and_scratchdir(struct s_bkpinfo *bkpinfo);


bool set_dev_to_this_if_rx_OK(char *output, char *dev);


void retract_CD_tray_and_defeat_autorun(void);
bool does_string_exist_in_first_N_blocks(char *dev, char *str, int n);

int inject_device(char *dev);

bool does_nonMS_partition_exist(void);
char *resolve_softlinks_to_get_to_actual_device_file(char *incoming);
void set_g_cdrom_and_g_dvd_to_bkpinfo_value(struct s_bkpinfo *bkpinfo);

bool is_dev_an_NTFS_dev(char *bigfile_fname);

char *which_partition_format(const char *drive);
