/* libmondo-devices.c                 Subroutines for handling devices
   $Id$
.


01/07/2005
- sensibly_set_tmpdir_and_scratchdir() --- exclude smb and smbfs and (new) cifs

10/23
- if root mounted at '/dev/rd/' then say FALSE, not mounted on ramdisk
  just to please Fred Beondo :-p

07/25
- updated get_phys_size_of_drive() to support newer, larger drives

07/18
- better support of users who boot from LVM CD and nuke-restore non-LVM backups

07/08
- BLKGETSIZE64 instead of BLKGETIO..whatever (Joshua)
- amended to use BLKGETSIZE if BLKGETSIZE64 missing (Hugo)
- resort to HDIO_GETGEO if BLKGETSIZE and BLKGETSIZE63 fail


06/25
- added Mandrake 9.2 support to which_boot_loader()
- cleaned up find_cdrom_device()

06/23
- rewrote resolve_softlinks_to_get_to_actual_device_file() in C

06/17
- don't use star automatically if SELINUX detected; let user do that

05/07
- usage of parted2fdisk instead of fdisk alone (ia32/ia64 compatibility)
  BCO

04/17
- replaced INTERNAL_TAPE_BLK_SIZE with bkpinfo->internal_tape_block_size

04/13
- if kernel >= 2.6 then ask for /dev entry, not SCSI node

04/12
- in mount_CDROM_...whatever, don't use bkpinfo_DONTUSETHIS to see if
  mounting a real CD device; look for /dev/ at start of $device instead

04/04
- when testing for tape drive, use INTERNAL_TAPE_BLK_SIZE, not TAPE_BLOCK_SIZE

04/03
- don't eject/retract drive if ISO

02/23/2004
- changed strstr() to strncmp() in a few places

12/11/2003
- if we can't find the DVD using dvdrecord then try it w/cdrecord

11/15
- changed a few []s to char*s
- better support of multiple CD-ROM drives

11/14
- better find_cdrom_device(), to cope w/ multiple CD writers

10/26
- call 'mt -f %s offline' to eject tape drive in eject_device()

10/25
- ask user to confirm which NFS share to mount
- improved which_boot_loader()

10/22
- tweaked find_cdrom_device() and find_cdrw_device()
- created find_dvd_device()

10/21
- changed "/mnt/cdrom" to MNT_CDROM

09/28
- interactive gui no longer asks dvd's speed

09/26
- interactive gui now supports dvd

09/23
- malloc/free global strings in new subroutines - malloc_libmondo_global_strings()
  and free_libmondo_global_strings() - which are in libmondo-tools.c

09/21
- ask for CD size when backing up interactively
- offer to exclude NFS shares if they're present

09/20
- exclude /dev/shm from sensibly_*()

09/18
- less verbose logging when insisting on CD #n

09/09
- added resolve_softlinks_to_get_to_actual_device_file()

09/05
- rewrote inisist_on_this_cd_number() and what_number_cd_is_this()

09/02
- is_this_a_valid_disk_format() -- treat ntfs (a.k.a. type 7) as recognized format
  just in case user's backup includes a partimagehack-imaged drive

08/01 - 08/31
- better boot-time tape detection
- remove backup-time, erroneous, "remove floppy" msg
- working on am_I_in_disaster_recovery_mode()
- interactively_obtain_...() - pause & ask user to remove CD/floppy

06/01 - 07/31
- in which_boot_loader(), also search /dev/cciss for boot loader
- fixed calls to popup_and_get_string()
- fixed bug in get_phys_size_of_drive()
- fixed bug in where_is_root_mounted()
- commented out a spurious assert()

05/01 - 05/31
- superior get_phys_size_of_drive() (Joshua Oreman)
- fixed call to inject_device() --- Andree L.
- find_cdrom_device() now may, if asked to, mount drive if possible,
  to test that it has a CD in it; it then unmounts
- mount_CDROM_here() now calls retract_cd_and_defeat_autorun() as well
- fixed support for subdir-within-NFS-mount
- cleaned up some FreeBSD-specific stuff
- added Joshua Oreman's FreeBSD patches
- mount_CDROM_here() --- mkdir -p mountpt
- add older drive support to get_phys_size_of_drive()

04/01 - 04/30
- made which_boot_loader() more friendly towards Red Hat 9
- find_and_mount_actual_cd() retracts all CD trays
- added lots of log_OS_error()'s and assert()'s
- fix ISO support in interactively_restore_...()
- made find_cdrom_device() skip dmesg if second-stage string is empty
- in find_cdrw_device(), find cdrecord before dvdrecord
- fixed am_i_in_disaster_recovery_mode()
- fixed insist_on_this_cd_number()

03/01 - 03/31
- more work on find_cdrom_device()
- find_cdrom_device() --- if nonexistent/not found then
  make sure to return '' as dev str and 1 as res
- cleaner find_cdrom_device(), to find DVD instead of CD-ROM
  as last resort
- fix insist_on_this_cd_number()
- add user the speed of CD writer, if writing
- if root is /dev/root then assume not a ramdisk

01/07 - 02/28
- fixed find_cdrom_device (Stan Benoit)
- if root is at /dev/root/root then LVM (not ramdisk)
- fix am_I_in_disaster_recovery_mode() to recognize that /dev/rd/cp*
  entries are not ramdisks but RAID disks
- find_cdrw_device() no longer complains to stderr if no cdrecord

01/02/2003
- sensibly_set_tmpdir_and_scratchdir() --- exclude smb and smbfs

12/01/2002
- don't ask for isodir path if in disaster recovery mode

11/01 - 11/30
- added g_restoring_live_from_cd;
- handle_incoming_parameters() will sensibly set tmpdir and scratchdir
- properly wipe spurious scratchdir and tmpdir
- modified sensibly_...() to use tmp.mondo.* and mondo.scratch.*
  instead of mondo.tmpdir.* and mondo.scratchdir.*
- line 861,912: set media_size[0] to 1999 as well as 1 thru N
- chmod 700, not 770, in make_fifo()
- sensibly_set_tmpdir_and_scratchdir() will now prefix a '/' to the
  strings if it is missing

10/01 - 10/31
- changed find_cdrom_device() to ignore DVD drives in the hd[a-d] search;
  consequently, the subroutine will try scd0, sr0, etc. afterwards
- commented code
- fixed obscure bug in find_and_mount_actual_cd()
- code which sensibly configures tmpdir,scratchdir is now sep. sub.
- changed sort -s to sort -n <-- sensibly...()

09/01 - 09/30
- added list_of_NFS_devices_and_mounts()
- try /dev/st0 if no tape specified at all
- change '64k' to TAPE_BLOCK_SIZE
- improved find_cdrom_device()
- if cdrecord not found then find_cdrom_device() returns 1
- if disaster recovery mode then don't ask for CD-ROM's /dev entry;
  assume /dev/cdrom
- if restoring data then ask for CD-ROM's /dev entry (not its SCSI node)
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_
- changed sort -n +4 to sort -s +4 (Troff)
- ask if user's kernel is sane; if 'no' then use FAILSAFE
- ask user to confirm the tape/CD device name
- if restoring (not archiving) then don't try to find CD-ROM's SCSI
  node; try to find its /dev entry instead
- better detection of biggest partition

08/01 - 08/31
- fixed verify bug --- CD#1 was being verified again & again & ...
- detect boot loader + device; if not detectible then _ask_
- if run interactively, assume tape size is irrelevant & don't ask
  for it; Mondo should handle end-of-tape gracefully now
- worked on handling unknowng media size
- added interactively_obtain_media_parameters_from_user()
- when trying to mount CD-ROM, find_and_mount_actual_cd() will
  now call find_cdrom_device() to try to find the /dev entry first
- fixed serious bug in zero_out_a_device()
- cleaned up some log_it() calls
- fixed minor bug in find_cdrom_device()
- better at distinguishing between live filesystem and ramdisk

07/24/2002
- created
*/

/**
 * @file
 * Functions to handle interactions with backup devices.
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-files-EXT.h"
#include "libmondo-devices.h"
#include "lib-common-externs.h"
#include "libmondo-string-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-stream-EXT.h"

#include <sys/types.h>
#ifdef __FreeBSD__
#define DKTYPENAMES
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/disk.h>
#elif linux
#define u64 unsigned long long
#include <linux/fs.h>			/* for BLKGETSIZE64 */
#include <linux/hdreg.h>
#endif

/*@unused@*/
//static char cvsid[] = "$Id$";

extern int g_current_media_number;
extern double g_kernel_version;

extern bool g_ISO_restore_mode;
extern struct s_bkpinfo *g_bkpinfo_DONTUSETHIS;
extern char *g_erase_tmpdir_and_scratchdir;
extern char *g_selfmounted_isodir;

static char g_cdrw_drive_is_here[MAX_STR_LEN / 4] = "";
static char g_cdrom_drive_is_here[MAX_STR_LEN / 4] = "";
static char g_dvd_drive_is_here[MAX_STR_LEN / 4] = "";


/**
 * ????? @bug ?????
 * @ingroup globalGroup
 */
bool g_restoring_live_from_cd = FALSE;

extern t_bkptype g_backup_media_type;	// set by main()




void set_g_cdrom_and_g_dvd_to_bkpinfo_value(struct s_bkpinfo *bkpinfo)
{
	strcpy(g_cdrom_drive_is_here, bkpinfo->media_device);	// just in case
	strcpy(g_dvd_drive_is_here, bkpinfo->media_device);	// just in case
}



/**
 * Retract all CD trays and wait for autorun to complete.
 * @ingroup deviceGroup
 */
void retract_CD_tray_and_defeat_autorun(void)
{
//  log_it("rctada: Retracting all CD trays", __LINE__);
	if (strlen(g_cdrom_drive_is_here) > 0) {
		inject_device(g_cdrom_drive_is_here);
	}
	if (strlen(g_dvd_drive_is_here) > 0) {
		inject_device(g_dvd_drive_is_here);
	}
	if (strlen(g_cdrw_drive_is_here) > 0) {
		inject_device(g_cdrw_drive_is_here);
	}
//  log_it("rctada: killing autorun");
//  run_program_and_log_output("killall autorun", TRUE);
	if (!run_program_and_log_output("ps | grep autorun | grep -v grep", 5)) {
		log_it("autorun detected; sleeping for 2 seconds");
		sleep(2);
	}
	log_it("rctada: Unmounting all CD drives", __LINE__);
	run_program_and_log_output("umount /dev/cdr* /dev/dvd*", 5);
}



/**
 * Determine whether we're booted off a ramdisk.
 * @return @c TRUE (we are) or @c FALSE (we aren't).
 * @ingroup utilityGroup
 */
bool am_I_in_disaster_recovery_mode(void)
{
	char *tmp, *comment;
	bool is_this_a_ramdisk = FALSE;

	malloc_string(tmp);
	malloc_string(comment);
	strcpy(tmp, where_is_root_mounted());
	sprintf(comment, "root is mounted at %s\n", tmp);
	log_msg(0, comment);
	log_msg(0,
			"No, Schlomo, that doesn't mean %s is the root partition. It's just a debugging message. Relax. It's part of am_I_in_disaster_recovery_mode().",
			tmp);

#ifdef __FreeBSD__
	if (strstr(tmp, "/dev/md")) {
		is_this_a_ramdisk = TRUE;
	}
#else
	if (!strncmp(tmp, "/dev/ram", 8)
		|| (!strncmp(tmp, "/dev/rd", 7) && !strcmp(tmp, "/dev/rd/")
			&& strncmp(tmp, "/dev/rd/cd", 10)) || strstr(tmp, "rootfs")
		|| !strcmp(tmp, "/dev/root")) {
		is_this_a_ramdisk = TRUE;
	} else {
		is_this_a_ramdisk = FALSE;
	}
#endif

	if (is_this_a_ramdisk) {
		if (!does_file_exist("/THIS-IS-A-RAMDISK")
			&& !does_file_exist("/tmp/mountlist.txt.sample")) {
			log_to_screen
				("Using /dev/root is stupid of you but I'll forgive you.");
			is_this_a_ramdisk = FALSE;
		}
	}
	if (does_file_exist("/THIS-IS-A-RAMDISK")) {
		is_this_a_ramdisk = TRUE;
	}
	paranoid_free(tmp);
	paranoid_free(comment);
	log_msg(1, "Is this a ramdisk? result = %d", is_this_a_ramdisk);
	return (is_this_a_ramdisk);
}





