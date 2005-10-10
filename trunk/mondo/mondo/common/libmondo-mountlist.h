/* libmondo-mountlist.h
 * $Id$
 */


int evaluate_drive_within_mountlist(struct mountlist_itself *mountlist,
									char *drive, char *flaws_str);
int evaluate_mountlist(struct mountlist_itself *mountlist,
					   char *flaws_str_A, char *flaws_str_B,
					   char *flaws_str_C);
int find_device_in_mountlist(struct mountlist_itself *mountlist,
							 char *device);
int look_for_duplicate_mountpoints(struct mountlist_itself *mountlist,
								   char *flaws_str);
int look_for_weird_formats(struct mountlist_itself *mountlist,
						   char *flaws_str);
int make_list_of_drives_in_mountlist(struct mountlist_itself *,
									 struct list_of_disks *);
long long size_of_specific_device_in_mountlist(struct mountlist_itself
											   *mountlist, char *device);


int load_mountlist(struct mountlist_itself *mountlist, char *fname);
void sort_mountlist_by_device(struct mountlist_itself *mountlist);
int save_mountlist_to_disk(struct mountlist_itself *mountlist,
						   char *fname);
void sort_mountlist_by_mountpoint(struct mountlist_itself *mountlist,
								  bool reverse);
void sort_mountlist_by_device(struct mountlist_itself *mountlist);
void swap_mountlist_entries(struct mountlist_itself *mountlist, int a,
							int b);
