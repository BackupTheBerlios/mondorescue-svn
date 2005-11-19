/***************************************************************************
mondo-prep.c  -  description
-----------------

begin: Fri Apr 19 16:40:35 EDT 2002
copyright : (C) 2002 Mondo  Hugo Rabson
email     : Hugo Rabson <hugorabson@msn.com>
edited by : by Stan Benoit 4/2002
email     : troff@nakedsoul.org
cvsid     : $Id$
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* mondo-prep.c             Hugo Rabson



07/20
- when starting RAID, don't do it twice :)
- moved Joshua's new get_phys_size_of_drive() code
  from resize_drive... into get_phys_size_of_drive()

06/29
- make sure software RAID devices are formatted IF user says they're to be
- drivelist is struct now, not char[][]

06/26
- drop make_relevant_partition_bootable(); do it yourself in C (mostly)
- offer to reboot if partition table is locked up by the kernel

06/22
- be sure not to resize non-NTFS images when restoring
- set non-NTFS images' partition types properly

06/19
- shut down all LVMs and VGs before prepping

05/07
- usage of parted2fdisk instead of fdisk alone (ia32/ia64 compatibility)
  BCO

03/31
- rewrote partitioning and formatting code to call fdisk once per disk

10/21/2003
- suspend/resume Newt gui before/after calling do_my_funky_lvm_stuff()

10/20
- rewrote format_everything() - what a mess it was.
  It now does things in three phases:-
  - formats software RAID devices (/dev/md0, etc.)
  - formats and configures LVM devices
  - formats regular partitions (/dev/hda1, /dev/sdb2, etc.) 
    and any LVMs recently prepped

10/07
- use strstr(format, "raid") instead of strcmp(format,"raid") to determin
  if partition is a RAID component

09/23
- better comments

09/18
- better logging of RAID activity

05/05
- added Joshua Oreman's FreeBSD patches

04/30
- added textonly mode

04/24
- added lots of assert()'s and log_OS_error()'s

04/21
- format_everything() --- don't let bar go too far
- mkfs -c to check for bad blocks when formatting

04/04
- misc clean-up (Tom Mortell)

01/15/2003
- added code for LVM and SW Raid (Brian Borgeson)

12/10/2002
- line 1238: friendlier output

11/20
- when wiping a given device in preparation for partitioning + formatting
  it, don't wipe the MBR; just the partition table. That allows for
  stupid-ass Compaq users who like to play with their MBR's.
- disable mountlist.txt-->mountlist.txt.pre-resize copying (superfluous)

09/09
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_

07/01 - 07/31
- added hooks to libmondo
- RAID enhancements (Philippe de Muyter)

01/01 - 03/31
- partition_device() will refuse to partition /dev/mdX now (cos it's a
  virtual drive); however, it will return 0 (i.e. not an error)
- make_list_of_drives() will exclude /dev/md* from list
- mkreiserfs -ff instead of -q (Andy Glass)
- don't resize drive if drive not found (or if its size cannot be det'd)
- when generating list of drives from mountlist, skip the 'p' at the end
  of drive name if /dev/ida/ or /dev/cciss/; just do it (Michael Eisenberg)
- don't specify journal size when formatting ext3
  (used to have -Jsize=10 in the call to mkfs)
- handles files >2GB in size
- format interactively, if Interactive Mode
- removed reference to g_tape_size
- when executing /tmp/i-want-my-lvm, only record the error# if the command
  was _not_ an 'insmod' command
- pass partition size to fdisk in Kilobytes now, not Megabytes
- log fdisk's output to /tmp/mondo-restore.log if it fails
- don't try to format partitions of type 'image'
- don't type to set types of 'image' partitions
- if format code is 1 or 2 chars then assume it is a hex string
- took out all '/ /' comments
- don't extrapolate/add partition from RAID dev to mountlist if it's already
  present in mountlist
- less repetitive logging in the event of vacuum-packing of last part'n
- no extrapolation at all: RAID partitions should be listed in mountlist
  already, thanks to either Mindi v0.5x or the mountlist editor itself
- no longer say, 'and logging to...' when setting a partition's type
- don't run mkfs on RAID partitions (/dev/hd*, /dev/sd*); just set type
- the setting of a partition's type now takes place in a separate subroutine
  from the subroutine that actually creates the partition
- no need to set type if 83: 83 is the default (under fdisk)
- turned on '-Wall'; cleaned up some cruft
- if vacuum-packing partition (i.e. size=0MB --> max) then say, "(maximum)"
  not, "(0 MB)"

11/22/2001
- preliminary code review
- created on Nov 22nd, 2001
*/

/**
 * @file
 * Functions for prepping hard drives: partitioning, formatting, etc.
 */


#include "../common/my-stuff.h"
#include "../common/mondostructures.h"
#include "mondoprep.h"
#include "../common/libmondo.h"
#include "mondo-rstr-tools-EXT.h"
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <math.h>


#define FDISK_LOG "/tmp/fdisk.log"

#ifdef __FreeBSD__
#define DKTYPENAMES
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <err.h>
#include <libgen.h>
#define OSSWAP(x,y) y
#else
#define OSSWAP(x,y) x
#endif

#define ARCHIVES_PATH MNT_CDROM"/archives"
#define MONDO_WAS_HERE "MONDOWOZEREMONDOWOZEREMONDOWOZEREhahahaMOJOJOJO"

//static char cvsid[] = "$Id$";

extern char *g_mountlist_fname;
extern long g_current_progress, g_maximum_progress;

extern bool g_text_mode;

extern void pause_for_N_seconds(int, char *);


FILE *g_fprep = NULL;



int g_partition_table_locked_up = 0;










void wipe_MBRs_and_reboot_if_necessary(struct mountlist_itself *mountlist)
{
	char *command;
	char *tmp;
	int lino;
	int i;
	FILE *fout;
	char *buf;
	const int blocksize = 512;
	struct list_of_disks *drivelist = NULL;
// If LVMs are present and a zero-and-reboot wasn't recently undertaken
// then zero & insist on reboot.
	malloc_string(command);
	malloc_string(tmp);
	buf = malloc(blocksize);
	if (does_file_exist("/tmp/i-want-my-lvm"))	// FIXME - cheating :)
	{
		drivelist = malloc(sizeof(struct list_of_disks));
		make_list_of_drives_in_mountlist(mountlist, drivelist);
		for (lino = 0; lino < drivelist->entries; lino++) {
			sprintf(command,
					"dd if=%s bs=512 count=1 2> /dev/null | grep \"%s\"",
					drivelist->el[lino].device, MONDO_WAS_HERE);
			if (!run_program_and_log_output(command, 1)) {
				log_msg(1, "Found MONDO_WAS_HERE marker on drive#%d (%s)",
						lino, drivelist->el[lino].device);
				break;
			}
		}

		if (lino == drivelist->entries) {
// zero & reboot
			log_to_screen
				("I am sorry for the inconvenience but I must ask you to reboot.");
			log_to_screen
				("I need to reset the Master Boot Record; in order to be");
			log_to_screen
				("sure the kernel notices, I must reboot after doing it.");
			log_to_screen("Please hit 'Enter' to reboot.");
			for (lino = 0; lino < drivelist->entries; lino++) {
				for (i = 0; i < blocksize; i++) {
					buf[i] = 0;
				}
				sprintf(buf, "%s\n", MONDO_WAS_HERE);
				fout = fopen(drivelist->el[lino].device, "w+");
				if (!fout) {
					log_msg(1, "Unable to open+wipe %s",
							drivelist->el[lino].device);
				} else {
					if (1 != fwrite(buf, blocksize, 1, fout)) {
						log_msg(1, "Failed to wipe %s",
								drivelist->el[lino].device);
					} else {
						log_msg(1, "Successfully wiped %s",
								drivelist->el[lino].device);
					}
					fclose(fout);
				}
			}
			system("sync");
			system("sync");
			system("sync");
			popup_and_OK
				("I must now reboot. Please leave the boot media in the drive and repeat your actions - e.g. type 'nuke' - and it should work fine.");
			system("reboot");
		}
	}
// Still here? Cool!
	paranoid_free(command);
	paranoid_free(tmp);
	log_msg(1, "Cool. I didn't have to wipe anything.");
}






int fput_string_one_char_at_a_time(FILE * fout, char *str)
{
	int i, j;
	FILE *fq;

	if (ferror(fout)) {
		return (-1);
	}
	log_msg(5, "Writing string '%s', one char at a time", str);
	j = strlen(str);
	for (i = 0; i < j; i++) {
		log_msg(6, "Writing %d ('%c')", str[i], str[i]);
		if ((fq = fopen(FDISK_LOG, "a+"))) {
			fputc(str[i], fq);
			fclose(fq);
		}
		fputc(str[i], fout);
		fflush(fout);
		usleep(1000L * 100L);
		if (str[i] < 32) {
			usleep(1000L * 10L);
		}
	}
	log_msg(5, "Returning");

	return (i);
}










/**
 * @addtogroup prepGroup
 * @{
 */
/**
 * Execute the commands in /tmp/i-want-my-lvm.
 * These should probably be commands to set up LVM.
 * @return The number of errors encountered (0 for success).
 */


