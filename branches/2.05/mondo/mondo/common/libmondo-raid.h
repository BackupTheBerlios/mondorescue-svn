/* libmondo-raid.h
 * $Id$
 */

#ifdef __FreeBSD__
#define raid_device_record vinum_volume
#endif

bool is_this_raid_personality_registered(int raidno);
int which_raid_device_is_using_this_partition(struct raidlist_itself
											  *raidlist, char *device);
void write_variableINT_to_raid_var_line(struct raid_device_record *raidrec,
										int lino, char *label, int value);
int where_in_drivelist_is_drive(struct list_of_disks *disklist,
								char *device);

int load_raidtab_into_raidlist(struct raidlist_itself *, char *);
int save_raidlist_to_raidtab(struct raidlist_itself *, char *);
void process_raidtab_line(FILE *, struct raid_device_record *, char *,
						  char *);
int save_raidlist_to_raidtab(struct raidlist_itself *raidlist,
							 char *fname);
void save_raidrec_to_file(struct raid_device_record *raidrec, FILE * fout);
void
save_disklist_to_file(char *listname,
					  struct list_of_disks *disklist, FILE * fout);
#ifdef __FreeBSD__
void add_disk_to_raid_device(struct vinum_plex *p, char *device_to_add);
void add_plex_to_volume(struct vinum_volume *v, int raidlevel,
						int stripesize);
void add_disk_to_raid_device(struct vinum_plex *p, char *device_to_add);
long long size_spec(char *spec);
bool get_option_state(int argc, char **argv, char *option);
char **get_option_vals(int argc, char **argv, char *option, int nval);
char *get_option_val(int argc, char **argv, char *option);
char **get_next_vinum_conf_line(FILE * f, int *argc);
void add_plex_to_volume(struct vinum_volume *v, int raidlevel,
						int stripesize);
#undef raid_device_record
#else
void add_disk_to_raid_device(struct list_of_disks *disklist,
							 char *device_to_add, int index);
#endif


int create_raidtab_from_mdstat(char *, char *);
int read_mdstat(struct s_mdstat *mdstat, char *mdstat_file);

int create_raidtab_from_mdstat(char *raidtab_fname, char *mdstat_fname);
