/* $Id$

for subroutines which manipulate the filelist
*/

/**
 * @file
 * Functions which create, chop, and edit the filelist.
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-filelist.h"
#include "libmondo-string-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-fork-EXT.h"
#include "newt-specific-EXT.h"
#include "libmondo-tools-EXT.h"


#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>


/**
 * The maximum depth of directories to put in the skeleton filelist.
 * This is a balance between performance and a good progress indicator.
 */
#define MAX_SKEL_DEPTH 3


int mondo_makefilelist(char *logfile, char *tmpdir, char *scratchdir,
					   char *include_paths, char *excp, int differential,
					   char *userdef_filelist);


/*@unused@*/
//static char cvsid[] = "$Id$";

/**
 * Number of lines in the filelist last loaded.
 * @warning This implies that two filesets cannot be loaded at once.
 * @ingroup globalGroup
 */
long g_original_noof_lines_in_filelist = 0;

/**
 * Number of filesets in the current backup.
 * @ingroup globalGroup
 */
long g_noof_sets = 0;

extern bool g_text_mode;
extern newtComponent g_progressForm;
extern int g_currentY;
extern int g_noof_rows;


/**
 * @addtogroup filelistGroup
 * @{
 */
/**
 * Call chop_filelist() to chop the filelist into sets.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->image_devs
 * - @c bkpinfo->optimal_set_size
 * - @c bkpinfo->scratchdir
 * - @c bkpinfo->tmpdir
 * @see chop_filelist
 */
int call_filelist_chopper(struct s_bkpinfo *bkpinfo)
{
	/*@ buffers *********************** */
	char *dev;
	char *filelist;
	char *tempfile;
	long noof_sets;

	/*@ pointers ********************** */
	char *ptr;
	FILE *fout;

	/*@ int *************************** */
	int i, retval = 0;

	mvaddstr_and_log_it(g_currentY, 0, _("Dividing filelist into sets"));

	log_to_screen(_("Dividing filelist into sets. Please wait."));
	i = 0;
/*
  if (find_home_of_exe("getfattr")) 
    { i++; log_to_screen ("NEW! Recording extended attributes."); }
  if (find_home_of_exe("getfacl"))
    { i++; log_to_screen ("NEW! Recording ACL information."); }
  if (i)
    { i=0; log_to_screen ("This will take more time. Please be patient."); }
*/
	asprintf(&filelist, "%s/archives/filelist.full", bkpinfo->scratchdir);
	if (!does_file_exist(filelist)) {
		log_it("filelist %s not found", filelist);
		fatal_error("call_filelist_chopper() -- filelist not found!");
	}

	noof_sets =
		chop_filelist(filelist, bkpinfo->tmpdir,
					  bkpinfo->optimal_set_size);
	paranoid_free(filelist);

	estimate_noof_media_required(bkpinfo, noof_sets);	// for cosmetic purposes

	asprintf(&tempfile, "%s/biggielist.txt", bkpinfo->tmpdir);
	if (!(fout = fopen(tempfile, "a"))) {
		log_OS_error("Cannot append to biggielist");
		retval++;
	}
	paranoid_free(tempfile);
	log_it(bkpinfo->image_devs);

	ptr = bkpinfo->image_devs;

	while (ptr && *ptr) {
		asprintf(&dev, ptr);
		log_it("Examining imagedev %s", dev);
		for (i = 0; i < (int) strlen(dev) && dev[i] != ' '; i++);
		dev[i] = '\0';
		if (!strlen(dev)) {
			continue;
		}
		fprintf(fout, "%s\n", dev);
		log_it("Adding '%s' to biggielist", dev);
		paranoid_free(dev);

		if ((ptr = strchr(ptr, ' '))) {
			ptr++;
		}
	}
	paranoid_fclose(fout);
	mvaddstr_and_log_it(g_currentY++, 74, "Done.");

	return (retval);
}



int sort_file(char *orig_fname)
{
	char *tmp_fname;
	char *command;
	int retval = 0;

	log_msg(1, "Sorting file %s", orig_fname);
	asprintf(&tmp_fname, "/tmp/sort.%d.%d.%d", (int) (random() % 32768),
			 (int) (random() % 32768), (int) (random() % 32768));

	if (!does_file_exist(orig_fname)) {
		return (0);
	}							// no sense in trying to sort an empty file

	asprintf(&command, "sort %s > %s 2>> %s", orig_fname, tmp_fname,
			 MONDO_LOGFILE);
	retval = system(command);
	paranoid_free(command);

	if (retval) {
		log_msg(2, "Failed to sort %s - oh dear", orig_fname);
	} else {
		log_msg(2, "Sorted %s --> %s OK. Copying it back to %s now",
				orig_fname, tmp_fname, orig_fname);
		asprintf(&command, "mv -f %s %s", tmp_fname, orig_fname);
		retval += run_program_and_log_output(command, 2);
		paranoid_free(command);

		if (retval) {
			log_msg(2, "Failed to copy %s back to %s - oh dear", tmp_fname,
					orig_fname);
		} else {
			log_msg(2, "%s was sorted OK.", orig_fname);
		}
	}
	paranoid_free(tmp_fname);
	log_msg(1, "Finished sorting file %s", orig_fname);
	return (retval);
}


/**
 * Chop the filelist into sets.
 * Each fileset is a list of files whose total (uncompressed) size is usually
 * about X KB. Files bigger than 8X KB are placed in a "biggielist"; they will
 * be sliced and compressed separately from the regular files.
 *
 * @param filelist The big filelist (filelist.full) to chop up.
 * @param outdir The directory to place the files (filelist.N where N is
 * an integer, biggielist.txt, and LAST-FILELIST-NUMBER) created
 * @param maxsetsizeK Optimal size of a fileset (X above).
 * @return number of errors encountered (0 for success).
 */
