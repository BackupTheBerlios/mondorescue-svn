/* libmondo-archive.c
   $Id$

subroutines to handle the archiving of files



07/14
- moved all ACL, xattr stuff here from libmondo-filelist.c

07/12
- when trying to find boot loader, look for /boot's /dev, not just /'s dev

07/10
- better detection of changed files
- added ACL, xattr support for afio

06/17
- backup with partimagehack if NTFS device; dd, if non-NTFS device

06/14
- use mondorescue.iso instead of mindi.iso

04/28
- cleaned up if/then architecture-specific stuff re: CD-R(W)s

04/03
- added star support

03/12
- modified offer_to_write_floppies() to support dual-disk boot/root thing

01/20/2004
- reformat dvd twice if necessary

12/01/2003
- added pause_for_N_seconds(5, "Letting DVD drive settle");

11/23
- added #define DVDRWFORMAT

11/20
- use --no-emul-boot -b isolinux.bin instead of -b mindi-boot.2880.img

10/23
- wipe DVD at start, whether or not disk is DVD-RW; this is
  just a test, to see why dvd+rw-format followed by growiosfs
  locks up the drive
- use the singlethreaded make_afioballs_and_images_OLD()
  instead of the multithreaded make_afioballs_and_images()
  if backing up to tape

10/21
- if backing up to dvd, look for relevant tools;
  abort if missing

10/15
- UI tweaks

10/10
- eject/inject DVD after wiping it

09/27
- pause_and_ask_for_cdr() will now blank DVD if necessary

09/26
- proper reporting of media type in displayed strings
  (i.e. DVD if DVD, CD if CD, etc.)
  
09/25
- add DVD support

09/23
- malloc/free global strings in new subroutines - malloc_libmondo_global_strings()
  and free_libmondo_global_strings() - which are in libmondo-tools.c

09/15
- changed a bunch of char[MAX_STR_LEN]'s to char*; malloc; free;

09/14
- cosmetic bug re: call to 'which dvdrecord'

09/09
- copy `locate isolinux.bin | tail -n1` to CD before calling mkisfso
  if file is missing (in which case, bug in Mindi.pl!)
- reduced noof_threads from 3 to 2
- fixed cosmetic bug in make_slices_and_images()

09/02
- fixed cosmetic bug in verify_data()

05/01 - 08/31
- added partimagehack hooks
- make twice as many archives at once as before 
- fixed syntax error re: system(xxx,FALSE)
- unmount CD-ROM before burning (necessary for RH8/9)
- only ask for new media if sensible
- fixed mondoarchive -Vi multi-CD verify bug (Tom Mortell)
- use single-threaded make_afioballs_and_images() if FreeBSD
- fixed bug on line 476 (Joshua Oreman)
- re-enabled the pause, for people w/ weird CD-ROM drives
- added Joshua Oreman's FreeBSD patches
- don't listen to please_dont_eject_when_restoring
  ...after all, we're not restoring :)

04/01 - 04/30
- cleaned up archive_this_fileset()
- decreased ARCH_THREADS from 3 to 2
- pause_and_ask_for_cd() --- calls retract_CD_tray_and_defeat_autorun()
- call assert() and log_OS_error() in various places
- cleaned up code a bit
- increased ARCH_THREADS from 2 to 3
- misc clean-up (Tom Mortell)

03/15/2003
- fixed problem w/ multi-ISO verify cycle (Tom Mortell)

11/01 - 12/31/2002
- removed references to make_afioballs_and_images_OLD()
- added some error-checking to make_afioballs_in_background
- make_iso_and_go_on() now copies Mondo's autorun file to CD
- set scratchdir dir to 1744 when burning CD
- cleaned up code a bit
- in call_mindi_...(), gracefully handle the user's input
  if they specify the boot loader but not the boot device
- offer to abort if GRUB is boot loader and /dev/md* is
  the boot device (bug in grub-install)
- if boot loader is R then write RAW as bootloader.name
- multithreaded make_afioballs_and_images()
- fixed slice_up_file_etc() for 0-compression users
- line 1198: added call to is_this_file_compressed() to stop
  slice_up_file_etc() from compressing files which are
  already compressed
- other hackery related to the above enhancement
- afio no longer forcibly compresses all files (i.e. I dropped
  the -U following suggestions from users); let's see if
  it works :)

10/01 - 10/31
- mondoarchive (with no parameters) wasn't working
  if user said yes when asked if BurnProof drive; FIXED
- if manual CD tray and writing to ISO's then prompt
  for each & every new CD
- moved a lot of subroutines here
  from mondo-archive.c and mondo-floppies.c

09/01 - 09/30
- if CD not burned OK then don't try to verify
- change 64k to TAPE_BLOCK_SIZE
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_
- orig_vfy_flag_val added to write_iso_and_go_on, to restore bkpinfo->verify_data's
  value if altered by verify_cd_image()

08/01 - 08/31
- use data structure to store the fname, checksum, mods & perms of each bigfile
  ... biggiestruct :)
- bigger tmp[]'s in a few places
- cleaned up the (user-friendly) counting of biggiefiles a bit
- if media_size[N]<=0 then catch it & abort, unless it's tape,
  in which case, allow it
- cleaned up a lot of log_it() calls
- fixed NULL filename-related bug in can_we_fit_these_files_on_media()
- deleted can_we_fit.....() subroutine because it was causing problems
  --- moved its code into the one subroutine which called it
- created [08/01/2002]
*/

/**
 * @file
 * Functions to handle backing up data.
 * This is the main file (at least the longest one) in libmondo.
 */

#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "libmondo-string-EXT.h"
#include "libmondo-stream-EXT.h"
#include "libmondo-devices-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-filelist-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-verify-EXT.h"
#include "libmondo-archive.h"
#include "lib-common-externs.h"
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdarg.h>
#define DVDRWFORMAT 1



/** @def DEFAULT_1722MB_DISK The default 1.722M floppy disk to write images to. */
/** @def BACKUP_1722MB_DISK  The 1.722M floppy disk to try if the default fails. */

#ifdef __FreeBSD__
#define DEFAULT_1722MB_DISK "/dev/fd0.1722"
#define BACKUP_1722MB_DISK "/dev/fd0.1722"
#else
#define DEFAULT_1722MB_DISK "/dev/fd0u1722"
#define BACKUP_1722MB_DISK "/dev/fd0H1722"
#ifndef _SEMUN_H
#define _SEMUN_H

	/**
	 * The semaphore union, provided only in case the user's system doesn't.
	 */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};
#endif
#endif							/* __FreeBSD__ */

/*@unused@*/
//static char cvsid[] = "$Id$";

/* *************************** external global vars ******************/
extern int g_current_media_number;
extern int g_currentY;
extern bool g_text_mode;
extern bool g_exiting;
extern long g_current_progress;
extern FILE *g_tape_stream;
extern long long g_tape_posK;
extern char *g_mondo_home;
extern char *g_tmpfs_mountpt;
extern bool g_cd_recovery;
extern char *g_serial_string;

/**
 * @addtogroup globalGroup
 * @{
 */
/**
 * The current backup media type in use.
 */
t_bkptype g_backup_media_type = none;

/**
 * Incremented by each archival thread when it starts up. After that,
 * this is the number of threads running.
 */
int g_current_thread_no = 0;

/* @} - end of globalGroup */

extern int g_noof_rows;

/* Semaphore-related code */

static int set_semvalue(void);
static void del_semvalue(void);
static int semaphore_p(void);
static int semaphore_v(void);

static int g_sem_id;
static int g_sem_key;




/**
 * Initialize the semaphore.
 * @see del_semvalue
 * @see semaphore_p
 * @see semaphore_v
 * @return 1 for success, 0 for failure.
 */
static int set_semvalue(void)	// initializes semaphore
{
	union semun sem_union;
	sem_union.val = 1;
	if (semctl(g_sem_id, 0, SETVAL, sem_union) == -1) {
		return (0);
	}
	return (1);
}

/**
 * Frees (deletes) the semaphore. Failure is indicated by a log
 * message.
 * @see set_semvalue
 */
static void del_semvalue(void)	// deletes semaphore
{
	union semun sem_union;

	if (semctl(g_sem_id, 0, IPC_RMID, sem_union) == -1) {
		log_msg(3, "Failed to delete semaphore");
	}
}

/**
 * Acquire (increment) the semaphore (change status to P).
 * @return 1 for success, 0 for failure.
 * @see semaphore_v
 */
static int semaphore_p(void)	// changes status to 'P' (waiting)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = -1;			// P()
	sem_b.sem_flg = SEM_UNDO;
	if (semop(g_sem_id, &sem_b, 1) == -1) {
		log_msg(3, "semaphore_p failed");
		return (0);
	}
	return (1);
}

/**
 * Free (decrement) the semaphore (change status to V).
 * @return 1 for success, 0 for failure.
 */
static int semaphore_v(void)	// changes status to 'V' (free)
{
	struct sembuf sem_b;

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;			// V()
	sem_b.sem_flg = SEM_UNDO;
	if (semop(g_sem_id, &sem_b, 1) == -1) {
		log_msg(3, "semaphore_v failed");
		return (0);
	}
	return (1);
}


//------------------------------------------------------


/**
 * Size in megabytes of the buffer afforded to the executable "buffer".
 * This figure is used when we calculate how much data we have probably 'lost'
 * when writing off the end of tape N, so that we can then figure out how much
 * data we must recreate & write to the start of tape N+1.
 */
extern int g_tape_buffer_size_MB;




int
archive_this_fileset_with_star(struct s_bkpinfo *bkpinfo, char *filelist,
							   char *fname, int setno)
{
	int retval = 0;
	unsigned int res = 0;
	int tries = 0;
	char *command;
	char *zipparams;
	char *tmp;
	char *p;

	malloc_string(command);
	malloc_string(zipparams);
	malloc_string(tmp);

	if (!does_file_exist(filelist)) {
		sprintf(tmp, "(archive_this_fileset) - filelist %s does not exist",
				filelist);
		log_to_screen(tmp);
		return (1);
	}

	sprintf(tmp, "echo hi > %s 2> /dev/null", fname);
	if (system(tmp)) {
		fatal_error("Unable to write tarball to scratchdir");
	}

	sprintf(command, "star H=star list=%s -c " STAR_ACL_SZ " file=%s",
			filelist, fname);
	if (bkpinfo->use_lzo) {
		fatal_error("Can't use lzop");
	}
	if (bkpinfo->compression_level > 0) {
		strcat(command, " -bz");
	}
	sprintf(command + strlen(command), " 2>> %s", MONDO_LOGFILE);
	log_msg(4, "command = '%s'", command);

	for (res = 99, tries = 0; tries < 3 && res != 0; tries++) {
		log_msg(5, "command='%s'", command);
		res = system(command);
		strcpy(tmp, last_line_of_file(MONDO_LOGFILE));
		log_msg(1, "res=%d; tmp='%s'", res, tmp);
		if (bkpinfo->use_star && (res == 254 || res == 65024)
			&& strstr(tmp, "star: Processed all possible files")
			&& tries > 0) {
			log_msg(1, "Star returned nonfatal error");
			res = 0;
		}
		if (res) {
			log_OS_error(command);
			p = strstr(command, "-acl ");
			if (p) {
				p[0] = p[1] = p[2] = p[3] = ' ';
				log_msg(1, "new command = '%s'", command);
			} else {
				log_msg(3,
						"Attempt #%d failed. Pausing 3 seconds and retrying...",
						tries + 1);
				sleep(3);
			}
		}
	}
	retval += res;
	if (retval) {
		log_msg(3, "Failed to write set %d", setno);
	} else if (tries > 1) {
		log_msg(3, "Succeeded in writing set %d, on try #%d", setno,
				tries);
	}

	paranoid_free(command);
	paranoid_free(zipparams);
	paranoid_free(tmp);
	return (retval);
}


/**
 * Call @c afio to archive the filelist @c filelist to the file @c fname.
 * 
 * @param bkpinfo The backup information structure. Fields used:
 * - @c compression_level
 * - @c scratchdir (only verifies existence)
 * - @c tmpdir (only verifies existence)
 * - @c zip_exe
 * - @c zip_suffix
 * @param filelist The path to a file containing a list of files to be archived
 * in this fileset.
 * @param fname The output file to archive to.
 * @param setno This fileset number.
 * @return The number of errors encountered (0 for success).
 * @ingroup LLarchiveGroup
 */
int
archive_this_fileset(struct s_bkpinfo *bkpinfo, char *filelist,
					 char *fname, int setno)
{

	/*@ int *************************************************************** */
	int retval = 0;
	int res = 0;
	int i = 0;
	int tries = 0;
	static int free_ramdisk_space = 9999;

	/*@ buffers ************************************************************ */
	char *command;
	char *zipparams;
	char *tmp;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(filelist);
	assert_string_is_neither_NULL_nor_zerolength(fname);

	if (bkpinfo->compression_level > 0 && bkpinfo->use_star) {
		return (archive_this_fileset_with_star
				(bkpinfo, filelist, fname, setno));
	}

	malloc_string(command);
	malloc_string(zipparams);
	malloc_string(tmp);

	if (!does_file_exist(filelist)) {
		sprintf(tmp, "(archive_this_fileset) - filelist %s does not exist",
				filelist);
		log_to_screen(tmp);
		return (1);
	}
	sprintf(tmp, "echo hi > %s 2> /dev/null", fname);
	if (system(tmp)) {
		fatal_error("Unable to write tarball to scratchdir");
	}


	if (bkpinfo->compression_level > 0) {
		sprintf(tmp, "%s/do-not-compress-these", g_mondo_home);
		//       -b %ld, TAPE_BLOCK_SIZE
		sprintf(zipparams, "-Z -P %s -G %d -T 3k", bkpinfo->zip_exe,
				bkpinfo->compression_level);
		if (does_file_exist(tmp)) {
			strcat(zipparams, " -E ");
			strcat(zipparams, tmp);
		} else {
			log_msg(3, "%s not found. Cannot exclude zipfiles, etc.", tmp);
		}
	} else {
		zipparams[0] = '\0';
	}

//  make_hole_for_file(fname);

	if (!does_file_exist(bkpinfo->tmpdir)) {
		log_OS_error("tmpdir not found");
		fatal_error("tmpdir not found");
	}
	if (!does_file_exist(bkpinfo->scratchdir)) {
		log_OS_error("scratchdir not found");
		fatal_error("scratchdir not found");
	}
	sprintf(command, "rm -f %s %s. %s.gz %s.%s", fname, fname, fname,
			fname, bkpinfo->zip_suffix);
	paranoid_system(command);

	sprintf(command, "afio -o -b %ld -M 16m %s %s < %s 2>> %s",
			TAPE_BLOCK_SIZE, zipparams, fname, filelist, MONDO_LOGFILE);

	sprintf(tmp, "echo hi > %s 2> /dev/null", fname);
	if (system(tmp)) {
		fatal_error("Unable to write tarball to scratchdir");
	}

	for (res = 99, tries = 0; tries < 3 && res != 0; tries++) {
		log_msg(5, "command='%s'", command);
		res = system(command);
		if (res) {
			log_OS_error(command);
			log_msg(3,
					"Attempt #%d failed. Pausing 3 seconds and retrying...",
					tries + 1);
			sleep(3);
		}
	}
	retval += res;
	if (retval) {
		log_msg(3, "Failed to write set %d", setno);
	} else if (tries > 1) {
		log_msg(3, "Succeeded in writing set %d, on try #%d", setno,
				tries);
	}

	if (g_tmpfs_mountpt[0] != '\0') {
		i = atoi(call_program_and_get_last_line_of_output
				 ("df -m -P | grep dev/shm | grep -v none | tr -s ' ' '\t' | cut -f4"));
		if (i > 0) {
			if (free_ramdisk_space > i) {
				free_ramdisk_space = i;
				log_msg(2, "min(free_ramdisk_space) is now %d",
						free_ramdisk_space);
				if (free_ramdisk_space < 10) {
					fatal_error
						("Please increase PPCFG_RAMDISK_SIZE in my-stuff.h to increase size of ramdisk ");
				}
			}
		}
	}
	paranoid_free(command);
	paranoid_free(zipparams);
	paranoid_free(tmp);
	return (retval);
}






/**
 * Wrapper function for all the backup commands.
 * Calls these other functions: @c prepare_filelist(),
 * @c call_filelist_chopper(), @c copy_mondo_and_mindi_stuff_to_scratchdir(),
 * @c call_mindi_to_supply_boot_disks(), @c do_that_initial_phase(),
 * @c make_those_afios_phase(), @c make_those_slices_phase(), and
 * @c do_that_final_phase(). If anything fails before @c do_that_initial_phase(),
 * @c fatal_error is called with a suitable message.
 * @param bkpinfo The backup information structure. Uses most fields.
 * @return The number of non-fatal errors encountered (0 for success).
 * @ingroup archiveGroup
 */