int do_my_funky_lvm_stuff(bool just_erase_existing_volumes,
						  bool vacuum_pack)
{
	/**  buffers **********************************************/
	char *tmp;
	char *incoming;
	char *command;
	char *lvscan_sz;
	char *lvremove_sz;
	char *pvscan_sz;
	char *vgscan_sz;
	char *vgcreate_sz;
	char *vgchange_sz;
	char *vgremove_sz;
//  char *do_this_last;

	/** char **************************************************/
	char *p;
	char *q;

	/** int ***************************************************/
	int retval = 0;
	int res = 0;
	int i;
	int lvmversion = 1;
	long extents;
	fpos_t orig_pos;

	/** pointers **********************************************/
	FILE *fin;

	/** end *****************************************************/

#ifdef __FreeBSD__
	return (0);
#endif

	if (!(fin = fopen("/tmp/i-want-my-lvm", "r"))) {
		log_OS_error("/tmp/i-want-my-lvm");
		return (1);
	}

	malloc_string(tmp);
	malloc_string(incoming);
	malloc_string(lvscan_sz);
	malloc_string(lvremove_sz);
	malloc_string(vgscan_sz);
	malloc_string(pvscan_sz);
	malloc_string(vgcreate_sz);
	malloc_string(vgchange_sz);
	malloc_string(vgremove_sz);
//  malloc_string(do_this_last); // postpone lvcreate call if necessary
	command = malloc(512);

//  do_this_last[0] = '\0';
	iamhere("STARTING");
	log_msg(1, "OK, opened i-want-my-lvm. Shutting down LVM volumes...");
	if (find_home_of_exe("lvm"))	// found it :) cool
	{
		strcpy(lvscan_sz, "lvm lvscan");
		strcpy(lvremove_sz, "lvm lvremove");
		strcpy(vgscan_sz, "lvm vgscan");
		strcpy(pvscan_sz, "lvm pvscan");
		strcpy(vgcreate_sz, "lvm vgcreate");
		strcpy(vgchange_sz, "lvm vgchange");
		strcpy(vgremove_sz, "lvm vgremove");
	} else {
		strcpy(lvscan_sz, "lvscan");
		strcpy(lvremove_sz, "lvremove");
		strcpy(vgscan_sz, "vgscan");
		strcpy(pvscan_sz, "pvscan");
		strcpy(vgcreate_sz, "vgcreate");
		strcpy(vgchange_sz, "vgchange");
		strcpy(vgremove_sz, "vgremove");
	}
	sprintf(command,
			"for i in `%s | cut -d\"'\" -f2 | sort -r` ; do echo \"Shutting down lv $i\" >> "
			MONDO_LOGFILE "; %s -f $i; done", lvscan_sz, lvremove_sz);
	run_program_and_log_output(command, 5);
	sleep(1);
	sprintf(command,
			"for i in `%s | grep -i lvm | cut -d'\"' -f2` ; do %s -a n $i ; %s $i; echo \"Shutting down vg $i\" >> "
			MONDO_LOGFILE "; done; %s -a n", vgscan_sz, vgchange_sz,
			vgremove_sz, vgremove_sz);
	run_program_and_log_output(command, 5);
	if (just_erase_existing_volumes) {
		paranoid_fclose(fin);
		log_msg(1, "Closed i-want-my-lvm. Finished erasing LVMs.");
		retval = 0;
		goto end_of_i_want_my_lvm;
	}

	log_msg(1, "OK, rewound i-want-my-lvm. Doing funky stuff...");
	rewind(fin);
	for (fgets(incoming, 512, fin); !feof(fin); fgets(incoming, 512, fin)) {
		fgetpos(fin, &orig_pos);
		if (incoming[0] != '#') {
			continue;
		}
		if (res && strstr(command, "create") && vacuum_pack) {
			sleep(2);
			system("sync");
			system("sync");
			system("sync");
		}
		if ((p = strstr(incoming, "vgcreate"))) {
// include next line(s) if they end in /dev (cos we've got a broken i-want-my-lvm)
			for (fgets(tmp, 512, fin); !feof(fin); fgets(tmp, 512, fin)) {
				if (tmp[0] == '#') {
					fsetpos(fin, &orig_pos);
					break;
				} else {
					fgetpos(fin, &orig_pos);
					strcat(incoming, tmp);
				}
			}
			for (q = incoming; *q != '\0'; q++) {
				if (*q < 32) {
					*q = ' ';
				}
			}
			strcpy(tmp, p + strlen("vgcreate") + 1);
			for (q = tmp; *q > 32; q++);
			*q = '\0';
			log_msg(1, "Deleting old entries at /dev/%s", tmp);
//             sprintf(command, "%s -f %s", vgremove_sz, tmp);
//             run_program_and_log_output(command, 1);
			sprintf(command, "rm -Rf /dev/%s", tmp);
			run_program_and_log_output(command, 1);
			run_program_and_log_output(vgscan_sz, 1);
			run_program_and_log_output(pvscan_sz, 1);
			log_msg(3,
					"After working around potentially broken i-want-my-lvm, incoming[] is now '%s'",
					incoming);
		}
		for (p = incoming + 1; *p == ' '; p++);
		strcpy(command, p);
		for (p = command; *p != '\0'; p++);
		for (; *(p - 1) < 32; p--);
		*p = '\0';
		res = run_program_and_log_output(command, 5);
		if (res > 0 && (p = strstr(command, "lvm "))) {
			*p = *(p + 1) = *(p + 2) = ' ';
			res = run_program_and_log_output(command, 5);
		}
		log_msg(0, "%s --> %d", command, res);
		if (res > 0) {
			res = 1;
		}
		if (res && strstr(command, "lvcreate") && vacuum_pack) {
			res = 0;
			if (strstr(command, "lvm lvcreate"))
				lvmversion = 2;
			log_it("%s... so I'll get creative.", tmp);
			if (lvmversion == 2) {
				strcpy(tmp, call_program_and_get_last_line_of_output
					   ("tail -n5 /var/log/mondo-archive.log | grep Insufficient | tail -n1"));
			} else {
				strcpy(tmp, call_program_and_get_last_line_of_output
					   ("tail -n5 /var/log/mondo-archive.log | grep lvcreate | tail -n1"));
			}
			for (p = tmp; *p != '\0' && !isdigit(*p); p++);
			extents = atol(p);
			log_msg(5, "p='%s' --> extents=%ld", p, extents);
			p = strstr(command, "-L");
			if (!p) {
				log_msg(0, "Fiddlesticks. '%s' returned %d", command, res);
			} else {
				if (lvmversion == 2) {
					*p++ = '-';
					*p++ = 'l';
					*p++ = ' ';
					for (q = p; *q != ' '; q++) {
						*q = ' ';
					}
					sprintf(p, "%ld", extents);
					i = strlen(p);
					*(p + i) = ' ';
				} else {
					p++;
					p++;
					p++;
					for (q = p; *q != ' '; q++) {
						*(q - 1) = ' ';
					}
					sprintf(p, "%ld%c", extents, 'm');
					i = strlen(p);
					*(p + i) = ' ';
				}
				log_msg(5, "Retrying with '%s'", command);
				res = run_program_and_log_output(command, 5);
				if (res > 0) {
					res = 1;
				}
				if (g_fprep) {
					fprintf(g_fprep, "%s\n", command);
				}
				log_msg(0, "%s --> %d", command, res);
				if (!res) {
					log_msg(5, "YAY! This time, it succeeded.");
				}
			}
		}
		if (strstr(command, "vgcreate")) {
			log_msg(0, "In case you're interested...");
			run_program_and_log_output(vgscan_sz, 1);
			run_program_and_log_output(pvscan_sz, 1);
		}
		if (res != 0 && !strstr(command, "insmod")) {
			retval++;
		}
		sprintf(tmp, "echo \"%s\" >> /tmp/out.sh", command);
		system(tmp);
		sleep(1);
	}
	paranoid_fclose(fin);
	log_msg(1, "Closed i-want-my-lvm. Finished doing funky stuff.");
  end_of_i_want_my_lvm:
	paranoid_free(tmp);
	paranoid_free(incoming);
	paranoid_free(command);
	paranoid_free(lvscan_sz);
	paranoid_free(lvremove_sz);
	paranoid_free(vgscan_sz);
	paranoid_free(pvscan_sz);
	paranoid_free(vgcreate_sz);
	paranoid_free(vgchange_sz);
	paranoid_free(vgremove_sz);
//  paranoid_free(do_this_last);
	system("sync");
	system("sync");
	system("sync");
	sleep(1);
	iamhere("ENDING");
	if (retval > 2) {
		log_msg(1, "%d errors. I'm reporting this.", retval);
		return (retval);
	} else {
		log_msg(1, "Not many errors. Returning 0.");
		return (0);
	}
}


/**
 * Add RAID partitions while copying @p old_mountlist to @p new_mountlist.
 * We go through @p old_mountlist and check if any RAID device (/dev/md? on Linux)
 * is in it; if it is, then we put the disks contained within that RAID device
 * into the mountlist as well.
 * @param old_mountlist The mountlist to read.
 * @param new_mountlist The mountlist to write, with the RAID partitions added.
 * @return 0 for success, nonzero for failure.
 */
int extrapolate_mountlist_to_include_raid_partitions(struct mountlist_itself
													 *new_mountlist, struct mountlist_itself
													 *old_mountlist)
{
	/** pointers *********************************************************/
	FILE *fin;

	/** int **************************************************************/
	int lino;
	int j;

	/** buffers **********************************************************/
	char *incoming;
	char *tmp;

	/** pointers *********************************************************/
	char *p;

	/** init *************************************************************/
	new_mountlist->entries = 0;

	/** end **************************************************************/

	malloc_string(incoming);
	malloc_string(tmp);
	assert(new_mountlist != NULL);
	assert(old_mountlist != NULL);

#ifdef __FreeBSD__
	log_to_screen
		("I don't know how to extrapolate the mountlist on FreeBSD. Sorry.");
	return (1);
#endif

	for (lino = 0; lino < old_mountlist->entries; lino++) {
		if (strstr(old_mountlist->el[lino].device, RAID_DEVICE_STUB))	// raid
		{
			if (!does_file_exist("/etc/raidtab")) {
				log_to_screen
					("Cannot find /etc/raidtab - cannot extrapolate the fdisk entries");
				finish(1);
			}
			if (!(fin = fopen("/etc/raidtab", "r"))) {
				log_OS_error("Cannot open /etc/raidtab");
				finish(1);
			}
			for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin)
				 && !strstr(incoming, old_mountlist->el[lino].device);
				 fgets(incoming, MAX_STR_LEN - 1, fin));
			if (!feof(fin)) {
				sprintf(tmp, "Investigating %s",
						old_mountlist->el[lino].device);
				log_it(tmp);
				for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin)
					 && !strstr(incoming, "raiddev");
					 fgets(incoming, MAX_STR_LEN - 1, fin)) {
					if (strstr(incoming, OSSWAP("device", "drive"))
						&& !strchr(incoming, '#')) {
						for (p = incoming + strlen(incoming);
							 *(p - 1) <= 32; p--);
						*p = '\0';
						for (p--; p > incoming && *(p - 1) > 32; p--);
						sprintf(tmp, "Extrapolating %s", p);
						log_it(tmp);
						for (j = 0;
							 j < new_mountlist->entries
							 && strcmp(new_mountlist->el[j].device, p);
							 j++);
						if (j >= new_mountlist->entries) {
							strcpy(new_mountlist->
								   el[new_mountlist->entries].device, p);
							strcpy(new_mountlist->
								   el[new_mountlist->entries].mountpoint,
								   "raid");
							strcpy(new_mountlist->
								   el[new_mountlist->entries].format,
								   "raid");
							new_mountlist->el[new_mountlist->entries].
								size = old_mountlist->el[lino].size;
							new_mountlist->entries++;
						} else {
							sprintf(tmp,
									"Not adding %s to mountlist: it's already there",
									p);
							log_it(tmp);
						}
					}
				}
			}
			paranoid_fclose(fin);
		} else {
			strcpy(new_mountlist->el[new_mountlist->entries].device,
				   old_mountlist->el[lino].device);
			strcpy(new_mountlist->el[new_mountlist->entries].mountpoint,
				   old_mountlist->el[lino].mountpoint);
			strcpy(new_mountlist->el[new_mountlist->entries].format,
				   old_mountlist->el[lino].format);
			new_mountlist->el[new_mountlist->entries].size =
				old_mountlist->el[lino].size;
			new_mountlist->entries++;
		}
	}
	paranoid_free(incoming);
	paranoid_free(tmp);

	return (0);
}


