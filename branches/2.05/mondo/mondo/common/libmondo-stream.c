/* libmondo-stream.c
   $Id$

...tools for talking to tapes, Monitas streams, etc.


07/10
- added ACL and XATTR support

04/17
- added bkpinfo->internal_tape_block_size
- fixed bug in set_tape_block_size_with_mt()

03/25
- call 'mt' when opening tape in/out, to set block size to 32K

02/06/2004
- only eject_device() at end of closein_tape() --- not closeout_tape()

10/26/2003
- call eject_device() at end of closein_tape() and closeout_tape()
 
09/25
- added docu-header to each subroutine

09/20
- working on multi-tape code
- friendlier message in insist_on_this_tape_number()
- delay is now 5s

09/10
- adding multi-tape support
- adding 'back catalog' of previous archives, so that we may resume
  from point of end-of-tape (approx.)

09/08
- cleaned up insist_on_this_tape_number() --- 10s delay for same tape now

08/27
- improved find_tape_device_and_size() --- tries /dev/osst*

05/05
- added Joshua Oreman's FreeBSD patches

04/24
- added lots of assert()'s and log_OS_error()'s

04/06/2003
- use find_home_of_exe() instead of which

12/13/2002
- cdrecord -scanbus call was missing 2> /dev/null

11/24
- disabled fatal_error("Bad marker")

10/29
- replaced convoluted grep wih wc (KP)

09/07
- removed g_end_of_tape_reached
- lots of multitape-related fixes

08/01 - 08/31
- write 16MB of zeroes to end of tape when closing(out)
- compensate for 'buffer' and its distortion of speed of writing/reading
  when writing/reading data disks at start of tape
- rewrote lots of multitape stuff
- wrote workaround to allow >2GB of archives w/buffering
- do not close/reopen tape when starting to read/write from
  new tape: no need! 'buffer' handles all that; we're writing
  to/reading from a FIFO, so no need to close/reopen when new tape
- write 8MB of zeroes at end of tape, just in case
- re-enable various calls to *_evalcall_form
- added g_end_of_tape_reached
- fixed bugs in start_to_[read|write]_[to|from]_next_tape
- added internal buffering, replacing the external 'buffer' exe
- wait 10 seconds (after user inserts new tape), to
  let tape stabilize in drive
- added insist_on_this_tape_number()
- worked on find_tape_device_and_size()
- cleaned up some log_it() calls
- added find_tape_device_and_size()
- replaced using_cdstream with backup_media_type
- replace *_from_tape with *_from_stream
- replace *_to_stream with *_to_stream

07/01 - 07/31
- leave 32MB at end of tape, to avoid overrunning
- started [07/24/2002]
*/


/**
 * @file
 * Functions for writing data to/reading data from streams (tape, CD stream, etc.)
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-devices.h"
#include "lib-common-externs.h"
#include "libmondo-stream.h"
#include "libmondo-string-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-fifo-EXT.h"

#define EXTRA_TAPE_CHECKSUMS

/*@unused@*/
//static char cvsid[] = "$Id$";
extern bool g_sigpipe;
extern int g_tape_buffer_size_MB;

/**
 * @addtogroup globalGroup
 * @{
 */
/**
 * The file pointer for the opened tape/CD stream.
 * Opened and closed by openin_tape(), openin_cdstream(),
 * openout_tape(), openout_cdstream(), closein_tape(), closein_cdstream(),
 * closeout_tape(), and closeout_cdstream().
 */
FILE *g_tape_stream = NULL;


/**
 * The position (in kilobytes) where we are on the tape.
 */
long long g_tape_posK = 0;

/**
 * The current media number we're using. This value is 1-based.
 */
int g_current_media_number = -1;

/**
 * The tape catalog that keeps track of files written to tape.
 */
struct s_tapecatalog *g_tapecatalog;

/* @} - end of globalGroup */

int write_backcatalog_to_tape(struct s_bkpinfo *bkpinfo);





/**
 * @addtogroup streamGroup
 * @{
 */
/**
 * Close the global output file descriptor which Mondo has used to read
 * from the CD stream.
 * @param bkpinfo The backup information structure. Passed directly to closein_tape().
 * @return 0 for success, nonzero for failure.
 * @note This should be called by restore processes only.
 */
int closein_cdstream(struct s_bkpinfo *bkpinfo)
{
	return (closein_tape(bkpinfo));
}


/**
 * Close the global output file descriptor which Mondo has used to read
 * from buffer or dd, which read from the tape.
 * @param bkpinfo The backup information structure. Unused.
 * @return 0 for success, nonzero for failure.
 * @note This should be called by restore processes only.
 * @note This function also works for cdstreams for now, but don't count on this behavior.
 * @bug @p bkpinfo parameter is unused.
 */
int closein_tape(struct s_bkpinfo *bkpinfo)
{
	/*@ int's ******************************************************* */
	int retval = 0;
	int res = 0;
	int ctrl_chr = '\0';

	/*@ buffers ***************************************************** */
	char fname[MAX_STR_LEN];

	/*@ long long's ************************************************* */
	long long size;
	char *blk;
	int i;

	blk = (char *) malloc(256 * 1024);

	log_it("closein_tape() -- entering");
	res = read_header_block_from_stream(&size, fname, &ctrl_chr);
	retval += res;
	if (ctrl_chr != BLK_END_OF_BACKUP) {
		wrong_marker(BLK_END_OF_BACKUP, ctrl_chr);
	}
	res = read_header_block_from_stream(&size, fname, &ctrl_chr);
	retval += res;
	if (ctrl_chr != BLK_END_OF_TAPE) {
		wrong_marker(BLK_END_OF_TAPE, ctrl_chr);
	}
	for (i = 0; i < 8 && !feof(g_tape_stream); i++) {
		(void) fread(blk, 1, 256 * 1024, g_tape_stream);
	}
	sleep(1);
	paranoid_system("sync");
	sleep(1);
	paranoid_pclose(g_tape_stream);
	log_it("closein_tape() -- leaving");
/*
  for(i=0; i < g_tapecatalog->entries; i++)
    {
      log_it("i=%d type=%s num=%d aux=%ld", i, (g_tapecatalog->el[i].type==fileset)?"fileset":"bigslice", g_tapecatalog->el[i].number, g_tapecatalog->el[i].aux);
    }
*/
	if (!bkpinfo->please_dont_eject) {
		eject_device(bkpinfo->media_device);
	}
	paranoid_free(blk);
	paranoid_free(g_tapecatalog);
	return (retval);
}



/**
 * Close the global output file descriptor which Mondo has been using to write
 * to the tape device (via buffer or dd).
 * @param bkpinfo The backup information structure. @c bkpinfo->media_size is the only field used.
 * @return 0 for success, nonzero for failure.
 * @note This should be called by backup processes only.
 */
