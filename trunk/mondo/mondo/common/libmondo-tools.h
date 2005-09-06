	/* libmondo-tools.h
 * $Id: libmondo-tools.h,v 1.2 2004/06/10 15:29:12 hugo Exp $
 */

void clean_up_KDE_desktop_if_necessary(void);

long get_time ();
extern void (*log_debug_msg) (int debug_level, const char *szFile,
			      const char *szFunction, int nLine, const char *fmt, ...);
void standard_log_debug_msg (int debug_level, const char *szFile,
			     const char *szFunction, int nLine, const char *fmt, ...);
int read_cfg_var (char *config_file, char *label, char *value);
int write_cfg_var (char *config_file, char *label, char *value);
void reset_bkpinfo (struct s_bkpinfo *bkpinfo);
#ifdef __FreeBSD__
void initialize_raidrec (struct vinum_volume *vv);
#else
void initialize_raidrec (struct raid_device_record *raidrec);
#endif
void log_trace (char *o);
int some_basic_system_sanity_checks ();


void insmod_crucial_modules(void);
int find_and_store_mondoarchives_home(char *home_sz);


void unmount_supermounts_if_necessary(void);
void remount_supermounts_if_necessary(void);

int post_param_configuration (struct s_bkpinfo *bkpinfo);


int pre_param_configuration(struct s_bkpinfo *bkpinfo);

long free_space_on_given_partition(char*partition);



void mount_boot_if_necessary(void);
void unmount_boot_if_necessary(void);

void restart_autofs_if_necessary(void);
void malloc_libmondo_global_strings(void);
void free_libmondo_global_strings(void);

double get_kernel_version();
char * get_architecture();
bool does_nonMS_partition_exist (void);
void stop_magicdev_if_necessary(void);
void restart_magicdev_if_necessary(void);
void stop_autofs_if_necessary(void);
void restart_autofs_if_necessary(void);