int chop_filelist(char *filelist, char *outdir, long maxsetsizeK)
{
/*@ long ****************************************/
	long lino = 0;
	long max_sane_size_for_a_file;
	long curr_set_size;
	long noof_lines;
	long siz;

	/*@ int **************************************** */
	int i;
	size_t n = 0;
	long curr_set_no;

	/*@ buffers ************************************* */
	char *outfname;
	char *biggie_fname;
	char *incoming = NULL;
	char *tmp;

	/*@ pointers *********************************** */
	FILE *fin;
	FILE *fout;
	FILE *fbig;

	/*@ structures ********************************* */
	struct stat buf;

	assert_string_is_neither_NULL_nor_zerolength(filelist);
	assert_string_is_neither_NULL_nor_zerolength(outdir);
	assert(maxsetsizeK > 0);

	max_sane_size_for_a_file = 32L * 1024L;
// max_sane_size_for_a_file = maxsetsizeK*2;
//  if (max_sane_size_for_a_file > 32*1024)
//    { max_sane_size_for_a_file = 32*1024; }

	log_it("filelist=%s;", filelist);
	open_evalcall_form("Dividing filelist into sets");
	noof_lines = count_lines_in_file(filelist);
	if (!(fin = fopen(filelist, "r"))) {
		log_OS_error("Cannot openin filelist");
		return (0);
	}
	curr_set_no = 0;
	curr_set_size = 0;
	asprintf(&outfname, "%s/filelist.%ld", outdir, curr_set_no);
	asprintf(&biggie_fname, "%s/biggielist.txt", outdir);
	log_it("outfname=%s; biggie_fname=%s", outfname, biggie_fname);
	if (!(fbig = fopen(biggie_fname, "w"))) {
		log_OS_error("Cannot openout biggie_fname");
		return (0);
	}
	if (!(fout = fopen(outfname, "w"))) {
		log_OS_error("Cannot openout outfname");
		return (0);
	}
	(void) getline(&incoming, &n, fin);
	while (!feof(fin)) {
		lino++;
		i = strlen(incoming) - 1;
		if (i < 0) {
			i = 0;
		}
		/* BERLIOS: Useless with getline
		   if (i > MAX_STR_LEN - 1) {
		   incoming[MAX_STR_LEN - 30] = '\0';
		   log_msg(1, "Warning - truncating file %s's name", incoming);
		   err++;
		   }
		 */
		if (incoming[i] < 32) {
			incoming[i] = '\0';
		}
		if (!strncmp(incoming, "/dev/", 5)) {
			siz = 1;
		} else if (lstat(incoming, &buf) != 0) {
			siz = 0;
		} else {
			siz = (long) (buf.st_size >> 10);
		}
		if (siz > max_sane_size_for_a_file)
// && strcmp(incoming+strlen(incoming)-4, ".bz2") && strcmp(incoming+strlen(incoming)-4, ".tbz"))
		{
			fprintf(fbig, "%s\n", incoming);
		} else {
			curr_set_size += siz;
			fprintf(fout, "%s\n", incoming);
			if (curr_set_size > maxsetsizeK) {
				paranoid_fclose(fout);
				sort_file(outfname);
				paranoid_free(outfname);

				curr_set_no++;
				curr_set_size = 0;
				asprintf(&outfname, "%s/filelist.%ld", outdir,
						 curr_set_no);
				if (!(fout = fopen(outfname, "w"))) {
					log_OS_error("Unable to openout outfname");
					return (0);
				}
				update_evalcall_form((int) (lino * 100 / noof_lines));
			}
		}
		(void) getline(&incoming, &n, fin);
	}
	paranoid_fclose(fin);
	paranoid_fclose(fout);
	paranoid_fclose(fbig);

	if (length_of_file(outfname) <= 2) {
		unlink(outfname);
		g_noof_sets--;
	}
	g_noof_sets = curr_set_no;
	sort_file(outfname);
	sort_file(biggie_fname);
	paranoid_free(biggie_fname);
	paranoid_free(outfname);

	asprintf(&outfname, "%s/LAST-FILELIST-NUMBER", outdir);
	asprintf(&tmp, "%ld", curr_set_no);
	if (write_one_liner_data_file(outfname, tmp)) {
		log_OS_error
			("Unable to echo write one-liner to LAST-FILELIST-NUMBER");
		return (0);
	}
	paranoid_free(outfname);
	paranoid_free(tmp);

	if (curr_set_no == 0) {
		asprintf(&tmp, "Only one fileset. Fine.");
	} else {
		asprintf(&tmp, "Filelist divided into %ld sets", curr_set_no + 1);
	}
	log_msg(1, tmp);
	paranoid_free(tmp);
	close_evalcall_form();
	/* This is to work around an obscure bug in Newt; open a form, close it,
	   carry on... I don't know why it works but it works. If you don't do this
	   then update_progress_form() won't show the "time taken / time remaining"
	   line. The bug only crops up AFTER the call to chop_filelist(). Weird. */
#ifndef _XWIN
	if (!g_text_mode) {
		open_progress_form("", "", "", "", 100);
		newtPopHelpLine();
		newtFormDestroy(g_progressForm);
		newtPopWindow();
	}
#endif
	return (curr_set_no + 1);
}


/**
 * Free all the memory used by a filelist structure.
 * Since this may take a long time for large filelists, a progress bar will be displayed.
 * @param filelist The filelist to free.
 */
void free_filelist(struct s_node *filelist)
{
	/*@ int's ******************************************************* */
	static int depth = 0;
	int percentage;

	/*@ long's ****************************************************** */
	static long i = 0;

	/*@ end vars **************************************************** */

	assert(filelist != NULL);
	if (depth == 0) {
		open_evalcall_form("Freeing memory");
		log_to_screen(_("Freeing memory formerly occupied by filelist"));
	}
	depth++;

	if (filelist->ch == '\0') {
		if (!(i++ % 1111)) {
			percentage =
				(int) (i * 100 / g_original_noof_lines_in_filelist);
			update_evalcall_form(percentage);

		}
	}

	if (filelist->right) {
		free_filelist(filelist->right);
		filelist->right = NULL;
	}
	if (filelist->down) {
/*      if (!(i++ %39999)) { update_evalcall_form(0); } */
		free_filelist(filelist->down);
		filelist->down = NULL;
	}
	filelist->ch = '\0';
	paranoid_free(filelist);
	depth--;
	if (depth == 0) {
		close_evalcall_form();
		log_it("Finished freeing memory");
	}
}


int call_exe_and_pipe_output_to_fd(char *syscall, FILE * pout)
{
	FILE *pattr;
	char *tmp = NULL;
	size_t n = 0;

	pattr = popen(syscall, "r");
	if (!pattr) {
		log_msg(1, "Failed to open fattr() %s", syscall);
		return (1);
	}
	if (feof(pattr)) {
		log_msg(1, "Failed to call fattr() %s", syscall);
		paranoid_pclose(pattr);
		return (2);
	}
	for (getline(&tmp, &n, pattr); !feof(pattr); getline(&tmp, &n, pattr)) {
		fputs(tmp, pout);
	}
	paranoid_pclose(pattr);
	paranoid_free(tmp);
	return (0);
}


int gen_aux_list(char *filelist, char *syscall_sprintf,
				 char *auxlist_fname)
{
	FILE *fin;
	FILE *pout;
	char *pout_command;
	char *syscall;
	char *tmp;
	char *file_to_analyze = NULL;
	int i;
	size_t n = 0;

	if (!(fin = fopen(filelist, "r"))) {
		log_msg(1, "Cannot openin filelist %s", filelist);
		return (1);
	}
	asprintf(&pout_command, "gzip -c1 > %s", auxlist_fname);
	if (!(pout = popen(pout_command, "w"))) {
		log_msg(1, "Cannot openout auxlist_fname %s", auxlist_fname);
		fclose(fin);
		paranoid_free(pout_command);
		return (4);
	}
	paranoid_free(pout_command);

	for (getline(&file_to_analyze, &n, fin); !feof(fin);
		 getline(&file_to_analyze, &n, fin)) {
		i = strlen(file_to_analyze);
		if (i > 0 && file_to_analyze[i - 1] < 32) {
			file_to_analyze[i - 1] = '\0';
		}
		log_msg(8, "Analyzing %s", file_to_analyze);
		asprintf(&tmp, "%s 2>> /dev/null", syscall_sprintf);
		asprintf(&syscall, tmp, file_to_analyze);
		paranoid_free(tmp);
		call_exe_and_pipe_output_to_fd(syscall, pout);
		paranoid_free(syscall);
	}
	paranoid_fclose(fin);
	paranoid_pclose(pout);
	paranoid_free(file_to_analyze);
	return (0);
}


