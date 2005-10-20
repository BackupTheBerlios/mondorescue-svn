/* $Id$
misc tools
*/

/**
 * @file
 * Miscellaneous tools that didn't really fit anywhere else.
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "lib-common-externs.h"
#include "libmondo-tools.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-raid-EXT.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*@unused@*/
//static char cvsid[] = "$Id$";

extern int g_tape_buffer_size_MB;
extern char *g_erase_tmpdir_and_scratchdir;
extern char *g_serial_string;
extern bool g_text_mode;
extern int g_currentY;
extern int g_current_media_number;

/**
 * @addtogroup globalGroup
 * @{
 */
bool g_remount_cdrom_at_end,	///< TRUE if we unmounted the CD-ROM and should remount it when done with the backup.
 g_remount_floppy_at_end;		///< TRUE if we unmounted the floppy and should remount it when done with the backup.
bool g_cd_recovery;				///< TRUE if we're making an "autonuke" backup.
double g_kernel_version;

/**
 * The place where /boot is mounted.
 */
char *g_boot_mountpt = NULL;

/**
 * The location of Mondo's home directory.
 */
char *g_mondo_home = NULL;

/**
 * The serial string (used to differentiate between backups) of the current backup.
 */
char *g_serial_string = NULL;

/**
 * The location where tmpfs is mounted, or "" if it's not mounted.
 */
char *g_tmpfs_mountpt = NULL;
char *g_magicdev_command = NULL;

/**
 * The default maximum level to log messages at or below.
 */
int g_loglevel = DEFAULT_DEBUG_LEVEL;

/* @} - end of globalGroup */


extern pid_t g_buffer_pid;
extern pid_t g_main_pid;

extern t_bkptype g_backup_media_type;

extern bool am_I_in_disaster_recovery_mode(void);


/**
 * @addtogroup utilityGroup
 * @{
 */
/**
 * Assertion handler. Prints a friendly message to the user,
 * offering to ignore all, dump core, break to debugger,
 * exit, or ignore. Intended to be used with an assert() macro.
 *
 * @param file The file in which the assertion triggered.
 * @param function The function (@c __FUNCTION__) in which the assertion triggered.
 * @param line The line number of the assert() statement.
 * @param exp The expression that failed (as a string).
 */
void _mondo_assert_fail(const char *file,
						const char *function, int line, const char *exp)
{
	static int ignoring_assertions = 0;
	bool is_valid = TRUE;

	log_it("ASSERTION FAILED: `%s' at %s:%d in %s", exp, file, line,
		   function);
	if (ignoring_assertions) {
		log_it("Well, the user doesn't care...");
		return;
	}
#ifndef _XWIN
	if (!g_text_mode)
		newtSuspend();
#endif
	printf("ASSERTION FAILED: `%s'\n", exp);
	printf("\tat %s:%d in %s\n\n", file, line, function);
	printf("(I)gnore, ignore (A)ll, (D)ebug, a(B)ort, or (E)xit? ");
	do {
		is_valid = TRUE;
		switch (toupper(getchar())) {
		case 'A':				// ignore (A)ll
			ignoring_assertions = 1;
			break;
		case 'B':				// a(B)ort
			signal(SIGABRT, SIG_DFL);	/* prevent SIGABRT handler from running */
			raise(SIGABRT);
			break;				/* "can't get here" */
		case 'D':				// (D)ebug, aka asm("int 3")
#ifdef __IA32__
			__asm__ __volatile__("int $3");	// break to debugger
#endif
			break;
		case 'E':				// (E)xit
			fatal_error("Failed assertion -- see above for details");
			break;				/* "can't get here" */
		case 'I':				// (I)gnore
			break;
			/* These next two work as follows:
			   the `default' catches the user's invalid choice and says so;
			   the '\n' catches the newline on the end and prints the prompt again.
			 */
		case '\n':
			printf
				("(I)gnore, ignore (A)ll, (D)ebug, a(B)ort, or (E)xit? ");
			break;
		default:
			is_valid = FALSE;
			printf("Invalid choice.\n");
			break;
		}
	} while (!is_valid);

	if (ignoring_assertions) {
		log_it("Ignoring ALL assertions from now on.");
	} else {
		log_it("Ignoring assertion: %s", exp);
	}

	getchar();					// skip \n

#ifndef _XWIN
	if (!g_text_mode)
		newtResume();
#endif
}

/**
 * Clean's up users' KDE desktops.
 * @bug Details about this function are unknown.
 */
void clean_up_KDE_desktop_if_necessary(void)
{
	char *tmp;

	malloc_string(tmp);
	strcpy(tmp,
		   "for i in `find /root /home -type d -name Desktop -maxdepth 2`; do \
file=$i/.directory; if [ -f \"$file\" ] ; then mv -f $file $file.old ; \
cat $file.old | awk '{if (index($0, \"rootimagesmindi\")) { while (length($0)>2) { getline;} ; } \
else { print $0;};}' > $file ; fi ; done");
	run_program_and_log_output(tmp, 5);
	paranoid_free(tmp);
}


/**
 * Locate mondoarchive's home directory. Searches in /usr/local/mondo, /usr/share/mondo,
 * /usr/local/share/mondo, /opt, or if all else fails, search /usr.
 *
 * @param home_sz String to store the home directory ("" if it could not be found).
 * @return 0 for success, nonzero for failure.
 */
int find_and_store_mondoarchives_home(char *home_sz)
{
	assert(home_sz != NULL);
	strcpy(home_sz,
		   call_program_and_get_last_line_of_output
		   ("find /usr/lib/ /usr/local/ /usr/share/ /usr/local/share/ /opt/ /ramdisk/usr/share/ -type d -maxdepth 2 -name include -prune -o -type d -maxdepth 2 -path '*/mondo/restore-scripts' -printf '%h\n' 2> /dev/null"));

	if (home_sz[0] == '\0') {
		strcpy(home_sz,
			   call_program_and_get_last_line_of_output
			   ("find /usr -type d -path '*/mondo/restore-scripts' -follow -maxdepth 3 -printf '%h\n' 2> /dev/null"));
	}
	if (home_sz[0] == '\0') {
		return (1);
	} else {
		return (0);
	}
}


char *get_architecture()
{
#ifdef __IA32__
	return ("i386");
#endif
#ifdef __IA64__
	return ("ia64");
#endif
	return ("unknown");
}



