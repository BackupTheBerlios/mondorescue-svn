/* 
   $Id$
*/


/**
 * @file
 * Functions for handling strings.
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-string.h"
#include "lib-common-externs.h"
#include "libmondo-files-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-tools-EXT.h"

/*@unused@*/
//static char cvsid[] = "$Id$";

extern int g_current_media_number;
extern long long g_tape_posK;

/**
 * @addtogroup stringGroup
 * @{
 */
/**
 * Build a partition name from a drive and a partition number.
 * @param drive The drive basename of the partition name (e.g. /dev/hda)
 * @param partno The partition number (e.g. 1)
 * @param partition Where to put the partition name (e.g. /dev/hda1)
 * @return @p partition.
 * @note If @p drive ends in a digit, then 'p' (on Linux) or 's' (on *BSD) is added before @p partno.
 */
char *build_partition_name(char *partition, const char *drive, int partno)
{
	char *p, *c;

	assert(partition != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drive);
	assert(partno >= 0);

	p = strcpy(partition, drive);
	/* is this a devfs device path? */
	c = strrchr(partition, '/');
	if (c && strncmp(c, "/disc", 5) == 0) {
		/* yup it's devfs, return the "part" path */
		strcpy(c + 1, "part");
		p = c + 5;
	} else {
		p += strlen(p);
		if (isdigit(p[-1])) {
			*p++ =
#ifdef BSD
				's';
#else
				'p';
#endif
		}
	}
	sprintf(p, "%d", partno);
	return (partition);
}


/**
 * Pad a string on both sides so it appears centered.
 * @param in_out The string to be center-padded (modified). The caller needs to free this string
 * @param width The width of the final result.
 */
void center_string(char *in_out, int width)
{
	char *scratch;
	char *out;
	char *p;
	int i;						/* purpose */
	int len;					/* purpose */
	int mid;					/* purpose */
	int x;						/* purpose */

	assert(in_out != NULL);
	assert(width > 2);

	if (strlen(in_out) == 0) {
		return;
	}
	for (p = in_out; *p == ' '; p++);
	asprintf(&scratch, p);
	len = (int) strlen(scratch);
	mid = width / 2;
	x = mid - len / 2;
	for (i = 0; i < x; i++) {
		in_out[i] = ' ';
	}
	in_out[i] = '\0';
	asprintf(&out, "%s%s", in_out, scratch);
	paranoid_free(scratch);
	in_out = out;
}


inline void turn_wildcard_chars_into_literal_chars(char *sout, char *sin)
{
	char *p, *q;

	for (p = sin, q = sout; *p != '\0'; *(q++) = *(p++)) {
		if (strchr("[]*?", *p)) {
			*(q++) = '\\';
		}
	}
	*q = *p;					// for the final '\0'
}


/*
 * Add commas every third place in @p input.
 * @param input The string to commarize.
 * @return The string with commas.
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *commarize(char *input)
{
	char *pos_w_commas;
	static char output[MAX_STR_LEN];
	char *tmp;
	int j;

	assert(input != NULL);

	asprintf(&tmp, "%s", input);
	if (strlen(tmp) > 6) {
		asprintf(&pos_w_commas, "%s", tmp);
		j = (int) strlen(pos_w_commas);
		tmp[j - 6] = ',';
		strcpy(tmp + j - 5, pos_w_commas + j - 6);
		paranoid_free(pos_w_commas);
		asprintf(&pos_w_commas, "%s", tmp);
	}
	if (strlen(tmp) > 3) {
		j = (int) strlen(tmp);
		asprintf(&pos_w_commas, "%s", tmp);
		pos_w_commas[j - 3] = ',';
		strcpy(pos_w_commas + j - 2, tmp + j - 3);
	} else {
		asprintf(&pos_w_commas, "%s", tmp);
	}
	strcpy(output, pos_w_commas);
	paranoid_free(pos_w_commas);
	paranoid_free(tmp);
	return (output);
}


/**
 * Turn an entry from the RAID editor's disklist into a GUI-friendly string.
 * The format is: the device left-aligned and padded to 24 places, followed by a space and the
 * index, right-aligned and padded to eight places. The total string length
 * is exactly 33.
 * @param disklist The disklist to operate on.
 * @param lino The line number from @p disklist to convert to a string.
 * @return The string form of the disklist entry.
 * @note The returned string points to static storage and will be overwritten with each call.
 */
