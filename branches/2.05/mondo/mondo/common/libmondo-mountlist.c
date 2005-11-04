/* libmondo-mountlist.c                            subroutines for handling mountlist
   $Id: libmondo-mountlist.c,v 1.2 2004/06/10 15:29:12 hugo Exp $



08/01
- when evaluating mountlist, skip drive entirely if it does not exist

07/14
- always exclude /devpts, /proc, /sys from mountlist
   
06/29/2004
- changed some char[] to *char
- drivelist is struct now, not char[][]

10/19/2003
- format_device() --- contract /dev/md/N to /dev/mdN to
  overcome devfs problems

06/11
- added support for 5th column in mountlist.txt, for labels

05/08
- added save_mountlist_to_disk() and load_mountlist()
  and misc other funcs from mondorestore.c

05/05
- added Joshua Oreman's FreeBSD patches

04/24
- added some assert()'s

04/23
- cleaned up evaluate_drive_within_mountlist() a bit

01/15/2003
- added code for LVM (Brian Borgeson)

10/19/2002
- added some comments

07/24/2002
- created
*/


/**
 * @file
 * Functions which manipulate the mountlist.
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-mountlist.h"
#include "lib-common-externs.h"
#include "libmondo-raid-EXT.h"
#include "libmondo-devices-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-string-EXT.h"
#include "libmondo-gui-EXT.h"

/*@unused@*/
//static char cvsid[] = "$Id: libmondo-mountlist.c,v 1.2 2004/06/10 15:29:12 hugo Exp $";

/**
 * A global copy of @c bkpinfo, to aid in debugging. As the name implies, <em>don't use this</em>.
 * @ingroup globalGroup
 */
struct s_bkpinfo *g_bkpinfo_DONTUSETHIS = NULL;

/**
 * @addtogroup mountlistGroup
 * @{
 */
/**
 * Evaluate a drive within the mountlist for flaws. For example, too many
 * primary partitions, the first logical isn't 5, duplicate partitions,
 * ovar-allocated or under-allocated, unsupported format, silly size, or
 * silly mountpoint. Under FreeBSD, this checks the disklabel too, for the above-mentioned
 * errors as well as too many BSD partitions (more than 'h').
 * @param mountlist The mountlist to check.
 * @param drive The drive to check (e.g. @c /dev/hda).
 * @param flaws_str Where to put the found flaws (human-readable).
 * @return The number of flaws found (0 for success).
 * @see evaluate_mountlist
 */
int evaluate_drive_within_mountlist (struct mountlist_itself *mountlist,
				 char *drive, char *flaws_str)
