/*
 * $Id: libmondo-string-EXT.h,v 1.2 2004/06/10 15:29:12 hugo Exp $
 */

extern char *build_partition_name (char *partition, const char *drive, int partno);
extern void center_string (char *in_out, int width);
extern char *commarize (char*);
extern char *disklist_entry_to_string (struct list_of_disks *disklist, int lino);
extern long friendly_sizestr_to_sizelong (char *incoming);
extern char *leftpad_string (char *incoming, int width);
extern char *marker_to_string (int marker);
extern char *mountlist_entry_to_string (struct mountlist_itself *mountlist, int lino);
extern char *number_of_disks_as_string (int noof_disks, char *label);
extern char *number_to_text (int i);
extern void resolve_naff_tokens (char *output, char *ip, char *value, char *token);
extern char *slice_fname (long bigfileno, long sliceno, char *path, char *s);
extern int special_dot_char (int i);
extern bool spread_flaws_across_three_lines (char *flaws_str, char *flaws_str_A,
				 char *flaws_str_B, char *flaws_str_C,
				 int res);
extern int strcmp_inc_numbers (char *stringA, char *stringB);
extern char * strip_afio_output_line (char *input);
extern void strip_spaces (char *in_out);
extern char *trim_empty_quotes (char *incoming);
extern char *truncate_to_drive_name (char *partition);
extern char *turn_raid_level_number_to_string (int raid_level);

extern void printf_silly_message(void);

extern int compare_two_filelist_entries(void*va,void*vb);
extern int severity_of_difference(char *filename, char *out_reason);

extern char *percent_media_full_comment (struct s_bkpinfo *bkpinfo);


extern char *media_descriptor_string(t_bkptype);

extern inline void turn_wildcard_chars_into_literal_chars(char*out, char*in);

