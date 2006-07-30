/***************************************************************************
 * $Id
**/

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
#include <unistd.h>


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
	int lino;
	int i;
	FILE *fout;
	char *buf;
	const int blocksize = 512;
	struct list_of_disks *drivelist = NULL;
// If LVMs are present and a zero-and-reboot wasn't recently undertaken
// then zero & insist on reboot.
	buf = malloc(blocksize);
	if (does_file_exist("/tmp/i-want-my-lvm"))	// FIXME - cheating :)
	{
		drivelist = malloc(sizeof(struct list_of_disks));
		make_list_of_drives_in_mountlist(mountlist, drivelist);
		for (lino = 0; lino < drivelist->entries; lino++) {
			asprintf(&command,
					 "dd if=%s bs=512 count=1 2> /dev/null | grep \"%s\"",
					 drivelist->el[lino].device, MONDO_WAS_HERE);
			if (!run_program_and_log_output(command, 1)) {
				log_msg(1, "Found MONDO_WAS_HERE marker on drive#%d (%s)",
						lino, drivelist->el[lino].device);
				paranoid_free(command);
				break;
			}
			paranoid_free(command);
		}

		if (lino == drivelist->entries) {
// zero & reboot
			log_to_screen
				(_
				 ("I am sorry for the inconvenience but I must ask you to reboot."));
			log_to_screen(_
						  ("I need to reset the Master Boot Record; in order to be"));
			log_to_screen(_
						  ("sure the kernel notices, I must reboot after doing it."));
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
			sync();
			sync();
			sync();
			popup_and_OK
				(_
				 ("I must reboot now. Please leave the boot media in the drive and repeat your actions - e.g. type 'nuke' - and it should work fine."));
			system("reboot");
		}
	}
// Still here? Cool!
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
	/** char **************************************************/
	char *tmp = NULL;
	char *tmp1 = NULL;
	char *incoming = NULL;
	char *command = NULL;
	char *lvscan_sz = NULL;
	char *lvremove_sz = NULL;
	char *pvscan_sz = NULL;
	char *vgscan_sz = NULL;
	char *vgchange_sz = NULL;
	char *vgremove_sz = NULL;
	char *p = NULL;
	char *q = NULL;

	/** int ***************************************************/
	int retval = 0;
	int res = 0;
	int i = 0;
	int lvmversion = 1;
	long extents;
	fpos_t orig_pos;
	size_t n = 0;
	size_t n1 = 0;

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

	iamhere("STARTING");
	log_msg(1, "OK, opened i-want-my-lvm. Shutting down LVM volumes...");
	tmp = find_home_of_exe("lvm");
	if (tmp)	// found it :) cool
	{
		asprintf(&lvscan_sz, "lvm lvscan");
		asprintf(&lvremove_sz, "lvm lvremove");
		asprintf(&vgscan_sz, "lvm vgscan");
		asprintf(&pvscan_sz, "lvm pvscan");
		asprintf(&vgchange_sz, "lvm vgchange");
		asprintf(&vgremove_sz, "lvm vgremove");
	} else {
		asprintf(&lvscan_sz, "lvscan");
		asprintf(&lvremove_sz, "lvremove");
		asprintf(&vgscan_sz, "vgscan");
		asprintf(&pvscan_sz, "pvscan");
		asprintf(&vgchange_sz, "vgchange");
		asprintf(&vgremove_sz, "vgremove");
	}
	paranoid_free(tmp);

	asprintf(&command,
			"for i in `%s | cut -d\"'\" -f2 | sort -r` ; do echo \"Shutting down lv $i\" >> "
			MONDO_LOGFILE "; %s -f $i; done", lvscan_sz, lvremove_sz);
	paranoid_free(lvscan_sz);
	paranoid_free(lvremove_sz);

	run_program_and_log_output(command, 5);
	paranoid_free(command);

	sleep(1);
	asprintf(&command,
			"for i in `%s | grep -i lvm | cut -d'\"' -f2` ; do %s -a n $i ; %s $i; echo \"Shutting down vg $i\" >> "
			MONDO_LOGFILE "; done;", vgscan_sz, vgchange_sz, vgremove_sz);
	paranoid_free(vgchange_sz);
	paranoid_free(vgremove_sz);

	run_program_and_log_output(command, 5);
	paranoid_free(command);

	if (just_erase_existing_volumes) {
		paranoid_fclose(fin);
		log_msg(1, "Closed i-want-my-lvm. Finished erasing LVMs.");
		sync();
		sync();
		sync();
		sleep(1);
		iamhere("ENDING");
		log_msg(1, "Not many errors. Returning 0.");
		return (0);
	}

	log_msg(1, "OK, rewound i-want-my-lvm. Doing funky stuff...");
	rewind(fin);
	for (getline(&incoming, &n1, fin); !feof(fin); getline(&incoming, &n1, fin)) {
		fgetpos(fin, &orig_pos);
		/* BERLIOS : Strange no ? */
		if (incoming[0] != '#') {
			continue;
		}
		if (res && strstr(command, "create") && vacuum_pack) {
			sleep(2);
			sync();
			sync();
			sync();
		}
		if ((p = strstr(incoming, "vgcreate"))) {
// include next line(s) if they end in /dev (cos we've got a broken i-want-my-lvm)
			for (getline(&tmp, &n, fin); !feof(fin); getline(&tmp, &n, fin)) {
				if (tmp[0] == '#') {
					fsetpos(fin, &orig_pos);
					break;
				} else {
					fgetpos(fin, &orig_pos);
					asprintf(&tmp1, "%s%s", incoming, tmp);
					paranoid_free(incoming);
					incoming = tmp1;
				}
			}
			paranoid_free(tmp);

			for (q = incoming; *q != '\0'; q++) {
				if (*q < 32) {
					*q = ' ';
				}
			}
			malloc_string(tmp1);
			strcpy(tmp1, p + strlen("vgcreate") + 1);
			for (q = tmp1; *q > 32; q++);
			*q = '\0';
			log_msg(1, "Deleting old entries at /dev/%s", tmp1);
			asprintf(&tmp, "rm -Rf /dev/%s", tmp1);
			paranoid_free(tmp1);
			run_program_and_log_output(tmp, 1);
			paranoid_free(tmp);

			run_program_and_log_output(vgscan_sz, 1);
			run_program_and_log_output(pvscan_sz, 1);
			log_msg(3,
					"After working around potentially broken i-want-my-lvm, incoming[] is now '%s'",
					incoming);
		}
		for (p = incoming + 1; *p == ' '; p++);
		paranoid_free(command);
		asprintf(&command, p);
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
			/* BERLIOS : this tmp may be uninitialized ?
			log_it("%s... so I'll get creative.", tmp);
			*/
			if (lvmversion == 2) {
				tmp = call_program_and_get_last_line_of_output
						("tail -n5 " MONDO_LOGFILE
						" | grep Insufficient | tail -n1");
			} else {
				tmp = call_program_and_get_last_line_of_output
					   ("tail -n5 " MONDO_LOGFILE
						" | grep lvcreate | tail -n1");
			}
			for (p = tmp; *p != '\0' && !isdigit(*p); p++);
			extents = atol(p);
			log_msg(5, "p='%s' --> extents=%ld", p, extents);
			paranoid_free(tmp);
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
		asprintf(&tmp, "echo \"%s\" >> /tmp/out.sh", command);
		system(tmp);
		paranoid_free(tmp);
		sleep(1);
	}
	paranoid_fclose(fin);
	paranoid_free(vgscan_sz);
	paranoid_free(pvscan_sz);
	paranoid_free(command);
	paranoid_free(incoming);

	log_msg(1, "Closed i-want-my-lvm. Finished doing funky stuff.");
	sync();
	sync();
	sync();
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
	FILE *fin = NULL;
	int lino = 0;
	int j = 0;
	char *incoming = NULL;
	char *tmp = NULL;
	char *p = NULL;
	size_t n = 0;

	/** init *************************************************************/
	new_mountlist->entries = 0;

	/** end **************************************************************/

	assert(new_mountlist != NULL);
	assert(old_mountlist != NULL);