char *disklist_entry_to_string(struct list_of_disks *disklist, int lino)
{

	/*@ buffers ********************************************************** */
	char *output;

	assert(disklist != NULL);

	asprintf(&output, "%-24s %8d", disklist->el[lino].device,
			 disklist->el[lino].index);
	return (output);
}


/**
 * Turn a "friendly" sizestring into a number of megabytes.
 * Supports the suffixes 'k'/'K', 'm'/'M', and 'g'/'G'. Calls
 * fatal_error() if an unknown suffix is encountered.
 * @param incoming The sizestring to convert (e.g. "40m", "2g").
 * @return The size in megabytes.
 */
long friendly_sizestr_to_sizelong(char *incoming)
{
	long outval;
	int i;
	char *tmp;
	char ch;

	assert_string_is_neither_NULL_nor_zerolength(incoming);

	if (!incoming[0]) {
		return (0);
	}
	if (strchr(incoming, '.')) {
		fatal_error("Please use integers only. No decimal points.");
	}
	asprintf(&tmp, "%s", incoming);
	i = (int) strlen(tmp);
	if (tmp[i - 1] == 'B' || tmp[i - 1] == 'b') {
		tmp[i - 1] = '\0';
	}
	for (i = 0; i < (int) strlen(tmp) && isdigit(tmp[i]); i++);
	ch = tmp[i];
	tmp[i] = '\0';
	outval = atol(tmp);
	paranoid_free(tmp);

	if (ch == 'g' || ch == 'G') {
		outval = outval * 1024;
	} else if (ch == 'k' || ch == 'K') {
		outval = outval / 1024;
	} else if (ch == 't' || ch == 'T')	// terabyte
	{
		outval *= 1048576;
	} else if (ch == 'Y' || ch == 'y')	// yottabyte - the biggest measure in the info file
	{
		log_it
			("Oh my gosh. You actually think a YOTTABYTE will get you anywhere? What're you going to do with 1,208,925,819,614,629,174,706,176 bytes of data?!?!");
		popup_and_OK
			("That sizespec is more than 1,208,925,819,614,629,174,706,176 bytes. You have a shocking amount of data. Please send a screenshot to the list :-)");
		fatal_error("Integer overflow.");
	} else if (ch != 'm' && ch != 'M') {
		asprintf(&tmp, "Re: parameter '%s' - bad multiplier ('%c')",
				 incoming, ch);
		fatal_error(tmp);
	}
	return (outval);
}


/**
 * Add spaces to the right of @p incoming to make it @p width characters wide.
 * @param incoming The string to left-pad.
 * @param width The width to pad it to.
 * @return The left-padded string.
 * @note The returned string points to static storage that will be overwritten with each call.
 * @bug Why does center_string() modify its argument but leftpad_string() returns a modified copy?
 */
/* BERLIOS; useless ?
char *leftpad_string(char *incoming, int width)
{
	char *output;

	int i;

	assert(incoming != NULL);
	assert(width > 2);

	asprintf(output, "%s", incoming);
	for (i = (int) strlen(output); i < width; i++) {
		output[i] = ' ';
	}
	output[i] = '\0';
	return (output);
}
*/



