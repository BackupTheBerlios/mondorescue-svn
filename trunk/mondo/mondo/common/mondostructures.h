/***************************************************************************
                          mondostructures.h  -  description
                             -------------------
    begin                : Fri Apr 19 2002
    copyright            : (C) 2002 by Stan Benoit
    email                : troff@nakedsoul.org
    cvsid                : $Id: mondostructures.h,v 1.3 2004/06/17 08:49:06 hugo Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/**
 * @file
 * The header file defining all of Mondo's structures.
 */


/** @def MAX_NOOF_MEDIA The maximum number of media that can be used in any one backup. */

///* So we can override it in config.h: */
//#ifndef MAX_NOOF_MEDIA
#define MAX_NOOF_MEDIA 50
//#endif

/**
 * Structure indicating one entry in the mountlist.
 * There is one mountlist_line for each device we're keeping track of in the mountlist.
 */
struct mountlist_line
{
  /**
   * The name of the device (/dev entry) for this mountlist line. Guaranteed to be unique.
   */
  char device[64];

  /**
   * The mountpoint for this mountlist line. Should be unique.
   * This can be "raid", for a RAID subdisk, or "lvm", for an LVM PV.
   */
  char mountpoint[256];

  /**
   * The filesystem type of this entry. Examples: ext2, ext3, reiserfs, xfs, swap.
   * Also, this can be "raid", for a RAID subdisk, or "lvm", for an LVM PV.
   */
  char format[64];

  /**
   * The size in kilobytes of this device. 0 or -1 indicates LVM.
   */
  long long size;

  /**
   * For ext2 and ext3, this is the filesystem label (if there is one). If not, this should be "".
   */
  char label[256];
};

/**
 * The mountlist structure.
 * This is used to keep track of a list of all the devices/partitions/formats/sizes/labels in the
 * system, so we can recreate them in a nuke restore.
 */
struct mountlist_itself
{
  /**
   * Number of entries in the mountlist.
   */
  int entries;

  /**
   * The list of entries, all @p entries of them.
   */
  struct mountlist_line el[MAX_TAPECATALOG_ENTRIES];
};

/**
 * A structure which holds references to elements of the mountlist.
 * This is used in resize_drive_proportionately_to_fit_new_drives() to
 * ensure accurate resizing.
 */
struct mountlist_reference
{
  /**
   * The number of entries in the list of mountlist references.
   */
  int entries;

  /**
   * The array of mountlist_line, allocated on demand.
   */
  struct mountlist_line **el;
};

/**
 * A line in @p additional_raid_variables.
 */
struct raid_var_line
{
  /**
   * The label for this RAID variable.
   */
  char label[64];

  /**
   * The value for this RAID variable.
   */
  char value[64];
};

/**
 * The additional RAID variables structure.
 * This is used to store a list of additional variables to be put in the raidtab,
 * to allow users to use (new) features of RAID which Mondo doesn't (yet) support directly.
 * Each @p raid_device_record has one.
 */
struct additional_raid_variables
{
  /**
   * The number of entries in the list.
   */
  int entries;

  /**
   * The list of entries, all @p entries of them.
   */
  struct raid_var_line el[MAXIMUM_ADDITIONAL_RAID_VARS];
};

/**
 * One disk in a @p list_of_disks.
 */
struct s_disk
{
#ifdef __FreeBSD__
    /**
     * The name of this disk. If blank it will eventually get filled in automatically.
     */
    char name[64];
#endif
  /**
   * The device this entry describes.
   */
  char device[64];

  /**
   * Index number of this entry in the whole disklist.
   */
  int index;
};

/**
 * A list of @p s_disk. Every @p raid_device_record has four.
 */
struct list_of_disks
{
  /**
   * The number of entries in the disklist.
   */
  int entries;

  /**
   * The entries themselves, all @p entries of them.
   */
  struct s_disk el[MAXIMUM_DISKS_PER_RAID_DEV];
};

/**
 * A type of media we're backing up to.
 */