#ifdef __FreeBSD__
	log_to_screen
		(_
		 ("I don't know how to extrapolate the mountlist on FreeBSD. Sorry."));
	return (1);
#endif

	for (lino = 0; lino < old_mountlist->entries; lino++) {
		if (strstr(old_mountlist->el[lino].device, RAID_DEVICE_STUB))	// raid
		{
			if (!does_file_exist("/etc/raidtab")) {
				log_to_screen
					(_
					 ("Cannot find /etc/raidtab - cannot extrapolate the fdisk entries"));
				finish(1);
			}
			if (!(fin = fopen("/etc/raidtab", "r"))) {
				log_OS_error("Cannot open /etc/raidtab");
				finish(1);
			}
			for (getline(&incoming, &n, fin); !feof(fin)
				 && !strstr(incoming, old_mountlist->el[lino].device);
				 getline(&incoming, &n, fin));
			if (!feof(fin)) {
				asprintf(&tmp, "Investigating %s",
						old_mountlist->el[lino].device);
				log_it(tmp);
				paranoid_free(tmp);

				for (getline(&incoming, &n, fin); !feof(fin)
					 && !strstr(incoming, "raiddev");
					 getline(&incoming, &n, fin)) {
					if (strstr(incoming, OSSWAP("device", "drive"))
						&& !strchr(incoming, '#')) {
						for (p = incoming + strlen(incoming);
							 *(p - 1) <= 32; p--);
						*p = '\0';
						for (p--; p > incoming && *(p - 1) > 32; p--);
						asprintf(&tmp, "Extrapolating %s", p);
						log_it(tmp);
						paranoid_free(tmp);

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
							asprintf(&tmp,
									"Not adding %s to mountlist: it's already there", p);
							log_it(tmp);
							paranoid_free(tmp);
						}
					}
				}
			}
			paranoid_free(incoming);
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

	return (0);
}


/**
 * Create @p RAID device using information from @p structure.
 * This will create the specified RAID devive using information provided in
 * raidlist by means of the mdadm tool.
 * @param raidlist The structure containing all RAID information
 * @param device The RAID device to create.
 * @return 0 for success, nonzero for failure.
 */