double get_kernel_version()
{
	char *p, tmp[200];
	double d;
#ifdef __FreeBSD__
	// JOSH - FIXME :)
	d = 5.2;					// :-)
#else
	strcpy(tmp, call_program_and_get_last_line_of_output("uname -r"));
	p = strchr(tmp, '.');
	if (p) {
		p = strchr(++p, '.');
		if (p) {
			while (*p) {
				*p = *(p + 1);
				p++;
			}
		}
	}
//  log_msg(1, "tmp = '%s'", tmp);
	d = atof(tmp);
#endif
	log_msg(1, "g_kernel_version = %f", d);
	return (d);
}





/**
 * Get the current time.
 * @return number of seconds since the epoch.
 */
long get_time()
{

#if 0

	/*@ pointers **************************************************** */
	FILE *fin;

	/*@ buffers ***************************************************** */
	char incoming[MAX_STR_LEN];

	/*@ end vars **************************************************** */

	if (!(fin = popen("date +%s", "r"))) {
		log_OS_error("Cannot popen date");
		return (0);
	}
	fgets(incoming, MAX_STR_LEN - 1, fin);
	paranoid_pclose(fin);
	return (atol(incoming));
#else
	return (long) time((void *) 0);
#endif
}







/**
 * Initialize a RAID volume structure, setting fields to zero. The
 * actual hard drive is unaffected.
 *
 * @param raidrec The RAID volume structure to initialize.
 * @note This function is system dependent.
 */
#ifdef __FreeBSD__
void initialize_raidrec(struct vinum_volume *raidrec)
{
	int i, j;
	raidrec->volname[0] = '\0';
	raidrec->plexes = 0;
	for (i = 0; i < 9; ++i) {
		raidrec->plex[i].raidlevel = -1;
		raidrec->plex[i].stripesize = 0;
		raidrec->plex[i].subdisks = 0;
		for (j = 0; j < 9; ++j) {
			strcpy(raidrec->plex[i].sd[j].which_device, "");
		}
	}
}
#else
void initialize_raidrec(struct raid_device_record *raidrec)
{
	assert(raidrec != NULL);
	raidrec->raid_device[0] = '\0';
	raidrec->raid_level = 0;
	raidrec->chunk_size = 4;
	raidrec->persistent_superblock = 1;
	raidrec->data_disks.entries = 0;
	raidrec->spare_disks.entries = 0;
	raidrec->parity_disks.entries = 0;
	raidrec->failed_disks.entries = 0;
	raidrec->additional_vars.entries = 0;
}
#endif




/**
 * Insert modules that Mondo requires.
 * Currently inserts @c dos, @c fat, @c vfat, and @c osst for Linux;
 * @c msdosfs and @c ext2fs for FreeBSD.
 */
void insmod_crucial_modules(void)
{
#ifdef __FreeBSD__
	system("kldstat | grep msdosfs || kldload msdosfs 2> /dev/null");
	system("kldstat | grep ext2fs  || kldload ext2fs 2> /dev/null");
#else
	system("modprobe dos &> /dev/null");
	system("modprobe fat &> /dev/null");
	system("modprobe vfat &> /dev/null");
	//  system("modprobe osst &> /dev/null");
#endif
}


/**
 * Log a trace message to the trace file.
 * @bug This function seems orphaned. Please remove.
 */
void log_trace(char *o)
{
	/*@ pointers **************************************************** */
	FILE *fout;

	/*@ buffers ***************************************************** */
	char output[MAX_STR_LEN];

	/*@ int    ****************************************************** */
	int i;

	/*@ end vars *************************************************** */

	if (o[0] == '\0') {
		return;
	}
	strcpy(output, o);
	i = (int) strlen(output);
	if (i <= 0) {
		return;
	}
	if (output[i - 1] < 32) {
		output[i - 1] = '\0';
	}
	if (g_text_mode
		/* && !strstr(last_line_of_file(MONDO_LOGFILE),output) */ ) {
		printf("%s\n", output);
	}

	fout = fopen(MONDO_TRACEFILE, "a");
	if (fout) {
		fprintf(fout, "%s\n", output);
		paranoid_fclose(fout);
	} else {
		log_OS_error("Cannot write to tracefile");
	}
}





/**
 * Finish configuring the backup information structure. Call this function
 * to set the parameters that depend on those that can be given on the command
 * line.
 *
 * @param bkpinfo The backup information structure. Fields modified/used:
 * - Used: @c bkpinfo->backup_data
 * - Used: @c bkpinfo->backup_media_type
 * - Used: @c bkpinfo->cdrw_speed
 * - Used: @c bkpinfo->compression_level
 * - Used: @c bkpinfo->include_paths
 * - Used: @c bkpinfo->prefix
 * - Used: @c bkpinfo->isodir
 * - Used: @c bkpinfo->manual_cd_tray
 * - Used: @c bkpinfo->make_cd_use_lilo
 * - Used: @c bkpinfo->media_device
 * - Used: @c bkpinfo->nfs_mount
 * - Used: @c bkpinfo->nonbootable_backup
 * - Used: @c bkpinfo->scratchdir
 * - Used: @c bkpinfo->tmpdir
 * - Used: @c bkpinfo->use_lzo
 * - Modified: @c bkpinfo->call_before_iso
 * - Modified: @c bkpinfo->call_make_iso
 * - Modified: @c bkpinfo->optimal_set_size
 * - Modified: @c bkpinfo->zip_exe
 * - Modified: @c bkpinfo->zip_suffix
 * 
 * @return number of errors, or 0 for success.
 * @note Also creates directories that are specified in the @c bkpinfo structure but
 * do not exist.
 */