int closeout_tape(struct s_bkpinfo *bkpinfo)
{
	/*@ int's ******************************************************* */
	int retval = 0;
//  int res = 0;
//  int ctrl_chr = '\0';

	/*@ buffers ***************************************************** */
//  char fname[MAX_STR_LEN];

	/*@ long long's ************************************************* */
//  long long size;
	int i;
	char *blk;

	blk = (char *) malloc(256 * 1024);

	sleep(1);
	paranoid_system("sync");
	sleep(1);
	log_it("closeout_tape() -- entering");
	retval +=
		write_header_block_to_stream(0, "end-of-backup",
									 BLK_END_OF_BACKUP);
	retval += write_header_block_to_stream(0, "end-of-tape", BLK_END_OF_TAPE);	/* just in case */
/* write 1MB of crap */
	for (i = 0; i < 256 * 1024; i++) {
		blk[i] = (int) (random() & 0xFF);
	}
	for (i = 0; i < 4 * 8; i++) {
		(void) fwrite(blk, 1, 256 * 1024, g_tape_stream);
		if (should_we_write_to_next_tape
			(bkpinfo->media_size[g_current_media_number], 256 * 1024)) {
			start_to_write_to_next_tape(bkpinfo);
		}
	}
/* write 1MB of zeroes */
/*
    for (i = 0; i < 256*1024; i++)
      {
        blk[i] = 0;
      }
    for (i = 0; i < 4; i++)
      {
        fwrite (blk, 1, 256*1024, g_tape_stream);
        if (should_we_write_to_next_tape (bkpinfo->media_size[g_current_media_number], 256*1024))
          {
            start_to_write_to_next_tape (bkpinfo);
          }
      }
*/
	sleep(2);
	paranoid_pclose(g_tape_stream);
	log_it("closeout_tape() -- leaving");
	for (i = 0; i < g_tapecatalog->entries; i++) {
		log_it("i=%d type=%s num=%d aux=%ld posK=%lld", i,
			   (g_tapecatalog->el[i].type ==
				fileset) ? "fileset" : "bigslice",
			   g_tapecatalog->el[i].number, g_tapecatalog->el[i].aux,
			   g_tapecatalog->el[i].tape_posK);
	}
	//  if (!bkpinfo->please_dont_eject)
	//    { eject_device(bkpinfo->media_device); }
	paranoid_free(blk);
	paranoid_free(g_tapecatalog);
	return (retval);
}



bool mt_says_tape_exists(char *dev)
{
	char *command;
	int res;

	malloc_string(command);
	sprintf(command, "mt -f %s status", dev);
	res = run_program_and_log_output(command, 1);
	paranoid_free(command);
	if (res) {
		return (FALSE);
	} else {
		return (TRUE);
	}
}



/**
 * Determine the name and size of the tape device. Tries the SCSI tape for
 * this platform, then the IDE tape, then "/dev/st0", then "/dev/osst0".
 * @param dev Where to put the found tape device.
 * @param siz Where to put the tape size (a string like "4GB")
 * @return 0 if success, nonzero if failure (in which @p dev and @p siz are undefined).
 */
int find_tape_device_and_size(char *dev, char *siz)
{
	char tmp[MAX_STR_LEN];
	char command[MAX_STR_LEN * 2];
	char cdr_exe[MAX_STR_LEN];
//  char tape_description_[MAX_STR_LEN];
//  char tape_manufacturer_cdr[MAX_STR_LEN];
//  FILE*fin;
	int res;

	log_to_screen("I am looking for your tape streamer. Please wait.");
	dev[0] = siz[0] = '\0';
	if (find_home_of_exe("cdrecord")) {
		strcpy(cdr_exe, "cdrecord");
	} else {
		strcpy(cdr_exe, "dvdrecord");
	}
	sprintf(command, "%s -scanbus 2> /dev/null | grep -i tape | wc -l",
			cdr_exe);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	if (atoi(tmp) != 1) {
		log_it
			("Either too few or too many tape streamers for me to detect...");
		strcpy(dev, VANILLA_SCSI_TAPE);
		return 1;
	}
	sprintf(command,
			"%s -scanbus 2> /dev/null | tr -s '\t' ' ' | grep \"[0-9]*,[0-9]*,[0-9]*\" | grep -v \"[0-9]*) \\*\" | grep -i TAPE | cut -d' ' -f2 | head -n1",
			cdr_exe);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	if (strlen(tmp) < 2) {
		log_it("Could not find tape device");
		return 1;
	}
	sprintf(command,
			"%s -scanbus 2> /dev/null | tr -s '\t' ' ' | grep \"[0-9]*,[0-9]*,[0-9]*\" | grep -v \"[0-9]*) \\*\" | grep -i TAPE | cut -d' ' -f3 | cut -d')' -f1 | head -n1",
			cdr_exe);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	strcpy(dev, VANILLA_SCSI_TAPE);
	dev[strlen(dev) - 1] = '\0';
	strcat(dev, tmp);			// e.g. '/dev/st0' becomes '/dev/stN'
	res = 0;
	if (!mt_says_tape_exists(dev)) {
		strcpy(dev, ALT_TAPE);
		if (!mt_says_tape_exists(dev)) {
			log_it("Cannot openin %s", dev);
			strcpy(dev, "/dev/st0");
			if (!mt_says_tape_exists(dev)) {
				log_it("Cannot openin %s", dev);
				strcpy(dev, "/dev/osst0");
				if (!mt_says_tape_exists(dev)) {
					res++;
				} else {
					res = 0;
				}
			}
		}
	}

	log_it("At this point, dev = %s and res = %d", dev, res);

	strcpy(tmp, call_program_and_get_last_line_of_output("\
cdrecord -scanbus 2> /dev/null | tr -s '\t' ' ' | \
grep \"[0-9]*,[0-9]*,[0-9]*\" | grep -v \"[0-9]*) \\*\" | grep -i TAPE | \
awk '{for(i=1; i<NF; i++) { if (index($i, \"GB\")>0) { print $i;};};};'"));

	if (mt_says_tape_exists(dev)) {
		res = 0;
	} else {
		log_it("Turning %s", dev);
		strcpy(tmp, (strrchr(dev, '/') != NULL) ? strrchr(dev, '/') : dev);
		sprintf(dev, "/dev/os%s", tmp);
		log_it("...into %s", dev);
		if (mt_says_tape_exists(dev)) {
			res = 0;
		} else {
			res++;
		}
	}

	siz[0] = '\0';
	log_it("res=%d; dev=%s", res, dev);

	if (res) {
		return (res);
	}

	if (strlen(tmp) < 2) {
		siz[0] = '\0';
		log_it("Warning - size of tape unknown");
		return (0);
	} else {
		strcpy(siz, tmp);
		return (0);
	}
}






int read_EXAT_files_from_tape(struct s_bkpinfo *bkpinfo,
							  long long *ptmp_size, char *tmp_fname,
							  int *pctrl_chr, char *xattr_fname,
							  char *acl_fname)
{
	int res = 0;
	int retval = 0;

// xattr
	res = read_header_block_from_stream(ptmp_size, tmp_fname, pctrl_chr);
	if (*pctrl_chr != BLK_START_EXAT_FILE) {
		wrong_marker(BLK_START_EXAT_FILE, *pctrl_chr);
	}
	if (!strstr(tmp_fname, "xattr")) {
		fatal_error("Wrong order, sunshine.");
	}
	read_file_from_stream_to_file(bkpinfo, xattr_fname, *ptmp_size);
	res = read_header_block_from_stream(ptmp_size, tmp_fname, pctrl_chr);
	if (*pctrl_chr != BLK_STOP_EXAT_FILE) {
		wrong_marker(BLK_STOP_EXAT_FILE, *pctrl_chr);
	}
// acl
	res = read_header_block_from_stream(ptmp_size, tmp_fname, pctrl_chr);
	if (!strstr(tmp_fname, "acl")) {
		fatal_error("Wrong order, sunshine.");
	}
	if (*pctrl_chr != BLK_START_EXAT_FILE) {
		wrong_marker(BLK_START_EXAT_FILE, *pctrl_chr);
	}
	read_file_from_stream_to_file(bkpinfo, acl_fname, *ptmp_size);
	res = read_header_block_from_stream(ptmp_size, tmp_fname, pctrl_chr);
	if (*pctrl_chr != BLK_STOP_EXAT_FILE) {
		wrong_marker(BLK_STOP_EXAT_FILE, *pctrl_chr);
	}
	res = read_header_block_from_stream(ptmp_size, tmp_fname, pctrl_chr);
	if (*pctrl_chr != BLK_STOP_EXTENDED_ATTRIBUTES) {
		wrong_marker(BLK_STOP_EXTENDED_ATTRIBUTES, *pctrl_chr);
	}
// tarball itself
	res = read_header_block_from_stream(ptmp_size, tmp_fname, pctrl_chr);
	log_msg(1, "Got xattr and acl; now looking for afioball");
	return (retval);
}