typedef enum { none=0,		///< No type has been set yet.
	       iso,		///< Back up to ISO images.
	       cdr,		///< Back up to recordable CDs (do not erase them).
	       cdrw,		///< Back up to CD-RWs and blank them first.
	       dvd,		///< Back up to DVD+R[W] or DVD-R[W] disks.
	       cdstream,	///< Back up to recordable CDs but treat them like a tape streamer.
	       nfs,		///< Back up to an NFS mount on the local subnet.
	       tape,		///< Back up to tapes.
	       udev		///< Back up to another unsupported device; just send a stream of bytes.
             } t_bkptype;

/**
 * A type of file in the catalog of recent archives.
 */
typedef enum { other,		///< Some other kind of file.
	       fileset,		///< An afioball (fileset), optionally compressed.
	       biggieslice	///< A slice of a biggiefile, optionally compressed.
             } t_archtype;


#ifdef __FreeBSD__

	struct vinum_subdisk
	{
	    char which_device[64];
	};

	struct vinum_plex
	{
	    int raidlevel;
	    int stripesize;
	    int subdisks;
	    struct vinum_subdisk sd[MAXIMUM_RAID_DEVS];
	};
	
	struct vinum_volume
	{
	    char volname[64];
	    int plexes;
	    struct vinum_plex plex[9];
	};
	
	struct raidlist_itself
	{
	    int entries;
	    struct list_of_disks spares;
	    struct list_of_disks disks;
	    struct vinum_volume el[MAXIMUM_RAID_DEVS];
	};
	
#else

        /**
         * A RAID device in the raidlist.
         */
	struct raid_device_record
	{
	  /**
	   * The name of the RAID device (e.g. /dev/md0).
	   */
	  char raid_device[64];

	  /**
	   * The RAID level (-1 to 5) we're using.
	   */
	  int raid_level;

	  /**
	   * Whether the disk has a persistent superblock.
	   */
	  int persistent_superblock;

	  /**
	   * The chunk size of this RAID device.
	   */
	  int chunk_size;

	  /**
	   * A list of the disks to use for storing data.
	   */
	  struct list_of_disks data_disks;

	  /**
	   * A list of the disks to use as "hot spares" in case one dies.
	   */
	  struct list_of_disks spare_disks;

	  /**
	   * A list of the disks to use for storing parity information.
	   */
	  struct list_of_disks parity_disks;

	  /**
	   * A list of the disks in this RAID device that have failed\. Rare.
	   */
	  struct list_of_disks failed_disks;

	  /**
	   * The additional RAID variables for this device.
	   */
	  struct additional_raid_variables additional_vars;
	};

        /**
         * The list of RAID devices.
         * This is intended to be used along with the mountlist, and it can be
         * directly loaded from/saved to raidtab format.
         */
	struct raidlist_itself
	{
	  /**
	   * The number of entries in the list.
	   */
	  int entries;

	  /**
	   * The RAID devices in the raidlist, all @p entries of them.
	   */
	  struct raid_device_record el[MAXIMUM_RAID_DEVS];
	};

#endif

/**
 * The backup information structure.
 *
 * This is the central structure to all the activity going on inside Mondo.
 * It is passed to almost every function that is not just a helper, even those
 * which only use one variable of it, because it is useful keeping all the information
 * together in one place. The usage of particular fields in the bkpinfo is marked in
 * function documentation, but it is best to fill out as many fields as apply, because
 * that function may in turn pass the bkpinfo to other functions which use other fields.
 *
 * To fill out the bkpinfo first call reset_bkpinfo() and pre_param_configuration(). Then set
 * the backup-specific parameters (see mondo/mondoarchive/mondo-cli.c-\>process_switches for
 * an example). After that, you should call post_param_configuration() to set some final
 * parameters based on those you have already set. Failure to do the last step will result in
 * extremely strange and hard-to-track errors in chop_filelist(), since optimal_set_size is 0.
 */
struct s_bkpinfo
{
  /**
   * The device we're backing up to.
   * If backup_media_type is @b cdr, @b cdrw, or @b cdstream, this should be the SCSI node (e.g. 0,1,0).
   * If backup_media_type is @b dvd, @b tape, or @b udev, this should be a /dev entry.
   * If backup_media_type is anything else, this should be blank.
   */
  char media_device[MAX_STR_LEN/4];

  /**
   * An array containing the sizes of each media in our backup set, in MB.
   * For example, media 1's size would be stored in media_size[1].
   * Element 0 is unused.
   * If the size should be autodetected, make it -1 (preferable) or 0.
   * @bug This should probably be only one variable, not an array.
   */
  long media_size[MAX_NOOF_MEDIA+1];