int backup_data(struct s_bkpinfo *bkpinfo)
{
	int retval = 0, res = 0;
	char *tmp;

	assert(bkpinfo != NULL);
	set_g_cdrom_and_g_dvd_to_bkpinfo_value(bkpinfo);
	malloc_string(tmp);
	if (bkpinfo->backup_media_type == dvd) {
#ifdef DVDRWFORMAT
		if (!find_home_of_exe("dvd+rw-format")) {
			fatal_error
				("Cannot find dvd+rw-format. Please install it or fix your PATH.");
		}
#endif
		if (!find_home_of_exe("growisofs")) {
			fatal_error
				("Cannot find growisofs. Please install it or fix your PATH.");
		}
	}

	if ((res = prepare_filelist(bkpinfo))) {	/* generate scratchdir/filelist.full */
		fatal_error("Failed to generate filelist catalog");
	}
	if (call_filelist_chopper(bkpinfo)) {
		fatal_error("Failed to run filelist chopper");
	}

/*
      sprintf(tmp, "wc -l %s/archives/filelist.full > %s/archives/filelist.count",bkpinfo->scratchdir, bkpinfo->scratchdir);
      if (run_program_and_log_output(tmp, 2))
        { fatal_error("Failed to count filelist.full"); }
*/
	sprintf(tmp, "gzip -9 %s/archives/filelist.full", bkpinfo->scratchdir);
	if (run_program_and_log_output(tmp, 2)) {
		fatal_error("Failed to gzip filelist.full");
	}
	sprintf(tmp, "cp -f %s/archives/*list*.gz %s", bkpinfo->scratchdir,
			bkpinfo->tmpdir);
	if (run_program_and_log_output(tmp, 2)) {
		fatal_error("Failed to copy to tmpdir");
	}

	copy_mondo_and_mindi_stuff_to_scratchdir(bkpinfo);	// payload, too, if it exists
#if __FreeBSD__ == 5
	strcpy(bkpinfo->kernel_path, "/boot/kernel/kernel");
#elif __FreeBSD__ == 4
	strcpy(bkpinfo->kernel_path, "/kernel");
#elif linux
	if (figure_out_kernel_path_interactively_if_necessary
		(bkpinfo->kernel_path)) {
		fatal_error
			("Kernel not found. Please specify manually with the '-k' switch.");
	}
#else
#error "I don't know about this system!"
#endif
	if ((res = call_mindi_to_supply_boot_disks(bkpinfo))) {
		fatal_error("Failed to generate boot+data disks");
	}
	retval += do_that_initial_phase(bkpinfo);	// prepare
	sprintf(tmp, "rm -f %s/images/*.iso", bkpinfo->scratchdir);
	run_program_and_log_output(tmp, 1);
	retval += make_those_afios_phase(bkpinfo);	// backup regular files
	retval += make_those_slices_phase(bkpinfo);	// backup BIG files
	retval += do_that_final_phase(bkpinfo);	// clean up
	log_msg(1, "Creation of archives... complete.");
	if (bkpinfo->verify_data) {
		sleep(2);
	}
	paranoid_free(tmp);
	return (retval);
}




/**
 * Call Mindi to generate boot and data disks.
 * @note This binds correctly to the new Perl version of mindi.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c boot_loader
 * - @c boot_device
 * - @c compression_level
 * - @c differential
 * - @c exclude_paths
 * - @c image_devs
 * - @c kernel_path
 * - @c make_cd_use_lilo
 * - @c media_device
 * - @c media_size
 * - @c nonbootable_backup
 * - @c scratchdir
 * - @c tmpdir
 * - @c use_lzo
 *
 * @return The number of errors encountered (0 for success)
 * @bug The code to automagically determine the boot drive
 * is messy and system-dependent. In particular, it breaks
 * for Linux RAID and LVM users.
 * @ingroup MLarchiveGroup
 */
int call_mindi_to_supply_boot_disks(struct s_bkpinfo *bkpinfo)
{
	/*@ buffer ************************************************************ */
	char *tmp;
	char *scratchdir;
	char *command;
	char *use_lzo_sz;
	char *use_comp_sz;
	char *use_star_sz;
	char *bootldr_str;
	char *tape_device;
	char *last_filelist_number;
	char *broken_bios_sz;
	char *cd_recovery_sz;
	char *tape_size_sz;
	char *devs_to_exclude;
	char *use_lilo_sz;
	char *value;
	char *bootdev;



	/*@ char ************************************************************** */
	char ch = '\0';

	/*@ long     ********************************************************** */
	long lines_in_filelist = 0;

	/*@ int     ************************************************************* */
	int res = 0;
	long estimated_total_noof_slices = 0;

	assert(bkpinfo != NULL);
	command = malloc(1200);
	malloc_string(tmp);
	malloc_string(scratchdir);
	malloc_string(use_lzo_sz);
	malloc_string(use_star_sz);
	malloc_string(use_comp_sz);
	malloc_string(bootldr_str);
	malloc_string(tape_device);
	malloc_string(last_filelist_number);
	malloc_string(broken_bios_sz);
	malloc_string(cd_recovery_sz);
	malloc_string(tape_size_sz);
	malloc_string(devs_to_exclude);
	malloc_string(use_lilo_sz);	/* BCO: shared between LILO/ELILO */
	malloc_string(value);
	malloc_string(bootdev);

	strcpy(scratchdir, bkpinfo->scratchdir);
	sprintf(tmp,
			"echo '%s' | tr -s ' ' '\n' | grep -x '/dev/.*' | tr -s '\n' ' ' | awk '{print $0\"\\n\";}'",
			bkpinfo->exclude_paths);
	strcpy(devs_to_exclude, call_program_and_get_last_line_of_output(tmp));
	sprintf(tmp, "devs_to_exclude = '%s'", devs_to_exclude);
	log_msg(2, tmp);
	mvaddstr_and_log_it(g_currentY, 0,
						"Calling MINDI to create boot+data disks");
	sprintf(tmp, "%s/filelist.full", bkpinfo->tmpdir);
	if (!does_file_exist(tmp)) {
		sprintf(tmp, "%s/tmpfs/filelist.full", bkpinfo->tmpdir);
		if (!does_file_exist(tmp)) {
			fatal_error
				("Cannot find filelist.full, so I cannot count its lines");
		}
	}
	lines_in_filelist = count_lines_in_file(tmp);
	sprintf(tmp, "%s/LAST-FILELIST-NUMBER", bkpinfo->tmpdir);
	strcpy(last_filelist_number, last_line_of_file(tmp));
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		sprintf(tape_size_sz, "%ld", bkpinfo->media_size[1]);
		strcpy(tape_device, bkpinfo->media_device);
	} else {
		tape_size_sz[0] = '\0';
		tape_device[0] = '\0';
	}
	if (bkpinfo->use_lzo) {
		strcpy(use_lzo_sz, "yes");
	} else {
		strcpy(use_lzo_sz, "no");
	}
	if (bkpinfo->use_star) {
		strcpy(use_star_sz, "yes");
	} else {
		strcpy(use_star_sz, "no");
	}

	if (bkpinfo->compression_level > 0) {
		strcpy(use_comp_sz, "yes");
	} else {
		strcpy(use_comp_sz, "no");
	}

	strcpy(broken_bios_sz, "yes");	/* assume so */
	if (g_cd_recovery) {
		strcpy(cd_recovery_sz, "yes");
	} else {
		strcpy(cd_recovery_sz, "no");
	}
	if (bkpinfo->make_cd_use_lilo) {
		strcpy(use_lilo_sz, "yes");
	} else {
		strcpy(use_lilo_sz, "no");
	}

	if (!bkpinfo->nonbootable_backup
		&& (bkpinfo->boot_loader == '\0'
			|| bkpinfo->boot_device[0] == '\0')) {

#ifdef __FreeBSD__
		strcpy(bootdev, call_program_and_get_last_line_of_output
			   ("mount | grep ' /boot ' | head -1 | cut -d' ' -f1 | sed 's/\\([0-9]\\).*/\\1/'"));
		if (!bootdev[0]) {
			strcpy(bootdev, call_program_and_get_last_line_of_output
				   ("mount | grep ' / ' | head -1 | cut -d' ' -f1 | sed 's/\\([0-9]\\).*/\\1/'"));
		}
#else
		strcpy(bootdev, call_program_and_get_last_line_of_output
			   ("mount | grep ' /boot ' | head -1 | cut -d' ' -f1 | sed 's/[0-9].*//'"));
		if (strstr(bootdev, "/dev/cciss/")) {
			strcpy(bootdev, call_program_and_get_last_line_of_output
				   ("mount | grep ' /boot ' | head -1 | cut -d' ' -f1 | cut -dp -f1"));
		}
		if (!bootdev[0]) {
			strcpy(bootdev, call_program_and_get_last_line_of_output
				   ("mount | grep ' / ' | head -1 | cut -d' ' -f1 | sed 's/[0-9].*//'"));
			if (strstr(bootdev, "/dev/cciss/")) {
				strcpy(bootdev, call_program_and_get_last_line_of_output
					   ("mount | grep ' / ' | head -1 | cut -d' ' -f1 | cut -dp -f1"));
			}
		}
#endif
		if (bootdev[0])
			ch = which_boot_loader(bootdev);
		else
			ch = 'U';
		if (bkpinfo->boot_loader != '\0') {
			sprintf(tmp, "User specified boot loader. It is '%c'.",
					bkpinfo->boot_loader);
			log_msg(2, tmp);
		} else {
			bkpinfo->boot_loader = ch;
		}
		if (bkpinfo->boot_device[0] != '\0') {
			sprintf(tmp, "User specified boot device. It is '%s'.",
					bkpinfo->boot_device);
			log_msg(2, tmp);
		} else {
			strcpy(bkpinfo->boot_device, bootdev);
		}
	}

	if (
#ifdef __FreeBSD__
		   bkpinfo->boot_loader != 'B' && bkpinfo->boot_loader != 'D' &&
#endif
#ifdef __IA64__
		   bkpinfo->boot_loader != 'E' &&
#endif
		   bkpinfo->boot_loader != 'L' && bkpinfo->boot_loader != 'G'
		   && bkpinfo->boot_loader != 'R'
		   && !bkpinfo->nonbootable_backup) {
		fatal_error
			("Please specify your boot loader and device, e.g. -l GRUB -f /dev/hda. Type 'man mondoarchive' to read the manual.");
	}
	if (bkpinfo->boot_loader == 'L') {
		strcpy(bootldr_str, "LILO");
		if (!does_file_exist("/etc/lilo.conf")) {
			fatal_error
				("The de facto standard location for your boot loader's config file is /etc/lilo.conf but I cannot find it there. What is wrong with your Linux distribution?");
		}
	} else if (bkpinfo->boot_loader == 'G') {
		strcpy(bootldr_str, "GRUB");
		if (!does_file_exist("/etc/grub.conf")
			&& does_file_exist("/boot/grub/grub.conf")) {
			run_program_and_log_output
				("ln -sf /boot/grub/grub.conf /etc/grub.conf", 5);
		}
		/* Detect Debian's grub config file */
		else if (!does_file_exist("/etc/grub.conf")
				 && does_file_exist("/boot/grub/menu.lst")) {
			run_program_and_log_output
				("ln -s /boot/grub/menu.lst /etc/grub.conf", 5);
		}
		if (!does_file_exist("/etc/grub.conf")) {
			fatal_error
				("The de facto standard location for your boot loader's config file is /etc/grub.conf but I cannot find it there. What is wrong with your Linux distribution? Try 'ln -s /boot/grub/menu.lst /etc/grub.conf'...");
		}
	} else if (bkpinfo->boot_loader == 'E') {
		strcpy(bootldr_str, "ELILO");
		/* BCO: fix it for SuSE, Debian, Mandrake, ... */
		if (!does_file_exist("/etc/elilo.conf")
			&& does_file_exist("/boot/efi/efi/redhat/elilo.conf")) {
			run_program_and_log_output
				("ln -sf /boot/efi/efi/redhat/elilo.conf /etc/elilo.conf",
				 5);
		}
		if (!does_file_exist("/etc/elilo.conf")) {
			fatal_error
				("The de facto mondo standard location for your boot loader's config file is /etc/elilo.conf but I cannot find it there. What is wrong with your Linux distribution? Try finding it under /boot/efi and do 'ln -s /boot/efi/..../elilo.conf /etc/elilo.conf'");
		}
	} else if (bkpinfo->boot_loader == 'R') {
		strcpy(bootldr_str, "RAW");
	}
#ifdef __FreeBSD__
	else if (bkpinfo->boot_loader == 'D') {
		strcpy(bootldr_str, "DD");
	}

	else if (bkpinfo->boot_loader == 'B') {
		strcpy(bootldr_str, "BOOT0");
	}
#endif
	else {
		strcpy(bootldr_str, "unknown");
	}
	sprintf(tmp, "Your boot loader is %s and it boots from %s",
			bootldr_str, bkpinfo->boot_device);
	log_to_screen(tmp);
	sprintf(tmp, "%s/BOOTLOADER.DEVICE", bkpinfo->tmpdir);
	if (write_one_liner_data_file(tmp, bkpinfo->boot_device)) {
		log_msg(1, "%ld: Unable to write one-liner boot device", __LINE__);
	}
	switch (bkpinfo->backup_media_type) {
	case cdr:
		strcpy(value, "cdr");
		break;
	case cdrw:
		strcpy(value, "cdrw");
		break;
	case cdstream:
		strcpy(value, "cdstream");
		break;
	case tape:
		strcpy(value, "tape");
		break;
	case udev:
		strcpy(value, "udev");
		break;
	case iso:
		strcpy(value, "iso");
		break;
	case nfs:
		strcpy(value, "nfs");
		break;
	case dvd:
		strcpy(value, "dvd");
		break;
	default:
		fatal_error("Unknown backup_media_type");
	}
	sprintf(tmp, "%s/BACKUP-MEDIA-TYPE", bkpinfo->tmpdir);
	if (write_one_liner_data_file(tmp, value)) {
		res++;
		log_msg(1, "%ld: Unable to write one-liner backup-media-type",
				__LINE__);
	}
	log_to_screen(bkpinfo->tmpdir);
	sprintf(tmp, "%s/BOOTLOADER.NAME", bkpinfo->tmpdir);
	if (write_one_liner_data_file(tmp, bootldr_str)) {
		res++;
		log_msg(1, "%ld: Unable to write one-liner bootloader.name",
				__LINE__);
	}
	sprintf(tmp, "%s/DIFFERENTIAL", bkpinfo->tmpdir);
	if (bkpinfo->differential) {
		res += write_one_liner_data_file(tmp, "1");
	} else {
		res += write_one_liner_data_file(tmp, "0");
	}

	estimated_total_noof_slices =
		size_of_all_biggiefiles_K(bkpinfo) / bkpinfo->optimal_set_size + 1;
/* add nfs stuff here? */
	sprintf(command, "mkdir -p %s/images", bkpinfo->scratchdir);
	if (system(command)) {
		res++;
		log_OS_error("Unable to make images directory");
	}
	sprintf(command, "mkdir -p %s%s", bkpinfo->scratchdir, MNT_FLOPPY);
	if (system(command)) {
		res++;
		log_OS_error("Unable to make mnt floppy directory");
	}
	sprintf(tmp, "BTW, I'm telling Mindi your kernel is '%s'",
			bkpinfo->kernel_path);

	log_msg(1, "lines_in_filelist = %ld", lines_in_filelist);

	sprintf(command,
/*	   "mindi --custom 2=%s 3=%s/images 4=\"%s\" 5=\"%s\" \
6=\"%s\" 7=%ld 8=\"%s\" 9=\"%s\" 10=\"%s\" \
11=\"%s\" 12=%s 13=%ld 14=\"%s\" 15=\"%s\" 16=\"%s\" 17=\"%s\" 18=%ld 19=%d",*/
			"mindi --custom %s %s/images '%s' '%s' \
'%s' %ld '%s' '%s' '%s' \
'%s' %s %ld '%s' '%s' '%s' '%s' %ld %d", bkpinfo->tmpdir,	// parameter #2
			bkpinfo->scratchdir,	// parameter #3
			bkpinfo->kernel_path,	// parameter #4
			tape_device,		// parameter #5
			tape_size_sz,		// parameter #6
			lines_in_filelist,	// parameter #7 (INT)
			use_lzo_sz,			// parameter #8
			cd_recovery_sz,		// parameter #9
			bkpinfo->image_devs,	// parameter #10
			broken_bios_sz,		// parameter #11
			last_filelist_number,	// parameter #12 (STRING)
			estimated_total_noof_slices,	// parameter #13 (INT)
			devs_to_exclude,	// parameter #14
			use_comp_sz,		// parameter #15
			use_lilo_sz,		// parameter #16
			use_star_sz,		// parameter #17
			bkpinfo->internal_tape_block_size,	// parameter #18 (LONG)
			bkpinfo->differential);	// parameter #19 (INT)

// Watch it! This next line adds a parameter...
	if (bkpinfo->nonbootable_backup) {
		strcat(command, " NONBOOTABLE");
	}
	log_msg(2, command);