int write_EXAT_files_to_tape(struct s_bkpinfo *bkpinfo, char *xattr_fname,
							 char *acl_fname)
{
	int res = 0;
// EXATs
	write_header_block_to_stream(length_of_file(xattr_fname), xattr_fname,
								 BLK_START_EXTENDED_ATTRIBUTES);
// xattr
	write_header_block_to_stream(length_of_file(xattr_fname), xattr_fname,
								 BLK_START_EXAT_FILE);
	write_file_to_stream_from_file(bkpinfo, xattr_fname);
	write_header_block_to_stream(-1, xattr_fname, BLK_STOP_EXAT_FILE);
// acl
	write_header_block_to_stream(length_of_file(acl_fname), acl_fname,
								 BLK_START_EXAT_FILE);
	write_file_to_stream_from_file(bkpinfo, acl_fname);
	write_header_block_to_stream(-1, acl_fname, BLK_STOP_EXAT_FILE);
	write_header_block_to_stream(length_of_file(xattr_fname), xattr_fname,
								 BLK_STOP_EXTENDED_ATTRIBUTES);
	return (res);
}




/**
 * Tell the user to insert tape @p tapeno and wait for it to settle (5 seconds).
 * @param tapeno The tape number to insist on.
 * @bug There is currently no way to actually verify that the user has actually
 * inserted the right tape.
 */
void insist_on_this_tape_number(int tapeno)
{
	int i;
	char tmp[MAX_STR_LEN];

	log_it("Insisting on tape #%d", tapeno);
	if (g_current_media_number != tapeno) {
		//      log_it("g_current_media_number = %d", g_current_media_number);
		sprintf(tmp,
				"When the tape drive goes quiet, please insert volume %d in this series.",
				tapeno);
		popup_and_OK(tmp);
		open_evalcall_form("Waiting while the tape drive settles");
	} else {
		open_evalcall_form("Waiting while the tape drive rewinds");
	}

	for (i = 0; i <= 100; i += 2) {
		usleep(100000);
		update_evalcall_form(i);
	}
	close_evalcall_form();
	log_it("I assume user has inserted it. They _say_ they have...");
	g_current_media_number = tapeno;

	//  log_it("g_current_media_number = %d", g_current_media_number);
	log_it("OK, I've finished insisting. On with the revelry.");
}




/**
 * Debugging aid - log the offset we're at on the tape (reading or writing).
 */
void log_tape_pos(void)
{
	/*@ buffers ***************************************************** */


	/*@ end vars *************************************************** */

	log_it("Tape position -- %ld KB (%ld MB)", (long) g_tape_posK,
		   (long) g_tape_posK >> 10);
}




/**
 * Add a file to a collection of recently archived filesets/slices.
 * The type is determined by the filename: if it contains ".afio." it is
 * assumed to be a fileset, otherwise if it contains "slice" it's a slice,
 * otherwise we generate a fatal_error().
 * @param td The current @c bkpinfo->tempdir (file will be placed in <tt>td</tt>/tmpfs/backcatalog/).
 * @param latest_fname The file to place in the collection.
 * @return 0, always.
 * @bug Return value is redundant.
 * @bug The detection won't work for uncompressed afioballs (they end in ".afio", no dot afterwards). // Not true. They end in '.' -Hugo
 */
int maintain_collection_of_recent_archives(char *td, char *latest_fname)
{
	long long final_alleged_writeK, final_projected_certain_writeK,
		final_actually_certain_writeK = 0, cposK, bufsize_K;
	int last, curr, i;
	t_archtype type = other;
	char command[MAX_STR_LEN];
	char tmpdir[MAX_STR_LEN];
	char old_fname[MAX_STR_LEN];
	char *p;
	char suffix[16];

	bufsize_K = (long long) (1024LL * (1 + g_tape_buffer_size_MB));
	sprintf(tmpdir, "%s/tmpfs/backcatalog", td);
	if ((p = strrchr(latest_fname, '.'))) {
		strcpy(suffix, ++p);
	} else {
		suffix[0] = '\0';
	}
	if (strstr(latest_fname, ".afio.") || strstr(latest_fname, ".star.")) {
		type = fileset;
	} else if (strstr(latest_fname, "slice")) {
		type = biggieslice;
	} else {
		log_it("fname = %s", latest_fname);
		fatal_error
			("Unknown type. Internal error in maintain_collection_of_recent_archives()");
	}
	mkdir(tmpdir, 0x700);
	sprintf(command, "cp -f %s %s", latest_fname, tmpdir);
	if (run_program_and_log_output(command, 6)) {
		log_it("Warning - failed to copy %s to backcatalog at %s",
			   latest_fname, tmpdir);
	}
	last = g_tapecatalog->entries - 1;
	if (last <= 0) {
		iamhere("Too early to start deleting from collection.");
		return (0);
	}
	final_alleged_writeK = g_tapecatalog->el[last].tape_posK;
	final_projected_certain_writeK = final_alleged_writeK - bufsize_K;
	for (curr = last; curr >= 0; curr--) {
		cposK = g_tapecatalog->el[curr].tape_posK;
		if (cposK < final_projected_certain_writeK) {
			final_actually_certain_writeK = cposK;
			break;
		}
	}
	if (curr < 0) {
		iamhere
			("Not far enough into tape to start deleting old archives from collection.");
		return (0);
	}
//  log_it( "There are %lld KB (more than %d KB) in my backcatalog", final_alleged_writeK - final_actually_certain_writeK, bufsize_K);

	for (i = curr - 1; i >= 0 && curr - i < 10; i--) {
		sprintf(old_fname, "%s/%s", tmpdir, g_tapecatalog->el[i].fname);
		unlink(old_fname);
	}
	return (0);
}




/**
 * Open the CD stream for input.
 * @param bkpinfo The backup information structure. Passed to openin_tape().
 * @return 0 for success, nonzero for failure.
 * @note Equivalent to openin_tape() for now, but don't count on this behavior.
 */
int openin_cdstream(struct s_bkpinfo *bkpinfo)
{
	return (openin_tape(bkpinfo));
}

/**
 * FIFO used to read/write to the tape device.
 * @bug This seems obsolete now that we call an external @c buffer program. Please look onto this.
 */
char g_tape_fifo[MAX_STR_LEN];



int set_tape_block_size_with_mt(char *tapedev,
								long internal_tape_block_size)
{
	char *tmp;
	int res;

	if (strncmp(tapedev, "/dev/", 5)) {
		log_msg(1,
				"Not using 'mt setblk'. This isn't an actual /dev entry.");
		return (0);
	}
	malloc_string(tmp);
	sprintf(tmp, "mt -f %s setblk %ld", tapedev, internal_tape_block_size);
	res = run_program_and_log_output(tmp, 3);
	paranoid_free(tmp);
	return (res);
}



/**
 * Open the tape device for input.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->media_device
 * - @c bkpinfo->tmpdir
 * @return 0 for success, nonzero for failure.
 * @note This will also work with a cdstream for now, but don't count on this behavior.
 */