int get_acl_list(char *filelist, char *facl_fname)
{
	char *command = NULL;
	char *tmp = NULL;
	int retval = 0;

	asprintf(&command, "touch %s", facl_fname);
	run_program_and_log_output(command, 8);
	paranoid_free(command);

	tmp = find_home_of_exe("getfacl");
	if (tmp) {
//      sort_file(filelist); // FIXME - filelist chopper sorts, so this isn't necessary
		asprintf(&command,
				 "getfacl --all-effective -P %s 2>> %s | gzip -c1 > %s 2>> %s",
				 filelist, MONDO_LOGFILE, facl_fname, MONDO_LOGFILE);
		iamhere(command);
		retval = system(command);
		paranoid_free(command);
	}
	paranoid_free(tmp);
	return (retval);
}


int get_fattr_list(char *filelist, char *fattr_fname)
{
	char *command = NULL;
	char *tmp = NULL;
	int retval = 0;

	asprintf(&command, "touch %s", fattr_fname);
	run_program_and_log_output(command, 8);
	paranoid_free(command);

	tmp = find_home_of_exe("getfattr");
	if (tmp) {
//      sort_file(filelist); // FIXME - filelist chopper sorts, so this isn't necessary
		retval =
			gen_aux_list(filelist, "getfattr --en=hex -P -d \"%s\"",
						 fattr_fname);
	}
	paranoid_free(tmp);
	return (retval);
}


/*
int set_acl_list(char*masklist, char*acl_fname)
{
  char*command;
  int retval=0;
  
  if (length_of_file(acl_fname) <= 0) { return(0); }
  log_msg(1, "FIXME - not using masklist");  
  malloc_string(command);
  if (find_home_of_exe("setfacl"))
    {
      sprintf(command, "gzip -dc %s | setfacl --restore - 2>> %s", acl_fname, MONDO_LOGFILE);
      log_msg(1, "command = %s", command);
      retval = system(command);
    }
  paranoid_free(command);
  return(retval);
}
*/


int set_EXAT_list(char *orig_msklist, char *original_exat_fname,
				  char *executable)
{
	const int my_depth = 8;
	char *command, *syscall_pin, *syscall_pout;
	char *incoming = NULL;
	char *current_subset_file = NULL;
	char *current_master_file, *masklist;
	int retval = 0;
	int i;
	size_t n = 0;
	char *p, *q;
	FILE *pin, *pout, *faclin;

	log_msg(1, "set_EXAT_list(%s, %s, %s)", orig_msklist,
			original_exat_fname, executable);
	if (!orig_msklist || !orig_msklist[0]
		|| !does_file_exist(orig_msklist)) {
		log_msg(1,
				"No masklist provided. I shall therefore set ALL attributes.");
		asprintf(&command, "gzip -dc %s | %s --restore - 2>> %s",
				 original_exat_fname, executable, MONDO_LOGFILE);
		log_msg(1, "command = %s", command);
		retval = system(command);
		paranoid_free(command);
		log_msg(1, "Returning w/ retval=%d", retval);
		return (retval);
	}
	if (length_of_file(original_exat_fname) <= 0) {
		log_msg(1,
				"original_exat_fname %s is empty or missing, so no need to set EXAT list",
				original_exat_fname);
		return (0);
	}
	asprintf(&masklist, "/tmp/%d.%d.mask", (int) (random() % 32768),
			 (int) (random() % 32768));
	asprintf(&command, "cp -f %s %s", orig_msklist, masklist);
	run_program_and_log_output(command, 1);
	paranoid_free(command);

	sort_file(masklist);
	asprintf(&syscall_pin, "gzip -dc %s", original_exat_fname);
	asprintf(&syscall_pout, "%s --restore - 2>> %s", executable,
			 MONDO_LOGFILE);

	log_msg(1, "syscall_pin = %s", syscall_pin);
	log_msg(1, "syscall_pout = %s", syscall_pout);
	pout = popen(syscall_pout, "w");
	if (!pout) {
		iamhere("Unable to openout to syscall_pout");
		return (1);
	}
	paranoid_free(syscall_pout);

	pin = popen(syscall_pin, "r");
	if (!pin) {
		pclose(pout);
		iamhere("Unable to openin from syscall");
		return (1);
	}
	paranoid_free(syscall_pin);

	faclin = fopen(masklist, "r");
	if (!faclin) {
		pclose(pin);
		pclose(pout);
		iamhere("Unable to openin masklist");
		return (1);
	}
//  printf("Hi there. Starting the loop\n");

	getline(&current_subset_file, &n, faclin);
	getline(&incoming, &n, pin);
	while (!feof(pin) && !feof(faclin)) {
//      printf("incoming = %s", incoming);

		asprintf(&current_master_file, incoming + 8);

		p = current_subset_file;
		if (*p == '/') {
			p++;
		}
		i = strlen(p);
		if (i > 0 && p[i - 1] < 32) {
			p[i - 1] = '\0';
		}


		q = current_master_file;
		if (*q == '/') {
			q++;
		}
		i = strlen(q);
		if (i > 0 && q[i - 1] < 32) {
			q[i - 1] = '\0';
		}

		i = strcmp(p, q);
		log_msg(my_depth, "'%s' v '%s' --> %d\n", p, q, i);

//      printf("%s v %s --> %d\n", p, q, i);

		if (i < 0) {			// read another subset file in.
			log_msg(my_depth, "Reading next subset line in\n\n");
			getline(&current_subset_file, &n, faclin);
			continue;
		}

		if (!i) {
			fputs(incoming, pout);
		}
		getline(&incoming, &n, pin);
		if (!i) {
			log_msg(my_depth, "Copying master %s", q);
		}
//      if (!i) { printf("Match --- %s\n", q); }

		while (!feof(pin) && strncmp(incoming, "# file: ", 8)) {
			if (!i) {

//    printf("%s", incoming);

				fputs(incoming, pout);
			}
			getline(&incoming, &n, pin);
		}
		if (!i) {
			getline(&current_subset_file, &n, faclin);
		}
		paranoid_free(current_master_file);
	}
	paranoid_free(current_subset_file);

	while (!feof(pin)) {
		getline(&incoming, &n, pin);
	}
	paranoid_free(incoming);
	fclose(faclin);
	pclose(pin);
	pclose(pout);

//  printf("OK, loop is done\n");

	unlink(masklist);
	paranoid_free(masklist);

	return (retval);
}


int set_fattr_list(char *masklist, char *fattr_fname)
{
	return (set_EXAT_list(masklist, fattr_fname, "setfattr"));
}



int set_acl_list(char *masklist, char *acl_fname)
{
	return (set_EXAT_list(masklist, acl_fname, "setfacl"));
}


/**
 * Get the number of the last fileset in the backup.
 * @param bkpinfo The backup information structure. Only the @c bkpinfo->tmpdir field is used.
 * @return The last filelist number.
 * @note This function should only be called at restore-time.
 */
int get_last_filelist_number(struct s_bkpinfo *bkpinfo)
{
	/*@ buffers ***************************************************** */
	char val_sz[MAX_STR_LEN];
	char *cfg_fname;

	/*@ long ******************************************************** */
	int val_i;

	/*@ end vars **************************************************** */

	assert(bkpinfo != NULL);

	asprintf(&cfg_fname, "%s/mondo-restore.cfg", bkpinfo->tmpdir);
	read_cfg_var(cfg_fname, "last-filelist-number", val_sz);
	val_i = atoi(val_sz);
	if (val_i <= 0) {
		val_i = 500;
	}
	paranoid_free(cfg_fname);
	return (val_i);
}