//  popup_and_OK("Pausing");

	res =
		run_program_and_log_to_screen(command,
									  "Generating boot+data disks");
	if (bkpinfo->nonbootable_backup) {
		res = 0;
	}							// hack
	if (!res) {
		log_to_screen("Boot+data disks were created OK");
		sprintf(command, "mkdir -p /root/images/mindi/");
		log_msg(2, command);
		run_program_and_log_output(command, FALSE);
		sprintf(command,
				"cp -f %s/images/mindi.iso /root/images/mindi/mondorescue.iso",
				bkpinfo->scratchdir);
		log_msg(2, command);
		run_program_and_log_output(command, FALSE);
		if (bkpinfo->nonbootable_backup) {
			sprintf(command, "cp -f %s/all.tar.gz %s/images",
					bkpinfo->tmpdir, bkpinfo->scratchdir);
			if (system(command)) {
				fatal_error("Unable to create temporary duff tarball");
			}
		}
		sprintf(command, "cp -f %s/mindi-*oot*.img %s/images",
				bkpinfo->tmpdir, bkpinfo->scratchdir);
		sprintf(tmp, "cp -f %s/images/all.tar.gz %s", bkpinfo->scratchdir,
				bkpinfo->tmpdir);
		if (system(tmp)) {
			fatal_error("Cannot find all.tar.gz in tmpdir");
		}
		if (res) {
			mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
		} else {
			mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		}
	} else {
		log_to_screen("Mindi failed to create your boot+data disks.");
		strcpy(command, "grep 'Fatal error' /var/log/mindi.log");
		strcpy(tmp, call_program_and_get_last_line_of_output(command));
		if (strlen(tmp) > 1) {
			popup_and_OK(tmp);
		}
	}
	paranoid_free(tmp);
	paranoid_free(use_lzo_sz);
	paranoid_free(scratchdir);
	paranoid_free(use_comp_sz);
	paranoid_free(bootldr_str);
	paranoid_free(tape_device);
	paranoid_free(last_filelist_number);
	paranoid_free(broken_bios_sz);
	paranoid_free(cd_recovery_sz);
	paranoid_free(tape_size_sz);
	paranoid_free(devs_to_exclude);
	paranoid_free(use_lilo_sz);
	paranoid_free(value);
	paranoid_free(bootdev);
	paranoid_free(command);
	paranoid_free(use_star_sz);
	return (res);
}



/**
 * Maximum number of filesets allowed in this function.
 */
#define MAX_NOOF_SETS_HERE 32767

/**
 * Offset of the bkpinfo pointer (in bytes) from the
 * buffer passed to create_afio_files_in_background.
 */
#define BKPINFO_LOC_OFFSET (16+MAX_NOOF_SETS_HERE/8+16)

/**
 * Main function for each @c afio thread.
 * @param inbuf A transfer block containing:
 * - @c p_last_set_archived: [offset 0] pointer to an @c int
 *   containing the last set archived.
 * - @c p_archival_threads_running: [offset 4] pointer to an @c int
 *   containing the number of archival threads currently running.
 * - @c p_next_set_to_archive: [offset 8] pointer to an @c int containing
 *   the next set that should be archived.
 * - @c p_list_of_fileset_flags: [offset 12] @c char pointer pointing to a
 *   bit array, where each bit corresponds to a filelist (1=needs
 *   to be archived, 0=archived).
 * - @c bkpinfo: [offset BKPINFO_LOC_OFFSET] pointer to backup information
 *   structure. Fields used:
 *   - @c tmpdir
 *   - @c zip_suffix
 *
 * Any of the above may be modified by the caller at any time.
 *
 * @bug Assumes @c int pointers are 4 bytes.
 * @see archive_this_fileset
 * @see make_afioballs_and_images
 * @return NULL, always.
 * @ingroup LLarchiveGroup
 */
void *create_afio_files_in_background(void *inbuf)
{
	long int archiving_set_no;
	char *archiving_filelist_fname;
	char *archiving_afioball_fname;
	char *curr_xattr_list_fname;
	char *curr_acl_list_fname;

	struct s_bkpinfo *bkpinfo;
	char *tmp;
	int res = 0, retval = 0;
	int *p_archival_threads_running;
	int *p_last_set_archived;
	int *p_next_set_to_archive;
	char *p_list_of_fileset_flags;
	int this_thread_no = g_current_thread_no++;

	malloc_string(curr_xattr_list_fname);
	malloc_string(curr_acl_list_fname);
	malloc_string(archiving_filelist_fname);
	malloc_string(archiving_afioball_fname);
	malloc_string(tmp);
	p_last_set_archived = (int *) inbuf;
	p_archival_threads_running = (int *) (inbuf + 4);
	p_next_set_to_archive = (int *) (inbuf + 8);
	p_list_of_fileset_flags = (char *) (inbuf + 12);
	bkpinfo = (struct s_bkpinfo *) (inbuf + BKPINFO_LOC_OFFSET);

	sprintf(archiving_filelist_fname, FILELIST_FNAME_RAW_SZ,
			bkpinfo->tmpdir, 0L);
	for (archiving_set_no = 0; does_file_exist(archiving_filelist_fname);
		 sprintf(archiving_filelist_fname, FILELIST_FNAME_RAW_SZ,
				 bkpinfo->tmpdir, archiving_set_no)) {
		if (g_exiting) {
			fatal_error("Execution run aborted (pthread)");
		}
		if (archiving_set_no >= MAX_NOOF_SETS_HERE) {
			fatal_error
				("Maximum number of filesets exceeded. Adjust MAX_NOOF_SETS_HERE, please.");
		}
		if (!semaphore_p()) {
			log_msg(3, "P sem failed (pid=%d)", (int) getpid());
			fatal_error("Cannot get semaphore P");
		}
		if (archiving_set_no < *p_next_set_to_archive) {
			archiving_set_no = *p_next_set_to_archive;
		}
		*p_next_set_to_archive = *p_next_set_to_archive + 1;
		if (!semaphore_v()) {
			fatal_error("Cannot get semaphore V");
		}

		/* backup this set of files */
		sprintf(archiving_afioball_fname, AFIOBALL_FNAME_RAW_SZ,
				bkpinfo->tmpdir, archiving_set_no, bkpinfo->zip_suffix);
		sprintf(archiving_filelist_fname, FILELIST_FNAME_RAW_SZ,
				bkpinfo->tmpdir, archiving_set_no);
		if (!does_file_exist(archiving_filelist_fname)) {
			log_msg(3,
					"%s[%d:%d] - well, I would archive %d, except that it doesn't exist. I'll stop now.",
					FORTY_SPACES, getpid(), this_thread_no,
					archiving_set_no);
			break;
		}

		sprintf(tmp, AFIOBALL_FNAME_RAW_SZ, bkpinfo->tmpdir,
				archiving_set_no - ARCH_BUFFER_NUM, bkpinfo->zip_suffix);
		if (does_file_exist(tmp)) {
			log_msg(4, "%s[%d:%d] - waiting for storer", FORTY_SPACES,
					getpid(), this_thread_no);
			while (does_file_exist(tmp)) {
				sleep(1);
			}
			log_msg(4, "[%d] - continuing", getpid());
		}

		log_msg(4, "%s[%d:%d] - EXATing %d...", FORTY_SPACES, getpid(),
				this_thread_no, archiving_set_no);
		sprintf(curr_xattr_list_fname, XATTR_LIST_FNAME_RAW_SZ,
				bkpinfo->tmpdir, archiving_set_no);
		sprintf(curr_acl_list_fname, ACL_LIST_FNAME_RAW_SZ,
				bkpinfo->tmpdir, archiving_set_no);
		get_fattr_list(archiving_filelist_fname, curr_xattr_list_fname);
		get_acl_list(archiving_filelist_fname, curr_acl_list_fname);

		log_msg(4, "%s[%d:%d] - archiving %d...", FORTY_SPACES, getpid(),
				this_thread_no, archiving_set_no);
		res =
			archive_this_fileset(bkpinfo, archiving_filelist_fname,
								 archiving_afioball_fname,
								 archiving_set_no);
		retval += res;

		if (res) {
			sprintf(tmp,
					"Errors occurred while archiving set %ld. Please review logs.",
					archiving_set_no);
			log_to_screen(tmp);
		}
		if (!semaphore_p()) {
			fatal_error("Cannot get semaphore P");
		}

		set_bit_N_of_array(p_list_of_fileset_flags, archiving_set_no, 5);

		if (*p_last_set_archived < archiving_set_no) {
			*p_last_set_archived = archiving_set_no;
		}						// finished archiving this one

		if (!semaphore_v()) {
			fatal_error("Cannot get semaphore V");
		}
		log_msg(4, "%s[%d:%d] - archived %d OK", FORTY_SPACES, getpid(),
				this_thread_no, archiving_set_no);
		archiving_set_no++;
	}
	if (!semaphore_p()) {
		fatal_error("Cannot get semaphore P");
	}
	(*p_archival_threads_running)--;
	if (!semaphore_v()) {
		fatal_error("Cannot get semaphore V");
	}
	log_msg(3, "%s[%d:%d] - exiting", FORTY_SPACES, getpid(),
			this_thread_no);
	paranoid_free(archiving_filelist_fname);
	paranoid_free(archiving_afioball_fname);
	paranoid_free(curr_xattr_list_fname);
	paranoid_free(curr_acl_list_fname);
	paranoid_free(tmp);
	pthread_exit(NULL);
}





/**
 * Finalize the backup.
 * For streaming backups, this writes the closing block
 * to the stream. For CD-based backups, this creates
 * the final ISO image.
 * @param bkpinfo The backup information structure, used only
 * for the @c backup_media_type.
 * @ingroup MLarchiveGroup
 */
int do_that_final_phase(struct s_bkpinfo *bkpinfo)
{

	/*@ int ************************************** */
	int res = 0;
	int retval = 0;

	/*@ buffers ********************************** */

	assert(bkpinfo != NULL);
	mvaddstr_and_log_it(g_currentY, 0,
						"Writing any remaining data to media         ");

	log_msg(1, "Closing tape/CD ... ");
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type))
		/* write tape/cdstream */
	{
		closeout_tape(bkpinfo);
	} else
		/* write final ISO */
	{
		res = write_final_iso_if_necessary(bkpinfo);
		retval += res;
		if (res) {
			log_msg(1, "write_final_iso_if_necessary returned an error");
		}
	}
	log_msg(2, "Fork is exiting ... ");

	mvaddstr_and_log_it(g_currentY++, 74, "Done.");

	/* final stuff */
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}

	return (retval);
}


/**
 * Initialize the backup.
 * Does the following:
 * - Sets up the serial number.
 * - For streaming backups, opens the tape stream and writes the data disks
 *   and backup headers.
 * - For CD-based backups, wipes the ISOs in the target directory.
 *
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c cdrw_speed
 * - @c prefix
 * - @c isodir
 * - @c media_device
 * - @c scratchdir
 * - @c tmpdir
 * @return The number of errors encountered (0 for success).
 * @ingroup MLarchiveGroup
 */
int do_that_initial_phase(struct s_bkpinfo *bkpinfo)
{
	/*@ int *************************************** */
	int retval = 0;

	/*@ buffers *********************************** */
	char *command, *tmpfile, *data_disks_file;

	assert(bkpinfo != NULL);
	malloc_string(command);
	malloc_string(tmpfile);
	malloc_string(data_disks_file);
	sprintf(data_disks_file, "%s/all.tar.gz", bkpinfo->tmpdir);

	snprintf(g_serial_string, MAX_STR_LEN - 1,
			 call_program_and_get_last_line_of_output("dd \
if=/dev/urandom bs=16 count=1 2> /dev/null | \
hexdump | tr -s ' ' '0' | head -n1"));
	strip_spaces(g_serial_string);
	strcat(g_serial_string, "...word.");
	log_msg(2, "g_serial_string = '%s'", g_serial_string);
	assert(strlen(g_serial_string) < MAX_STR_LEN);

	sprintf(tmpfile, "%s/archives/SERIAL-STRING", bkpinfo->scratchdir);
	if (write_one_liner_data_file(tmpfile, g_serial_string)) {
		log_msg(1, "%ld: Failed to write serial string", __LINE__);
	}

	mvaddstr_and_log_it(g_currentY, 0, "Preparing to archive your data");
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		if (bkpinfo->backup_media_type == cdstream) {
			openout_cdstream(bkpinfo->media_device, bkpinfo->cdrw_speed);
		} else {
			openout_tape(bkpinfo->media_device, bkpinfo->internal_tape_block_size);	/* sets g_tape_stream */
		}
		if (!g_tape_stream) {
			fatal_error("Cannot open backup (streaming) device");
		}
		log_msg(1, "Backup (stream) opened OK");
		write_data_disks_to_stream(data_disks_file);
	} else {
		log_msg(1, "Backing up to CD's");
	}

	sprintf(command, "rm -f %s/%s/%s-[1-9]*.iso", bkpinfo->isodir,
			bkpinfo->nfs_remote_dir, bkpinfo->prefix);
	paranoid_system(command);
	wipe_archives(bkpinfo->scratchdir);
	mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		write_header_block_to_stream(0, "start-of-tape",
									 BLK_START_OF_TAPE);
		write_header_block_to_stream(0, "start-of-backup",
									 BLK_START_OF_BACKUP);
	}
	paranoid_free(command);
	paranoid_free(tmpfile);
	paranoid_free(data_disks_file);
	return (retval);
}


/**
 * Calls floppy-formatting @c cmd and tracks its progress if possible.
 *
 * @param cmd The command to run (e.g. @c fdformat @c /dev/fd0).
 * @param title The human-friendly description of the floppy you are writing.
 * This will be used as the title in the progress bar window. Example:
 * "Formatting disk /dev/fd0".
 * @see format_disk
 * @return The exit code of fdformat/superformat.
 */
int format_disk_SUB(char *cmd, char *title)
{

	/*@ int *************************************************************** */
	int res = 0;
	int percentage = 0;
	int maxtracks = 0;
	int trackno = 0;
	int last_trkno = 0;

	/*@ buffers *********************************************************** */
	char *command;
	char *tempfile;

	/*@ pointers ********************************************************** */
	FILE *pin;

	assert_string_is_neither_NULL_nor_zerolength(cmd);
	assert_string_is_neither_NULL_nor_zerolength(title);

	malloc_string(command);
	malloc_string(tempfile);
#ifdef __FreeBSD__
/* Ugh. FreeBSD fdformat prints out this pretty progress indicator that's
   impossible to parse. It looks like
   VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVF-------------------
   where V means verified, E means error, F means formatted, and - means
   not done yet.
*/
	return (run_program_and_log_to_screen(cmd, title));
#endif

/* if Debian then do bog-standard superformat; don't be pretty */
	if (strstr(cmd, "superformat")) {
		return (run_program_and_log_to_screen(cmd, title));
	}
/* if not Debian then go ahead & use fdformat */
	strcpy(tempfile,
		   call_program_and_get_last_line_of_output
		   ("mktemp -q /tmp/mondo.XXXXXXXX"));
	sprintf(command, "%s >> %s 2>> %s; rm -f %s", cmd, tempfile, tempfile,
			tempfile);
	log_msg(3, command);
	open_evalcall_form(title);
	if (!(pin = popen(command, "r"))) {
		log_OS_error("fmt err");
		return (1);
	}
	if (strstr(command, "1722")) {
		maxtracks = 82;
	} else {
		maxtracks = 80;
	}
	for (sleep(1); does_file_exist(tempfile); sleep(1)) {
		trackno = get_trackno_from_logfile(tempfile);
		if (trackno < 0 || trackno > 80) {
			log_msg(1, "Weird track#");
			continue;
		}
		percentage = trackno * 100 / maxtracks;
		if (trackno <= 5 && last_trkno > 40) {
			close_evalcall_form();
			strcpy(title, "Verifying format");
			open_evalcall_form(title);
		}
		last_trkno = trackno;
		update_evalcall_form(percentage);
	}
	close_evalcall_form();
	if (pclose(pin)) {
		res++;
		log_OS_error("Unable to pclose");
	}
	unlink(tempfile);
	paranoid_free(command);
	paranoid_free(tempfile);
	return (res);
}

/**
 * Wrapper around @c format_disk_SUB().
 * This function calls @c format_disk_SUB() with a @c device of its @c device
 * parameter and a @c title of Formatting disk @c device. If the format
 * fails, the user will be given the option of retrying.
 *
 * @param device The floppy drive to write to.
 * @see format_disk_SUB
 * @return The exit code of fdformat/superformat.
 * @ingroup deviceGroup
 */
int format_disk(char *device)
{

	/*@ int ************************************************************** */
	int res = 0;

	/*@ buffer *********************************************************** */
	char *command;
	char *title;


	assert_string_is_neither_NULL_nor_zerolength(device);
	malloc_string(title);
	command = malloc(1000);
	if (!system("which superformat > /dev/null 2> /dev/null")) {
		sprintf(command, "superformat %s", device);
	} else {
#ifdef __FreeBSD__
		sprintf(command, "fdformat -y %s", device);
#else
		sprintf(command, "fdformat %s", device);
#endif
	}
	sprintf(title, "Formatting disk %s", device);
	while ((res = format_disk_SUB(command, title))) {
		if (!ask_me_yes_or_no("Failed to format disk. Retry?")) {
			return (res);
		}
	}
	paranoid_free(title);
	paranoid_free(command);
	return (res);
}