  /**
   * The boot loader that is installed. Available choices are:
   * - 'G' for GRUB
   * - 'L' for LILO
   * - 'E' for ELILO
   * - (FreeBSD only) 'B' for boot0
   * - (FreeBSD only) 'D' for dangerously dedicated
   * - 'R' for Raw
   * - 'U' for Unknown or None
   *
   * The function which_boot_loader() can help you set this.
   */
  char boot_loader;

  /**
   * The boot device on which @p boot_loader is installed.
   * This is a bit difficult to autodetect; you may want
   * to take truncate_to_drive_name() of where_is_root_mounted().
   */
  char boot_device[MAX_STR_LEN/4];

  /**
   * The compression program to use. Currently supported
   * choices are lzop and bzip2; gzip may also work. This is ignored if
   * compression_level is 0.
   */
  char zip_exe[MAX_STR_LEN/4];

  /**
   * The extension your compression program uses. lzop uses lzo, bzip uses
   * bz2, gzip uses gz, etc. Do not include the dot.
   */
  char zip_suffix[MAX_STR_LEN/4];

  /**
   * Devices to back up as biggiefiles.
   *
   * This is useful for backing up NTFS partitions.
   * @c partimage is used to back up only the used sectors, so the space tradeoff is not bad.
   * However, several caveats apply to such a partition:
   * - It must not be mounted during the backup
   * - It must be in a format that partimage knows how to handle
   * - It cannot be verified during the verify or compare phase
   * - It may not be resized or selectively restored at restore-time (all or nothing)
   *
   * This is a useful feature, but use at your own risk.
   */
  char image_devs[MAX_STR_LEN/4];

  /**
   * The compression level (1-9) to use. 0 disables compression.
   */
  int compression_level;

  /**
   * If TRUE, then use @c lzop to compress data.
   * This is used mainly in estimates. The backup/restore may or may
   * not work if you do not set this. You should also set @p zip_exe
   * and @p zip_suffix.
   */
  bool use_lzo;

  /**
   * A filename containing a list of extensions, one per line, to not
   * compress. If this is set to "", afio will still exclude a set of well-known
   * compressed files from compression, but biggiefiles that are compressed
   * will be recompressed again.
   */
  char do_not_compress_these[MAX_STR_LEN/2];

  /**
   * If TRUE, then we should verify a backup.
   */
  bool verify_data;

  /**
   * If TRUE, then we should back up some data.
   */
  bool backup_data;

  /**
   * If TRUE, then we should restore some data.
   */
  bool restore_data;

  
  /**
    * If TRUE, then we should backup/restore using star, not afio
   */
   bool use_star;
   

  /**
   * Size of internal block reads/writes
   */
  long internal_tape_block_size;
   
  /**
   * If TRUE, we're making a CD that will autonuke without confirmation when booted.
   */
  bool disaster_recovery;

  /**
   * The directory we're backing up to.
   * If backup_media_type is @b iso, then this is that directory.
   * If backup_media_type is anything else, this is ignored.
   */
  char isodir[MAX_STR_LEN/4];

/**
   * The prefix to put in front of media number
   * If backup_media_type is @b iso, then this is the prefix for the filename
   * If backup_media_type is anything else, this is ignored.
   */
  char prefix[MAX_STR_LEN/4];

  /**
   * The scratch directory to use.
   * This is the "stage" that the CD image is made directly from.
   * As such, it needs to be at least as large as the largest CD/DVD/ISO.
   */
  char scratchdir[MAX_STR_LEN/4];

  /**
   * The temp directory to use.
   * This is where filesets are stored by the archival threads before
   * the main thread moves them to the scratchdir. You don't need a lot
   * of space here.
   */
  char tmpdir[MAX_STR_LEN/4];

  /**
   * The optimal size for each fileset. This is set automatically in
   * post_param_configuration() based on your @p backup_media_type; you
   * needn't set it yourself.
   */
  long optimal_set_size;

  /**
   * The type of media we're backing up to.
   */
  t_bkptype backup_media_type;
//  bool blank_dvd_first;