int post_param_configuration(struct s_bkpinfo *bkpinfo)
{
	char *extra_cdrom_params;
	char *mondo_mkisofs_sz;
	char *command;
	char *mtpt;
	char *hostname, *ip_address;
	int retval = 0;
	long avm = 0;
	char *colon;
	char *cdr_exe;
	char *tmp;
	int rdsiz_MB;
	char *iso_dev;
	char *iso_mnt;
	char *iso_tmp;
	char *iso_path;

	assert(bkpinfo != NULL);
	malloc_string(extra_cdrom_params);
	malloc_string(mondo_mkisofs_sz);
	malloc_string(command);
	malloc_string(mtpt);
	malloc_string(hostname);
	malloc_string(ip_address);
	malloc_string(cdr_exe);
	malloc_string(tmp);
	malloc_string(iso_dev);
	malloc_string(iso_mnt);
	malloc_string(iso_tmp);
	malloc_string(iso_path);
	bkpinfo->optimal_set_size =
		(IS_THIS_A_STREAMING_BACKUP(bkpinfo->backup_media_type) ? 4 : 8) *
		1024;

	log_msg(1, "Foo");
	if (bkpinfo->backup_media_type == tape) {
		log_msg(1, "Bar");
		sprintf(tmp, "mt -f %s status", bkpinfo->media_device);
		log_msg(1, "tmp = '%s'", tmp);
		if (run_program_and_log_output(tmp, 3)) {
			fatal_error
				("Unable to open tape device. If you haven't specified it with -d, do so. If you already have, check your parameter. I think it's wrong.");
		}
	}
	make_hole_for_dir(bkpinfo->scratchdir);
	make_hole_for_dir(bkpinfo->tmpdir);
	if (bkpinfo->backup_media_type == iso)
		make_hole_for_dir(bkpinfo->isodir);

	run_program_and_log_output("uname -a", 5);
	run_program_and_log_output("cat /etc/*issue*", 5);
	sprintf(g_tmpfs_mountpt, "%s/tmpfs", bkpinfo->tmpdir);
	sprintf(command, "mkdir -p %s", g_tmpfs_mountpt);
	paranoid_system(command);
	rdsiz_MB = PPCFG_RAMDISK_SIZE + g_tape_buffer_size_MB;
#ifdef __FreeBSD__
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("vmstat | tail -1 | tr -s ' ' | cut -d' ' -f6"));
	avm += atol(tmp);
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("swapinfo | grep -v Device | tr -s ' ' | cut -d' ' -f4 | tr '\n' '+' | sed 's/+$//' | bc"));
	avm += atol(tmp);
	sprintf(command, "mdmfs -s %d%c md9 %s", rdsiz_MB, 'm',
			g_tmpfs_mountpt);
#else
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("free | grep \":\" | tr -s ' ' '\t' | cut -f2 | head -n1"));
	avm += atol(tmp);
	sprintf(command, "mount /dev/shm -t tmpfs %s -o size=%d%c",
			g_tmpfs_mountpt, rdsiz_MB, 'm');
	run_program_and_log_output("cat /proc/cpuinfo", 5);
	run_program_and_log_output
		("rpm -q newt newt-devel slang slang-devel ncurses ncurses-devel gcc",
		 5);
#endif
	if (avm / 1024 > rdsiz_MB * 3) {
		if (run_program_and_log_output(command, 5)) {
			g_tmpfs_mountpt[0] = '\0';
			log_it("Failed to mount tmpfs");
		} else {
			log_it("Tmpfs mounted OK - %d MB", rdsiz_MB);
		}
	} else {
		g_tmpfs_mountpt[0] = '\0';
		log_it("It doesn't seem you have enough swap to use tmpfs. Fine.");
	}

	if (bkpinfo->use_lzo) {
		strcpy(bkpinfo->zip_exe, "lzop");
		strcpy(bkpinfo->zip_suffix, "lzo");
	} else if (bkpinfo->compression_level != 0) {
		strcpy(bkpinfo->zip_exe, "bzip2");
		strcpy(bkpinfo->zip_suffix, "bz2");
	} else {
		bkpinfo->zip_exe[0] = bkpinfo->zip_suffix[0] = '\0';
	}

// DVD

	if (bkpinfo->backup_media_type == dvd) {
		extra_cdrom_params[0] = '\0';
		mondo_mkisofs_sz[0] = '\0';
		if (find_home_of_exe("growisofs")) {
			strcpy(cdr_exe, "growisofs");
		}						// unlikely to be used
		else {
			fatal_error("Please install growisofs.");
		}
		if (bkpinfo->nonbootable_backup) {
			strcat(mondo_mkisofs_sz, MONDO_GROWISOFS_NONBOOT);
		} else if
#ifdef __FreeBSD__
			(TRUE)
#else
			(bkpinfo->make_cd_use_lilo)
#endif
#ifdef __IA64__
	{
		strcat(mondo_mkisofs_sz, MONDO_GROWISOFS_REGULAR_ELILO);
	}
#else
	{
		strcat(mondo_mkisofs_sz, MONDO_GROWISOFS_REGULAR_LILO);
	}
#endif
		else
		{
			strcat(mondo_mkisofs_sz, MONDO_GROWISOFS_REGULAR_SYSLINUX);
		}
		if (bkpinfo->manual_cd_tray) {
			fatal_error("Manual CD tray + DVD not supported yet.");
			// -m isn't supported by growisofs, BTW...
		} else {
			sprintf(bkpinfo->call_make_iso,
					"%s %s -Z %s . 2>> _ERR_",
					mondo_mkisofs_sz,
					extra_cdrom_params, bkpinfo->media_device);
		}
		if (getenv ("SUDO_COMMAND")) {
		  fatal_error("Can't write DVDs as sudo because growisofs doesn't support this - please see the growisofs manpage for details.");
		}
		log_msg(2, "call_make_iso (DVD res) is ... %s",
				bkpinfo->call_make_iso);
	}							// end of DVD code