/**
 * Get the <tt>N</tt>th bit of @c array.
 * @param array The bit-array (as a @c char pointer).
 * @param N The number of the bit you want.
 * @return TRUE (bit is set) or FALSE (bit is not set).
 * @see set_bit_N_of_array
 * @ingroup utilityGroup
 */
bool get_bit_N_of_array(char *array, int N)
{
	int element_number;
	int bit_number;
	int mask;

	element_number = N / 8;
	bit_number = N % 8;
	mask = 1 << bit_number;
	if (array[element_number] & mask) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}









/**
 * @addtogroup LLarchiveGroup
 * @{
 */
/**
 * Start up threads to archive your files.
 *
 * This function starts @c ARCH_THREADS threads,
 * each starting execution in @c create_afio_files_in_background().
 * Each thread will archive individual filesets, based on the
 * pointers passed to it and continually updated, until all files
 * have been backed up. This function coordinates the threads
 * and copies their output to the @c scratchdir.
 *
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c scratchdir
 * - @c tmpdir
 * - @c zip_suffix
 *
 * @return The number of errors encountered (0 for success)
 */
int make_afioballs_and_images(struct s_bkpinfo *bkpinfo)
{

	/*@ int ************************************************** */
	int retval = 0;
	long int storing_set_no = 0;
	int res = 0;
	bool done_storing = FALSE;
	char *result_str;
	char *transfer_block;
	void *vp;
	void **pvp;

	/*@ buffers ********************************************** */
	char *storing_filelist_fname;
	char *storing_afioball_fname;
	char *tmp;
	char *media_usage_comment;
	pthread_t archival_thread[ARCH_THREADS];
	char *p_list_of_fileset_flags;
	int *p_archival_threads_running;
	int *p_last_set_archived;
	int *p_next_set_to_archive;
	int noof_threads;
	int i;
	char *curr_xattr_list_fname;
	char *curr_acl_list_fname;
	int misc_counter_that_is_not_important = 0;

	log_msg(8, "here");
	assert(bkpinfo != NULL);
	tmp = malloc(MAX_STR_LEN * 2);
	malloc_string(result_str);
	malloc_string(curr_xattr_list_fname);
	malloc_string(curr_acl_list_fname);
	malloc_string(storing_filelist_fname);
	malloc_string(media_usage_comment);
	malloc_string(storing_afioball_fname);
	transfer_block =
		malloc(sizeof(struct s_bkpinfo) + BKPINFO_LOC_OFFSET + 64);
	memset((void *) transfer_block, 0,
		   sizeof(struct s_bkpinfo) + BKPINFO_LOC_OFFSET + 64);
	p_last_set_archived = (int *) transfer_block;
	p_archival_threads_running = (int *) (transfer_block + 4);
	p_next_set_to_archive = (int *) (transfer_block + 8);
	p_list_of_fileset_flags = (char *) (transfer_block + 12);
	memcpy((void *) (transfer_block + BKPINFO_LOC_OFFSET),
		   (void *) bkpinfo, sizeof(struct s_bkpinfo));
	pvp = &vp;
	vp = (void *) result_str;
	*p_archival_threads_running = 0;
	*p_last_set_archived = -1;
	*p_next_set_to_archive = 0;
	sprintf(tmp, "%s/archives/filelist.full", bkpinfo->scratchdir);
	log_to_screen("Archiving regular files");
	log_msg(5, "Go, Shorty. It's your birthday.");
	open_progress_form("Backing up filesystem",
					   "I am backing up your live filesystem now.",
					   "Please wait. This may take a couple of hours.",
					   "Working...",
					   get_last_filelist_number(bkpinfo) + 1);

	log_msg(5, "We're gonna party like it's your birthday.");

	srand((unsigned int) getpid());
	g_sem_key = 1234 + random() % 30000;
	if ((g_sem_id =
		 semget((key_t) g_sem_key, 1,
				IPC_CREAT | S_IREAD | S_IWRITE)) == -1) {
		fatal_error("MABAI - unable to semget");
	}
	if (!set_semvalue()) {
		fatal_error("Unable to init semaphore");
	}							// initialize semaphore
	for (noof_threads = 0; noof_threads < ARCH_THREADS; noof_threads++) {
		log_msg(8, "Creating thread #%d", noof_threads);
		(*p_archival_threads_running)++;
		if ((res =
			 pthread_create(&archival_thread[noof_threads], NULL,
							create_afio_files_in_background,
							(void *) transfer_block))) {
			fatal_error("Unable to create an archival thread");
		}
	}

	log_msg(8, "About to enter while() loop");
	while (!done_storing) {
		if (g_exiting) {
			fatal_error("Execution run aborted (main loop)");
		}
		if (*p_archival_threads_running == 0
			&& *p_last_set_archived == storing_set_no - 1) {
			log_msg(2,
					"No archival threads are running. The last stored set was %d and I'm looking for %d. Take off your make-up; the party's over... :-)",
					*p_last_set_archived, storing_set_no);
			done_storing = TRUE;
		} else
			if (!get_bit_N_of_array
				(p_list_of_fileset_flags, storing_set_no)) {
			misc_counter_that_is_not_important =
				(misc_counter_that_is_not_important + 1) % 5;
			if (!misc_counter_that_is_not_important) {
				update_progress_form(media_usage_comment);
			}
			sleep(1);
		} else
			// store set N
		{
			sprintf(storing_filelist_fname, FILELIST_FNAME_RAW_SZ,
					bkpinfo->tmpdir, storing_set_no);
			sprintf(storing_afioball_fname, AFIOBALL_FNAME_RAW_SZ,
					bkpinfo->tmpdir, storing_set_no, bkpinfo->zip_suffix);
			sprintf(curr_xattr_list_fname, XATTR_LIST_FNAME_RAW_SZ,
					bkpinfo->tmpdir, storing_set_no);
			sprintf(curr_acl_list_fname, ACL_LIST_FNAME_RAW_SZ,
					bkpinfo->tmpdir, storing_set_no);

			log_msg(2, "Storing set %d", storing_set_no);
			while (!does_file_exist(storing_filelist_fname)
				   || !does_file_exist(storing_afioball_fname)) {
				log_msg(2,
						"Warning - either %s or %s doesn't exist yet. I'll pause 5 secs.",
						storing_filelist_fname, storing_afioball_fname);
				sleep(5);
			}
			strcpy(media_usage_comment,
				   percent_media_full_comment(bkpinfo));
			/* copy to CD (scratchdir) ... and an actual CD-R if necessary */
			if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
				register_in_tape_catalog(fileset, storing_set_no, -1,
										 storing_afioball_fname);
				maintain_collection_of_recent_archives(bkpinfo->tmpdir,
													   storing_afioball_fname);
				iamhere("Writing EXAT files");
				res +=
					write_EXAT_files_to_tape(bkpinfo,
											 curr_xattr_list_fname,
											 curr_acl_list_fname);
// archives themselves
				res +=
					move_files_to_stream(bkpinfo, storing_afioball_fname,
										 NULL);
			} else {
				res =
					move_files_to_cd(bkpinfo, storing_filelist_fname,
									 curr_xattr_list_fname,
									 curr_acl_list_fname,
									 storing_afioball_fname, NULL);
			}
			retval += res;
			g_current_progress++;
			update_progress_form(media_usage_comment);
			if (res) {
				sprintf(tmp,
						"Failed to add archive %ld's files to CD dir\n",
						storing_set_no);
				log_to_screen(tmp);
				fatal_error
					("Is your hard disk full? If not, please send the author the logfile.");
			}
			storing_set_no++;
			//      sleep(2);
		}
	}
	close_progress_form();

	sprintf(tmp, "Your regular files have been archived ");
	log_msg(2, "Joining background threads to foreground thread");
	for (i = 0; i < noof_threads; i++) {
		pthread_join(archival_thread[i], pvp);
		log_msg(3, "Thread %d of %d: closed OK", i + 1, noof_threads);
	}
	del_semvalue();
	log_msg(2, "Done.");
	if (retval) {
		strcat(tmp, "(with errors).");
	} else {
		strcat(tmp, "successfully.");
	}
	log_to_screen(tmp);
	paranoid_free(transfer_block);
	paranoid_free(result_str);
	paranoid_free(storing_filelist_fname);
	paranoid_free(media_usage_comment);
	paranoid_free(storing_afioball_fname);
	paranoid_free(curr_xattr_list_fname);
	paranoid_free(curr_acl_list_fname);
	return (retval);
}


void pause_for_N_seconds(int how_long, char *msg)
{
	int i;
	open_evalcall_form(msg);
	for (i = 0; i < how_long; i++) {
		update_evalcall_form((int) ((100.0 / (float) (how_long) * i)));
		sleep(1);
	}
	close_evalcall_form();
}




/**
 * Create an ISO image in @c destfile, from files in @c bkpinfo->scratchdir.
 *
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c call_after_iso
 * - @c call_before_iso
 * - @c call_burn_iso
 * - @c call_make_iso
 * - @c make_cd_use_lilo
 * - @c manual_cd_tray
 * - @c nonbootable_backup
 * - @c scratchdir
 *
 * @param destfile Where to put the generated ISO image.
 * @return The number of errors encountered (0 for success)
 */