  /**
   * Whether we should use a premade filelist or generate our own.
   * If TRUE, then we generate our own filelist from the directories in @p include_paths.
   * If FALSE, then we use the filelist whose name is specified in @p include_paths.
   */
  bool make_filelist;

  /**
   * Directories to back up, or (if !make_filelist) the filelist to use.
   * In the former case, multiple directories should be separated by spaces.
   * If you do nothing, "/" will be used.
   */
  char include_paths[MAX_STR_LEN];

  /**
   * Directories to NOT back up. Ignored if make_filelist == FALSE.
   * Multiple directories should be separated by spaces. /tmp, /proc,
   * the scratchdir, and the tempdir are automatically excluded.
   */
  char exclude_paths[MAX_STR_LEN];

  /**
   * The path to restore files relative to during a restore.
   * This is useful if you want to extract the files (to test, for example)
   * without overwriting the old ones. Ignored during a backup.
   */
  char restore_path[MAX_STR_LEN];

  /**
   * A command to call BEFORE making an ISO image.
   */
  char call_before_iso[MAX_STR_LEN];

  /**
   * A command to call to make an ISO image.
   */
  char call_make_iso[MAX_STR_LEN];

  /**
   * A command to call to burn the ISO image.
   */
  char call_burn_iso[MAX_STR_LEN];

  /**
   * A command to call AFTER making an ISO image.
   */
  char call_after_iso[MAX_STR_LEN];

  /**
   * Path to the user's kernel, or "FAILSAFE" or "SUCKS" to use the kernel
   * included with Mindi.
   */
  char kernel_path[MAX_STR_LEN];

  /**
   * The NFS mount to back up to/restore from.
   * If backup_media_type is not @b nfs, this is ignored.
   * It must contain a colon, and the server's address should be in dotted-decimal IP
   * address form. (Domain names will be resolved in post_param_configuration().)
   */
  char nfs_mount[MAX_STR_LEN];

  /**
   * The directory, relative to the root of @p nfs_mount, to put
   * the backups in.
   */
  char nfs_remote_dir[MAX_STR_LEN];

  /**
   * A tarball containing a program called "usr/bin/post-nuke" that will be run
   * after nuking the system. If "", do not use a post-nuke tarball.
   */
  char postnuke_tarball[MAX_STR_LEN];

  /**
   * If TRUE, then pass cdrecord the argument "blank=fast" to wipe the CDs before
   * writing to them. This has no effect for DVDs.
   */
  bool wipe_media_first;

// patch by Herman Kuster  
  /**
   * The differential level of this backup. Currently only 0 (full backup) and 1
   * (files changed since last full backup) are supported.
   */
  int differential;
// end patch  

  /**
   * If TRUE, then don't eject media when backing up or restoring.
   */
  bool please_dont_eject;

  /**
   * The speed of the CD-R[W] drive.
   */
  int cdrw_speed;

  /**
   * If TRUE, then cdrecord will be passed some flags to help compensate for PCs
   * with eccentric CD-ROM drives. If it has BurnProof technology, or is in a laptop,
   * it probably falls into this category.
   */
  bool manual_cd_tray;

  /**
   * If TRUE, do not make the first CD bootable. This is dangerous but it saves a minute
   * or so. It is useful in testing. Use with care.
   */
  bool nonbootable_backup;

  /**
   * If TRUE, make the bootable CD use LILO/ELILO. If FALSE, use isolinux (the default).
   */
  bool make_cd_use_lilo;
};



/**
 * A node in a directory structure.
 * Its internals are managed by load_filelist() et al; you only need to keep track of the top node.
 * @bug My understanding of this structure is horrendously incomplete. Could you please fill in the details?
 */
struct s_node
{
  /**
   * The character this node contains.
   */
  char ch;

  /**
   * The node to the right of this one.
   */
  struct s_node *right;

  /**
   * The node below this one.
   */
  struct s_node *down;

  /**
   * If TRUE, then this node is selected (for restore, for example).
   */
  bool selected;

  /**
   * If TRUE, then we want to see the directories below this one.
   */
  bool expanded;
};



/**
 * A structure to wrap a FIFO device for writing to a tape/CD stream.
 * @bug Is this structure used (w/the move to a standalone @c buffer and all)?
 */
struct s_wrapfifo {
        /**
	 * The device we write to or read from (a FIFO).
	 */
	char public_device[MAX_STR_LEN/4];