/**
 * Add a string at @p startnode.
 * @param startnode The node to start at when searching for where to add the string.
 * @param string_to_add The string to add.
 * @return 0 for success, 1 for failure.
 * @bug I don't understand this function. Would someone care to explain it?
 */

int add_string_at_node(struct s_node *startnode, char *string_to_add)
{

	int noof_chars;
	int i;
	int res;

	struct s_node *node, *newnode;

	char char_to_add;

	const bool sosodef = FALSE;

	static int depth = 0;
	static char original_string[MAX_STR_LEN];

	assert(startnode != NULL);
	assert(string_to_add != NULL);

	if (!depth) {
		strcpy(original_string, string_to_add);
	}

	noof_chars = strlen(string_to_add) + 1;	// we include the '\0'

	// walk across tree if necessary
	node = startnode;
	char_to_add = string_to_add[0];
	if (node->right != NULL && node->ch < char_to_add) {
		log_msg(7, "depth=%d --- going RIGHT ... %c-->%c", depth,
				char_to_add, node->ch, (node->right)->ch);
		return (add_string_at_node(node->right, string_to_add));
	}
	// walk down tree if appropriate
	if (node->down != NULL && node->ch == char_to_add) {
		log_msg(7, "depth=%d char=%c --- going DOWN", depth, char_to_add);
		depth++;
		res = add_string_at_node(node->down, string_to_add + 1);
		depth--;
		return (res);
	}

	if (char_to_add == '\0' && node->ch == '\0') {
		log_msg(6, "%s already in tree", original_string);
		return (1);
	}
	// add here
	if (!(newnode = (struct s_node *) malloc(sizeof(struct s_node)))) {
		log_to_screen(_("failed to malloc"));
		depth--;
		return (1);
	}
	if (char_to_add < node->ch)	// add to the left of node
	{
		log_msg(7, "depth=%d char=%c --- adding (left)", depth,
				char_to_add);
		memcpy((void *) newnode, (void *) node, sizeof(struct s_node));
		node->right = newnode;
	} else if (char_to_add > node->ch)	// add to the right of node
	{
		log_msg(7, "depth=%d char=%c --- adding (right)", depth,
				char_to_add);
		newnode->right = node->right;	// newnode is to the RIGHT of node
		node->right = newnode;
		node = newnode;
	}
	// from now on, we're working on 'node'
	node->down = NULL;
	node->ch = char_to_add;
	node->expanded = node->selected = FALSE;
	if (char_to_add == '\0') {
		log_msg(6, "Added %s OK", original_string);
		return (0);
	}
	// add the rest
	log_msg(6, "Adding remaining chars ('%s')", string_to_add + 1);
	for (i = 1; i < noof_chars; i++) {
		if (!
			(node->down =
			 (struct s_node *) malloc(sizeof(struct s_node)))) {
			log_to_screen(_("%s - failed to malloc"), string_to_add);
			return (1);
		}
		node = node->down;
		char_to_add = string_to_add[i];
		log_msg(6, "Adding '%c'", char_to_add);
		node->ch = char_to_add;
		node->right = node->down = NULL;
		node->expanded = node->selected = FALSE;
		if (!node->ch) {
			node->selected = sosodef;
		}
	}
	log_msg(6, "Finally - added %s OK", original_string);
	return (0);
}


/**
 * Load a filelist into a <tt>struct s_node</tt>.
 * When you are done with the filelist, call free_filelist().
 * @param filelist_fname The file to load the filelist from.
 * @return A filelist tree structure.
 */
struct s_node *load_filelist(char *filelist_fname)
{

	struct s_node *filelist = NULL;
	FILE *pin = NULL;
	char *command_to_open_fname = NULL;
	char *fname = NULL;
	char *tmp = NULL;
	char *tmp1 = NULL;
	int pos_in_fname = 0;
	size_t n = 0;
	int percentage = 0;

	long lines_in_filelist = 0L;
	long lino = 0L;

	assert_string_is_neither_NULL_nor_zerolength(filelist_fname);

	if (!does_file_exist(filelist_fname)) {
		fatal_error("filelist does not exist -- cannot load it");
	}
	log_to_screen(_("Loading filelist"));
	asprintf(&command_to_open_fname, "gzip -dc %s", filelist_fname);
	asprintf(&tmp, "zcat %s | wc -l", filelist_fname);
	log_msg(6, "tmp = %s", tmp);
	tmp1 = call_program_and_get_last_line_of_output(tmp);
	paranoid_free(tmp);

	lines_in_filelist = atol(tmp1);
	paranoid_free(tmp1);

	if (lines_in_filelist < 3) {
		log_to_screen(_("Warning - surprisingly short filelist."));
	}
	g_original_noof_lines_in_filelist = lines_in_filelist;
	if (!(filelist = (struct s_node *) malloc(sizeof(struct s_node)))) {
		return (NULL);
	}
	filelist->ch = '/';
	filelist->right = NULL;
	filelist->down = malloc(sizeof(struct s_node));
	filelist->expanded = filelist->selected = FALSE;
	(filelist->down)->ch = '\0';
	(filelist->down)->right = (filelist->down)->down = FALSE;
	(filelist->down)->expanded = (filelist->down)->selected = FALSE;
	if (!(pin = popen(command_to_open_fname, "r"))) {
		log_OS_error("Unable to openin filelist_fname");
		return (NULL);
	}
	paranoid_free(command_to_open_fname);

	open_evalcall_form(_("Loading filelist from disk"));
	for (getline(&fname, &n, pin); !feof(pin); getline(&fname, &n, pin)) {
		if ((fname[strlen(fname) - 1] == 13
			 || fname[strlen(fname) - 1] == 10) && strlen(fname) > 0) {
			fname[strlen(fname) - 1] = '\0';
		}
		if (!strlen(fname)) {
			continue;
		}
		for (pos_in_fname = 0; fname[pos_in_fname] != '\0'; pos_in_fname++) {
			if (fname[pos_in_fname] != '/') {
				continue;
			}
			asprintf(&tmp, fname);
			tmp[pos_in_fname] = '\0';
			if (strlen(tmp)) {
				add_string_at_node(filelist, tmp);
			}
			paranoid_free(tmp);
		}
		add_string_at_node(filelist, fname);
		if (!(++lino % 1111)) {
			percentage = (int) (lino * 100 / lines_in_filelist);
			update_evalcall_form(percentage);
		}
	}
	paranoid_free(fname);
	paranoid_pclose(pin);
	close_evalcall_form();
	log_it("Finished loading filelist");
	return (filelist);
}


/**
 * Log a list of files in @p node.
 * @param node The toplevel node to use.
 */
void show_filelist(struct s_node *node)
{
	static int depth = 0;
	static char current_string[200];

	if (depth == 0) {
		log_msg(0, "----------------show filelist--------------");
	}
	current_string[depth] = node->ch;

	log_msg(3, "depth=%d", depth);
	if (node->down) {
		log_msg(3, "moving down");
		depth++;
		show_filelist(node->down);
		depth--;
	}

	if (!node->ch) {
		log_msg(0, "%s\n", current_string);
	}

	if (node->right) {
		log_msg(3, "moving right");
		show_filelist(node->right);
	}
	if (depth == 0) {
		log_msg(0, "----------------show filelist--------------");
	}
	return;
}