int make_iso_fs(struct s_bkpinfo *bkpinfo, char *destfile)
{
	/*@ int ********************************************** */
	int retval = 0;
	int res;

	/*@ buffers ****************************************** */
	char *tmp;
	char *old_pwd;
	char *result_sz;
	char *message_to_screen;
	char *sz_blank_disk;
	char *fnam;
	char *tmp2;
	char *tmp3;
	bool cd_is_mountable;

	malloc_string(old_pwd);
	malloc_string(result_sz);
	malloc_string(message_to_screen);
	malloc_string(sz_blank_disk);
	malloc_string(fnam);
	tmp = malloc(1200);
	tmp2 = malloc(1200);
	tmp3 = malloc(1200);
	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(destfile);

	sprintf(tmp, "%s/isolinux.bin", bkpinfo->scratchdir);
	sprintf(tmp2, "%s/isolinux.bin", bkpinfo->tmpdir);
	if (does_file_exist(tmp)) {
		sprintf(tmp3, "cp -f %s %s", tmp, tmp2);
		paranoid_system(tmp3);
	}
	if (!does_file_exist(tmp) && does_file_exist(tmp2)) {
		sprintf(tmp3, "cp -f %s %s", tmp2, tmp);
		paranoid_system(tmp3);
	}
	/*
	   if (!does_file_exist(tmp))
	   {
	   log_msg (2, "Silly bug in Mindi.pl; workaround in progress...");
	   strcpy(fnam, call_program_and_get_last_line_of_output("locate isolinux.bin | tail -n1"));
	   if (strlen(fnam)>0 && does_file_exist(fnam))
	   {
	   sprintf(tmp, "cp -f %s %s", fnam, bkpinfo->scratchdir);
	   res = run_program_and_log_output(tmp, FALSE);
	   }
	   else
	   {
	   res = 1;
	   }
	   if (res)
	   {
	   log_msg (2, "Could not work around silly bug in Mindi.pl - sorry! Isolinux.bin missing");
	   }
	   }
	 */
	free(tmp2);
	free(tmp3);
	tmp2 = NULL;
	tmp3 = NULL;
	if (bkpinfo->backup_media_type == iso && bkpinfo->manual_cd_tray) {
		popup_and_OK("Please insert new media and press Enter.");
	}

	log_msg(2, "make_iso_fs --- scratchdir=%s --- destfile=%s",
			bkpinfo->scratchdir, destfile);
	(void) getcwd(old_pwd, MAX_STR_LEN - 1);
	sprintf(tmp, "chmod 744 %s", bkpinfo->scratchdir);
	run_program_and_log_output(tmp, FALSE);
	chdir(bkpinfo->scratchdir);

	if (bkpinfo->call_before_iso[0] != '\0') {
		sprintf(message_to_screen, "Running pre-ISO call for CD#%d",
				g_current_media_number);
		res =
			eval_call_to_make_ISO(bkpinfo, bkpinfo->call_before_iso,
								  destfile, g_current_media_number,
								  MONDO_LOGFILE, message_to_screen);
		if (res) {
			strcat(message_to_screen, "...failed");
		} else {
			strcat(message_to_screen, "...OK");
		}
		log_to_screen(message_to_screen);
		retval += res;
	}

	if (bkpinfo->call_make_iso[0] != '\0') {
		log_msg(2, "bkpinfo->call_make_iso = %s", bkpinfo->call_make_iso);
		sprintf(tmp, "%s/archives/NOT-THE-LAST", bkpinfo->scratchdir);
		sprintf(message_to_screen, "Making an ISO (%s #%d)",
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);

		pause_and_ask_for_cdr(2, &cd_is_mountable);	/* if g_current_media_number >= 2 then pause & ask */
		if (retval) {
			log_to_screen
				("Serious error(s) occurred already. I shan't try to write to media.");
		} else {
			res =
				eval_call_to_make_ISO(bkpinfo, bkpinfo->call_make_iso,
									  bkpinfo->scratchdir,
									  g_current_media_number,
									  MONDO_LOGFILE, message_to_screen);
			if (res) {
				log_to_screen("%s...failed to write", message_to_screen);
			} else {
				log_to_screen("%s...OK", message_to_screen);
				if (!run_program_and_log_output
					("tail -n10 /var/log/mondo-archive.log | fgrep ':-('",
					 1)) {
					log_to_screen
						("Despite nonfatal errors, growisofs confirms the write was successful.");
				}
			}
			retval += res;
#ifdef DVDRWFORMAT
			sprintf(tmp,
					"tail -n8 %s | grep 'blank=full.*dvd-compat.*DAO'",
					MONDO_LOGFILE);
			if (g_backup_media_type == dvd
				&& (res || !run_program_and_log_output(tmp, 1))) {
				log_to_screen
					("Failed to write to disk. I shall blank it and then try again.");
				sleep(5);
				system("sync");
				pause_for_N_seconds(5, "Letting DVD drive settle");

// dvd+rw-format --- OPTION 2
				if (!bkpinfo->please_dont_eject) {
					log_to_screen("Ejecting media to clear drive status.");
					eject_device(bkpinfo->media_device);
					inject_device(bkpinfo->media_device);
				}
				pause_for_N_seconds(5, "Letting DVD drive settle");
				sprintf(sz_blank_disk, "dvd+rw-format %s",
						bkpinfo->media_device);
				log_msg(3, "sz_blank_disk = '%s'", sz_blank_disk);
				res =
					run_external_binary_with_percentage_indicator_NEW
					("Blanking DVD disk", sz_blank_disk);
				if (res) {
					log_to_screen
						("Warning - format failed. (Was it a DVD-R?) Sleeping for 5 seconds to take a breath...");
					pause_for_N_seconds(5,
										"Letting DVD drive settle... and trying again.");
					res =
						run_external_binary_with_percentage_indicator_NEW
						("Blanking DVD disk", sz_blank_disk);
					if (res) {
						log_to_screen("Format failed a second time.");
					}
				} else {
					log_to_screen
						("Format succeeded. Sleeping for 5 seconds to take a breath...");
				}
				pause_for_N_seconds(5, "Letting DVD drive settle");
				if (!bkpinfo->please_dont_eject) {
					log_to_screen("Ejecting media to clear drive status.");
					eject_device(bkpinfo->media_device);
					inject_device(bkpinfo->media_device);
				}
				pause_for_N_seconds(5, "Letting DVD drive settle");
				res =
					eval_call_to_make_ISO(bkpinfo, bkpinfo->call_make_iso,
										  bkpinfo->scratchdir,
										  g_current_media_number,
										  MONDO_LOGFILE,
										  message_to_screen);
				retval += res;
				if (!bkpinfo->please_dont_eject) {
					log_to_screen("Ejecting media.");
					eject_device(bkpinfo->media_device);
				}
				if (res) {
					log_to_screen("Dagnabbit. It still failed.");
				} else {
					log_to_screen
						("OK, this time I successfully backed up to DVD.");
				}
			}
#endif
			if (g_backup_media_type == dvd && !bkpinfo->please_dont_eject) {
				eject_device(bkpinfo->media_device);
			}
		}
	} else {
		sprintf(message_to_screen, "Running mkisofs to make %s #%d",
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
		log_msg(1, message_to_screen);
		sprintf(result_sz, "Call to mkisofs to make ISO (%s #%d) ",
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
		if (bkpinfo->nonbootable_backup) {
			log_msg(1, "Making nonbootable backup");
// FIXME --- change mkisofs string to MONDO_MKISOFS_NONBOOTABLE and add ' .' at end
			res =
				eval_call_to_make_ISO(bkpinfo,
									  "mkisofs -o _ISO_ -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ .",
									  destfile, g_current_media_number,
									  MONDO_LOGFILE, message_to_screen);
		} else {
			log_msg(1, "Making bootable backup");

#ifdef __FreeBSD__
			bkpinfo->make_cd_use_lilo = TRUE;
#endif


			log_msg(1, "make_cd_use_lilo is actually %d",
					bkpinfo->make_cd_use_lilo);
			if (bkpinfo->make_cd_use_lilo) {
				log_msg(1, "make_cd_use_lilo = TRUE");
// FIXME --- change mkisofs string to MONDO_MKISOFS_REGULAR_SYSLINUX/LILO depending on bkpinfo->make_cd_usE_lilo
// and add ' .' at end
#ifdef __IA64__
				log_msg(1, "IA64 --> elilo");
				res = eval_call_to_make_ISO(bkpinfo,
											//-b images/mindi-boot.2880.img
											"mkisofs -no-emul-boot -b images/mindi-bootroot."
											IA64_BOOT_SIZE
											".img -c boot.cat -o _ISO_ -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ .",
											destfile,
											g_current_media_number,
											MONDO_LOGFILE,
											message_to_screen);
#else
// FIXME --- change mkisofs string to MONDO_MKISOFS_REGULAR_SYSLINUX/LILO depending on bkpinfo->make_cd_usE_lilo
// and add ' .' at end
				log_msg(1, "Non-ia64 --> lilo");
				res =
					eval_call_to_make_ISO(bkpinfo,
										  "mkisofs -b images/mindi-bootroot.2880.img -c boot.cat -o _ISO_ -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ .",
										  destfile, g_current_media_number,
										  MONDO_LOGFILE,
										  message_to_screen);
#endif
			} else {
				log_msg(1, "make_cd_use_lilo = FALSE");
				log_msg(1, "Isolinux");
				res =
					eval_call_to_make_ISO(bkpinfo,
										  "mkisofs -no-emul-boot -b isolinux.bin -boot-load-size 4 -boot-info-table -c boot.cat -o _ISO_ -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ .",
										  destfile, g_current_media_number,
										  MONDO_LOGFILE,
										  message_to_screen);
			}
		}
		if (res) {
			strcat(result_sz, "...failed");
		} else {
			strcat(result_sz, "...OK");
		}
		log_to_screen(result_sz);
		retval += res;
	}

	if (bkpinfo->backup_media_type == cdr
		|| bkpinfo->backup_media_type == cdrw) {
		if (is_this_device_mounted(bkpinfo->media_device)) {
			log_msg(2,
					"Warning - %s mounted. I'm unmounting it before I burn to it.",
					bkpinfo->media_device);
			sprintf(tmp, "umount %s", bkpinfo->media_device);
			run_program_and_log_output(tmp, FALSE);
		}
	}

	if (bkpinfo->call_burn_iso[0] != '\0') {
		log_msg(2, "bkpinfo->call_burn_iso = %s", bkpinfo->call_burn_iso);
		sprintf(message_to_screen, "Burning %s #%d",
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
		pause_and_ask_for_cdr(2, &cd_is_mountable);
		res =
			eval_call_to_make_ISO(bkpinfo, bkpinfo->call_burn_iso,
								  destfile, g_current_media_number,
								  MONDO_LOGFILE, message_to_screen);
		if (res) {
			strcat(message_to_screen, "...failed");
		} else {
			strcat(message_to_screen, "...OK");
		}
		log_to_screen(message_to_screen);
		retval += res;
	}

	if (bkpinfo->call_after_iso[0] != '\0') {
		sprintf(message_to_screen, "Running post-ISO call (%s #%d)",
				media_descriptor_string(bkpinfo->backup_media_type),
				g_current_media_number);
		res =
			eval_call_to_make_ISO(bkpinfo, bkpinfo->call_after_iso,
								  destfile, g_current_media_number,
								  MONDO_LOGFILE, message_to_screen);
		if (res) {
			strcat(message_to_screen, "...failed");
		} else {
			strcat(message_to_screen, "...OK");
		}
		log_to_screen(message_to_screen);
		retval += res;
	}

	chdir(old_pwd);
	if (retval) {
		log_msg(1, "WARNING - make_iso_fs returned an error");
	}
	paranoid_free(old_pwd);
	paranoid_free(result_sz);
	paranoid_free(message_to_screen);
	paranoid_free(sz_blank_disk);
	paranoid_free(fnam);
	paranoid_free(tmp);
	return (retval);
}









bool is_dev_an_NTFS_dev(char *bigfile_fname)
{
	char *tmp;
	char *command;
	malloc_string(tmp);
	malloc_string(command);
	sprintf(command,
			"dd if=%s bs=512 count=1 2> /dev/null | strings | head -n1",
			bigfile_fname);
	log_msg(1, "command = '%s'", command);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	log_msg(1, "--> tmp = '%s'", tmp);
	if (strstr(tmp, "NTFS")) {
		iamhere("TRUE");
		return (TRUE);
	} else {
		iamhere("FALSE");
		return (FALSE);
	}
}


/**
 * Back up big files by chopping them up.
 * This function backs up all "big" files (where "big" depends
 * on your backup media) in "chunks" (whose size again depends
 * on your media).
 *
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c optimal_set_size
 * @param biggielist_fname The path to a file containing a list of
 * all "big" files.
 * @return The number of errors encountered (0 for success)
 * @see slice_up_file_etc
 */
int
make_slices_and_images(struct s_bkpinfo *bkpinfo, char *biggielist_fname)
{

	/*@ pointers ******************************************* */
	FILE *fin;
	char *p;

	/*@ buffers ******************************************** */
	char *tmp;
	char *bigfile_fname;
	char *sz_devfile;
	char *ntfsprog_fifo = NULL;
	/*@ long *********************************************** */
	long biggie_file_number = 0;
	long noof_biggie_files = 0;
	long estimated_total_noof_slices = 0;

	/*@ int ************************************************ */
	int retval = 0;
	int res = 0;
	pid_t pid;
	FILE *ftmp = NULL;
	bool delete_when_done;
	bool use_ntfsprog;
	/*@ long long ****************************************** */
	long long biggie_fsize;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(biggielist_fname);

	malloc_string(tmp);
	malloc_string(bigfile_fname);
	malloc_string(sz_devfile);
	estimated_total_noof_slices =
		size_of_all_biggiefiles_K(bkpinfo) / bkpinfo->optimal_set_size + 1;

	log_msg(1, "size of all biggiefiles = %ld",
			size_of_all_biggiefiles_K(bkpinfo));
	log_msg(1, "estimated_total_noof_slices = %ld KB / %ld KB = %ld",
			size_of_all_biggiefiles_K(bkpinfo), bkpinfo->optimal_set_size,
			estimated_total_noof_slices);

	if (length_of_file(biggielist_fname) < 6) {
		log_msg(1, "No biggiefiles; fair enough...");
		return (0);
	}
	sprintf(tmp, "I am now backing up all large files.");
	log_to_screen(tmp);
	noof_biggie_files = count_lines_in_file(biggielist_fname);
	open_progress_form("Backing up big files", tmp,
					   "Please wait. This may take some time.", "",
					   estimated_total_noof_slices);
	if (!(fin = fopen(biggielist_fname, "r"))) {
		log_OS_error("Unable to openin biggielist");
		return (1);
	}
	for (fgets(bigfile_fname, MAX_STR_LEN, fin); !feof(fin);
		 fgets(bigfile_fname, MAX_STR_LEN, fin), biggie_file_number++) {
		use_ntfsprog = FALSE;
		if (bigfile_fname[strlen(bigfile_fname) - 1] < 32) {
			bigfile_fname[strlen(bigfile_fname) - 1] = '\0';
		}
		biggie_fsize = length_of_file(bigfile_fname);
		delete_when_done = FALSE;

		if (!does_file_exist(bigfile_fname)) {
			ftmp = fopen(bigfile_fname, "w");
			paranoid_fclose(ftmp);
			sprintf(tmp, "bigfile %s was deleted - creating a dummy",
					bigfile_fname);
			delete_when_done = TRUE;
		} else {
// Call ntfsclone (formerly partimagehack) if it's a /dev entry (i.e. a partition to be imaged)
			log_msg(2, "bigfile_fname = %s", bigfile_fname);
			use_ntfsprog = FALSE;
			if (!strncmp(bigfile_fname, "/dev/", 5)
				&& is_dev_an_NTFS_dev(bigfile_fname)) {
				use_ntfsprog = TRUE;
				log_msg(2,
						"Calling ntfsclone in background because %s is an NTFS partition",
						bigfile_fname);
				sprintf(sz_devfile, "/tmp/%d.%d.000",
						(int) (random() % 32768),
						(int) (random() % 32768));
				mkfifo(sz_devfile, 0x770);
				ntfsprog_fifo = sz_devfile;
				switch (pid = fork()) {
				case -1:
					fatal_error("Fork failure");
				case 0:
					log_msg(2,
							"CHILD - fip - calling feed_into_ntfsprog(%s, %s)",
							bigfile_fname, sz_devfile);
					res = feed_into_ntfsprog(bigfile_fname, sz_devfile);
					exit(res);
					break;
				default:
					log_msg(2,
							"feed_into_ntfsprog() called in background --- pid=%ld",
							(long int) (pid));
				}
			}
// Otherwise, use good old 'dd' and 'bzip2'
			else {
				sz_devfile[0] = '\0';
				ntfsprog_fifo = NULL;
			}

// Whether partition or biggiefile, just do your thang :-)
			sprintf(tmp, "Bigfile #%ld is '%s' (%ld KB)",
					biggie_file_number + 1, bigfile_fname,
					(long) biggie_fsize >> 10);
			if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
				write_header_block_to_stream(biggie_fsize, bigfile_fname,
											 use_ntfsprog ?
											 BLK_START_A_PIHBIGGIE :
											 BLK_START_A_NORMBIGGIE);
			}
			res =
				slice_up_file_etc(bkpinfo, bigfile_fname,
								  ntfsprog_fifo, biggie_file_number,
								  noof_biggie_files, use_ntfsprog);
			if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
				write_header_block_to_stream(0,
											 calc_checksum_of_file
											 (bigfile_fname),
											 BLK_STOP_A_BIGGIE);
			}
			retval += res;
			p = strrchr(bigfile_fname, '/');
			if (p) {
				p++;
			} else {
				p = bigfile_fname;
			}
			sprintf(tmp, "Archiving %s ... ", bigfile_fname);
			if (res) {
				strcat(tmp, "Failed!");
			} else {
				strcat(tmp, "OK");
			}
			if (delete_when_done) {
				unlink(bigfile_fname);
				delete_when_done = FALSE;
			}
		}
#ifndef _XWIN
		if (!g_text_mode) {
			newtDrawRootText(0, g_noof_rows - 2, tmp);
			newtRefresh();
		}
#endif
	}
	log_msg(1, "Finished backing up bigfiles");
	log_msg(1, "estimated slices = %ld; actual slices = %ld",
			estimated_total_noof_slices, g_current_progress);
	close_progress_form();
	paranoid_fclose(fin);
	paranoid_free(tmp);
	paranoid_free(bigfile_fname);
	paranoid_free(sz_devfile);
	return (retval);
}




/**
 * Single-threaded version of @c make_afioballs_and_images().
 * @see make_afioballs_and_images
 */
int make_afioballs_and_images_OLD(struct s_bkpinfo *bkpinfo)
{

	/*@ int ************************************************** */
	int retval = 0;
	long int curr_set_no = 0;
	int res = 0;

	/*@ buffers ********************************************** */
	char *curr_filelist_fname;
	char *curr_afioball_fname;
	char *curr_xattr_list_fname;
	char *curr_acl_list_fname;
	char *tmp;
	char *media_usage_comment;

	malloc_string(curr_afioball_fname);
	malloc_string(media_usage_comment);
	malloc_string(curr_filelist_fname);
	malloc_string(curr_xattr_list_fname);
	malloc_string(curr_acl_list_fname);

	tmp = malloc(MAX_STR_LEN * 2);

	sprintf(tmp, "%s/archives/filelist.full", bkpinfo->scratchdir);

	log_to_screen("Archiving regular files");

	open_progress_form("Backing up filesystem",
					   "I am backing up your live filesystem now.",
					   "Please wait. This may take a couple of hours.",
					   "Working...",
					   get_last_filelist_number(bkpinfo) + 1);

	sprintf(curr_filelist_fname, FILELIST_FNAME_RAW_SZ, bkpinfo->tmpdir,
			0L);

	for (curr_set_no = 0; does_file_exist(curr_filelist_fname);
		 sprintf(curr_filelist_fname, FILELIST_FNAME_RAW_SZ,
				 bkpinfo->tmpdir, ++curr_set_no)) {
		/* backup this set of files */
		sprintf(curr_filelist_fname, FILELIST_FNAME_RAW_SZ,
				bkpinfo->tmpdir, curr_set_no);
		sprintf(curr_afioball_fname, AFIOBALL_FNAME_RAW_SZ,
				bkpinfo->tmpdir, curr_set_no, bkpinfo->zip_suffix);

		log_msg(1, "EXAT'g set %ld", curr_set_no);
		sprintf(curr_xattr_list_fname, XATTR_LIST_FNAME_RAW_SZ,
				bkpinfo->tmpdir, curr_set_no);
		sprintf(curr_acl_list_fname, ACL_LIST_FNAME_RAW_SZ,
				bkpinfo->tmpdir, curr_set_no);
		get_fattr_list(curr_filelist_fname, curr_xattr_list_fname);
		get_acl_list(curr_filelist_fname, curr_acl_list_fname);

		log_msg(1, "Archiving set %ld", curr_set_no);
		res =
			archive_this_fileset(bkpinfo, curr_filelist_fname,
								 curr_afioball_fname, curr_set_no);
		retval += res;
		if (res) {
			sprintf(tmp,
					"Errors occurred while archiving set %ld. Perhaps your live filesystem changed?",
					curr_set_no);
			log_to_screen(tmp);
		}

		strcpy(media_usage_comment, percent_media_full_comment(bkpinfo));

		/* copy to CD (scratchdir) ... and an actual CD-R if necessary */
		if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
			register_in_tape_catalog(fileset, curr_set_no, -1,
									 curr_afioball_fname);
			maintain_collection_of_recent_archives(bkpinfo->tmpdir,
												   curr_afioball_fname);
			iamhere("Writing EXAT files");
			res +=
				write_EXAT_files_to_tape(bkpinfo, curr_xattr_list_fname,
										 curr_acl_list_fname);
// archives themselves
			res = move_files_to_stream(bkpinfo, curr_afioball_fname, NULL);
		} else {
			res =
				move_files_to_cd(bkpinfo, curr_filelist_fname,
								 curr_xattr_list_fname,
								 curr_acl_list_fname, curr_afioball_fname,
								 NULL);
		}
		retval += res;
		g_current_progress++;
		update_progress_form(media_usage_comment);

		if (res) {
			sprintf(tmp, "Failed to add archive %ld's files to CD dir\n",
					curr_set_no);
			log_to_screen(tmp);
			fatal_error
				("Is your hard disk is full? If not, please send the author the logfile.");
		}
	}
	close_progress_form();
	sprintf(tmp, "Your regular files have been archived ");
	if (retval) {
		strcat(tmp, "(with errors).");
	} else {
		strcat(tmp, "successfully.");
	}
	log_to_screen(tmp);
	paranoid_free(tmp);
	paranoid_free(curr_filelist_fname);
	paranoid_free(curr_afioball_fname);
	paranoid_free(media_usage_comment);
	paranoid_free(curr_xattr_list_fname);
	paranoid_free(curr_acl_list_fname);
	return (retval);
}

/* @} - end of LLarchiveGroup */


/**
 * Wrapper around @c make_afioballs_and_images().
 * @param bkpinfo the backup information structure. Only the
 * @c backup_media_type field is used within this function.
 * @return return code of make_afioballs_and_images
 * @see make_afioballs_and_images
 * @ingroup MLarchiveGroup
 */
int make_those_afios_phase(struct s_bkpinfo *bkpinfo)
{
	/*@ int ******************************************* */
	int res = 0;
	int retval = 0;

	assert(bkpinfo != NULL);

	mvaddstr_and_log_it(g_currentY, 0,
						"Archiving regular files to media          ");

	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		write_header_block_to_stream(0, "start-of-afioballs",
									 BLK_START_AFIOBALLS);
#if __FreeBSD__ == 5
		log_msg(1,
				"Using single-threaded make_afioballs_and_images() to suit b0rken FreeBSD 5.0");
		res = make_afioballs_and_images_OLD(bkpinfo);
#else
		res = make_afioballs_and_images_OLD(bkpinfo);
#endif
		write_header_block_to_stream(0, "stop-afioballs",
									 BLK_STOP_AFIOBALLS);
	} else {
		res = make_afioballs_and_images(bkpinfo);
	}

	retval += res;
	if (res) {
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
		log_msg(1, "make_afioballs_and_images returned an error");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	return (retval);
}