// CD-R or CD-RW
	if (bkpinfo->backup_media_type == cdrw
		|| bkpinfo->backup_media_type == cdr) {
		extra_cdrom_params[0] = '\0';
		if (!bkpinfo->manual_cd_tray) {
			strcat(extra_cdrom_params, "-waiti ");
		}
		if (bkpinfo->backup_media_type == cdrw) {
			strcat(extra_cdrom_params, "blank=fast ");
		}
		if (find_home_of_exe("cdrecord")) {
			strcpy(cdr_exe, "cdrecord");
		} else if (find_home_of_exe("dvdrecord")) {
			strcpy(cdr_exe, "dvdrecord");
		} else {
			fatal_error("Please install either cdrecord or dvdrecord.");
		}
		if (bkpinfo->nonbootable_backup) {
			strcpy(mondo_mkisofs_sz, MONDO_MKISOFS_NONBOOT);
		} else if
#ifdef __FreeBSD__
			(TRUE)
#else
			(bkpinfo->make_cd_use_lilo)
#endif
#ifdef __IA64__
	{
		strcat(mondo_mkisofs_sz, MONDO_MKISOFS_REGULAR_ELILO);
	}
#else
	{
		strcpy(mondo_mkisofs_sz, MONDO_MKISOFS_REGULAR_LILO);
	}
#endif
		else
		{
			strcpy(mondo_mkisofs_sz, MONDO_MKISOFS_REGULAR_SYSLINUX);
		}
		if (bkpinfo->manual_cd_tray) {
			sprintf(bkpinfo->call_before_iso,
					"%s -o %s/temporary.iso . 2>> _ERR_", mondo_mkisofs_sz,
					bkpinfo->tmpdir);
			sprintf(bkpinfo->call_make_iso,
					"%s %s -v %s fs=4m dev=%s speed=%d %s/temporary.iso",
					cdr_exe, (bkpinfo->please_dont_eject) ? " " : "-eject",
					extra_cdrom_params, bkpinfo->media_device,
					bkpinfo->cdrw_speed, bkpinfo->tmpdir);
		} else {
			sprintf(bkpinfo->call_make_iso,
					"%s . 2>> _ERR_ | %s %s %s fs=4m dev=%s speed=%d -",
					mondo_mkisofs_sz, cdr_exe,
					(bkpinfo->please_dont_eject) ? " " : "-eject",
					extra_cdrom_params, bkpinfo->media_device,
					bkpinfo->cdrw_speed);
		}
	}							// end of CD code

	/*
	   if (bkpinfo->backup_data && bkpinfo->backup_media_type == tape)
	   {
	   sprintf (tmp,
	   "dd if=/dev/zero of=%s bs=%ld count=32 2> /dev/null",
	   bkpinfo->media_device, bkpinfo->internal_tape_block_size);
	   if (system(tmp))
	   {
	   retval++;
	   fprintf (stderr,
	   "Cannot write to tape device. Is the tape set read-only?\n");
	   }
	   } // end of tape code
	 */


	if (bkpinfo->backup_media_type == iso) {

/* Patch by Conor Daly <conor.daly@met.ie> 
 * 23-june-2004
 * Break up isodir into iso_mnt and iso_path
 * These will be used along with iso-dev at restore time
 * to locate the ISOs where ever they're mounted
 */

		log_it("isodir = %s", bkpinfo->isodir);
		sprintf(command, "df %s | tail -n1 | cut -d' ' -f1",
				bkpinfo->isodir);
		log_it("command = %s", command);
		log_it("res of it = %s",
			   call_program_and_get_last_line_of_output(command));
		sprintf(iso_dev, "%s",
				call_program_and_get_last_line_of_output(command));
		sprintf(tmp, "%s/ISO-DEV", bkpinfo->tmpdir);
		write_one_liner_data_file(tmp,
								  call_program_and_get_last_line_of_output
								  (command));

		sprintf(command, "mount | grep -w %s | tail -n1 | cut -d' ' -f3",
				iso_dev);
		log_it("command = %s", command);
		log_it("res of it = %s",
			   call_program_and_get_last_line_of_output(command));
		sprintf(iso_mnt, "%s",
				call_program_and_get_last_line_of_output(command));
		sprintf(tmp, "%s/ISO-MNT", bkpinfo->tmpdir);
		write_one_liner_data_file(tmp,
								  call_program_and_get_last_line_of_output
								  (command));
		log_it("isomnt: %s, %d", iso_mnt, strlen(iso_mnt));
		sprintf(iso_tmp, "%s", bkpinfo->isodir);
		if (strlen(iso_tmp) < strlen(iso_mnt)) {
			iso_path[0] = '\0';
		} else {
			sprintf(iso_path, "%s", iso_tmp + strlen(iso_mnt));
		}
		sprintf(tmp, "%s/ISODIR", bkpinfo->tmpdir);
		write_one_liner_data_file(tmp, iso_path);
		log_it("isodir: %s", iso_path);

/* End patch */
	}							// end of iso code

	if (bkpinfo->backup_media_type == nfs) {
		strcpy(hostname, bkpinfo->nfs_mount);
		colon = strchr(hostname, ':');
		if (!colon) {
			log_it("nfs mount doesn't have a colon in it");
			retval++;
		} else {
			struct hostent *hent;

			*colon = '\0';
			hent = gethostbyname(hostname);
			if (!hent) {
				log_it("Can't resolve NFS mount (%s): %s", hostname,
					   hstrerror(h_errno));
				retval++;
			} else {
				strcpy(ip_address, inet_ntoa((struct in_addr)
											 *((struct in_addr *) hent->
											   h_addr)));
				strcat(ip_address, strchr(bkpinfo->nfs_mount, ':'));
				strcpy(bkpinfo->nfs_mount, ip_address);
			}
		}
		store_nfs_config (bkpinfo);
	}

	log_it("Finished processing incoming params");
	if (retval) {
		fprintf(stderr, "Type 'man mondoarchive' for help.\n");
	}
	sprintf(tmp, "%s", MONDO_TMPISOS);	/* added #define 22 apr 2002 */
	if (does_file_exist(tmp)) {
		unlink(tmp);
	}
	if (strlen(bkpinfo->tmpdir) < 2 || strlen(bkpinfo->scratchdir) < 2) {
		log_it("tmpdir or scratchdir are blank/missing");
		retval++;
	}
	if (bkpinfo->include_paths[0] == '\0') {
		//      fatal_error ("Why no backup path?");
		strcpy(bkpinfo->include_paths, "/");
	}
	chmod(bkpinfo->scratchdir, 0700);
	chmod(bkpinfo->tmpdir, 0700);
	g_backup_media_type = bkpinfo->backup_media_type;
	paranoid_free(mtpt);
	paranoid_free(extra_cdrom_params);
	paranoid_free(mondo_mkisofs_sz);
	paranoid_free(command);
	paranoid_free(hostname);
	paranoid_free(ip_address);
	paranoid_free(cdr_exe);
	paranoid_free(tmp);
	paranoid_free(iso_dev);
	paranoid_free(iso_mnt);
	paranoid_free(iso_tmp);
	paranoid_free(iso_path);
	return (retval);
}



/**
 * Do some miscellaneous setup tasks to be performed before filling @c bkpinfo.
 * Seeds the random-number generator, loads important modules, checks the sanity
 * of the user's Linux distribution, and deletes logfile.
 * @param bkpinfo The backup information structure. Will be initialized.
 * @return number of errors (0 for success)
 */
int pre_param_configuration(struct s_bkpinfo *bkpinfo)
{
	int res = 0;

	make_hole_for_dir(MNT_CDROM);
	assert(bkpinfo != NULL);
	srandom((unsigned long) (time(NULL)));
	insmod_crucial_modules();
	reset_bkpinfo(bkpinfo);		// also sets defaults ('/'=backup path, 3=compression level)
	if (bkpinfo->disaster_recovery) {
		if (!does_nonMS_partition_exist()) {
			fatal_error
				("I am in disaster recovery mode\nPlease don't run mondoarchive.");
		}
	}

	unlink(MONDO_TRACEFILE);
	run_program_and_log_output("rm -Rf /tmp/changed.files*", FALSE);
	if (find_and_store_mondoarchives_home(g_mondo_home)) {
		fprintf(stderr,
				"Cannot find Mondo's homedir. I think you have >1 'mondo' directory on your hard disk. Please delete the superfluous 'mondo' directories and try again\n");
		res++;
		return (res);
	}
	res += some_basic_system_sanity_checks();
	if (res) {
		log_it("Your distribution did not pass Mondo's sanity test.");
	}
	g_current_media_number = 1;
	bkpinfo->postnuke_tarball[0] = bkpinfo->nfs_mount[0] = '\0';
	return (res);
}




