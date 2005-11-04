/* libmondo-mountlist-EXT.h */

extern int evaluate_drive_within_mountlist (struct mountlist_itself *mountlist,
				 char *drive, char *flaws_str);
extern int evaluate_mountlist (struct mountlist_itself *mountlist, char *flaws_str_A,
		    char *flaws_str_B, char *flaws_str_C);
extern int find_device_in_mountlist (struct mountlist_itself *mountlist, char *device);
extern int look_for_duplicate_mountpoints (struct mountlist_itself *mountlist, char *flaws_str);
extern int look_for_weird_formats (struct mountlist_itself *mountlist, char *flaws_str);
extern int make_list_of_drives_in_mountlist(struct mountlist_itself*,struct list_of_disks*);
extern long long size_of_specific_device_in_mountlist (struct mountlist_itself *mountlist, char *device);




extern int load_mountlist( struct mountlist_itself *mountlist, char *fname);
extern void sort_mountlist_by_device( struct mountlist_itself *mountlist);
extern int save_mountlist_to_disk(struct mountlist_itself *mountlist, char *fname);
extern void sort_mountlist_by_mountpoint(struct mountlist_itself *mountlist, bool reverse);
extern void  sort_mountlist_by_device( struct mountlist_itself *mountlist);
extern void swap_mountlist_entries(struct mountlist_itself *mountlist, int a, int b);