int create_raid_device_via_mdadm(struct raidlist_itself *raidlist,
								 char *device)
{
  /** int **************************************************************/
	int i = 0;
	int j = 0;
	int res = 0;

  /** buffers ***********************************************************/
	char *devices = NULL;
	char *strtmp = NULL;
	char *level = NULL;
	char *program = NULL;

	// leave straight away if raidlist is initial or has no entries
	if (!raidlist || raidlist->entries == 0) {
		log_msg(1, "No RAID arrays found.");
		return 1;
	} else {
		log_msg(1, "%d RAID arrays found.", raidlist->entries);
	}
	// find raidlist entry for requested device
	for (i = 0; i < raidlist->entries; i++) {
		if (!strcmp(raidlist->el[i].raid_device, device))
			break;
	}
	// check whether RAID device was found in raidlist
	if (i == raidlist->entries) {
		log_msg(1, "RAID device %s not found in list.", device);
		return 1;
	}
	// create device list from normal disks followed by spare ones
	asprintf(&devices, raidlist->el[i].data_disks.el[0].device);
	for (j = 1; j < raidlist->el[i].data_disks.entries; j++) {
		asprintf(&strtmp, "%s", devices);
		paranoid_free(devices);
		asprintf(&devices, "%s %s", strtmp,
				 raidlist->el[i].data_disks.el[j].device);
		paranoid_free(strtmp);
	}
	for (j = 0; j < raidlist->el[i].spare_disks.entries; j++) {
		asprintf(&strtmp, "%s", devices);
		paranoid_free(devices);
		asprintf(&devices, "%s %s", strtmp,
				 raidlist->el[i].spare_disks.el[j].device);
		paranoid_free(strtmp);
	}
	// translate RAID level
	if (raidlist->el[i].raid_level == -2) {
		asprintf(&level, "multipath");
	} else if (raidlist->el[i].raid_level == -1) {
		asprintf(&level, "linear");
	} else {
		asprintf(&level, "raid%d", raidlist->el[i].raid_level);
	}
	// create RAID device:
	// - RAID device, number of devices and devices mandatory
	// - parity algorithm, chunk size and spare devices optional
	// - faulty devices ignored
	// - persistent superblock always used as this is recommended
	asprintf(&program,
			 "mdadm --create --force --run --auto=yes %s --level=%s --raid-devices=%d",
			 raidlist->el[i].raid_device, level,
			 raidlist->el[i].data_disks.entries);
	if (raidlist->el[i].parity != -1) {
		asprintf(&strtmp, "%s", program);
		paranoid_free(program);
		switch (raidlist->el[i].parity) {
		case 0:
			asprintf(&program, "%s --parity=%s", strtmp, "la");
			break;
		case 1:
			asprintf(&program, "%s --parity=%s", strtmp, "ra");
			break;
		case 2:
			asprintf(&program, "%s --parity=%s", strtmp, "ls");
			break;
		case 3:
			asprintf(&program, "%s --parity=%s", strtmp, "rs");
			break;
		default:
			fatal_error("Unknown RAID parity algorithm.");
			break;
		}
		paranoid_free(strtmp);
	}
	if (raidlist->el[i].chunk_size != -1) {
		asprintf(&strtmp, "%s", program);
		paranoid_free(program);
		asprintf(&program, "%s --chunk=%d", strtmp,
				 raidlist->el[i].chunk_size);
		paranoid_free(strtmp);
	}
	if (raidlist->el[i].spare_disks.entries > 0) {
		asprintf(&strtmp, "%s", program);
		paranoid_free(program);
		asprintf(&program, "%s --spare-devices=%d", strtmp,
				 raidlist->el[i].spare_disks.entries);
		paranoid_free(strtmp);
	}
	asprintf(&strtmp, "%s", program);
	paranoid_free(program);
	asprintf(&program, "%s %s", strtmp, devices);
	paranoid_free(strtmp);
	res = run_program_and_log_output(program, 1);
	// free memory
	paranoid_free(devices);
	paranoid_free(level);
	paranoid_free(program);
	// return to calling instance
	return res;
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
int format_device(char *device, char *format,
				  struct raidlist_itself *raidlist)
{
	/** int **************************************************************/
	int res = 0;
	int retval = 0;
#ifdef __FreeBSD__
	static bool vinum_started_yet = FALSE;
#endif

	/** buffers ***********************************************************/
	char *program = NULL;
	char *tmp = NULL;
	char *tmp1 = NULL;
	char *line = NULL;
	char *status = NULL;
	FILE *pin;
	FILE *fin;
	size_t n = 0;
	size_t n1 = 0;

	/** end ****************************************************************/

	assert_string_is_neither_NULL_nor_zerolength(device);
	assert(format != NULL);

	if (strstr(format, "raid")) {	// do not form RAID disks; do it to /dev/md* instead
		asprintf(&tmp, "Not formatting %s (it is a RAID disk)", device);
		log_it(tmp);
		paranoid_free(tmp);
		return (0);
	}
#ifdef __FreeBSD__
	if (strcmp(format, "swap") == 0) {
		log_it("Not formatting %s - it's swap", device);
		return (0);
	}
#endif
	if (strlen(format) <= 2) {
		asprintf(&tmp,
				"%s has a really small format type ('%s') - this is probably a hexadecimal string, which would suggest the partition is an image --- I shouldn't format it",
				device, format);
		log_it(tmp);
		paranoid_free(tmp);
		return (0);
	}
	if (is_this_device_mounted(device)) {
		asprintf(&tmp, _("%s is mounted - cannot format it       "), device);
		log_to_screen(tmp);
		paranoid_free(tmp);
		return (1);
	}
	if (strstr(device, RAID_DEVICE_STUB)) {
		newtSuspend();
#ifdef __FreeBSD__
		if (!vinum_started_yet) {
			if (!does_file_exist("/tmp/raidconf.txt")) {
				log_to_screen
					(_
					 ("/tmp/raidconf.txt does not exist. I therefore cannot start Vinum."));
			} else {
				int res;
				res =
					run_program_and_log_output
					("vinum create /tmp/raidconf.txt", TRUE);
				if (res) {
					log_to_screen
						(_
						 ("`vinum create /tmp/raidconf.txt' returned errors. Please fix them and re-run mondorestore."));
					finish(1);
				}
				vinum_started_yet = TRUE;
			}
		}

		if (vinum_started_yet) {
			asprintf(&tmp,
					_
					("Initializing Vinum device %s (this may take a *long* time)"),
					device);
			log_to_screen(tmp);
			paranoid_free(tmp);

			/* format raid partition */
			asprintf(&program,
					"for plex in `vinum lv -r %s | grep '^P' | tr '\t' ' ' | tr -s ' ' | cut -d' ' -f2`; do echo $plex; done > /tmp/plexes",
					basename(device));
			system(program);

			if (g_fprep) {
				fprintf(g_fprep, "%s\n", program);
			}
			paranoid_free(program);

			fin = fopen("/tmp/plexes", "r");
			while (getline(&line, &n, fin)) {
				if (strchr(line, '\n'))
					*(strchr(line, '\n')) = '\0';	// get rid of the \n on the end

				asprintf(&tmp, "Initializing plex: %s", line);
				open_evalcall_form(tmp);
				paranoid_free(tmp);

				asprintf(&tmp, "vinum init %s", line);
				system(tmp);
				paranoid_free(tmp);

				while (1) {
					asprintf(&tmp,
							"vinum lp -r %s | grep '^S' | head -1 | tr -s ' ' | cut -d: -f2 | cut -f1 | sed 's/^ //' | sed 's/I //' | sed 's/%%//'",
							line);
					pin = popen(tmp, "r");
					paranoid_free(tmp);

					getline(&status, &n1, pin);
					pclose(pin);

					if (!strcmp(status, "up")) {
						break;	/* it's done */
					}
					update_evalcall_form(atoi(status));
					usleep(250000);
					paranoid_free(status);
				}
				close_evalcall_form();
			}
			paranoid_free(line);
			fclose(fin);
			unlink("/tmp/plexes");
			/* retval+=res; */
		}
#else
		asprintf(&tmp, _("Initializing RAID device %s"), device);
		log_to_screen(tmp);
		paranoid_free(tmp);

// Shouldn't be necessary.
		log_to_screen(_("Stopping %s"), device);
		stop_raid_device(device);
		sync();
		sleep(1);
		/* BERLIOS: This code is wrong as program has not been initialized
		if (g_fprep) {
			fprintf(g_fprep, "%s\n", program);
		}
		*/

		log_msg(1, "Making %s", device);
		// use mkraid if it exists, otherwise use mdadm
		if (run_program_and_log_output("which mkraid", FALSE)) {
			res = create_raid_device_via_mdadm(raidlist, device);
			log_msg(1, "Creating RAID device %s via mdadm returned %d",
					device, res);
		} else {
			asprintf(&program, "mkraid --really-force %s", device);
			res = run_program_and_log_output(program, 1);
			log_msg(1, "%s returned %d", program, res);
			sync();
			sleep(3);
			start_raid_device(device);
			if (g_fprep) {
				fprintf(g_fprep, "%s\n", program);
			}
			paranoid_free(program);
		}
		sync();
		sleep(2);
#endif
		sync();
		sleep(1);
		newtResume();
	}
//#ifndef __FreeBSD__
//#endif

	if (!strcmp(format, "lvm")) {
		log_msg(1, "Don't format %s - it's part of an lvm volume", device);
		return (0);
	}
	/* This function allocates program */
	res = which_format_command_do_i_need(format, program);
	if (strstr(program, "kludge")) {
		asprintf(&tmp, "%s %s /", program, device);
	} else {
		asprintf(&tmp, "%s %s", program, device);
	}
	paranoid_free(program);

	asprintf(&program, "sh -c 'echo -en \"y\\ny\\ny\\n\" | %s'", tmp);
	paranoid_free(tmp);

	asprintf(&tmp, "Formatting %s as %s", device, format);
	update_progress_form(tmp);

	res = run_program_and_log_output(program, FALSE);
	if (res && strstr(program, "kludge")) {
#ifdef __FreeBSD__
		paranoid_free(program);
		asprintf(&program, "newfs_msdos -F 32 %s", device);
#else
#ifdef __IA64__
		/* For EFI partitions take fat16 
		 * as we want to make small ones */
		paranoid_free(program);
		asprintf(&program, "mkfs -t %s -F 16 %s", format, device);
#else
		paranoid_free(program);
		asprintf(&program, "mkfs -t %s -F 32 %s", format, device);
#endif
#endif
		res = run_program_and_log_output(program, FALSE);
		if (g_fprep) {
			fprintf(g_fprep, "%s\n", program);
		}
	}
	paranoid_free(program);

	retval += res;
	if (retval) {
		asprintf(&tmp1, "%s%s",tmp, _("...failed"));
	} else {
		asprintf(&tmp1, "%s%s",tmp, _("...OK"));
	}
	paranoid_free(tmp);

	log_to_screen(tmp1);
	paranoid_free(tmp1);
	sync();
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
					  bool interactively, struct raidlist_itself *raidlist)
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
	asprintf(&tmp, "format_everything (mountlist, interactively = %s",
			(interactively) ? "true" : "false");
	log_it(tmp);
	paranoid_free(tmp);

	mvaddstr_and_log_it(g_currentY, 0, _("Formatting partitions     "));
	open_progress_form(_("Formatting partitions"),
					   _("I am now formatting your hard disk partitions."),
					   _("This may take up to five minutes."), "",
					   mountlist->entries + 1);

	progress_step =
		(mountlist->entries >
		 0) ? g_maximum_progress / mountlist->entries : 1;
// start soft-raids now (because LVM might depend on them)
// ...and for simplicity's sake, let's format them at the same time :)
	log_msg(1, "Stopping all RAID devices");
	stop_all_raid_devices(mountlist);
	sync();
	sync();
	sync();
	sleep(2);
	log_msg(1, "Prepare soft-RAIDs");	// prep and format too
	for (lino = 0; lino < mountlist->entries; lino++) {
		me = &mountlist->el[lino];	// the current mountlist entry
		log_msg(2, "Examining %s", me->device);
		if (!strncmp(me->device, "/dev/md", 7)) {
			if (interactively) {
				// ask user if we should format the current device
				asprintf(&tmp, "Shall I format %s (%s) ?", me->device,
						me->mountpoint);
				do_it = ask_me_yes_or_no(tmp);
				paranoid_free(tmp);
			} else {
				do_it = TRUE;
			}
			if (do_it) {
				// NB: format_device() also stops/starts RAID device if necessary
				retval += format_device(me->device, me->format, raidlist);
			}
			g_current_progress += progress_step;
		}
	}
	sync();
	sync();
	sync();
	sleep(2);
// This last step is probably necessary
//  log_to_screen("Re-starting software RAIDs...");
//  start_all_raid_devices(mountlist);
//  system("sync"); system("sync"); system("sync"); 
//  sleep(5);
// do LVMs now
	log_msg(1, "Creating LVMs");
	if (does_file_exist("/tmp/i-want-my-lvm")) {
		wait_until_software_raids_are_prepped("/proc/mdstat", 100);
		log_to_screen(_("Configuring LVM"));
		if (!g_text_mode) {
			newtSuspend();
		}
		res = do_my_funky_lvm_stuff(FALSE, TRUE);
		if (!g_text_mode) {
			newtResume();
		}
		if (!res) {
			log_to_screen("LVM initialized OK");
		} else {
			log_to_screen("Failed to initialize LVM");
		}
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
			asprintf(&tmp, "Not formatting %s - it's an image", me->device);
			log_it(tmp);
			paranoid_free(tmp);
		} else if (!strcmp(me->format, "raid")) {
			asprintf(&tmp, "Not formatting %s - it's a raid-let",
					me->device);
			log_it(tmp);
			paranoid_free(tmp);
			continue;
		} else if (!strcmp(me->format, "lvm")) {
			asprintf(&tmp, "Not formatting %s - it's an LVM", me->device);
			log_it(tmp);
			paranoid_free(tmp);
			continue;
		} else if (!strncmp(me->device, "/dev/md", 7)) {
			asprintf(&tmp, "Already formatted %s - it's a soft-RAID dev",
					me->device);
			log_it(tmp);
			paranoid_free(tmp);
			continue;
		} else if (!does_file_exist(me->device)
				   && strncmp(me->device, "/dev/hd", 7)
				   && strncmp(me->device, "/dev/sd", 7)) {
			asprintf(&tmp,
					"Not formatting %s yet - doesn't exist - probably an LVM",
					me->device);
			log_it(tmp);
			paranoid_free(tmp);
			continue;
		} else {
			if (interactively) {
				// ask user if we should format the current device
				asprintf(&tmp, "Shall I format %s (%s) ?", me->device,
						me->mountpoint);
				do_it = ask_me_yes_or_no(tmp);
				paranoid_free(tmp);
			} else {
				do_it = TRUE;
			}

			if (do_it)
				retval += format_device(me->device, me->format, raidlist);
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
		mvaddstr_and_log_it(g_currentY++, 74, _("Failed."));
		log_to_screen
			(_
			 ("Errors occurred during the formatting of your hard drives."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
	}

	asprintf(&tmp, "format_everything () - %s",
			(retval) ? "failed!" : "finished successfully");
	log_it(tmp);
	paranoid_free(tmp);

	if (g_partition_table_locked_up > 0) {
		if (retval > 0 && !interactively) {
//123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
			log_to_screen
				(_
				 ("Partition table locked up %d times. At least one 'mkfs' (format) command"),
				 g_partition_table_locked_up);
			log_to_screen(_
						  ("failed. I think these two events are related. Sometimes, fdisk's ioctl() call"));
			log_to_screen(_
						  ("to refresh its copy of the partition table causes the kernel to lock the "));
			log_to_screen(_
						  ("partition table. I believe this has just happened."));
			if (ask_me_yes_or_no
				(_
				 ("Please choose 'yes' to reboot and try again; or 'no' to ignore this warning and continue.")))
			{
				sync();
				sync();
				sync();
				system("reboot");
			}
		} else {
			log_to_screen
				(_
				 ("Partition table locked up %d time%c. However, disk formatting succeeded."),
				 g_partition_table_locked_up,
				 (g_partition_table_locked_up == 1) ? '.' : 's');
		}
	}
	newtSuspend();
	system("clear");
	newtResume();
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

	assert_string_is_neither_NULL_nor_zerolength(drivename);

	if (devno_we_must_allow_for >= 5) {
		asprintf(&tmp, "Making dummy primary %s%d", drivename, 1);
		log_it(tmp);
		paranoid_free(tmp);

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
		asprintf(&tmp, "Creating dummy partition %s%d", drivename,
				current_devno);
		log_it(tmp);
		paranoid_free(tmp);

		g_maximum_progress++;
		res =
			partition_device(pout_to_fdisk, drivename, current_devno,
							 previous_devno, OSSWAP("ext2", "ufs"), 32000);
		retval += res;
		previous_devno = current_devno;
	}
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
	char *lnamebuf;
	int f;
	u_int secsize, u;
	off_t mediasize;

	asprintf(&lnamebuf, "%s", dkname);
	if ((f = open(lnamebuf, O_RDONLY)) == -1) {
		warn("cannot open %s", lnamebuf);
		paranoid_free(lnamebuf);
		return (NULL);
	}
	paranoid_free(lnamebuf);

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
	char *subdev_str;
	char *command;
	struct disklabel *lp;
	int i, lo = 0;
	int retval = 0;
	char c;
	FILE *ftmp;

	lp = get_virgin_disklabel(drivename);
	for (c = 'a'; c <= 'z'; ++c) {
		int idx;
		asprintf(&subdev_str, "%s%c", drivename, c);
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
		paranoid_free(subdev_str);
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
	asprintf(&command, "disklabel -wr %s auto", canonical_name(drivename));
	retval += run_program_and_log_output(command, TRUE);
	paranoid_free(command);

	asprintf(&command, "disklabel -R %s /tmp/disklabel",
			canonical_name(drivename));
	retval += run_program_and_log_output(command, TRUE);
	paranoid_free(command);
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
	int current_devno = 0;
	int previous_devno = 0;
	int lino = 0;
	int retval = 0;
	int i = 0;
	FILE *pout_to_fdisk = NULL;

#ifdef __FreeBSD__
	bool fbsd_part = FALSE;
	char *subdev_str = NULL;
#endif

	/** long long *******************************************************/
	long long partsize;

	/** buffers *********************************************************/
	char *device_str = NULL;
	char *format = NULL;
	char *tmp = NULL;
	char *tmp1 = NULL;

	/** end *************************************************************/

	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drivename);

	asprintf(&tmp, "Partitioning drive %s", drivename);
	log_it(tmp);
	paranoid_free(tmp);

#if __FreeBSD__
	log_it("(Not opening fdisk now; that's the Linux guy's job)");
	pout_to_fdisk = NULL;
#else
	make_hole_for_file(FDISK_LOG);
	asprintf(&tmp, "parted2fdisk %s >> %s 2>> %s", drivename, FDISK_LOG,
			FDISK_LOG);
	pout_to_fdisk = popen(tmp, "w");
	paranoid_free(tmp);

	if (!pout_to_fdisk) {
		log_to_screen(_("Cannot call parted2fdisk to configure %s"),
					  drivename);
		return (1);
	}
#endif
	for (current_devno = 1; current_devno < 99; current_devno++) {
		device_str = build_partition_name(drivename, current_devno);
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
					asprintf(&subdev_str, "%s%c", drivename, c);
					if (find_device_in_mountlist(mountlist, subdev_str) > 0) {
						fbsd_part = TRUE;
					}
					paranoid_free(subdev_str);
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
							(_
							 ("Warning! Unable to make the drive bootable."));
					}
					paranoid_free(device_str);

					return r;
				}
			}
			for (c = 'a'; c <= 'z'; c++) {
				asprintf(&subdev_str, "%s%c", device_str, c);
				if (find_device_in_mountlist(mountlist, subdev_str) > 0) {
					fbsd_part = TRUE;
				}
				paranoid_free(subdev_str);
			}
			// Now we check the subpartitions of the current partition.
			if (fbsd_part) {
				int i, line;

				asprintf(&format, "ufs");
				partsize = 0;
				for (i = 'a'; i < 'z'; ++i) {
					asprintf(&subdev_str, "%s%c", device_str, i);
					line = find_device_in_mountlist(mountlist, subdev_str);
					paranoid_free(subdev_str);

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
			file = open(drivename, O_WRONLY);
			if (!file) {
				asprintf(&tmp,
						_("Warning - unable to open %s for wiping it's partition table"),
						drivename);
				log_to_screen(tmp);
 				paranoid_free(tmp);
			}

			for (i = 0; i < 512; i++) {
				if (!write(file, "\0", 1)) {
					asprintf(&tmp, _("Warning - unable to write to %s"),
							drivename);
					log_to_screen(tmp);
 					paranoid_free(tmp);
				}
			}
			sync();
#else
			iamhere("New, kernel-friendly partition remover");
			for (i = 20; i > 0; i--) {
				fprintf(pout_to_fdisk, "d\n%d\n", i);
				fflush(pout_to_fdisk);
			}
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

			asprintf(&format, mountlist->el[lino].format);
			partsize = mountlist->el[lino].size;

#ifdef __FreeBSD__
		}
#endif

		if (current_devno == 5 && previous_devno == 4) {
			log_to_screen
				(_
				 ("You must leave at least one partition spare as the Extended partition."));
			paranoid_free(device_str);
			paranoid_free(format);

			return (1);
		}

		retval +=
			partition_device(pout_to_fdisk, drivename, current_devno,
							 previous_devno, format, partsize);

#ifdef __FreeBSD__
		if ((current_devno <= 4) && fbsd_part) {
			asprintf(&tmp, "disklabel -B %s", basename(device_str));
			retval += label_drive_or_slice(mountlist, device_str, 0);
			if (system(tmp)) {
				log_to_screen
					(_("Warning! Unable to make the slice bootable."));
			}
			paranoid_free(tmp);
		}
#endif

		previous_devno = current_devno;
	}
	paranoid_free(device_str);
	paranoid_free(format);

	if (pout_to_fdisk) {
		// mark relevant partition as bootable
		tmp1 = call_program_and_get_last_line_of_output
				("make-me-bootable /tmp/mountlist.txt dummy");
		asprintf(&tmp, "a\n%s\n", tmp1);
		paranoid_free(tmp1);

		fput_string_one_char_at_a_time(pout_to_fdisk, tmp);
		paranoid_free(tmp);

		// close fdisk
		fput_string_one_char_at_a_time(pout_to_fdisk, "w\n");
		sync();
		paranoid_pclose(pout_to_fdisk);
		log_msg(0,
				"------------------- fdisk.log looks like this ------------------");
		asprintf(&tmp, "cat %s >> %s", FDISK_LOG, MONDO_LOGFILE);
		system(tmp);
		paranoid_free(tmp);

		log_msg(0,
				"------------------- end of fdisk.log... word! ------------------");
		asprintf(&tmp, "tail -n6 %s | grep -F \"16: \"", FDISK_LOG);
		if (!run_program_and_log_output(tmp, 5)) {
			g_partition_table_locked_up++;
			log_to_screen
				(_
				 ("A flaw in the Linux kernel has locked the partition table."));
		}
		paranoid_free(tmp);
	}
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
	char *output;

	/** pointers **********************************************************/
	char *p;
	char *part_table_fmt;
	FILE *fout;

	/** end ***************************************************************/

	assert_string_is_neither_NULL_nor_zerolength(drive);
	assert(format != NULL);

	log_it("partition_device('%s', %d, %d, '%s', %lld) --- starting",
		   drive, partno, prev_partno, format, partsize);

	if (!strncmp(drive, RAID_DEVICE_STUB, strlen(RAID_DEVICE_STUB))) {
		asprintf(&tmp, "Not partitioning %s - it is a virtual drive", drive);
		log_it(tmp);
		paranoid_free(tmp);
		return (0);
	}
	partition_name = build_partition_name(drive, partno);
	if (partsize <= 0) {
		asprintf(&tmp, "Partitioning device %s (max size)", partition_name);
	} else {
		asprintf(&tmp, "Partitioning device %s (%lld MB)", partition_name,
				(long long) partsize / 1024);
	}
	update_progress_form(tmp);
	log_it(tmp);
	paranoid_free(tmp);

	if (is_this_device_mounted(partition_name)) {
		asprintf(&tmp, _("%s is mounted, and should not be partitioned"),
				partition_name);
		paranoid_free(partition_name);

		log_to_screen(tmp);
		paranoid_free(tmp);
		return (1);
	}

	p = (char *) strrchr(partition_name, '/');
	asprintf(&program, "parted2fdisk %s >> %s 2>> %s", drive, MONDO_LOGFILE,
			MONDO_LOGFILE);

	/* BERLIOS: should not be called each time */
	part_table_fmt = which_partition_format(drive);
	/* make it a primary/extended/logical */
	if (partno <= 4) {
		asprintf(&output,"n\np\n%d\n", partno);
	} else {
		/* MBR needs an extended partition if more than 4 partitions */
		if (strcmp(part_table_fmt, "MBR") == 0) {
			if (partno == 5) {
				if (prev_partno >= 4) {
					log_to_screen
						(_
						 ("You need to leave at least one partition free, for 'extended/logical'"));
					paranoid_free(partition_name);
					paranoid_free(program);

					paranoid_free(output);
					return (1);
				} else {
					asprintf(&output,"n\ne\n%d\n\n\n",prev_partno + 1);
				}
			}
			asprintf(&tmp,"%sn\nl\n",output);
			paranoid_free(output);
			output = tmp;
		} else {
			/* GPT allows more than 4 primary partitions */
			asprintf(&output,"n\np\n%d\n",partno);
		}
	}
	paranoid_free(part_table_fmt);
	/*start block (ENTER for next free blk */
	asprintf(&tmp,"%s\n",output);
	paranoid_free(output);
	output = tmp;

	if (partsize > 0) {
		if (!strcmp(format, "7")) {
			log_msg(1, "Adding 512K, just in case");
			partsize += 512;
		}
		asprintf(&tmp,"%s+%lldK", output,  (long long) (partsize));
		paranoid_free(output);
		output = tmp;
	}
	asprintf(&tmp,"%s\n",output);
	paranoid_free(output);
	output = tmp;
#if 0
/*
#endif
	asprintf(&tmp,"PARTSIZE = +%ld",(long)partsize);
	log_it(tmp);
	paranoid_free(tmp);

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
		tmp = last_line_of_file(FDISK_LOG);
		if (strstr(tmp, " (m ")) {
			log_msg(1, "Successfully created %s%d", drive, partno);
		} else {
			log_msg(1, "last line = %s", tmp);
			log_msg(1, "Failed to create %s%d; sending 'Enter'...", drive,
					partno);
		}
		paranoid_free(tmp);

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
		asprintf(&tmp,"%sw\n\n",output);
		paranoid_free(output);
		output = tmp;

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
				asprintf(&tmp, "Failed to vacuum-pack %s", partition_name);
				log_it(tmp);
				paranoid_free(tmp);

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
				asprintf(&tmp, "Partitioned %s but failed to set its type",
						partition_name);
				log_it(tmp);
				paranoid_free(tmp);
			} else {
				if (partsize > 0) {
					asprintf(&tmp, "Partition %s created+configured OK",
							partition_name);
					log_to_screen(tmp);
					paranoid_free(tmp);
				} else {
					log_it("Returning from a successful vacuum-pack");
				}
			}
		} else {
			asprintf(&tmp, "Failed to partition %s", partition_name);
			if (partsize > 0) {
				log_to_screen(tmp);
			} else {
				log_it(tmp);
			}
			paranoid_free(tmp);
			retval++;
		}
	}
	g_current_progress++;
	log_it("partition_device() --- leaving");
	paranoid_free(partition_name);
	paranoid_free(program);
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

	open_progress_form(_("Partitioning devices"),
					   _("I am now going to partition all your drives."),
					   _("This should not take more than five minutes."),
					   "", mountlist->entries);

	make_list_of_drives_in_mountlist(mountlist, drivelist);

	/* partition each drive */
	for (lino = 0; lino < drivelist->entries; lino++) {
		res = partition_drive(mountlist, drivelist->el[lino].device);
		retval += res;
	}
	close_progress_form();
	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, _("Failed."));
		log_to_screen
			(_
			 ("Errors occurred during the partitioning of your hard drives."));
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, _("Done."));
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
	char *partition = NULL;
	char *command = NULL;
	char *output = NULL;
	char *tmp = NULL;
	char *tmp1 = NULL;
	char *partcode = NULL;

	/** pointers *********************************************************/
	char *p = NULL;
	FILE *fout = NULL;

	/** int **************************************************************/
	int res = 0;

	/** end **************************************************************/

	assert_string_is_neither_NULL_nor_zerolength(drive);
	assert(format != NULL);

	partition = build_partition_name(drive, partno);
	p = (char *) strrchr(partition, '/');
	if (strcmp(format, "swap") == 0) {
		asprintf(&partcode, "82");
	} else if (strcmp(format, "vfat") == 0) {
		if (partsize / 1024 > 8192) {
			asprintf(&partcode, "c");
		} else {
			asprintf(&partcode, "b");
		}
	} else if (strcmp(format, "ext2") == 0
			   || strcmp(format, "reiserfs") == 0
			   || strcmp(format, "ext3") == 0 || strcmp(format, "xfs") == 0
			   || strcmp(format, "jfs") == 0) {
		asprintf(&partcode, "83");
	} else if (strcmp(format, "minix") == 0) {
		asprintf(&partcode, "81");
	} else if (strcmp(format, "raid") == 0) {
		asprintf(&partcode, "fd");
	} else if ((strcmp(format, "ufs") == 0)
			   || (strcmp(format, "ffs") == 0)) {	/* raid autodetect */
		asprintf(&partcode, "a5");
	} else if (strcmp(format, "lvm") == 0) {
		asprintf(&partcode, "8e");
	} else if (format[0] == '\0') {	/* LVM physical partition */
		asprintf(&partcode, "");
	} else if (strlen(format) >= 1 && strlen(format) <= 2) {
		asprintf(&partcode, format);
	} else {
		/* probably an image */
		asprintf(&tmp,
				"Unknown format ('%s') - using supplied string anyway",
				format);
		mvaddstr_and_log_it(g_currentY++, 0, tmp);
		paranoid_free(tmp);
#ifdef __FreeBSD__
		asprintf(&partcode, format);	// was a5
#else
		asprintf(&partcode, format);	// was 83
#endif
	}
	asprintf(&tmp, "Setting %s's type to %s (%s)", partition, format,
			partcode);
	paranoid_free(partition);

	log_msg(1, tmp);
	paranoid_free(tmp);
	if (partcode[0] != '\0' && strcmp(partcode, "83")) {	/* no need to set type if 83: 83 is default */

		if (pout_to_fdisk) {
			res = 0;
			fput_string_one_char_at_a_time(pout_to_fdisk, "t\n");
			tmp1 = last_line_of_file(FDISK_LOG);
			if (partno > 1
				|| strstr(tmp1, " (1-4)")) {
				log_msg(5, "Specifying partno (%d) - yay", partno);
				asprintf(&tmp, "%d\n", partno);
				fput_string_one_char_at_a_time(pout_to_fdisk, tmp);
				paranoid_free(tmp);
				paranoid_free(tmp1);
				tmp1 = last_line_of_file(FDISK_LOG);
				log_msg(5, "A - last line = '%s'", tmp1);
			}
			paranoid_free(tmp1);

			asprintf(&tmp, "%s\n", partcode);
			fput_string_one_char_at_a_time(pout_to_fdisk, tmp);
			paranoid_free(tmp);
			tmp1 = last_line_of_file(FDISK_LOG);
			log_msg(5, "B - last line = '%s'",tmp1);
			paranoid_free(tmp1);

			fput_string_one_char_at_a_time(pout_to_fdisk, "\n");
			tmp1 = last_line_of_file(FDISK_LOG);
			log_msg(5, "C - last line = '%s'",tmp1);
			paranoid_free(tmp1);

			tmp = last_line_of_file(FDISK_LOG);
			if (!strstr(tmp, " (m ")) {
				log_msg(1, "last line = '%s'; part type set failed", tmp);
				res++;
				fput_string_one_char_at_a_time(pout_to_fdisk, "\n");
			}
			paranoid_free(tmp);
			fput_string_one_char_at_a_time(pout_to_fdisk, "p\n");
		} else {
			asprintf(&output, "t\n%d\n%s\nw\n", partno, partcode);
			asprintf(&command, "parted2fdisk %s >> %s 2>> %s", drive,
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
			paranoid_free(command);
			paranoid_free(output);
		}
		/* BERLIOS: Useless as command not initialized in all cases
		if (res) {
			log_OS_error(command);
		}
		*/
	}
	paranoid_free(partcode);


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

#ifdef __FreeBSD__
	if (is_this_device_mounted(raid_device)) {
		log_it("Can't start %s when it's mounted!", raid_device);
		return 1;
	}
	asprintf(&program, "vinum start -f %s", raid_device);
#else
	asprintf(&program, "raidstart %s", raid_device);
//      sprintf (program, "raidstart " RAID_DEVICE_STUB "*");
#endif
	log_msg(1, "program = %s", program);
	res = run_program_and_log_output(program, 1);
	if (g_fprep) {
		fprintf(g_fprep, "%s\n", program);
	}
	paranoid_free(program);

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

#ifdef __FreeBSD__
	if (is_this_device_mounted(raid_device)) {
		log_it("Can't stop %s when it's mounted!", raid_device);
		return 1;
	}
	asprintf(&program, "vinum stop -f %s", raid_device);
#else
	// use raidstop if it exists, otherwise use mdadm
	if (run_program_and_log_output("which raidstop", FALSE)) {
		asprintf(&program, "mdadm -S %s", raid_device);
	} else {
		asprintf(&program, "raidstop %s", raid_device);
	}
#endif
	log_msg(1, "program = %s", program);
	res = run_program_and_log_output(program, 1);
	if (g_fprep) {
		fprintf(g_fprep, "%s\n", program);
	}
	paranoid_free(program);

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
	int res = 0;
#endif

	/** char ************************************************************/
	char *incoming = NULL;
#ifndef __FreeBSD__
	char *dev = NULL;
#endif
	/** pointers ********************************************************/
#ifndef __FreeBSD__
	char *p = NULL;
#endif
	FILE *fin = NULL;
	int i = 0;
	size_t n = 0;

	/** end ****************************************************************/

	assert(mountlist != NULL);

	for (i = 0; i < 3; i++) {
#ifdef __FreeBSD__
		fin =
			popen
			("vinum list | grep '^[PVS]' | sed 's/S/1/;s/P/2/;s/V/3/' | sort | cut -d' ' -f2",
			 "r");
		if (!fin) {
			return (1);
		}
		for (getline(&incoming, &n, fin); !feof(fin);
			 getline(&incoming, &n, fin)) {
			retval += stop_raid_device(incoming);
		}
#else
		fin = fopen("/proc/mdstat", "r");
		if (!fin) {
			log_OS_error("/proc/mdstat");
			return (1);
		}
		for (getline(&incoming, &n, fin); !feof(fin);
			 getline(&incoming, &n, fin)) {
			for (p = incoming;
				 *p != '\0' && (*p != 'm' || *(p + 1) != 'd'
								|| !isdigit(*(p + 2))); p++);
			if (*p != '\0') {
				asprintf(&dev, "/dev/%s", p);
				/* BERLIOS : 32 Hard coded value */
				for (p = dev; *p > 32; p++);
				*p = '\0';
				res = stop_raid_device(dev);
				paranoid_free(dev);
			}
		}
#endif
		paranoid_free(incoming);
	}
	paranoid_fclose(fin);
	if (retval) {
		log_msg(1, "Warning - unable to stop some RAID devices");
	}
	sync();
	sync();
	sync();
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
		asprintf(&tmp, "Unknown format (%s) - assuming '%s' will do", format,
				program);
		log_it(tmp);
		paranoid_free(tmp);
		res = 0;
	}
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

	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drive_name);

	for (original_size_of_drive = 0, partno = 0;
		 partno < mountlist->entries; partno++) {
		if (strncmp
			(mountlist->el[partno].device, drive_name,
			 strlen(drive_name)) == 0) {
			original_size_of_drive += mountlist->el[partno].size;
		} else {
			asprintf(&tmp, "Skipping %s", mountlist->el[partno].device);
//                      log_to_screen(tmp);
			paranoid_free(tmp);
		}
	}
	original_size_of_drive = original_size_of_drive / 1024;
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

	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(drive_name);

	if (strlen(drive_name) >= strlen(RAID_DEVICE_STUB)) {
		if (strncmp(drive_name, RAID_DEVICE_STUB, strlen(RAID_DEVICE_STUB))
			== 0) {
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
		return;
	}
	asprintf(&tmp, _("Expanding entries to suit drive %s (%ld MB)"),
			drive_name, current_size_of_drive);
	log_to_screen(tmp);
	paranoid_free(tmp);

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
		asprintf(&tmp, _("Cannot resize %s's entries. Drive not found."),
				drive_name);
		log_to_screen(tmp);
		paranoid_free(tmp);
		return;
	}
	factor =
		(float) (current_size_of_drive) / (float) (original_size_of_drive);
	asprintf(&tmp, "Disk %s was %ld MB; is now %ld MB; factor = %f",
			drive_name, original_size_of_drive, current_size_of_drive,
			factor);
	log_to_screen(tmp);
	paranoid_free(tmp);

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
		asprintf(&tmp, _("Changing %s from %lld KB to %ld KB"),
				drivemntlist->el[partno]->device,
				drivemntlist->el[partno]->size, newsizL);
		log_to_screen(tmp);
		paranoid_free(tmp);
		drivemntlist->el[partno]->size = newsizL;
	}
	final_size = get_phys_size_of_drive(drive_name);
	asprintf(&tmp, _("final_size = %ld MB"), final_size);
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
	log_to_screen(_("Mountlist adjusted to suit current hard drive(s)"));
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

	asprintf(&tmp_drive_name,drive_name);
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
	paranoid_free(tmp_drive_name);
}

/* @} - end of prepGroup */