/**
 * Reset all fields of the backup information structure to a sensible default.
 * @param bkpinfo The @c bkpinfo to reset.
 */
void reset_bkpinfo(struct s_bkpinfo *bkpinfo)
{
	int i;

	log_msg(1, "Hi");
	assert(bkpinfo != NULL);
	memset((void *) bkpinfo, 0, sizeof(struct s_bkpinfo));
	bkpinfo->manual_cd_tray = FALSE;
	bkpinfo->internal_tape_block_size = DEFAULT_INTERNAL_TAPE_BLOCK_SIZE;
	bkpinfo->media_device[0] = '\0';
	for (i = 0; i <= MAX_NOOF_MEDIA; i++) {
		bkpinfo->media_size[i] = -1;
	}
	bkpinfo->boot_loader = '\0';
	bkpinfo->boot_device[0] = '\0';
	bkpinfo->zip_exe[0] = '\0';
	bkpinfo->zip_suffix[0] = '\0';
	bkpinfo->restore_path[0] = '\0';
	bkpinfo->use_lzo = FALSE;
	bkpinfo->do_not_compress_these[0] = '\0';
	bkpinfo->verify_data = FALSE;
	bkpinfo->backup_data = FALSE;
	bkpinfo->restore_data = FALSE;
	bkpinfo->disaster_recovery =
		(am_I_in_disaster_recovery_mode()? TRUE : FALSE);
	if (bkpinfo->disaster_recovery) {
		strcpy(bkpinfo->isodir, "/");
	} else {
		strcpy(bkpinfo->isodir, "/root/images/mondo");
	}
	strcpy(bkpinfo->prefix, "mondorescue");

	bkpinfo->scratchdir[0] = '\0';
	bkpinfo->make_filelist = TRUE;	// unless -J supplied to mondoarchive
	sprintf(bkpinfo->tmpdir, "/tmp/tmpfs/mondo.tmp.%d", (int) (random() % 32768));	// for mondorestore
	bkpinfo->optimal_set_size = 0;
	bkpinfo->backup_media_type = none;
	strcpy(bkpinfo->include_paths, "/");
	bkpinfo->exclude_paths[0] = '\0';
	bkpinfo->call_before_iso[0] = '\0';
	bkpinfo->call_make_iso[0] = '\0';
	bkpinfo->call_burn_iso[0] = '\0';
	bkpinfo->call_after_iso[0] = '\0';
	bkpinfo->image_devs[0] = '\0';
	bkpinfo->postnuke_tarball[0] = '\0';
	bkpinfo->kernel_path[0] = '\0';
	bkpinfo->nfs_mount[0] = '\0';
	bkpinfo->nfs_remote_dir[0] = '\0';
	bkpinfo->wipe_media_first = FALSE;
	bkpinfo->differential = FALSE;
	bkpinfo->cdrw_speed = 0;
// patch by Herman Kuster  
	bkpinfo->differential = 0;
// patch end
	bkpinfo->compression_level = 3;
}




/**
 * Get the remaining free space (in MB) on @p partition.
 * @param partition The partition to check free space on (either a device or a mountpoint).
 * @return The free space on @p partition, in MB.
 */
long free_space_on_given_partition(char *partition)
{
	char command[MAX_STR_LEN], out_sz[MAX_STR_LEN];
	long res;

	assert_string_is_neither_NULL_nor_zerolength(partition);

	sprintf(command, "df -m %s &> /dev/null", partition);
	if (system(command)) {
		return (-1);
	}							// partition does not exist
	sprintf(command, "df -m %s | tail -n1 | tr -s ' ' '\t' | cut -f4",
			partition);
	strcpy(out_sz, call_program_and_get_last_line_of_output(command));
	if (strlen(out_sz) == 0) {
		return (-1);
	}							// error within df, probably
	res = atol(out_sz);
	return (res);
}



/**
 * Check the user's system for sanity. Checks performed:
 * - make sure user has enough RAM (32mb required, 64mb recommended)
 * - make sure user has enough free space in @c /
 * - check kernel for ramdisk support
 * - make sure afio, cdrecord, mkisofs, bzip2, awk, md5sum, strings, mindi, and buffer exist
 * - make sure CD-ROM is unmounted
 * - make sure /etc/modules.conf exists
 * - make sure user's mountlist is OK by running <tt>mindi --makemountlist</tt>
 *
 * @return number of problems with the user's setup (0 for success)
 */
int some_basic_system_sanity_checks()
{

	/*@ buffers ************ */
	char tmp[MAX_STR_LEN];
	//  char command[MAX_STR_LEN];

	/*@ int's *************** */
	int retval = 0;
	long Lres;


	mvaddstr_and_log_it(g_currentY, 0,
						"Checking sanity of your Linux distribution");
#ifndef __FreeBSD__
	if (system("which mkfs.vfat &> /dev/null")
		&& !system("which mkfs.msdos &> /dev/null")) {
		log_it
			("OK, you've got mkfs.msdos but not mkfs.vfat; time for the fairy to wave her magic wand...");
		run_program_and_log_output
			("ln -sf `which mkfs.msdos` /sbin/mkfs.vfat", FALSE);
	}
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("free | grep Mem | head -n1 | tr -s ' ' '\t' | cut -f2"));
	if (atol(tmp) < 35000) {
		retval++;
		log_to_screen("You must have at least 32MB of RAM to use Mondo.");
	}
	if (atol(tmp) < 66000) {
		log_to_screen
			("WARNING! You have very little RAM. Please upgrade to 64MB or more.");
	}
#endif

	if ((Lres = free_space_on_given_partition("/root")) == -1) {
		Lres = free_space_on_given_partition("/");
	}
	log_it("Free space on given partition = %ld MB", Lres);

	if (Lres < 50) {
		run_program_and_log_output
			("rm -Rf /root/images/mindi; mkdir -p /home/root/images/mindi; mkdir -p /root/images; ln -sf /home/root/images/mindi /root/images/mindi",
			 3);
		//      fatal_error("Your / (or /root) partition has <50MB free. Please adjust your partition table to something saner."); 
	}

	if (system("which " MKE2FS_OR_NEWFS " > /dev/null 2> /dev/null")) {
		retval++;
		log_to_screen
			("Unable to find " MKE2FS_OR_NEWFS " in system path.");
		fatal_error
			("Please use \"su -\", not \"su\" to become root. OK? ...and please don't e-mail the mailing list or me about this. Just read the message. :)");
	}