/**
 * Reset the filelist to the state it was when it was loaded. This does not
 * touch the file on disk.
 * @param filelist The filelist tree structure.
 */
void reload_filelist(struct s_node *filelist)
{
	assert(filelist != NULL);
	toggle_node_selection(filelist, FALSE);
	toggle_path_expandability(filelist, "/", FALSE);
	toggle_all_root_dirs_on(filelist);
}


/**
 * Save a filelist tree structure to disk.
 * @param filelist The filelist tree structure to save.
 * @param outfname Where to save it.
 */
void save_filelist(struct s_node *filelist, char *outfname)
{
	/*@ int ********************************************************* */
	static int percentage;
	static int depth = 0;

	/*@ buffers ***************************************************** */
	static char str[MAX_STR_LEN];

	/*@ structures ************************************************** */
	struct s_node *node;

	/*@ pointers **************************************************** */
	static FILE *fout = NULL;

	/*@ long ******************************************************** */
	static long lines_in_filelist = 0;
	static long lino = 0;

	/*@ end vars *************************************************** */

	assert(filelist != NULL);
	assert(outfname != NULL);	// will be zerolength if save_filelist() is called by itself
	if (depth == 0) {
		log_to_screen(_("Saving filelist"));
		if (!(fout = fopen(outfname, "w"))) {
			fatal_error("Cannot openout/save filelist");
		}
		lines_in_filelist = g_original_noof_lines_in_filelist;	/* set by load_filelist() */
		open_evalcall_form(_("Saving selection to disk"));
	}
	for (node = filelist; node != NULL; node = node->right) {
		str[depth] = node->ch;
		log_msg(5, "depth=%d ch=%c", depth, node->ch);
		if (!node->ch) {
//    if (node->selected)
//      {
			fprintf(fout, "%s\n", str);
//      }
			if (!(++lino % 1111)) {
				percentage = (int) (lino * 100 / lines_in_filelist);
				update_evalcall_form(percentage);
			}
		}
		if (node->down) {
			depth++;
			save_filelist(node->down, "");
			depth--;
		}
	}
	if (depth == 0) {
		paranoid_fclose(fout);
		close_evalcall_form();
		log_it("Finished saving filelist");
	}
}


/**
 * Toggle all root dirs on.
 * @param filelist The filelist tree structure to operate on.
 * @bug I don't understand this function. Would someone care to explain it?
 */
void toggle_all_root_dirs_on(struct s_node *filelist)
{
	/*@ structures ************************************************** */
	struct s_node *node;

	/*@ int ********************************************************* */
	static int depth = 0;
	static int root_dirs_expanded;

	/*@ buffers ***************************************************** */
	static char filename[MAX_STR_LEN];

	/*@ end vars *************************************************** */

	assert(filelist != NULL);
	if (depth == 0) {
		log_it("Toggling all root dirs ON");
		root_dirs_expanded = 0;
	}
	for (node = filelist; node != NULL; node = node->right) {
		filename[depth] = node->ch;
		if (node->ch == '\0' && strlen(filename) > 1
			&& (!strchr(filename + 1, '/'))) {
			node->selected = FALSE;
			node->expanded = TRUE;
//    log_it (filename);
			root_dirs_expanded++;
		}
		if (node->down) {
			depth++;
			toggle_all_root_dirs_on(node->down);
			depth--;
		}
	}
	if (depth == 0) {
		log_it("Finished toggling all root dirs ON");
	}
}


/**
 * Toggle the expandability of a path.
 * @param filelist The filelist tree to operate on.
 * @param pathname The path to toggle expandability of.
 * @param on_or_off Whether to toggle it on or off.
 * @bug I don't understand this function. Would someone care to explain it?
 */