/**
 * Turn a marker byte (e.g. BLK_START_OF_BACKUP) into a string (e.g. "BLK_START_OF_BACKUP").
 * Unknown markers are identified as "BLK_UNKNOWN (%d)" where %d is the decimal value.
 * @param marker The marker byte to stringify.
 * @return @p marker as a string. this should be freed by the caller
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *marker_to_string(int marker)
{
	/*@ buffer ****************************************************** */
	char *outstr;


	/*@ end vars *************************************************** */

	switch (marker) {
	case BLK_START_OF_BACKUP:
		asprintf(&outstr, "%s", "BLK_START_OF_BACKUP");
		break;
	case BLK_START_OF_TAPE:
		asprintf(&outstr, "%s", "BLK_START_OF_TAPE");
		break;
	case BLK_START_AN_AFIO_OR_SLICE:
		asprintf(&outstr, "%s", "BLK_START_AN_AFIO_OR_SLICE");
		break;
	case BLK_STOP_AN_AFIO_OR_SLICE:
		asprintf(&outstr, "%s", "BLK_STOP_AN_AFIO_OR_SLICE");
		break;
	case BLK_START_AFIOBALLS:
		asprintf(&outstr, "%s", "BLK_START_AFIOBALLS");
		break;
	case BLK_STOP_AFIOBALLS:
		asprintf(&outstr, "%s", "BLK_STOP_AFIOBALLS");
		break;
	case BLK_STOP_BIGGIEFILES:
		asprintf(&outstr, "%s", "BLK_STOP_BIGGIEFILES");
		break;
	case BLK_START_A_NORMBIGGIE:
		asprintf(&outstr, "%s", "BLK_START_A_NORMBIGGIE");
		break;
	case BLK_START_A_PIHBIGGIE:
		asprintf(&outstr, "%s", "BLK_START_A_PIHBIGGIE");
		break;
	case BLK_START_EXTENDED_ATTRIBUTES:
		asprintf(&outstr, "%s", "BLK_START_EXTENDED_ATTRIBUTES");
		break;
	case BLK_STOP_EXTENDED_ATTRIBUTES:
		asprintf(&outstr, "%s", "BLK_STOP_EXTENDED_ATTRIBUTES");
		break;
	case BLK_START_EXAT_FILE:
		asprintf(&outstr, "%s", "BLK_START_EXAT_FILE");
		break;
	case BLK_STOP_EXAT_FILE:
		asprintf(&outstr, "%s", "BLK_STOP_EXAT_FILE");
		break;
	case BLK_START_BIGGIEFILES:
		asprintf(&outstr, "%s", "BLK_START_BIGGIEFILES");
		break;
	case BLK_STOP_A_BIGGIE:
		asprintf(&outstr, "%s", "BLK_STOP_A_BIGGIE");
		break;
	case BLK_END_OF_TAPE:
		asprintf(&outstr, "%s", "BLK_END_OF_TAPE");
		break;
	case BLK_END_OF_BACKUP:
		asprintf(&outstr, "%s", "BLK_END_OF_BACKUP");
		break;
	case BLK_ABORTED_BACKUP:
		asprintf(&outstr, "%s", "BLK_ABORTED_BACKUP");
		break;
	case BLK_START_FILE:
		asprintf(&outstr, "%s", "BLK_START_FILE");
		break;
	case BLK_STOP_FILE:
		asprintf(&outstr, "%s", "BLK_STOP_FILE");
		break;
	default:
		asprintf(&outstr, "%s", "BLK_UNKNOWN (%d)", marker);
		break;
	}
	return (outstr);
}


/**
 * Turn a line from the mountlist into a GUI-friendly string.
 * The format is as follows: the left-aligned @p device field padded to 24 places,
 * a space, the left-aligned @p mountpoint field again padded to 24 places, a space,
 * the left-aligned @p format field padded to 10 places, a space, and the right-aligned
 * @p size field (in MB) padded to 8 places. The total string length is exactly 69.
 * @param mountlist The mountlist to operate on.
 * @param lino The line number in @p mountlist to stringify.
 * @return The string form of <tt>mountlist</tt>-\>el[<tt>lino</tt>]. To be freed by the caller
 * @note The returned string points to static storage and will be overwritten with each call.
 */
char *mountlist_entry_to_string(struct mountlist_itself *mountlist,
								int lino)
{

	/*@ buffer *********************************************************** */
	char *output;

	assert(mountlist != NULL);

	asprintf(&output, "%-24s %-24s %-10s %8lld",
			 mountlist->el[lino].device, mountlist->el[lino].mountpoint,
			 mountlist->el[lino].format, mountlist->el[lino].size / 1024);
	return (output);
}


/**
 * Generate a friendly string containing "X blah blah disk(s)"
 * @param noof_disks The number of disks (the X).
 * @param label The "blah blah" part in the middle. If you leave this blank
 * there will be a weird double space in the middle, so pass *something*.
 * @return The string containing "X blah blah disk(s)". To be  freed by the caller
 * @note The returned string points to static storage and will be overwritten with each call.
 */
char *number_of_disks_as_string(int noof_disks, char *label)
{

	/*@ buffers ********************************************************* */
	char *output;

	/*@ char     ******************************************************** */
	char p;

	assert(label != NULL);

	if (noof_disks > 1) {
		p = 's';
	} else {
		p = ' ';
	}
	asprintf(&output, "%d %s disk%c    ", noof_disks, label, p);
	/* BERLIOS: replaced with space^^^^ here ! 
	   while (strlen(output) < 14) {
	   strcat(output, " ");
	   }
	 */
	return (output);
}