/**
 * Format @p device as a @p format filesystem.
 * This will use the format command returned by which_format_command_do_i_need().
 * If @p device is an LVM PV, it will not be formatted, and LVM will be started
 * (if not already done). If it's an imagedev, software RAID component, or
 * (under BSD) swap partition, no format will be done.
 * @param device The device to format.
 * @param format The filesystem type to format it as.
 * @return 0 for success, nonzero for failure.
 */
int format_device(char *device, char *format)
{
	/** int **************************************************************/
	int res;
	int retval = 0;
#ifdef __FreeBSD__
	static bool vinum_started_yet = FALSE;
#endif

	/** buffers ***********************************************************/
	char *program;
	char *tmp;

	/** end ****************************************************************/

	malloc_string(program);
	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(device);
	assert(format != NULL);

	if (strstr(format, "raid")) {	// do not form RAID disks; do it to /dev/md* instead
		sprintf(tmp, "Not formatting %s (it is a RAID disk)", device);
		log_it(tmp);
		paranoid_free(program);
		paranoid_free(tmp);
		return (0);
	}
#ifdef __FreeBSD__
	if (strcmp(format, "swap") == 0) {
		log_it("Not formatting %s - it's swap", device);
		paranoid_free(program);
		paranoid_free(tmp);
		return (0);
	}
#endif
	if (strlen(format) <= 2) {
		sprintf(tmp,
				"%s has a really small format type ('%s') - this is probably a hexadecimal string, which would suggest the partition is an image --- I shouldn't format it",
				device, format);
		log_it(tmp);
		paranoid_free(program);
		paranoid_free(tmp);
		return (0);
	}
	if (is_this_device_mounted(device)) {
		sprintf(tmp, "%s is mounted - cannot format it       ", device);
		log_to_screen(tmp);
		paranoid_free(program);
		paranoid_free(tmp);
		return (1);
	}
	if (strstr(device, RAID_DEVICE_STUB)) {
		newtSuspend();
#ifdef __FreeBSD__
		if (!vinum_started_yet) {
			if (!does_file_exist("/tmp/raidconf.txt")) {
				log_to_screen
					("/tmp/raidconf.txt does not exist. I therefore cannot start Vinum.");
			} else {
				int res;
				res =
					run_program_and_log_output
					("vinum create /tmp/raidconf.txt", TRUE);
				if (res) {
					log_to_screen
						("`vinum create /tmp/raidconf.txt' returned errors. Please fix them and re-run mondorestore.");
					finish(1);
				}
				vinum_started_yet = TRUE;
			}
		}

		if (vinum_started_yet) {
			FILE *fin;
			char line[MAX_STR_LEN];
			sprintf(tmp,
					"Initializing Vinum device %s (this may take a *long* time)",
					device);
			log_to_screen(tmp);
			/* format raid partition */
			//      sprintf (program, "mkraid --really-force %s", device); --- disabled -- BB, 02/12/2003
			sprintf(program,
					"for plex in `vinum lv -r %s | grep '^P' | tr '\t' ' ' | tr -s ' ' | cut -d' ' -f2`; do echo $plex; done > /tmp/plexes",
					basename(device));
			system(program);
			if (g_fprep) {
				fprintf(g_fprep, "%s\n", program);
			}
			fin = fopen("/tmp/plexes", "r");
			while (fgets(line, MAX_STR_LEN - 1, fin)) {
				if (strchr(line, '\n'))
					*(strchr(line, '\n')) = '\0';	// get rid of the \n on the end

				sprintf(tmp, "Initializing plex: %s", line);
				open_evalcall_form(tmp);
				sprintf(tmp, "vinum init %s", line);
				system(tmp);
				while (1) {
					sprintf(tmp,
							"vinum lp -r %s | grep '^S' | head -1 | tr -s ' ' | cut -d: -f2 | cut -f1 | sed 's/^ //' | sed 's/I //' | sed 's/%%//'",
							line);
					FILE *pin = popen(tmp, "r");
					char status[MAX_STR_LEN / 4];
					fgets(status, MAX_STR_LEN / 4 - 1, pin);
					pclose(pin);

					if (!strcmp(status, "up")) {
						break;	/* it's done */
					}
					update_evalcall_form(atoi(status));
					usleep(250000);
				}
				close_evalcall_form();
			}
			fclose(fin);
			unlink("/tmp/plexes");
			/* retval+=res; */
		}
#else
		sprintf(tmp, "Initializing RAID device %s", device);
		log_to_screen(tmp);

// Shouldn't be necessary.
		log_to_screen("Stopping %s", device);
		stop_raid_device(device);
		system("sync");
		sleep(1);
		if (g_fprep) {
			fprintf(g_fprep, "%s\n", program);
		}

		log_msg(1, "Making %s", device);
		sprintf(program, "mkraid --really-force %s", device);
		res = run_program_and_log_output(program, 1);
		log_msg(1, "%s returned %d", program, res);
		system("sync");
		sleep(3);
		start_raid_device(device);
		if (g_fprep) {
			fprintf(g_fprep, "%s\n", program);
		}
		system("sync");
		sleep(2);

//      log_to_screen("Starting %s", device);
//      sprintf(program, "raidstart %s", device);
//      res = run_program_and_log_output(program, 1);
//      log_msg(1, "%s returned %d", program, res);
//      system("sync"); sleep(1);
		if (g_fprep) {
			fprintf(g_fprep, "%s\n", program);
		}
#endif
		system("sync");
		sleep(1);
		newtResume();
	}
//#ifndef __FreeBSD__
//#endif

	if (!strcmp(format, "lvm")) {
		log_msg(1, "Don't format %s - it's part of an lvm volume", device);
		paranoid_free(program);
		paranoid_free(tmp);
		return (0);
	}
	res = which_format_command_do_i_need(format, program);
	sprintf(tmp, "%s %s", program, device);
	if (strstr(program, "kludge")) {
		strcat(tmp, " /");
	}
	sprintf(program, "sh -c 'echo -en \"y\\ny\\ny\\n\" | %s'", tmp);
	sprintf(tmp, "Formatting %s as %s", device, format);
	update_progress_form(tmp);
	res = run_program_and_log_output(program, FALSE);
	if (res && strstr(program, "kludge")) {
		sprintf(tmp, "Kludge failed; using regular mkfs.%s to format %s",
				format, device);
#ifdef __FreeBSD__
		sprintf(program, "newfs_msdos -F 32 %s", device);
#else
#ifdef __IA64__
		/* For EFI partitions take fat16 
		 * as we want to make small ones */
		sprintf(program, "mkfs -t %s -F 16 %s", format, device);
#else
		sprintf(program, "mkfs -t %s -F 32 %s", format, device);
#endif
#endif
		res = run_program_and_log_output(program, FALSE);
		if (g_fprep) {
			fprintf(g_fprep, "%s\n", program);
		}
	}
	retval += res;
	if (retval) {
		strcat(tmp, "...failed");
	} else {
		strcat(tmp, "...OK");
	}

	log_to_screen(tmp);
	paranoid_free(program);
	paranoid_free(tmp);
	system("sync");
	sleep(1);
	return (retval);
}





/**
 * Format all drives (except those excluded by format_device()) in @p mountlist.
 * @param mountlist The mountlist containing partitions to be formatted.
 * @param interactively If TRUE, then prompt the user before each partition.
 * @return The number of errors encountered (0 for success).
 */