/**
 * Wrapper around @c make_slices_and_images().
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c scratchdir
 * - @c tmpdir
 * @return The number of errors encountered (0 for success)
 * @ingroup MLarchiveGroup
 */
int make_those_slices_phase(struct s_bkpinfo *bkpinfo)
{

	/*@ int ***************************************************** */
	int res = 0;
	int retval = 0;

	/*@ buffers ************************************************** */
	char *biggielist;
	char *command;
	char *blah;
	char *xattr_fname;
	char *acl_fname;

	assert(bkpinfo != NULL);
	/* slice big files */
	malloc_string(blah);
	malloc_string(biggielist);
	malloc_string(xattr_fname);
	malloc_string(acl_fname);
	command = malloc(1200);
	mvaddstr_and_log_it(g_currentY, 0,
						"Archiving large files to media           ");
	sprintf(biggielist, "%s/archives/biggielist.txt", bkpinfo->scratchdir);
	sprintf(xattr_fname, XATTR_BIGGLST_FNAME_RAW_SZ, bkpinfo->tmpdir);
	sprintf(acl_fname, ACL_BIGGLST_FNAME_RAW_SZ, bkpinfo->tmpdir);

	sprintf(command, "cp %s/biggielist.txt %s", bkpinfo->tmpdir,
			biggielist);
	paranoid_system(command);
	sprintf(blah, "biggielist = %s", biggielist);
	log_msg(2, blah);

	if (!does_file_exist(biggielist)) {
		log_msg(1, "BTW, the biggielist does not exist");
	}

	get_fattr_list(biggielist, xattr_fname);
	get_acl_list(biggielist, acl_fname);
	sprintf(command, "cp %s %s/archives/", xattr_fname,
			bkpinfo->scratchdir);
	paranoid_system(command);
	sprintf(command, "cp %s %s/archives/", acl_fname, bkpinfo->scratchdir);
	paranoid_system(command);

	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		res += write_EXAT_files_to_tape(bkpinfo, xattr_fname, acl_fname);
		sprintf(blah, "%ld", count_lines_in_file(biggielist));
		write_header_block_to_stream(0, blah, BLK_START_BIGGIEFILES);
	}
	res = make_slices_and_images(bkpinfo, biggielist);
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		write_header_block_to_stream(0, "end-of-biggiefiles",
									 BLK_STOP_BIGGIEFILES);
	}
	retval += res;
	if (res) {
		log_msg(1, "make_slices_and_images returned an error");
		mvaddstr_and_log_it(g_currentY++, 74, "Errors.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	paranoid_free(blah);
	paranoid_free(biggielist);
	paranoid_free(command);
	paranoid_free(xattr_fname);
	paranoid_free(acl_fname);
	return (retval);
}



/**
 * @addtogroup LLarchiveGroup
 * @{
 */
/**
 * Function pointer to an appropriate @c move_files_to_cd routine.
 * You can set this to your own function (for example, one to
 * transfer files over the network) or leave it as is.
 */
int (*move_files_to_cd) (struct s_bkpinfo *, char *, ...) =
	_move_files_to_cd;

/**
 * Move some files to the ISO scratch directory.
 * This function moves files specified as parameters, into the directory
 * @c bkpinfo->scratchdir, where the files that will be stored on the next
 * CD are waiting.
 *
 * @param bkpinfo The backup information structure. Fields used:
 * - @c media_size
 * - @c scratchdir
 * @param files_to_add The files to add to the scratchdir.
 * @warning The list of @c files_to_add must be terminated with @c NULL.
 * @note If and when the space occupied by the scratchdir would exceed
 * the capacity of the current CD,
 * <tt>write_iso_and_go_on(bkpinfo, FALSE)</tt> is called and the
 * scratchdir is emptied.
 *
 * @return The number of errors encountered (0 for success)
 */
int _move_files_to_cd(struct s_bkpinfo *bkpinfo, char *files_to_add, ...)
{

	/*@ int ************************************************************ */
	int retval = 0;
	int res = 0;

	/*@ buffers ******************************************************** */
	char *tmp, *curr_file, *cf;

	/*@ long ************************************************************ */
	va_list ap;
	long long would_occupy;

	assert(bkpinfo != NULL);
	malloc_string(curr_file);
	tmp = malloc(1200);
	would_occupy = space_occupied_by_cd(bkpinfo->scratchdir);
	va_start(ap, files_to_add);	// initialize the variable arguments
	for (cf = files_to_add; cf != NULL; cf = va_arg(ap, char *)) {
		if (!cf) {
			continue;
		}
		strcpy(curr_file, cf);
		if (!does_file_exist(curr_file)) {
			log_msg(1,
					"Warning - you're trying to add a non-existent file - '%s' to the CD",
					curr_file);
		} else {
			log_msg(8, "Trying to add file %s to CD", curr_file);
			would_occupy += length_of_file(curr_file) / 1024;
		}
	}
	va_end(ap);

	if (bkpinfo->media_size[g_current_media_number] <= 0) {
		fatal_error("move_files_to_cd() - unknown media size");
	}
	if (would_occupy / 1024 > bkpinfo->media_size[g_current_media_number]) {
		res = write_iso_and_go_on(bkpinfo, FALSE);	/* FALSE because this is not the last CD we'll write */
		retval += res;
		if (res) {
			log_msg(1, "WARNING - write_iso_and_go_on returned an error");
		}
	}

	va_start(ap, files_to_add);	// initialize the variable arguments
	for (cf = files_to_add; cf != NULL; cf = va_arg(ap, char *)) {
		if (!cf) {
			continue;
		}
		strcpy(curr_file, cf);

		sprintf(tmp, "mv -f %s %s/archives/", curr_file,
				bkpinfo->scratchdir);
		res = run_program_and_log_output(tmp, 5);
		retval += res;
		if (res) {
			log_msg(1, "(move_files_to_cd) '%s' failed", tmp);
		} else {
			log_msg(8, "Moved %s to CD OK", tmp);
		}
		//      unlink (curr_file);
	}
	va_end(ap);

	if (retval) {
		log_msg(1,
				"Warning - errors occurred while I was adding files to CD dir");
	}
	paranoid_free(tmp);
	paranoid_free(curr_file);
	return (retval);
}

/* @} - end of LLarchiveGroup */








/**
 * Offer to write boot and data disk images to 3.5" floppy disks.
 * @param bkpinfo The backup information structure. Only the
 * @c backup_media_type field is used in this function.
 * @param imagesdir The directory containing the floppy images (usually
 * /root/images/mindi).
 *
 * @return The number of errors encountered (0 for success)
 * @see write_image_to_floppy
 * @see format_disk
 * @ingroup MLarchiveGroup
 */
int offer_to_write_floppies(struct s_bkpinfo *bkpinfo, char *imagesdir)
{
	/*@ buffer ************************************************************ */
	char *tmp;
	char *comment;
	char *bootdisk_dev;
	char *datadisk_dev;
	char *bootdisk_file;
	char *rootdisk_file;

	/*@ int *************************************************************** */
	int i = 0;
	int res = 0;

	/*@ bool ************************************************************** */
	bool format_first;
	bool root_disk_exists = FALSE;

	malloc_string(tmp);
	malloc_string(comment);
	malloc_string(bootdisk_dev);
	malloc_string(datadisk_dev);
	malloc_string(rootdisk_file);
	malloc_string(bootdisk_file);
	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(imagesdir);


	if (!ask_me_yes_or_no
		("Write boot and data disk images to 3.5\" floppy disks?")) {
		return (0);
	}
	if (does_device_exist(DEFAULT_1722MB_DISK)) {
#ifdef __FreeBSD__
		// tell the system that this is a 1.72m floppy
		system("/usr/sbin/fdcontrol -F 1722 /dev/fd0.1722");
#endif
		strcpy(bootdisk_dev, DEFAULT_1722MB_DISK);
	} else if (does_device_exist(BACKUP_1722MB_DISK)) {
		sprintf(bootdisk_dev, "/dev/fd0H1722");
	} else {
		log_msg(1, "Warning - can't find a 1.72MB floppy device *sigh*");
		strcpy(bootdisk_dev, DEFAULT_1722MB_DISK);
//      return (1);
	}
	strcpy(datadisk_dev, "/dev/fd0");
	if (!does_device_exist(datadisk_dev)) {
		log_msg(1, "Warning - can't find a 1.44MB floppy device *sigh*");
		strcpy(datadisk_dev, "/dev/fd0");
//      return (1);
	}
	format_first =
		ask_me_yes_or_no
		("Do you want me to format the disks before I write to them?");

/* boot disk */
	if (ask_me_OK_or_cancel("About to write boot disk")) {
		log_to_screen("Writing boot floppy");
#ifdef __FreeBSD__
		sprintf(tmp, "%s/mindi-kern.1722.img", imagesdir);
		if (format_first) {
			format_disk(bootdisk_dev);
		}
		res += write_image_to_floppy(bootdisk_dev, tmp);
		if (ask_me_OK_or_cancel("About to write 1.44MB mfsroot disk")) {
			log_to_screen("Writing mfsroot floppy");
			if (format_first) {
				format_disk(datadisk_dev);
			}
			sprintf(tmp, "%s/mindi-mfsroot.1440.img", imagesdir);
			write_image_to_floppy(datadisk_dev, tmp);
		}
#else
		sprintf(bootdisk_file, "%s/mindi-bootroot.1722.img", imagesdir);
		if (does_file_exist(bootdisk_file)) {
			if (format_first) {
				format_disk(bootdisk_dev);
			}
			res += write_image_to_floppy(bootdisk_dev, bootdisk_file);
		} else {
			sprintf(bootdisk_file, "%s/mindi-boot.1440.img", imagesdir);
			sprintf(rootdisk_file, "%s/mindi-root.1440.img", imagesdir);
			root_disk_exists = TRUE;
			if (!does_file_exist(rootdisk_file)
				|| !does_file_exist(bootdisk_file)) {
				popup_and_OK
					("Cannot write boot/root floppies. Files not found.");
				log_to_screen
					("Failed to find boot/root floppy images. Oh dear.");
				return (1);
			}
			if (format_first) {
				format_disk(datadisk_dev);
			}
			/*
			   sprintf(tmp, "cat %s > %s", bootdisk_file, datadisk_dev);
			   res += run_external_binary_with_percentage_indicator_NEW("Writing boot floppy", tmp);
			 */
			res += write_image_to_floppy(datadisk_dev, bootdisk_file);
			if (ask_me_OK_or_cancel("About to write root disk")) {
				log_to_screen("Writing root floppy");
				if (format_first) {
					format_disk(datadisk_dev);
				}
				sprintf(tmp, "cat %s > %s", rootdisk_file, datadisk_dev);
				log_msg(1, "tmp = '%s'", tmp);
				res +=
					run_external_binary_with_percentage_indicator_NEW
					("Writing root floppy", tmp);
//              res += write_image_to_floppy (datadisk_dev, rootdisk_file);
			}
		}
#endif
	}
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		log_to_screen
			("FYI, the data disks are stored on tape/CD for your convenience.");
		return (0);
	}
	for (i = 1; i < 99; i++) {
		sprintf(tmp, "%s/mindi-data-%d.img", imagesdir, i);
		log_msg(3, tmp);
		if (!does_file_exist(tmp)) {
			log_msg(3, "...not found");
			break;
		}
		sprintf(comment, "About to write data disk #%d", i);
		if (ask_me_OK_or_cancel(comment)) {
			sprintf(comment, "Writing data disk #%3d", i);
			log_to_screen(comment);
			if (format_first) {
				res += format_disk(datadisk_dev);
			}
			res += write_image_to_floppy(datadisk_dev, tmp);
		}
	}
	paranoid_free(tmp);
	paranoid_free(comment);
	paranoid_free(bootdisk_dev);
	paranoid_free(datadisk_dev);
	return (res);
}








/**
 * Wrapper around @c offer_to_write_floppies().
 * @param bkpinfo The backup information structure. Used only
 * in the call to @c offer_to_write_floppies().
 * @return 0 if the boot floppies were found (not necessarily written OK),
 * 1 otherwise.
 * @see offer_to_write_floppies
 * @ingroup MLarchiveGroup
 */

int offer_to_write_boot_floppies_to_physical_disks(struct s_bkpinfo
												   *bkpinfo)
{
	int res = 0;

	assert(bkpinfo != NULL);

	mvaddstr_and_log_it(g_currentY, 0,
						"Writing boot+data floppy images to disk");

	if (!bkpinfo->nonbootable_backup) {
#ifdef __FreeBSD__
		if (!does_file_exist("/root/images/mindi/mindi-kern.1722.img"))
#else
		if (!does_file_exist("/root/images/mindi/mindi-bootroot.1722.img")
			&& !does_file_exist("/root/images/mindi/mindi-boot.1440.img"))
#endif
		{
			mvaddstr_and_log_it(g_currentY++, 74, "No Imgs");
			if (does_file_exist("/root/images/mindi/mondorescue.iso")) {
				popup_and_OK
					("Boot+data floppy creation failed. However, FYI, you may burn /root/images/mindi/mondorescue.iso to a CD and boot from that instead if you wish.");
				res++;
			}
		} else {
			offer_to_write_floppies(bkpinfo, "/root/images/mindi");
			mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		}
	} else {
		popup_and_OK
			("Since you opted for a nonbootable backup, no boot floppies were created.");
	}

	return (res);
}




/**
 * @addtogroup LLarchiveGroup
 * @{
 */
/**
 * Function pointer to an appropriate @c move_files_to_stream routine.
 * You can set this to your own function (for example, one to
 * transfer files over the network) or leave it as is.
 */
int (*move_files_to_stream) (struct s_bkpinfo *, char *, ...) =
	_move_files_to_stream;

/**
 * Copy some files to tape.
 * This function copies the files specified as parameters into the tape stream.
 *
 * @param bkpinfo The backup information structure. Used only in the call to
 * @c write_file_to_stream_from_file().
 *
 * @param files_to_add The files to copy to the tape stream.
 * @warning The list of @c files_to_add must be terminated with @c NULL.
 * @note Files may be split across multiple tapes if necessary.
 *
 * @return The number of errors encountered (0 for success)
 */
int
_move_files_to_stream(struct s_bkpinfo *bkpinfo, char *files_to_add, ...)
{

	/*@ int ************************************************************ */
	int retval = 0;
	int res = 0;
	/*@ buffers ******************************************************** */

	/*@ char *********************************************************** */
	char start_chr;
	char stop_chr;
	char *curr_file, *cf;
	/*@ long long ****************************************************** */
	long long length_of_incoming_file = 0;
	t_archtype type;
	va_list ap;

	assert(bkpinfo != NULL);
	malloc_string(curr_file);
	va_start(ap, files_to_add);
	for (cf = files_to_add; cf != NULL; cf = va_arg(ap, char *)) {
		if (!cf) {
			continue;
		}
		strcpy(curr_file, cf);
		if (!does_file_exist(curr_file)) {
			log_msg(1,
					"Warning - you're trying to add a non-existent file - '%s' to the tape",
					curr_file);
		}
/* create header chars */
		start_chr = BLK_START_AN_AFIO_OR_SLICE;
		stop_chr = BLK_STOP_AN_AFIO_OR_SLICE;
/* ask for new tape if necessary */
		length_of_incoming_file = length_of_file(curr_file);
		write_header_block_to_stream(length_of_incoming_file, curr_file,
									 start_chr);
		if (strstr(curr_file, ".afio.") || strstr(curr_file, ".star.")) {
			type = fileset;
		} else if (strstr(curr_file, "slice")) {
			type = biggieslice;
		} else {
			type = other;
		}
		res = write_file_to_stream_from_file(bkpinfo, curr_file);
		retval += res;
		unlink(curr_file);
/* write closing header */
		write_header_block_to_stream(0, "finished-writing-file", stop_chr);
	}
	va_end(ap);

	if (retval) {
		log_msg(1,
				"Warning - errors occurred while I was adding file to tape");
	}
	paranoid_free(curr_file);
	return (retval);
}

/* @} - end of LLarchiveGroup */



/**
 * @addtogroup utilityGroup
 * @{
 */
/**
 * Make sure the user has a valid CD-R(W) in the CD drive.
 * @param cdrw_dev Set to the CD-R(W) device checked.
 * @param keep_looping If TRUE, keep pestering user until they insist
 * or insert a correct CD; if FALSE, only check once.
 * @return 0 (there was an OK CD in the drive) or 1 (there wasn't).
 */
