/* mondo-restore-EXT.h */

#ifdef __FreeBSD__
#define raid_device_record vinum_volume
#endif

extern bool g_sigpipe_caught;
extern bool g_ISO_restore_mode;	/* are we in Iso Mode? */
extern bool g_I_have_just_nuked;
extern char *g_tmpfs_mountpt;
extern char *g_isodir_device;
extern char *g_isodir_format;

extern struct s_bkpinfo *g_bkpinfo_DONTUSETHIS;
extern char *g_biggielist_txt;
extern char *g_filelist_full;
extern char *g_biggielist_pot;
extern char *g_filelist_potential;
extern char *g_filelist_imagedevs;
extern char *g_filelist_restthese;
extern char *g_biggielist_restthese;
extern char *g_imagedevs_restthese;
extern char *g_mondo_cfg_file;
extern char *g_mountlist_fname;
extern char *g_mondo_home;


extern void ask_about_these_imagedevs(char *, char *);
extern int catchall_mode(struct s_bkpinfo *, struct mountlist_itself *,
						 struct raidlist_itself *);
extern void sort_mountlist_by_device(struct mountlist_itself *);
extern void find_pathname_of_executable_preferably_in_RESTORING(char *,
																char *,
																char *);
extern int interactive_mode(struct s_bkpinfo *, struct mountlist_itself *,
							struct raidlist_itself *);
extern int nuke_mode(struct s_bkpinfo *, struct mountlist_itself *,
					 struct raidlist_itself *);
extern int compare_mode(struct s_bkpinfo *, struct mountlist_itself *,
						struct raidlist_itself *);
extern int iso_mode(struct s_bkpinfo *bkpinfo,
					struct mountlist_itself *mountlist,
					struct raidlist_itself *raidlist, bool nuke_me_please);
extern int load_mountlist(struct mountlist_itself *, char *);
extern int load_raidtab_into_raidlist(struct raidlist_itself *, char *);
extern int restore_mode(struct s_bkpinfo *, struct mountlist_itself *,
						struct raidlist_itself *);
extern int save_raidlist_to_raidtab(struct raidlist_itself *, char *);
extern void process_raidtab_line(FILE *, struct raid_device_record *,
								 char *, char *);
extern int restore_a_biggiefile_from_CD(struct s_bkpinfo *, long,
										struct s_node *);
extern int restore_a_biggiefile_from_stream(struct s_bkpinfo *, char *,
											long, char *, long long,
											struct s_node *);
extern int restore_a_tarball_from_CD(char *, int, struct s_node *);
extern int restore_a_tarball_from_stream(struct s_bkpinfo *, char *, int,
										 struct s_node *, long long);
extern int restore_all_biggiefiles_from_CD(struct s_bkpinfo *,
										   struct s_node *);
extern int restore_all_biggiefiles_from_stream(struct s_bkpinfo *,
											   struct s_node *);
extern int restore_all_tarballs_from_CD(struct s_bkpinfo *,
										struct s_node *);
extern int restore_all_tarballs_from_stream(struct s_bkpinfo *,
											struct s_node *);
extern int restore_everything(struct s_bkpinfo *, struct s_node *);
extern int restore_live_from_monitas_server(struct s_bkpinfo *, char *,
											char *, char *);
extern int restore_to_live_filesystem(struct s_bkpinfo *);
extern void swap_mountlist_entries(struct mountlist_itself *, int, int);
extern void sort_mountlist_by_mountpoint(struct mountlist_itself *, bool);
extern void sort_mountlist_by_device(struct mountlist_itself *);
extern void twenty_seconds_til_yikes(void);
extern int run_raw_mbr(bool offer_to_hack_scripts, char *bd);
extern int save_mountlist_to_disk(struct mountlist_itself *, char *);
extern void save_raidrec_to_file(struct raid_device_record *raidrec,
								 FILE * fout);
extern int save_raidlist_to_raidtab(struct raidlist_itself *raidlist,
									char *fname);
extern int what_number_cd_is_this(struct s_bkpinfo *);

#ifdef __FreeBSD__
#undef raid_device_record
#endif
