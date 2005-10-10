/* mondo-restore.h
 * $Id$
 */

/**************************************************************************
 * Prototypes:                                                            *
 **************************************************************************/

void ask_about_these_imagedevs(char *, char *);
int catchall_mode(struct s_bkpinfo *, struct mountlist_itself *,
				  struct raidlist_itself *);
void find_pathname_of_executable_preferably_in_RESTORING(char *, char *,
														 char *);
int interactive_mode(struct s_bkpinfo *, struct mountlist_itself *,
					 struct raidlist_itself *);
int nuke_mode(struct s_bkpinfo *, struct mountlist_itself *,
			  struct raidlist_itself *);
int compare_mode(struct s_bkpinfo *, struct mountlist_itself *,
				 struct raidlist_itself *);
int iso_mode(struct s_bkpinfo *bkpinfo, struct mountlist_itself *mountlist,
			 struct raidlist_itself *raidlist, bool nuke_me_please);
int restore_mode(struct s_bkpinfo *, struct mountlist_itself *,
				 struct raidlist_itself *);
int restore_a_biggiefile_from_CD(struct s_bkpinfo *, long, struct s_node *,
								 char *);
int restore_a_biggiefile_from_stream(struct s_bkpinfo *, char *, long,
									 char *, long long, struct s_node *,
									 int, char *);
int restore_a_tarball_from_CD(char *, long, struct s_node *);
int restore_a_tarball_from_stream(struct s_bkpinfo *, char *, long,
								  struct s_node *, long long, char *,
								  char *);
int restore_all_biggiefiles_from_CD(struct s_bkpinfo *, struct s_node *);
int restore_all_biggiefiles_from_stream(struct s_bkpinfo *,
										struct s_node *);
int restore_all_tarballs_from_CD(struct s_bkpinfo *, struct s_node *);
int restore_all_tarballs_from_stream(struct s_bkpinfo *, struct s_node *);
int restore_everything(struct s_bkpinfo *, struct s_node *);
int restore_live_from_monitas_server(struct s_bkpinfo *, char *, char *,
									 char *);
int restore_to_live_filesystem(struct s_bkpinfo *);
void swap_mountlist_entries(struct mountlist_itself *, int, int);
void sort_mountlist_by_mountpoint(struct mountlist_itself *, bool);
void sort_mountlist_by_device(struct mountlist_itself *);
int what_number_cd_is_this(struct s_bkpinfo *);
