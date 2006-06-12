/*
 * Common defines across the project
 *
 *  $Id$
 */

#ifndef _MY_STUFF_H_
#define _MY_STUFF_H_

/* BERLIOS
#define HAVE_MALLOC 1
*/

// Extra info for ACLs and SELINUX users
#define STAR_ACL_SZ "-xfflags -acl"
//#define STAR_ACL_SZ "-xfflags"
//#define STAR_ACL_SZ ""
// Enable the first line and disable the second if you are a Fedora Core 2 user

/**
 * @file
 * The main header file for Mondo.
 */

/* Required for the use of getline, ... */
#define _GNU_SOURCE

#include <stdio.h>

#if !defined(bool) && !defined(__cplusplus)
/**
 * Create the illusion of a Boolean type.
 */
#define bool unsigned char
#define TRUE 1
#define FALSE 0
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/* BERLIOS
 * Useful ?
*/

#ifndef __FreeBSD__
#include <getopt.h>
#endif

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <sys/sem.h>
#include <sys/param.h>

#include <stdlib.h>
/* BERLIOS
#ifndef  __USE_FILE_OFFSET64
#define  __USE_FILE_OFFSET64
#endif
#ifndef  __USE_LARGEFILE64
#define  __USE_LARGEFILE64
#endif
*/
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifndef S_SPLINT_S
#include <signal.h>
#endif
#include <newt.h>
#include <ctype.h>
#include <string.h>
#ifndef S_SPLINT_S
#include <pthread.h>
#endif
#include <assert.h>

/*
#if defined(DEBUG) && !__cplusplus
int count;
char trace_log[255];
char *trace_log_ptr;
#endif
*/

#define STD_PREFIX "mondorescue"

/**
 * The biggielist stub (appended to the directory where all.tar.gz was unpacked).
 */
#define BIGGIELIST_TXT_STUB "tmp/biggielist.txt"

/**
 * The filelist stub (appended to the directory where all.tar.gz was unpacked).
 */
#define FILELIST_FULL_STUB "tmp/filelist.full.gz"

/**
 * The mountlist stub (appended to the directory where all.tar.gz was unpacked).
 */
#define MOUNTLIST_FNAME_STUB "tmp/mountlist.txt"

/**
 * The mondo-restore.cfg stub (appended to the directory where all.tar.gz was unpacked).
 */
#define MONDO_CFG_FILE_STUB "tmp/mondo-restore.cfg"

/**
 * The RAID kernel proc file
 */
#define MDSTAT_FILE "/proc/mdstat"

/**
 * @bug Apparently unused.
 */
#define MONDO_TRACEFILE	"/var/log/mondo-tracefile.log"

#undef assert

extern void _mondo_assert_fail(const char *file, const char *function,
							   int line, const char *exp);

/**
 * An assert macro that calls _mondo_assert_fail() when it fails.
 */
#ifdef NDEBUG
#	define assert(exp) ((void)0)
#else
#	ifndef S_SPLINT_S
#		define assert(exp) ((exp)?((void)0):_mondo_assert_fail(__FILE__, __FUNCTION__, __LINE__, #exp))
#	else
#		define assert(exp) ((void)0)
#	endif
#endif

#define CRC_M16	0xA001			///< Mask for crc16.
#define	CRC_MTT	0x1021			///< Mask for crc-ccitt.

#define SCREEN_LENGTH 25		///< The default size of the screen.
#define NOOF_ERR_LINES 6		///< The number of lines of log output to keep at the bottom of the screen.
#define ARBITRARY_MAXIMUM 2000	///< The maximum number of items showing at once in the mountlist or filelist editor.
#define MAX_TAPECATALOG_ENTRIES 4096	///< The maximum number of entries in the tape catalog.
#define MAX_STR_LEN 380			///< The maximum length of almost all @p char buffers in Mondo.
#define MAXIMUM_RAID_DEVS 32	///< The maximum number of RAID devices in the raidlist.
#define MAXIMUM_ADDITIONAL_RAID_VARS 32	///< The maximum number of additional RAID variables per RAID device in the raidlist.
#define MAXIMUM_DISKS_PER_RAID_DEV 32	///< The maximum number of disks per RAID device in the raidtab.

#define RAIDTAB_FNAME "/etc/raidtab"	///< The filename of the raidtab file, at least on Linux.

