/* libmondo-filelist.h
 * $Id: libmondo-filelist.h,v 1.2 2004/06/10 15:29:12 hugo Exp $
 */

int chop_filelist (char *filelist, char *outdir, long maxsetsizeK);
int call_filelist_chopper (struct s_bkpinfo *bkpinfo);
void free_filelist (struct s_node *filelist);
int get_last_filelist_number (struct s_bkpinfo *bkpinfo);
int add_string_at_node (struct s_node *startnode, char *string_to_add);
struct s_node *load_filelist (char *filelist_fname);
void reload_filelist (struct s_node *filelist);
void save_filelist (struct s_node *filelist, char *outfname);
void toggle_all_root_dirs_on (struct s_node *filelist);
void toggle_path_expandability (struct s_node *filelist, char *pathname, bool on_or_off);
void toggle_path_selection (struct s_node *filelist, char *pathname, bool on_or_off);
void toggle_node_selection (struct s_node *filelist, bool on_or_off);
int prepare_filelist (struct s_bkpinfo *bkpinfo);

long save_filelist_entries_in_common(
			char*needles_list_fname,
			struct s_node *filelist,
			char*matches_fname, bool use_star);
struct s_node *find_string_at_node (struct s_node *startnode, char *string_to_find);

int add_list_of_files_to_filelist(struct s_node *filelist, char*list_of_files_fname, bool flag_em);
void show_filelist (struct s_node *node);
int get_fattr_list(char*filelist, char*fattr_fname);
int get_acl_list(char*filelist, char*acl_fname);
int set_fattr_list(char*masklist, char*fattr_fname);
int set_acl_list(char*masklist, char*acl_fname);