#ifndef __FreeBSD__
	if (run_program_and_log_output
		("cat /proc/devices | grep ramdisk", FALSE)) {
		if (!ask_me_yes_or_no
			("Your kernel has no ramdisk support. That's mind-numbingly stupid but I'll allow it if you're planning to use a failsafe kernel. Are you?"))
		{
			//          retval++;
			log_to_screen
				("It looks as if your kernel lacks ramdisk and initrd support.");
			log_to_screen
				("I'll allow you to proceed but FYI, if I'm right, your kernel is broken.");
		}
	}
#endif
	retval += whine_if_not_found(MKE2FS_OR_NEWFS);
	retval += whine_if_not_found("mkisofs");
	if (system("which dvdrecord > /dev/null 2> /dev/null")) {
		retval += whine_if_not_found("cdrecord");
	}
	retval += whine_if_not_found("bzip2");
	retval += whine_if_not_found("awk");
	retval += whine_if_not_found("md5sum");
	retval += whine_if_not_found("strings");
	retval += whine_if_not_found("mindi");
	retval += whine_if_not_found("buffer");

	// abort if Windows partition but no ms-sys and parted
	if (!run_program_and_log_output
		("mount | grep -w vfat | grep -v /dev/fd | grep -v nexdisk", 0)
		||
		!run_program_and_log_output
		("mount | grep -w dos | grep -v /dev/fd | grep -v nexdisk", 0)) {
		log_to_screen("I think you have a Windows 9x partition.");
		retval += whine_if_not_found("parted");
#ifndef __IA64__
		/* IA64 always has one vfat partition for EFI even without Windows */
		// retval += 
		if (!find_home_of_exe("ms-sys")) {
			log_to_screen("Please install ms-sys just in case.");
		}
#endif
	}

	if (!find_home_of_exe("cmp")) {
		if (!find_home_of_exe("true")) {
			whine_if_not_found("cmp");
		} else {
			log_to_screen
				("Your system lacks the 'cmp' binary. I'll create a dummy cmp for you.");
			if (run_program_and_log_output
				("cp -f `which true` /usr/bin/cmp", 0)) {
				fatal_error("Failed to create dummy 'cmp' file.");
			}
		}
	}
	run_program_and_log_output
		("umount `mount | grep cdr | cut -d' ' -f3 | tr '\n' ' '`", 5);
	strcpy(tmp,
		   call_program_and_get_last_line_of_output
		   ("mount | grep -E \"cdr(om|w)\""));
	if (strcmp("", tmp)) {
		if (strstr(tmp, "autofs")) {
			log_to_screen
				("Your CD-ROM is mounted via autofs. I therefore cannot tell");
			log_to_screen
				("if a CD actually is inserted. If a CD is inserted, please");
			log_to_screen("eject it. Thank you.");
			log_it
				("Ignoring autofs CD-ROM 'mount' since we hope nothing's in it.");
		} else
			if (run_program_and_log_output("uname -a | grep Knoppix", 5)) {
			retval++;
			fatal_error
				("Your CD-ROM drive is mounted. Please unmount it.");
		}
	}
#ifndef __FreeBSD__
	if (!does_file_exist("/etc/modules.conf")) {
		if (does_file_exist("/etc/conf.modules")) {
			log_it("Linking /etc/modules.conf to /etc/conf.modules");
			run_program_and_log_output
				("ln -sf /etc/conf.modules /etc/modules.conf", 5);
		} else if (does_file_exist("/etc/modprobe.d")) {
			log_it
				("Directory /etc/modprobe.d found. mindi will use its contents.");
		} else if (does_file_exist("/etc/modprobe.conf")) {
			log_it("Linking /etc/modules.conf to /etc/modprobe.conf");
			run_program_and_log_output
				("ln -sf /etc/modprobe.conf /etc/modules.conf", 5);
		} else {
			retval++;
			log_to_screen
				("Please find out what happened to /etc/modules.conf");
		}
	}
#endif

	run_program_and_log_output("cat /etc/fstab", 5);
#ifdef __FreeBSD__
	run_program_and_log_output("vinum printconfig", 5);
#else
	run_program_and_log_output("cat /etc/raidtab", 5);
#endif

	if (run_program_and_log_output("mindi -V", 1)) {
		log_to_screen("Could not ascertain mindi's version number.");
		log_to_screen
			("You have not installed Mondo and/or Mindi properly.");
		log_to_screen("Please uninstall and reinstall them both.");
		fatal_error("Please reinstall Mondo and Mindi.");
	}
	if (run_program_and_log_output
		("mindi --makemountlist /tmp/mountlist.txt.test", 5)) {
		log_to_screen
			("Mindi --makemountlist /tmp/mountlist.txt.test failed for some reason.");
		log_to_screen
			("Please run that command by hand and examine /var/log/mindi.log");
		log_to_screen
			("for more information. Perhaps your /etc/fstab file is insane.");
		log_to_screen
			("Perhaps Mindi's MakeMountlist() subroutine has a bug. We'll see.");
		retval++;
	}

	if (!run_program_and_log_output("fdisk -l | grep -i raid", 1)
		&& !does_file_exist("/etc/raidtab")) {
		log_to_screen
			("You have RAID partitions but no /etc/raidtab - creating one from /proc/mdstat");
		create_raidtab_from_mdstat("/etc/raidtab", "/proc/mdstat");
	}

	if (retval) {
		mvaddstr_and_log_it(g_currentY++, 74, "Failed.");
	} else {
		mvaddstr_and_log_it(g_currentY++, 74, "Done.");
	}
	return (retval);
}

/**
 * Retrieve the line containing @p label from the config file.
 * @param config_file The file to read from, usually @c /tmp/mondo-restore.cfg.
 * @param label What to read from the file.
 * @param value Where to put it.
 * @return 0 for success, 1 for failure.
 */