#define BLK_START_OF_BACKUP     1	///< Marker block: start a backup.
#define BLK_START_OF_TAPE       2	///< Marker block: start a tape.
#define BLK_START_AFIOBALLS	10	///< Marker block: start the afioball section.
#define BLK_STOP_AFIOBALLS	19	///< Marker block: stop the afioball section.
#define BLK_START_AN_AFIO_OR_SLICE    	20	///< Marker block: start an afioball or a slice.
#define BLK_STOP_AN_AFIO_OR_SLICE	29	///< Marker block: stop an afioball or a slice.
#define BLK_START_BIGGIEFILES	30	///< Marker block: start the biggiefile section.
#define BLK_STOP_BIGGIEFILES	39	///< Marker block: stop the biggiefile section.
#define BLK_START_A_NORMBIGGIE	40	///< Marker block: start a normal biggiefile.
#define BLK_START_A_PIHBIGGIE	41	///< Marker block: start a ntfsprog'd biggiefile
#define BLK_START_EXTENDED_ATTRIBUTES 45	///< Marker block: start xattr/acl info
#define BLK_STOP_EXTENDED_ATTRIBUTES 46	///< Marker block: stop xattr/acl info
#define BLK_START_EXAT_FILE     47
#define BLK_STOP_EXAT_FILE      48
#define BLK_STOP_A_BIGGIE	59	///< Marker block: stop a biggiefile.
#define BLK_START_FILE          80	///< Marker block: start a file (non-afio or slice).
#define BLK_STOP_FILE           89	///< Marker block: stop a file (non-afio or slice).
#define BLK_END_OF_TAPE         100	///< Marker block: end of tape.
#define BLK_END_OF_BACKUP       101	///< Marker block: end of backup.
#define BLK_ABORTED_BACKUP      102	///< Marker block: backup was aborted.

/// The external tape blocksize.
#ifdef EXTTAPE
#define TAPE_BLOCK_SIZE (long)EXTTAPE
#else
#define TAPE_BLOCK_SIZE 131072L	/* was 8192; 06/2002-->65536; 11/2002-->131072 */
#endif

#define DEFAULT_INTERNAL_TAPE_BLOCK_SIZE 32768	// Nov 2003?




#define SLICE_SIZE 4096			///< The size of a slice of a biggiefile.






/**
 * Determine whether @p x (t_bkptype) is a streaming backup.
 */
#define IS_THIS_A_STREAMING_BACKUP(x) (x == tape || x == udev || x == cdstream)


/**
 * @c mkisofs command to generate a nonbootable CD, except for -o option and the directory to image.
 */
#define MONDO_MKISOFS_NONBOOT	"mkisofs -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL_Version -V _CD#_"

/**
 * @c mkisofs command to generate a bootable CD using isolinux, except for -o option and the directory to image.
 */
#define MONDO_MKISOFS_REGULAR_SYSLINUX	"mkisofs -J -boot-info-table -no-emul-boot -b isolinux.bin -c boot.cat -boot-load-size 4 -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL_Version -V _CD#_"
#define MONDO_MKISOFS_REGULAR_LILO      "mkisofs -J -boot-info-table -no-emul-boot -b isolinux.bin -c boot.cat -boot-load-size 4 -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_"
#define MONDO_MKISOFS_REGULAR_ELILO     "mkisofs -no-emul-boot -b images/mindi-bootroot.8192.img -c boot.cat -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_"

/**
 * @c mkisofs command to generate a bootable CD using LILO, except for -o option and the directory to image.
 */
// -b images/mindi-boot.2880.img

/**
 * @c mkisofs command to generate a bootable CD using ELILO, except for -o option and the directory to image.
 */
// -b images/mindi-boot.2880.img
// Should replace 8192 by IA64_BOOT_SIZE

/**
 * The stub name of the temporary ISO image to create, burn, and remove.
 */
#define MONDO_TMPISOS "/temporary.iso"

/**
 * @c growisofs command to generate a bootable DVD using isolinux, except for the directory to image.
 */
#define MONDO_GROWISOFS_REGULAR_SYSLINUX "growisofs -use-the-force-luke -J -no-emul-boot -boot-load-size 4 -b isolinux.bin --boot-info-table -c boot.cat -boot-load-size 4 -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL_Version -V _CD#_ -v"

/**
 * @c growisofs command to generate a bootable DVD using LILO, except for the directory to image.
	 */// -b images/mindi-boot.2880.img
#define MONDO_GROWISOFS_REGULAR_ELILO     "growisofs -use-the-force-luke -no-emul-boot -b images/mindi-boot.2880.img -c boot.cat -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ -v"

/**
 * @c growisofs command to generate a bootable DVD using LILO, except for the directory to image.
	 */// -b images/mindi-boot.2880.img