int openin_tape(struct s_bkpinfo *bkpinfo)
{
	/*@ buffer ***************************************************** */
	char fname[MAX_STR_LEN];
	char *datablock;
	char tmp[MAX_STR_LEN];
	char old_cwd[MAX_STR_LEN];
	char outfname[MAX_STR_LEN];
	/*@ int ******************************************************* */
	int i;
	int j;
	int res;
	long length, templong;
	size_t k;
	int retval = 0;
	int ctrl_chr;

	/*@ long long ************************************************* */
	long long size;

	/*@ pointers ************************************************** */
	FILE *fout;

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(bkpinfo->media_device);
	if (!(g_tapecatalog = malloc(sizeof(struct s_tapecatalog)))) {
		fatal_error("Cannot alloc mem for tape catalog");
	}
	g_tapecatalog->entries = 0;
	g_tape_posK = 0;
	if (g_tape_stream) {
		log_it("FYI - I won't 'openin' the tape. It's already open.");
		return (0);
	}
	insist_on_this_tape_number(1);
	sprintf(outfname, "%s/tmp/all.tar.gz", bkpinfo->tmpdir);
	make_hole_for_file(outfname);

	set_tape_block_size_with_mt(bkpinfo->media_device,
								bkpinfo->internal_tape_block_size);

//  start_buffer_process( bkpinfo->media_device, g_tape_fifo, FALSE);
	log_it("Opening IN tape");
	if (!
		(g_tape_stream =
		 open_device_via_buffer(bkpinfo->media_device, 'r',
								bkpinfo->internal_tape_block_size))) {
		log_OS_error(g_tape_fifo);
		log_to_screen("Cannot openin stream device");
		return (1);
	}
	log_to_screen("Reading stream");
	log_it("stream device = '%s'", bkpinfo->media_device);
/* skip data disks */
	open_evalcall_form("Skipping data disks on stream");
	log_to_screen("Skipping data disks on stream");
	if (!(fout = fopen(outfname, "w"))) {
		log_OS_error(outfname);
		log_to_screen("Cannot openout datadisk all.tar.gz file");
		return (-1);
	}
	if (!(datablock = (char *) malloc(256 * 1024))) {
		log_to_screen("Unable to malloc 256*1024");
		exit(1);
	}
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 4; j++) {
			for (length = 0, k = 0; length < 256 * 1024; length += k) {
				k = fread(datablock + length, 1, 256 * 1024 - length,
						  g_tape_stream);
			}
			(void) fwrite(datablock, 1, (size_t) length, fout);
			g_tape_posK += length / 1024;
		}
		if (i > 8)				// otherwise, 'buffer' distorts calculations
		{
			templong = ((i - 8) * 4 + j) * 100 / (128 - 8 * 4);
			update_evalcall_form((int) (templong));
		}
	}
	paranoid_fclose(fout);
	paranoid_free(datablock);
/* find initial blocks */
	res = read_header_block_from_stream(&size, fname, &ctrl_chr);
	retval += res;
	if (ctrl_chr != BLK_START_OF_TAPE) {
		wrong_marker(BLK_START_OF_TAPE, ctrl_chr);
	}
	res = read_header_block_from_stream(&size, fname, &ctrl_chr);
	retval += res;
	if (ctrl_chr != BLK_START_OF_BACKUP) {
		wrong_marker(BLK_START_OF_BACKUP, ctrl_chr);
	}
	close_evalcall_form();
	log_it("Saved all.tar.gz to '%s'", outfname);
	(void) getcwd(old_cwd, MAX_STR_LEN);
	chdir(bkpinfo->tmpdir);
	sprintf(tmp, "tar -zxf %s tmp/mondo-restore.cfg 2> /dev/null",
			outfname);
	paranoid_system(tmp);
	paranoid_system("cp -f tmp/mondo-restore.cfg . 2> /dev/null");
	chdir(old_cwd);
	unlink(outfname);
	return (retval);
}


/**
 * Start writing to a CD stream.
 * @param cddev The CD device to openout via cdrecord.
 * @param speed The speed to write at.
 * @return 0 for success, nonzero for failure.
 * @note This should be called only from backup processes.
 */
int openout_cdstream(char *cddev, int speed)
{
	/*@ buffers ***************************************************** */
	char command[MAX_STR_LEN * 2];

	/*@ end vars *************************************************** */

/*  add 'dummy' if testing */
	sprintf(command,
			"cdrecord -eject dev=%s speed=%d fs=24m -waiti - >> %s 2>> %s",
			cddev, speed, MONDO_LOGFILE, MONDO_LOGFILE);
/*  initialise the catalog */
	g_current_media_number = 1;
	if (!(g_tapecatalog = malloc(sizeof(struct s_tapecatalog)))) {
		fatal_error("Cannot alloc mem for tape catalog");
	}
	g_tapecatalog->entries = 0;
/* log stuff */
	log_it("Opening OUT cdstream with the command");
	log_it(command);
/*  log_it("Let's see what happens, shall we?"); */
	g_tape_stream = popen(command, "w");
	if (g_tape_stream) {
		return (0);
	} else {
		log_to_screen("Failed to openout to cdstream (fifo)");
		return (1);
	}
}

/**
 * Start writing to a tape device for the backup.
 * @param tapedev The tape device to open for writing.
 * @return 0 for success, nonzero for failure.
 * @note This should be called ONLY from backup processes. It will OVERWRITE ANY
 * EXISTING DATA on the tape!
 */
int openout_tape(char *tapedev, long internal_tape_block_size)
{
//  char sz_call_to_buffer[MAX_STR_LEN];

	g_current_media_number = 1;
	if (g_tape_stream) {
		log_it("FYI - I won't 'openout' the tape. It's already open.");
		return (0);
	}
	if (!(g_tapecatalog = malloc(sizeof(struct s_tapecatalog)))) {
		fatal_error("Cannot alloc mem for tape catalog");
	}
	g_tapecatalog->entries = 0;
	g_tape_posK = 0;

	set_tape_block_size_with_mt(tapedev, internal_tape_block_size);
	log_it("Opening OUT tape");
	if (!
		(g_tape_stream =
		 open_device_via_buffer(tapedev, 'w', internal_tape_block_size))) {
		log_OS_error(g_tape_fifo);
		log_to_screen("Cannot openin stream device");
		return (1);
	}
	return (0);
}




/**
 * Copy a file from the opened stream (CD or tape) to @p outfile.
 * @param bkpinfo The backup information structure. @c bkpinfo->media_device is the only field used.
 * @param outfile The file to write to.
 * @param size The size of the file in the input stream.
 * @return 0 for success, nonzero for failure.
 */
int
read_file_from_stream_to_file(struct s_bkpinfo *bkpinfo, char *outfile,
							  long long size)
{

	/*@ int ******************************************************** */
	int res;

	/*@ end vars *************************************************** */

	res = read_file_from_stream_FULL(bkpinfo, outfile, NULL, size);

	return (res);
}



/**
 * Copy a file from the currently opened stream (CD or tape) to the stream indicated
 * by @p fout.
 * @param bkpinfo The backup information structure. @c bkpinfo->media_size is the only field used.
 * @param fout The stream to write the file to.
 * @param size The size of the file in bytes.
 * @return 0 for success, nonzero for failure.
 */
int
read_file_from_stream_to_stream(struct s_bkpinfo *bkpinfo, FILE * fout,
								long long size)
{

	/*@ int ******************************************************** */
	int res;

	/*@ end vars *************************************************** */

	res = read_file_from_stream_FULL(bkpinfo, NULL, fout, size);
/*  fflush(g_tape_stream);
  fflush(fout);*/
	return (res);
}