        /**
	 * The actual device that data from the FIFO should be buffered and written to.
	 */
	char private_device[MAX_STR_LEN/4];

        /**
	 * A buffer for holding data read from the FIFO.
	 */
	char internal_buffer_IN_fifo[MAX_STR_LEN/4];

        /**
	 * A buffer for holding data to be written to the FIFO.
	 */
	char internal_buffer_OUT_fifo[MAX_STR_LEN/4];

        /**
	 * If TRUE, then we're writing directly to the tape streamer; if FALSE, we're writing to the FIFO.
	 */
	bool writing_to_private_device;
};



/**
 * Information about one file.
 * This is used as the "zeroth slice" of a biggiefile to be able to recreate
 * its name, mode, owner, group, mtime, atime, and to be able to verify it in Compare Mode.
 */
struct s_filename_and_lstat_info {
        /**
	 * The filename of the file this structure is describing.
	 */
	char filename[MAX_STR_LEN];

        /**
	 * The MD5 checksum (32 hex digits) of this file.
	 */
	char checksum[64];

        /**
	 * Unused; kept for backwards compatibility.
	 */
	char for_backward_compatibility;

        /**
	 * The stat buffer for this file.
	 * Generated with a call to <tt>lstat(&(struc->properties))</tt> where @p struc
	 * is the @p s_filename_and_lstat_info.
	 */
	struct stat properties;
	bool use_partimagehack;
};


/**
 * A file with associated severity if it differed in a verify or compare.
 */
struct s_filelist_entry {
        /**
	 * The name of the file.
	 */
	char filename[MAX_STR_LEN];
        /**
	 * The severity if the file has changed between the backup and live filesystem.
	 * This is on a scale from 1 to 3, 3 being the most important. File patterns which cause
	 * a severity of 1 are:
	 * - /etc/adjtime
	 * - /etc/mtab
	 * - /var/lib/slocate
	 * - /var/lock
	 * - /var/log
	 * - /var/spool (except /var/spool/mail)
	 * - /var/run
	 * - *~
	 * - *.log
	 * - *cache*
	 * - other temporary or unimportant files
	 *
	 * File patterns which cause a severity of 2 are:
	 * - /var (except /var/lock, /var/log, /var/run, /var/spool)
	 * - /home
	 * - /root/.*
	 * - /var/lib (except /var/lib/slocate, /var/lib/rpm)
	 * - /var/spool/mail
	 *
	 * File patterns which cause a severity of 3 are:
	 * - /etc (except /etc/adjtime, /etc/mtab)
	 * - /root (except /root/.*)
	 * - /usr
	 * - /var/lib/rpm
	 * - Anything else not matched explicitly
	 *
	 * @see severity_of_difference
	 */
        int severity;
};


/**
 * A list of @c s_filelist_entry.
 */
struct s_filelist {
        /**
	 * The number of entries in the list.
	 */
        int entries;

        /**
	 * The entries themselves, all @p entries of them.
	 */
	struct s_filelist_entry el[ARBITRARY_MAXIMUM];
};


/**
 * An entry in the tape catalog.
 */
struct s_tapecat_entry {
        /**
	 * The type of archive it is (afioball, slice, or something else).
	 */
	t_archtype type;

        /**
	 * The filelist number or biggiefile (not slice!) number.
	 */
	int number;

        /**
	 * The slice number if it's a biggiefile.
	 */
	long aux;

        /**
	 * The tape position at the point this entry was added.
	 */
	long long tape_posK;

        /**
	 * The filename of the file cataloged here.
	 */
	char fname[MAX_TAPECAT_FNAME_LEN+1];
};


/**
 * A tape catalog, made of a list of @p s_tapecat_entry.
 */
struct s_tapecatalog {
        /**
	 * The number of entries in the tape catalog.
	 */
	int entries;

        /**
	 * The entries themselves, all @p entries of them.
	 */
	struct s_tapecat_entry el[MAX_TAPECATALOG_ENTRIES];
};



struct s_mdrec {
	int md; 	// /dev/mdN
	int raidlevel;	// 0, 1, 5
	struct list_of_disks disks;
	int progress;
};

struct s_mdstat {
	int entries;
	struct s_mdrec el[MAXIMUM_RAID_DEVS];
};