int format_everything(struct mountlist_itself *mountlist,
					  bool interactively)
{
	/** int **************************************************************/
	int retval = 0;
	int lino;
	int res;
//  int i;
//  struct list_of_disks *drivelist;

	/** long *************************************************************/
	long progress_step;

	/** bools ************************************************************/
	bool do_it;

	/** buffers **********************************************************/
	char *tmp;

	/** pointers *********************************************************/
	struct mountlist_line *me;	// mountlist entry
	/** end **************************************************************/

	assert(mountlist != NULL);
	malloc_string(tmp);
	sprintf(tmp, "format_everything (mountlist, interactively = %s",
			(interactively) ? "true" : "false");
	log_it(tmp);
	mvaddstr_and_log_it(g_currentY, 0, "Formatting partitions     ");
	open_progress_form("Formatting partitions",
					   "I am now formatting your hard disk partitions.",
					   "This may take up to five minutes.", "",
					   mountlist->entries + 1);

	progress_step =
		(mountlist->entries >
		 0) ? g_maximum_progress / mountlist->entries : 1;
// start soft-raids now (because LVM might depend on them)
// ...and for simplicity's sake, let's format them at the same time :)
	log_msg(1, "Stopping all RAID devices");
	stop_all_raid_devices(mountlist);
	system("sync");
	system("sync");
	system("sync");
	sleep(2);
	log_msg(1, "Prepare soft-RAIDs");	// prep and format too
	for (lino = 0; lino < mountlist->entries; lino++) {
		me = &mountlist->el[lino];	// the current mountlist entry
		log_msg(2, "Examining %s", me->device);
		if (!strncmp(me->device, "/dev/md", 7)) {
			if (interactively) {
				// ask user if we should format the current device
				sprintf(tmp, "Shall I format %s (%s) ?", me->device,
						me->mountpoint);
				do_it = ask_me_yes_or_no(tmp);
			} else {
				do_it = TRUE;
			}
			if (do_it) {
				// NB: format_device() also stops/starts RAID device if necessary
				retval += format_device(me->device, me->format);
			}
			g_current_progress += progress_step;
		}
	}
	system("sync");
	system("sync");
	system("sync");
	sleep(2);
// This last step is probably necessary
//  log_to_screen("Re-starting software RAIDs...");
//  start_all_raid_devices(mountlist);
//  system("sync"); system("sync"); system("sync"); 
//  sleep(5);
// do LVMs now
	log_msg(1, "Creating LVMs");
	if (does_file_exist("/tmp/i-want-my-lvm")) {
		wait_until_software_raids_are_prepped("/proc/mdstat", 10);
		log_to_screen("Configuring LVM");
		if (!g_text_mode) {
			newtSuspend();
		}
/*
		for(i=0; i<3; i++)
		  {
		    res = do_my_funky_lvm_stuff(FALSE, FALSE);
		    if (!res) { break; }
		    sleep(3);
		    res = do_my_funky_lvm_stuff(TRUE, FALSE);
		    sleep(3);
		  }
		if (res) {
			log_msg(1, "Vacuum-packing...");
*/
		res = do_my_funky_lvm_stuff(FALSE, TRUE);
/*
		}
*/
		if (!g_text_mode) {
			newtResume();
		}
		if (!res) {
			log_to_screen("LVM initialized OK");
		} else {
			log_to_screen("Failed to initialize LVM");
		}
		// retval += res;
		if (res) {
			retval++;
		}
		sleep(3);
	}
// do regulars at last
	sleep(2);					// woo!
	log_msg(1, "Formatting regulars");
	for (lino = 0; lino < mountlist->entries; lino++) {
		me = &mountlist->el[lino];	// the current mountlist entry
		if (!strcmp(me->mountpoint, "image")) {
			sprintf(tmp, "Not formatting %s - it's an image", me->device);
			log_it(tmp);
		} else if (!strcmp(me->format, "raid")) {
			sprintf(tmp, "Not formatting %s - it's a raid-let",
					me->device);
			log_it(tmp);
			continue;
		} else if (!strcmp(me->format, "lvm")) {
			sprintf(tmp, "Not formatting %s - it's an LVM", me->device);
			log_it(tmp);
			continue;
		} else if (!strncmp(me->device, "/dev/md", 7)) {
			sprintf(tmp, "Already formatted %s - it's a soft-RAID dev",
					me->device);
			log_it(tmp);
			continue;
		} else if (!does_file_exist(me->device)
				   && strncmp(me->device, "/dev/hd", 7)
				   && strncmp(me->device, "/dev/sd", 7)) {
			sprintf(tmp,
					"Not formatting %s yet - doesn't exist - probably an LVM",
					me->device);
			log_it(tmp);
			continue;
		} else {
			if (interactively) {
				// ask user if we should format the current device
				sprintf(tmp, "Shall I format %s (%s) ?", me->device,
						me->mountpoint);
				do_it = ask_me_yes_or_no(tmp);
			} else {
				do_it = TRUE;
			}

			if (do_it)
				retval += format_device(me->device, me->format);
		}

		// update progress bar
		g_current_progress += progress_step;
	}


	// update progress bar to 100% to compensate for
	// rounding errors of the progress_step calculation
	if (lino >= mountlist->entries)
		g_current_progress = g_maximum_progress;

	close_progress_form();

	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
		log_to_screen
			("Errors occurred during the formatting of your hard drives.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}

	sprintf(tmp, "format_everything () - %s",
			(retval) ? "failed!" : "finished successfully");
	log_it(tmp);

	if (g_partition_table_locked_up > 0) {
		if (retval > 0 && !interactively) {
//123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
			log_to_screen
				("Partition table locked up %d times. At least one 'mkfs' (format) command",
				 g_partition_table_locked_up);
			log_to_screen
				("failed. I think these two events are related. Sometimes, fdisk's ioctl() call");
			log_to_screen
				("to refresh its copy of the partition table causes the kernel to lock the ");
			log_to_screen
				("partition table. I believe this has just happened.");
			if (ask_me_yes_or_no
				("Please choose 'yes' to reboot and try again; or 'no' to ignore this warning and continue."))
			{
				system("sync");
				system("sync");
				system("sync");
				system("reboot");
			}
		} else {
			log_to_screen
				("Partition table locked up %d time%c. However, disk formatting succeeded.",
				 g_partition_table_locked_up,
				 (g_partition_table_locked_up == 1) ? '.' : 's');
		}
	}
	newtSuspend();
	system("clear");
	newtResume();
	paranoid_free(tmp);
	return (retval);
}


/**
 * Create small dummy partitions to fill in the gaps in partition numbering for @p drivename.
 * Each partition created is 32k in size.
 * @param drivename The drive to create the dummy partitions on.
 * @param devno_we_must_allow_for The lowest-numbered real partition; create
 * dummies up to (this - 1).
 * @return The number of errors encountered (0 for success).
 */
int make_dummy_partitions(FILE * pout_to_fdisk, char *drivename,
						  int devno_we_must_allow_for)
{
	/** int **************************************************************/
	int current_devno;
	int previous_devno;
	int retval = 0;
	int res;

	/** buffers **********************************************************/
	char *tmp;

	/** end **************************************************************/

	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(drivename);

	if (devno_we_must_allow_for >= 5) {
		sprintf(tmp, "Making dummy primary %s%d", drivename, 1);
		log_it(tmp);
		g_maximum_progress++;
		res =
			partition_device(pout_to_fdisk, drivename, 1, 0, "ext2",
							 32000);
		retval += res;
		previous_devno = 1;
		current_devno = 5;
	} else {
		previous_devno = 0;
		current_devno = 1;
	}
	for (; current_devno < devno_we_must_allow_for; current_devno++) {
		sprintf(tmp, "Creating dummy partition %s%d", drivename,
				current_devno);
		log_it(tmp);
		g_maximum_progress++;
		res =
			partition_device(pout_to_fdisk, drivename, current_devno,
							 previous_devno, OSSWAP("ext2", "ufs"), 32000);
		retval += res;
		previous_devno = current_devno;
	}
	paranoid_free(tmp);
	return (previous_devno);
}


/**
 * Decide whether @p mountlist contains any RAID devices.
 * @param mountlist The mountlist to examine.
 * @return TRUE if it does, FALSE if it doesn't.
 */
bool mountlist_contains_raid_devices(struct mountlist_itself * mountlist)
{
	/** int *************************************************************/
	int i;
	int matching = 0;

	/** end **************************************************************/

	assert(mountlist != NULL);

	for (i = 0; i < mountlist->entries; i++) {
		if (strstr(mountlist->el[i].device, RAID_DEVICE_STUB)) {
			matching++;
		}
	}
	if (matching) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

/* The following 2 functions are stolen from /usr/src/sbin/disklabel/disklabel.c */
#ifdef __FreeBSD__
static void display_disklabel(FILE * f, const struct disklabel *lp)
{
	int i, j;
	const struct partition *pp;

	fprintf(f, "# %s\n", "Generated by Mondo Rescue");
	if (lp->d_type < DKMAXTYPES)
		fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		fprintf(f, "type: %u\n", lp->d_type);
	fprintf(f, "disk: %.*s\n", (int) sizeof(lp->d_typename),
			lp->d_typename);
	fprintf(f, "label: %.*s\n", (int) sizeof(lp->d_packname),
			lp->d_packname);
	fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		fprintf(f, " removeable");
	if (lp->d_flags & D_ECC)
		fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		fprintf(f, " badsect");
	fprintf(f, "\n");
	fprintf(f, "bytes/sector: %lu\n", (u_long) lp->d_secsize);
	fprintf(f, "sectors/track: %lu\n", (u_long) lp->d_nsectors);
	fprintf(f, "tracks/cylinder: %lu\n", (u_long) lp->d_ntracks);
	fprintf(f, "sectors/cylinder: %lu\n", (u_long) lp->d_secpercyl);
	fprintf(f, "cylinders: %lu\n", (u_long) lp->d_ncylinders);
	fprintf(f, "sectors/unit: %lu\n", (u_long) lp->d_secperunit);
	fprintf(f, "rpm: %u\n", lp->d_rpm);
	fprintf(f, "interleave: %u\n", lp->d_interleave);
	fprintf(f, "trackskew: %u\n", lp->d_trackskew);
	fprintf(f, "cylinderskew: %u\n", lp->d_cylskew);
	fprintf(f, "headswitch: %lu\t\t# milliseconds\n",
			(u_long) lp->d_headswitch);
	fprintf(f, "track-to-track seek: %ld\t# milliseconds\n",
			(u_long) lp->d_trkseek);
	fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		fprintf(f, "%lu ", (u_long) lp->d_drivedata[j]);
	fprintf(f, "\n\n%u partitions:\n", lp->d_npartitions);
	fprintf(f,
			"#        size   offset    fstype   [fsize bsize bps/cpg]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			fprintf(f, "  %c: %8lu %8lu  ", 'a' + i, (u_long) pp->p_size,
					(u_long) pp->p_offset);
			if (pp->p_fstype < FSMAXTYPES)
				fprintf(f, "%8.8s", fstypenames[pp->p_fstype]);
			else
				fprintf(f, "%8d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_UNUSED:	/* XXX */
				fprintf(f, "    %5lu %5lu %5.5s ", (u_long) pp->p_fsize,
						(u_long) (pp->p_fsize * pp->p_frag), "");
				break;

			case FS_BSDFFS:
				fprintf(f, "    %5lu %5lu %5u ", (u_long) pp->p_fsize,
						(u_long) (pp->p_fsize * pp->p_frag), pp->p_cpg);
				break;

			case FS_BSDLFS:
				fprintf(f, "    %5lu %5lu %5d", (u_long) pp->p_fsize,
						(u_long) (pp->p_fsize * pp->p_frag), pp->p_cpg);
				break;

			default:
				fprintf(f, "%20.20s", "");
				break;
			}
			fprintf(f, "\t# (Cyl. %4lu",
					(u_long) (pp->p_offset / lp->d_secpercyl));
			if (pp->p_offset % lp->d_secpercyl)
				putc('*', f);
			else
				putc(' ', f);
			fprintf(f, "- %lu",
					(u_long) ((pp->p_offset + pp->p_size +
							   lp->d_secpercyl - 1) / lp->d_secpercyl -
							  1));
			if (pp->p_size % lp->d_secpercyl)
				putc('*', f);
			fprintf(f, ")\n");
		}
	}
	fflush(f);
}

static struct disklabel *get_virgin_disklabel(char *dkname)
{
	static struct disklabel loclab;
	struct partition *dp;
	char lnamebuf[BBSIZE];
	int f;
	u_int secsize, u;
	off_t mediasize;

	(void) snprintf(lnamebuf, BBSIZE, "%s", dkname);
	if ((f = open(lnamebuf, O_RDONLY)) == -1) {
		warn("cannot open %s", lnamebuf);
		return (NULL);
	}

	/* New world order */
	if ((ioctl(f, DIOCGMEDIASIZE, &mediasize) != 0)
		|| (ioctl(f, DIOCGSECTORSIZE, &secsize) != 0)) {
		close(f);
		return (NULL);
	}
	memset(&loclab, 0, sizeof loclab);
	loclab.d_magic = DISKMAGIC;
	loclab.d_magic2 = DISKMAGIC;
	loclab.d_secsize = secsize;
	loclab.d_secperunit = mediasize / secsize;