/**
 * Change @p i into a friendly string. If @p i is \<= 10 then write out the
 * number (e.g. "one", "two", ..., "nine", "ten", "11", ...).
 * @param i The number to stringify.
 * @return The string form of @p i. To be freed by caller.
 * @note The returned value points to static strorage that will be overwritten with each call.
 */
char *number_to_text(int i)
{

	/*@ buffers ***************************************************** */
	char *output;


	/*@ end vars *************************************************** */

	switch (i) {
	case 0:
		asprintf(&output, "%s", "zero");
		break;
	case 1:
		asprintf(&output, "%s", "one");
		break;
	case 2:
		asprintf(&output, "%s", "two");
		break;
	case 3:
		asprintf(&output, "%s", "three");
		break;
	case 4:
		asprintf(&output, "%s", "four");
		break;
	case 5:
		asprintf(&output, "%s", "five");
		break;
	case 6:
		asprintf(&output, "%s", "six");
		break;
	case 7:
		asprintf(&output, "%s", "seven");
		break;
	case 8:
		asprintf(&output, "%s", "eight");
		break;
	case 9:
		asprintf(&output, "%s", "nine");
	case 10:
		asprintf(&output, "%s", "ten");
	default:
		asprintf(&output, "%d", i);
	}
	return (output);
}


/**
 * Replace all occurences of @p token with @p value while copying @p ip to @p output.
 * @param ip The input string containing zero or more <tt>token</tt>s.
 * @param output The output string written with the <tt>token</tt>s replaced by @p value.
 * @param token The token to be relaced with @p value.
 * @param value The value to replace @p token.
 */
void resolve_naff_tokens(char *output, char *ip, char *value, char *token)
{
	/*@ buffers *** */
	char *input;

	/*@ pointers * */
	char *p;

	assert_string_is_neither_NULL_nor_zerolength(ip);
	assert_string_is_neither_NULL_nor_zerolength(token);
	assert(value != NULL);

	strcpy(output, ip);			/* just in case the token doesn't appear in string at all */
	asprintf(&input, "%s", ip);
	while (strstr(input, token)) {
		strcpy(output, input);
		p = strstr(output, token);
		*p = '\0';
		strcat(output, value);
		p = strstr(input, token) + strlen(token);
		strcat(output, p);
		paranoid_free(input);
		asprintf(&input, "%s", output);
	}
	paranoid_free(input);
}


/**
 * Generate the filename of slice @p sliceno of biggiefile @p bigfileno
 * in @p path with suffix @p s. The format is as follows: @p path, followed
 * by "/slice-" and @p bigfileno zero-padded to 7 places, followed by
 * a dot and @p sliceno zero-padded to 5 places, followed by ".dat" and the
 * suffix. The string is a minimum of 24 characters long.
 * @param bigfileno The biggiefile number. Starts from 0.
 * @param sliceno The slice number of biggiefile @p bigfileno. 0 is a "header"
 * slice (no suffix) containing the biggiestruct, then are the compressed
 * slices, then an empty uncompressed "trailer" slice.
 * @param path The path to append (with a / in the middle) to the slice filename.
 * @param s If not "" then add a "." and this to the end.
 * @return The slice filename. To be freed by caller
 * @note The returned value points to static storage and will be overwritten with each call.
 */
char *slice_fname(long bigfileno, long sliceno, char *path, char *s)
{

	/*@ buffers **************************************************** */
	char *output;
	char *suffix;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(path);
	if (s[0] != '\0') {
		asprintf(&suffix, ".%s", s);
	} else {
		asprintf(&suffix, "%s", "");
	}
	asprintf(&output, "%s/slice-%07ld.%05ld.dat%s", path, bigfileno,
			 sliceno, suffix);
	paranoid_free(suffix);
	return (output);
}


/**
 * Generate a spinning symbol based on integer @p i.
 * The symbol rotates through the characters / - \ | to form an ASCII "spinner"
 * if successively written to the same location on screen.
 * @param i The amount of progress or whatever else to use to determine the character
 * for this iteration of the spinner.
 * @return The character for this iteration.
 */
int special_dot_char(int i)
{
	switch (i % 4) {
	case 0:
		return ('/');
	case 1:
		return ('-');
	case 2:
		return ('\\');
	case 3:
		return ('|');
	default:
		return ('.');
	}
	return ('.');
}


