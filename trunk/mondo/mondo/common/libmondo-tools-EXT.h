/* libmondo-tools-EXT.h */

extern void clean_up_KDE_desktop_if_necessary(void);
extern long get_time();
extern void (*log_debug_msg) (int debug_level, const char *szFile,
							  const char *szFunction, int nLine,
							  const char *fmt, ...);
extern void standard_log_debug_msg(int debug_level, const char *szFile,
								   const char *szFunction, int nLine,
								   const char *fmt, ...);
extern int read_cfg_var(char *config_file, char *label, char *value);
extern int write_cfg_var(char *config_file, char *label, char *value);
extern void reset_bkpinfo(struct s_bkpinfo *bkpinfo);
#ifdef __FreeBSD__
extern void initialize_raidrec(struct vinum_volume *vv);
#else
extern void initialize_raidrec(struct raid_device_record *raidrec);
#endif
extern int some_basic_system_sanity_checks();


extern int g_loglevel;


extern void insmod_crucial_modules(void);
extern char *find_and_store_mondoarchives_home(void);

extern void unmount_supermounts_if_necessary(void);
extern void remount_supermounts_if_necessary(void);

extern int post_param_configuration(struct s_bkpinfo *bkpinfo);


extern int pre_param_configuration(struct s_bkpinfo *bkpinfo);


extern void mount_boot_if_necessary(void);
extern void unmount_boot_if_necessary(void);
extern void restart_autofs_if_necessary(void);
extern void malloc_libmondo_global_strings(void);
extern void free_libmondo_global_strings(void);

extern double get_kernel_version();
extern char *get_architecture();

extern bool does_nonMS_partition_exist(void);


extern void stop_magicdev_if_necessary(void);
extern void restart_magicdev_if_necessary(void);
extern void stop_autofs_if_necessary(void);
extern void restart_autofs_if_necessary(void);