	/*
	 * Nobody in these enligthened days uses the CHS geometry for
	 * anything, but nontheless try to get it right.  If we fail
	 * to get any good ideas from the device, construct something
	 * which is IBM-PC friendly.
	 */
	if (ioctl(f, DIOCGFWSECTORS, &u) == 0)
		loclab.d_nsectors = u;
	else
		loclab.d_nsectors = 63;
	if (ioctl(f, DIOCGFWHEADS, &u) == 0)
		loclab.d_ntracks = u;
	else if (loclab.d_secperunit <= 63 * 1 * 1024)
		loclab.d_ntracks = 1;
	else if (loclab.d_secperunit <= 63 * 16 * 1024)
		loclab.d_ntracks = 16;
	else
		loclab.d_ntracks = 255;
	loclab.d_secpercyl = loclab.d_ntracks * loclab.d_nsectors;
	loclab.d_ncylinders = loclab.d_secperunit / loclab.d_secpercyl;
	loclab.d_npartitions = MAXPARTITIONS;

	/* Various (unneeded) compat stuff */
	loclab.d_rpm = 3600;
	loclab.d_bbsize = BBSIZE;
	loclab.d_interleave = 1;;
	strncpy(loclab.d_typename, "amnesiac", sizeof(loclab.d_typename));

	dp = &loclab.d_partitions[RAW_PART];
	dp->p_size = loclab.d_secperunit;
	loclab.d_checksum = dkcksum(&loclab);
	close(f);
	return (&loclab);
}

/* End stolen from /usr/src/sbin/disklabel/disklabel.c. */

char *canonical_name(char *drivename)
{
	if (drivename) {
		if (strncmp(drivename, "/dev/", 5) == 0) {
			return drivename + 5;
		}
	}
	return drivename;
}

/**
 * (BSD only) Create a disklabel on @p drivename according to @p mountlist.
 * @param mountlist The mountlist to get the subpartition information from.
 * @param drivename The drive or slice to create a disklabel on.
 * @param ret If non-NULL, store the created disklabel here.
 * @return The number of errors encountered (0 for success).
 */
int label_drive_or_slice(struct mountlist_itself *mountlist,
						 char *drivename, struct disklabel *ret)
{
	char subdev_str[MAX_STR_LEN];
	char command[MAX_STR_LEN];
	struct disklabel *lp;
	int i, lo = 0;
	int retval = 0;
	char c;
	FILE *ftmp;

	lp = get_virgin_disklabel(drivename);
	for (c = 'a'; c <= 'z'; ++c) {
		int idx;
		sprintf(subdev_str, "%s%c", drivename, c);
		if ((idx = find_device_in_mountlist(mountlist, subdev_str)) < 0) {
			lp->d_partitions[c - 'a'].p_size = 0;
			lp->d_partitions[c - 'a'].p_fstype = FS_UNUSED;
		} else {
			lo = c - 'a';
			lp->d_partitions[c - 'a'].p_size = mountlist->el[idx].size * 2;
			lp->d_partitions[c - 'a'].p_fsize = 0;
			lp->d_partitions[c - 'a'].p_frag = 0;
			lp->d_partitions[c - 'a'].p_cpg = 0;
			if (!strcmp(mountlist->el[idx].format, "ufs")
				|| !strcmp(mountlist->el[idx].format, "ffs")
				|| !strcmp(mountlist->el[idx].format, "4.2BSD")) {
				lp->d_partitions[c - 'a'].p_fstype = FS_BSDFFS;
				lp->d_partitions[c - 'a'].p_fsize = 2048;
				lp->d_partitions[c - 'a'].p_frag = 8;
				lp->d_partitions[c - 'a'].p_cpg = 64;
			} else if (!strcasecmp(mountlist->el[idx].format, "raid")
					   || !strcasecmp(mountlist->el[idx].format, "vinum")) {
				lp->d_partitions[c - 'a'].p_fstype = FS_VINUM;
			} else if (!strcmp(mountlist->el[idx].format, "swap")) {
				lp->d_partitions[c - 'a'].p_fstype = FS_SWAP;
			} else
				lp->d_partitions[c - 'a'].p_fstype = FS_OTHER;
		}
	}

	// fix up the offsets
	lp->d_partitions[0].p_offset = 0;
	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;

	for (i = 1; i < lp->d_npartitions; ++i) {
		int lastone;
		if ((i == RAW_PART) || (lp->d_partitions[i].p_size == 0))
			continue;
		for (lastone = i - 1; lastone >= 0; lastone--) {
			if ((lp->d_partitions[lastone].p_size)
				&& (lastone != RAW_PART))
				break;
		}
		lp->d_partitions[i].p_offset =
			lp->d_partitions[lastone].p_offset +
			lp->d_partitions[lastone].p_size;
	}
	if (lp->d_partitions[lo].p_offset + lp->d_partitions[lo].p_size >
		lp->d_secperunit) {
		lp->d_partitions[lo].p_size =
			lp->d_secperunit - lp->d_partitions[lo].p_offset;
	}

	ftmp = fopen("/tmp/disklabel", "w");
	display_disklabel(ftmp, lp);
	fclose(ftmp);
	sprintf(command, "disklabel -wr %s auto", canonical_name(drivename));
	retval += run_program_and_log_output(command, TRUE);
	sprintf(command, "disklabel -R %s /tmp/disklabel",
			canonical_name(drivename));
	retval += run_program_and_log_output(command, TRUE);
	if (ret)
		*ret = *lp;
	return retval;
}
#endif


/**
 * Partition @p drivename based on @p mountlist.
 * @param mountlist The mountlist to use to guide the partitioning.
 * @param drivename The drive to partition.
 * @return 0 for success, nonzero for failure.
 */
int partition_drive(struct mountlist_itself *mountlist, char *drivename)
{
	/** int *************************************************************/
	int current_devno;
	int previous_devno = 0;
	int lino;
	int retval = 0;
	int i;
	FILE *pout_to_fdisk = NULL;

#ifdef __FreeBSD__
	bool fbsd_part = FALSE;
	char subdev_str[MAX_STR_LEN];
#endif

	/** long long *******************************************************/
	long long partsize;

	/** buffers *********************************************************/
	char *device_str;
	char *format;
	char *tmp;

	/** end *************************************************************/

	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drivename);

	malloc_string(device_str);
	malloc_string(format);
	malloc_string(tmp);

	sprintf(tmp, "Partitioning drive %s", drivename);
	log_it(tmp);

#if __FreeBSD__
	log_it("(Not opening fdisk now; that's the Linux guy's job)");
	pout_to_fdisk = NULL;
#else
	make_hole_for_file(FDISK_LOG);
#ifdef __IA64__
	sprintf(tmp, "parted2fdisk %s >> %s 2>> %s", drivename, FDISK_LOG,
			FDISK_LOG);
#else
	sprintf(tmp, "fdisk %s >> %s 2>> %s", drivename, FDISK_LOG, FDISK_LOG);
#endif
	pout_to_fdisk = popen(tmp, "w");
	if (!pout_to_fdisk) {
		log_to_screen("Cannot call fdisk to configure %s", drivename);
		paranoid_free(device_str);
		paranoid_free(format);
		paranoid_free(tmp);
		return (1);
	}
#endif
	for (current_devno = 1; current_devno < 99; current_devno++) {
		build_partition_name(device_str, drivename, current_devno);
		lino = find_device_in_mountlist(mountlist, device_str);

		if (lino < 0) {
			// device not found in mountlist
#if __FreeBSD__
			// If this is the first partition (just as a sentinel value),
			// then see if the user has picked 'dangerously-dedicated' mode.
			// If so, then we just call label_drive_or_slice() and return.
			char c;
			if (current_devno == 1) {
				// try DangerouslyDedicated mode
				for (c = 'a'; c <= 'z'; c++) {
					sprintf(subdev_str, "%s%c", drivename, c);
					if (find_device_in_mountlist(mountlist, subdev_str) >
						0) {
						fbsd_part = TRUE;
					}
				}
				if (fbsd_part) {
					int r = label_drive_or_slice(mountlist,
												 drivename,
												 0);
					char command[MAX_STR_LEN];
					sprintf(command, "disklabel -B %s",
							basename(drivename));
					if (system(command)) {
						log_to_screen
							("Warning! Unable to make the drive bootable.");
					}
					paranoid_free(device_str);
					paranoid_free(format);
					paranoid_free(tmp);
					return r;
				}
			}
			for (c = 'a'; c <= 'z'; c++) {
				sprintf(subdev_str, "%s%c", device_str, c);
				if (find_device_in_mountlist(mountlist, subdev_str) > 0) {
					fbsd_part = TRUE;
				}
			}
			// Now we check the subpartitions of the current partition.
			if (fbsd_part) {
				int i, line;

				strcpy(format, "ufs");
				partsize = 0;
				for (i = 'a'; i < 'z'; ++i) {
					sprintf(subdev_str, "%s%c", device_str, i);
					line = find_device_in_mountlist(mountlist, subdev_str);
					if (line > 0) {
						// We found one! Add its size to the total size.
						partsize += mountlist->el[line].size;
					}
				}
			} else {
				continue;
			}
#else
			continue;
#endif
		}

		/* OK, we've found partition /dev/hdxN in mountlist; let's prep it */
		/* For FreeBSD, that is      /dev/adXsY */

		log_it("Found partition %s in mountlist", device_str);
		if (!previous_devno) {

			log_it("Wiping %s's partition table", drivename);
#if __FreeBSD__
			// FreeBSD doesn't let you write to blk devices in <512byte chunks.
//          sprintf(tmp, "dd if=/dev/zero of=%s count=1 bs=512", drivename);
//          if (run_program_and_log_output(tmp, TRUE)) {
			file = open(drivename, O_WRONLY);
			if (!file) {
				sprintf(tmp,
						"Warning - unable to open %s for wiping it's partition table",
						drivename);
				log_to_screen(tmp);
			}

			for (i = 0; i < 512; i++) {
				if (!write(file, "\0", 1)) {
					sprintf(tmp, "Warning - unable to write to %s",
							drivename);
					log_to_screen(tmp);
				}
			}
			system("sync");
#else
			iamhere("New, kernel-friendly partition remover");
			for (i = 20; i > 0; i--) {
				fprintf(pout_to_fdisk, "d\n%d\n", i);
				fflush(pout_to_fdisk);
			}
//          sprintf(tmp, "dd if=/dev/zero of=%s count=1 bs=512", drivename);
//          run_program_and_log_output(tmp, 1);
#endif
			if (current_devno > 1) {
				previous_devno =
					make_dummy_partitions(pout_to_fdisk, drivename,
										  current_devno);
			}
		}
#ifdef __FreeBSD__
		if (!fbsd_part) {
#endif

			strcpy(format, mountlist->el[lino].format);
			partsize = mountlist->el[lino].size;

#ifdef __FreeBSD__
		}
#endif

		if (current_devno == 5 && previous_devno == 4) {
			log_to_screen
				("You must leave at least one partition spare as the Extended partition.");
			paranoid_free(device_str);
			paranoid_free(format);
			paranoid_free(tmp);
			return (1);
		}

		retval +=
			partition_device(pout_to_fdisk, drivename, current_devno,
							 previous_devno, format, partsize);

#ifdef __FreeBSD__
		if ((current_devno <= 4) && fbsd_part) {
			sprintf(tmp, "disklabel -B %s", basename(device_str));
			retval += label_drive_or_slice(mountlist, device_str, 0);
			if (system(tmp)) {
				log_to_screen
					("Warning! Unable to make the slice bootable.");
			}
		}
#endif

		previous_devno = current_devno;
	}

	if (pout_to_fdisk) {
// mark relevant partition as bootable
		sprintf(tmp, "a\n%s\n",
				call_program_and_get_last_line_of_output
				("make-me-bootable /tmp/mountlist.txt dummy"));
		fput_string_one_char_at_a_time(pout_to_fdisk, tmp);
// close fdisk
		fput_string_one_char_at_a_time(pout_to_fdisk, "w\n");
		system("sync");
		paranoid_pclose(pout_to_fdisk);
		log_msg(0,
				"------------------- fdisk.log looks like this ------------------");
		sprintf(tmp, "cat %s >> %s", FDISK_LOG, MONDO_LOGFILE);
		system(tmp);
		log_msg(0,
				"------------------- end of fdisk.log... word! ------------------");
		sprintf(tmp, "tail -n6 %s | fgrep \"16: \"", FDISK_LOG);
		if (!run_program_and_log_output(tmp, 5)) {
			g_partition_table_locked_up++;
			log_to_screen
				("A flaw in the Linux kernel has locked the partition table.");
		}
	}
	paranoid_free(device_str);
	paranoid_free(format);
	paranoid_free(tmp);
	return (retval);
}