/**
 * Wrap @p flaws_str across three lines. The first two are no more than 74 characters wide.
 * @param flaws_str The original string to split.
 * @param flaws_str_A Where to put the first 74-or-less characters.
 * @param flaws_str_B Where to put the second 74-or-less characters.
 * @param flaws_str_C Where to put the rest.
 * @param res The result of the original evaluate_mountlist() operation.
 * @return TRUE if res == 0, FALSE otherwise.
 */
bool
spread_flaws_across_three_lines(char *flaws_str, char *flaws_str_A,
								char *flaws_str_B, char *flaws_str_C,
								int res)
{

	/*@ int ************************************************************* */
	int i = 0;

	/*@ initialize ****************************************************** */
	assert(flaws_str_A != NULL);
	assert(flaws_str_B != NULL);
	assert(flaws_str_C != NULL);
	assert(flaws_str != NULL);

	flaws_str_A[0] = flaws_str_B[0] = flaws_str_C[0] = '\0';


	if (!res && !strlen(flaws_str)) {
		return (TRUE);
	}
	if (strlen(flaws_str) > 0) {
		sprintf(flaws_str_A, "%s", flaws_str + 1);
	}
	if (strlen(flaws_str_A) >= 74) {
		for (i = 74; flaws_str_A[i] != ' '; i--);
		strcpy(flaws_str_B, flaws_str_A + i + 1);
		flaws_str_A[i] = '\0';
	}
	if (strlen(flaws_str_B) >= 74) {
		for (i = 74; flaws_str_B[i] != ' '; i--);
		strcpy(flaws_str_C, flaws_str_B + i + 1);
		flaws_str_B[i] = '\0';
	}
	if (res) {
		return (FALSE);
	} else {
		return (TRUE);
	}
}


/**
 * Compare @p stringA and @p stringB. This uses an ASCII sort for everything
 * up to the digits on the end but a numerical sort for the digits on the end.
 * @param stringA The first string to compare.
 * @param stringB The second string to compare.
 * @return The same as strcmp() - <0 if A<B, 0 if A=B, >0 if A>B.
 * @note This function only does a numerical sort on the @e last set of numbers. If
 * there are any in the middle those will be sorted ASCIIbetically.
 */
int strcmp_inc_numbers(char *stringA, char *stringB)
{
	/*@ int ********************************************************* */
	int i;
	int start_of_numbers_in_A;
	int start_of_numbers_in_B;
	int res;

	/*@ long ******************************************************* */
	long numA;
	long numB;

	/*@ end vars *************************************************** */
	assert(stringA != NULL);
	assert(stringB != NULL);

	if (strlen(stringA) == strlen(stringB)) {
		return (strcmp(stringA, stringB));
	}
	for (i = (int) strlen(stringA); i > 0 && isdigit(stringA[i - 1]); i--);
	if (i == (int) strlen(stringA)) {
		return (strcmp(stringA, stringB));
	}
	start_of_numbers_in_A = i;
	for (i = (int) strlen(stringB); i > 0 && isdigit(stringB[i - 1]); i--);
	if (i == (int) strlen(stringB)) {
		return (strcmp(stringA, stringB));
	}
	start_of_numbers_in_B = i;
	if (start_of_numbers_in_A != start_of_numbers_in_B) {
		return (strcmp(stringA, stringB));
	}
	res = strncmp(stringA, stringB, (size_t) i);
	if (res) {
		return (res);
	}
	numA = atol(stringA + start_of_numbers_in_A);
	numB = atol(stringB + start_of_numbers_in_B);
	/*
	   sprintf(tmp,"Comparing %s and %s --> %ld,%ld\n",stringA,stringB,numA,numB);
	   log_to_screen(tmp);
	 */
	return ((int) (numA - numB));
}



/**
 * Strip excess baggage from @p input, which should be a line from afio.
 * For now this copies the whole line unless it finds a set of quotes, in which case
 * it copies their contents only.
 * @param input The input line (presumably from afio).
 * @return The stripped line.
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *strip_afio_output_line(char *input)
{
	/*@ buffer ****************************************************** */
	char *output;

	/*@ pointers **************************************************** */
	char *p;
	char *q;
	/*@ end vars *************************************************** */

	assert(input != NULL);
	asprintf(&output, "%s", input);
	p = strchr(input, '\"');
	if (p) {
		q = strchr(++p, '\"');
		if (q) {
			paranoid_free(output);
			asprintf(&output, "%s", p);
			*(strchr(output, '\"')) = '\0';
		}
	}
	return (output);
}