void
toggle_path_expandability(struct s_node *filelist, char *pathname,
						  bool on_or_off)
{

	/*@ int ******************************************************** */
	static int depth = 0;
	static int total_expanded;
	static int root_depth;
	int j;
	/*@ structures ************************************************* */
	struct s_node *node;

	/*@ buffers **************************************************** */
	static char current_filename[MAX_STR_LEN];

	/*@ end vars *************************************************** */

	assert(filelist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(pathname);
	if (depth == 0) {
		total_expanded = 0;
//      log_it ("Toggling path's expandability");
		for (root_depth = (int) strlen(pathname);
			 root_depth > 0 && pathname[root_depth - 1] != '/';
			 root_depth--);
		if (root_depth < 2) {
			root_depth = (int) strlen(pathname);
		}
	}
	for (node = filelist; node != NULL; node = node->right) {
		current_filename[depth] = node->ch;
		if (node->down) {
			depth++;
			toggle_path_expandability(node->down, pathname, on_or_off);
			depth--;
		}
		if (node->ch == '\0') {
			if (!strncmp(pathname, current_filename, strlen(pathname))) {
				for (j = root_depth;
					 current_filename[j] != '/'
					 && current_filename[j] != '\0'; j++);
				if (current_filename[j] != '\0') {
					for (j++;
						 current_filename[j] != '/'
						 && current_filename[j] != '\0'; j++);
				}
				if (current_filename[j] == '\0') {
					node->expanded =
						(!strcmp(pathname, current_filename) ? TRUE :
						 on_or_off);
				}
			}
		}
		if (node->expanded) {
			if (total_expanded < ARBITRARY_MAXIMUM - 32
				|| !strrchr(current_filename + strlen(pathname), '/')) {
				total_expanded++;
			} else {
				node->expanded = FALSE;
			}
		}
	}
	if (depth == 0) {
//      log_it ("Finished toggling expandability");
	}
}


/**
 * Toggle whether a path is selected.
 * @param filelist The filelist tree to operate on.
 * @param pathname The path to toggle selection of.
 * @param on_or_off Whether to toggle it on or off.
 * @bug I don't understand this function. Would someone care to explain it?
 */
void
toggle_path_selection(struct s_node *filelist, char *pathname,
					  bool on_or_off)
{
	/*@ int ********************************************************* */
	static int depth = 0;
	int j;

	/*@ structures ************************************************** */
	struct s_node *node;

	/*@ buffers ***************************************************** */
	static char current_filename[MAX_STR_LEN];

	/*@ end vars *************************************************** */
	assert(filelist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(pathname);
	if (depth == 0) {
		log_it("Toggling path's selection");
	}
	for (node = filelist; node != NULL; node = node->right) {
		current_filename[depth] = node->ch;
		if (node->down) {
			depth++;
			toggle_path_selection(node->down, pathname, on_or_off);
			depth--;
		}
		if (node->ch == '\0') {
			if (!strncmp(pathname, current_filename, strlen(pathname))) {
				for (j = 0;
					 pathname[j] != '\0'
					 && pathname[j] == current_filename[j]; j++);
				if (current_filename[j] == '/'
					|| current_filename[j] == '\0') {
					node->selected = on_or_off;
				}
			}
		}
	}
	if (depth == 0) {
		log_it("Finished toggling selection");
	}
}


/**
 * Toggle node selection of a filelist tree.
 * @param filelist The filelist tree to operate on.
 * @param on_or_off Whether to toggle selection on or off.
 * @bug I don't understand this function. Would someone care to explain it?
 */
void toggle_node_selection(struct s_node *filelist, bool on_or_off)
{
	/*@ structure ************************************************** */
	struct s_node *node;

	/*@ end vars *************************************************** */
	assert(filelist != NULL);
	for (node = filelist; node != NULL; node = node->right) {
		if (node->ch == '/') {
			continue;
		}						/* don't go deep */
		if (node->ch == '\0') {
			node->selected = on_or_off;
		}
		if (node->down) {
			toggle_node_selection(node->down, on_or_off);
		}
	}
}


/**
 * Number of entries in the skeleton filelist.
 */
long g_skeleton_entries = 0;


/**
 * Wrapper around mondo_makefilelist().
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->differential
 * - @c bkpinfo->exclude_paths
 * - @c bkpinfo->include_paths
 * - @c bkpinfo->make_filelist
 * - @c bkpinfo->scratchdir
 * - @c bkpinfo->tmpdir
 * @return 0 for success, nonzero for failure.
 * @see mondo_makefilelist
 */
int prepare_filelist(struct s_bkpinfo *bkpinfo)
{

	/*@ int **************************************************** */
	int res = 0;
	/*@ buffers ************************************************ */

	assert(bkpinfo != NULL);
	log_it("tmpdir=%s; scratchdir=%s", bkpinfo->tmpdir,
		   bkpinfo->scratchdir);
	if (bkpinfo->make_filelist) {
		mvaddstr_and_log_it(g_currentY, 0,
							_("Making catalog of files to be backed up"));
	} else {
		mvaddstr_and_log_it(g_currentY, 0,
							_("Using supplied catalog of files to be backed up"));
	}

	if (bkpinfo->make_filelist) {
		res =
			mondo_makefilelist(MONDO_LOGFILE, bkpinfo->tmpdir,
							   bkpinfo->scratchdir, bkpinfo->include_paths,
							   bkpinfo->exclude_paths,
							   bkpinfo->differential, NULL);
	} else {
		res =
			mondo_makefilelist(MONDO_LOGFILE, bkpinfo->tmpdir,
							   bkpinfo->scratchdir, NULL,
							   bkpinfo->exclude_paths,
							   bkpinfo->differential,
							   bkpinfo->include_paths);
	}

	if (res) {
		log_OS_error("Call to mondo_makefilelist failed");
		mvaddstr_and_log_it(g_currentY++, 74, _("Failed."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}
	return (res);
}


/**
 * Recursively list all files in @p dir newer than @p time_of_last_full_backup to @p fout.
 * @param dir The directory to list to @p fout.
 * @param sth The directories to skip (exclude).
 * @param fout The file to write everything to.
 * @param time_of_last_full_backup Only backup files newer than this (0 to disable).
 * @return 0, always.
 * @bug Return value should be @c void.
 */
int open_and_list_dir(char *dir, char *sth, FILE * fout,
					  time_t time_of_last_full_backup,
					  char *skeleton_filelist)
{
	DIR *dip = NULL;
	struct dirent *dit = NULL;
	struct stat statbuf;
	char *new = NULL;
	char *tmp = NULL;
	char *tmp1 = NULL;
	static int percentage = 0;
	char *skip_these = NULL;
	char *new_with_spaces = NULL;
	static char *name_of_evalcall_form = NULL;
	int i = 0;
	static int depth = 0;
	char *p = NULL;
	static int counter = 0;
	static int uberctr = 0;
	static char *find_skeleton_marker = NULL;
	static long skeleton_lino = 0;
	static time_t last_time = 0;
	time_t this_time;

	p = strrchr(dir, '/');
	if (p) {
		if (!strcmp(p, "/.") || !strcmp(p, "/..")) {
			return (0);
		}
	}

	if (!depth) {
		malloc_string(find_skeleton_marker);
#if linux
		// 2.6 has /sys as a proc-type thing -- must be excluded
		asprintf(&tmp,
				 "find %s -maxdepth %d -fstype mvfs -prune -o -path /proc -prune -o -path /sys -prune -o -path /dev/shm -prune -o -path /media/floppy -prune -o -type d -a -print > %s 2> /dev/null",
				 dir, MAX_SKEL_DEPTH, skeleton_filelist);
#else
		// On BSD, for example, /sys is the kernel sources -- don't exclude
		asprintf(&tmp,
				 "find %s -maxdepth %d -fstype mvfs -prune -o -path /proc -prune -o -type d -a -print > %s 2> /dev/null",
				 dir, MAX_SKEL_DEPTH, skeleton_filelist);
#endif
		system(tmp);
		paranoid_free(tmp);

		asprintf(&tmp, "wc -l %s | awk '{print $1;}'", skeleton_filelist);
		tmp1 = call_program_and_get_last_line_of_output(tmp);
		paranoid_free(tmp);

		g_skeleton_entries = 1 + atol(tmp1);
		paranoid_free(tmp1);

		asprintf(&name_of_evalcall_form, "Making catalog of %s", dir);
		open_evalcall_form(name_of_evalcall_form);
		find_skeleton_marker[0] = '\0';
		skeleton_lino = 1;
		log_msg(5, "entries = %ld", g_skeleton_entries);
		percentage = 0;
	} else if (depth <= MAX_SKEL_DEPTH)	// update evalcall form if appropriate
	{
		sprintf(find_skeleton_marker,
				"grep -Fv \"%s\" %s > %s.new 2> /dev/null", dir,
				skeleton_filelist, skeleton_filelist);
//    log_msg(0, "fsm = %s", find_skeleton_marker);
		if (!system(find_skeleton_marker)) {
			percentage = (int) (skeleton_lino * 100 / g_skeleton_entries);
			skeleton_lino++;
//        log_msg(5, "Found %s", dir);
//        log_msg(2, "Incrementing skeleton_lino; now %ld/%ld (%d%%)", skeleton_lino, g_skeleton_entries, percentage);
			sprintf(find_skeleton_marker, "mv -f %s.new %s",
					skeleton_filelist, skeleton_filelist);
//        log_msg(6, "fsm = %s", find_skeleton_marker);
			run_program_and_log_output(find_skeleton_marker, 8);
			time(&this_time);
			if (this_time != last_time) {
				last_time = this_time;
#ifndef _XWIN
				if (!g_text_mode) {
					asprintf(&tmp, _("Reading %-68s"), dir);
					newtDrawRootText(0, g_noof_rows - 3, tmp);
					paranoid_free(tmp);
				}
#endif
				update_evalcall_form(percentage);
			}
		}
	}

	depth++;

//  log_msg(0, "Cataloguing %s", dir);
	if (sth[0] == ' ') {
		asprintf(&skip_these, "%s", sth);
	} else {
		asprintf(&skip_these, " %s ", sth);
	}
	asprintf(&new_with_spaces, " %s ", dir);
	if ((dip = opendir(dir)) == NULL) {
		log_OS_error("opendir");
	} else if (strstr(skip_these, new_with_spaces)) {
		fprintf(fout, "%s\n", dir);	// if excluded dir then print dir ONLY
	} else {
		fprintf(fout, "%s\n", dir);
		while ((dit = readdir(dip)) != NULL) {
			i++;
			if (strcmp(dir, "/")) {
				asprintf(&new, "%s/%s", dir, dit->d_name);
			} else {
				asprintf(&new, "%s%s", dir, dit->d_name);
			}
			paranoid_free(new_with_spaces);
			asprintf(&new_with_spaces, " %s ", new);
			/* BERLIOS: Old code
			   new_with_spaces[0] = ' ';
			   strcpy(new_with_spaces + 1, new);
			   strcat(new_with_spaces, " ");
			 */
			if (strstr(skip_these, new_with_spaces)) {
				fprintf(fout, "%s\n", new);
			} else {
				if (!lstat(new, &statbuf)) {
					if (!S_ISLNK(statbuf.st_mode)
						&& S_ISDIR(statbuf.st_mode)) {
						open_and_list_dir(new, skip_these, fout,
										  time_of_last_full_backup,
										  skeleton_filelist);
					} else {
						if (time_of_last_full_backup == 0
							|| time_of_last_full_backup <
							statbuf.st_ctime) {
							fprintf(fout, "%s\n", new);
							if ((counter++) > 128) {
								counter = 0;
								uberctr++;
								asprintf(&tmp, " %c ",
										 special_dot_char(uberctr));
#ifndef _XWIN
								if (!g_text_mode) {
									newtDrawRootText(77, g_noof_rows - 3,
													 tmp);
									newtRefresh();
								}
								paranoid_free(tmp);
#endif
							}
						}
					}
				}
			}
			paranoid_free(new);
		}
	}
	paranoid_free(new_with_spaces);
	paranoid_free(skip_these);

	if (dip) {
		if (closedir(dip) == -1) {
			log_OS_error("closedir");
		}
	}
	depth--;
	if (!depth) {
		close_evalcall_form();
		paranoid_free(name_of_evalcall_form);
		paranoid_free(find_skeleton_marker);
		unlink(skeleton_filelist);
		log_msg(5, "g_skeleton_entries = %ld", g_skeleton_entries);
	}
	return (0);
}


/**
 * Get the next entry in the space-separated list in @p incoming.
 * So if @p incoming was '"one and two" three four', we would
 * return "one and two".
 * @param incoming The list to get the next entry from.
 * @return The first item in the list (respecting double quotes).
 * @note The returned string points to static data that will be overwritten with each call.
 */
char *next_entry(char *incoming)
{
	static char sz_res[MAX_STR_LEN];
	char *p;
	bool in_quotes = FALSE;

	strcpy(sz_res, incoming);
	p = sz_res;
	while ((*p != ' ' || in_quotes) && *p != '\0') {
		if (*p == '\"') {
			in_quotes = !in_quotes;
		}
		p++;
	}
	*p = '\0';
	return (sz_res);
}


/**
 * Create the filelist for the backup. It will be stored in [scratchdir]/archives/filelist.full.
 * @param logfile Unused.
 * @param tmpdir The tmpdir of the backup.
 * @param scratchdir The scratchdir of the backup.
 * @param include_paths The paths to back up, or NULL if you're using a user-defined filelist.
 * @param excp The paths to NOT back up.
 * @param differential The differential level (currently only 0 and 1 are supported).
 * @param userdef_filelist The user-defined filelist, or NULL if you're using @p include_paths.
 * @return 0, always.
 * @bug @p logfile is unused.
 * @bug Return value is meaningless.
 */
int mondo_makefilelist(char *logfile, char *tmpdir, char *scratchdir,
					   char *include_paths, char *excp, int differential,
					   char *userdef_filelist)
{
	char sz_datefile_wildcard[] = "/var/cache/mondo/difflevel.%d";
	char *p = NULL;
   	char *q = NULL;
   	char *tmp = NULL;
   	char *tmp1 = NULL;
   	char *tmp2 = NULL;
	char *sz_datefile;
	char *sz_filelist = NULL;
   	char *exclude_paths = NULL;
	int i = 0;
	FILE *fout = NULL;
	char *command = NULL;
	time_t time_of_last_full_backup = 0;
	struct stat statbuf;
	char *skeleton_filelist = NULL;

	// The pathname to the skeleton filelist, used to give better progress reporting for mondo_makefilelist().
	asprintf(&sz_datefile, sz_datefile_wildcard, 0);
	if (!include_paths && !userdef_filelist) {
		fatal_error
			("Please supply either include_paths or userdef_filelist");
	}
// make hole for filelist
	asprintf(&command, "mkdir -p %s/archives", scratchdir);
	paranoid_system(command);
	paranoid_free(command);

	asprintf(&sz_filelist, "%s/tmpfs/filelist.full", tmpdir);
	make_hole_for_file(sz_filelist);

	if (differential == 0) {
		// restore last good datefile if it exists
		asprintf(&command, "cp -f %s.aborted %s", sz_datefile,
				 sz_datefile);
		run_program_and_log_output(command, 3);
		paranoid_free(command);

		// backup last known good datefile just in case :)
		if (does_file_exist(sz_datefile)) {
			asprintf(&command, "mv -f %s %s.aborted", sz_datefile,
					 sz_datefile);
			paranoid_system(command);
			paranoid_free(command);
		}
		make_hole_for_file(sz_datefile);
		tmp = call_program_and_get_last_line_of_output("date +%s");
		write_one_liner_data_file(sz_datefile, tmp);
		paranoid_free(tmp);
	} else if (lstat(sz_datefile, &statbuf)) {
		log_msg(2,
				"Warning - unable to find date of previous backup. Full backup instead.");
		differential = 0;
		time_of_last_full_backup = 0;
	} else {
		time_of_last_full_backup = statbuf.st_mtime;
		log_msg(2, "Differential backup. Yay.");
	}
	paranoid_free(sz_datefile);

// use user-specified filelist (if specified)
	if (userdef_filelist) {
		log_msg(1,
				"Using the user-specified filelist - %s - instead of calculating one",
				userdef_filelist);
		asprintf(&command, "cp -f %s %s", userdef_filelist, sz_filelist);
		if (run_program_and_log_output(command, 3)) {
			fatal_error("Failed to copy user-specified filelist");
		}
		paranoid_free(command);
	} else {
		log_msg(2, "include_paths = '%s'", include_paths);
		log_msg(1, "Calculating filelist");
		tmp = call_program_and_get_last_line_of_output("locate /win386.swp 2> /dev/null");
		tmp1 = call_program_and_get_last_line_of_output("locate /hiberfil.sys 2> /dev/null");
		tmp2 = call_program_and_get_last_line_of_output("locate /pagefile.sys 2> /dev/null");
		asprintf(&exclude_paths, " %s %s %s %s %s %s . .. \
" MNT_CDROM " " MNT_FLOPPY " /media/cdrom /media/cdrecorder \
/proc /sys /tmp /var/cache/mondo /var/cache/mindi", excp, tmp, tmp1, tmp2, (tmpdir[0] == '/' && tmpdir[1] == '/') ? (tmpdir + 1) : tmpdir, (scratchdir[0] == '/' && scratchdir[1] == '/') ? (scratchdir + 1) : scratchdir);
		paranoid_free(tmp);
		paranoid_free(tmp1);
		paranoid_free(tmp2);

		log_msg(2, "Excluding paths = '%s'", exclude_paths);
		log_msg(2,
				"Generating skeleton filelist so that we can track our progress");
		asprintf(&skeleton_filelist, "%s/tmpfs/skeleton.txt", tmpdir);
		make_hole_for_file(skeleton_filelist);

		log_msg(4, "g_skeleton_entries = %ld", g_skeleton_entries);
		log_msg(2, "Opening out filelist to %s", sz_filelist);
		if (!(fout = fopen(sz_filelist, "w"))) {
			fatal_error("Cannot openout to sz_filelist");
		}
		i = 0;
		if (strlen(include_paths) == 0) {
			log_msg(1, "Including only '/' in %s", sz_filelist);
			open_and_list_dir("/", exclude_paths, fout,
							  time_of_last_full_backup, skeleton_filelist);
		} else {
			p = include_paths;
			while (*p) {
				q = next_entry(p);
				log_msg(1, "Including %s in filelist %s", q, sz_filelist);
				open_and_list_dir(q, exclude_paths, fout,
								  time_of_last_full_backup,
								  skeleton_filelist);
				p += strlen(q);
				while (*p == ' ') {
					p++;
				}
			}
		}
		paranoid_fclose(fout);
		paranoid_free(skeleton_filelist);
		paranoid_free(exclude_paths);
	}
	log_msg(2, "Copying new filelist to scratchdir");
	asprintf(&command, "mkdir -p %s/archives", scratchdir);
	paranoid_system(command);
	paranoid_free(command);

	asprintf(&command, "cp -f %s %s/archives/", sz_filelist, scratchdir);
	paranoid_system(command);
	paranoid_free(command);

	asprintf(&command, "mv -f %s %s", sz_filelist, tmpdir);
	paranoid_system(command);
	paranoid_free(command);
	paranoid_free(sz_filelist);

	log_msg(2, "Exiting");
	return (0);
}


/**
 * Locate the string @p string_to_find in the tree rooted at @p startnode.
 * @param startnode The node containing the root of the directory tree.
 * @param string_to_find The string to look for at @p startnode.
 * @return The node containing the last element of @p string_to_find, or NULL if
 * it was not found.
 */
struct s_node *find_string_at_node(struct s_node *startnode,
								   char *string_to_find)
{
	/*@ int ******************************************************** */
	int noof_chars;
	static int depth = 0;
	static char original_string[MAX_STR_LEN];

	/*@ sturctures ************************************************* */
	struct s_node *node;

	/*@ char  ****************************************************** */
	char char_to_find;

	/*@ bools ****************************************************** */

	if (!depth) {
		strcpy(original_string, string_to_find);
	}

	assert(startnode != NULL);
	assert(string_to_find != NULL);

	noof_chars = strlen(string_to_find) + 1;	/* we include the '\0' */

	log_msg(7, "starting --- str=%s", string_to_find);

/* walk across tree if necessary */
	node = startnode;
	char_to_find = string_to_find[0];
	if (node->right != NULL && node->ch < char_to_find) {
		log_msg(7, "depth=%d --- going RIGHT ... %c-->%c", depth,
				char_to_find, node->ch, (node->right)->ch);
		return (find_string_at_node(node->right, string_to_find));
	}

/* walk down tree if appropriate */
	if (node->down != NULL && node->ch == char_to_find) {
		log_msg(7, "depth=%d char=%c --- going DOWN", depth, char_to_find);
		depth++;
		node = find_string_at_node(node->down, string_to_find + 1);
		depth--;
		return (node);
	}

	if (char_to_find == '\0' && node->ch == '\0') {
		log_msg(7, "%s is in tree", original_string);
		return (node);
	} else {
		log_msg(7, "%s is NOT in tree", original_string);
		return (NULL);
	}
}


/**
 * Write all entries in @p needles_list_fname which are also in
 * @p filelist to @p matches_list_fname.
 * @param needles_list_fname A file containing strings to look for, 1 per line.
 * @param filelist The node for the root of the directory structure to search in.
 * @param matches_list_fname The filename where we should put the matches.
 * @return The number of matches found.
 */
long save_filelist_entries_in_common(char *needles_list_fname,
									 struct s_node *filelist,
									 char *matches_list_fname,
									 bool use_star)
{
	int retval = 0;
	struct s_node *found_node;
	FILE *fin;
	FILE *fout;
	char *fname = NULL;
	char *tmp;
	size_t len = 0;				// Scrub's patch doesn't work without that

//  log_msg(1, "use_star = %s", (use_star)?"TRUE":"FALSE");
	log_msg(5, "starting");
	log_msg(5, "needles_list_fname = %s", needles_list_fname);
	log_msg(5, "matches_list_fname = %s", matches_list_fname);
	if (!(fin = fopen(needles_list_fname, "r"))) {
		fatal_error("Cannot openin needles_list_fname");
	}
	if (!(fout = fopen(matches_list_fname, "w"))) {
		fatal_error("Cannot openout matches_list_fname");
	}
	while (!feof(fin)) {
		getline(&fname, &len, fin);
		if (!use_star) {
			if (fname[0] == '/') {
				asprintf(&tmp, fname);
			} else {
				asprintf(&tmp, "/%s", fname);
			}
			paranoid_free(fname);
			fname = tmp;
		}
		while (strlen(fname) > 0 && fname[strlen(fname) - 1] < 32) {
			fname[strlen(fname) - 1] = '\0';
		}

		log_msg(5, "Looking for '%s'", fname);
		found_node = find_string_at_node(filelist, fname);
		if (found_node) {
			if (found_node->selected) {
				if (fname[0] == '/') {
					asprintf(&tmp, fname + 1);
					paranoid_free(fname);
					fname = tmp;
				}
				log_msg(5, "Found '%s'", fname);
				turn_wildcard_chars_into_literal_chars(tmp, fname);
				fprintf(fout, "%s\n", tmp);
				paranoid_free(tmp);
				retval++;
			}
		}
		paranoid_free(fname);
	}
	paranoid_fclose(fout);
	paranoid_fclose(fin);
	return (retval);
}


/**
 * Add all files listed in @p list_of_files_fname to the directory structure rooted at
 * @p filelist.
 * @param filelist The top node of the directory structure to add the files to.
 * @param list_of_files_fname The file containing the files to add, 1 per line.
 * @param flag_em If TRUE, then flag the added files for restoration.
 * @return 0 for success, nonzero for failure.
 */
int add_list_of_files_to_filelist(struct s_node *filelist,
								  char *list_of_files_fname, bool flag_em)
{
	FILE *fin;
	char *tmp = NULL;
	size_t n = 0;
	struct s_node *nod;

	log_msg(3, "Adding %s to filelist", list_of_files_fname);
	if (!(fin = fopen(list_of_files_fname, "r"))) {
		iamhere(list_of_files_fname);
		return (1);
	}
	for (getline(&tmp, &n, fin); !feof(fin); getline(&tmp, &n, fin)) {
		if (!tmp[0]) {
			continue;
		}
		if ((tmp[strlen(tmp) - 1] == 13 || tmp[strlen(tmp) - 1] == 10)
			&& strlen(tmp) > 0) {
			tmp[strlen(tmp) - 1] = '\0';
		}
		log_msg(2, "tmp = '%s'", tmp);
		if (!tmp[0]) {
			continue;
		}
		if ((nod = find_string_at_node(filelist, tmp))) {
			log_msg(5, "Found '%s' in filelist already. Cool.", tmp);
		} else {
			add_string_at_node(filelist, tmp);
			nod = find_string_at_node(filelist, tmp);
		}

		if (nod && flag_em) {
			toggle_path_selection(filelist, tmp, TRUE);
			log_msg(5, "Flagged '%s'", tmp);
		}
	}
	paranoid_fclose(fin);
	paranoid_free(tmp);
	return (0);
}

/* @} - end of filelistGroup */