/**
 * Create partition number @p partno on @p drive with @p fdisk.
 * @param drive The drive to create the partition on.
//  * @param partno The partition number of the new partition (1-4 are primary, >=5 is logical).
 * @param prev_partno The partition number of the most recently prepped partition.
 * @param format The filesystem type of this partition (used to set the type).
 * @param partsize The size of the partition in @e bytes.
 * @return 0 for success, nonzero for failure.
 */
int partition_device(FILE * pout_to_fdisk, const char *drive, int partno,
					 int prev_partno, const char *format,
					 long long partsize)
{
	/** int **************************************************************/
	int retval = 0;
	int res = 0;

	/** buffers **********************************************************/
	char *program;
	char *partition_name;
	char *tmp;
	char *logfile;
	char *output;

	/** pointers **********************************************************/
	char *p;
	char *part_table_fmt;
	FILE *fout;

	/** end ***************************************************************/

	malloc_string(program);
	malloc_string(partition_name);
	malloc_string(tmp);
	malloc_string(logfile);
	malloc_string(output);

	assert_string_is_neither_NULL_nor_zerolength(drive);
	assert(format != NULL);

	log_it("partition_device('%s', %d, %d, '%s', %lld) --- starting",
		   drive, partno, prev_partno, format, partsize);

	if (!strncmp(drive, RAID_DEVICE_STUB, strlen(RAID_DEVICE_STUB))) {
		sprintf(tmp, "Not partitioning %s - it is a virtual drive", drive);
		log_it(tmp);
		paranoid_free(program);
		paranoid_free(partition_name);
		paranoid_free(tmp);
		paranoid_free(logfile);
		paranoid_free(output);
		return (0);
	}
	build_partition_name(partition_name, drive, partno);
	if (partsize <= 0) {
		sprintf(tmp, "Partitioning device %s (max size)", partition_name);
	} else {
		sprintf(tmp, "Partitioning device %s (%lld MB)", partition_name,
				(long long) partsize / 1024);
	}
	update_progress_form(tmp);
	log_it(tmp);

	if (is_this_device_mounted(partition_name)) {
		sprintf(tmp, "%s is mounted, and should not be partitioned",
				partition_name);
		log_to_screen(tmp);
		paranoid_free(program);
		paranoid_free(partition_name);
		paranoid_free(tmp);
		paranoid_free(logfile);
		paranoid_free(output);
		return (1);
/*
	} else if (does_partition_exist(drive, partno)) {
		sprintf(tmp, "%s already has a partition", partition_name);
		log_to_screen(tmp);
		return (1);
*/
	}


	/*  sprintf(tmp,"Partitioning %s  ",partition_name); */
	/*  mvaddstr_and_log_it(g_currentY+1,30,tmp); */
	p = (char *) strrchr(partition_name, '/');
	sprintf(logfile, "/tmp/fdisk.log.%s", ++p);
	sprintf(program, "parted2fdisk %s >> %s 2>> %s", drive, MONDO_LOGFILE,
			MONDO_LOGFILE);

	/* BERLIOS: shoould not be called each time */
	part_table_fmt = which_partition_format(drive);
	output[0] = '\0';
	/* make it a primary/extended/logical */
	if (partno <= 4) {
		sprintf(output + strlen(output), "n\np\n%d\n", partno);
	} else {
		/* MBR needs an extended partition if more than 4 partitions */
		if (strcmp(part_table_fmt, "MBR") == 0) {
			if (partno == 5) {
				if (prev_partno >= 4) {
					log_to_screen
						("You need to leave at least one partition free, for 'extended/logical'");
					paranoid_free(program);
					paranoid_free(partition_name);
					paranoid_free(tmp);
					paranoid_free(logfile);
					paranoid_free(output);
					return (1);
				} else {
					sprintf(output + strlen(output), "n\ne\n%d\n\n\n",
							prev_partno + 1);
				}
			}
			strcat(output + strlen(output), "n\nl\n");
		} else {
			/* GPT allows more than 4 primary partitions */
			sprintf(output + strlen(output), "n\np\n%d\n", partno);
		}
	}
	strcat(output + strlen(output), "\n");	/*start block (ENTER for next free blk */
	if (partsize > 0) {
		if (!strcmp(format, "7")) {
			log_msg(1, "Adding 512K, just in case");
			partsize += 512;
		}
		sprintf(output + strlen(output), "+%lldK", (long long) (partsize));
	}
	strcat(output + strlen(output), "\n");
#if 0
/*
#endif
	sprintf(tmp,"PARTSIZE = +%ld",(long)partsize);
	log_it(tmp);
	log_it("---fdisk command---");
	log_it(output);
	log_it("---end of fdisk---");
#if 0
*/
#endif


	if (pout_to_fdisk) {
		log_msg(1, "Doing the new all-in-one fdisk thing");
		log_msg(1, "output = '%s'", output);
		fput_string_one_char_at_a_time(pout_to_fdisk, output);
		fput_string_one_char_at_a_time(pout_to_fdisk, "\n\np\n");
		strcpy(tmp, last_line_of_file(FDISK_LOG));
		if (strstr(tmp, " (m ")) {
			log_msg(1, "Successfully created %s%d", drive, partno);
		} else {
			log_msg(1, "last line = %s", tmp);
			log_msg(1, "Failed to create %s%d; sending 'Enter'...", drive,
					partno);
		}
		if (!retval) {
			log_msg(1, "Trying to set %s%d's partition type now", drive,
					partno);
			retval =
				set_partition_type(pout_to_fdisk, drive, partno, format,
								   partsize);
			if (retval) {
				log_msg(1, "Failed. Trying again...");
				retval =
					set_partition_type(pout_to_fdisk, drive, partno,
									   format, partsize);
			}
		}
		if (retval) {
			log_msg(1, "...but failed to set type");
		}
	} else {
		strcat(output, "w\n\n");
		if (g_fprep) {
			fprintf(g_fprep, "echo \"%s\" | %s\n", output, program);
		}
		/* write to disk; close fdisk's stream */
		if (!(fout = popen(program, "w"))) {
			log_OS_error("can't popen-out to program");
		} else {
			fputs(output, fout);
			paranoid_pclose(fout);
		}
		if (!does_partition_exist(drive, partno) && partsize > 0) {
			log_it("Vaccum-packing");
			g_current_progress--;
			res =
				partition_device(pout_to_fdisk, drive, partno, prev_partno,
								 format, -1);
			if (res) {
				sprintf(tmp, "Failed to vacuum-pack %s", partition_name);
				log_it(tmp);
				retval++;
			} else {
				retval = 0;
			}
		}
		if (does_partition_exist(drive, partno)) {
			retval =
				set_partition_type(pout_to_fdisk, drive, partno, format,
								   partsize);
			if (retval) {
				sprintf(tmp, "Partitioned %s but failed to set its type",
						partition_name);
				log_it(tmp);
			} else {
				if (partsize > 0) {
					sprintf(tmp, "Partition %s created+configured OK",
							partition_name);
					log_to_screen(tmp);
				} else {
					log_it("Returning from a successful vacuum-pack");
				}
			}
		} else {
			sprintf(tmp, "Failed to partition %s", partition_name);
			if (partsize > 0) {
				log_to_screen(tmp);
			} else {
				log_it(tmp);
			}
			retval++;
		}
	}
	g_current_progress++;
	log_it("partition_device() --- leaving");
	paranoid_free(program);
	paranoid_free(partition_name);
	paranoid_free(tmp);
	paranoid_free(logfile);
	paranoid_free(output);
	return (retval);
}



/**
 * Create all partitions listed in @p mountlist.
 * @param mountlist The mountlist to use to guide the partitioning.
 * @return The number of errors encountered (0 for success).
 * @note This sets the partition types but doesn't actually do the formatting.
 * Use format_everything() for that.
 */