/**
 * Remove all characters whose ASCII value is less than or equal to 32
 * (spaces and control characters) from both sides of @p in_out.
 * @param in_out The string to strip spaces/control characters from (modified).
 */
void strip_spaces(char *in_out)
{
	/*@ buffers ***************************************************** */
	char *tmp;
	char *tmp1;

	/*@ pointers **************************************************** */
	char *p;

	/*@ int ******************************************************** */
	int i;
	int original_incoming_length;

	/*@ end vars *************************************************** */

	assert(in_out != NULL);
	original_incoming_length = (int) strlen(in_out);
	for (i = 0; in_out[i] <= ' ' && i < (int) strlen(in_out); i++);
	asprintf(&tmp, "%s", in_out + i);
	for (i = (int) strlen(tmp); i > 0 && tmp[i - 1] <= 32; i--);
	tmp[i] = '\0';
	for (i = 0; i < original_incoming_length; i++) {
		in_out[i] = ' ';
	}
	in_out[i] = '\0';
	i = 0;
	p = tmp;
	while (*p != '\0') {
		in_out[i] = *(p++);
		in_out[i + 1] = '\0';
		if (in_out[i] < 32 && i > 0) {
			if (in_out[i] == 8) {
				i--;
			} else if (in_out[i] == 9) {
				in_out[i++] = ' ';
			} else if (in_out[i] == '\r') {
				asprintf(&tmp1, "%s", in_out + i);
				strcpy(in_out, tmp1);
				paranoid_free(tmp1);
				i = -1;
				continue;
			} else if (in_out[i] == '\t') {
				for (i++; i % 5; i++);
			} else if (in_out[i] >= 10 && in_out[i] <= 13) {
				break;
			} else {
				i--;
			}
		} else {
			i++;
		}
	}
	in_out[i] = '\0';
	paranoid_free(tmp);
}


/**
 * If there are double quotes "" around @p incoming then remove them.
 * This does not affect other quotes that may be embedded within the string.
 * @param incoming The string to trim quotes from (modified).
 * @return @p outcoming. To be freed by caller
 */
char *trim_empty_quotes(char *incoming)
{
	/*@ buffer ****************************************************** */
	char *outgoing;

	/*@ end vars *************************************************** */
	assert(incoming != NULL);

	if (incoming[0] == '\"' && incoming[strlen(incoming) - 1] == '\"') {
		asprintf(&outgoing, "%s", incoming + 1);
		outgoing[strlen(outgoing) - 1] = '\0';
	} else {
		asprintf(&outgoing, incoming);
	}
	return (outgoing);
}




/**
 * Remove any partition info from @p partition, leaving just the drive name.
 * @param partition The partition name soon-to-become drive name. (modified)
 * @return @p partition.
 */
char *truncate_to_drive_name(char *partition)
{
	int i = strlen(partition) - 1;
	char *c;

#ifdef __FreeBSD__

	if (islower(partition[i]))	// BSD subpartition
		i--;
	if (partition[i - 1] == 's') {
		while (isdigit(partition[i]))
			i--;
		i--;
	}
	partition[i + 1] = '\0';

#else

	assert_string_is_neither_NULL_nor_zerolength(partition);
	/* first see if it's a devfs style device */
	c = strrchr(partition, '/');
	if (c && strncmp(c, "/part", 5) == 0) {
		/* yup it's devfs, return the "disc" path */
		strcpy(c + 1, "disc");
		return partition;
	}

	for (i = strlen(partition); isdigit(partition[i - 1]); i--)
		continue;
	if (partition[i - 1] == 'p' && isdigit(partition[i - 2])) {
		i--;
	}
	partition[i] = '\0';

#endif

	return partition;
}


/**
 * Turn a RAID level number (-1 to 5) into a friendly string. The string
 * is either "Linear RAID" for -1, or " RAID %-2d " (%d = @p raid_level)
 * for anything else.
 * @param raid_level The RAID level to stringify.
 * @return The string form of @p raid_level. To be freed by caller
 * @note The returned value points to static storage that will be overwritten with each call.
 */
char *turn_raid_level_number_to_string(int raid_level)
{

	/*@ buffer ********************************************************** */
	char *output;

	if (raid_level >= 0) {
		asprintf(&output, " RAID %-2d ", raid_level);
	} else {
		asprintf(&output, "Linear RAID");
	}
	return (output);
}