/**
 * Copy a file from the currently opened stream (CD or tape) to either a
 * @c FILE pointer indicating a currently-opened file, or a filename indicating
 * a new file to create. This is the backbone function that read_file_from_stream_to_file()
 * and read_file_from_stream_to_stream() are wrappers for.
 * @param bkpinfo The backup information structure. @c bkpinfo->media_size is the only field used.
 * @param outfname If non-NULL, write to this file.
 * @param foutstream If non-NULL, write to this stream.
 * @param orig_size The original length of the file in bytes.
 * @return 0 for success, nonzero for failure.
 * @note Only @b one of @p outfname or @p foutstream may be non-NULL.
 */
int
read_file_from_stream_FULL(struct s_bkpinfo *bkpinfo, char *outfname,
						   FILE * foutstream, long long orig_size)
{
	/*@ buffers ***************************************************** */
	char *tmp;
	char *datablock;
	char *temp_fname;
	char *temp_cksum;
	char *actual_cksum;
//  char *pA, *pB;

	/*@ int ********************************************************* */
	int retval = 0;
#ifdef EXTRA_TAPE_CHECKSUMS
	int i, ch;
#endif
	int noof_blocks;
	int ctrl_chr;
	int res;
	/*@ pointers **************************************************** */
	FILE *fout;

	/*@ long    ***************************************************** */
	long bytes_to_write = 0 /*,i */ ;
//  long bytes_successfully_read_in_this_time = 0;

	/*@ long long *************************************************** */
	long long temp_size, size;
	long bytes_read, bytes_to_read;
	long long total_read_from_tape_for_this_file = 0;
	long long where_I_was_before_tape_change = 0;
	/*@ unsigned int ************************************************ */
	/*  unsigned int ch; */
	unsigned int crc16;
	unsigned int crctt;

	bool had_to_resync = FALSE;

	/*@ init  ******************************************************* */
	malloc_string(tmp);
	malloc_string(temp_fname);
	malloc_string(temp_cksum);
	malloc_string(actual_cksum);
	datablock = malloc(TAPE_BLOCK_SIZE);
	crc16 = 0;
	crctt = 0;
	size = orig_size;

	/*@ end vars *************************************************** */

	res = read_header_block_from_stream(&temp_size, temp_fname, &ctrl_chr);
	if (orig_size != temp_size && orig_size != -1) {
		sprintf(tmp,
				"output file's size should be %ld K but is apparently %ld K",
				(long) size >> 10, (long) temp_size >> 10);
		log_to_screen(tmp);
	}
	if (ctrl_chr != BLK_START_FILE) {
		wrong_marker(BLK_START_FILE, ctrl_chr);
		return (1);
	}
	sprintf(tmp, "Reading file from tape; writing to '%s'; %ld KB",
			outfname, (long) size >> 10);

	if (foutstream) {
		fout = foutstream;
	} else {
		fout = fopen(outfname, "w");
	}
	if (!fout) {
		log_OS_error(outfname);
		log_to_screen("Cannot openout file");
		return (1);
	}
	total_read_from_tape_for_this_file = 0;
	for (noof_blocks = 0; size > 0;
		 noof_blocks++, size -=
		 bytes_to_write, total_read_from_tape_for_this_file +=
		 bytes_read) {
		bytes_to_write =
			(size < TAPE_BLOCK_SIZE) ? (long) size : TAPE_BLOCK_SIZE;
		bytes_to_read = TAPE_BLOCK_SIZE;
		bytes_read = fread(datablock, 1, bytes_to_read, g_tape_stream);
		while (bytes_read < bytes_to_read) {	// next tape!
//    crctt=crc16=0;
			where_I_was_before_tape_change = size;
			log_msg(4, "where_I_was_... = %lld",
					where_I_was_before_tape_change);
			start_to_read_from_next_tape(bkpinfo);
			log_msg(4, "Started reading from next tape.");
			skip_incoming_files_until_we_find_this_one(temp_fname);
			log_msg(4, "Skipped irrelevant files OK.");
			for (size = orig_size; size > where_I_was_before_tape_change;
				 size -= bytes_to_write) {
				bytes_read =
					fread(datablock, 1, bytes_to_read, g_tape_stream);
			}
			log_msg(4, "'size' is now %lld (should be %lld)", size,
					where_I_was_before_tape_change);
			log_to_screen("Successfully re-sync'd tape");
			had_to_resync = TRUE;
			bytes_read = fread(datablock, 1, bytes_to_read, g_tape_stream);
		}

		(void) fwrite(datablock, 1, (size_t) bytes_to_write, fout);	// for blocking reasons, bytes_successfully_read_in isn't necessarily the same as bytes_to_write

#ifdef EXTRA_TAPE_CHECKSUMS
		for (i = 0; i < (int) bytes_to_write; i++) {
			ch = datablock[i];
			crc16 = updcrcr(crc16, (unsigned) ch);
			crctt = updcrc(crctt, (unsigned) ch);
		}
#endif
	}
	log_msg(6, "Total read from tape for this file = %lld",
			total_read_from_tape_for_this_file);
	log_msg(6, ".......................... Should be %lld", orig_size);
	g_tape_posK += total_read_from_tape_for_this_file / 1024;
	sprintf(actual_cksum, "%04x%04x", crc16, crctt);
	if (foutstream) {			/*log_it("Finished writing to foutstream"); */
	} else {
		paranoid_fclose(fout);
	}
	res = read_header_block_from_stream(&temp_size, temp_cksum, &ctrl_chr);
	if (ctrl_chr != BLK_STOP_FILE) {
		wrong_marker(BLK_STOP_FILE, ctrl_chr);
//      fatal_error("Bad marker"); // return(1);
	}
	if (strcmp(temp_cksum, actual_cksum)) {
		sprintf(tmp, "actual cksum=%s; recorded cksum=%s", actual_cksum,
				temp_cksum);
		log_to_screen(tmp);
		sprintf(tmp, "%s (%ld K) is corrupt on tape", temp_fname,
				(long) orig_size >> 10);
		log_to_screen(tmp);
		retval++;
	} else {
		sprintf(tmp, "%s is GOOD on tape", temp_fname);
		/*      log_it(tmp); */
	}
	paranoid_free(datablock);
	paranoid_free(tmp);
	paranoid_free(temp_fname);
	paranoid_free(temp_cksum);
	paranoid_free(actual_cksum);
	return (retval);
}



/**
 * Read a header block from the currently opened stream (CD or tape).
 * This block indicates the length of the following file (if it's file-related)
 * the filename (if it's file-related), and the block type.
 * @param plen Where to put the length of the file. Valid only for file-related header blocks.
 * @param filename Where to put the name of the file. Valid only for file-related header blocks.
 * @param pcontrol_char Where to put the type of block (e.g. start-file, end-file, start-tape, ...)
 * @return 0 for success, nonzero for failure.
 * @note If you read a marker (@p pcontrol_char) you're not expecting, you can call wrong_marker().
 */