#define MONDO_GROWISOFS_REGULAR_LILO     "growisofs -no-emul-boot -b isolinux.bin -c boot.cat -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ -v"

/**
 * @c growisofs command to generate a nonbootable DVD, except for the directory to image.
 */
#define MONDO_GROWISOFS_NONBOOT          "growisofs -use-the-force-luke -J -r -p MondoRescue -publisher www.mondorescue.org -A Mondo_Rescue_GPL -V _CD#_ -v"

/**
 * Welcome string displayed at the top of the newt interface.
 */
#define WELCOME_STRING _("W E L C O M E   T O   M O N D O   R E S C U E")

/**
 * The maximum length of a filename in the tape catalog.
 */
#define MAX_TAPECAT_FNAME_LEN 32

/**
 * Compatibility #define to ease the transition to logfile-in-a-variable.
 */
#define MONDO_LOGFILE    "/var/log/mondo-archive.log"

/**
 * Assert that (@p x != NULL) and (@p x[0] != '\\0').
 */
#define assert_string_is_neither_NULL_nor_zerolength(x) {assert(x!=NULL);assert(x[0]!='\0');}

/**
 * Log the file, line, Mondo error message, and OS error message (errno).
 */
#define log_OS_error(x) {log_msg(0, "%s, line %ld: %s (%s)", __FILE__, __LINE__, x, strerror(errno));}

/**
 * Assert that (@p x != NULL).
 */
#define assert_pointer_is_not_NULL(x) {assert(x!=NULL);}

/**
 * close() @p x and log a message if it fails.
 */
#define paranoid_close(x) {if(close(x)) {log_msg(5, "close err");} x=-999; }

/**
 * fclose() @p x and log a message if it fails.
 */
#define paranoid_fclose(x) {if(fclose(x)) {log_msg(5, "fclose err");} x=NULL; }

/**
 * pclose() @p x and log a message if it fails.
 */
#define paranoid_pclose(x) {if(pclose(x)) {log_msg(5, "pclose err");} x=NULL; }

/**
 * Run the command @p x and log it if it fails.
 */
#define paranoid_system(x) {if(system(x)) log_msg(4, x); }

/**
 * Free @p x and set it to NULL.
 */
#define paranoid_free(x) {if ((x) != NULL) free(x); (x)=NULL;}

/**
 * Free variables and call finish(@p x).
 */
#define paranoid_MR_finish(x) {free_MR_global_filenames (); if (g_bkpinfo_DONTUSETHIS) paranoid_free ( g_bkpinfo_DONTUSETHIS ); finish(x); }

/**
 * Log file, function, line, and @p x.
 */
#define iamhere(x) {log_it("%s, %s, %ld: %s", __FILE__, __FUNCTION__, __LINE__, x);}

/**
 * Yes, we want malloc() to help us fix bugs.
 */
#define MALLOC_CHECK_ 1

/**
 * Kill any process containing the string @p x surrounded by spaces in its commandline.
 */
#define kill_anything_like_this(x) {run_program_and_log_output("kill `ps wax | grep \"" x "\" | awk '{print $1;}' | grep -vx \"\\?\"`", TRUE);}

/**
 * Malloc @p x to be MAX_STR_LEN bytes and call fatal_error() if we're out of memory.
 */
#define malloc_string(x) { x = (char *)malloc(MAX_STR_LEN); if (!x) { fatal_error("Unable to malloc"); } x[0] = x[1] = '\0'; }

/**
 * Path to the location the hard drive is mounted on during a restore.
 */
#define MNT_RESTORING "/mnt/RESTORING"

/** @def VANILLA_SCSI_CDROM The first SCSI CD-ROM in the system (most likely to be the one to write to). */
/** @def VANILLA_SCSI_TAPE  The first SCSI tape in the system (most likely to be the one towrite to. */
/** @def DONT_KNOW_HOW_TO_EVALUATE_THIS_DEVICE_TYPE A string whose presence in a device name indicates the
 * inability to check this device for errors in the mountlist. */