/**
 * Determine the severity (1-3, 1 being low) of the fact that
 * @p fn changed in the live filesystem (verify/compare).
 * @param fn The filename that changed.
 * @param out_reason If non-NULL, a descriptive reason for the difference will be copied here.
 * @return The severity (1-3).
 */
int severity_of_difference(char *fn, char *out_reason)
{
	int sev = 0;
	char *reason;
	char *filename;

	// out_reason might be null on purpose, so don't bomb if it is :) OK?
	assert_string_is_neither_NULL_nor_zerolength(fn);
	if (!strncmp(fn, MNT_RESTORING, strlen(MNT_RESTORING))) {
		asprintf(&filename, "%s", fn + strlen(MNT_RESTORING));
	} else if (fn[0] != '/') {
		asprintf(&filename, "/%s", fn);
	} else {
		asprintf(&filename, "%s", fn);
	}

	if (!strncmp(filename, "/var/", 5)) {
		sev = 2;
		asprintf(&reason,
				"/var's contents will change regularly, inevitably.");
	}
	if (!strncmp(filename, "/home", 5)) {
		sev = 2;
		asprintf(&reason,
				"It's in your /home partiton. Therefore, it is important.");
	}
	if (!strncmp(filename, "/usr/", 5)) {
		sev = 3;
		asprintf(&reason,
				"You may have installed/removed software during the backup.");
	}
	if (!strncmp(filename, "/etc/", 5)) {
		sev = 3;
		asprintf(&reason,
				"Do not edit config files while backing up your PC.");
	}
	if (!strcmp(filename, "/etc/adjtime")
		|| !strcmp(filename, "/etc/mtab")) {
		sev = 1;
		asprintf(&reason, "This file changes all the time. It's OK.");
	}
	if (!strncmp(filename, "/root/", 6)) {
		sev = 3;
		asprintf(&reason, "Were you compiling/editing something in /root?");
	}
	if (!strncmp(filename, "/root/.", 7)) {
		sev = 2;
		asprintf(&reason, "Temp or 'dot' files changed in /root.");
	}
	if (!strncmp(filename, "/var/lib/", 9)) {
		sev = 2;
		asprintf(&reason, "Did you add/remove software during backing?");
	}
	if (!strncmp(filename, "/var/lib/rpm", 12)) {
		sev = 3;
		asprintf(&reason, "Did you add/remove software during backing?");
	}
	if (!strncmp(filename, "/var/lib/slocate", 16)) {
		sev = 1;
		asprintf(&reason,
				"The 'update' daemon ran during backup. This does not affect the integrity of your backup.");
	}
	if (!strncmp(filename, "/var/log/", 9)
		|| strstr(filename, "/.xsession")
		|| !strcmp(filename + strlen(filename) - 4, ".log")) {
		sev = 1;
		asprintf(&reason,
				"Log files change frequently as the computer runs. Fret not.");
	}
	if (!strncmp(filename, "/var/spool", 10)) {
		sev = 1;
		asprintf(&reason,
				"Background processes or printers were active. This does not affect the integrity of your backup.");
	}
	if (!strncmp(filename, "/var/spool/mail", 10)) {
		sev = 2;
		asprintf(&reason, "Mail was sent/received during backup.");
	}
	if (filename[strlen(filename) - 1] == '~') {
		sev = 1;
		asprintf(&reason,
				"Backup copy of another file which was modified recently.");
	}
	if (strstr(filename, "cache")) {
		sev = 1;
		asprintf(&reason,
				"Part of a cache of data. Caches change from time to time. Don't worry.");
	}
	if (!strncmp(filename, "/var/run/", 9)
		|| !strncmp(filename, "/var/lock", 8)
		|| strstr(filename, "/.DCOPserver") || strstr(filename, "/.MCOP")
		|| strstr(filename, "/.Xauthority")) {
		sev = 1;
		asprintf(&reason,
				"Temporary file (a lockfile, perhaps) used by software such as X or KDE to register its presence.");
	}
	paranoid_free(filename);

	if (sev == 0) {
		sev = 3;
		asprintf(&reason,
			"Changed since backup. Consider running a differential backup in a day or two.");
	}

	out_reason = reason;
	return (sev);
}