int interrogate_disk_currently_in_cdrw_drive(char *cdrw_dev,
											 bool keep_looping)
{
	char *tmp;
	int res = 0;
	char *bkp;
	char *cdrecord;

	malloc_string(tmp);
	malloc_string(bkp);
	malloc_string(cdrecord);
	strcpy(bkp, cdrw_dev);
	if (find_cdrw_device(cdrw_dev)) {
		strcpy(cdrw_dev, bkp);
	} else {
		if (!system("which cdrecord > /dev/null 2> /dev/null")) {
			sprintf(cdrecord, "cdrecord dev=%s -atip", cdrw_dev);
		} else if (!system("which dvdrecord > /dev/null 2> /dev/null")) {
			sprintf(cdrecord, "cdrecord dev=%s -atip", cdrw_dev);
		} else {
			cdrecord[0] = '\0';
			log_msg(2, "Oh well. I guess I'll just pray then.");
		}
		if (cdrecord[0]) {
			if (!keep_looping) {
				retract_CD_tray_and_defeat_autorun();
				res = run_program_and_log_output(cdrecord, 5);
			} else {
				while ((res = run_program_and_log_output(cdrecord, 5))) {
					retract_CD_tray_and_defeat_autorun();
					if (ask_me_yes_or_no
						("Unable to examine CD. Are you sure this is a valid CD-R(W) CD?"))
					{
						log_msg(1, "Well, he insisted...");
						break;
					}
				}
			}
		}
	}
//  retract_CD_tray_and_defeat_autorun();
	paranoid_free(tmp);
	paranoid_free(cdrecord);
	paranoid_free(bkp);
	return (res);
}









/**
 * Asks the user to put a CD-R(W) in the drive.
 * @param ask_for_one_if_more_than_this (unused)
 * @param pmountable If non-NULL, pointed-to value is set to TRUE if the CD is mountable, FALSE otherwise.
 */
void
pause_and_ask_for_cdr(int ask_for_one_if_more_than_this, bool * pmountable)
{

	/*@ buffers ********************************************* */
	char *tmp;
	char *szmsg;
	char *cdrom_dev;
	char *cdrw_dev;
	char *our_serial_str;
	bool ok_go_ahead_burn_it;
	int cd_number = -1;
	int attempt_to_mount_returned_this = 999;
	char *mtpt;
	char *szcdno;
	char *szserfname;
	char *szunmount;

	malloc_string(tmp);
	malloc_string(szmsg);
	malloc_string(cdrom_dev);
	malloc_string(cdrw_dev);
	malloc_string(mtpt);
	malloc_string(szcdno);
	malloc_string(szserfname);
	malloc_string(our_serial_str);
	malloc_string(szunmount);

	sprintf(szmsg, "I am about to burn %s #%d",
			media_descriptor_string(g_backup_media_type),
			g_current_media_number);
	log_to_screen(szmsg);
	if (g_current_media_number < ask_for_one_if_more_than_this) {
		return;
	}
	log_to_screen("Scanning CD-ROM drive...");
	sprintf(mtpt, "/tmp/cd.mtpt.%ld.%ld", (long int) random(),
			(long int) random());
	make_hole_for_dir(mtpt);

  gotos_make_me_puke:
	ok_go_ahead_burn_it = TRUE;
	if (!find_cdrom_device(cdrom_dev, FALSE)) {
/* When enabled, it made CD eject-and-retract when wrong CD inserted.. Weird
      log_msg(2, "paafcd: Retracting CD-ROM drive if possible" );
      retract_CD_tray_and_defeat_autorun();
*/
		sprintf(tmp, "umount %s", cdrom_dev);
		run_program_and_log_output(tmp, 1);
		sprintf(szcdno, "%s/archives/THIS-CD-NUMBER", mtpt);
		sprintf(szserfname, "%s/archives/SERIAL-STRING", mtpt);
		sprintf(szunmount, "umount %s", mtpt);
		cd_number = -1;
		our_serial_str[0] = '\0';
		sprintf(tmp, "mount %s %s", cdrom_dev, mtpt);
		if ((attempt_to_mount_returned_this =
			 run_program_and_log_output(tmp, 1))) {
			log_msg(4, "Failed to mount %s at %s", cdrom_dev, mtpt);
			log_to_screen("If there's a CD/DVD in the drive, it's blank.");
			/*
			   if (interrogate_disk_currently_in_cdrw_drive(cdrw_dev, FALSE))
			   {
			   ok_go_ahead_burn_it = FALSE;
			   log_to_screen("There isn't a writable CD/DVD in the drive.");
			   }
			   else
			   {
			   log_to_screen("Confirmed. There is a blank CD/DVD in the drive.");
			   }
			 */
		} else if (!does_file_exist(szcdno)
				   || !does_file_exist(szserfname)) {
			log_to_screen
				("%s has data on it but it's probably not a Mondo CD.",
				 media_descriptor_string(g_backup_media_type));
		} else {
			log_to_screen("%s found in drive. It's a Mondo disk.",
						  media_descriptor_string(g_backup_media_type));
			cd_number = atoi(last_line_of_file(szcdno));
			sprintf(tmp, "cat %s 2> /dev/null", szserfname);
			strcpy(our_serial_str,
				   call_program_and_get_last_line_of_output(tmp));
			// FIXME - should be able to use last_line_of_file(), surely?
		}
		run_program_and_log_output(szunmount, 1);
		log_msg(2, "paafcd: cd_number = %d", cd_number);
		log_msg(2, "our serial str = %s; g_serial_string = %s",
				our_serial_str, g_serial_string);
		if (cd_number > 0 && !strcmp(our_serial_str, g_serial_string)) {
			log_msg(2, "This %s is part of this backup set!",
					media_descriptor_string(g_backup_media_type));
			ok_go_ahead_burn_it = FALSE;
			if (cd_number == g_current_media_number - 1) {
				log_to_screen
					("I think you've left the previous %s in the drive.",
					 media_descriptor_string(g_backup_media_type));
			} else {
				log_to_screen
					("Please remove this %s. It is part of the backup set you're making now.",
					 media_descriptor_string(g_backup_media_type));
			}
		} else {
			log_to_screen("...but not part of _our_ backup set.");
		}
	} else {
		log_msg(2,
				"paafcd: Can't find CD-ROM drive. Perhaps it has a blank %s in it?",
				media_descriptor_string(g_backup_media_type));
		if (interrogate_disk_currently_in_cdrw_drive(cdrw_dev, FALSE)) {
			ok_go_ahead_burn_it = FALSE;
			log_to_screen("There isn't a writable %s in the drive.",
						  media_descriptor_string(g_backup_media_type));
		}
	}

/*
  if (g_current_media_number > ask_for_one_if_more_than_this)
    {
      ok_go_ahead_burn_it = FALSE;
      log_it("paafcd: %d > %d, so I'll definitely pause.", g_current_media_number > ask_for_one_if_more_than_this);
    }
*/

	if (!ok_go_ahead_burn_it) {
		eject_device(cdrom_dev);
		sprintf(tmp,
				"I am about to burn %s #%d of the backup set. Please insert %s and press Enter.",
				media_descriptor_string(g_backup_media_type),
				g_current_media_number,
				media_descriptor_string(g_backup_media_type));
		popup_and_OK(tmp);
		goto gotos_make_me_puke;
	} else {
		log_msg(2, "paafcd: OK, going ahead and burning it.");
	}

	log_msg(2,
			"paafcd: OK, I assume I have a blank/reusable %s in the drive...",
			media_descriptor_string(g_backup_media_type));

	//  if (ask_for_one_if_more_than_this>1) { popup_and_OK(szmsg); }

	log_to_screen("Proceeding w/ %s in drive.",
				  media_descriptor_string(g_backup_media_type));
	paranoid_free(tmp);
	paranoid_free(szmsg);
	paranoid_free(cdrom_dev);
	paranoid_free(cdrw_dev);
	paranoid_free(mtpt);
	paranoid_free(szcdno);
	paranoid_free(szserfname);
	paranoid_free(our_serial_str);
	paranoid_free(szunmount);
	if (pmountable) {
		if (attempt_to_mount_returned_this) {
			*pmountable = FALSE;
		} else {
			*pmountable = TRUE;
		}
	}

}








/**
 * Set the <tt>N</tt>th bit of @c array to @c true_or_false.
 * @param array The bit array (as a @c char pointer).
 * @param N The bit number to set or reset.
 * @param true_or_false If TRUE then set bit @c N, if FALSE then reset bit @c N.
 * @see get_bit_N_of_array
 */
void set_bit_N_of_array(char *array, int N, bool true_or_false)
{
	int bit_number;
	int mask, orig_val, to_add;
	int element_number;

	assert(array != NULL);

	element_number = N / 8;
	bit_number = N % 8;
	to_add = (1 << bit_number);
	mask = 255 - to_add;
	orig_val = array[element_number] & mask;
	//  log_it("array[%d]=%02x; %02x&%02x = %02x", element_number, array[element_number], mask, orig_val);
	if (true_or_false) {
		array[element_number] = orig_val | to_add;
	}
}

/* @} - end of utilityGroup */








/**
 * Chop up @c filename.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c compression_level
 * - @c optimal_set_size
 * - @c tmpdir
 * - @c use_lzo
 * - @c zip_exe
 * - @c zip_suffix
 *
 * @param biggie_filename The file to chop up.
 * @param ntfsprog_fifo The FIFO to ntfsclone if this is an imagedev, NULL otherwise.
 * @param biggie_file_number The sequence number of this biggie file (starting from 0).
 * @param noof_biggie_files The number of biggie files there are total.
 * @return The number of errors encountered (0 for success)
 * @see make_slices_and_images
 * @ingroup LLarchiveGroup
 */
int
slice_up_file_etc(struct s_bkpinfo *bkpinfo, char *biggie_filename,
				  char *ntfsprog_fifo, long biggie_file_number,
				  long noof_biggie_files, bool use_ntfsprog)
{

	/*@ buffers ************************************************** */
	char *tmp, *checksum_line, *command;
	char *tempblock;
	char *curr_slice_fname_uncompressed;
	char *curr_slice_fname_compressed;
	char *file_to_archive;
	char *file_to_openin;
	/*@ pointers ************************************************** */
	char *pB;
	FILE *fin, *fout;

	/*@ bool ****************************************************** */
	bool finished = FALSE;

	/*@ long ****************************************************** */
	size_t blksize = 0;
	long slice_num = 0;
	long i;
	long optimal_set_size;
	bool should_I_compress_slices;
	char *suffix;				// for compressed slices

	/*@ long long ************************************************** */
	long long totalread = 0;
	long long totallength = 0;
	long long length;

	/*@ int ******************************************************** */
	int retval = 0;
	int res = 0;

	/*@ structures ************************************************** */
	struct s_filename_and_lstat_info biggiestruct;
//  struct stat statbuf;

	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(biggie_filename);
	malloc_string(tmp);
	malloc_string(checksum_line);
	malloc_string(curr_slice_fname_uncompressed);
	malloc_string(curr_slice_fname_compressed);
	malloc_string(file_to_archive);
	malloc_string(suffix);
	command = malloc(MAX_STR_LEN * 8);

	biggiestruct.for_backward_compatibility = '\n';
	biggiestruct.use_ntfsprog = use_ntfsprog;
	if (!(tempblock = (char *) malloc(256 * 1024))) {
		fatal_error("malloc error 256*1024");
	}
	optimal_set_size = bkpinfo->optimal_set_size;
	if (is_this_file_compressed(biggie_filename)
		|| bkpinfo->compression_level == 0) {
		suffix[0] = '\0';
		//      log_it("%s is indeed compressed :-)", filename);
		should_I_compress_slices = FALSE;
	} else {
		strcpy(suffix, bkpinfo->zip_suffix);
		should_I_compress_slices = TRUE;
	}

	if (optimal_set_size < 999) {
		fatal_error("bkpinfo->optimal_set_size is insanely small");
	}
	if (ntfsprog_fifo) {
		file_to_openin = ntfsprog_fifo;
		strcpy(checksum_line, "IGNORE");
		log_msg(2,
				"Not calculating checksum for %s: it would take too long",
				biggie_filename);
		if ( !find_home_of_exe("ntfsresize")) {
			fatal_error("ntfsresize not found");
		}
		sprintf(command, "ntfsresize --force --info %s|grep '^You might resize at '|cut -d' ' -f5", biggie_filename);
		log_it("command = %s", command);
		strcpy (tmp, call_program_and_get_last_line_of_output(command));
		log_it("res of it = %s", tmp); 
		totallength = atoll(tmp);
	} else {
		file_to_openin = biggie_filename;
		sprintf(command, "md5sum '%s'", biggie_filename);
		if (!(fin = popen(command, "r"))) {
			log_OS_error("Unable to popen-in command");
			return (1);
		}
		(void) fgets(checksum_line, MAX_STR_LEN, fin);
		pclose(fin);
		totallength = length_of_file (biggie_filename);
	}
	lstat(biggie_filename, &biggiestruct.properties);
	strcpy(biggiestruct.filename, biggie_filename);
	pB = strchr(checksum_line, ' ');
	if (!pB) {
		pB = strchr(checksum_line, '\t');
	}
	if (pB) {
		*pB = '\0';
	}
	strcpy(biggiestruct.checksum, checksum_line);

	strcpy(tmp, slice_fname(biggie_file_number, 0, bkpinfo->tmpdir, ""));
	fout = fopen(tmp, "w");
	(void) fwrite((void *) &biggiestruct, 1, sizeof(biggiestruct), fout);
	paranoid_fclose(fout);
	length = totallength / optimal_set_size / 1024;
	log_msg(1, "Opening in %s; slicing it and writing to CD/tape",
			file_to_openin);
	if (!(fin = fopen(file_to_openin, "r"))) {
		log_OS_error("Unable to openin biggie_filename");
		sprintf(tmp, "Cannot archive bigfile '%s': not found",
				biggie_filename);
		log_to_screen(tmp);
		paranoid_free(tempblock);
		paranoid_free(tmp);
		paranoid_free(checksum_line);
		paranoid_free(command);
		return (1);
	}
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		res =
			move_files_to_stream(bkpinfo,
								 slice_fname(biggie_file_number, 0,
											 bkpinfo->tmpdir, ""), NULL);
	} else {
		res =
			move_files_to_cd(bkpinfo,
							 slice_fname(biggie_file_number, 0,
										 bkpinfo->tmpdir, ""), NULL);
	}
	i = bkpinfo->optimal_set_size / 256;
	for (slice_num = 1; !finished; slice_num++) {
		strcpy(curr_slice_fname_uncompressed,
			   slice_fname(biggie_file_number, slice_num, bkpinfo->tmpdir,
						   ""));
		strcpy(curr_slice_fname_compressed,
			   slice_fname(biggie_file_number, slice_num, bkpinfo->tmpdir,
						   suffix));

		strcpy(tmp, percent_media_full_comment(bkpinfo));
		update_progress_form(tmp);
		if (!(fout = fopen(curr_slice_fname_uncompressed, "w"))) {
			log_OS_error(curr_slice_fname_uncompressed);
			return (1);
		}
		if ((i == bkpinfo->optimal_set_size / 256)
			&& (totalread < 1.1 * totallength)) {
			for (i = 0; i < bkpinfo->optimal_set_size / 256; i++) {
				blksize = fread(tempblock, 1, 256 * 1024, fin);
				if (blksize > 0) {
					totalread = totalread + blksize;
					(void) fwrite(tempblock, 1, blksize, fout);
				} else {
					break;
				}
			}
		} else {
			i = 0;
		}
		paranoid_fclose(fout);
		if (i > 0)				// length_of_file (curr_slice_fname_uncompressed)
		{
			if (!does_file_exist(curr_slice_fname_uncompressed)) {
				log_msg(2,
						"Warning - '%s' doesn't exist. How can I compress slice?",
						curr_slice_fname_uncompressed);
			}
			if (should_I_compress_slices && bkpinfo->compression_level > 0) {
				sprintf(command, "%s -%d %s", bkpinfo->zip_exe,
						bkpinfo->compression_level,
						curr_slice_fname_uncompressed);
				log_msg(2, command);
				if ((res = system(command))) {
					log_OS_error(command);
				}
				//          did_I_compress_slice = TRUE;
			} else {
				sprintf(command, "mv %s %s 2>> %s",
						curr_slice_fname_uncompressed,
						curr_slice_fname_compressed, MONDO_LOGFILE);
				res = 0;		// don't do it :)
				//          did_I_compress_slice = FALSE;
			}
			retval += res;
			if (res) {
				log_msg(2, "Failed to compress the slice");
			}
			if (bkpinfo->use_lzo
				&& strcmp(curr_slice_fname_compressed,
						  curr_slice_fname_uncompressed)) {
				unlink(curr_slice_fname_uncompressed);
			}
			if (res) {
				sprintf(tmp, "Problem with slice # %ld", slice_num);
			} else {
				sprintf(tmp,
						"%s - Bigfile #%ld, slice #%ld compressed OK          ",
						biggie_filename, biggie_file_number + 1,
						slice_num);
			}
#ifndef _XWIN
			if (!g_text_mode) {
				newtDrawRootText(0, g_noof_rows - 2, tmp);
				newtRefresh();
			} else {
				log_msg(2, tmp);
			}
#else
			log_msg(2, tmp);
#endif
			strcpy(file_to_archive, curr_slice_fname_compressed);
			g_current_progress++;
		} else {				/* if i==0 then ... */

			finished = TRUE;
			strcpy(file_to_archive, curr_slice_fname_uncompressed);
			if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
				break;
			}
		}

		if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
			register_in_tape_catalog(biggieslice, biggie_file_number,
									 slice_num, file_to_archive);
			maintain_collection_of_recent_archives(bkpinfo->tmpdir,
												   file_to_archive);
			res = move_files_to_stream(bkpinfo, file_to_archive, NULL);
		} else {
			res = move_files_to_cd(bkpinfo, file_to_archive, NULL);
		}
		retval += res;
		if (res) {
			sprintf(tmp,
					"Failed to add slice %ld of bigfile %ld to scratchdir",
					slice_num, biggie_file_number + 1);
			log_to_screen(tmp);
			fatal_error
				("Hard disk full. You should have bought a bigger one.");
		}
	}
	paranoid_fclose(fin);
	sprintf(tmp, "Sliced bigfile #%ld", biggie_file_number + 1);
	if (retval) {
		strcat(tmp, "...FAILED");
	} else {
		strcat(tmp, "...OK!");
	}
	log_msg(1, tmp);
	paranoid_free(tempblock);
	paranoid_free(tmp);
	paranoid_free(checksum_line);
	paranoid_free(command);
	paranoid_free(curr_slice_fname_uncompressed);
	paranoid_free(curr_slice_fname_compressed);
	paranoid_free(file_to_archive);
	paranoid_free(suffix);
	return (retval);
}