int read_cfg_var(char *config_file, char *label, char *value)
{
	/*@ buffer ****************************************************** */
	char command[MAX_STR_LEN * 2];
	char tmp[MAX_STR_LEN];

	/*@ end vars *************************************************** */

	assert_string_is_neither_NULL_nor_zerolength(config_file);
	assert_string_is_neither_NULL_nor_zerolength(label);
	if (!does_file_exist(config_file)) {
		sprintf(tmp, "(read_cfg_var) Cannot find %s config file",
				config_file);
		log_to_screen(tmp);
		value[0] = '\0';
		return (1);
	} else if (strstr(value, "/dev/") && strstr(value, "t0")
			   && !strcmp(label, "media-dev")) {
		log_msg(2, "FYI, I shan't read new value for %s - already got %s",
				label, value);
		return (0);
	} else {
		sprintf(command, "cat %s | grep \"%s .*\" | cut -d' ' -f2,3,4,5",
				config_file, label);
		strcpy(value, call_program_and_get_last_line_of_output(command));
		if (strlen(value) == 0) {
			return (1);
		} else {
			return (0);
		}
	}
}



/**
 * Remount @c supermount if it was unmounted earlier.
 */
void remount_supermounts_if_necessary()
{
	if (g_remount_cdrom_at_end) {
		run_program_and_log_output("mount " MNT_CDROM, FALSE);
	}
	if (g_remount_floppy_at_end) {
		run_program_and_log_output("mount " MNT_FLOPPY, FALSE);
	}
}

/**
 * Unmount @c supermount if it's mounted.
 */
void unmount_supermounts_if_necessary()
{
	if (run_program_and_log_output
		("mount | grep cdrom | grep super", FALSE) == 0) {
		g_remount_cdrom_at_end = TRUE;
		run_program_and_log_output("umount " MNT_CDROM, FALSE);
	}
	if (run_program_and_log_output
		("mount | grep floppy | grep super", FALSE) == 0) {
		g_remount_floppy_at_end = TRUE;
		run_program_and_log_output("umount " MNT_FLOPPY, FALSE);
	}
}

/**
 * Whether we had to stop autofs (if so, restart it at end).
 */
bool g_autofs_stopped = FALSE;

/**
 * Path to the autofs initscript ("" if none exists).
 */
char g_autofs_exe[MAX_STR_LEN];

/**
 * Autofs initscript in Xandros Linux distribution.
 */
#define XANDROS_AUTOFS_FNAME "/etc/init.d/xandros-autofs"

/**
 * Autofs initscript in most Linux distributions.
 */
#define STOCK_AUTOFS_FNAME "/etc/rc.d/init.d/autofs"

/**
 * If autofs is mounted, stop it (restart at end).
 */
void stop_autofs_if_necessary()
{
	char tmp[MAX_STR_LEN];

	g_autofs_exe[0] = '\0';
	if (does_file_exist(XANDROS_AUTOFS_FNAME)) {
		strcpy(g_autofs_exe, XANDROS_AUTOFS_FNAME);
	} else if (does_file_exist(STOCK_AUTOFS_FNAME)) {
		strcpy(g_autofs_exe, STOCK_AUTOFS_FNAME);
	}

	if (!g_autofs_exe[0]) {
		log_msg(3, "No autofs detected.");
	} else {
		log_msg(3, "%s --- autofs detected", g_autofs_exe);
// FIXME -- only disable it if it's running ---  sprintf(tmp, "%s status", autofs_exe);
		sprintf(tmp, "%s stop", g_autofs_exe);
		if (run_program_and_log_output(tmp, 2)) {
			log_it("Failed to stop autofs - I assume it wasn't running");
		} else {
			g_autofs_stopped = TRUE;
			log_it("Stopped autofs OK");
		}
	}
}

/**
 * If autofs was stopped earlier, restart it.
 */
void restart_autofs_if_necessary()
{
	char tmp[MAX_STR_LEN];

	if (!g_autofs_stopped || !g_autofs_exe[0]) {
		log_msg(3, "No autofs detected.");
		return;
	}
	sprintf(tmp, "%s start", g_autofs_exe);
	if (run_program_and_log_output(tmp, 2)) {
		log_it("Failed to start autofs");
	} else {
		g_autofs_stopped = FALSE;
		log_it("Started autofs OK");
	}
}


/**
 * If this is a distribution like Gentoo that doesn't keep /boot mounted, mount it.
 */
void mount_boot_if_necessary()
{
	char tmp[MAX_STR_LEN];
	char command[MAX_STR_LEN];

	log_msg(1, "Started sub");
	log_msg(4, "About to set g_boot_mountpt[0] to '\\0'");
	g_boot_mountpt[0] = '\0';
	log_msg(4, "Done. Great. Seeting command to something");
	strcpy(command,
		   "cat /etc/fstab | grep -v \":\" | grep -vx \"#.*\" | grep -w \"/boot\" | tr -s ' ' '\t' | cut -f1 | head -n1");
	log_msg(4, "Cool. Command = '%s'", command);
	strcpy(tmp, call_program_and_get_last_line_of_output(command));
	log_msg(4, "tmp = '%s'", tmp);
	if (tmp[0]) {
		log_it("/boot is at %s according to /etc/fstab", tmp);
		if (strstr(tmp, "LABEL=")) {
			if (!run_program_and_log_output("mount /boot", 5)) {
				strcpy(g_boot_mountpt, "/boot");
				log_msg(1, "Mounted /boot");
			} else {
				log_it("...ignored cos it's a label :-)");
			}
		} else {
			sprintf(command, "mount | grep -w \"%s\"", tmp);
			log_msg(3, "command = %s", command);
			if (run_program_and_log_output(command, 5)) {
				strcpy(g_boot_mountpt, tmp);
				sprintf(tmp,
						"%s (your /boot partition) is not mounted. I'll mount it before backing up",
						g_boot_mountpt);
				log_it(tmp);
				sprintf(tmp, "mount %s", g_boot_mountpt);
				if (run_program_and_log_output(tmp, 5)) {
					g_boot_mountpt[0] = '\0';
					log_msg(1, "Plan B");
					if (!run_program_and_log_output("mount /boot", 5)) {
						strcpy(g_boot_mountpt, "/boot");
						log_msg(1, "Plan B worked");
					} else {
						log_msg(1,
								"Plan B failed. Unable to mount /boot for backup purposes. This probably means /boot is mounted already, or doesn't have its own partition.");
					}
				}
			}
		}
	}
	log_msg(1, "Ended sub");
}


/**
 * If we mounted /boot earlier, unmount it.
 */
void unmount_boot_if_necessary()
{
	char tmp[MAX_STR_LEN];

	log_msg(3, "starting");
	if (g_boot_mountpt[0]) {
		sprintf(tmp, "umount %s", g_boot_mountpt);
		if (run_program_and_log_output(tmp, 5)) {
			log_it("WARNING - unable to unmount /boot");
		}
	}
	log_msg(3, "leaving");
}