int partition_everything(struct mountlist_itself *mountlist)
{
	/** int ************************************************************/
	int lino;
	int retval = 0;
	int i;
	int res;

	/** buffer *********************************************************/
	struct list_of_disks *drivelist;
	/*  struct mountlist_itself new_mtlist, *mountlist; */

	/** end ************************************************************/

	drivelist = malloc(sizeof(struct list_of_disks));
	assert(mountlist != NULL);

	log_it("partition_everything() --- starting");
	mvaddstr_and_log_it(g_currentY, 0, "Partitioning hard drives      ");
	/*  mountlist=orig_mtlist; */
	if (mountlist_contains_raid_devices(mountlist)) {
		/*      mountlist=&new_mtlist; */
		/*      extrapolate_mountlist_to_include_raid_partitions(mountlist,orig_mtlist); */
		log_msg(0,
				"Mountlist, including the partitions incorporated in RAID devices:-");
		for (i = 0; i < mountlist->entries; i++) {
			log_it(mountlist->el[i].device);
		}
		log_msg(0, "End of mountlist.");
	}
	log_msg(0, "Stopping all LVMs, just in case");
	if (!g_text_mode) {
		newtSuspend();
	}
	do_my_funky_lvm_stuff(TRUE, FALSE);	// just remove old partitions
	if (!g_text_mode) {
		newtResume();
	}
	log_msg(0, "Stopping all software RAID devices, just in case");
	stop_all_raid_devices(mountlist);
	log_msg(0, "Done.");

/*	
	if (does_file_exist("/tmp/i-want-my-lvm"))
	  {
	    wipe_MBRs_and_reboot_if_necessary(mountlist); // i.e. if it wasn't done recently
	  }
*/

	open_progress_form("Partitioning devices",
					   "I am now going to partition all your drives.",
					   "This should not take more than five minutes.", "",
					   mountlist->entries);

	make_list_of_drives_in_mountlist(mountlist, drivelist);

	/* partition each drive */
	for (lino = 0; lino < drivelist->entries; lino++) {
		res = partition_drive(mountlist, drivelist->el[lino].device);
		retval += res;
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
		log_to_screen
			("Errors occurred during the partitioning of your hard drives.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
		paranoid_system("rm -f /tmp/fdisk*.log 2> /dev/null");
	}
	newtSuspend();
	system("clear");
	newtResume();
	paranoid_free(drivelist);
	return (retval);
}






/**
 * Set the type of partition number @p partno on @p drive to @p format.
 * @param drive The drive to change the type of a partition on.
 * @param partno The partition number on @p drive to change the type of.
 * @param format The filesystem type this partition will eventually contain.
 * @param partsize The size of this partition, in @e bytes (used for vfat
 * type calculations).
 * @return 0 for success, nonzero for failure.
 */
int set_partition_type(FILE * pout_to_fdisk, const char *drive, int partno,
					   const char *format, long long partsize)
{
	/** buffers *********************************************************/
	char *partition;
	char *command;
	char *output;
	char *tmp;
	char *partcode;
	char *logfile;

	/** pointers *********************************************************/
	char *p;
	FILE *fout;

	/** int **************************************************************/
	int res = 0;

	/** end **************************************************************/

	assert_string_is_neither_NULL_nor_zerolength(drive);
	assert(format != NULL);

	malloc_string(partition);
	malloc_string(command);
	malloc_string(output);
	malloc_string(tmp);
	malloc_string(partcode);
	malloc_string(logfile);

	build_partition_name(partition, drive, partno);
	p = (char *) strrchr(partition, '/');
	sprintf(logfile, "/tmp/fdisk-set-type.%s.log", ++p);
	if (strcmp(format, "swap") == 0) {
		strcpy(partcode, "82");
	} else if (strcmp(format, "vfat") == 0) {
		if (partsize / 1024 > 8192) {
			strcpy(partcode, "c");
		} else {
			strcpy(partcode, "b");
		}
	} else if (strcmp(format, "ext2") == 0
			   || strcmp(format, "reiserfs") == 0
			   || strcmp(format, "ext3") == 0 || strcmp(format, "xfs") == 0
			   || strcmp(format, "jfs") == 0) {
		strcpy(partcode, "83");
	} else if (strcmp(format, "minix") == 0) {
		strcpy(partcode, "81");
	} else if (strcmp(format, "raid") == 0) {
		strcpy(partcode, "fd");
	} else if ((strcmp(format, "ufs") == 0)
			   || (strcmp(format, "ffs") == 0)) {	/* raid autodetect */
		strcpy(partcode, "a5");
	} else if (strcmp(format, "lvm") == 0) {
		strcpy(partcode, "8e");
	} else if (format[0] == '\0') {	/* LVM physical partition */
		partcode[0] = '\0';
	} else if (strlen(format) >= 1 && strlen(format) <= 2) {
		strcpy(partcode, format);
	} else {
		/* probably an image */
		sprintf(tmp,
				"Unknown format ('%s') - using supplied string anyway",
				format);
		mvaddstr_and_log_it(g_currentY++, 0, tmp);
#ifdef __FreeBSD__
		strcpy(partcode, format);	// was a5
#else
		strcpy(partcode, format);	// was 83
#endif
	}
	sprintf(tmp, "Setting %s's type to %s (%s)", partition, format,
			partcode);
	log_msg(1, tmp);
	if (partcode[0] != '\0' && strcmp(partcode, "83")) {	/* no need to set type if 83: 83 is default */

		if (pout_to_fdisk) {
			res = 0;
			fput_string_one_char_at_a_time(pout_to_fdisk, "t\n");
			if (partno > 1
				|| strstr(last_line_of_file(FDISK_LOG), " (1-4)")) {
				log_msg(5, "Specifying partno (%d) - yay", partno);
				sprintf(tmp, "%d\n", partno);
				fput_string_one_char_at_a_time(pout_to_fdisk, tmp);
				log_msg(5, "A - last line = '%s'",
						last_line_of_file(FDISK_LOG));
			}

			sprintf(tmp, "%s\n", partcode);
			fput_string_one_char_at_a_time(pout_to_fdisk, tmp);
			log_msg(5, "B - last line = '%s'",
					last_line_of_file(FDISK_LOG));
			fput_string_one_char_at_a_time(pout_to_fdisk, "\n");
			log_msg(5, "C - last line = '%s'",
					last_line_of_file(FDISK_LOG));

			strcpy(tmp, last_line_of_file(FDISK_LOG));
			if (!strstr(tmp, " (m ")) {
				log_msg(1, "last line = '%s'; part type set failed", tmp);
				res++;
				fput_string_one_char_at_a_time(pout_to_fdisk, "\n");
			}
			fput_string_one_char_at_a_time(pout_to_fdisk, "p\n");
		} else {
			sprintf(output, "t\n%d\n%s\n", partno, partcode);
			strcat(output, "w\n");
			sprintf(command, "parted2fdisk %s >> %s 2>> %s", drive,
					MONDO_LOGFILE, MONDO_LOGFILE);
			log_msg(5, "output = '%s'", output);
			log_msg(5, "partno=%d; partcode=%s", partno, partcode);
			log_msg(5, "command = '%s'", command);
			fout = popen(command, "w");
			if (!fout) {
				log_OS_error(command);
				res = 1;
			} else {
				res = 0;
				fprintf(fout, output);
				paranoid_pclose(fout);
			}
		}
		if (res) {
			log_OS_error(command);
		}
	}

	paranoid_free(partition);
	paranoid_free(command);
	paranoid_free(output);
	paranoid_free(tmp);
	paranoid_free(partcode);
	paranoid_free(logfile);

	return (res);
}


int start_raid_device(char *raid_device)
{
	/** int *************************************************************/
	int res;
	int retval = 0;

	/** buffers *********************************************************/
	char *program;

	/** end *************************************************************/

	assert_string_is_neither_NULL_nor_zerolength(raid_device);
	malloc_string(program);

#ifdef __FreeBSD__
	if (is_this_device_mounted(raid_device)) {
		log_it("Can't start %s when it's mounted!", raid_device);
		return 1;
	}
	sprintf(program, "vinum start -f %s", raid_device);
#else
	sprintf(program, "raidstart %s", raid_device);
//      sprintf (program, "raidstart " RAID_DEVICE_STUB "*");
#endif
	log_msg(1, "program = %s", program);
	res = run_program_and_log_output(program, 1);
	if (g_fprep) {
		fprintf(g_fprep, "%s\n", program);
	}
	if (res) {
		log_msg(1, "Warning - failed to start RAID device %s",
				raid_device);
	}
	retval += res;
	sleep(1);
	return (retval);
}



/**
 * Stop @p raid_device using @p raidstop.
 * @param raid_device The software RAID device to stop.
 * @return 0 for success, nonzero for failure.
 */
int stop_raid_device(char *raid_device)
{
	/** int *************************************************************/
	int res;
	int retval = 0;

	/** buffers *********************************************************/
	char *program;

	/** end *************************************************************/

	assert_string_is_neither_NULL_nor_zerolength(raid_device);
	malloc_string(program);

#ifdef __FreeBSD__
	if (is_this_device_mounted(raid_device)) {
		log_it("Can't stop %s when it's mounted!", raid_device);
		return 1;
	}
	sprintf(program, "vinum stop -f %s", raid_device);
#else
	sprintf(program, "raidstop %s", raid_device);
//      sprintf (program, "raidstop " RAID_DEVICE_STUB "*");
#endif
	log_msg(1, "program = %s", program);
	res = run_program_and_log_output(program, 1);
	if (g_fprep) {
		fprintf(g_fprep, "%s\n", program);
	}
	if (res) {
		log_msg(1, "Warning - failed to stop RAID device %s", raid_device);
	}
	retval += res;
	return (retval);
}


int start_all_raid_devices(struct mountlist_itself *mountlist)
{
	int i;
	int retval = 0;
	int res;

	for (i = 0; i < mountlist->entries; i++) {
		if (!strncmp
			(mountlist->el[i].device, RAID_DEVICE_STUB,
			 strlen(RAID_DEVICE_STUB))) {
			log_msg(1, "Starting %s", mountlist->el[i].device);
			res = start_raid_device(mountlist->el[i].device);
			retval += res;
		}
	}
	if (retval) {
		log_msg(1, "Started all s/w raid devices OK");
	} else {
		log_msg(1, "Failed to start some/all s/w raid devices");
	}
	return (retval);
}

/**
 * Stop all software RAID devices listed in @p mountlist.
 * @param mountlist The mountlist to stop the RAID devices in.
 * @return The number of errors encountered (0 for success).
 * @bug @p mountlist is not used.
 */
int stop_all_raid_devices(struct mountlist_itself *mountlist)
{
	/** int *************************************************************/
	int retval = 0;
#ifndef __FreeBSD__
	int res;
#endif

	/** char ************************************************************/
	char *incoming;
#ifndef __FreeBSD__
	char *dev;
#endif
	/** pointers ********************************************************/
#ifndef __FreeBSD__
	char *p;
#endif
	FILE *fin;
	int i;

	/** end ****************************************************************/

	malloc_string(dev);
	malloc_string(incoming);
	assert(mountlist != NULL);

	for (i = 0; i < 3; i++) {
#ifdef __FreeBSD__
		fin =
			popen
			("vinum list | grep '^[PVS]' | sed 's/S/1/;s/P/2/;s/V/3/' | sort | cut -d' ' -f2",
			 "r");
		if (!fin) {
			paranoid_free(dev);
			paranoid_free(incoming);
			return (1);
		}
		for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin);
			 fgets(incoming, MAX_STR_LEN - 1, fin)) {
			retval += stop_raid_device(incoming);
		}
#else
		fin = fopen("/proc/mdstat", "r");
		if (!fin) {
			log_OS_error("/proc/mdstat");
			paranoid_free(dev);
			paranoid_free(incoming);
			return (1);
		}
		for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin);
			 fgets(incoming, MAX_STR_LEN - 1, fin)) {
			for (p = incoming;
				 *p != '\0' && (*p != 'm' || *(p + 1) != 'd'
								|| !isdigit(*(p + 2))); p++);
			if (*p != '\0') {
				sprintf(dev, "/dev/%s", p);
				for (p = dev; *p > 32; p++);
				*p = '\0';
				res = stop_raid_device(dev);
			}
		}