/**
 * Remove the archives in @c d.
 * This could possibly include any of:
 * - all afioballs (compressed and not)
 * - all filelists
 * - all slices
 * - all checksums
 * - a zero filler file
 *
 * @param d The directory to wipe the archives from.
 * @ingroup utilityGroup
 */
void wipe_archives(char *d)
{
	/*@ buffers ********************************************* */
	char *tmp;
	char *dir;

	malloc_string(tmp);
	malloc_string(dir);
	assert_string_is_neither_NULL_nor_zerolength(d);

	sprintf(dir, "%s/archives", d);
	sprintf(tmp, "find %s -name '*.afio*' -exec rm -f '{}' \\;", dir);
	run_program_and_log_output(tmp, FALSE);
	sprintf(tmp, "find %s -name '*list.[0-9]*' -exec rm -f '{}' \\;", dir);
	run_program_and_log_output(tmp, FALSE);
	sprintf(tmp, "find %s -name 'slice*' -exec rm -f '{}' \\;", dir);
	run_program_and_log_output(tmp, FALSE);
	sprintf(tmp, "rm -f %s/cklist*", dir);
	run_program_and_log_output(tmp, FALSE);
	sprintf(tmp, "rm -f %s/zero", dir);
	run_program_and_log_output(tmp, FALSE);
	log_msg(1, "Wiped %s's archives", dir);
	sprintf(tmp, "ls -l %s", dir);
	run_program_and_log_output(tmp, FALSE);
	paranoid_free(tmp);
	paranoid_free(dir);
}



/**
 * @addtogroup LLarchiveGroup
 * @{
 */
/**
 * Write the final ISO image.
 * @param bkpinfo The backup information structure. Used only
 * in the call to @c write_iso_and_go_on().
 * @return The number of errors encountered (0 for success)
 * @see write_iso_and_go_on
 * @see make_iso_fs
 * @bug The final ISO is written even if there are no files on it. In practice,
 * however, this occurs rarely.
 */
int write_final_iso_if_necessary(struct s_bkpinfo *bkpinfo)
{
	/*@ int ***************************************************** */
	int res;

	/*@ buffers ************************************************** */
	char *tmp;

	malloc_string(tmp);
	assert(bkpinfo != NULL);

// I should really check if there are any slices or tarballs to be copied to CD-R(W)'s; the odds are approx. 1 in a million that there are no files here, so I'll just go ahead & make one more CD anyway

	sprintf(tmp, "Writing the final ISO");
	log_msg(2, tmp);
	center_string(tmp, 80);
#ifndef _XWIN
	if (!g_text_mode) {
		newtPushHelpLine(tmp);
	}
#endif
	res = write_iso_and_go_on(bkpinfo, TRUE);
#ifndef _XWIN
	if (!g_text_mode) {
		newtPopHelpLine();
	}
#endif
	log_msg(2, "Returning from writing final ISO (res=%d)", res);
	paranoid_free(tmp);
	return (res);
}


/**
 * Write an ISO image to <tt>[bkpinfo->isodir]/bkpinfo->prefix-[g_current_media_number].iso</tt>.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_media_type
 * - @c prefix
 * - @c isodir
 * - @c manual_cd_tray
 * - @c media_size
 * - @c nfs_mount
 * - @c nfs_remote_dir
 * - @c scratchdir
 * - @c verify_data
 *
 * @param last_cd If TRUE, this is the last CD to write; if FALSE, it's not.
 * @return The number of errors encountered (0 for success)
 * @see make_iso_fs
 */
int write_iso_and_go_on(struct s_bkpinfo *bkpinfo, bool last_cd)
{
	/*@ pointers **************************************************** */
	FILE *fout;

	/*@ buffers ***************************************************** */
	char *tmp;
	char *cdno_fname;
	char *lastcd_fname;
	char *isofile;

	/*@ bool ******************************************************** */
	bool that_one_was_ok;
	bool using_nfs;
	bool orig_vfy_flag_val;

	/*@ int *********************************************************** */
	int res = 0;

	malloc_string(tmp);
	malloc_string(cdno_fname);
	malloc_string(lastcd_fname);
	malloc_string(isofile);

	assert(bkpinfo != NULL);
	orig_vfy_flag_val = bkpinfo->verify_data;
	if (bkpinfo->media_size[g_current_media_number] <= 0) {
		fatal_error("write_iso_and_go_on() - unknown media size");
	}

	if (strlen(bkpinfo->nfs_mount) > 1) {
		using_nfs = TRUE;
	} else {
		using_nfs = FALSE;
	}
	log_msg(1, "OK, time to make %s #%d",
			media_descriptor_string(bkpinfo->backup_media_type),
			g_current_media_number);

	/* label the ISO with its number */

	sprintf(cdno_fname, "%s/archives/THIS-CD-NUMBER", bkpinfo->scratchdir);
	fout = fopen(cdno_fname, "w");
	fprintf(fout, "%d", g_current_media_number);
	paranoid_fclose(fout);

	sprintf(tmp, "cp -f %s/autorun %s/", g_mondo_home,
			bkpinfo->scratchdir);
	if (run_program_and_log_output(tmp, FALSE)) {
		log_msg(2, "Warning - unable to copy autorun to scratchdir");
	}

	/* last CD or not? Label accordingly */
	sprintf(lastcd_fname, "%s/archives/NOT-THE-LAST", bkpinfo->scratchdir);
	if (last_cd) {
		unlink(lastcd_fname);
		log_msg(2,
				"OK, you're telling me this is the last CD. Fair enough.");
	} else {
		fout = fopen(lastcd_fname, "w");
		fprintf(fout,
				"You're listening to 90.3 WPLN, Nashville Public Radio.\n");
		paranoid_fclose(fout);
	}
	if (space_occupied_by_cd(bkpinfo->scratchdir) / 1024 >
		bkpinfo->media_size[g_current_media_number]) {
		sprintf(tmp,
				"Warning! CD is too big. It occupies %ld KB, which is more than the %ld KB allowed.",
				(long) space_occupied_by_cd(bkpinfo->scratchdir),
				(long) bkpinfo->media_size[g_current_media_number]);
		log_to_screen(tmp);
	}
	sprintf(isofile, "%s/%s/%s-%d.iso", bkpinfo->isodir,
			bkpinfo->nfs_remote_dir, bkpinfo->prefix,
			g_current_media_number);
	for (that_one_was_ok = FALSE; !that_one_was_ok;) {
		res = make_iso_fs(bkpinfo, isofile);
		if (g_current_media_number == 1 && !res
			&& (bkpinfo->backup_media_type == cdr
				|| bkpinfo->backup_media_type == cdrw)) {
			if (find_cdrom_device(tmp, FALSE))	// make sure find_cdrom_device() finds, records CD-R's loc
			{
				log_msg(3, "*Sigh* Mike, I hate your computer.");
				bkpinfo->manual_cd_tray = TRUE;
			}					// if it can't be found then force pausing
			else {
				log_msg(3, "Great. Found Mike's CD-ROM drive.");
			}
		}
		if (bkpinfo->verify_data && !res) {
			log_to_screen
				("Please reboot from the 1st %s in Compare Mode, as a precaution.",
				 media_descriptor_string(g_backup_media_type));
			chdir("/");
			iamhere("Before calling verify_cd_image()");
			res += verify_cd_image(bkpinfo);
			iamhere("After calling verify_cd_image()");
		}
		if (!res) {
			that_one_was_ok = TRUE;
		} else {
			sprintf(tmp, "Failed to burn %s #%d. Retry?",
					media_descriptor_string(bkpinfo->backup_media_type),
					g_current_media_number);
			res = ask_me_yes_or_no(tmp);
			if (!res) {
				if (ask_me_yes_or_no("Abort the backup?")) {
					fatal_error("FAILED TO BACKUP");
				} else {
					break;
				}
			} else {
				log_msg(2, "Retrying, at user's request...");
				res = 0;
			}
		}
	}
/*
  if (using_nfs)
    {
      sprintf(tmp,"mv -f %s %s/%s/", isofile, bkpinfo->isodir, bkpinfo->nfs_remote_dir);
      if (run_program_and_log_output(tmp, FALSE))
        { log_to_screen("Unable to move ISO to NFS dir"); }
    }
*/
	g_current_media_number++;
	if (g_current_media_number > MAX_NOOF_MEDIA) {
		fatal_error("Too many CD-R(W)'s. Use tape or net.");
	}
	wipe_archives(bkpinfo->scratchdir);
	sprintf(tmp, "rm -Rf %s/images/*gz %s/images/*data*img",
			bkpinfo->scratchdir, bkpinfo->scratchdir);
	if (system(tmp)) {
		log_msg
			(2,
			 "Error occurred when I tried to delete the redundant IMGs and GZs");
	}

	if (last_cd) {
		log_msg(2, "This was your last CD.");
	} else {
		log_msg(2, "Continuing to backup your data...");
	}

	bkpinfo->verify_data = orig_vfy_flag_val;
	paranoid_free(tmp);
	paranoid_free(cdno_fname);
	paranoid_free(lastcd_fname);
	paranoid_free(isofile);
	return (0);
}

/* @} - end of LLarchiveGroup */




/**
 * Verify the user's data.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c backup_data
 * - @c backup_media_type
 * - @c media_device
 * - @c verify_data
 *
 * @return The number of errors encountered (0 for success)
 * @ingroup verifyGroup
 */
int verify_data(struct s_bkpinfo *bkpinfo)
{
	int res = 0, retval = 0, cdno = 0;
	char *tmp;
	long diffs = 0;

	malloc_string(tmp);
	assert(bkpinfo != NULL);
	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		chdir("/");
		mvaddstr_and_log_it(g_currentY, 0,
							"Verifying archives against live filesystem");
		if (bkpinfo->backup_media_type == cdstream) {
			strcpy(bkpinfo->media_device, "/dev/cdrom");
		}
		verify_tape_backups(bkpinfo);
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	} else if (bkpinfo->backup_data)
		//bkpinfo->backup_media_type == cdrw || bkpinfo->backup_media_type == cdr))
	{
		log_msg(2,
				"Not verifying again. Per-CD/ISO verification already carried out.");
		paranoid_system
			("cat /tmp/changed.files.* > /tmp/changed.files 2> /dev/null");
	} else {
		g_current_media_number = cdno;
		if (bkpinfo->backup_media_type != iso) {
			find_cdrom_device(bkpinfo->media_device, FALSE);	// replace 0,0,0 with /dev/cdrom
		}
		chdir("/");
		for (cdno = 1; cdno < 99 && bkpinfo->verify_data; cdno++) {
			if (cdno != g_current_media_number) {
				log_msg(2,
						"Warning - had to change g_current_media_number from %d to %d",
						g_current_media_number, cdno);
				g_current_media_number = cdno;
			}
			if (bkpinfo->backup_media_type != iso) {
				insist_on_this_cd_number(bkpinfo, cdno);
			}
			res = verify_cd_image(bkpinfo);	// sets verify_data to FALSE if it's time to stop verifying
			retval += res;
			if (res) {
				sprintf(tmp,
						"Warnings/errors were reported while checking %s #%d",
						media_descriptor_string(bkpinfo->
												backup_media_type),
						g_current_media_number);
				log_to_screen(tmp);

			}
		}
/*
	  sprintf (tmp,
		   "cat %s | grep \"afio: \" | cut -d'\"' -f2 | sort -u | awk '{print \"/\"$0;};' | tr -s '/' '/' | grep -vx \"/afio:.*\" > /tmp/changed.files",
		   MONDO_LOGFILE);
	  system (tmp);
*/
		sprintf(tmp,
				"grep 'afio: ' %s | sed 's/afio: //' | grep -vx '/dev/.*' >> /tmp/changed.files",
				MONDO_LOGFILE);
		system(tmp);

		sprintf(tmp,
				"grep 'star: ' %s | sed 's/star: //' | grep -vx '/dev/.*' >> /tmp/changed.files",
				MONDO_LOGFILE);
		system(tmp);
		run_program_and_log_output("umount " MNT_CDROM, FALSE);
//    if (bkpinfo->backup_media_type != iso && !bkpinfo->please_dont_eject_when_restoring)
//{
		eject_device(bkpinfo->media_device);
//}
	}
	diffs = count_lines_in_file("/tmp/changed.files");

	if (diffs > 0) {
		if (retval == 0) {
			retval = (int) (-diffs);
		}
	}
	paranoid_free(tmp);
	return (retval);
}





/**
 * @addtogroup utilityGroup
 * @{
 */
/**
 * Write an image to a real 3.5" floppy disk.
 * @param device The device to write to (e.g. @c /dev/fd0)
 * @param datafile The image to write to @p device.
 * @return The number of errors encountered (0 for success)
 * @see write_image_to_floppy
 */
int write_image_to_floppy_SUB(char *device, char *datafile)
{
	/*@ int *************************************************************** */
	int res = 0;
	int percentage = 0;
	int blockno = 0;
	int maxblocks = 0;

	/*@ buffers************************************************************ */
	char *tmp;
	char blk[1024];
	char *title;

	/*@ pointers ********************************************************** */
	char *p;
	FILE *fout, *fin;


	malloc_string(tmp);
	malloc_string(title);
	/* pretty stuff */
	if (!(p = strrchr(datafile, '/'))) {
		p = datafile;
	} else {
		p++;
	}
	sprintf(title, "Writing %s to floppy", p);
	open_evalcall_form(title);
	/* functional stuff */
	for (p = device + strlen(device); p != device && isdigit(*(p - 1));
		 p--);
	maxblocks = atoi(p);
	if (!maxblocks) {
		maxblocks = 1440;
	}
	sprintf(tmp, "maxblocks = %d; p=%s", maxblocks, p);
	log_msg(2, tmp);
	/* copy data from image to floppy */
	if (!(fin = fopen(datafile, "r"))) {
		log_OS_error("Cannot open img");
		return (1);
	}
	if (!(fout = fopen(device, "w"))) {
		log_OS_error("Cannot open fdd");
		return (1);
	}
	for (blockno = 0; blockno < maxblocks; blockno++) {
		percentage = blockno * 100 / maxblocks;
		if (fread(blk, 1, 1024, fin) != 1024) {
			if (feof(fin)) {
				log_msg(1,
						"img read err - img ended prematurely - non-fatal error");
				sleep(3);
				return (res);
			}
			res++;
			log_to_screen("img read err");
		}
		if (fwrite(blk, 1, 1024, fout) != 1024) {
			res++;
			log_to_screen("fdd write err");
		}
		if (((blockno + 1) % 128) == 0) {
			paranoid_system("sync");	/* fflush doesn't work; dunno why */
			update_evalcall_form(percentage);
		}
	}
	paranoid_fclose(fin);
	paranoid_fclose(fout);
	paranoid_free(tmp);
	paranoid_free(title);
	close_evalcall_form();
	return (res);
}









/**
 * Wrapper around @c write_image_to_floppy_SUB().
 * This function, unlike @c write_image_to_floppy_SUB(),
 * gives the user the opportunity to retry if the write fails.
 * @see write_image_to_floppy_SUB
 */
int write_image_to_floppy(char *device, char *datafile)
{
	/*@ int ************************************************************** */
	int res = 0;

	assert_string_is_neither_NULL_nor_zerolength(device);
	assert_string_is_neither_NULL_nor_zerolength(datafile);

	while ((res = write_image_to_floppy_SUB(device, datafile))) {
		if (!ask_me_yes_or_no("Failed to write image to floppy. Retry?")) {
			return (res);
		}
	}
	return (res);
}

/* @} - end of utilityGroup */

void setenv_mondo_share(void) {

setenv("MONDO_SHARE", MONDO_SHARE, 1);
}