#ifdef __FreeBSD__
{
// FreeBSD-specific version of evaluate_drive_within_mountlist()
	/*@ int **************************************************************/
  int prev_part_no = 0;
	int curr_part_no = 0;
	int pos = 0, npos = 0;
	int res = 0;
	int mountpoint_copies = 0;
	int device_copies = 0;
	int i = 0;
	int cur_sp_no = 0;
	int prev_sp_no = 0;
	int foundsome = FALSE;

	/*@ buffers *********************************************************/
  char tmp[MAX_STR_LEN];
	char device[MAX_STR_LEN];
	char mountpoint[MAX_STR_LEN];

	/*@ long ************************************************************/
  long physical_drive_size = 0;
	long amount_allocated = 0;

	/*@ pointers ********************************************************/
	char *part_table_fmt;

	/*@ initialize ******************************************************/
  flaws_str[0] = '\0';
  prev_part_no = 0;
  tmp[0] = '\0';


  physical_drive_size = get_phys_size_of_drive (drive);

  if (physical_drive_size < 0)
    {
      sprintf (tmp, " %s does not exist.", drive);
      strcat (flaws_str, tmp);
    }
  else
    {
      sprintf (tmp, "%s is %ld MB", drive, physical_drive_size);
    }
  log_it (tmp);
  

  /* check DD */
  for (cur_sp_no = 'a'; cur_sp_no < 'z'; ++cur_sp_no) {
      sprintf (device, "%s%c", drive, cur_sp_no);
      if (find_device_in_mountlist (mountlist, device) >= 0)
	  foundsome = TRUE;
  }
  if (foundsome) {
      for (cur_sp_no = 'a'; cur_sp_no < 'z'; ++cur_sp_no) {	  
	  sprintf (device, "%s%c", drive, cur_sp_no);
	  pos = find_device_in_mountlist (mountlist, device);
	  if (pos < 0)
	      {
		  continue;
	      }
	  strcpy (mountpoint, mountlist->el[pos].mountpoint);
	  /* is it too big? */
	  if (curr_part_no > 'h')
	      {
		  sprintf (tmp, " Can only have up to 'h' in disklabel.");
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  /* does partition /dev/adXsYZ exist more than once in the mountlist? */
	  for (i = 0, mountpoint_copies = 0, device_copies = 0;
	       i < mountlist->entries; i++)
	      {
		  if (!strcmp (device, mountlist->el[i].device))
		      {
			  device_copies++;
		      }
	      }
	  if (device_copies > 1)
	      {
		  sprintf (tmp, " %s %s's.", number_to_text (device_copies), device);
		  if (!strstr (flaws_str, tmp))
		      {
			  log_it (tmp);
			  strcat (flaws_str, tmp);
			  res++;
		      }
	      }
	  /* silly partition size? */
	  if (mountlist->el[pos].size < 8192
	      && strcmp (mountlist->el[pos].mountpoint, "lvm"))
	      {
		  sprintf (tmp, " %s is tiny!", device);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  /* mountpoint should begin with / unless it is swap, lvm or raid */
	  if (strcmp (mountlist->el[pos].mountpoint, "swap")
	      && strcmp (mountlist->el[pos].mountpoint, "lvm")
	      && strcmp (mountlist->el[pos].mountpoint, "raid")
	      && strcmp (mountlist->el[pos].mountpoint, "image")
	      && strcmp (mountlist->el[pos].mountpoint, "none")
	      && mountlist->el[pos].mountpoint[0] != '/')
	      {
		  sprintf (tmp, " %s has a weird mountpoint.", device);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  /* is format sensible? */
	  if (!is_this_a_valid_disk_format (mountlist->el[pos].format))
	      {
		  sprintf (tmp, " %s has unsupported format.", device);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  amount_allocated += mountlist->el[pos].size / 1024;
	  prev_sp_no = cur_sp_no;
      }
  }

  npos = pos = 0;
  for (curr_part_no = 1; curr_part_no < 99; curr_part_no++)
    {
      sprintf (device, "%ss%d", drive, curr_part_no);
      pos = find_device_in_mountlist (mountlist, device);
      npos = 0;
      for (cur_sp_no = 'a'; cur_sp_no <= 'h'; cur_sp_no++) {
	  sprintf (device, "%ss%i%c", device, curr_part_no, cur_sp_no);
	  if (find_device_in_mountlist (mountlist, device) >= 0) npos++;
      }
      if (((pos >= 0) || npos) && foundsome) {
	  sprintf (flaws_str + strlen (flaws_str), " %s has both DD and PC-style partitions.", drive);
	  return ++res; // fatal error
      }
	  
      sprintf (device, "%ss%d", drive, curr_part_no);
      strcpy (mountpoint, mountlist->el[pos].mountpoint);
      if (pos > 0 && !npos) {
	  /* gap in the partition list? */
	  if (curr_part_no - prev_part_no > 1)
	      {
		  if (prev_part_no == 0)
		      {
			  sprintf (tmp, " Gap prior to %s.", device);
			  log_it (tmp);
			  strcat (flaws_str, tmp);
			  res++;
		      }
		  else if (curr_part_no > 5
			   || (curr_part_no <= 4 && prev_part_no > 0))
		      {
			  sprintf (tmp, " Gap between %ss%d and %d.", drive, prev_part_no,
				   curr_part_no);
			  log_it (tmp);
			  strcat (flaws_str, tmp);
			  res++;
		      }
	      }
	  /* GPT allows more than 4 primary partitions */
	  part_table_fmt = which_partition_format(drive);
	  /* no spare primary partitions to help accommodate the logical(s)? */
	  if ((curr_part_no >= 5 && prev_part_no == 4) && (strcmp(part_table_fmt,"MBR") == 0)) 
	      {
		  sprintf (tmp, " Partition %ss4 is occupied.", drive);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  /* does partition /dev/adXsY exist more than once in the mountlist? */
	  for (i = 0, mountpoint_copies = 0, device_copies = 0;
	       i < mountlist->entries; i++)
	      {
		  if (!strcmp (device, mountlist->el[i].device))
		      {
			  device_copies++;
		      }
	      }
	  if (device_copies > 1)
	      {
		  sprintf (tmp, " %s %s's.", number_to_text (device_copies), device);
		  if (!strstr (flaws_str, tmp))
		      {
			  log_it (tmp);
			  strcat (flaws_str, tmp);
			  res++;
		      }
	      }
	  /* silly partition size? */
	  if (mountlist->el[pos].size < 8192
	      && strcmp (mountlist->el[pos].mountpoint, "lvm"))
	      {
		  sprintf (tmp, " %s is tiny!", device);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  /* mountpoint should begin with / unless it is swap, lvm or raid */
	  if (strcmp (mountlist->el[pos].mountpoint, "swap")
	      && strcmp (mountlist->el[pos].mountpoint, "lvm")
	      && strcmp (mountlist->el[pos].mountpoint, "raid")
	      && strcmp (mountlist->el[pos].mountpoint, "image")
	      && strcmp (mountlist->el[pos].mountpoint, "none")
	      && mountlist->el[pos].mountpoint[0] != '/')
	      {
		  sprintf (tmp, " %s has a weird mountpoint.", device);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
	  /* is format sensible? */
	  if (!is_this_a_valid_disk_format (mountlist->el[pos].format))
	      {
		  sprintf (tmp, " %s has unsupported format.", device);
		  log_it (tmp);
		  strcat (flaws_str, tmp);
		  res++;
	      }
      } else {
	  /* Check subpartitions */
	  for (cur_sp_no = 'a'; cur_sp_no < 'z'; ++cur_sp_no) {	  
	      sprintf (device, "%ss%d%c", drive, curr_part_no, cur_sp_no);
	      pos = find_device_in_mountlist (mountlist, device);
	      if (pos < 0)
	      {
		  continue;
	      }
	      strcpy (mountpoint, mountlist->el[pos].mountpoint);
	      /* is it too big? */
	      if (curr_part_no > 'h')
		  {
		      sprintf (tmp, " Can only have up to 'h' in disklabel.");
		      log_it (tmp);
		      strcat (flaws_str, tmp);
		      res++;
	      }
	      /* does partition /dev/adXsYZ exist more than once in the mountlist? */
	      for (i = 0, mountpoint_copies = 0, device_copies = 0;
		   i < mountlist->entries; i++)
		  {
		      if (!strcmp (device, mountlist->el[i].device))
			  {
			      device_copies++;
			  }
		  }
	      if (device_copies > 1)
		  {
		      sprintf (tmp, " %s %s's.", number_to_text (device_copies), device);
		      if (!strstr (flaws_str, tmp))
			  {
			      log_it (tmp);
			      strcat (flaws_str, tmp);
			      res++;
			  }
		  }
	      /* silly partition size? */
	      if (mountlist->el[pos].size < 8192
		  && strcmp (mountlist->el[pos].mountpoint, "lvm"))
		  {
		      sprintf (tmp, " %s is tiny!", device);
		      log_it (tmp);
		      strcat (flaws_str, tmp);
		      res++;
		  }
	      /* mountpoint should begin with / unless it is swap, lvm or raid */
	      if (strcmp (mountlist->el[pos].mountpoint, "swap")
		  && strcmp (mountlist->el[pos].mountpoint, "lvm")
		  && strcmp (mountlist->el[pos].mountpoint, "raid")
		  && strcmp (mountlist->el[pos].mountpoint, "image")
		  && strcmp (mountlist->el[pos].mountpoint, "none")
		  && mountlist->el[pos].mountpoint[0] != '/')
		  {
		      sprintf (tmp, " %s has a weird mountpoint.", device);
		      log_it (tmp);
		      strcat (flaws_str, tmp);
		      res++;
		  }
	      /* is format sensible? */
	      if (!is_this_a_valid_disk_format (mountlist->el[pos].format))
		  {
		      sprintf (tmp, " %s has unsupported format.", device);
		      log_it (tmp);
		      strcat (flaws_str, tmp);
		      res++;
		  }
	      amount_allocated += mountlist->el[pos].size / 1024;
	      prev_sp_no = cur_sp_no;
	  }
      }
      
      /* OK, continue with main loop */
      amount_allocated += mountlist->el[pos].size / 1024;
      prev_part_no = curr_part_no;
    }

  /* Over-allocated the disk? Unallocated space on disk? */
  if (amount_allocated > physical_drive_size)  // Used to be +1, but what if you're 1 MB too high?
    {
      sprintf (tmp, " %ld MB over-allocated on %s.",
	       amount_allocated - physical_drive_size, drive);
      log_it (tmp);
      strcat (flaws_str, tmp);
      res++;
    }
  else if (amount_allocated < physical_drive_size - 1)
    {				/* NOT AN ERROR, JUST A WARNING :-) */
      sprintf (tmp, " %ld MB unallocated on %s.",
	       physical_drive_size - amount_allocated, drive);
      log_it (tmp), strcat (flaws_str, tmp);
    }
  if (res)
    {
      return (FALSE);
    }
  else
    {
      return (TRUE);
    }
}

#else
// Linux-specific version of evaluate_drive_within_mountlist()
{

	/*@ int **************************************************************/
  int prev_part_no = 0;
	int curr_part_no = 0;
	int pos = 0;
	int res = 0;
	int mountpoint_copies = 0;
	int device_copies = 0;
	int i = 0;

	/*@ buffers *********************************************************/
  char *tmp;
	char *device;
	char *mountpoint;

	/*@ long ************************************************************/
  long physical_drive_size = 0;
	long amount_allocated = 0;

	/*@ pointers ********************************************************/
	char *part_table_fmt;

	/*@ initialize ******************************************************/
  assert_string_is_neither_NULL_nor_zerolength(drive);
  assert(mountlist!=NULL);
  assert(flaws_str!=NULL);

  malloc_string(tmp);
  malloc_string(device);
  malloc_string(mountpoint);
  flaws_str[0] = '\0';
  prev_part_no = 0;
  tmp[0] = '\0';


  physical_drive_size = get_phys_size_of_drive (drive);

  if (physical_drive_size < 0)
    {
      sprintf (tmp, " %s does not exist.", drive);
      strcat (flaws_str, tmp);
      res++;
      log_msg(1, tmp);
      goto endoffunc;
    }
  else
    {
      sprintf (tmp, "%s is %ld MB", drive, physical_drive_size);
      log_it (tmp);
    }

  for (curr_part_no = 1; curr_part_no < 99; curr_part_no++)
    {
      sprintf (device, "%s%d", drive, curr_part_no);
      pos = find_device_in_mountlist (mountlist, device);
      if (pos < 0)
	{
	  continue;
	}
      if (physical_drive_size < 0)
        {
          sprintf(tmp, " %s refers to non-existent hardware.", device);
          strcat (flaws_str, tmp);
          res++;
          continue;
        }
      strcpy (mountpoint, mountlist->el[pos].mountpoint);
      /* gap in the partition list? */
      if (curr_part_no - prev_part_no > 1)
	{
	  if (prev_part_no == 0)
	    {
	      sprintf (tmp, " Gap prior to %s.", device);
	      log_it (tmp);
	      strcat (flaws_str, tmp);
	      res++;
	    }
	  else if (curr_part_no > 5
		   || (curr_part_no <= 4 && prev_part_no > 0))
	    {
	      sprintf (tmp, " Gap between %s%d and %d.", drive, prev_part_no,
		       curr_part_no);
	      log_it (tmp);
	      strcat (flaws_str, tmp);
	      res++;
	    }
	}
      /* GPT allows more than 4 primary partitions */
      part_table_fmt = which_partition_format(drive);
      /* no spare primary partitions to help accommodate the logical(s)? */
      if ((curr_part_no >= 5 && prev_part_no == 4) && (strcmp(part_table_fmt,"MBR") == 0)) 
	{
	  sprintf (tmp, " Partition %s4 is occupied.", drive);
	  log_it (tmp);
	  strcat (flaws_str, tmp);
	  res++;
	}
      /* does partition /dev/hdNX exist more than once in the mountlist? */
      for (i = 0, mountpoint_copies = 0, device_copies = 0;
	   i < mountlist->entries; i++)
	{
	  if (!strcmp (device, mountlist->el[i].device))
	    {
	      device_copies++;
	    }
	}
      if (device_copies > 1)
	{
	  sprintf (tmp, " %s %s's.", number_to_text (device_copies), device);
	  if (!strstr (flaws_str, tmp))
	    {
	      log_it (tmp);
	      strcat (flaws_str, tmp);
	      res++;
	    }
	}
      /* silly partition size? */
      if (mountlist->el[pos].size < 8192
	  && strcmp (mountlist->el[pos].mountpoint, "lvm"))
	{
	  sprintf (tmp, " %s is tiny!", device);
	  log_it (tmp);
	  strcat (flaws_str, tmp);
	  res++;
	}
      /* mountpoint should begin with / unless it is swap, lvm or raid */
      if (strcmp (mountlist->el[pos].mountpoint, "swap")
	  && strcmp (mountlist->el[pos].mountpoint, "lvm")
	  && strcmp (mountlist->el[pos].mountpoint, "raid")
	  && strcmp (mountlist->el[pos].mountpoint, "image")
	  && mountlist->el[pos].mountpoint[0] != '/')
	{
	  sprintf (tmp, " %s has a weird mountpoint.", device);
	  log_it (tmp);
	  strcat (flaws_str, tmp);
	  res++;
	}
      /* is format sensible? */
      if (!is_this_a_valid_disk_format (mountlist->el[pos].format))
	{
	  sprintf (tmp, " %s has unsupported format.", device);
	  log_it (tmp);
	  strcat (flaws_str, tmp);
	  res++;
	}
      /* OK, continue with main loop */
      amount_allocated += mountlist->el[pos].size / 1024;
      prev_part_no = curr_part_no;
    }

  /* Over-allocated the disk? Unallocated space on disk? */
  if (amount_allocated > physical_drive_size + 1)
    {
      sprintf (tmp, " %ld MB over-allocated on %s.",
	       amount_allocated - physical_drive_size, drive);
      log_it (tmp);
      strcat (flaws_str, tmp);
      res++;
    }
  else if (amount_allocated < physical_drive_size - 1)
    {				/* NOT AN ERROR, JUST A WARNING :-) */
      sprintf (tmp, " %ld MB unallocated on %s.",
	       physical_drive_size - amount_allocated, drive);
      log_it (tmp), strcat (flaws_str, tmp);
    }

endoffunc:
  paranoid_free(tmp);
  paranoid_free(device);
  paranoid_free(mountpoint);
  
  if (res)
    {
      return (FALSE);
    }
  else
    {
      return (TRUE);
    }
}
#endif



/**
 * Evaluate a whole mountlist for flaws. Calls evaluate_drive_within_mountlist()
 * for each drive, and then spreads the flaws across three lines.
 * @param mountlist The mountlist to evaluate.
 * @param flaws_str_A Where to put the first line listing errors found.
 * @param flaws_str_B Where to put the second line listing errors found.
 * @param flaws_str_C Where to put the third line listing errors found.
 * @return The number of flaws found (0 for success).
 * @see evaluate_drive_within_mountlist
 */
int
evaluate_mountlist (struct mountlist_itself *mountlist, char *flaws_str_A,
		    char *flaws_str_B, char *flaws_str_C)
{

	/*@ buffer ************************************************************/
  	struct list_of_disks *drivelist;
	char *tmp;
	char *flaws_str;

	/*@ int ***************************************************************/
	int i = 0;
	int res = 0;

	/*@ initialize ********************************************************/

  drivelist = malloc(sizeof(struct list_of_disks));
  malloc_string(tmp);
  malloc_string(flaws_str);
  assert(mountlist!=NULL);
  assert(flaws_str_A!=NULL);
  assert(flaws_str_B!=NULL);
  assert(flaws_str_C!=NULL);
  flaws_str[0] = '\0';

  make_list_of_drives_in_mountlist (mountlist, drivelist);

  log_it ("Evaluating mountlist...");

 for (i = 0; i < drivelist->entries; i++)
    {
      if (strstr (drivelist->el[i].device, DONT_KNOW_HOW_TO_EVALUATE_THIS_DEVICE_TYPE))
	{
	  sprintf (tmp, " Not evaluating %s (I don't know how yet)",
		   drivelist->el[i].device);
	  log_it (tmp);
	  tmp[0] = '\0';
	}
      else
	{
	  if (!evaluate_drive_within_mountlist
	      (mountlist, drivelist->el[i].device, tmp))
	    {
	      res++;
	    }
	}
      strcat (flaws_str, tmp);
    }
  res += look_for_duplicate_mountpoints (mountlist, flaws_str);
/*  res+=look_for_weird_formats(mountlist,flaws_str); .. not necessary, now that we can check to see
 which formarts are actually _supported_ by the kernel */
  /* log_it(flaws_str); */
  return (spread_flaws_across_three_lines
	  (flaws_str, flaws_str_A, flaws_str_B, flaws_str_C, res));
}


/**
 * Find the index number of @p device in the mountlist.
 * The device given must match @p mountlist->el[N].device exactly, case-sensitive.
 * @param mountlist The mountlist to search in.
 * @param device The device to search for.
 * @return The zero-based index of the device, or -1 if it could not be found.
 */
int
find_device_in_mountlist (struct mountlist_itself *mountlist, char *device)
{

	/*@ int ***************************************************************/
  int i = 0;
  char*tmp;
  char*flaws_str;
  
  malloc_string(tmp);
  malloc_string(flaws_str);
 
  assert(mountlist!=NULL);
  assert_string_is_neither_NULL_nor_zerolength(device);
  for (i = 0;
       i < mountlist->entries
       && strcmp (mountlist->el[i].device, device) != 0; i++);
  
  paranoid_free(tmp);
  paranoid_free(flaws_str);

  if (i == mountlist->entries)
    {
      return (-1);
    }
  else
    {
      return (i);
    }
}





/**
 * Look for duplicate mountpoints in @p mountlist.
 * @param mountlist The mountlist to check.
 * @param flaws_str The flaws string to append the results to.
 * @return The number of mountpoints that have duplicates, or 0 for success.
 */
int
look_for_duplicate_mountpoints (struct mountlist_itself *mountlist,
				char *flaws_str)
{

	/*@ int **************************************************************/
  int res = 0;
	int currline = 0;
	int i = 0;
	int copies = 0;
	int last_copy = 0;

	/*@ buffetr **********************************************************/
  char *curr_mountpoint;
	char *tmp;

  malloc_string(curr_mountpoint);
  malloc_string(tmp);
  assert(mountlist!=NULL);
  assert(flaws_str!=NULL);
  for (currline = 0; currline < mountlist->entries; currline++)
    {
      strcpy (curr_mountpoint, mountlist->el[currline].mountpoint);
      for (i = 0, copies = 0, last_copy = -1; i < mountlist->entries; i++)
	{
	  if (!strcmp (mountlist->el[i].mountpoint, curr_mountpoint)
	      && strcmp (mountlist->el[i].mountpoint, "lvm")
	      && strcmp (mountlist->el[i].mountpoint, "swap"))
	    {
	      last_copy = i;
	      copies++;
	    }
	}
      if (copies > 1 && last_copy == currline
	  && strcmp (curr_mountpoint, "raid"))
	{
	  sprintf (tmp, " %s %s's.", number_to_text (copies),
		   curr_mountpoint);
	  strcat (flaws_str, tmp);
	  log_it (tmp);
	  res++;
	}
    }
  paranoid_free(curr_mountpoint);
  paranoid_free(tmp);
  return (res);
}

/**
 * Look for strange formats. Does not respect /proc/filesystems.
 * @param mountlist The mountlist to check.
 * @param flaws_str The flaws string to append the results to.
 * @return The number of weird formats found, or 0 for success.
 * @bug Seems orphaned; please remove.
 */
int
look_for_weird_formats (struct mountlist_itself *mountlist, char *flaws_str)
{

	/*@ int **************************************************************/
  int i	= 0;
	int res = 0;

	/*@ buffers **********************************************************/
  char *tmp;
	char *format_sz;

  malloc_string(tmp);
  malloc_string(format_sz);
  
  assert(mountlist!=NULL);
  assert(flaws_str!=NULL);

  for (i = 0; i < mountlist->entries; i++)
    {
      sprintf (format_sz, " %s ", mountlist->el[i].format);
      if (!strstr
	  (SANE_FORMATS, format_sz) 
		&& strcmp (mountlist->el[i].mountpoint, "image") != 0)
	{
	  sprintf (tmp, " %s has unknown format.", mountlist->el[i].device);
	  log_it (tmp);
	  strcat (flaws_str, tmp);
	  res++;
	}
      else
	if ((!strcmp (mountlist->el[i].format, "swap")
	     && strcmp (mountlist->el[i].mountpoint, "swap") && strcmp (mountlist->el[i].mountpoint, "none"))
	    || (strcmp (mountlist->el[i].format, "swap")
		&& !strcmp (mountlist->el[i].mountpoint, "swap") && !strcmp (mountlist->el[i].mountpoint, "none")))
	{
	  sprintf (tmp, " %s is half-swap.", mountlist->el[i].device);
	  log_it (tmp);
	  strcat (flaws_str, tmp);
	  res++;
	}
    }
  paranoid_free(tmp);
  paranoid_free(format_sz);
  return (res);
}



/**
 * Make a list of the drives mentioned in the mountlist.
 * @param mountlist The mountlist to examine.
 * @param drivelist Where to put the list of drives found.
 * @return The number of physical (non-RAID non-LVM) drives found, or \<= 0 for error.
 */
int
make_list_of_drives_in_mountlist (struct mountlist_itself *mountlist,
		     struct list_of_disks *drivelist)
{

		/*@ int **************************************************************/
  int lino;
  int noof_drives;
  int j;

		/*@ buffers **********************************************************/
  char *drive;
  char *tmp;

  long long size;

  malloc_string(drive);
  malloc_string(tmp);
  assert(mountlist!=NULL);
  assert(drivelist!=NULL);
  log_it ("Making list of drives");
  for (lino = 0, noof_drives = 0; lino < mountlist->entries; lino++)
    {
      
      strcpy (drive, mountlist->el[lino].device);
      if (!strncmp (drive, RAID_DEVICE_STUB, strlen(RAID_DEVICE_STUB)))
	{
	  sprintf (tmp,
		   "Not putting %s in list of drives: it's a virtual drive",
		   drive);
	  log_msg (8, tmp);
	  continue;
	}

      size=mountlist->el[lino].size;
      if ( size == 0 )
        {
          sprintf (tmp,
                   "Not putting %s in list of drives: it has zero size (maybe an LVM volume)",
                   drive);
          log_msg (8, tmp);
          continue;
        }

/*
      for (i = strlen (drive); isdigit (drive[i - 1]); i--);
      drive[i] = '\0';
      if (get_phys_size_of_drive (drive) <= 0 && drive[i - 1] == 'p')
	{
	  i--;
	  drive[i] = '\0';
	}
      for (j = 0; j < noof_drives && strcmp (drivelist[j], drive) != 0; j++);
*/

      sprintf (tmp,
    	   "Putting %s with size %lli in list of drives",
    	   drive, size);
      log_msg (8, tmp);

      (void) truncate_to_drive_name (drive);
      for (j = 0; j < noof_drives && strcmp (drivelist->el[j].device, drive) != 0; j++)
	continue;
      if (j == noof_drives)
	{
	  strcpy (drivelist->el[noof_drives++].device, drive);
	}
    }
  drivelist->entries = noof_drives;
  log_msg (8, "Made list of drives");
  paranoid_free(drive);
  paranoid_free(tmp);

  return (noof_drives);
}






/**
 * Make a list of RAID partitions not currently associated with any RAID device.
 * The user may add any of these partitions to the RAID device.
 * @param output_list Where to put the list of unallocated RAID partitions.
 * @param mountlist The mountlist to examine.
 * @param raidlist The raidlist to examine.
 */
void
make_list_of_unallocated_raid_partitions (struct mountlist_itself
					  *output_list,
					  struct mountlist_itself *mountlist,
					  struct raidlist_itself *raidlist)
{

	/*@ int **************************************************************/
  int items = 0;
	int i = 0;
	int used_by = 0;

	/*@ buffers **********************************************************/
  char *tmp;

  malloc_string(tmp);
  assert(output_list!=NULL);
  assert(mountlist!=NULL);
  assert(raidlist!=NULL);
  log_it ("MLOURP -- starting");
  items = 0;


  for (i = 0; i < mountlist->entries; i++)
    {
      if (strstr (mountlist->el[i].mountpoint, "raid"))
	{
	  used_by =
	    which_raid_device_is_using_this_partition (raidlist,
						       mountlist->el[i].
						       device);
	  if (used_by < 0)
	    {
	      memcpy ((void *) &output_list->el[items++],
		      (void *) &mountlist->el[i],
		      sizeof (struct mountlist_line));
	      sprintf (tmp,
		       "%s is available; user may choose to add it to raid device",
		       output_list->el[items - 1].device);
	      log_it (tmp);
	    }
	}
    }
  output_list->entries = items;
  log_it ("MLUORP -- ending");
  paranoid_free(tmp);
}





/**
 * Get the size of a mountlist entry by the @c device field.
 * @param mountlist The mountlist to search in.
 * @param device The device to search for
 * @return The size of the device (in KB), or -1 if it could not be found.
 */
long long
size_of_specific_device_in_mountlist (struct mountlist_itself *mountlist, char *device)
{
	/*@ int ***************************************************************/
  int i = 0;


  assert(mountlist!=NULL);
  assert_string_is_neither_NULL_nor_zerolength(device);

  for (i = 0;
       i < mountlist->entries && strcmp (mountlist->el[i].device, device);
       i++);
  if (i == mountlist->entries)
    {
      return (-1);
    }
  else
    {
      return (mountlist->el[i].size);
    }
}




/**
 * Load a file on disk into @p mountlist.
 * The file on disk should consist of multiple lines, each containing 4 or 5
 * columns: the device, the mountpoint, the filesystem type, the size in kilobytes, and optionally the filesystem label.
 * Comments begin with a '#' without any leading whitespace. Any duplicate
 * entries are renamed.
 * @param mountlist The mountlist to load into.
 * @param fname The name of the file to load the mountlist from.
 * @return 0 for success, 1 for failure.
 */
int
load_mountlist( struct mountlist_itself *mountlist, char *fname)
{
  FILE *fin;
  /* malloc ***/
  char *incoming;
  char *siz;
  char *tmp;
  char*p;

  int items;
  int j;

  assert(mountlist!=NULL);
  assert_string_is_neither_NULL_nor_zerolength(fname);
  malloc_string(incoming);
  malloc_string(siz);
  malloc_string(tmp);
  if ( !( fin = fopen( fname, "r" ) ) )
    {
      log_it( "Unable to open mountlist - '%s'", fname );
      log_to_screen( "Cannot open mountlist" );
      paranoid_free(incoming);
      paranoid_free(siz);
      paranoid_free(tmp);
      return( 1 );
    }
  items = 0;
  (void) fgets( incoming, MAX_STR_LEN - 1, fin );
  log_it( "Loading mountlist..." );
  while( !feof( fin ) )
    {
#if linux
      sscanf( incoming,
	      "%s %s %s %s %s",
	      mountlist->el[items].device,
	      mountlist->el[items].mountpoint,
	      mountlist->el[items].format,
	      siz,
	      mountlist->el[items].label);
#elif __FreeBSD__
      sscanf (incoming,
	      "%s %s %s %s",
	      mountlist->el[items].device,
	      mountlist->el[items].mountpoint,
	      mountlist->el[items].format,
	      siz);
      strcpy (mountlist->el[items].label, "");
#endif

      if (
        !strcmp(mountlist->el[items].device,"/proc") ||
        !strcmp(mountlist->el[items].device, "proc") ||
        !strcmp(mountlist->el[items].device,"/sys") ||
        !strcmp(mountlist->el[items].device, "sys") ||
        !strcmp(mountlist->el[items].device,"/devpts") ||
	!strcmp(mountlist->el[items].device, "devpts")
      )
        {
	  log_msg(1, "Ignoring %s in mountlist - not loading that line :) ", mountlist->el[items].device);
          (void) fgets( incoming, MAX_STR_LEN - 1,fin );
	  continue;
	}
      mountlist->el[items].size = atoll( siz );
      if ( mountlist->el[items].device[0] != '\0' && mountlist->el[items].device[0] != '#' )
	{
	  if ( items >= ARBITRARY_MAXIMUM )
	    {
	      log_to_screen( "Too many lines in mountlist.. ABORTING" );
	      finish( 1 );
	    }
	  for( j=0; j < items && strcmp( mountlist->el[j].device, mountlist->el[items].device ); j++);
	  if (j < items)
	    {
	      strcat( mountlist->el[items].device, "_dup" );
	      sprintf( tmp, "Duplicate entry in mountlist - renaming to %s",mountlist->el[items].device );
	      log_it( tmp );
	    }
	strcpy(tmp, mountlist->el[items].device);
	if (strstr(tmp, "/dev/md/"))
	  {
	    log_it("format_device() --- Contracting %s", tmp);
	    p = strrchr(tmp, '/');
	    if (p) { *p = *(p+1); *(p+1) = *(p+2); *(p+2) = *(p+3); }
	    log_it("It was %s; it is now %s", mountlist->el[items].device, tmp);
	    strcpy(mountlist->el[items].device, tmp);
	  }

	  sprintf( tmp,
		   "%s %s %s %lld %s",
		   mountlist->el[items].device,
		   mountlist->el[items].mountpoint,
		   mountlist->el[items].format,
		   mountlist->el[items].size,
		   mountlist->el[items].label);

	  log_it( tmp );
	  items++;
	}
      (void) fgets( incoming, MAX_STR_LEN - 1,fin );
    }
  paranoid_fclose( fin );
  mountlist->entries = items;

  log_it( "Mountlist loaded successfully." );
  sprintf( tmp, "%d entries in mountlist", items );
  log_it( tmp );
  paranoid_free(incoming);
  paranoid_free(siz);
  paranoid_free(tmp);
  return( 0 );
}



/**
 * Save @p mountlist to a file on disk.
 * @param mountlist The mountlist to save.
 * @param fname The file to save it to.
 * @return 0 for success, 1 for failure.
 * @see load_mountlist
 */
int 
save_mountlist_to_disk(struct mountlist_itself *mountlist, char *fname)
{
  FILE *fout;
  int i;

  assert(mountlist!=NULL);
  assert_string_is_neither_NULL_nor_zerolength(fname);

  log_it("save_mountlist_to_disk() --- saving to %s", fname);
  if (!(fout=fopen(fname,"w"))) 
    {
      log_OS_error("WMTD - Cannot openout mountlist");
      return(1);
    }
  for( i = 0; i < mountlist->entries; i++)
    {
      fprintf(fout,
	      "%-15s %-15s %-15s %-15lld %-15s\n",mountlist->el[i].device, 
	      mountlist->el[i].mountpoint, 
	      mountlist->el[i].format, 
	      mountlist->el[i].size,
	      mountlist->el[i].label);
    }
  paranoid_fclose(fout);
  return(0);
}



/**
 * Sort the mountlist alphabetically by device.
 * The sorting is done in-place.
 * @param mountlist The mountlist to sort.
 */
void 
sort_mountlist_by_device( struct mountlist_itself *mountlist)
{
  int diff;
  int lino = -999;

  assert(mountlist!=NULL);

  while (lino < mountlist->entries)
    {
      for(lino = 1; lino < mountlist->entries; lino++)
	{
	  diff = strcmp_inc_numbers(mountlist->el[lino-1].device,mountlist->el[lino].device);
	  if (diff > 0)
	    {
	      swap_mountlist_entries( mountlist, lino - 1, lino);
	      break;
	    }
	}
    }
}

/**
 * Sort the mountlist alphabetically by mountpoint.
 * The sorting is done in-place.
 * @param mountlist The mountlist to sort.
 * @param reverse If TRUE, then do a reverse sort.
 */
void 
sort_mountlist_by_mountpoint(struct mountlist_itself *mountlist, bool reverse)
{
  int diff;
  int lino = -999;

  assert(mountlist!=NULL);

  while(lino < mountlist->entries)
    {
      for(lino = 1; lino < mountlist->entries; lino++)
	{
	  diff = strcmp( mountlist->el[lino-1].mountpoint,mountlist->el[lino].mountpoint);
	  if ((diff >0 && !reverse) || ((diff <0 && reverse)))
	    {
	      swap_mountlist_entries(mountlist, lino - 1, lino);
	      break;
	    }
	}
    }
}


/**
 * Swap two entries in the mountlist in-place.
 * @param mountlist The mountlist to swap the entries in.
 * @param a The index number of the first entry.
 * @param b The index number of the second entry.
 */
void 
swap_mountlist_entries(struct mountlist_itself *mountlist, int a, int b)
{
  /*@ mallocs ****/
  char device[64];
  char mountpoint[256];
  char format[64];

  long long size;

  assert(mountlist!=NULL);
  assert(a>=0);
  assert(b>=0);

  strcpy(device,                     mountlist->el[a].device);
  strcpy(mountpoint,                 mountlist->el[a].mountpoint);
  strcpy(format,                     mountlist->el[a].format);

  size=                              mountlist->el[a].size;

  strcpy(mountlist->el[a].device,    mountlist->el[b].device);
  strcpy(mountlist->el[a].mountpoint,mountlist->el[b].mountpoint);
  strcpy(mountlist->el[a].format,    mountlist->el[b].format);

  mountlist->el[a].size=             mountlist->el[b].size;

  strcpy(mountlist->el[b].device,    device);
  strcpy(mountlist->el[b].mountpoint,mountpoint);
  strcpy(mountlist->el[b].format,    format);

  mountlist->el[b].size=             size;
}

/* @} - end of mountlistGroup */