/**
 * Compare the filenames in two filelist entries (s_filelist_entry*) casted
 * to void*.
 * @param va The first filelist entry, cast as a @c void pointer.
 * @param vb The second filelist entry, cast as a @c void pointer.
 * @return The return value of strcmp().
 */
int compare_two_filelist_entries(void *va, void *vb)
{
	static int res;
	struct s_filelist_entry *fa, *fb;

	assert(va != NULL);
	assert(vb != NULL);
	fa = (struct s_filelist_entry *) va;
	fb = (struct s_filelist_entry *) vb;
	res = strcmp(fa->filename, fb->filename);
	return (res);
}


/**
 * Generate a line intended to be passed to update_evalcall_form(), indicating
 * the current media fill percentage (or number of kilobytes if size is not known).
 * @param bkpinfo The backup media structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->media_size
 * - @c bkpinfo->scratchdir
 * @return The string indicating media fill. Needs to be freed by caller
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *percent_media_full_comment(struct s_bkpinfo *bkpinfo)
{
	/*@ int *********************************************** */
	int percentage = 0;
	int i;
	int j;

	/*@ buffers ******************************************* */
	char *outstr;
	char *tmp;
	char *tmp1;
	char *tmp2;
	char *prepstr;
	char *p;

	assert(bkpinfo != NULL);

	if (bkpinfo->media_size[g_current_media_number] <= 0) {
		asprintf(&tmp, "%lld", g_tape_posK);
		asprintf(&outstr, "Volume %d: %s kilobytes archived so far",
				g_current_media_number, commarize(tmp));
		paranoid_free(tmp);
		return (outstr);
	}

/* update screen */
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		percentage =
			(int) (g_tape_posK / 10 /
				   bkpinfo->media_size[g_current_media_number]);
		asprintf(&prepstr, "Volume %d: [", g_current_media_number);
	} else {
		percentage =
			(int) (space_occupied_by_cd(bkpinfo->scratchdir) * 100 / 1024 /
				   bkpinfo->media_size[g_current_media_number]);
		asprintf(&prepstr, "%s %d: [",
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
	}
	if (percentage > 100) {
		percentage = 100;
	}
	j = trunc(percentage/5);
	tmp1 = (char *)malloc((j + 1) * sizeof(char));
	for (i = 0, p = tmp1 ; i < j ; i++, p++) {
			*p = '*';
	}
	*p = '\0';

	tmp2 = (char *)malloc((20 - j + 1) * sizeof(char));
	for (i = 0, p = tmp2 ; i < 20 - j ; i++, p++) {
			*p = '.';
	}
	*p = '\0';

	/* BERLIOS There is a bug here I can't solve for the moment. If you 
	 * replace %% in the asprintf below by 'percent' it just works, but 
	 * like this it creates a huge number. Memory pb somewhere */
	/*
	log_it("percentage: %d", percentage);
	asprintf(&outstr, "%s%s%s] %3d%% used", prepstr, tmp1, tmp2, percentage);
	*/
	asprintf(&outstr, "%s%s%s] %3d percent used", prepstr, tmp1, tmp2, percentage);
	paranoid_free(prepstr);
	paranoid_free(tmp1);
	paranoid_free(tmp2);
	return (outstr);
}

/**
 * Get a string form of @p type_of_bkp.
 * @param type_of_bkp The backup type to stringify.
 * @return The stringification of @p type_of_bkp.
 * @note The returned string points to static storage that will be overwritten with each call.
 */
char *media_descriptor_string(t_bkptype type_of_bkp)
{
	static char *type_of_backup = NULL;

	if (!type_of_backup) {
		malloc_string(type_of_backup);
	}

	switch (type_of_bkp) {
	case dvd:
		strcpy(type_of_backup, "DVD");
		break;
	case cdr:
		strcpy(type_of_backup, "CDR");
		break;
	case cdrw:
		strcpy(type_of_backup, "CDRW");
		break;
	case tape:
		strcpy(type_of_backup, "tape");
		break;
	case cdstream:
		strcpy(type_of_backup, "CDR");
		break;
	case udev:
		strcpy(type_of_backup, "udev");
		break;
	case iso:
		strcpy(type_of_backup, "ISO");
		break;
	case nfs:
		strcpy(type_of_backup, "nfs");
		break;
	default:
		strcpy(type_of_backup, "ISO");
	}
	return (type_of_backup);
}

/* @} - end of stringGroup */