/** @def RAID_DEVICE_STUB The stub of a RAID device (set up RAID if we find it). */
/** @def SANE_FORMATS Sane formats for this OS, separated by spaces. */
/** @def ALT_TAPE The first IDE tape in the system. */
/** @def MKE2FS_OR_NEWFS @c mke2fs or @c newfs, depending on the OS. */
/** @def CP_BIN The GNU @c cp binary to use. */
#ifdef __FreeBSD__
#define VANILLA_SCSI_CDROM	"/dev/cd0"
#define VANILLA_SCSI_TAPE	"/dev/sa0"
#define DONT_KNOW_HOW_TO_EVALUATE_THIS_DEVICE_TYPE	"/dev/vinum/"
#define RAID_DEVICE_STUB	DONT_KNOW_HOW_TO_EVALUATE_THIS_DEVICE_TYPE
#define SANE_FORMATS		"swap image msdosfs nfs ntfs raid lvm ffs ufs ext2fs"
#define ALT_TAPE		"/dev/ast0"
#define MKE2FS_OR_NEWFS	"newfs"
#define CP_BIN		"gcp"
#else
#define VANILLA_SCSI_CDROM	"/dev/scd0"
#define VANILLA_SCSI_TAPE	"/dev/st0"
#define DONT_KNOW_HOW_TO_EVALUATE_THIS_DEVICE_TYPE	"/dev/md"
#define RAID_DEVICE_STUB	DONT_KNOW_HOW_TO_EVALUATE_THIS_DEVICE_TYPE
#define SANE_FORMATS		"swap image vfat ext2 ext3 xfs vfs jfs reiserfs dos minix coda nfs ntfs hpfs raid lvm cifs"
#define ALT_TAPE		"/dev/ht0"
#define MKE2FS_OR_NEWFS	"mke2fs"
#define CP_BIN		"cp"
#endif


/**
 * The template for a filelist filename.
 * The first argument (%s) is the tempdir and the second (%d) is the filelist number.
 */
#define FILELIST_FNAME_RAW_SZ "%s/filelist.%ld"

#define XATTR_LIST_FNAME_RAW_SZ      "%s/xattr_list.%ld.gz"
#define XATTR_BIGGLST_FNAME_RAW_SZ   "%s/xattr_list.big.gz"
#define ACL_LIST_FNAME_RAW_SZ        "%s/acl_list.%ld.gz"
#define ACL_BIGGLST_FNAME_RAW_SZ     "%s/acl_list.big.gz"

/**
 * The template for an afioball filename.
 * The first argument (%s) is the tempdir and the second (%d) is the filelist number.
 */
#define AFIOBALL_FNAME_RAW_SZ (bkpinfo->use_star)?"%s/tmpfs/%ld.star.%s":"%s/tmpfs/%ld.afio.%s"
#define ARCH_THREADS 2			///< The number of simultaneous threads running afio in the background.
#define ARCH_BUFFER_NUM (ARCH_THREADS*4)	// Number of permissible queued afio files
#define FORTY_SPACES "                                         "	///< 40 spaces.
#define PPCFG_RAMDISK_SIZE 250	///< Size of the tmpfs, in megabytes, to attempt to mount (to speed up Mondo).

#define DO_MBR_PLEASE "/tmp/DO-MBR-PLEASE"


/**
 * Compatibility define to change log_it() calls to log_debug_msg() calls.
 */
#define log_it(format, args...) log_debug_msg(2, __FILE__, __FUNCTION__, __LINE__, format, ## args)

/**
 * Macro to log a message along with file, line, and function information.
 */
#define log_msg(level, format, args...) log_debug_msg(level, __FILE__, __FUNCTION__, __LINE__, format, ## args)

#define DEFAULT_DVD_DISK_SIZE 4380	///< The default size (in MB) of a DVD disk, unless the user says otherwise.

#define DEFAULT_DEBUG_LEVEL 4	///< By default, don't log messages with a loglevel higher than this.

#define SZ_NTFSPROG_VOLSIZE "1048576"	// was 4096
#define NTFSPROG_PARAMS "-z0 -V" SZ_NTFSPROG_VOLSIZE " -o -b -d -g1"

#define MNT_CDROM "/mnt/cdrom"
#define MNT_FLOPPY "/mnt/floppy"

#define DEFAULT_MR_LOGLEVEL 4

#ifdef ENABLE_NLS  
# include <libintl.h>  
# undef _  
# define _(String) dgettext (PACKAGE, String)
# ifdef gettext_noop  
#  define N_(String) gettext_noop (String)  
# else  
#  define N_(String) (String)  
# endif  
#else  
# define textdomain(String) (String)  
# define gettext(String) (String)  
# define dgettext(Domain,Message) (Message)  
# define dcgettext(Domain,Message,Type) (Message)  
# define bindtextdomain(Domain,Directory) (Domain)  
# define _(String) (String)  
# define N_(String) (String)  

#endif 


#endif							/* _MY_STUFF_H_ */
