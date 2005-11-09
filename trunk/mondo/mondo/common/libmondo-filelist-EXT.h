/* libmondo-filelist-EXT.h */

extern int chop_filelist(char *filelist, char *outdir, long maxsetsizeK);
extern int call_filelist_chopper(struct s_bkpinfo *bkpinfo);
extern void free_filelist(struct s_node *filelist);
extern int get_last_filelist_number(struct s_bkpinfo *bkpinfo);
/* BERLIOS: Useless ?
extern int add_string_at_node(struct s_node *startnode,
							  char *string_to_add);
							  */
extern struct s_node *load_filelist(char *filelist_fname);
extern void reload_filelist(struct s_node *filelist);
extern void save_filelist(struct s_node *filelist, char *outfname);
extern void toggle_all_root_dirs_on(struct s_node *filelist);
extern void toggle_path_expandability(struct s_node *filelist,
									  char *pathname, bool on_or_off);
extern void toggle_path_selection(struct s_node *filelist, char *pathname,
								  bool on_or_off);
extern void toggle_node_selection(struct s_node *filelist, bool on_or_off);
extern int prepare_filelist(struct s_bkpinfo *bkpinfo);

extern long save_filelist_entries_in_common(char *needles_list_fname,
											struct s_node *filelist,
											char *matches_fname,
											bool use_star);
extern struct s_node *find_string_at_node(struct s_node *startnode,
										  char *string_to_find);

extern int add_list_of_files_to_filelist(struct s_node *filelist,
										 char *list_of_files_fname,
										 bool flag_em);

extern void show_filelist(struct s_node *node);
extern int get_fattr_list(char *filelist, char *fattr_fname);
extern int get_acl_list(char *filelist, char *acl_fname);
extern int set_fattr_list(char *masklist, char *fattr_fname);
extern int set_acl_list(char *masklist, char *acl_fname);