/**
 * Write a line to a configuration file. Writes a line of the form,
 * @c label @c value.
 * @param config_file The file to write to. Usually @c mondo-restore.cfg.
 * @param label What to call this bit of data you're writing.
 * @param value The bit of data you're writing.
 * @return 0 for success, 1 for failure.
 */
int write_cfg_var(char *config_file, char *label, char *value)
{
	/*@ buffers ***************************************************** */
	char command[MAX_STR_LEN * 2];
	char tempfile[MAX_STR_LEN];
	char tmp[MAX_STR_LEN];


	/*@ end vars *************************************************** */
	assert_string_is_neither_NULL_nor_zerolength(config_file);
	assert_string_is_neither_NULL_nor_zerolength(label);
	assert(value != NULL);
	if (!does_file_exist(config_file)) {
		sprintf(tmp, "(write_cfg_file) Cannot find %s config file",
				config_file);
		log_to_screen(tmp);
		return (1);
	}
	strcpy(tempfile,
		   call_program_and_get_last_line_of_output
		   ("mktemp -q /tmp/mojo-jojo.blah.XXXXXX"));
	if (does_file_exist(config_file)) {
		sprintf(command, "cat %s | grep -vx \"%s .*\" > %s", config_file,
				label, tempfile);
		paranoid_system(command);
	}
	sprintf(command, "echo \"%s %s\" >> %s", label, value, tempfile);
	paranoid_system(command);
	sprintf(command, "mv -f %s %s", tempfile, config_file);
	paranoid_system(command);
	unlink(tempfile);
	return (0);
}


/**
 * The standard log_debug_msg() (log_msg() also due to a macro). Writes some describing
 * information to the logfile.
 */
void standard_log_debug_msg(int debug_level, const char *szFile,
							const char *szFunction, int nLine,
							const char *fmt, ...)
{
	va_list args;
	int i;
	static int depth = 0;
	char *tmp;
	FILE *fout;

	if (depth > 5) {
		depth--;
		return;
	}
	depth++;

	malloc_string(tmp);

	if (debug_level <= g_loglevel) {
		va_start(args, fmt);
		if (!(fout = fopen(MONDO_LOGFILE, "a"))) {
			return;
		}						// fatal_error("Failed to openout to logfile - sheesh..."); }

		// add tabs to distinguish log levels
		if (debug_level > 0) {
			for (i = 1; i < debug_level; i++)
				fprintf(fout, "\t");
			if (getpid() == g_main_pid)
				fprintf(fout, "[Main] %s->%s#%d: ", szFile, szFunction,
						nLine);
			else if (getpid() == g_buffer_pid && g_buffer_pid > 0)
				fprintf(fout, "[Buff] %s->%s#%d: ", szFile, szFunction,
						nLine);
			else
				fprintf(fout, "[TH=%d] %s->%s#%d: ", getpid(), szFile,
						szFunction, nLine);
		}
		vfprintf(fout, fmt, args);

		// do not slow down the progran if standard debug level
		// must be enabled: if no flush, the log won't be up-to-date if there
		// is a segfault
		//if (g_dwDebugLevel != 1)

		va_end(args);
		fprintf(fout, "\n");
		paranoid_fclose(fout);
	}
	depth--;
	paranoid_free(tmp);
}

/**
 * Function pointer to the @c log_debug_msg function to use. Points to standard_log_debug_msg() by default.
 */
void (*log_debug_msg) (int, const char *, const char *, int, const char *,
					   ...) = standard_log_debug_msg;


/**
 * If @p y, malloc @p x, else free @p x.
 * @bug This function seems orphaned. Please remove.
 */
#define do_alloc_or_free_depending(x,y) { if(y) {x=malloc(MAX_STR_LEN);} else {paranoid_free(x);} }

/**
 * Allocate or free important globals, depending on @p mal.
 * @param mal If TRUE, malloc; if FALSE, free.
 */
void do_libmondo_global_strings_thing(int mal)
{
	if (mal) {
		iamhere("Malloc'ing globals");
		malloc_string(g_boot_mountpt);
		malloc_string(g_mondo_home);
		malloc_string(g_tmpfs_mountpt);
		malloc_string(g_erase_tmpdir_and_scratchdir);
		malloc_string(g_serial_string);
		malloc_string(g_magicdev_command);
	} else {
		iamhere("Freeing globals");
		paranoid_free(g_boot_mountpt);
		paranoid_free(g_mondo_home);
		paranoid_free(g_tmpfs_mountpt);
		paranoid_free(g_erase_tmpdir_and_scratchdir);
		paranoid_free(g_serial_string);
		paranoid_free(g_magicdev_command);
	}

	/*
	   char**list_of_arrays[] = {
	   &g_boot_mountpt,
	   &g_mondo_home,
	   &g_tmpfs_mountpt,
	   &g_erase_tmpdir_and_scratchdir,
	   &g_serial_string,
	   &g_magicdev_command,
	   NULL};

	   char**ppcurr;
	   int i;

	   for(i=0;list_of_arrays[i];i++)
	   {
	   log_msg(5, "Allocating %d", i);
	   ppcurr = list_of_arrays[i];
	   if (mal)
	   { *ppcurr = malloc(MAX_STR_LEN); }
	   else
	   {
	   if (*ppcurr)
	   {
	   free(*ppcurr);
	   }
	   }
	   }
	   log_msg(5, "Returning");
	 */
}

/**
 * Allocate important globals.
 * @see do_libmondo_global_strings_thing
 */
void malloc_libmondo_global_strings(void)
{
	do_libmondo_global_strings_thing(1);
}

/**
 * Free important globals.
 * @see do_libmondo_global_strings_thing
 */
void free_libmondo_global_strings(void)
{
	do_libmondo_global_strings_thing(0);
}



/**
 * Stop @c magicdev if it's running.
 * The command used to start it is saved in @p g_magicdev_command.
 */
void stop_magicdev_if_necessary()
{
	strcpy(g_magicdev_command,
		   call_program_and_get_last_line_of_output
		   ("ps ax | grep -w magicdev | grep -v grep | tr -s '\t' ' '| cut -d' ' -f6-99"));
	if (g_magicdev_command[0]) {
		log_msg(1, "g_magicdev_command = '%s'", g_magicdev_command);
		paranoid_system("killall magicdev");
	}
}


/**
 * Restart magicdev if it was stopped.
 */
void restart_magicdev_if_necessary()
{
	char *tmp;

	malloc_string(tmp);
	if (g_magicdev_command && g_magicdev_command[0]) {
		sprintf(tmp, "%s &", g_magicdev_command);
		paranoid_system(tmp);
	}
	paranoid_free(tmp);
}

/* @} - end of utilityGroup */
