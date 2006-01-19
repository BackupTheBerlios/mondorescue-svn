/* libmondo-raid-EXT.h */

#ifdef __FreeBSD__
#define raid_device_record vinum_volume
#endif

extern bool is_this_raid_personality_registered(int raidno);
extern int which_raid_device_is_using_this_partition(struct raidlist_itself
													 *raidlist,
													 char *device);
extern void write_variableINT_to_raid_var_line(struct raid_device_record
											   *raidrec, int lino,
											   char *label, int value);

extern int where_in_drivelist_is_drive(struct list_of_disks *disklist,
									   char *device);



extern int load_raidtab_into_raidlist(struct raidlist_itself *, char *);
extern int save_raidlist_to_raidtab(struct raidlist_itself *, char *);
extern void process_raidtab_line(FILE *, struct raid_device_record *,
								 char *, char *);
extern int save_raidlist_to_raidtab(struct raidlist_itself *raidlist,
									char *fname);
extern void save_raidrec_to_file(struct raid_device_record *raidrec,
								 FILE * fout);

extern void
save_disklist_to_file(char *listname,
					  struct list_of_disks *disklist, FILE * fout);


#ifdef __FreeBSD__
extern void add_disk_to_raid_device(struct vinum_plex *p,
									char *device_to_add);
extern void add_plex_to_volume(struct vinum_volume *v, int raidlevel,
							   int stripesize);
extern void add_disk_to_raid_device(struct vinum_plex *p,
									char *device_to_add);
extern long long size_spec(char *spec);
extern bool get_option_state(int argc, char **argv, char *option);
extern char **get_option_vals(int argc, char **argv, char *option,
							  int nval);
extern char *get_option_val(int argc, char **argv, char *option);
extern char **get_next_vinum_conf_line(FILE * f, int *argc);
extern void add_plex_to_volume(struct vinum_volume *v, int raidlevel,
							   int stripesize);
#undef raid_device_record
#else
extern void add_disk_to_raid_device(struct list_of_disks *disklist,
									char *device_to_add, int index);
#endif

extern int create_raidtab_from_mdstat(char *, char *);
extern int read_mdstat(struct s_mdstat *mdstat, char *mdstat_file);

extern int create_raidtab_from_mdstat(char *raidtab_fname,
									  char *mdstat_fname);