/**
 * Turn @c bkpinfo->backup_media_type into a human-readable string.
 * @return The human readable string (e.g. @c cdr becomes <tt>"cdr"</tt>).
 * @note The returned string points to static storage that will be overwritten with each call.
 * @ingroup stringGroup
 */
static char *bkptype_to_string(t_bkptype bt)
{
	static char output[MAX_STR_LEN / 4];
	switch (bt) {
	case none:
		strcpy(output, "none");
		break;
	case iso:
		strcpy(output, "iso");
		break;
	case cdr:
		strcpy(output, "cdr");
		break;
	case cdrw:
		strcpy(output, "cdrw");
		break;
	case cdstream:
		strcpy(output, "cdstream");
		break;
	case nfs:
		strcpy(output, "nfs");
		break;
	case tape:
		strcpy(output, "tape");
		break;
	case udev:
		strcpy(output, "udev");
		break;
	default:
		strcpy(output, "default");
	}
	return (output);
}



/**
 * @addtogroup deviceGroup
 * @{
 */
/**
 * Eject the tray of the specified CD device.
 * @param dev The device to eject.
 * @return the return value of the @c eject command. (0=success, nonzero=failure)
 */
int eject_device(char *dev)
{
	char *command;
	int res1 = 0, res2 = 0;

	malloc_string(command);

	if (IS_THIS_A_STREAMING_BACKUP(g_backup_media_type)
		&& g_backup_media_type != udev) {
		sprintf(command, "mt -f %s offline", dev);
		res1 = run_program_and_log_output(command, 1);
	} else {
		res1 = 0;
	}

#ifdef __FreeBSD__
	if (strstr(dev, "acd")) {
		sprintf(command, "cdcontrol -f %s eject", dev);
	} else {
		sprintf(command, "camcontrol eject `echo %s | sed 's|/dev/||'`",
				dev);
	}
#else
	sprintf(command, "eject %s", dev);
#endif

	log_msg(3, "Ejecting %s", dev);
	res2 = run_program_and_log_output(command, 1);
	paranoid_free(command);
	if (res1 && res2) {
		return (1);
	} else {
		return (0);
	}
}

/**
 * Load (inject) the tray of the specified CD device.
 * @param dev The device to load/inject.
 * @return 0 for success, nonzero for failure.
 */
int inject_device(char *dev)
{
	char *command;
	int i;

	malloc_string(command);


#ifdef __FreeBSD__
	if (strstr(dev, "acd")) {
		sprintf(command, "cdcontrol -f %s close", dev);
	} else {
		sprintf(command, "camcontrol load `echo %s | sed 's|/dev/||'`",
				dev);
	}
#else
	sprintf(command, "eject -t %s", dev);
#endif
	i = run_program_and_log_output(command, FALSE);
	paranoid_free(command);
	return (i);
}


/**
 * Determine whether the specified @p device (really, you can use any file)
 * exists.
 * @return TRUE if it exists, FALSE if it doesn't.
 */
bool does_device_exist(char *device)
{

	/*@ buffers *********************************************************** */
	char *tmp;
	bool ret;

	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(device);

	sprintf(tmp, "ls %s > /dev/null 2> /dev/null", device);

	if (system(tmp)) {
		ret = FALSE;
	} else {
		ret = TRUE;
	}
	paranoid_free(tmp);
	return (ret);
}


/**
 * Determine whether a non-Microsoft partition exists on any connected hard drive.
 * @return TRUE (there's a Linux/FreeBSD partition) or FALSE (Microsoft has taken over yet another innocent PC).
 */
bool does_nonMS_partition_exist(void)
{
#if __FreeBSD__
	return
		!system
		("for drive in /dev/ad? /dev/da?; do fdisk $drive | grep -q FreeBSD && exit 0; done; false");
#else
	return
		!system
		("parted2fdisk -l 2>/dev/null | grep '^/dev/' | egrep -qv '(MS|DOS|FAT|NTFS)'");
#endif
}

/**
 * Determine whether the specified @p partno exists on the specified @p drive.
 * @param drive The drive to search for the partition in.
 * @param partno The partition number to look for.
 * @return 0 if it exists, nonzero otherwise.
 */
int does_partition_exist(const char *drive, int partno)
{
	/*@ buffers **************************************************** */
	char *program;
	char *incoming;
	char *searchstr;
	char *tmp;

	/*@ ints ******************************************************* */
	int res = 0;

	/*@ pointers *************************************************** */
	FILE *fin;


	/*@ end vars *************************************************** */
	assert_string_is_neither_NULL_nor_zerolength(drive);
	assert(partno >= 0 && partno < 999);

	malloc_string(program);
	malloc_string(incoming);
	malloc_string(searchstr);
	malloc_string(tmp);

#ifdef __FreeBSD__
	// We assume here that this is running from mondorestore. (It is.)
	sprintf(program, "ls %s >/dev/null 2>&1", drive,
			build_partition_name(tmp, drive, partno));
	return system(program);
#else
	tmp[0] = '\0';
#endif

	sprintf(program, "parted2fdisk -l %s 2> /dev/null", drive);
	fin = popen(program, "r");
	if (!fin) {
		log_it("program=%s", program);
		log_OS_error("Cannot popen-in program");
		return (0);
	}
	(void) build_partition_name(searchstr, drive, partno);
	strcat(searchstr, " ");
	for (res = 0; !res && fgets(incoming, MAX_STR_LEN - 1, fin);) {
		if (strstr(incoming, searchstr)) {
			res = 1;
		}
	}
	if (pclose(fin)) {
		log_OS_error("Cannot pclose fin");
	}
	paranoid_free(program);
	paranoid_free(incoming);
	paranoid_free(searchstr);
	paranoid_free(tmp);
	return (res);
}





/**
 * Determine whether given NULL-terminated @p str exists in the MBR of @p dev.
 * @param dev The device to look in.
 * @param str The string to look for.
 * @return TRUE if it exists, FALSE if it doesn't.
 */
bool does_string_exist_in_boot_block(char *dev, char *str)
{
	/*@ buffers **************************************************** */
	char *command;

	/*@ end vars *************************************************** */
	int i;

	assert_string_is_neither_NULL_nor_zerolength(dev);
	assert_string_is_neither_NULL_nor_zerolength(str);

	malloc_string(command);
	sprintf(command,
			"dd if=%s bs=446 count=1 2> /dev/null | strings | grep \"%s\" > /dev/null 2> /dev/null",
			dev, str);
	i = system(command);
	paranoid_free(command);
	if (i) {
		return (FALSE);
	} else {
		return (TRUE);
	}
}

/**
 * Determine whether specified @p str exists in the first @p n sectors of
 * @p dev.
 * @param dev The device to look in.
 * @param str The string to look for.
 * @param n The number of 512-byte sectors to search.
 */
bool does_string_exist_in_first_N_blocks(char *dev, char *str, int n)
{
	/*@ buffers **************************************************** */
	char *command;
	/*@ end vars *************************************************** */
	int i;

	malloc_string(command);
	sprintf(command,
			"dd if=%s bs=512 count=%i 2> /dev/null | strings | grep \"%s\" > /dev/null 2> /dev/null",
			dev, n, str);
	i = system(command);
	paranoid_free(command);
	if (i) {
		return (FALSE);
	} else {
		return (TRUE);
	}
}



/**
 * Try to mount CD-ROM at @p mountpoint. If the CD-ROM is not found or has
 * not been specified, call find_cdrom_device() to find it.
 * @param bkpinfo The backup information structure. The only field used is @c bkpinfo->media_device.
 * @param mountpoint Where to mount the CD-ROM.
 * @return 0 for success, nonzero for failure.
 * @see mount_CDROM_here
 */
int find_and_mount_actual_cd(struct s_bkpinfo *bkpinfo, char *mountpoint)
{
	/*@ buffers ***************************************************** */

	/*@ int's  ****************************************************** */
	int res;
	char *dev;

	/*@ end vars **************************************************** */

	malloc_string(dev);
	assert(bkpinfo != NULL);
	assert_string_is_neither_NULL_nor_zerolength(mountpoint);

	if (g_backup_media_type == dvd) {
		strcpy(dev, g_dvd_drive_is_here);
		if (!dev[0]) {
			find_dvd_device(dev, FALSE);
		}
	} else {
		strcpy(dev, g_cdrom_drive_is_here);
		if (!dev[0]) {
			find_cdrom_device(dev, FALSE);
		}
	}

	if (bkpinfo->backup_media_type != iso) {
		retract_CD_tray_and_defeat_autorun();
	}

	if (!dev[0] || (res = mount_CDROM_here(dev, mountpoint))) {
		if (!popup_and_get_string
			("CD-ROM device", "Please enter your CD-ROM's /dev device",
			 dev, MAX_STR_LEN / 4)) {
			res = 1;
		} else {
			res = mount_CDROM_here(dev, mountpoint);
		}
	}
	if (res) {
		log_msg(1, "mount failed");
	} else {
		log_msg(1, "mount succeeded with %s", dev);
	}
	paranoid_free(dev);
	return (res);
}





/**
 * Locate a CD-R/W writer's SCSI node.
 * @param cdrw_device SCSI node will be placed here.
 * @return 0 for success, nonzero for failure.
 */


/**
 * Locate a CD-R/W writer's SCSI node.
 * @param cdrw_device SCSI node will be placed here.
 * @return 0 for success, nonzero for failure.
 */
int find_cdrw_device(char *cdrw_device)
{
	/*@ buffers ************************ */
	char *comment;
	char *tmp;
	char *cdr_exe;
	char *command;

	malloc_string(comment);
	malloc_string(tmp);
	malloc_string(cdr_exe);
	malloc_string(command);
	if (g_cdrw_drive_is_here[0]) {
		strcpy(cdrw_device, g_cdrw_drive_is_here);
		log_msg(3, "Been there, done that. Returning %s", cdrw_device);
		paranoid_free(comment);
		paranoid_free(tmp);
		paranoid_free(cdr_exe);
		paranoid_free(command);
		return (0);
	}
	if (g_backup_media_type == dvd) {
		log_msg(1,
				"This is dumb. You're calling find_cdrw_device() but you're backing up to DVD. WTF?");
		paranoid_free(comment);
		paranoid_free(tmp);
		paranoid_free(cdr_exe);
		paranoid_free(command);
		return (1);
	}
	run_program_and_log_output("insmod ide-scsi", -1);
	if (find_home_of_exe("cdrecord")) {
		strcpy(cdr_exe, "cdrecord");
	} else {
		strcpy(cdr_exe, "dvdrecord");
	}
	tmp[0] = '\0';
	if (find_home_of_exe(cdr_exe)) {
		sprintf(command,
				"%s -scanbus 2> /dev/null | tr -s '\t' ' ' | grep \"[0-9]*,[0-9]*,[0-9]*\" | grep -v \"[0-9]*) \\*\" | grep CD | cut -d' ' -f2 | head -n1",
				cdr_exe);
		strcpy(tmp, call_program_and_get_last_line_of_output(command));
	}
	if (strlen(tmp) < 2) {
		paranoid_free(comment);
		paranoid_free(tmp);
		paranoid_free(cdr_exe);
		paranoid_free(command);
		return 1;
	} else {
		strcpy(cdrw_device, tmp);
		sprintf(comment, "Found CDRW device - %s", cdrw_device);
		log_it(comment);
		strcpy(g_cdrw_drive_is_here, cdrw_device);
		paranoid_free(comment);
		paranoid_free(tmp);
		paranoid_free(cdr_exe);
		paranoid_free(command);
		return (0);
	}
}