int
read_header_block_from_stream(long long *plen, char *filename,
							  int *pcontrol_char)
{

	/*@ buffers ***************************************************** */
	char *tempblock;

	/*@ int ********************************************************* */
	int i, retval;

	/*@ end vars *************************************************** */

	tempblock = (char *) malloc((size_t) TAPE_BLOCK_SIZE);

	for (i = 0; i < (int) TAPE_BLOCK_SIZE; i++) {
		tempblock[i] = 0;
	}
	while (!(*pcontrol_char = tempblock[7000])) {
		g_tape_posK +=
			fread(tempblock, 1, (size_t) TAPE_BLOCK_SIZE,
				  g_tape_stream) / 1024;
	}
/*  memcpy((char*)plength_of_incoming_file,(char*)tempblock+7001,sizeof(long long)); */
/*  for(*plen=0,i=7;i>=0;i--) {*plen<<=8; *plen |= tempblock[7001+i];} */
	memcpy((char *) plen, tempblock + 7001, sizeof(long long));
	if (strcmp(tempblock + 6000 + *pcontrol_char, "Mondolicious, baby")) {
		log_it("Bad header block at %ld K", (long) g_tape_posK);
	}
	strcpy(filename, tempblock + 1000);
/*  strcpy(cksum,tempblock+5555);*/
/*  log_it( "%s  (reading) fname=%s, filesize=%ld K",
	   marker_to_string (*pcontrol_char), filename,
	   (long) ((*plen) >> 10));
*/
	if (*pcontrol_char == BLK_ABORTED_BACKUP) {
		log_to_screen("I can't verify an aborted backup.");
		retval = 1;
	} else {
		retval = 0;
	}
	for (i = 1000; i < 1020; i++) {
		if (tempblock[i] < 32 || tempblock[i] > 126) {
			tempblock[i] = ' ';
		}
	}
	tempblock[i] = '\0';
	log_msg(6, "%s (fname=%s, size=%ld K)",
			marker_to_string(*pcontrol_char), tempblock + 1000,
			(long) (*plen) >> 10);
	paranoid_free(tempblock);
	return (retval);
}



/**
 * Add specified file/slice to the internal catalog of all archives written.
 * This lets us restart on a new CD/tape/whatever if it runs out of room. We just
 * write the last [buffer size] MB from the catalog to the new tape, so we know
 * we have @e all archives on some CD/tape/whatever.
 * @param type The type of file we're cataloging (afioball, slice, something else)
 * @param number The fileset number or biggiefile number.
 * @param aux The slice number if it's a biggiefile, or any other additional info.
 * @param fn The original full pathname of the file we're recording.
 * @return The index of the record we just added.
 */
int register_in_tape_catalog(t_archtype type, int number, long aux,
							 char *fn)
{
	int last;
	char fname[MAX_TAPECAT_FNAME_LEN];
	char *p;

	p = strrchr(fn, '/');
	if (p) {
		p++;
	} else {
		p = fn;
	}
	strncpy(fname, p, MAX_TAPECAT_FNAME_LEN);
	fname[MAX_TAPECAT_FNAME_LEN] = '\0';
	last = g_tapecatalog->entries;
	if (last >= MAX_TAPECATALOG_ENTRIES) {
		log_it
			("Warning - can't log #%d in tape catalog - too many entries already",
			 number);
		return (-1);
	}
	g_tapecatalog->el[last].type = type;
	g_tapecatalog->el[last].number = number;
	g_tapecatalog->el[last].aux = aux;
	g_tapecatalog->el[last].tape_posK = g_tape_posK;
	strcpy(g_tapecatalog->el[last].fname, fname);
	g_tapecatalog->entries++;
	return (last);				// returns the index of the record we've jsut added
}




/**
 * Decide whether we should start a new tape. This is TRUE if we've run out of tape
 * (got SIGPIPE) or look like we will.
 * @param mediasize The size of the tape in megabytes.
 * @param length_of_incoming_file The length of the file we're about to write, in bytes.
 * @bug This seems like it'll only work for media_size != autodetect, but Mondo only allows
 * autodetecting the size. Huh?
 */
bool
should_we_write_to_next_tape(long mediasize,
							 long long length_of_incoming_file)
{
	/*@ bool's ***************************************************** */
	bool we_need_a_new_tape = FALSE;

	/*@ end vars *************************************************** */

	if (mediasize == 0) {
		return (FALSE);
	}
	if (mediasize > 0 && (g_tape_posK >> 10 >= mediasize)) {
		log_it("mediasize = %ld", mediasize);
		we_need_a_new_tape = TRUE;
		log_to_screen("Should have started a new tape/CD already");
	}
	if ((g_tape_posK + length_of_incoming_file / 1024) >> 10 >=
		mediasize - (SLICE_SIZE * 4 / 1024)) {
		log_it("g_tape_posK = %ld\nmediasize = %ld\n", g_tape_posK,
			   mediasize);
		we_need_a_new_tape = TRUE;
	}
	return (we_need_a_new_tape);
}


/**
 * Seek through the stream until we find a header block where the NAME field matches
 * @p the_file_I_was_reading. This is useful if you've just started reading from
 * a new tape and want to find the file you were reading when the tape ended.
 * @param the_file_I_was_reading File name to look for.
 * @return 0 for success, nonzero for failure.
 */