#endif
	}
	paranoid_fclose(fin);
	if (retval) {
		log_msg(1, "Warning - unable to stop some RAID devices");
	}
	paranoid_free(dev);
	paranoid_free(incoming);
	system("sync");
	system("sync");
	system("sync");
	sleep(1);
	return (retval);
}



/**
 * Decide which command we need to use to format a device of type @p format.
 * @param format The filesystem type we are about to format.
 * @param program Where to put the binary name for this format.
 * @return 0 for success, nonzero for failure.
 */
int which_format_command_do_i_need(char *format, char *program)
{
	/** int *************************************************************/
	int res = 0;

	/** buffers *********************************************************/
	char *tmp;

	/** end ***************************************************************/

	malloc_string(tmp);
	assert_string_is_neither_NULL_nor_zerolength(format);
	assert(program != NULL);

	if (strcmp(format, "swap") == 0) {
#ifdef __FreeBSD__
		strcpy(program, "true");
#else
		strcpy(program, "mkswap");
#endif
	} else if (strcmp(format, "vfat") == 0) {
		strcpy(program, "format-and-kludge-vfat");
#ifndef __FreeBSD__
	} else if (strcmp(format, "reiserfs") == 0) {
		strcpy(program, "mkreiserfs -ff");
	} else if (strcmp(format, "xfs") == 0) {
		strcpy(program, "mkfs.xfs -f -q");
	} else if (strcmp(format, "jfs") == 0) {
		strcpy(program, "mkfs.jfs");
	} else if (strcmp(format, "ext3") == 0) {
		strcpy(program, "mkfs -t ext2 -F -j -q");
	} else if (strcmp(format, "minix") == 0) {
		strcpy(program, "mkfs.minix");
#endif
	} else if (strcmp(format, "ext2") == 0) {
		strcpy(program, "mke2fs -F -q");
	} else {
#ifdef __FreeBSD__
		sprintf(program, "newfs_%s", format);
#else
		sprintf(program, "mkfs -t %s -c", format);	// -c checks for bad blocks
#endif
		sprintf(tmp, "Unknown format (%s) - assuming '%s' will do", format,
				program);
		log_it(tmp);
		res = 0;
	}
	paranoid_free(tmp);
	return (res);
}


/**
 * Calculate the probable size of @p drive_name by adding up sizes in
 * @p mountlist.
 * @param mountlist The mountlist to use to calculate the size.
 * @param drive_name The drive to calculate the original size of.
 * @return The size of @p drive_name in kilobytes.
 */
long calc_orig_size_of_drive_from_mountlist(struct mountlist_itself
											*mountlist, char *drive_name)
{
	/** long ************************************************************/
	long original_size_of_drive;

	/** int *************************************************************/
	int partno;

	/** buffers *********************************************************/
	char *tmp;

	/** end *************************************************************/

	malloc_string(tmp);
	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drive_name);

	for (original_size_of_drive = 0, partno = 0;
		 partno < mountlist->entries; partno++) {
		if (strncmp
			(mountlist->el[partno].device, drive_name,
			 strlen(drive_name)) == 0) {
			original_size_of_drive += mountlist->el[partno].size;
		} else {
			sprintf(tmp, "Skipping %s", mountlist->el[partno].device);
//                      log_to_screen(tmp);
		}
	}
	original_size_of_drive = original_size_of_drive / 1024;
	paranoid_free(tmp);
	return (original_size_of_drive);
}


/**
 * Resize a drive's entries in @p mountlist proportionately to fit its new size.
 * There are a few problems with this function:
 * - It won't work if there was any unallocated space on the user's hard drive
 *   when it was backed up.
 * - It won't work if the user's hard drive lies about its size (more common
 *   than you'd think).
 *
 * @param mountlist The mountlist to use for resizing @p drive_name.
 * @param drive_name The drive to resize.
 */
void resize_drive_proportionately_to_suit_new_drives(struct mountlist_itself
													 *mountlist,
													 char *drive_name)
{
	/**buffers **********************************************************/
	char *tmp;

	/** int *************************************************************/
	int partno, lastpart;
			   /** remove driveno, noof_drives stan benoit apr 2002**/

	/** float ***********************************************************/
	float factor;
	float new_size;
//  float newcylinderno;

	/** long *************************************************************/
	long newsizL;
	long current_size_of_drive = 0;
	long original_size_of_drive = 0;
	long final_size;			/* all in Megabytes */
	struct mountlist_reference *drivemntlist;

	/** structures *******************************************************/

	/** end **************************************************************/

	malloc_string(tmp);
	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drive_name);

	if (strlen(drive_name) >= strlen(RAID_DEVICE_STUB)) {
		if (strncmp(drive_name, RAID_DEVICE_STUB, strlen(RAID_DEVICE_STUB))
			== 0) {
			paranoid_free(tmp);
			return;
		}
	}

	/*
	   sprintf (tmp, "cp -f %s %s.pre-resize", g_mountlist_fname, g_mountlist_fname);
	   run_program_and_log_output (tmp, FALSE);
	 */

	current_size_of_drive = get_phys_size_of_drive(drive_name);

	if (current_size_of_drive <= 0) {
		log_it("Not resizing to match %s - can't find drive", drive_name);
		paranoid_free(tmp);
		return;
	}
	sprintf(tmp, "Expanding entries to suit drive %s (%ld MB)", drive_name,
			current_size_of_drive);
	log_to_screen(tmp);

	drivemntlist = malloc(sizeof(struct mountlist_reference));
	drivemntlist->el =
		malloc(sizeof(struct mountlist_line *) * MAX_TAPECATALOG_ENTRIES);

	if (!drivemntlist) {
		fatal_error("Cannot malloc temporary mountlist\n");
	}
	create_mountlist_for_drive(mountlist, drive_name, drivemntlist);

	for (partno = 0; partno < drivemntlist->entries; partno++) {
		original_size_of_drive += drivemntlist->el[partno]->size;
	}
	original_size_of_drive = original_size_of_drive / 1024;

	if (original_size_of_drive <= 0) {
		sprintf(tmp, "Cannot resize %s's entries. Drive not found.",
				drive_name);
		log_to_screen(tmp);
		paranoid_free(tmp);
		return;
	}
	factor =
		(float) (current_size_of_drive) / (float) (original_size_of_drive);
	sprintf(tmp, "Disk %s was %ld MB; is now %ld MB; factor = %f",
			drive_name, original_size_of_drive, current_size_of_drive,
			factor);
	log_to_screen(tmp);

	lastpart = drivemntlist->entries - 1;
	for (partno = 0; partno < drivemntlist->entries; partno++) {
		/* the 'atoi' thing is to make sure we don't try to resize _images_, whose formats will be numeric */
		if (!atoi(drivemntlist->el[partno]->format)) {
			new_size = (float) (drivemntlist->el[partno]->size) * factor;
		} else {
			new_size = drivemntlist->el[partno]->size;
		}

		if (!strcmp(drivemntlist->el[partno]->mountpoint, "image")) {
			log_msg(1, "Skipping %s (%s) because it's an image",
					drivemntlist->el[partno]->device,
					drivemntlist->el[partno]->mountpoint);
			newsizL = (long) new_size;	// It looks wrong but it's not
		} else {
			newsizL = (long) new_size;
		}
		sprintf(tmp, "Changing %s from %lld KB to %ld KB",
				drivemntlist->el[partno]->device,
				drivemntlist->el[partno]->size, newsizL);
		log_to_screen(tmp);
		drivemntlist->el[partno]->size = newsizL;
	}
	final_size = get_phys_size_of_drive(drive_name);
	sprintf(tmp, "final_size = %ld MB", final_size);
	log_to_screen(tmp);
	paranoid_free(tmp);
}


/**
 * Resize all partitions in @p mountlist proportionately (each one
 * grows or shrinks by the same percentage) to fit them into the new
 * drives (presumably different from the old ones).
 * @param mountlist The mountlist to resize the drives in.
 */
void resize_mountlist_proportionately_to_suit_new_drives(struct mountlist_itself
														 *mountlist)
{
	/** buffers *********************************************************/
	struct list_of_disks *drivelist;

	/** int *************************************************************/
	int driveno;

	/** end *************************************************************/

	drivelist = malloc(sizeof(struct list_of_disks));
	assert(mountlist != NULL);

	if (g_mountlist_fname[0] == '\0') {
		log_it
			("resize_mountlist_prop...() - warning - mountlist fname is blank");
		log_it("That does NOT affect the functioning of this subroutine.");
		log_it("--- Hugo, 2002/11/20");
	}
	iamhere("Resizing mountlist");
	make_list_of_drives_in_mountlist(mountlist, drivelist);
	iamhere("Back from MLoDiM");
	for (driveno = 0; driveno < drivelist->entries; driveno++) {
		resize_drive_proportionately_to_suit_new_drives(mountlist,
														drivelist->
														el[driveno].
														device);
	}
	log_to_screen("Mountlist adjusted to suit current hard drive(s)");
	paranoid_free(drivelist);
}

/**
 * Create a mountlist_reference structure for @p drive_name in @p mountlist.
 * @param mountlist The complete mountlist to get the drive references from.
 * @param drive_name The drive to put in @p drivemntlist.
 * @param drivemntlist The mountlist_reference structure to put the drive's entries in.
 * @note @p drivemntlist and @p drivemntlist->el must be allocated by the caller.
 * @author Ralph Grewe
 */
void create_mountlist_for_drive(struct mountlist_itself *mountlist,
								char *drive_name,
								struct mountlist_reference *drivemntlist)
{
	int partno;
	char *tmp_drive_name, *c;

	assert(mountlist != NULL);
	assert(drive_name != NULL);
	assert(drivemntlist != NULL);

	log_msg(1, "Creating list of partitions for drive %s", drive_name);

	tmp_drive_name = strdup(drive_name);
	if (!tmp_drive_name)
		fatal_error("Out of memory");

	/* devfs devices? */
	c = strrchr(tmp_drive_name, '/');
	if (c && strncmp(c, "/disc", 5) == 0) {
		/* yup its devfs, change the "disc" to "part" so the existing code works */
		strcpy(c + 1, "part");
	}
	drivemntlist->entries = 0;
	for (partno = 0; partno < mountlist->entries; partno++) {
		if (strncmp
			(mountlist->el[partno].device, tmp_drive_name,
			 strlen(tmp_drive_name)) == 0) {
			drivemntlist->el[drivemntlist->entries] =
				&mountlist->el[partno];
			drivemntlist->entries++;
		}
	}
	if (tmp_drive_name)
		free(tmp_drive_name);
}

/* @} - end of prepGroup */