/**
 * Attempt to locate a CD-ROM device's /dev entry.
 * Several different methods may be used to find the device, including
 * calling @c cdrecord, searching @c dmesg, and trial-and-error.
 * @param output Where to put the located /dev entry.
 * @param try_to_mount Whether to mount the CD as part of the test; if mount
 * fails then return failure.
 * @return 0 for success, nonzero for failure.
 */
int find_cdrom_device(char *output, bool try_to_mount)
{
	/*@ pointers **************************************************** */
	FILE *fin;
	char *p;
	char *q;
	char *r;
	int retval = 0;

	/*@ bool's ****************************************************** */
	bool found_it = FALSE;

	/*@ buffers ***************************************************** */
	char *tmp;
	char *cdr_exe;
	char *phrase_one;
	char *phrase_two;
	char *command;
	char *dvd_last_resort;
	char *mountpoint;
	static char the_last_place_i_found_it[MAX_STR_LEN] = "";

	/*@ intialize *************************************************** */
	malloc_string(tmp);
	malloc_string(cdr_exe);
	malloc_string(phrase_one);
	malloc_string(phrase_two);
	malloc_string(command);
	malloc_string(dvd_last_resort);
	malloc_string(mountpoint);

	output[0] = '\0';
	phrase_one[0] = '\0';
	phrase_two[0] = '\0';
	dvd_last_resort[0] = '\0';

	/*@ end vars **************************************************** */

	if (g_cdrom_drive_is_here[0] && !isdigit(g_cdrom_drive_is_here[0])) {
		strcpy(output, g_cdrom_drive_is_here);
		log_msg(3, "Been there, done that. Returning %s", output);
		retval = 0;
		goto end_of_find_cdrom_device;
	}
	if (the_last_place_i_found_it[0] != '\0' && !try_to_mount) {
		strcpy(output, the_last_place_i_found_it);
		log_msg(3,
				"find_cdrom_device() --- returning last found location - '%s'",
				output);
		retval = 0;
		goto end_of_find_cdrom_device;
	}

	sprintf(mountpoint, "/tmp/cd.%d", (int) (random() % 32767));
	make_hole_for_dir(mountpoint);

	if (find_home_of_exe("cdrecord")) {
		strcpy(cdr_exe, "cdrecord");
	} else {
		strcpy(cdr_exe, "dvdrecord");
	}
	tmp[0] = '\0';
	if (!find_home_of_exe(cdr_exe)) {
		strcpy(output, "/dev/cdrom");
		log_msg(4, "Can't find cdrecord; assuming %s", output);
		if (!does_device_exist(output)) {
			log_msg(4, "That didn't work. Sorry.");
			retval = 1;
			goto end_of_find_cdrom_device;
		} else {
			retval = 0;
			goto end_of_find_cdrom_device;
		}
	}

	sprintf(command, "%s -scanbus 2> /dev/null", cdr_exe);
	fin = popen(command, "r");
	if (!fin) {
		log_msg(4, "command=%s", command);
		log_OS_error("Cannot popen command");
		return (1);
	}
	for (fgets(tmp, MAX_STR_LEN, fin); !feof(fin);
		 fgets(tmp, MAX_STR_LEN, fin)) {
		p = strchr(tmp, '\'');
		if (p) {
			q = strchr(++p, '\'');
			if (q) {
				for (r = q; *(r - 1) == ' '; r--);
				*r = '\0';
				strcpy(phrase_one, p);
				p = strchr(++q, '\'');
				if (p) {
					q = strchr(++p, '\'');
					if (q) {
						while (*(q - 1) == ' ') {
							q--;
						}
						*q = '\0';
						strcpy(phrase_two, p);
					}
				}
			}
		}
	}
	paranoid_pclose(fin);

#ifndef __FreeBSD__
	if (strlen(phrase_two) == 0) {
		log_msg(4, "Not running phase two. String is empty.");
	} else {
		sprintf(command, "dmesg | grep \"%s\" 2> /dev/null", phrase_two);
		fin = popen(command, "r");
		if (!fin) {
			log_msg(4, "Cannot run 2nd command - non-fatal, fortunately");
		} else {
			for (fgets(tmp, MAX_STR_LEN, fin); !feof(fin);
				 fgets(tmp, MAX_STR_LEN, fin)) {
				log_msg(5, "--> '%s'", tmp);
				if (tmp[0] != ' ' && tmp[1] != ' ') {
					p = strchr(tmp, ':');
					if (p) {
						*p = '\0';
						if (strstr(tmp, "DVD")) {
							sprintf(dvd_last_resort, "/dev/%s", tmp);
							log_msg(4,
									"Ignoring '%s' because it's a DVD drive",
									tmp);
						} else {
							sprintf(output, "/dev/%s", tmp);
							found_it = TRUE;
						}
					}
				}
			}
			paranoid_pclose(fin);
		}
	}

#endif
#ifdef __FreeBSD__
	if (!found_it) {
		log_msg(4, "OK, approach 2");
		if (!(found_it = set_dev_to_this_if_rx_OK(output, "/dev/cdrom"))) {
			if (!
				(found_it =
				 set_dev_to_this_if_rx_OK(output, "/dev/cdrom1"))) {
				if (!
					(found_it =
					 set_dev_to_this_if_rx_OK(output, "/dev/dvd"))) {
					if (!
						(found_it =
						 set_dev_to_this_if_rx_OK(output, "/dev/acd0"))) {
						if (!
							(found_it =
							 set_dev_to_this_if_rx_OK(output,
													  "/dev/cd01"))) {
							if (!
								(found_it =
								 set_dev_to_this_if_rx_OK(output,
														  "/dev/acd1"))) {
								if (!
									(found_it =
									 set_dev_to_this_if_rx_OK(output,
															  "/dev/cd1")))
								{
									retval = 1;
									goto end_of_find_cdrom_device;
								}
							}
						}
					}
				}
			}
		}
	}
#else
	if (!found_it && strlen(dvd_last_resort) > 0) {
		log_msg(4, "Well, I'll use the DVD - %s - as a last resort",
				dvd_last_resort);
		strcpy(output, dvd_last_resort);
		found_it = TRUE;
	}
	if (found_it) {
		sprintf(tmp, "grep \"%s=ide-scsi\" /proc/cmdline &> /dev/null",
				strrchr(output, '/') + 1);
		if (system(tmp) == 0) {
			log_msg(4,
					"%s is not right. It's being SCSI-emulated. Continuing.",
					output);
			found_it = FALSE;
			output[0] = '\0';
		}
	}

	if (found_it) {
		log_msg(4, "(find_cdrom_device) --> '%s'", output);
		if (!does_device_exist(output)) {
			found_it = FALSE;
			log_msg(4, "OK, I was wrong, I haven't found it... yet.");
		}
	}

	if (!found_it) {
		log_msg(4, "OK, approach 2");
		if (!(found_it = set_dev_to_this_if_rx_OK(output, "/dev/scd0"))) {
			if (!(found_it = set_dev_to_this_if_rx_OK(output, "/dev/sr0"))) {
				if (!
					(found_it =
					 set_dev_to_this_if_rx_OK(output, "/dev/cdrom"))) {
					if (!
						(found_it =
						 set_dev_to_this_if_rx_OK(output,
												  "/dev/cdrom0"))) {
						if (!
							(found_it =
							 set_dev_to_this_if_rx_OK(output,
													  "/dev/cdrom1"))) {
							if (!
								(found_it =
								 set_dev_to_this_if_rx_OK(output,
														  "/dev/sr1"))) {
								if (!
									(found_it =
									 set_dev_to_this_if_rx_OK(output,
															  "/dev/dvd")))
								{
									if (!
										(found_it =
										 set_dev_to_this_if_rx_OK(output,
																  g_cdrw_drive_is_here)))
									{
										retval = 1;
										goto end_of_find_cdrom_device;
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif

	if (found_it && try_to_mount) {
		if (mount_CDROM_here(output, mountpoint)) {
			log_msg(4, "[Cardigans] I've changed my mind");
			found_it = FALSE;
		} else {
			sprintf(tmp, "%s/archives", mountpoint);
			if (!does_file_exist(tmp)) {
				log_msg(4, "[Cardigans] I'll take it back");
				found_it = FALSE;
			} else {
				sprintf(command, "umount %s", output);
				paranoid_system(command);
				log_msg(4, "I'm confident the Mondo CD is in %s", output);
			}
		}
	}
	unlink(mountpoint);

	if (found_it) {
		if (!does_file_exist(output)) {
			log_msg(3, "I still haven't found it.");
			return (1);
		}
		log_msg(3, "(find_cdrom_device) --> '%s'", output);
		strcpy(the_last_place_i_found_it, output);
		strcpy(g_cdrom_drive_is_here, output);
		retval = 0;
		goto end_of_find_cdrom_device;
	}

	sprintf(command,
			"%s -scanbus | grep \"[0-9],[0-9],[0-9]\" | grep \"[D|C][V|D]\" | grep -n \"\" | grep \"%s\" | cut -d':' -f2",
			cdr_exe, g_cdrw_drive_is_here);
	log_msg(1, "command=%s", command);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	if (tmp[0]) {
		strcpy(output, tmp);
		log_msg(4, "Finally found it at %s", output);
		retval = 0;
		goto end_of_find_cdrom_device;
	} else {
		log_msg(4, "Still couldn't find it.");
		retval = 1;
		goto end_of_find_cdrom_device;
	}
  end_of_find_cdrom_device:
	paranoid_free(tmp);
	paranoid_free(cdr_exe);
	paranoid_free(phrase_one);
	paranoid_free(phrase_two);
	paranoid_free(command);
	paranoid_free(dvd_last_resort);
	paranoid_free(mountpoint);
	return (retval);
}





int find_dvd_device(char *output, bool try_to_mount)
{
	char *command;
	char *tmp;
	int retval = 0, devno = -1;

	malloc_string(command);
	malloc_string(tmp);

	if (g_dvd_drive_is_here[0]) {
		strcpy(output, g_dvd_drive_is_here);
		log_msg(3, "Been there, done that. Returning %s", output);
		return (0);
	}

	sprintf(tmp, call_program_and_get_last_line_of_output
			("dvdrecord -scanbus 2> /dev/null | grep \") '\" | grep -n \"\" | grep DVD | cut -d':' -f1")
		);
	log_msg(5, "tmp = '%s'", tmp);
	if (!tmp[0])
		sprintf(tmp, call_program_and_get_last_line_of_output
				("cdrecord -scanbus 2> /dev/null | grep \") '\" | grep -n \"\" | grep DVD | cut -d':' -f1")
			);
	if (tmp[0]) {
		devno = atoi(tmp) - 1;
	}
	if (devno >= 0) {
		retval = 0;
		sprintf(output, "/dev/scd%d", devno);
		strcpy(g_dvd_drive_is_here, output);
		log_msg(2, "I think DVD is at %s", output);
	} else {
		log_msg(2, "I cannot find DVD");
		retval = 1;
	}

	if (try_to_mount) {
		log_msg(1, "Ignoring the fact that try_to_mount==TRUE");
	}
	return (retval);
}





#include <sys/ioctl.h>

/**
 * Find the size of the specified @p drive, in megabytes. Uses @c ioctl calls
 * and @c dmesg.
 * @param drive The device to find the size of.
 * @return size in megabytes.
 */
long get_phys_size_of_drive(char *drive)
{
	int fd;
#if linux
	unsigned long long s = 0;
	int fileid, cylinders = 0, cylindersleft = 0;
	int cylindersize = 0;
	int gotgeo = 0;


	struct hd_geometry hdgeo;
#elif __FreeBSD__
	off_t s;
#endif

	long outvalA = -1;
	long outvalB = -1;
	long outvalC = -1;

	if ((fd = open(drive, O_RDONLY)) != -1) {
		if (ioctl(fd,
#if linux
#ifdef BLKGETSIZE64
				  BLKGETSIZE64,
#else
				  BLKGETSIZE,
#endif
#elif __FreeBSD__
				  DIOCGMEDIASIZE,
#endif
				  &s) != -1) {
			close(fd);
			// s>>11 works for older disks but not for newer ones
			outvalB =
#if linux
#ifdef BLKGETSIZE64
				s >> 20
#else
				s >> 11
#endif
#else
				s >> 20
#endif
				;
		}
	}

	if (outvalB <= 0) {
		log_msg(1, "Error getting size of %s: %s", drive, strerror(errno));
	}
#if linux
	fileid = open(drive, O_RDONLY);
	if (fileid) {
		if (ioctl(fileid, HDIO_GETGEO, &hdgeo) != -1) {
			if (hdgeo.cylinders && hdgeo.heads && hdgeo.sectors) {
				cylindersleft = cylinders = hdgeo.cylinders;
				cylindersize = hdgeo.heads * hdgeo.sectors / 2;
				outvalA = cylindersize * cylinders / 1024;
				log_msg(2, "Got Harddisk geometry, C:%d, H:%d, S:%d",
						hdgeo.cylinders, hdgeo.heads, hdgeo.sectors);
				gotgeo = 1;
			} else {
				log_msg(1, "Harddisk geometry wrong");
			}
		} else {
			log_msg(1,
					"Error in ioctl() getting new hard disk geometry (%s), resizing in unsafe mode",
					strerror(errno));
		}
		close(fileid);
	} else {
		log_msg(1, "Failed to open %s for reading: %s", drive,
				strerror(errno));
	}
	if (!gotgeo) {
		log_msg(1, "Failed to get harddisk geometry, using old mode");
	}
/*  
  if ((fd = open (drive, O_RDONLY)) != -1) {
      if (ioctl (fd, HDIO_GETGEO, &hdgeo) != -1)  {
	  close (fd);
	  log_msg (2, "Geometry of drive %s is C:%d, H:%d, S%d, its size is %d MB", drive, hdgeo.cylinders, hdgeo.heads, hdgeo.sectors, (hdgeo.cylinders * hdgeo.heads * hdgeo.sectors / 2 / 1024));
	  if ( hdgeo.cylinders && hdgeo.heads && hdgeo.sectors ) {
		  outvalB = ((long) (hdgeo.cylinders * hdgeo.heads * hdgeo.sectors / 2 / 1024));
	  }
      }
      close (fd);
 */
#endif

// OLDER DISKS will give ridiculously low value for outvalB (so outvalA is returned) :)
// NEWER DISKS will give sane value for outvalB (close to outvalA, in other words) :)

	outvalC = (outvalA > outvalB) ? outvalA : outvalB;

//  log_msg (5, "drive = %s, error = %s", drive, strerror (errno));
//  fatal_error ("GPSOD: Unable to get size of drive");
	log_msg(1, "%s --> %ld or %ld --> %ld", drive, outvalA, outvalB,
			outvalC);

	return (outvalC);
}

/* The old version */
#if 0
long get_phys_size_of_drive(char *drive)
{
	/*@ pointers **************************************************** */
#if linux
	FILE *fin;
	char *p;
	char *q;
	char *r;
	/*@ buffers ***************************************************** */
	char *tmp;
	char *command;

	/*@ long ******************************************************** */
	long outL;
	long tempLa;
	long tempLb;
	long tempLc;

#endif

	struct hd_geometry hdgeo;
	int fd;

#ifdef __FreeBSD__
	off_t o;

	if ((fd = open(drive, O_RDONLY)) != -1) {
		if (ioctl(fd, DIOCGMEDIASIZE, &o) != -1) {
			close(fd);
			return (long) (o / (off_t) (1024 * 1024));
		}
		close(fd);
	}
	log_msg(4, "drive = %s, error = %s", drive, strerror(errno));
	fatal_error("GPSOD: Unable to get size of drive");
#else

	malloc_string(tmp);
	malloc_string(command);

	if ((fd = open(drive, O_RDONLY)) != -1) {
		if (ioctl(fd, HDIO_GETGEO, &hdgeo) != -1) {
			close(fd);
			log_msg(2,
					"Geometry of drive %s is C:%d, H:%d, S%d, its size is %d MB",
					drive, hdgeo.cylinders, hdgeo.heads, hdgeo.sectors,
					(hdgeo.cylinders * hdgeo.heads * hdgeo.sectors / 2 /
					 1024));
			if (hdgeo.cylinders && hdgeo.heads && hdgeo.sectors) {
				return ((long)
						(hdgeo.cylinders * hdgeo.heads * hdgeo.sectors /
						 2 / 1024));
			}
		}
		close(fd);
	}

	assert_string_is_neither_NULL_nor_zerolength(drive);

	sprintf(command,
			"parted2fdisk -l %s | head -n4 | tr -s '\n' '\t' | tr -s ' ' '\t' | cut -f8,14,16",
			drive);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	if (tmp[0]) {
		p = tmp;
		q = strchr(p, ' ');
		if (q) {
			*(q++) = '\0';
			r = strchr(q, ' ');
			if (r) {
				*(r++) = '\0';
				tempLa = atol(p);
				tempLb = atol(q);
				tempLc = atol(r);
				outL = tempLa * tempLb / 1024 * tempLc / 1024;
				if (outL > 100) {
					paranoid_free(tmp);
					paranoid_free(command);
					return (outL);
				}
			}
		}
	}

	/* try to grep for 'Drive xxxx: yyy MB' */
	sprintf(command,
			"parted2fdisk -l %s | grep MB | tr -s ' ' '\t' | cut -f3",
			drive);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	if (atol(tmp) > 0) {
		paranoid_free(tmp);
		paranoid_free(command);
		return (atol(tmp));
	}

	/* else, do it the old-fashioned way */
	p = strrchr(drive, (int) '/');
	if (p) {
		strcpy(tmp, p + 1);
	} else {
		paranoid_free(tmp);
		paranoid_free(command);
		return (-1);
	}
	sprintf(command, "dmesg | grep %s 2> /dev/null", tmp);
	if (!(fin = popen(command, "r"))) {
		log_OS_error("Cannot popen dmesg command");
	} else {
		fgets(tmp, MAX_STR_LEN - 1, fin);
		while (!feof(fin) && !strstr(tmp, "GB") && !strstr(tmp, "MB")) {
			fgets(tmp, MAX_STR_LEN - 1, fin);
		}
		if (pclose(fin)) {
			log_OS_error("Cannot pclose dmesg fin");
		}
	}
	if (!(p = strstr(tmp, "GB")) && !(p = strstr(tmp, "MB"))) {
		log_msg(3, "Cannot find %s's size: dmesg isn't helping either.",
				drive);
		paranoid_free(tmp);
		paranoid_free(command);
		return (-1);
	}
	for (; !isdigit(*(p - 1)); p--);
	*p = '\0';
	for (p--; isdigit(*(p - 1)); p--);
	outL = atol(p);
	if (outL <= 0) {
		paranoid_free(tmp);
		paranoid_free(command);
		return (-1);
	}
	if (strstr(tmp, "GB")) {
		outL = outL * 1024;
	}
	paranoid_free(tmp);
	paranoid_free(command);
	return (outL * 19 / 20);
#endif
}
#endif							/* 0 */





/**
 * Determine whether @p format is supported by the kernel. Uses /proc/filesystems
 * under Linux and @c lsvfs under FreeBSD.
 * @param format The format to test.
 * @return TRUE if the format is supported, FALSE if not.
 */
bool is_this_a_valid_disk_format(char *format)
{
	char *good_formats;
	char *command;
	char *format_sz;

	FILE *pin;
	int retval;
	malloc_string(good_formats);
	malloc_string(command);
	malloc_string(format_sz);

	assert_string_is_neither_NULL_nor_zerolength(format);

	sprintf(format_sz, "%s ", format);

#ifdef __FreeBSD__
	sprintf(command,
			"lsvfs | tr -s '\t' ' ' | grep -v Filesys | grep -v -- -- | cut -d' ' -f1 | tr -s '\n' ' '");
#else
	sprintf(command,
			"cat /proc/filesystems | grep -v nodev | tr -s '\t' ' ' | cut -d' ' -f2 | tr -s '\n' ' '");
#endif

	pin = popen(command, "r");
	if (!pin) {
		log_OS_error("Unable to read good formats");
		retval = 0;
	} else {
		strcpy(good_formats, " ");
		(void) fgets(good_formats + 1, MAX_STR_LEN, pin);
		if (pclose(pin)) {
			log_OS_error("Cannot pclose good formats");
		}
		strip_spaces(good_formats);
		strcat(good_formats, " swap lvm raid ntfs 7 ");	// " ntfs 7 " -- um, cheating much? :)
		if (strstr(good_formats, format_sz)) {
			retval = 1;
		} else {
			retval = 0;
		}
	}
	paranoid_free(good_formats);
	paranoid_free(command);
	paranoid_free(format_sz);
	return (retval);
}


/** @def SWAPLIST_COMMAND The command to list the swap files/partitions in use. */

/**
 * Determine whether @p device_raw is currently mounted.
 * @param device_raw The device to check.
 * @return TRUE if it's mounted, FALSE if not.
 */
bool is_this_device_mounted(char *device_raw)
{

	/*@ pointers **************************************************** */
	FILE *fin;

	/*@ buffers ***************************************************** */
	char *incoming;
	char *device_with_tab;
	char *device_with_space;
	char *tmp;
	int retval = 0;

#ifdef __FreeBSD__
#define SWAPLIST_COMMAND "swapinfo"
#else
#define SWAPLIST_COMMAND "cat /proc/swaps"
#endif

	/*@ end vars **************************************************** */

	malloc_string(incoming);
	malloc_string(device_with_tab);
	malloc_string(device_with_space);
	malloc_string(tmp);
	assert(device_raw != NULL);
//  assert_string_is_neither_NULL_nor_zerolength(device_raw);
	if (device_raw[0] != '/' && !strstr(device_raw, ":/")) {
		log_msg(1, "%s needs to have a '/' prefixed - I'll do it",
				device_raw);
		sprintf(tmp, "/%s", device_raw);
	} else {
		strcpy(tmp, device_raw);
	}
	log_msg(1, "Is %s mounted?", tmp);
	if (!strcmp(tmp, "/proc") || !strcmp(tmp, "proc")) {
		log_msg(1,
				"I don't know how the heck /proc made it into the mountlist. I'll ignore it.");
		return (0);
	}
	sprintf(device_with_tab, "%s\t", tmp);
	sprintf(device_with_space, "%s ", tmp);

	if (!(fin = popen("mount", "r"))) {
		log_OS_error("Cannot popen 'mount'");
		return (FALSE);
	}
	for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin);
		 fgets(incoming, MAX_STR_LEN - 1, fin)) {
		if (strstr(incoming, device_with_space)	//> incoming
			|| strstr(incoming, device_with_tab))	// > incoming)
		{
			paranoid_pclose(fin);
			retval = 1;
			goto end_of_func;
		}
	}
	paranoid_pclose(fin);
	sprintf(tmp, "%s | grep -w \"%s\" > /dev/null 2> /dev/null",
			SWAPLIST_COMMAND, device_with_space);
	log_msg(4, "tmp (command) = '%s'", tmp);
	if (!system(tmp)) {
		retval = 1;
		goto end_of_func;
	}
  end_of_func:
	paranoid_free(incoming);
	paranoid_free(device_with_tab);
	paranoid_free(device_with_space);
	paranoid_free(tmp);
	return (retval);
}

#ifdef __FreeBSD__
//                       CODE IS FREEBSD-SPECIFIC
/**
 * Create a loopback device for specified @p fname.
 * @param fname The file to associate with a device.
 * @return /dev entry for the device, or NULL if it couldn't be allocated.
 */
char *make_vn(char *fname)
{
	char *device = (char *) malloc(MAX_STR_LEN);
	char *mddevice = (char *) malloc(32);
	char command[MAX_STR_LEN];
	int vndev = 2;
	if (atoi
		(call_program_and_get_last_line_of_output
		 ("/sbin/sysctl -n kern.osreldate")) < 500000) {
		do {
			sprintf(mddevice, "vn%ic", vndev++);
			sprintf(command, "vnconfig %s %s", mddevice, fname);
			if (vndev > 10) {
				return NULL;
			}
		}
		while (system(command));
	} else {
		sprintf(command, "mdconfig -a -t vnode -f %s", fname);
		mddevice = call_program_and_get_last_line_of_output(command);
		if (!strstr(mddevice, "md")) {
			return NULL;
		}
	}
	sprintf(device, "/dev/%s", mddevice);
	return device;
}



//                       CODE IS FREEBSD-SPECIFIC
/**
 * Deallocate specified @p dname.
 * This should be called when you are done with the device created by make_vn(),
 * so the system does not run out of @c vn devices.
 * @param dname The device to deallocate.
 * @return 0 for success, nonzero for failure.
 */
int kick_vn(char *dname)
{
	char command[MAX_STR_LEN];

	if (strncmp(dname, "/dev/", 5) == 0) {
		dname += 5;
	}

	if (atoi
		(call_program_and_get_last_line_of_output
		 ("/sbin/sysctl -n kern.osreldate")) < 500000) {
		sprintf(command, "vnconfig -d %s", dname);
		return system(command);
	} else {
		sprintf(command, "mdconfig -d -u %s", dname);
		return system(command);
	}
	 /*NOTREACHED*/ return 255;
}
#endif


/**
 * Mount the CD-ROM at @p mountpoint.
 * @param device The device (or file if g_ISO_restore_mode) to mount.
 * @param mountpoint The place to mount it.
 * @return 0 for success, nonzero for failure.
 */
int mount_CDROM_here(char *device, char *mountpoint)
{
	/*@ buffer ****************************************************** */
	char *command;
	char *dev;
	char *options;
	int retval;

	malloc_string(command);
	malloc_string(dev);
	malloc_string(options);
	assert_string_is_neither_NULL_nor_zerolength(device);
	assert_string_is_neither_NULL_nor_zerolength(mountpoint);

	make_hole_for_dir(mountpoint);
	strcpy(options, "ro");
	if (isdigit(device[0])) {
		find_cdrom_device(device, FALSE);
	} else {
		strcpy(dev, device);
	}
	if (g_ISO_restore_mode) {

#ifdef __FreeBSD__
		strcpy(dev, make_vn(device));
		if (!dev) {
			sprintf(command, "Unable to mount ISO (make_vn(%s) failed)",
					device);
			fatal_error(command);
		}
		strcpy(device, dev);
#else
		strcat(options, ",loop");
#endif

	}
	log_msg(4, "(mount_CDROM_here --- device=%s, mountpoint=%s", device,
			mountpoint);
	/*@ end vars *************************************************** */

#ifdef __FreeBSD__
	sprintf(command, "mount_cd9660 -r %s %s 2>> %s",
			device, mountpoint, MONDO_LOGFILE);

#else
	sprintf(command, "mount %s -o %s -t iso9660 %s 2>> %s",
			device, options, mountpoint, MONDO_LOGFILE);
#endif

	log_msg(4, command);
	if (strncmp(device, "/dev/", 5) == 0) {
		retract_CD_tray_and_defeat_autorun();
	}
	retval = system(command);
	log_msg(1, "system(%s) returned %d", command, retval);

	paranoid_free(command);
	paranoid_free(dev);
	paranoid_free(options);
	return (retval);
}






/**
 * Ask the user for CD number @p cd_number_i_want.
 * Sets g_current_media_number once the correct CD is inserted.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->prefix
 * - @c bkpinfo->isodir
 * - @c bkpinfo->media_device
 * - @c bkpinfo->please_dont_eject_when_restoring
 * @param cd_number_i_want The CD number to ask for.
 */
void
insist_on_this_cd_number(struct s_bkpinfo *bkpinfo, int cd_number_i_want)
{

	/*@ int ************************************************************* */
	int res = 0;


	/*@ buffers ********************************************************* */
	char *tmp;
	char *request;

	assert(bkpinfo != NULL);
	assert(cd_number_i_want > 0);

//  log_msg(3, "Insisting on CD number %d", cd_number_i_want);

	if (IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type)) {
		log_msg(3,
				"No need to insist_on_this_cd_number when the backup type isn't CD-R(W) or NFS or ISO");
		return;
	}
	malloc_string(tmp);
	malloc_string(request);
	sprintf(tmp, "mkdir -p " MNT_CDROM);
	run_program_and_log_output(tmp, 5);
	if (g_ISO_restore_mode || bkpinfo->backup_media_type == iso
		|| bkpinfo->backup_media_type == nfs) {
		log_msg(3, "Remounting CD");
		g_ISO_restore_mode = TRUE;
// FIXME --- I'm tempted to do something about this...
// Why unmount and remount again and again?
		if (is_this_device_mounted(MNT_CDROM)) {
			run_program_and_log_output("umount " MNT_CDROM, 5);
		}
		system("mkdir -p /tmp/isodir &> /dev/null");
		sprintf(tmp, "%s/%s/%s-%d.iso", bkpinfo->isodir,
				bkpinfo->nfs_remote_dir, bkpinfo->prefix,
				cd_number_i_want);
		if (!does_file_exist(tmp)) {
			sprintf(tmp, "/tmp/isodir/%s/%s-%d.iso",
					bkpinfo->nfs_remote_dir, bkpinfo->prefix,
					cd_number_i_want);
			if (does_file_exist(tmp)) {
				log_msg(1,
						"FIXME - hacking bkpinfo->isodir from '%s' to /tmp/isodir",
						bkpinfo->isodir);
				strcpy(bkpinfo->isodir, "/tmp/isodir");
			}
		}
		log_msg(3, "Mounting %s at %s", tmp, MNT_CDROM);
		if (mount_CDROM_here(tmp, MNT_CDROM)) {
			fatal_error("Mommy!");
		}
//    g_current_media_number = cd_number_i_want;
//    return;
	}
	if ((res = what_number_cd_is_this(bkpinfo)) != cd_number_i_want) {
		log_msg(3, "Currently, we hold %d but we want %d", res,
				cd_number_i_want);
		sprintf(tmp, "Insisting on %s #%d",
				media_descriptor_string(bkpinfo->backup_media_type),
				cd_number_i_want);
		sprintf(request, "Please insert %s #%d and press Enter.",
				media_descriptor_string(bkpinfo->backup_media_type),
				cd_number_i_want);
		log_msg(3, tmp);
		while (what_number_cd_is_this(bkpinfo) != cd_number_i_want) {
			paranoid_system("sync");
			if (is_this_device_mounted(MNT_CDROM)) {
				res =
					run_program_and_log_output("umount " MNT_CDROM, FALSE);
			} else {
				res = 0;
			}
			if (res) {
				log_to_screen("WARNING - failed to unmount CD-ROM drive");
			}
			if (!bkpinfo->please_dont_eject) {
				res = eject_device(bkpinfo->media_device);
			} else {
				res = 0;
			}
			if (res) {
				log_to_screen("WARNING - failed to eject CD-ROM disk");
			}
			popup_and_OK(request);
			if (!bkpinfo->please_dont_eject) {
				inject_device(bkpinfo->media_device);
			}
			paranoid_system("sync");
		}
		log_msg(1, "Thankyou. Proceeding...");
		g_current_media_number = cd_number_i_want;
	}
	paranoid_free(tmp);
	paranoid_free(request);
}

/* @} - end of deviceGroup */






/**
 * Ask user for details of backup/restore information.
 * Called when @c mondoarchive doesn't get any parameters.
 * @param bkpinfo The backup information structure to fill out with the user's data.
 * @param archiving_to_media TRUE if archiving, FALSE if restoring.
 * @return 0, always.
 * @bug No point of `int' return value.
 * @ingroup archiveGroup
 */
int interactively_obtain_media_parameters_from_user(struct s_bkpinfo
													*bkpinfo,
													bool
													archiving_to_media)
// archiving_to_media is TRUE if I'm being called by mondoarchive
// archiving_to_media is FALSE if I'm being called by mondorestore
{
	char *tmp;
	char *sz_size;
	char *command;
	char *comment;
	char *prompt;
	int i;
	FILE *fin;

	malloc_string(tmp);
	malloc_string(sz_size);
	malloc_string(command);
	malloc_string(comment);
	malloc_string(prompt);
	assert(bkpinfo != NULL);
	sz_size[0] = '\0';
	bkpinfo->nonbootable_backup = FALSE;

// Tape, CD, NFS, ...?
	srandom(getpid());
	bkpinfo->backup_media_type =
		(g_restoring_live_from_cd) ? cdr :
		which_backup_media_type(bkpinfo->restore_data);
	if (bkpinfo->backup_media_type == none) {
		log_to_screen("User has chosen not to backup the PC");
		finish(1);
	}
	if (bkpinfo->backup_media_type == tape && bkpinfo->restore_data) {
		popup_and_OK("Please remove CD/floppy from drive(s)");
	}
	log_msg(3, "media type = %s",
			bkptype_to_string(bkpinfo->backup_media_type));
	if (archiving_to_media) {
		sensibly_set_tmpdir_and_scratchdir(bkpinfo);
	}
	bkpinfo->cdrw_speed = (bkpinfo->backup_media_type == cdstream) ? 2 : 4;
	bkpinfo->compression_level =
		(bkpinfo->backup_media_type == cdstream) ? 1 : 5;
	bkpinfo->use_lzo =
		(bkpinfo->backup_media_type == cdstream) ? TRUE : FALSE;

/*
  if (find_home_of_exe("star") && (!find_home_of_exe("afio") || find_home_of_exe("selinuxenabled")))
    {
      bkpinfo->use_star = FALSE;    
      bkpinfo->use_lzo = FALSE;
      log_to_screen("Using star, not afio");
      if (!find_home_of_exe("afio"))
	{ log_to_screen("...because afio not found"); }
      if (find_home_of_exe("selinuxenabled"))
	{ log_to_screen("...because SELINUX found"); }
    }
*/

	mvaddstr_and_log_it(2, 0, " ");

// Find device's /dev (or SCSI) entry
	switch (bkpinfo->backup_media_type) {
	case cdr:
	case cdrw:
	case dvd:
		if (archiving_to_media) {
			if (ask_me_yes_or_no
				("Is your computer a laptop, or does the CD writer incorporate BurnProof technology?"))
			{
				bkpinfo->manual_cd_tray = TRUE;
			}
			if ((bkpinfo->compression_level =
				 which_compression_level()) == -1) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			sprintf(comment, "What speed is your %s (re)writer?",
					media_descriptor_string(bkpinfo->backup_media_type));
			if (bkpinfo->backup_media_type == dvd) {
				find_dvd_device(bkpinfo->media_device, FALSE);
				strcpy(tmp, "1");
				sprintf(sz_size, "%d", DEFAULT_DVD_DISK_SIZE);	// 4.7 salesman's GB = 4.482 real GB = 4582 MB
				log_msg(1, "Setting to DVD defaults");
			} else {
				strcpy(bkpinfo->media_device, VANILLA_SCSI_CDROM);
				strcpy(tmp, "4");
				strcpy(sz_size, "650");
				log_msg(1, "Setting to CD defaults");
			}
			if (bkpinfo->backup_media_type != dvd) {
				if (!popup_and_get_string("Speed", comment, tmp, 4)) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
			}
			bkpinfo->cdrw_speed = atoi(tmp);	// if DVD then this shouldn't ever be used anyway :)
			sprintf(comment,
					"How much data (in Megabytes) will each %s store?",
					media_descriptor_string(bkpinfo->backup_media_type));
			if (!popup_and_get_string("Size", comment, sz_size, 5)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
				bkpinfo->media_size[i] = atoi(sz_size);
			}
			if (bkpinfo->media_size[0] <= 0) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
		}
	case cdstream:
		if (bkpinfo->disaster_recovery) {
			strcpy(bkpinfo->media_device, "/dev/cdrom");
			log_msg(2, "CD-ROM device assumed to be at %s",
					bkpinfo->media_device);
		} else if (bkpinfo->restore_data
				   || bkpinfo->backup_media_type == dvd) {
			if (!bkpinfo->media_device[0]) {
				strcpy(bkpinfo->media_device, "/dev/cdrom");
			}					// just for the heck of it :)
			log_msg(1, "bkpinfo->media_device = %s",
					bkpinfo->media_device);
			if (bkpinfo->backup_media_type == dvd
				|| find_cdrom_device(bkpinfo->media_device, FALSE)) {
				log_msg(1, "bkpinfo->media_device = %s",
						bkpinfo->media_device);
				sprintf(comment,
						"Please specify your %s drive's /dev entry",
						media_descriptor_string(bkpinfo->
												backup_media_type));
				if (!popup_and_get_string
					("Device?", comment, bkpinfo->media_device,
					 MAX_STR_LEN / 4)) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
			}
			log_msg(2, "%s device found at %s",
					media_descriptor_string(bkpinfo->backup_media_type),
					bkpinfo->media_device);
		} else {
			if (find_cdrw_device(bkpinfo->media_device)) {
				bkpinfo->media_device[0] = '\0';
			}
			if (bkpinfo->media_device[0]) {
				sprintf(tmp,
						"I think I've found your %s burner at SCSI node %s; am I right on the money?",
						media_descriptor_string(bkpinfo->
												backup_media_type),
						bkpinfo->media_device);
				if (!ask_me_yes_or_no(tmp)) {
					bkpinfo->media_device[0] = '\0';
				}
			}
			if (!bkpinfo->media_device[0]) {
				if (g_kernel_version < 2.6) {
					i = popup_and_get_string("Device node?",
											 "What is the SCSI node of your CD (re)writer, please?",
											 bkpinfo->media_device,
											 MAX_STR_LEN / 4);
				} else {
					i = popup_and_get_string("/dev entry?",
											 "What is the /dev entry of your CD (re)writer, please?",
											 bkpinfo->media_device,
											 MAX_STR_LEN / 4);
				}
				if (!i) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
			}
		}
		if (bkpinfo->backup_media_type == cdstream) {
			for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
				bkpinfo->media_size[i] = 650;
			}
		}
		break;
	case udev:
		if (!ask_me_yes_or_no
			("This option is for advanced users only. Are you sure?")) {
			log_to_screen("User has chosen not to backup the PC");
			finish(1);
		}
	case tape:

		if (find_tape_device_and_size(bkpinfo->media_device, sz_size)) {
			log_msg(3, "Ok, using vanilla scsi tape.");
			strcpy(bkpinfo->media_device, VANILLA_SCSI_TAPE);
			if ((fin = fopen(bkpinfo->media_device, "r"))) {
				paranoid_fclose(fin);
			} else {
				strcpy(bkpinfo->media_device, "/dev/osst0");
			}
		}
		if (bkpinfo->media_device[0]) {
			if ((fin = fopen(bkpinfo->media_device, "r"))) {
				paranoid_fclose(fin);
			} else {
				if (does_file_exist("/tmp/mondo-restore.cfg")) {
					read_cfg_var("/tmp/mondo-restore.cfg", "media-dev",
								 bkpinfo->media_device);
				}
			}
			sprintf(tmp,
					"I think I've found your tape streamer at %s; am I right on the money?",
					bkpinfo->media_device);
		}
		if (bkpinfo->media_device[0]) {
			sprintf(tmp,
					"I think I've found your tape streamer at %s; am I right on the money?",
					bkpinfo->media_device);
			if (!ask_me_yes_or_no(tmp)) {
				bkpinfo->media_device[0] = '\0';
			}
		}
		if (!bkpinfo->media_device[0]) {
			if (!popup_and_get_string
				("Device name?",
				 "What is the /dev entry of your tape streamer?",
				 bkpinfo->media_device, MAX_STR_LEN / 4)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
		}
		sprintf(tmp, "ls -l %s", bkpinfo->media_device);
		if (run_program_and_log_output(tmp, FALSE)) {
			log_to_screen("User has not specified a valid /dev entry");
			finish(1);
		}
		log_msg(4, "sz_size = %s", sz_size);
		sz_size[0] = '\0';
/*
	if ((size_sz[0]=='\0' || atol(size_sz)==0) && archiving_to_media)
	  {
	    if (!popup_and_get_string("Tape size", "How much COMPRESSED data will one of your tape cartridges hold? (e.g. 4GB for 4 gigabytes)", size_sz, 16))
	      { log_to_screen("User has chosen not to backup the PC"); finish(1); }
	  }
*/
		if (sz_size[0] == '\0') {
			bkpinfo->media_size[0] = 0;
		} else {
			bkpinfo->media_size[0] =
				friendly_sizestr_to_sizelong(sz_size) / 2 - 50;
		}
		log_msg(4, "media_size[0] = %ld", bkpinfo->media_size[0]);
		if (bkpinfo->media_size[0] <= 0) {
			bkpinfo->media_size[0] = 0;
		}
		for (i = 1; i <= MAX_NOOF_MEDIA; i++) {
			bkpinfo->media_size[i] = bkpinfo->media_size[0];
		}
		if (archiving_to_media) {
			if ((bkpinfo->compression_level =
				 which_compression_level()) == -1) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
		}
		break;



	case nfs:
		if (!bkpinfo->nfs_mount[0]) {
			strcpy(bkpinfo->nfs_mount,
				   call_program_and_get_last_line_of_output
				   ("mount | grep \":\" | cut -d' ' -f1 | head -n1"));
		}
#ifdef __FreeBSD__
		if (TRUE)
#else
		if (!bkpinfo->disaster_recovery)
#endif
		{
			if (!popup_and_get_string
				("NFS dir.",
				 "Please enter path and directory where archives are stored remotely. (Mondo has taken a guess at the correct value. If it is incorrect, delete it and type the correct one.)",
				 bkpinfo->nfs_mount, MAX_STR_LEN / 4)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			if (!bkpinfo->restore_data) {
				if ((bkpinfo->compression_level =
					 which_compression_level()) == -1) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
			}
			// check whether already mounted - we better remove
			// surrounding spaces and trailing '/' for this
			strip_spaces(bkpinfo->nfs_mount);
			if (bkpinfo->nfs_mount[strlen(bkpinfo->nfs_mount) - 1] == '/')
				bkpinfo->nfs_mount[strlen(bkpinfo->nfs_mount) - 1] = '\0';
			sprintf(command, "mount | grep \"%s \" | cut -d' ' -f3",
					bkpinfo->nfs_mount);
			strcpy(bkpinfo->isodir,
				   call_program_and_get_last_line_of_output(command));
		}
		if (bkpinfo->disaster_recovery) {
			system("umount /tmp/isodir 2> /dev/null");
			if (!popup_and_get_string
				("NFS share", "Which remote NFS share should I mount?",
				 bkpinfo->nfs_mount, MAX_STR_LEN)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
		}
		if (!is_this_device_mounted(bkpinfo->nfs_mount)) {
			sprintf(bkpinfo->isodir, "/tmp/isodir.mondo.%d",
					(int) (random() % 32768));
			sprintf(command, "mkdir -p %s", bkpinfo->isodir);
			run_program_and_log_output(command, 5);
			sprintf(tmp, "mount %s -t nfs %s", bkpinfo->nfs_mount,
					bkpinfo->isodir);
			run_program_and_log_output(tmp, 5);
			malloc_string(g_selfmounted_isodir);
			strcpy(g_selfmounted_isodir, bkpinfo->isodir);
		}
		if (!is_this_device_mounted(bkpinfo->nfs_mount)) {
			popup_and_OK
				("Please mount that partition before you try to backup to or restore from it.");
			finish(1);
		}
		strcpy(tmp, bkpinfo->nfs_remote_dir);
		if (!popup_and_get_string
			("Directory", "Which directory within that mountpoint?", tmp,
			 MAX_STR_LEN)) {
			log_to_screen("User has chosen not to backup the PC");
			finish(1);
		}
		strcpy(bkpinfo->nfs_remote_dir, tmp);
		// check whether writable - we better remove surrounding spaces for this
		strip_spaces(bkpinfo->nfs_remote_dir);
		sprintf(command, "echo hi > %s/%s/.dummy.txt", bkpinfo->isodir,
				bkpinfo->nfs_remote_dir);
		while (run_program_and_log_output(command, FALSE)) {
			strcpy(tmp, bkpinfo->nfs_remote_dir);
			sprintf(prompt,
					"Directory '%s' under mountpoint '%s' does not exist or is not writable. You can fix this or change the directory and retry or cancel the backup.",
					bkpinfo->nfs_remote_dir, bkpinfo->isodir);
			if (!popup_and_get_string
				("Directory", prompt, tmp, MAX_STR_LEN)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			strcpy(bkpinfo->nfs_remote_dir, tmp);
			// check whether writable - we better remove surrounding space s for this
			strip_spaces(bkpinfo->nfs_remote_dir);
			sprintf(command, "echo hi > %s/%s/.dummy.txt", bkpinfo->isodir,
					bkpinfo->nfs_remote_dir);
		}

		for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
			bkpinfo->media_size[i] = 650;
		}
		log_msg(3, "Just set nfs_remote_dir to %s",
				bkpinfo->nfs_remote_dir);
		log_msg(3, "isodir is still %s", bkpinfo->isodir);
		break;

	case iso:
		if (!bkpinfo->disaster_recovery) {
			if (!popup_and_get_string
				("Storage dir.",
				 "Please enter the full path that contains your ISO images.  Example: /mnt/raid0_0",
				 bkpinfo->isodir, MAX_STR_LEN / 4)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			if (archiving_to_media) {
				if ((bkpinfo->compression_level =
					 which_compression_level()) == -1) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
				if (!popup_and_get_string
					("ISO size.",
					 "Please enter how big you want each ISO image to be (in megabytes). This should be less than or equal to the size of the CD-R[W]'s or DVD's you plan to backup to.",
					 sz_size, 16)) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
				for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
					bkpinfo->media_size[i] = atoi(sz_size);
				}
				if (!popup_and_get_string
					("Prefix.",
					 "Please enter the prefix that will be prepended to your ISO filename.  Example: machine1 to obtain machine1-[1-9]*.iso files",
					 bkpinfo->prefix, MAX_STR_LEN / 4)) {
					log_to_screen("User has chosen not to backup the PC");
					finish(1);
				}
			} else {
				for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
					bkpinfo->media_size[i] = 650;
				}
			}
		}
		break;
	default:
		fatal_error
			("I, Mojo Jojo, shall defeat those pesky Powerpuff Girls!");
	}

	if (archiving_to_media) {

#ifdef __FreeBSD__
		strcpy(bkpinfo->boot_device,
			   call_program_and_get_last_line_of_output
			   ("mount | grep ' / ' | head -1 | cut -d' ' -f1 | sed 's/\\([0-9]\\).*/\\1/'"));
#else
		strcpy(bkpinfo->boot_device,
			   call_program_and_get_last_line_of_output
			   ("mount | grep ' / ' | head -1 | cut -d' ' -f1 | sed 's/[0-9].*//'"));
#endif
		i = which_boot_loader(bkpinfo->boot_device);
		if (i == 'U')			// unknown
		{

#ifdef __FreeBSD__
			if (!popup_and_get_string
				("Boot device",
				 "What is your boot device? (e.g. /dev/ad0)",
				 bkpinfo->boot_device, MAX_STR_LEN / 4)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			i = which_boot_loader(bkpinfo->boot_device);
#else
			if (!popup_and_get_string
				("Boot device",
				 "What is your boot device? (e.g. /dev/hda)",
				 bkpinfo->boot_device, MAX_STR_LEN / 4)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			if (does_string_exist_in_boot_block
				(bkpinfo->boot_device, "LILO")) {
				i = 'L';
			} else
				if (does_string_exist_in_boot_block
					(bkpinfo->boot_device, "ELILO")) {
				i = 'E';
			} else
				if (does_string_exist_in_boot_block
					(bkpinfo->boot_device, "GRUB")) {
				i = 'G';
			} else {
				i = 'U';
			}
#endif
			if (i == 'U') {
				if (ask_me_yes_or_no
					("Unidentified boot loader. Shall I restore it byte-for-byte at restore time and hope for the best?"))
				{
					i = 'R';	// raw
				} else {
					log_to_screen
						("I cannot find your boot loader. Please run mondoarchive with parameters.");
					finish(1);
				}
			}
		}
		bkpinfo->boot_loader = i;
		strcpy(bkpinfo->include_paths, "/");
		if (!popup_and_get_string
			("Backup paths",
			 "Please enter paths which you want me to backup. The default is '/' (i.e. everything).",
			 bkpinfo->include_paths, MAX_STR_LEN)) {
			log_to_screen("User has chosen not to backup the PC");
			finish(1);
		}
		strcpy(tmp, list_of_NFS_mounts_only());
		if (strlen(tmp) > 2) {
			if (bkpinfo->exclude_paths[0]) {
				strcat(bkpinfo->exclude_paths, " ");
			}
			strncpy(bkpinfo->exclude_paths, tmp, MAX_STR_LEN);
		}
// NTFS 
		strcpy(tmp,
			   call_program_and_get_last_line_of_output
			   ("parted2fdisk -l | grep -i ntfs | awk '{ print $1};' | tr -s '\\n' ' ' | awk '{ print $0};'"));
		if (strlen(tmp) > 2) {
			if (!popup_and_get_string
				("NTFS partitions",
				 "Please enter/confirm the NTFS partitions you wish to backup as well.",
				 tmp, MAX_STR_LEN / 4)) {
				log_to_screen("User has chosen not to backup the PC");
				finish(1);
			}
			strncpy(bkpinfo->image_devs, tmp, MAX_STR_LEN / 4);
		}


		if (!popup_and_get_string
			("Exclude paths",
			 "Please enter paths which you do NOT want to backup. Separate them with spaces. NB: /tmp and /proc are always excluded. :-) Just hit 'Enter' if you want to do a full system backup.",
			 bkpinfo->exclude_paths, MAX_STR_LEN)) {
			log_to_screen("User has chosen not to backup the PC");
			finish(1);
		}
		bkpinfo->make_cd_use_lilo = FALSE;
		bkpinfo->backup_data = TRUE;
		bkpinfo->verify_data =
			ask_me_yes_or_no
			("Will you want to verify your backups after Mondo has created them?");

#ifndef __FreeBSD__
		if (!ask_me_yes_or_no
			("Are you confident that your kernel is a sane, sensible, standard Linux kernel? Say 'no' if you are using a Gentoo <1.4 or Debian <3.0, please."))
#endif
		{
			strcpy(bkpinfo->kernel_path, "FAILSAFE");
		}

		if (!ask_me_yes_or_no
			("Are you sure you want to proceed? Hit 'no' to abort.")) {
			log_to_screen("User has chosen not to backup the PC");
			finish(1);
		}
	} else {
		bkpinfo->restore_data = TRUE;	// probably...
	}

	if (bkpinfo->backup_media_type == iso
		|| bkpinfo->backup_media_type == nfs) {
		g_ISO_restore_mode = TRUE;
	}
#ifdef __FreeSD__
// skip
#else
	if (bkpinfo->backup_media_type == nfs) {
		sprintf(tmp, "mount | grep \"%s\" | cut -d' ' -f3",
				bkpinfo->nfs_mount);
//      strcpy(bkpinfo->isodir, call_program_and_get_last_line_of_output(tmp));
		log_msg(3, "I think the NFS mount is mounted at %s",
				bkpinfo->isodir);
	}
	log_it("isodir = %s", bkpinfo->isodir);
	log_it("nfs_mount = '%s'", bkpinfo->nfs_mount);
#endif

	log_it("media device = %s", bkpinfo->media_device);
	log_it("media size = %ld", bkpinfo->media_size[1]);
	log_it("media type = %s",
		   bkptype_to_string(bkpinfo->backup_media_type));
	log_it("compression = %ld", bkpinfo->compression_level);
	log_it("include_paths = '%s'", bkpinfo->include_paths);
	log_it("exclude_paths = '%s'", bkpinfo->exclude_paths);
	log_it("scratchdir = '%s'", bkpinfo->scratchdir);
	log_it("tmpdir = '%s'", bkpinfo->tmpdir);
	log_it("boot_device = '%s' (loader=%c)", bkpinfo->boot_device,
		   bkpinfo->boot_loader);
	if (bkpinfo->media_size[0] < 0) {
		if (archiving_to_media) {
			fatal_error("Media size is less than zero.");
		} else {
			log_msg(2, "Warning - media size is less than zero.");
			bkpinfo->media_size[0] = 0;
		}
	}
	paranoid_free(tmp);
	paranoid_free(sz_size);
	paranoid_free(command);
	paranoid_free(comment);
	paranoid_free(prompt);
	return (0);
}




/**
 * @addtogroup utilityGroup
 * @{
 */
/**
 * Get a space-separated list of NFS devices and mounts.
 * @return The list created.
 * @note The return value points to static data that will be overwritten with each call.
 */
char *list_of_NFS_devices_and_mounts(void)
{
	char *exclude_these_devices;
	char *exclude_these_directories;
	static char result_sz[512];

	malloc_string(exclude_these_devices);
	malloc_string(exclude_these_directories);
	strcpy(exclude_these_directories,
		   call_program_and_get_last_line_of_output
		   ("mount -t coda,ncpfs,nfs,smbfs,cifs | tr -s '\t' ' ' | cut -d' ' -f3 | tr -s '\n' ' ' | awk '{print $0;}'"));
	strcpy(exclude_these_devices,
		   call_program_and_get_last_line_of_output
		   ("cat /etc/fstab | tr -s '\t' ' ' | grep -E '( (coda|ncpfs|nfs|smbfs|cifs) )' | cut -d' ' -f1 | tr -s '\n' ' ' | awk '{print $0;}'"));
	sprintf(result_sz, "%s %s", exclude_these_directories,
			exclude_these_devices);
	paranoid_free(exclude_these_devices);
	paranoid_free(exclude_these_directories);
	return (result_sz);
}




/**
 * Get a space-separated list of NFS mounts.
 * @return The list created.
 * @note The return value points to static data that will be overwritten with each call.
 * @bug Even though we only want the mounts, the devices are still checked.
 */
char *list_of_NFS_mounts_only(void)
{
	char *exclude_these_devices;
	char *exclude_these_directories;
	static char result_sz[512];

	malloc_string(exclude_these_devices);
	malloc_string(exclude_these_directories);
	strcpy(exclude_these_directories,
		   call_program_and_get_last_line_of_output
		   ("mount -t coda,ncpfs,nfs,smbfs,cifs | tr -s '\t' ' ' | cut -d' ' -f3 | tr -s '\n' ' ' | awk '{print $0;}'"));
	strcpy(exclude_these_devices,
		   call_program_and_get_last_line_of_output
		   ("cat /etc/fstab | tr -s '\t' ' ' | grep -E '( (coda|ncpfs|nfs|smbfs|cifs) )' | cut -d' ' -f1 | tr -s '\n' ' ' | awk '{print $0;}'"));
	sprintf(result_sz, "%s", exclude_these_directories);
	paranoid_free(exclude_these_devices);
	paranoid_free(exclude_these_directories);
	return (result_sz);
}

/* @} - end of utilityGroup */





/**
 * Create a randomly-named FIFO. The format is @p stub "." [random] [random] where
 * [random] is a random number between 1 and 32767.
 * @param store_name_here Where to store the new filename.
 * @param stub A random number will be appended to this to make the FIFO's name.
 * @ingroup deviceGroup
 */
void make_fifo(char *store_name_here, char *stub)
{
	char *tmp;

	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(stub);

	sprintf(store_name_here, "%s%d%d", stub, (int) (random() % 32768),
			(int) (random() % 32768));
	make_hole_for_file(store_name_here);
	mkfifo(store_name_here, S_IRWXU | S_IRWXG);
	sprintf(tmp, "chmod 770 %s", store_name_here);
	paranoid_system(tmp);
	paranoid_free(tmp);
}






/**
 * Set the tmpdir and scratchdir to reside on the partition with the most free space.
 * Automatically excludes DOS, NTFS, SMB, and NFS filesystems.
 * @param bkpinfo The backup information structure. @c bkpinfo->tmpdir and @c bkpinfo->scratchdir will be set.
 * @ingroup utilityGroup
 */
void sensibly_set_tmpdir_and_scratchdir(struct s_bkpinfo *bkpinfo)
{
	char *tmp, *command, *sz;

	malloc_string(tmp);
	malloc_string(command);
	malloc_string(sz);
	assert(bkpinfo != NULL);

#ifdef __FreeBSD__
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("df -m -t nonfs,msdosfs,ntfs,smbfs,smb,cifs | tr -s '\t' ' ' | grep -v none | grep -v Filesystem | awk '{printf \"%s %s\\n\", $4, $6;}' | sort -n | tail -n1 | awk '{print $NF;}'"));
#else
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("df -m -x nfs -x vfat -x ntfs -x smbfs -x smb -x cifs | sed 's/                  /devdev/' | tr -s '\t' ' ' | grep -v none | grep -v Filesystem | grep -v /dev/shm | awk '{printf \"%s %s\\n\", $4, $6;}' | sort -n | tail -n1 | awk '{print $NF;}'"));
#endif

	if (tmp[0] != '/') {
		strcpy(sz, tmp);
		strcpy(tmp, "/");
		strcat(tmp, sz);
	}
	if (!tmp[0]) {
		fatal_error("I couldn't figure out the tempdir!");
	}
	sprintf(bkpinfo->tmpdir, "%s/tmp.mondo.%d", tmp,
			(int) (random() % 32768));
	log_it("bkpinfo->tmpdir is being set to %s", bkpinfo->tmpdir);

	sprintf(bkpinfo->scratchdir, "%s/mondo.scratch.%d", tmp,
			(int) (random() % 32768));
	log_it("bkpinfo->scratchdir is being set to %s", bkpinfo->scratchdir);

	sprintf(g_erase_tmpdir_and_scratchdir, "rm -Rf %s %s", bkpinfo->tmpdir,
			bkpinfo->scratchdir);

	sprintf(command, "rm -Rf %s/tmp.mondo.* %s/mondo.scratch.*", tmp, tmp);
	paranoid_system(command);
	paranoid_free(tmp);
	paranoid_free(command);
	paranoid_free(sz);
}






/**
 * @addtogroup deviceGroup
 * @{
 */
/**
 * If we can read @p dev, set @p output to it.
 * If @p dev cannot be read, set @p output to "".
 * @param dev The device to check for.
 * @param output Set to @p dev if @p dev exists, "" otherwise.
 * @return TRUE if @p dev exists, FALSE if it doesn't.
 */
bool set_dev_to_this_if_rx_OK(char *output, char *dev)
{
	char *command;

	malloc_string(command);
	if (!dev || dev[0] == '\0') {
		output[0] = '\0';
		return (FALSE);
	}
//  assert_string_is_neither_NULL_nor_zerolength(dev);
	log_msg(10, "Injecting %s", dev);
	inject_device(dev);
	if (!does_file_exist(dev)) {
		log_msg(10, "%s doesn't exist. Returning FALSE.", dev);
		return (FALSE);
	}
	sprintf(command, "dd bs=%ld count=1 if=%s of=/dev/null &> /dev/null",
			512L, dev);
	if (!run_program_and_log_output(command, FALSE)
		&& !run_program_and_log_output(command, FALSE)) {
		strcpy(output, dev);
		log_msg(4, "Found it - %s", dev);
		return (TRUE);
	} else {
		output[0] = '\0';
		log_msg(4, "It's not %s", dev);
		return (FALSE);
	}
}





/**
 * Find out what number CD is in the drive.
 * @param bkpinfo The backup information structure. The @c bkpinfo->media_device field is the only one used.
 * @return The current CD number, or -1 if it could not be found.
 * @note If the CD is not mounted, it will be mounted
 * (and remain mounted after this function returns).
 */
int what_number_cd_is_this(struct s_bkpinfo *bkpinfo)
{
	int cd_number = -1;
	char *mountdev;
	char *tmp;

	malloc_string(mountdev);
	malloc_string(tmp);
	assert(bkpinfo != NULL);
//  log_it("Asking what_number_cd_is_this");
	if (g_ISO_restore_mode) {
		sprintf(tmp, "mount | grep iso9660 | awk '{print $3;}'");
//      log_it("tmp = %s", tmp);

		strcpy(mountdev, call_program_and_get_last_line_of_output(tmp));
		strcat(mountdev, "/archives/THIS-CD-NUMBER");
//      log_it("mountdev = %s", mountdev);
		cd_number = atoi(last_line_of_file(mountdev));
//      log_it("cd_number = %d", cd_number);
		paranoid_free(mountdev);
		paranoid_free(tmp);
		return (cd_number);
	}

	strcpy(mountdev, bkpinfo->media_device);
	if (!mountdev[0]) {
		log_it
			("(what_number_cd_is_this) Warning - media_device unknown. Finding out...");
		find_cdrom_device(bkpinfo->media_device, FALSE);
	}
	if (!is_this_device_mounted(MNT_CDROM)) {
		mount_CDROM_here(mountdev, MNT_CDROM);
	}
	cd_number =
		atoi(last_line_of_file(MNT_CDROM "/archives/THIS-CD-NUMBER"));
//  log_it("cd_number..later.. = %d", cd_number);
	paranoid_free(mountdev);
	paranoid_free(tmp);
	return (cd_number);
}







/**
 * Find out what device is mounted as root (/).
 * @return Root device.
 * @note The returned string points to static storage and will be overwritten with every call.
 * @bug A bit of a misnomer; it's actually finding out the root device.
 * The mountpoint (where it's mounted) will obviously be '/'.
 */
char *where_is_root_mounted()
{
	/*@ buffers **************** */
	static char tmp[MAX_STR_LEN];


#ifdef __FreeBSD__
	strcpy(tmp, call_program_and_get_last_line_of_output
		   ("mount | grep \" on / \" | cut -d' ' -f1"));
#else
	strcpy(tmp, call_program_and_get_last_line_of_output
		   ("mount | grep \" on / \" | cut -d' ' -f1 | sed s/[0-9]// | sed s/[0-9]//"));
	if (strstr(tmp, "/dev/cciss/")) {
		strcpy(tmp, call_program_and_get_last_line_of_output
			   ("mount | grep \" on / \" | cut -d' ' -f1 | cut -dp -f1"));
	}
	if (strstr(tmp, "/dev/md")) {
		strcpy(tmp,
			   call_program_and_get_last_line_of_output
			   ("mount | grep \" on / \" | cut -d' ' -f1"));
	}
#endif

	return (tmp);
}


/**
 * Find out which boot loader is in use.
 * @param which_device Device to look for the boot loader on.
 * @return 'L' for LILO, 'E'for ELILO, 'G' for GRUB, 'B' or 'D' for FreeBSD boot loaders, or 'U' for Unknown.
 * @note Under Linux, all drives are examined, not just @p which_device.
 */
#ifdef __FreeBSD__
char which_boot_loader(char *which_device)
{
	int count_lilos = 0;
	int count_grubs = 0;
	int count_boot0s = 0;
	int count_dangerouslydedicated = 0;

	log_it("looking at drive %s's MBR", which_device);
	if (does_string_exist_in_boot_block(which_device, "GRUB")) {
		count_grubs++;
	}
	if (does_string_exist_in_boot_block(which_device, "LILO")) {
		count_lilos++;
	}
	if (does_string_exist_in_boot_block(which_device, "Drive")) {
		count_boot0s++;
	}
	if (does_string_exist_in_first_N_blocks
		(which_device, "FreeBSD/i386", 17)) {
		count_dangerouslydedicated++;
	}
	log_it("%d grubs and %d lilos and %d elilos and %d boot0s and %d DD\n",
		   count_grubs, count_lilos, count_elilos, count_boot0s,
		   count_dangerouslydedicated);

	if (count_grubs && !count_lilos) {
		return ('G');
	} else if (count_lilos && !count_grubs) {
		return ('L');
	} else if (count_grubs == 1 && count_lilos == 1) {
		log_it("I'll bet you used to use LILO but switched to GRUB...");
		return ('G');
	} else if (count_boot0s == 1) {
		return ('B');
	} else if (count_dangerouslydedicated) {
		return ('D');
	} else {
		log_it("Unknown boot loader");
		return ('U');
	}
}

#else

char which_boot_loader(char *which_device)
{
	/*@ buffer ***************************************************** */
	char *list_drives_cmd;
	char *current_drive;

	/*@ pointers *************************************************** */
	FILE *pdrives;

	/*@ int ******************************************************** */
	int count_lilos = 0;
	int count_grubs = 0;

	/*@ end vars *************************************************** */

	malloc_string(list_drives_cmd);
	malloc_string(current_drive);

#ifdef __IA64__
	/* No choice for it */
	return ('E');
#endif
	assert(which_device != NULL);
	//  sprintf (list_drives_cmd,
	//       "fdisk -l | grep /dev | grep cyl | tr ':' ' ' | cut -d' ' -f2");

	sprintf(list_drives_cmd,
			// "parted2fdisk
			"fdisk -l 2>/dev/null | grep \"/dev/.*:\" | tr -s ':' ' ' | tr -s ' ' '\n' | grep /dev/; echo %s",
			where_is_root_mounted());
	log_it("list_drives_cmd = %s", list_drives_cmd);

	if (!(pdrives = popen(list_drives_cmd, "r"))) {
		log_OS_error("Unable to open list of drives");
		paranoid_free(list_drives_cmd);
		paranoid_free(current_drive);
		return ('\0');
	}
	for (fgets(current_drive, MAX_STR_LEN, pdrives); !feof(pdrives);
		 fgets(current_drive, MAX_STR_LEN, pdrives)) {
		strip_spaces(current_drive);
		log_it("looking at drive %s's MBR", current_drive);
		if (does_string_exist_in_boot_block(current_drive, "GRUB")) {
			count_grubs++;
			strcpy(which_device, current_drive);
			break;
		}
		if (does_string_exist_in_boot_block(current_drive, "LILO")) {
			count_lilos++;
			strcpy(which_device, current_drive);
			break;
		}
	}
	if (pclose(pdrives)) {
		log_OS_error("Cannot pclose pdrives");
	}
	log_it("%d grubs and %d lilos\n", count_grubs, count_lilos);
	paranoid_free(list_drives_cmd);
	paranoid_free(current_drive);
	if (count_grubs && !count_lilos) {
		return ('G');
	} else if (count_lilos && !count_grubs) {
		return ('L');
	} else if (count_grubs == 1 && count_lilos == 1) {
		log_it("I'll bet you used to use LILO but switched to GRUB...");
		return ('G');
	} else {
		log_it("Unknown boot loader");
		return ('U');
	}
}
#endif




/**
 * Write zeroes over the first 16K of @p device.
 * @param device The device to zero.
 * @return 0 for success, 1 for failure.
 */
int zero_out_a_device(char *device)
{
	FILE *fout;
	int i;

	assert_string_is_neither_NULL_nor_zerolength(device);

	log_it("Zeroing drive %s", device);
	if (!(fout = fopen(device, "w"))) {
		log_OS_error("Unable to open/write to device");
		return (1);
	}
	for (i = 0; i < 16384; i++) {
		fputc('\0', fout);
	}
	paranoid_fclose(fout);
	log_it("Device successfully zeroed.");
	return (0);
}

/**
 * Return the device pointed to by @p incoming.
 * @param incoming The device to resolve symlinks for.
 * @return The path to the real device file.
 * @note The returned string points to static storage that will be overwritten with each call.
 * @bug Won't work with file v4.0; needs to be written in C.
 */
char *resolve_softlinks_to_get_to_actual_device_file(char *incoming)
{
	static char output[MAX_STR_LEN];
	char *command;
	char *curr_fname;
	char *scratch;
	char *tmp;
	char *p;

	struct stat statbuf;
	command = malloc(1000);
	malloc_string(tmp);
	malloc_string(scratch);
	malloc_string(curr_fname);
	if (!does_file_exist(incoming)) {
		log_it
			("resolve_softlinks_to_get_to_actual_device_file --- device not found");
		strcpy(output, incoming);
	} else {
		strcpy(curr_fname, incoming);
		lstat(curr_fname, &statbuf);
		while (S_ISLNK(statbuf.st_mode)) {
			log_msg(1, "curr_fname = %s", curr_fname);
			sprintf(command, "file %s", curr_fname);
			strcpy(tmp, call_program_and_get_last_line_of_output(command));
			for (p = tmp + strlen(tmp); p != tmp && *p != '`' && *p != ' ';
				 p--);
			p++;
			strcpy(scratch, p);
			for (p = scratch; *p != '\0' && *p != '\''; p++);
			*p = '\0';
			log_msg(0, "curr_fname %s --> '%s' --> %s", curr_fname, tmp,
					scratch);
			if (scratch[0] == '/') {
				strcpy(curr_fname, scratch);	// copy whole thing because it's an absolute softlink
			} else {			// copy over the basename cos it's a relative softlink
				p = curr_fname + strlen(curr_fname);
				while (p != curr_fname && *p != '/') {
					p--;
				}
				if (*p == '/') {
					p++;
				}
				strcpy(p, scratch);
			}
			lstat(curr_fname, &statbuf);
		}
		strcpy(output, curr_fname);
		log_it("resolved %s to %s", incoming, output);
	}
	paranoid_free(command);
	paranoid_free(curr_fname);
	paranoid_free(tmp);
	return (output);
}

/* @} - end of deviceGroup */


/**
 * Return the type of partition format (GPT or MBR)
 */
char *which_partition_format(const char *drive)
{
	static char output[4];
	char *tmp;
	char *command;
	char *fdisk;
#ifdef __IA64__
	struct stat buf;
#endif

	malloc_string(tmp);
	malloc_string(command);
	malloc_string(fdisk);
	log_msg(0, "Looking for partition table format type");
// BERLIOS: Do that temporarily: we need to put back parted2fdisk everywhere
#ifdef __IA64__
	sprintf(fdisk, "/usr/local/bin/fdisk");
	if (stat(fdisk, &buf) != 0) {
#endif
		sprintf(fdisk, "/sbin/fdisk");
#ifdef __IA64__
	}
#endif
	log_msg(1, "Using %s", fdisk);
	sprintf(command, "%s -l %s | grep 'EFI GPT'", fdisk, drive);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	if (strstr(tmp, "GPT") == NULL) {
		strcpy(output, "MBR");
	} else {
		strcpy(output, "GPT");
	}
	log_msg(0, "Found %s partition table format type", output);
	paranoid_free(command);
	paranoid_free(tmp);
	paranoid_free(fdisk);
	return (output);
}

/* @} - end of deviceGroup */