int skip_incoming_files_until_we_find_this_one(char
											   *the_file_I_was_reading)
{
	char *pA;
	char *pB;
	int res;
	int ctrl_chr;
	char *temp_fname;
	char *datablock;
	long long temp_size, size;
	long bytes_to_write;

	datablock = malloc(TAPE_BLOCK_SIZE);
	malloc_string(temp_fname);
	pB = strrchr(the_file_I_was_reading, '/');
	if (pB) {
		pB++;
	} else {
		pB = the_file_I_was_reading;
	}
	log_msg(1, "skip_incoming_..(%s)", pB);
	log_msg(2, "Looking for initial START_AN_AFIO_OR_SLICE");
	ctrl_chr = -1;
	while (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
		res =
			read_header_block_from_stream(&temp_size, temp_fname,
										  &ctrl_chr);
		if (ctrl_chr == BLK_START_AN_AFIO_OR_SLICE) {
			break;
		}
		log_msg(1, "%lld %s %c", temp_size, temp_fname, ctrl_chr);
		wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		log_msg(3, "Still trying to re-sync w/ tape");
	}
	while (ctrl_chr != BLK_START_FILE) {
		res =
			read_header_block_from_stream(&temp_size, temp_fname,
										  &ctrl_chr);
		if (ctrl_chr == BLK_START_FILE) {
			break;
		}
		log_msg(1, "%lld %s %c", temp_size, temp_fname, ctrl_chr);
		wrong_marker(BLK_START_FILE, ctrl_chr);
		log_msg(3, "Still trying to re-sync w/ tape");
	}
	pA = strrchr(temp_fname, '/');
	if (pA) {
		pA++;
	} else {
		pA = temp_fname;
	}
	pB = strrchr(the_file_I_was_reading, '/');
	if (pB) {
		pB++;
	} else {
		pB = the_file_I_was_reading;
	}
	while (strcmp(pA, pB)) {
		log_msg(6, "Skipping %s (it's not %s)", temp_fname,
				the_file_I_was_reading);
		for (size = temp_size; size > 0; size -= bytes_to_write) {
			bytes_to_write =
				(size < TAPE_BLOCK_SIZE) ? (long) size : TAPE_BLOCK_SIZE;
			// FIXME - needs error-checking and -catching
			fread(datablock, 1, (size_t) TAPE_BLOCK_SIZE, g_tape_stream);
		}
		res =
			read_header_block_from_stream(&temp_size, temp_fname,
										  &ctrl_chr);
		if (ctrl_chr != BLK_STOP_FILE) {
			wrong_marker(BLK_STOP_FILE, ctrl_chr);
		}
		res =
			read_header_block_from_stream(&temp_size, temp_fname,
										  &ctrl_chr);
		if (ctrl_chr != BLK_STOP_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_STOP_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		res =
			read_header_block_from_stream(&temp_size, temp_fname,
										  &ctrl_chr);
		if (ctrl_chr != BLK_START_AN_AFIO_OR_SLICE) {
			wrong_marker(BLK_START_AN_AFIO_OR_SLICE, ctrl_chr);
		}
		res =
			read_header_block_from_stream(&temp_size, temp_fname,
										  &ctrl_chr);
		if (ctrl_chr != BLK_START_FILE) {
			wrong_marker(BLK_START_FILE, ctrl_chr);
		}
		pA = strrchr(temp_fname, '/');
		if (pA) {
			pA++;
		} else {
			pA = temp_fname;
		}
		pB = strrchr(the_file_I_was_reading, '/');
		if (pB) {
			pB++;
		} else {
			pB = the_file_I_was_reading;
		}
	}
	log_msg(2, "Reading %s (it matches %s)", temp_fname,
			the_file_I_was_reading);
	paranoid_free(temp_fname);
	paranoid_free(datablock);
	return (0);
}


/**
 * Start to read from the next tape. Assumes the user has already inserted it.
 * @param bkpinfo The backup information structure. @c bkpinfo->media_device is the only field used.
 * @return 0 for success, nonzero for failure.
 */
int start_to_read_from_next_tape(struct s_bkpinfo *bkpinfo)
{
	/*@ int ********************************************************* */
	int res = 0;
	char *sz_msg;
	int ctrlchr;
	long long temp_size;
	malloc_string(sz_msg);
	/*@ end vars *************************************************** */

	paranoid_pclose(g_tape_stream);
	system("sync");
	system("sync");
	system("sync");
	log_it("Next tape requested.");
	insist_on_this_tape_number(g_current_media_number + 1);	// will increment it, too
	log_it("Opening IN the next tape");
	if (!
		(g_tape_stream =
		 open_device_via_buffer(bkpinfo->media_device, 'r',
								bkpinfo->internal_tape_block_size))) {
		log_OS_error(g_tape_fifo);
		log_to_screen("Cannot openin stream device");
		return (1);
	}
	g_tape_posK = 0;
	g_sigpipe = FALSE;
	res += read_header_block_from_stream(&temp_size, sz_msg, &ctrlchr);	/* just in case */
	if (ctrlchr != BLK_START_OF_TAPE) {
		wrong_marker(BLK_START_OF_TAPE, ctrlchr);
	}
	res += read_header_block_from_stream(&temp_size, sz_msg, &ctrlchr);	/* just in case */
	if (ctrlchr != BLK_START_OF_BACKUP) {
		wrong_marker(BLK_START_OF_BACKUP, ctrlchr);
	} else {
		log_msg(3, "Next tape opened OK. Whoopee!");
	}
	paranoid_free(sz_msg);
	return (res);
}



/**
 * Start to write to the next tape. Assume the user has already inserted it.
 * @param bkpinfo The backup information structure. @c bkpinfo->media_device is the only field used.
 * @return 0 for success, nonzero for failure.
 */
int start_to_write_to_next_tape(struct s_bkpinfo *bkpinfo)
{
	int res = 0;
	char command[MAX_STR_LEN * 2];
	paranoid_pclose(g_tape_stream);
	system("sync");
	system("sync");
	system("sync");
	log_it("New tape requested.");
	insist_on_this_tape_number(g_current_media_number + 1);	// will increment g_current_media, too
	if (g_current_media_number > MAX_NOOF_MEDIA) {
		res++;
		log_to_screen("Too many tapes. Man, you need to use nfs!");
	}
	if (bkpinfo->backup_media_type == cdstream) {
		sprintf(command,
				"cdrecord -eject dev=%s speed=%d fs=24m -waiti - >> %s 2>> %s",
				bkpinfo->media_device, bkpinfo->cdrw_speed, MONDO_LOGFILE,
				MONDO_LOGFILE);
		log_it("Opening OUT to next CD with the command");
		log_it(command);
		log_it("Let's see what happens, shall we?");
		g_tape_stream = popen(command, "w");
		if (!g_tape_stream) {
			log_to_screen("Failed to openout to cdstream (fifo)");
			return (1);
		}
	} else {
		log_it("Opening OUT to next tape");
		if (!
			(g_tape_stream =
			 open_device_via_buffer(bkpinfo->media_device, 'w',
									bkpinfo->internal_tape_block_size))) {
			log_OS_error(g_tape_fifo);
			log_to_screen("Cannot openin stream device");
			return (1);
		}
	}
	g_tape_posK = 0;
	g_sigpipe = FALSE;
	res += write_header_block_to_stream(0, "start-of-tape", BLK_START_OF_TAPE);	/* just in case */
	res += write_header_block_to_stream(0, "start-of-backup", BLK_START_OF_BACKUP);	/* just in case */
	return (res);
}




/**
 * Write a bufferfull of the most recent archives to the tape. The
 * rationale behind this is a bit complex. If the previous tape ended
 * suddenly (EOF or otherwise) some archives were probably left in the
 * buffer. That means that Mondo thinks they have been written, but
 * the external binary @c buffer has not actually written them. So to
 * be safe, we start the new tape by writing the last bufferfull from
 * the old one. This insures that all archives will be on at least
 * one tape. Sounds inelegant, but it works.
 * @param bkpinfo The backup information structure. @c bkpinfo->tmpdir is the only field used.
 * @return 0 for success, nonzero for failure.
 */
int write_backcatalog_to_tape(struct s_bkpinfo *bkpinfo)
{
	int i, last, res = 0;
	char *fname;

	log_msg(2, "I am now writing back catalog to tape");
	malloc_string(fname);
	last = g_tapecatalog->entries - 1;
	for (i = 0; i <= last; i++) {
		sprintf(fname, "%s/tmpfs/backcatalog/%s", bkpinfo->tmpdir,
				g_tapecatalog->el[i].fname);
		if (!does_file_exist(fname)) {
			log_msg(6, "Can't write %s - it doesn't exist.", fname);
		} else {
			write_header_block_to_stream(length_of_file(fname),
										 "start-backcatalog-afio-or-slice",
										 BLK_START_AN_AFIO_OR_SLICE);
			log_msg(2, "Writing %s", fname);
			if (write_file_to_stream_from_file(bkpinfo, fname)) {
				res++;
				log_msg(2, "%s failed", fname);
			}
			if (i != last) {
				write_header_block_to_stream(0,
											 "stop-backcatalog-afio-or-slice",
											 BLK_STOP_AN_AFIO_OR_SLICE);
			}
		}
	}
	paranoid_free(fname);
	log_msg(2, "Finished writing back catalog to tape");
	return (res);
}



/**
 * Write all.tar.gz (produced by Mindi) to the first 32M of the first tape.
 * @param fname The path to all.tar.gz.
 * @return 0 for success, nonzero for failure.
 */
int write_data_disks_to_stream(char *fname)
{
	/*@ pointers *************************************************** */
	FILE *fin;
	char tmp[MAX_STR_LEN];

	/*@ long ******************************************************* */
	long m = -1;
	long templong;

	/*@ int ******************************************************** */
	int i, j;

	/*@ buffers **************************************************** */
	char tempblock[256 * 1024];

	/*@ end vars *************************************************** */

	open_evalcall_form("Writing data disks to tape");
	log_to_screen("Writing data disks to tape");
	log_it("Data disks = %s", fname);
	if (!does_file_exist(fname)) {
		sprintf(tmp, "Cannot find %s", fname);
		log_to_screen(tmp);
		return (1);
	}
	if (!(fin = fopen(fname, "r"))) {
		log_OS_error(fname);
		fatal_error("Cannot openin the data disk");
	}
	for (i = 0; i < 32; i++) {	/* 32MB */
		for (j = 0; j < 4; j++) {	/* 256K x 4 = 1MB (1024K) */
			if (!feof(fin)) {
				m = (long) fread(tempblock, 1, 256 * 1024, fin);
			} else {
				m = 0;
			}
			for (; m < 256 * 1024; m++) {
				tempblock[m] = '\0';
			}
			g_tape_posK +=
				fwrite(tempblock, 1, 256 * 1024, g_tape_stream) / 1024;
		}
		if (i > g_tape_buffer_size_MB)	// otherwise, 'buffer' distorts calculations
		{
			templong = ((i - 8) * 4 + j) * 100 / (128 - 8 * 4);
			update_evalcall_form((int) (templong));
		}
	}
	paranoid_fclose(fin);
	close_evalcall_form();
	return (0);
}




/**
 * Copy @p infile to the opened stream (CD or tape).
 * @param bkpinfo The backup information structure. @c bkpinfo->media_size is the only field used.
 * @param infile The file to write to the stream.
 * @return 0 for success, nonzero for failure.
 */
int write_file_to_stream_from_file(struct s_bkpinfo *bkpinfo, char *infile)
{
	/*@ buffers **************************************************** */
	char tmp[MAX_STR_LEN];
	char datablock[TAPE_BLOCK_SIZE];
	char checksum[MAX_STR_LEN];
	char *infile_basename;

	/*@ int ******************************************************** */
	int retval = 0;
	int noof_blocks;

	/*  unsigned int ch; */
	unsigned int crc16;
	unsigned int crctt;

	/*@ pointers *************************************************** */
	FILE *fin;
	char *p;

	/*@ long ******************************************************* */
	long bytes_to_read = 0;
	long i;

	/*@ long long ************************************************** */
	long long filesize;

#ifdef EXTRA_TAPE_CHECKSUMS
	int ch;
#endif

	/*@ initialize ************************************************ */
	crc16 = 0;
	crctt = 0;



	/*@ end vars *************************************************** */

	infile_basename = strrchr(infile, '/');
	if (infile_basename) {
		infile_basename++;
	} else {
		infile_basename = infile;
	}
	filesize = length_of_file(infile);
	if (should_we_write_to_next_tape
		(bkpinfo->media_size[g_current_media_number], filesize)) {
		start_to_write_to_next_tape(bkpinfo);
		write_backcatalog_to_tape(bkpinfo);
	}
	p = strrchr(infile, '/');
	if (!p) {
		p = infile;
	} else {
		p++;
	}
	sprintf(tmp, "Writing file '%s' to tape (%ld KB)", p,
			(long) filesize >> 10);
	log_it(tmp);
	write_header_block_to_stream(filesize, infile_basename,
								 BLK_START_FILE);
//go_here_to_restart_saving_of_file:
	if (!(fin = fopen(infile, "r"))) {
		log_OS_error(infile);
		return (1);
	}
	for (noof_blocks = 0; filesize > 0;
		 noof_blocks++, filesize -= bytes_to_read) {
		if (filesize < TAPE_BLOCK_SIZE) {
			bytes_to_read = (long) filesize;
			for (i = 0; i < TAPE_BLOCK_SIZE; i++) {
				datablock[i] = '\0';
			}
		} else {
			bytes_to_read = TAPE_BLOCK_SIZE;
		}
		(void) fread(datablock, 1, (size_t) bytes_to_read, fin);
		g_tape_posK +=
			fwrite(datablock, 1, /*bytes_to_read */ 
				   (size_t) TAPE_BLOCK_SIZE,
				   g_tape_stream) / 1024;
		if (g_sigpipe) {
			iamhere("Sigpipe occurred recently. I'll start a new tape.");
			fclose(fin);
			g_sigpipe = FALSE;
			start_to_write_to_next_tape(bkpinfo);
			write_backcatalog_to_tape(bkpinfo);	// kinda-sorta recursive :)
			return (0);
		}
#ifdef EXTRA_TAPE_CHECKSUMS
		for (i = 0; i < bytes_to_read; i++) {
			ch = datablock[i];
			crc16 = updcrcr(crc16, (unsigned) ch);
			crctt = updcrc(crctt, (unsigned) ch);
		}
#endif
	}
	paranoid_fclose(fin);
	sprintf(checksum, "%04x%04x", crc16, crctt);
	write_header_block_to_stream(g_current_media_number, checksum,
								 BLK_STOP_FILE);
//  log_it("File '%s' written to tape.", infile);
	return (retval);
}





/**
 * Write a header block to the opened stream (CD or tape).
 * @param length_of_incoming_file The length to store in the header block.
 * Usually matters only if this is a file-related header, in which case it should
 * be the length of the file that will follow.
 * @param filename The filename to store in the header block. Usually matters
 * only if this is a file-related header, in which case this should be the name
 * if the file that will follow.
 * @param control_char The type of header block this is (start-file, end-file, start-tape, ...)
 * @return 0 for success, nonzero for failure.
 */
int
write_header_block_to_stream(long long length_of_incoming_file,
							 char *filename, int control_char)
{
	/*@ buffers **************************************************** */
	char tempblock[TAPE_BLOCK_SIZE];
	char tmp[MAX_STR_LEN];
	char *p;

	/*@ int ******************************************************** */
	int i;

	/*@ long long ************************************************** */
	long long olen;

	/*@ end vars *************************************************** */


	olen = length_of_incoming_file;
	p = strrchr(filename, '/');	/* Make 'em go, "Unnnh!" Oh wait, that was _Master_ P... */
	if (!p) {
		p = filename;
	} else {
		p++;
	}
	if (!g_tape_stream) {
		log_to_screen
			("You're not backing up to tape. Why write a tape header?");
		return (1);
	}
	for (i = 0; i < (int) TAPE_BLOCK_SIZE; i++) {
		tempblock[i] = 0;
	}
	sprintf(tempblock + 6000 + control_char, "Mondolicious, baby");
	tempblock[7000] = control_char;
/*  for(i=0;i<8;i++) {tempblock[7001+i]=olen&0xff; olen>>=8;} */
	memcpy(tempblock + 7001, (char *) &olen, sizeof(long long));
/*  if (length_of_incoming_file) {memcpy(tempblock+7001,(char*)&length_of_incoming_file,sizeof(long long));} */
	strcpy(tempblock + 1000, filename);
/*  strcpy(tempblock+5555,cksum); */
	g_tape_posK +=
		fwrite(tempblock, 1, (size_t) TAPE_BLOCK_SIZE,
			   g_tape_stream) / 1024;
	sprintf(tmp, "%s (fname=%s, size=%ld K)",
			marker_to_string(control_char), p,
			(long) length_of_incoming_file >> 10);
	log_msg(6, tmp);
/*  log_tape_pos(); */
	return (0);
}










/**
 * Log (to screen) an erroneous marker, along with what it should have been.
 * @param should_be What we were expecting.
 * @param it_is What we got.
 */
void wrong_marker(int should_be, int it_is)
{
	/*@ buffer ***************************************************** */
	char tmp[MAX_STR_LEN];


	/*@ end vars *************************************************** */
	sprintf(tmp, "Wrong marker! (Should be %s, ",
			marker_to_string(should_be));
	sprintf(tmp + strlen(tmp), "is actually %s)", marker_to_string(it_is));
	log_to_screen(tmp);
}

/* @} - end of streamGroup */
