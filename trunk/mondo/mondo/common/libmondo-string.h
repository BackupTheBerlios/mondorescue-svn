/*
 * $Id: libmondo-string.h,v 1.2 2004/06/10 15:29:12 hugo Exp $
 */

char *build_partition_name (char *partition, const char *drive, int partno);
void center_string (char *in_out, int width);
char *commarize(char*);
char *disklist_entry_to_string (struct list_of_disks *disklist, int lino);
long friendly_sizestr_to_sizelong (char *incoming);
char *leftpad_string (char *incoming, int width);
char *marker_to_string (int marker);
char *mountlist_entry_to_string (struct mountlist_itself *mountlist, int lino);
char *number_of_disks_as_string (int noof_disks, char *label);
char *number_to_text (int i);
void resolve_naff_tokens (char *output, char *ip, char *value, char *token);
char *slice_fname (long bigfileno, long sliceno, char *path, char *s);
int special_dot_char (int i);
bool spread_flaws_across_three_lines (char *flaws_str, char *flaws_str_A,
				 char *flaws_str_B, char *flaws_str_C,
				 int res);
int strcmp_inc_numbers (char *stringA, char *stringB);
char * strip_afio_output_line (char *input);
void strip_spaces (char *in_out);
char *trim_empty_quotes (char *incoming);
char *truncate_to_drive_name (char *partition);
char *turn_raid_level_number_to_string (int raid_level);
void printf_silly_message(void);

int compare_two_filelist_entries(void*va,void*vb);
int severity_of_difference(char *filename, char *out_reason);

char *percent_media_full_comment (struct s_bkpinfo *bkpinfo);
char *media_descriptor_string(t_bkptype);
inline void turn_wildcard_chars_into_literal_chars(char*out, char*in);
