/***************************************************************************
 * $Id$
**/


/**
 * @file
 * Functions for handling GUI interfaces in the restore.
 */

#ifdef __FreeBSD__
#define OSSWAP(linux,fbsd) fbsd
#else
#define OSSWAP(linux,fbsd) linux
#endif

#include "mondo-rstr-newt.h"

/**
 * @defgroup restoreGuiDisklist Disklist GUI
 * Functions for manipulating the disklist GUI.
 * @ingroup restoreGuiGroup
 */
/**
 * @defgroup restoreGuiMountlist Mountlist GUI
 * Functions for manipulating the mountlist/raidlist GUI.
 * @ingroup restoreGuiGroup
 */
/**
 * @defgroup restoreGuiVarslist RAID Variables GUI
 * Functions for manipulating the RAID variables GUI.
 * @ingroup restoreGuiGroup
 */

/**
 * @addtogroup restoreGuiGroup
 * @{
 */
/**
 * Add an entry in @p disklist from the list in @p unallocated_raid_partitions.
 * @param disklist The disklist to add an entry to.
 * @param raid_device Unused; make sure it's non-NULL non-"".
 * @param unallocated_raid_partitions The list of unallocated RAID partitions
 * that the user may choose from.
 * @bug raid_device is unused.
 * @ingroup restoreGuiDisklist
 */
void
add_disklist_entry(struct list_of_disks *disklist, char *raid_device,
				   struct mountlist_itself *unallocated_raid_partitions)
{
	/** buffers ***********************************************************/
	char *tmp = NULL;

	/** newt **************************************************************/
	newtComponent myForm;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent b_res;
	newtComponent partitionsListbox;
	newtComponent headerMsg;

  /** prototypes *********************************************************/
	void *keylist[ARBITRARY_MAXIMUM];
	void *curr_choice;

	/** int ****************************************************************/
	int i = 0;
	int index = 0;
	int currline = 0;
	int items = 0;

	assert(disklist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(raid_device);
	assert(unallocated_raid_partitions != NULL);

	newtPushHelpLine
		(_
		 ("   Add one of the following unallocated RAID partitions to this RAID device."));
	asprintf(&tmp, "%-26s %s", _("Device"), _("Size"));
	headerMsg = newtLabel(1, 1, tmp);
	paranoid_free(tmp);

	partitionsListbox =
		newtListbox(1, 2, 6, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
	redraw_unallocpartnslist(unallocated_raid_partitions, keylist,
							 partitionsListbox);
	i = 7;
	bOK = newtCompactButton(i, 9, _("  OK  "));
	bCancel = newtCompactButton(i += 9, 9, _("Cancel"));
	newtOpenWindow(22, 6, 36, 10, _("Unallocated RAID partitions"));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, headerMsg, partitionsListbox, bOK,
						  bCancel, NULL);
	b_res = newtRunForm(myForm);
	if (b_res != bCancel) {
		curr_choice = newtListboxGetCurrent(partitionsListbox);
		for (currline = 0;
			 currline < unallocated_raid_partitions->entries
			 && keylist[currline] != curr_choice; currline++);
		if (currline == unallocated_raid_partitions->entries
			&& unallocated_raid_partitions->entries > 0) {
			log_it("I don't know what this button does");
		} else {
			index = find_next_free_index_in_disklist(disklist);

			items = disklist->entries;
			strcpy(disklist->el[items].device,
				   unallocated_raid_partitions->el[currline].device);
			disklist->el[items].index = index;
			disklist->entries = ++items;

		}
	}
	newtFormDestroy(myForm);
	newtPopWindow();
	newtPopHelpLine();
}


/**
 * Add an entry to @p mountlist.
 * @param mountlist The mountlist to add an entry to.
 * @param raidlist The raidlist that accompanies @p mountlist.
 * @param listbox The listbox component in the mountlist editor.
 * @param currline The line selected in @p listbox.
 * @param keylist The list of keys for @p listbox.
 * @ingroup restoreGuiMountlist
 */
void
add_mountlist_entry(struct mountlist_itself *mountlist,
					struct raidlist_itself *raidlist,
					newtComponent listbox, int currline, void *keylist[])
{

	/** int **************************************************************/
	int i = 0;
	int num_to_add = 0;

	/** newt *************************************************************/
	newtComponent myForm;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent b_res;
	newtComponent mountpointComp;
	newtComponent label0;
	newtComponent label1;
	newtComponent label2;
	newtComponent label3;
	newtComponent sizeComp;
	newtComponent deviceComp;
	newtComponent formatComp;

	char *drive_to_add = NULL;
	char *mountpoint_str = NULL;
	char *size_str = NULL;
	char *device_str = NULL;
	char *format_str = NULL;
	char *mountpoint_here = NULL;
	char *size_here = NULL;
	char *device_here = NULL;
	char *format_here = NULL;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert(listbox != NULL);
	assert(keylist != NULL);

	asprintf(&device_str, "/dev/");
	asprintf(&mountpoint_str, "/");
#ifdef __FreeBSD__
	asprintf(&format_str, "ufs");
#else
	asprintf(&format_str, "ext2");
#endif
	newtOpenWindow(20, 5, 48, 10, _("Add entry"));
	label0 = newtLabel(2, 1, _("Device:    "));
	label1 = newtLabel(2, 2, _("Mountpoint:"));
	label2 = newtLabel(2, 3, _("Size (MB): "));
	label3 = newtLabel(2, 4, _("Format:    "));
	deviceComp =
		newtEntry(14, 1, device_str, 30, (void *) &device_here, 0);
	mountpointComp =
		newtEntry(14, 2, mountpoint_str, 30, (void *) &mountpoint_here, 0);

	formatComp =
		newtEntry(14, 4, format_str, 15, (void *) &format_here, 0);
	sizeComp = newtEntry(14, 3, size_str, 10, (void *) &size_here, 0);
	bOK = newtButton(5, 6, _("  OK  "));
	bCancel = newtButton(17, 6, _("Cancel"));
	newtPushHelpLine
		(_
		 ("To add an entry to the mountlist, please fill in these fields and then hit 'OK'"));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, deviceComp, mountpointComp, sizeComp,
						  formatComp, label0, label1, label2, label3, bOK,
						  bCancel, NULL);
	for (b_res = NULL; b_res != bOK && b_res != bCancel;) {
		b_res = newtRunForm(myForm);

		paranoid_free(device_str);
		asprintf(&device_str, device_here);
		strip_spaces(device_str);

		paranoid_free(format_str);
		asprintf(&format_str, format_here);
		strip_spaces(format_str);

		paranoid_free(mountpoint_str);
		asprintf(&mountpoint_str, mountpoint_here);
		strip_spaces(mountpoint_str);

		paranoid_free(size_str);
		asprintf(&size_str, size_here);
		strip_spaces(size_str);

		if (b_res == bOK) {
			if (device_str[strlen(device_str) - 1] == '/') {
				popup_and_OK(_("You left the device nearly blank!"));
				b_res = NULL;
			}
			if (size_of_specific_device_in_mountlist(mountlist, device_str)
				>= 0) {
				popup_and_OK(_
							 ("Can't add this - you've got one already!"));
				b_res = NULL;
			}
		}
	}
	newtFormDestroy(myForm);
	newtPopHelpLine();
	newtPopWindow();
	if (b_res == bCancel) {
		return;
	}
	asprintf(&drive_to_add, device_str);
	for (i = strlen(drive_to_add); isdigit(drive_to_add[i - 1]); i--);
	num_to_add = atoi(drive_to_add + i);
	drive_to_add[i] = '\0';
	paranoid_free(drive_to_add);

	currline = mountlist->entries;
	strcpy(mountlist->el[currline].device, device_str);
	strcpy(mountlist->el[currline].mountpoint, mountpoint_str);
	paranoid_free(mountpoint_str);

	strcpy(mountlist->el[currline].format, format_str);
	paranoid_free(format_str);

	mountlist->el[currline].size = atol(size_str) * 1024;
	paranoid_free(size_str);

	mountlist->entries++;
	if (strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)) {
		initiate_new_raidlist_entry(raidlist, mountlist, currline,
									device_str);
	}
	paranoid_free(device_str);
	redraw_mountlist(mountlist, keylist, listbox);
}


#ifndef __FreeBSD__
/**
 * Add an entry to the additional RAID variables section of @p raidrec.
 * @param raidrec The RAID device record containing the RAID variables list to add to.
 * @ingroup restoreGuiVarslist
 */
void add_varslist_entry(struct raid_device_record *raidrec)
{

	/** buffers ***********************************************************/
	char *sz_out = NULL;

	/** int ****************************************************************/
	int items = 0;
	int i = 0;

	assert(raidrec != NULL);

	if (popup_and_get_string
		("Add variable", _("Enter the name of the variable to add"),
		 sz_out)) {
		items = raidrec->additional_vars.entries;
		for (i = 0;
			 i < items
			 && strcmp(raidrec->additional_vars.el[i].label, sz_out); i++);
		if (i < items) {
			popup_and_OK
				(_
				 ("No need to add that variable. It is already listed here."));
		} else {
			strcpy(raidrec->additional_vars.el[items].label, sz_out);
			edit_varslist_entry(raidrec, items);
			raidrec->additional_vars.entries = ++items;
		}
	}
	paranoid_free(sz_out);
}
#endif

/**
 * Calculate the size of @p raid_device.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @param raid_device The device to calculate the size of.
 * @return The size of the RAID device in Kilobytes.
 * @ingroup restoreUtilityGroup
 */
long
calculate_raid_device_size(struct mountlist_itself *mountlist,
						   struct raidlist_itself *raidlist,
						   char *raid_device)
{
#ifdef __FreeBSD__
	/** FreeBSD-specific version of calculate_raid_device_size() **/

	/** structures ********************************************************/
	struct vinum_volume *raidrec = NULL;

	int i = 0, j = 0;
	int noof_partitions = 0;

	long total_size = 0;
	long plex_size = 0;
	long smallest_partition = 999999999;
	long smallest_plex = 999999999;
	long sp = 0;

	char *tmp = NULL;
	char *devname = NULL;

	for (i = 0;
		 i < raidlist->entries
		 && strcmp(raidlist->el[i].volname, basename(raid_device)); i++);
	if (i == raidlist->entries) {
		asprintf(&tmp,
				 "Cannot calc size of raid device %s - cannot find it in raidlist",
				 raid_device);
		log_it(tmp);
		paranoid_free(tmp);
		return (0);				// Isn't this more sensible than 999999999? If the raid dev !exists,
		// then it has no size, right?
	}
	raidrec = &raidlist->el[i];
	total_size = 0;
	if (raidrec->plexes == 0)
		return 0;
	for (j = 0; j < raidrec->plexes; j++) {
		plex_size = 0;
		int k = 0, l = 0;
		for (k = 0; k < raidrec->plex[j].subdisks; ++k) {
			asprintf(&devname, raidrec->plex[j].sd[k].which_device);
			for (l = 0; l < raidlist->disks.entries; ++l) {
				if (!strcmp(devname, raidlist->disks.el[l].name)) {
					switch (raidrec->plex[j].raidlevel) {
					case -1:
						plex_size +=
							size_of_specific_device_in_mountlist(mountlist,
																 raidlist->
																 disks.
																 el[l].
																 device);
						break;
					case 0:
					case 5:
						if (size_of_specific_device_in_mountlist(mountlist,
																 raidlist->
																 disks.
																 el[l].
																 device) <
							smallest_partition) {
							smallest_partition =
								size_of_specific_device_in_mountlist
								(mountlist, raidlist->disks.el[l].device);
						}
						break;
					}
				}
			}
			paranoid_free(devname);
		}

		if (!is_this_raid_personality_registered
			(raidrec->plex[j].raidlevel)) {
			log_it
				("%s has a really weird RAID level - couldn't calc size :(",
				 raid_device);
			return (999999999);
		}
		if (raidrec->plex[j].raidlevel != -1) {
			plex_size = smallest_partition * (raidrec->plex[j].subdisks -
											  (raidrec->plex[j].
											   raidlevel == 5 ? 1 : 0));
		}
		if (plex_size < smallest_plex)
			smallest_plex = plex_size;

		smallest_partition = 999999999;
	}

	asprintf(&tmp, "I have calculated %s's real size to be %ld",
			 raid_device, (long) smallest_plex);
	log_it(tmp);
	paranoid_free(tmp);
	return (smallest_plex);
#else
	/** Linux-specific version of calculate_raid_device_size() **/

	/** structures ********************************************************/
	struct raid_device_record *raidrec = NULL;

	/** int ***************************************************************/
	int i = 0;
	int noof_partitions = 0;

	/** long **************************************************************/
	long total_size = 0;
	long smallest_partition = 999999999;
	long sp = 0;

	/** buffers ***********************************************************/
	char *tmp = NULL;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(raid_device);

	for (i = 0;
		 i < raidlist->entries
		 && strcmp(raidlist->el[i].raid_device, raid_device); i++);
	if (i == raidlist->entries) {
		asprintf(&tmp,
				 "Cannot calc size of raid device %s - cannot find it in raidlist",
				 raid_device);
		log_it(tmp);
		paranoid_free(tmp);
		return (999999999);
	}
	raidrec = &raidlist->el[i];
	noof_partitions = raidrec->data_disks.entries;
	if (raidrec->raid_level == -1 || raidrec->raid_level == 0) {
		for (total_size = 0, i = 0; i < noof_partitions; i++) {
			total_size +=
				size_of_specific_device_in_mountlist(mountlist,
													 raidrec->data_disks.
													 el[i].device);
		}
	} else {
		for (i = 0; i < noof_partitions; i++) {
			sp = size_of_specific_device_in_mountlist(mountlist,
													  raidrec->data_disks.
													  el[i].device);
			if (smallest_partition > sp) {
				smallest_partition = sp;
			}
		}
		total_size = smallest_partition * (noof_partitions - 1);
	}
	asprintf(&tmp, "I have calculated %s's real size to be %ld",
			 raid_device, (long) total_size);
	log_it(tmp);
	paranoid_free(tmp);
	return (total_size);
#endif
}


/**
 * Choose the RAID level for the RAID device record in @p raidrec.
 * @param raidrec The RAID device record to set the RAID level of.
 * @ingroup restoreGuiMountlist
 */
void
choose_raid_level(struct OSSWAP (raid_device_record, vinum_plex) * raidrec)
{

#ifdef __FreeBSD__

	/** int ***************************************************************/
	int res = 0;
	int out = 0;

	/** buffers ***********************************************************/
	char *tmp = NULL;
	char *prompt = NULL;
	char *sz = NULL;

	asprintf(&prompt,
			 _
			 ("Please enter the RAID level you want. (concat, striped, raid5)"));
	if (raidrec->raidlevel == -1) {
		asprintf(&tmp, "concat");
	} else if (raidrec->raidlevel == 0) {
		asprintf(&tmp, "striped");
	} else {
		asprintf(&tmp, "raid%i", raidrec->raidlevel);
	}
	for (out = 999; out == 999;) {
		res = popup_and_get_string("Specify RAID level", prompt, tmp);
		if (!res) {
			return;
		}
		/* BERLIOS: Useless ??? 
		if (tmp[0] == '[' && tmp[strlen(tmp) - 1] == ']') {
			asprintf(&sz, tmp);
			strncpy(tmp, sz + 1, strlen(sz) - 2);
			tmp[strlen(sz) - 2] = '\0';
			paranoid_free(sz);
		}
		*/
		if (!strcmp(tmp, "concat")) {
			out = -1;
		} else if (!strcmp(tmp, "striped")) {
			out = 0;
		} else if (!strcmp(tmp, "raid5")) {
			out = 5;
		}
		log_it(tmp);
		paranoid_free(tmp);
		if (is_this_raid_personality_registered(out)) {
			log_it
				("Groovy. You've picked a RAID personality which is registered.");
		} else {
			if (ask_me_yes_or_no
				("You have chosen a RAID personality which is not registered with the kernel. Make another selection?"))
			{
				out = 999;
			}
		}
	}
	paranoid_free(prompt);
	raidrec->raidlevel = out;
#else
	/** buffers ***********************************************************/
	char *tmp = NULL;
	char *personalities = NULL;
	char *prompt = NULL;
	char *sz = NULL;
	int out = 0, res = 0;


	assert(raidrec != NULL);
	system
		("grep Pers /proc/mdstat > /tmp/raid-personalities.txt 2> /dev/null");
	personalities = last_line_of_file("/tmp/raid-personalities.txt"));
	asprintf(&prompt, _("Please enter the RAID level you want. %s"),
			 personalities);
	paranoid_free(personalities);

	if (raidrec->raid_level == -1) {
		asprintf(&tmp, "linear");
	} else {
		asprintf(&tmp, "%d", raidrec->raid_level);
	}
	for (out = 999;
		 out != -1 && out != 0 && out != 1 && out != 4 && out != 5
		 && out != 10;) {
		res =
			popup_and_get_string(_("Specify RAID level"), prompt, tmp);
		if (!res) {
			return;
		}
		/* BERLIOS: Useless ???
		if (tmp[0] == '[' && tmp[strlen(tmp) - 1] == ']') {
			asprintf(&sz, tmp);
			paranoid_free(tmp);

			asprintf(&tmp, sz + 1);
			tmp[strlen(sz) - 2] = '\0';
			paranoid_free(sz);
		}
		*/
		if (!strcmp(tmp, "linear")) {
			out = -1;
		} else if (!strncmp(tmp, "raid", 4)) {
			out = atoi(tmp + 4);
		} else {
			out = atoi(tmp);
		}
		log_it(tmp);
		if (is_this_raid_personality_registered(out)) {
			log_it
				("Groovy. You've picked a RAID personality which is registered.");
		} else {
			if (ask_me_yes_or_no
				(_
				 ("You have chosen a RAID personality which is not registered with the kernel. Make another selection?")))
			{
				out = 999;
			}
		}
	}
	paranoid_free(tmp);
	paranoid_free(prompt);
	raidrec->raid_level = out;
#endif
}


/**
 * Delete the partitions in @p disklist from @p mountlist because they
 * were part of a deleted RAID device.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist that goes with @p mounntlist.
 * @param disklist The list of disks to remove from @p mountlist.
 * @ingroup restoreGuiDisklist
 */
void
del_partns_listed_in_disklist(struct mountlist_itself *mountlist,
							  struct raidlist_itself *raidlist,
							  struct list_of_disks *disklist)
{

	/** int ***************************************************************/
	int i = 0;
	int pos = 0;

	/** buffers ***********************************************************/
	char *tmp = NULL;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert(disklist != NULL);

	for (i = 0; i < disklist->entries; i++) {
		for (pos = 0;
			 pos < mountlist->entries
			 && strcmp(mountlist->el[pos].device, disklist->el[i].device);
			 pos++);
		if (pos < mountlist->entries) {
			log_it("Deleting partition %s cos it was part of a now-defunct RAID",
					 mountlist->el[pos].device);
			memcpy((void *) &mountlist->el[pos],
				   (void *) &mountlist->el[mountlist->entries - 1],
				   sizeof(struct mountlist_line));
			mountlist->entries--;
		}
	}
}


/**
 * Delete entry number @p currline from @p disklist.
 * @param disklist The disklist to remove the entry from.
 * @param raid_device The RAID device containing the partition we're removing.
 * Used only in the popup "are you sure?" box.
 * @param currline The line number (starting from 0) of the item to delete.
 * @ingroup restoreGuiDisklist
 */
void
delete_disklist_entry(struct list_of_disks *disklist, char *raid_device,
					  int currline)
{

	/** int ***************************************************************/
	int pos = 0;

	/** buffers ***********************************************************/
	char *tmp = NULL;

	assert(disklist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(raid_device);

	asprintf(&tmp, _("Delete %s from RAID device %s - are you sure?"),
			 disklist->el[currline].device, raid_device);
	if (!ask_me_yes_or_no(tmp)) {
		paranoid_free(tmp);
		return;
	}
	paranoid_free(tmp);
	for (pos = currline; pos < disklist->entries - 1; pos++) {
		/* memcpy((void*)&disklist->el[pos], (void*)&disklist->el[pos+1], sizeof(struct s_disk)); */
		strcpy(disklist->el[pos].device, disklist->el[pos + 1].device);
	}
	disklist->entries--;
}


/**
 * Delete entry number @p currline from @p mountlist.
 * @param mountlist The mountlist to delete the entry from.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @param listbox The Newt listbox component in the mountlist editor.
 * @param currline The line number (starting from 0) of the item to delete.
 * @param keylist The list of keys for @p listbox.
 * @ingroup restoreGuiMountlist
 */
void
delete_mountlist_entry(struct mountlist_itself *mountlist,
					   struct raidlist_itself *raidlist,
					   newtComponent listbox, int currline,
					   void *keylist[])
{

	/** int ***************************************************************/
	int pos = 0;

	/** buffers ***********************************************************/
	char *tmp = NULL;
	char *device = NULL;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert(listbox != NULL);
	assert(keylist != NULL);

	pos =
		which_raid_device_is_using_this_partition(raidlist,
												  mountlist->el[currline].
												  device);
	if (pos >= 0) {
		asprintf(&tmp,
				 _("Cannot delete %s: it is in use by RAID device %s"),
				 mountlist->el[currline].device,
				 raidlist->el[pos].OSSWAP(raid_device, volname));
		popup_and_OK(tmp);
		paranoid_free(tmp);
		return;
	}
	asprintf(&tmp, _("Delete %s - are you sure?"),
			 mountlist->el[currline].device);
	if (!ask_me_yes_or_no(tmp)) {
		paranoid_free(tmp);
		return;
	}
	paranoid_free(tmp);

	if (strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)) {
		asprintf(&device, mountlist->el[currline].device);
		delete_raidlist_entry(mountlist, raidlist, device);
		for (currline = 0;
			 currline < mountlist->entries
			 && strcmp(mountlist->el[currline].device, device);
			 currline++);
			if (currline == mountlist->entries) {
				log_it("Dev is gone. I can't delete it. Ho-hum");
				paranoid_free(device);
				return;
			}
		paranoid_free(device);
	}
	memcpy((void *) &mountlist->el[currline],
		   (void *) &mountlist->el[mountlist->entries - 1],
		   sizeof(struct mountlist_line));
	mountlist->entries--;
	redraw_mountlist(mountlist, keylist, listbox);
}


/**
 * Delete @p device from @p raidlist.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist containing the RAID device to delete.
 * @param device The device (e.g. /dev/md0) to delete.
 * @ingroup restoreGuiMountlist
 */
void
delete_raidlist_entry(struct mountlist_itself *mountlist,
					  struct raidlist_itself *raidlist, char *device)
{

	/** int ***************************************************************/
	int i = 0;
	int items = 0;

	/** bool **************************************************************/
	bool delete_partitions_too;

	/** buffers ***********************************************************/
	char *tmp = NULL;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(device);

	i = find_raid_device_in_raidlist(raidlist, device);
	if (i < 0) {
		return;
	}
	asprintf(&tmp,
			 _("Do you want me to delete %s's partitions, too?"), device);
	delete_partitions_too = ask_me_yes_or_no(tmp);
	if (delete_partitions_too) {
#ifdef __FreeBSD__
		// static so it's zeroed
		static struct list_of_disks d;
		int x, y, z;

		for (x = 0; x < raidlist->el[i].plexes; ++x) {
			for (y = 0; y < raidlist->el[i].plex[x].subdisks; ++y) {
				for (z = 0; z < raidlist->disks.entries; ++z) {
					if (!strcmp(raidlist->el[i].plex[x].sd[y].which_device,
								raidlist->disks.el[z].name)) {
						strcpy(d.el[d.entries].name,
							   raidlist->disks.el[z].name);
						strcpy(d.el[d.entries++].device,
							   raidlist->disks.el[z].device);
					}
				}
			}
		}

		del_partns_listed_in_disklist(mountlist, raidlist, &d);
#else
		del_partns_listed_in_disklist(mountlist, raidlist,
									  &raidlist->el[i].data_disks);
		del_partns_listed_in_disklist(mountlist, raidlist,
									  &raidlist->el[i].spare_disks);
		del_partns_listed_in_disklist(mountlist, raidlist,
									  &raidlist->el[i].parity_disks);
		del_partns_listed_in_disklist(mountlist, raidlist,
									  &raidlist->el[i].failed_disks);
#endif
	}
	items = raidlist->entries;
	if (items == 1) {
		items = 0;
	} else {
		log_it(tmp);
		memcpy((void *) &raidlist->el[i],
			   (void *) &raidlist->el[items - 1],
			   sizeof(struct OSSWAP (raid_device_record, vinum_volume)));
		items--;
	}
	paranoid_free(tmp);

	raidlist->entries = items;
}


#ifndef __FreeBSD__
/**
 * Delete entry number @p lino in the additional RAID variables section of @p raidrec.
 * @param raidrec The RAID device record containing the RAID variable to delete.
 * @param lino The line number (starting from 0) of the variable to delete.
 * @ingroup restoreGuiVarslist
 */
void delete_varslist_entry(struct raid_device_record *raidrec, int lino)
{

	/** buffers ************************************************************/
	char *tmp = NULL;

	/** structures *********************************************************/
	struct additional_raid_variables *av = NULL;

	assert(raidrec != NULL);

	av = &raidrec->additional_vars;
	asprintf(&tmp, _("Delete %s - are you sure?"), av->el[lino].label);
	if (ask_me_yes_or_no(tmp)) {
		if (!strcmp(av->el[lino].label, "persistent-superblock")
			|| !strcmp(av->el[lino].label, "chunk-size")) {
			paranoid_free(tmp);
			asprintf(&tmp, _("%s must not be deleted. It would be bad."),
					 av->el[lino].label);
			popup_and_OK(tmp);
		} else {
			memcpy((void *) &av->el[lino], (void *) &av->el[av->entries--],
				   sizeof(struct raid_var_line));
		}
	}
	paranoid_free(tmp);
}
#endif


/**
 * Redraw the filelist display.
 * @param filelist The filelist structure to edit.
 * @param keylist The list of keys for @p listbox.
 * @param listbox The Newt listbox component containing some of the filelist entries.
 * @return The number of lines currently being displayed.
 * @ingroup restoreGuiGroup
 */
int
redraw_filelist(struct s_node *filelist, void *keylist[ARBITRARY_MAXIMUM],
				newtComponent listbox)
{

	/** int ***************************************************************/
	static int lines_in_flist_window = 0;
	static int depth = 0;
	int i = 0;

	/** structures *******************************************************/
	struct s_node *node = NULL;

	/** buffers **********************************************************/
	static char *current_filename = NULL;
	char *tmp = NULL;

	/** bool *************************************************************/
	/*  void*dummyptr; */
	bool dummybool;
	static bool warned_already;

	assert(filelist != NULL);
	assert(keylist != NULL);
	assert(listbox != NULL);

	if (depth == 0) {
		lines_in_flist_window = 0;
		warned_already = FALSE;
		for (i = 0; i < ARBITRARY_MAXIMUM; i++) {
			g_strings_of_flist_window[i][0] = '\0';
			g_is_path_selected[i] = FALSE;
		}
	}
	for (node = filelist; node != NULL; node = node->right) {
		current_filename[depth] = node->ch;
		if (node->down) {
			depth++;
			i = redraw_filelist(node->down, keylist, listbox);
			depth--;
		}
		if (node->ch == '\0' && node->expanded) {
			if (lines_in_flist_window == ARBITRARY_MAXIMUM) {
				if (!warned_already) {
					warned_already = TRUE;
					asprintf(&tmp,
							 _
							 ("Too many lines. Displaying first %d entries only. Close a directory to see more."),
							 ARBITRARY_MAXIMUM);
					popup_and_OK(tmp);
					paranoid_free(tmp);
				}
			} else {
				strcpy(g_strings_of_flist_window[lines_in_flist_window],
					   current_filename);
				g_is_path_selected[lines_in_flist_window] = node->selected;
				lines_in_flist_window++;
			}
		}
	}
	if (depth == 0) {
		if (lines_in_flist_window > ARBITRARY_MAXIMUM) {
			lines_in_flist_window = ARBITRARY_MAXIMUM;
		}
/* do an elementary sort */
		for (i = 1; i < lines_in_flist_window; i++) {
			if (strcmp
				(g_strings_of_flist_window[i],
				 g_strings_of_flist_window[i - 1]) < 0) {
				asprintf(&tmp, g_strings_of_flist_window[i]);
				strcpy(g_strings_of_flist_window[i],
					   g_strings_of_flist_window[i - 1]);
				strcpy(g_strings_of_flist_window[i - 1], tmp);
				paranoid_free(tmp);

				dummybool = g_is_path_selected[i];
				g_is_path_selected[i] = g_is_path_selected[i - 1];
				g_is_path_selected[i - 1] = dummybool;
				i = 0;
			}
		}
/* write list to screen */
		newtListboxClear(listbox);
		for (i = 0; i < lines_in_flist_window; i++) {
			asprintf(&tmp, "%c%c %-80s",
					 (g_is_path_selected[i] ? '*' : ' '),
					 (g_is_path_expanded[i] ? '+' : '-'),
					 strip_path(g_strings_of_flist_window[i]));
			// BERLIOS: this is dangerous now  => Memory leak
			if (strlen(tmp) > 71) {
				tmp[70] = '\0';
			}
			keylist[i] = (void *) i;
			newtListboxAppendEntry(listbox, tmp, keylist[i]);
			paranoid_free(tmp);
		}
		return (lines_in_flist_window);
	} else {
		return (0);
	}
}


/**
 * Strip a path to the bare minimum (^ pointing to the directory above, plus filename).
 * @param tmp The path to strip.
 * @return The stripped path.
 * @author Conor Daly
 * @ingroup restoreUtilityGroup
 */
char *strip_path(char *tmp)
{

	int i = 0, j = 0, slashcount = 0;
	int slashloc = 0, lastslashloc = 0;

	while (tmp[i] != '\0') {	/* Count the slashes in tmp
								   1 slash per dir */
		if (tmp[i] == '/') {
			slashcount++;
			lastslashloc = slashloc;
			slashloc = i;
			if (tmp[i + 1] == '\0') {	/* if this slash is last char, back off */
				slashcount--;
				slashloc = lastslashloc;
			}
		}
		i++;
	}
	if (slashcount > 0)
		slashcount--;			/* Keep one slash 'cos Hugh does... */

	/* BERLIOS: tmpnopath and prev not defined !! How can this compile ?? */
	for (i = 0; i < slashcount; i++) {	/* Replace each dir with a space char */
		tmpnopath[i] = ' ';
	}

	i = slashloc;
	j = slashcount;
	while (tmp[i] != '\0') {	/* Now add what's left of tmp */
		if ((tmpprevpath[j] == ' ' || tmpprevpath[j] == '^')
			&& tmp[i] == '/' && tmpnopath[j - 1] != '^' && j != 0) {	/* Add a pointer upwards if this is not in the same dir as line above */
			tmpnopath[j - 1] = '^';
		} else {
			tmpnopath[j++] = tmp[i++];
		}
	}
	tmpnopath[j] = '\0';

	strcpy(tmpprevpath, tmpnopath);	/* Make a copy for next iteration */

	return (tmpnopath);
}


/**
 * Allow the user to edit the filelist and choose which files to restore.
 * @param filelist The node structure containing the filelist.
 * @return 0 if the user pressed OK, 1 if they pressed Cancel.
 */
int edit_filelist(struct s_node *filelist)
{

	/** newt **************************************************************/
	newtComponent myForm;
	newtComponent bLess = NULL;
	newtComponent bMore = NULL;
	newtComponent bToggle = NULL;
	newtComponent bOK = NULL;
	newtComponent bCancel = NULL;
	newtComponent b_res = NULL;
	newtComponent filelistListbox = NULL;
	newtComponent bRegex = NULL;

	/** int ***************************************************************/
	int finished = FALSE;
	int lines_in_flist_window = 0;
	int indexno = 0;
	int j = 0;

	/** ???? **************************************************************/
	void *curr_choice;
	void *keylist[ARBITRARY_MAXIMUM];

	/** bool **************************************************************/
	bool dummybool;

/*  struct s_node *node; */

	assert(filelist != NULL);

	log_to_screen(_("Editing filelist"));
	newtPushHelpLine
		(_
		 ("   Please edit the filelist to your satisfaction, then click OK or Cancel."));
	j = 4;
	bLess = newtCompactButton(j, 17, _(" Less "));
	bMore = newtCompactButton(j += 12, 17, _(" More "));
	bToggle = newtCompactButton(j += 12, 17, _("Toggle"));
	bRegex = newtCompactButton(j += 12, 17, _("RegEx"));
	bCancel = newtCompactButton(j += 12, 17, _("Cancel"));
	bOK = newtCompactButton(j += 12, 17, _("  OK  "));
	filelistListbox =
		newtListbox(2, 1, 15, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
	toggle_all_root_dirs_on(filelist);
	lines_in_flist_window =
		redraw_filelist(filelist, keylist, filelistListbox);
	newtOpenWindow(1, 3, 77, 18, _("Editing filelist"));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, filelistListbox, bLess, bMore, bToggle,
						  bRegex, bCancel, bOK, NULL);
	while (!finished) {
		b_res = newtRunForm(myForm);
		if (b_res == bOK) {
			finished =
				ask_me_yes_or_no
				(_("Are you happy with your file selection?"));
		} else if (b_res == bCancel) {
			finished = TRUE;
		} else if (b_res == bRegex) {
			popup_and_OK(_("I haven't implemented this yet..."));
		} else {
			curr_choice = newtListboxGetCurrent(filelistListbox);
			for (indexno = 0;
				 indexno < lines_in_flist_window
				 && keylist[indexno] != curr_choice; indexno++);
			if (indexno == lines_in_flist_window) {
				log_it
					("I don't know what this button does; assuming I am to toggle 1st entry");
				indexno = 0;
			}
			log_it("You selected '%s'",
					g_strings_of_flist_window[indexno]);
			if (b_res == bMore) {
				g_is_path_expanded[indexno] = TRUE;
				toggle_path_expandability(filelist,
										  g_strings_of_flist_window
										  [indexno], TRUE);
				lines_in_flist_window =
					redraw_filelist(filelist, keylist, filelistListbox);
				newtListboxSetCurrentByKey(filelistListbox, curr_choice);
			} else if (b_res == bLess) {
				g_is_path_expanded[indexno] = FALSE;
				toggle_path_expandability(filelist,
										  g_strings_of_flist_window
										  [indexno], FALSE);
				lines_in_flist_window =
					redraw_filelist(filelist, keylist, filelistListbox);
				newtListboxSetCurrentByKey(filelistListbox, curr_choice);
			} else {
				if (!strcmp(g_strings_of_flist_window[indexno], "/")) {
					dummybool = !g_is_path_selected[indexno];
					for (j = 1; j < lines_in_flist_window; j++) {
						toggle_path_selection(filelist,
											  g_strings_of_flist_window[j],
											  dummybool);
					}
				} else {
					toggle_path_selection(filelist,
										  g_strings_of_flist_window
										  [indexno],
										  !g_is_path_selected[indexno]);
					lines_in_flist_window =
						redraw_filelist(filelist, keylist,
										filelistListbox);
				}
				newtListboxSetCurrentByKey(filelistListbox, curr_choice);
			}
			for (indexno = 0;
				 indexno < lines_in_flist_window
				 && keylist[indexno] != curr_choice; indexno++);
			if (indexno == lines_in_flist_window) {
				log_it
					("Layout of table has changed. Y pointer is reverting to zero.");
				indexno = 0;
			}
		}
	}
	newtFormDestroy(myForm);
	newtPopWindow();
	newtPopHelpLine();
	if (b_res == bOK) {
		return (0);
	} else {
/*    popup_and_OK("You pushed 'cancel'. I shall now abort."); */
		return (1);
	}
}


/**
 * Edit an entry in @p mountlist.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param raidlist The raidlist to accompany @p mountlist.
 * @param listbox The Newt listbox component in the mountlist editor.
 * @param currline The selected line (starting from 0) in @p listbox.
 * @param keylist The list of keys for @p listbox.
 * @ingroup restoreGuiMountlist
 */
void
edit_mountlist_entry(struct mountlist_itself *mountlist,
					 struct raidlist_itself *raidlist,
					 newtComponent listbox, int currline, void *keylist[])
{

	/** structures ********************************************************/
	static struct raidlist_itself bkp_raidlist;

	/** newt **************************************************************/
	newtComponent myForm;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent b_res;
	newtComponent mountpointComp;
	newtComponent label0;
	newtComponent label1;
	newtComponent label2;
	newtComponent label3;
	newtComponent sizeComp;
	newtComponent deviceComp;
	newtComponent formatComp;
	newtComponent b_raid = NULL;

	char *device_str = NULL;
	char *mountpoint_str = NULL;
	char *size_str = NULL;
	char *format_str = NULL;
	char *tmp = NULL;
	char *device_used_to_be = NULL;
	char *mountpt_used_to_be = NULL;
	char *device_here = NULL;
	char *mountpoint_here = NULL;
	char *size_here = NULL;
	char *format_here = NULL;

	int j = 0;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert(listbox != NULL);
	assert(keylist != NULL);

	memcpy((void *) &bkp_raidlist, (void *) raidlist,
		   sizeof(struct raidlist_itself));
	asprintf(&device_str, mountlist->el[currline].device);
	asprintf(&device_used_to_be, mountlist->el[currline].device);
	asprintf(&mountpoint_str, mountlist->el[currline].mountpoint);
	asprintf(&mountpt_used_to_be, mountlist->el[currline].mountpoint);
	asprintf(&format_str, mountlist->el[currline].format);
	asprintf(&size_str, "%lld", mountlist->el[currline].size / 1024);

	newtOpenWindow(20, 5, 48, 10, "Edit entry");
	label0 = newtLabel(2, 1, _("Device:"));
	label1 = newtLabel(2, 2, _("Mountpoint:"));
	label2 = newtLabel(2, 3, _("Size (MB): "));
	label3 = newtLabel(2, 4, _("Format:    "));
	deviceComp =
		newtEntry(14, 1, device_str, 30, (void *) &device_here, 0);
	paranoid_free(device_str);

	mountpointComp =
		newtEntry(14, 2, mountpoint_str, 30, (void *) &mountpoint_here, 0);
	paranoid_free(mountpoint_str);

	formatComp =
		newtEntry(14, 4, format_str, 15, (void *) &format_here, 0);
	paranoid_free(format_str);

	if (strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)
		|| !strcmp(mountlist->el[currline].mountpoint, "image")) {
		sizeComp = newtLabel(14, 3, size_str);
	} else {
		sizeComp = newtEntry(14, 3, size_str, 10, (void *) &size_here, 0);
	}
	paranoid_free(size_str);

	bOK = newtButton(2, 6, _("  OK  "));
	bCancel = newtButton(14, 6, _("Cancel"));
	if (strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)) {
		b_raid = newtButton(26, 6, "RAID..");
	}
	newtPushHelpLine
		(_
		 ("       Edit this partition's mountpoint, size and format; then click 'OK'."));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, deviceComp, mountpointComp, sizeComp,
						  formatComp, label0, label1, label2, label3, bOK,
						  bCancel, b_raid, NULL);
	for (b_res = NULL; b_res != bOK && b_res != bCancel;) {
		b_res = newtRunForm(myForm);

		paranoid_free(device_str);
		asprintf(&device_str, device_here);
		strip_spaces(device_str);

		paranoid_free(mountpoint_str);
		asprintf(&mountpoint_str, mountpoint_here);
		strip_spaces(mountpoint_str);

		paranoid_free(format_str);
		asprintf(&format_str, format_here);
		paranoid_free(format_here);
		strip_spaces(format_str);
		if (b_res == bOK && strstr(device_str, RAID_DEVICE_STUB)
			&& strstr(device_used_to_be, RAID_DEVICE_STUB)
			&& strcmp(device_str, device_used_to_be)) {
			popup_and_OK(_("You can't change /dev/mdX to /dev/mdY."));
			b_res = NULL;
			continue;
		} else if (b_res == bOK && !strcmp(mountpoint_str, "image")
				   && strcmp(mountpt_used_to_be, "image")) {
			popup_and_OK(_
						 ("You can't change a regular device to an image."));
			b_res = NULL;
			continue;
		}
		paranoid_free(mountpt_used_to_be);

		if (!strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)
			&& strcmp(mountlist->el[currline].mountpoint, "image")) {
			asprintf(&size_str, size_here);
			strip_spaces(size_str);
		} else {
			asprintf(&size_str, "%ld",
					calculate_raid_device_size(mountlist, raidlist,
											   mountlist->el[currline].
											   device) / 1024);
			newtLabelSetText(sizeComp, size_str);
		}
		paranoid_free(size_here);

		/* do not let user click RAID button if user has changed device_str */
		if (b_res == b_raid) {
			if (strcmp(device_str, mountlist->el[currline].device)) {
				/*
				   can't change mountlist's entry from /dex/mdX to /dev/mdY: it would ugly 
				   when you try to map the changes over to the raidtab list, trust me
				 */
				popup_and_OK
					(_
					 ("You cannot edit the RAID settings until you have OK'd your change to the device node."));
			} else {
				j = find_raid_device_in_raidlist(raidlist,
												 mountlist->el[currline].
												 device);
				if (j < 0) {
					sprintf(tmp,
							_
							("/etc/raidtab does not have an entry for %s; please delete it and add it again"),
							mountlist->el[currline].device);
					popup_and_OK(tmp);
				} else {
					log_it(_("edit_raidlist_entry - calling"));
					edit_raidlist_entry(mountlist, raidlist,
										&raidlist->el[j], currline);
				}
			}
		}
	}
	paranoid_free(device_here);
	paranoid_free(mountpoint_here);

	newtFormDestroy(myForm);
	newtPopHelpLine();
	newtPopWindow();
	if (b_res == bCancel) {
		memcpy((void *) raidlist, (void *) &bkp_raidlist,
			   sizeof(struct raidlist_itself));
		return;
	}
	strcpy(mountlist->el[currline].device, device_str);
	strcpy(mountlist->el[currline].mountpoint, mountpoint_str);
	paranoid_free(mountpoint_str);

	strcpy(mountlist->el[currline].format, format_str);
	paranoid_free(format_str);

	if (strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)
		|| !strcmp(mountlist->el[currline].mountpoint, "image")) {
		mountlist->el[currline].size =
			calculate_raid_device_size(mountlist, raidlist,
									   mountlist->el[currline].device);
		} else {
			mountlist->el[currline].size = atol(size_str) * 1024;
		}
	}
	paranoid_free(size_str);

	newtListboxSetEntry(listbox, (int) keylist[currline],
						mountlist_entry_to_string(mountlist, currline));
	/* if new /dev/md RAID device then do funky stuff */
	if (strstr(mountlist->el[currline].device, RAID_DEVICE_STUB)
		&& !strstr(device_used_to_be, RAID_DEVICE_STUB)) {
		initiate_new_raidlist_entry(raidlist, mountlist, currline,
									device_str);
	}
	/* if moving from RAID to non-RAID then do funky stuff */
	else if (strstr(device_used_to_be, RAID_DEVICE_STUB)
			 && !strstr(device_str, RAID_DEVICE_STUB)) {
		delete_raidlist_entry(mountlist, raidlist, device_str);
	}
	/* if moving a non-RAID to another non-RAID then re-jig any RAID disks, if necessary */
	else if (!strstr(device_used_to_be, RAID_DEVICE_STUB)
			 && !strstr(device_str, RAID_DEVICE_STUB)) {
		rejig_partition_name_in_raidlist_if_necessary(raidlist,
													  device_used_to_be,
													  device_str);
	}
/* else, moving a RAID to another RAID; bad idea, or so I thought */
#ifndef __FreeBSD__				/* It works fine under FBSD. */
	else if (strcmp(device_used_to_be, device_str)) {
		popup_and_OK
			(_
			 ("You are renaming a RAID device as another RAID device. I don't like it but I'll allow it."));
	}
#endif
	redraw_mountlist(mountlist, keylist, listbox);
	paranoid_free(device_str);
	paranoid_free(device_used_to_be);
}


#if __FreeBSD__
/**
 * Add a subdisk to @p raidrec.
 * @param raidlist The raidlist containing information about RAID partitions.
 * @param raidrec The RAID device record to add the subdisk to.
 * @param temp The device name of the RAID disk to add it to.
 * @author Joshua Oreman
 * @ingroup restoreGuiMountlist
 */
void
add_raid_subdisk(struct raidlist_itself *raidlist,
				 struct vinum_plex *raidrec, char *temp)
{
	int i;
	bool found = FALSE;

	for (i = 0; i < raidlist->disks.entries; ++i) {
		if (!strcmp(raidlist->disks.el[i].device, temp)) {
			strcpy(raidrec->sd[raidrec->subdisks].which_device,
				   raidlist->disks.el[i].name);
			found = TRUE;
		}
	}
	if (!found) {
		sprintf(raidlist->disks.el[raidlist->disks.entries].name,
				"drive%i", raidlist->disks.entries);
		sprintf(raidrec->sd[raidrec->subdisks].which_device, "drive%i",
				raidlist->disks.entries);
		strcpy(raidlist->disks.el[raidlist->disks.entries++].device, temp);
	}
	raidrec->subdisks++;
}


/**
 * Determine the /dev entry for @p vinum_name.
 * @param raidlist The raidlist containing information about RAID devices.
 * @param vinum_name The name of the Vinum drive to map to a /dev entry.
 * @return The /dev entry, or NULL if none was found.
 * @note The returned string points to static storage that will be overwritten with each call.
 * @author Joshua Oreman
 * @ingroup restoreUtilityGroup
 */
char *find_dev_entry_for_raid_device_name(struct raidlist_itself *raidlist,
										  char *vinum_name)
{
	int i;
	for (i = 0; i < raidlist->disks.entries; ++i) {
		if (!strcmp(raidlist->disks.el[i].name, vinum_name)) {
			return raidlist->disks.el[i].device;
		}
	}
	return NULL;
}


void
edit_raidlist_plex(struct mountlist_itself *mountlist,
				   struct raidlist_itself *raidlist,
				   struct vinum_plex *raidrec, int currline,
				   int currline2);

#endif


/**
 * Edit the entry for @p raidrec in @p raidlist.
 * @param mountlist The mountlist to get some information from.
 * @param raidlist The raidlist containing information about RAID devices.
 * @param raidrec The RAID device record for this partition.
 * @param currline The line number (starting from 0) in the mountlist of the RAID device.
 * @ingroup restoreGuiMountlist
 */
void
edit_raidlist_entry(struct mountlist_itself *mountlist,
					struct raidlist_itself *raidlist,
					struct OSSWAP (raid_device_record,
								   vinum_volume) * raidrec, int currline)
{

#ifdef __FreeBSD__
	/** structures ********************************************************/
	struct vinum_volume bkp_raidrec;


	/** buffers ***********************************************************/
	char *title_of_editraidForm_window;

	/** newt **************************************************************/
	newtComponent editraidForm;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent bEdit;
	newtComponent bAdd;
	newtComponent bDelete;
	newtComponent b_res;
	newtComponent plexesListbox;
	newtComponent plexesHeader;

	void *keylist[10];
	void *curr_choice;

	int currline2 = 0;
	char *pname = NULL;
	char *raidlevel = NULL;
	char *chunksize = NULL;
	char *entry = NULL;
	char *msg = NULL;
	int i = 0;
	char *headerstr = NULL;

	log_it(_("Started edit_raidlist_entry"));
	memcpy((void *) &bkp_raidrec, (void *) raidrec,
		   sizeof(struct vinum_volume));
	asprintf(&title_of_editraidForm_window, _("Plexes on %s"),
			raidrec->volname);
	newtPushHelpLine(_("   Please select a plex to edit"));
	newtOpenWindow(13, 5, 54, 15, title_of_editraidForm_window);
	paranoid_free(title_of_editraidForm_window);

	for (;;) {
		asprintf(&headerstr, "%-14s %-8s  %11s  %8s", _("Plex"), _("Level",) _("Stripe Size"), _("Subdisks"));

		bOK = newtCompactButton(2, 13, _("  OK  "));
		bCancel = newtCompactButton(12, 13, _("Cancel"));
		bAdd = newtCompactButton(22, 13, _(" Add "));
		bEdit = newtCompactButton(32, 13, _(" Edit "));
		bDelete = newtCompactButton(42, 13, _("Delete"));

		plexesListbox =
			newtListbox(2, 3, 9, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
		plexesHeader = newtLabel(2, 2, headerstr);
		paranoid_free(headerstr);

		editraidForm = newtForm(NULL, NULL, 0);

		newtListboxClear(plexesListbox);
		for (i = 0; i < 10; ++i) {
			keylist[i] = (void *) i;
			if (i < raidrec->plexes) {
				switch (raidrec->plex[i].raidlevel) {
				case -1:
					asprintf(&raidlevel, "concat");
					break;
				case 0:
					asprintf(&raidlevel, "striped");
					break;
				case 5:
					asprintf(&raidlevel, "raid5");
					break;
				default:
					asprintf(&raidlevel, "raid%i",
							raidrec->plex[i].raidlevel);
					break;
				}

				if (raidrec->plex[i].raidlevel == -1) {
					asprintf(&chunksize, "N/A");
				} else {
					asprintf(&chunksize, "%dk", raidrec->plex[i].stripesize);
				}
				asprintf(&pname, "%s.p%i", raidrec->volname, i);
				asprintf(&entry, "%-14s %-8s  %11s  %8d",
						 pname, raidlevel, chunksize,
						 raidrec->plex[i].subdisks);
				paranoid_free(pname);
				paranoid_free(chunksize);
				paranoid_free(raidlevel);
				newtListboxAppendEntry(plexesListbox, entry, keylist[i]);
				paranoid_free(entry);
			}
		}

		newtFormAddComponents(editraidForm, bOK, bCancel, bAdd, bEdit,
							  bDelete, plexesListbox, plexesHeader, NULL);

		b_res = newtRunForm(editraidForm);
		if (b_res == bOK || b_res == bCancel) {
			break;
		}

		curr_choice = newtListboxGetCurrent(plexesListbox);
		for (currline2 = 0; currline2 < raidrec->plexes; ++currline2) {
			if (currline2 > 9)
				break;
			if (keylist[currline2] == curr_choice)
				break;
		}

		if (b_res == bDelete) {
			asprintf(&msg, _("Are you sure you want to delete %s.p%i?"),
					raidrec->volname, currline2);
			if (ask_me_yes_or_no(msg)) {
				log_it(_("Deleting RAID plex"));
				memcpy((void *) &raidrec->plex[currline2],
					   (void *) &raidrec->plex[raidrec->plexes - 1],
					   sizeof(struct vinum_plex));
				raidrec->plexes--;
			}
			paranoid_free(msg);
			continue;
		}
		if (b_res == bAdd) {
			raidrec->plex[raidrec->plexes].raidlevel = 0;
			raidrec->plex[raidrec->plexes].stripesize = 279;
			raidrec->plex[raidrec->plexes].subdisks = 0;
			currline2 = raidrec->plexes++;
		}
		edit_raidlist_plex(mountlist, raidlist, &raidrec->plex[currline2],
						   currline, currline2);
		newtFormDestroy(editraidForm);
	}
	if (b_res == bCancel) {
		memcpy((void *) raidrec, (void *) &bkp_raidrec,
			   sizeof(struct vinum_volume));
	}
	newtPopHelpLine();
	newtPopWindow();
	mountlist->el[currline].size =
		calculate_raid_device_size(mountlist, raidlist, raidrec->volname);
#else
	/** structures ********************************************************/
	struct raid_device_record *bkp_raidrec = NULL;


	/** buffers ***********************************************************/
	char *title_of_editraidForm_window = NULL;
	char *sz_raid_level = NULL;
	char *sz_data_disks = NULL;
	char *sz_spare_disks = NULL;
	char *sz_parity_disks = NULL;
	char *sz_failed_disks = NULL;

	/** newt **************************************************************/
	newtComponent editraidForm;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent bAdditional;
	newtComponent bChangeRaid;
	newtComponent bSelectData;
	newtComponent bSelectSpare;
	newtComponent bSelectParity;
	newtComponent bSelectFailed;
	newtComponent b_res;

	assert(mountlist != NULL);
	assert(raidlist != NULL);
	assert(raidrec != NULL);

	if (!(bkp_raidrec = malloc(sizeof(struct raid_device_record)))) {
		fatal_error("Cannot malloc space for raidrec");
	}

	log_it("Started edit_raidlist_entry");

	memcpy((void *) bkp_raidrec, (void *) raidrec,
		   sizeof(struct raid_device_record));
	asprintf(&title_of_editraidForm_window, _("Edit %s"), raidrec->raid_device);
	log_msg(2, "Opening newt window");
	newtOpenWindow(20, 5, 40, 14, title_of_editraidForm_window);
	paranoid_free(title_of_editraidForm_window);

	for (;;) {
		log_msg(2, "Main loop");
		asprintf(&sz_raid_level,
			   turn_raid_level_number_to_string(raidrec->raid_level));
		asprintf(&sz_data_disks,
			   number_of_disks_as_string(raidrec->data_disks.entries,
										 _("data")));
		asprintf(&sz_spare_disks,
			   number_of_disks_as_string(raidrec->spare_disks.entries,
										 _("spare")));
		asprintf(&sz_parity_disks,
			   number_of_disks_as_string(raidrec->parity_disks.entries,
										 _("parity")));
		asprintf(&sz_failed_disks,
			   number_of_disks_as_string(raidrec->failed_disks.entries,
										 _("failed")));
		bSelectData = newtButton(1, 1, sz_data_disks);
		bSelectSpare = newtButton(20, 1, sz_spare_disks);
		bSelectParity = newtButton(1, 5, sz_parity_disks);
		bSelectFailed = newtButton(20, 5, sz_failed_disks);
		bChangeRaid = newtButton(1, 9, sz_raid_level);
		paranoid_free(sz_raid_level);
		paranoid_free(sz_data_disks);
		paranoid_free(sz_spare_disks);
		paranoid_free(sz_parity_disks);
		paranoid_free(sz_failed_disks);

		bOK = newtButton(16 + (raidrec->raid_level == -1), 9, _("  OK  "));
		bCancel = newtButton(28, 9, _("Cancel"));
		bAdditional =
			newtCompactButton(1, 13,
							  _("Additional settings and information"));
		newtPushHelpLine
			(_
			 ("  Edit the RAID device's settings to your heart's content, then hit OK/Cancel."));
		editraidForm = newtForm(NULL, NULL, 0);
		newtFormAddComponents(editraidForm, bSelectData, bSelectParity,
							  bChangeRaid, bSelectSpare, bSelectFailed,
							  bOK, bCancel, bAdditional);
		b_res = newtRunForm(editraidForm);
		if (b_res == bChangeRaid) {
			choose_raid_level(raidrec);
		} else if (b_res == bSelectData) {
			select_raid_disks(mountlist, raidlist, raidrec, _("data"),
							  &raidrec->data_disks);
		} else if (b_res == bSelectSpare) {
			select_raid_disks(mountlist, raidlist, raidrec, _("spare"),
							  &raidrec->spare_disks);
		} else if (b_res == bSelectParity) {
			select_raid_disks(mountlist, raidlist, raidrec, _("parity"),
							  &raidrec->parity_disks);
		} else if (b_res == bSelectFailed) {
			select_raid_disks(mountlist, raidlist, raidrec, _("failed"),
							  &raidrec->failed_disks);
		} else if (b_res == bAdditional) {
			edit_raidrec_additional_vars(raidrec);
		}
		newtFormDestroy(editraidForm);
		if (b_res == bOK || b_res == bCancel) {
			break;
		}
	}
	if (b_res == bCancel) {
		memcpy((void *) raidrec, (void *) bkp_raidrec,
			   sizeof(struct raid_device_record));
	}
	newtPopHelpLine();
	newtPopWindow();
	mountlist->el[currline].size =
		calculate_raid_device_size(mountlist, raidlist,
								   raidrec->raid_device);
	paranoid_free(bkp_raidrec);
#endif
}
#ifdef __FreeBSD__


/**
 * Edit the plex @p raidrec in @p raidlist.
 * @param mountlist The mountlist to get some of the information from.
 * @param raidlist The raidlist containing information about RAID devices.
 * @param raidrec The plex to edit.
 * @param currline The line number (starting from 0) of the RAID device in @p mountlist.
 * @param currline2 The line number (starting from 0) of the plex within the RAID device.
 * @author Joshua Oreman
 * @ingroup restoreGuiMountlist
 */
void
edit_raidlist_plex(struct mountlist_itself *mountlist,
				   struct raidlist_itself *raidlist,
				   struct vinum_plex *raidrec, int currline, int currline2)
{

	/** structures ********************************************************/
	struct vinum_plex bkp_raidrec;


	/** buffers ***********************************************************/
	char *title_of_editraidForm_window = NULL;
	char *tmp = NULL;
	char *entry = NULL;

	/** newt **************************************************************/
	newtComponent editraidForm;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent bEdit;
	newtComponent bAdd;
	newtComponent bDelete;
	newtComponent b_res;
	newtComponent unallocListbox, allocListbox;
	newtComponent bLevel, sLevel;
	newtComponent bStripeSize, sStripeSize;
	newtComponent bAlloc, bUnalloc;

	void *keylist[ARBITRARY_MAXIMUM];
	void *curr_choice_a, *curr_choice_u;
	int currline_a, currline_u;
	int i;

	struct mountlist_itself *unallocparts = NULL;

	/* BERLIOS: Check return value */
	unallocparts = malloc(sizeof(struct mountlist_itself));

	log_it("Started edit_raidlist_entry");
	memcpy((void *) &bkp_raidrec, (void *) raidrec,
		   sizeof(struct vinum_plex));
	asprintf(&title_of_editraidForm_window, "%s.p%i",
			raidlist->el[currline].volname, currline2);
	newtPushHelpLine
		(_
		 ("   Please select a subdisk to edit, or edit this plex's parameters"));
	newtOpenWindow(13, 3, 54, 18, title_of_editraidForm_window);
	paranoid_free(title_of_editraidForm_window);

	for (;;) {
		switch (raidrec->raidlevel) {
		case -1:
			asprintf(&tmp, "concat");
			break;
		case 0:
			asprintf(&tmp, "striped");
			break;
		case 5:
			asprintf(&tmp, "raid5");
			break;
		default:
			asprintf(&tmp, _("unknown (%i)"), raidrec->raidlevel);
			break;
		}
		bLevel = newtCompactButton(2, 2, _(" RAID level "));
		sLevel = newtLabel(19, 2, tmp);
		paranoid_free(tmp);

		if (raidrec->raidlevel >= 0) {
			asprintf(&tmp, "%ik", raidrec->stripesize);
			bStripeSize = newtCompactButton(2, 4, _(" Stripe size "));
		} else {
			asprintf(&tmp, "N/A");
			bStripeSize = newtLabel(2, 4, _("Stripe size:"));
		}
		sStripeSize = newtLabel(19, 4, tmp);
		paranoid_free(tmp);

		bOK = newtCompactButton(2, 16, _("  OK  "));
		bCancel = newtCompactButton(12, 16, _("Cancel"));
		bAdd = newtCompactButton(22, 16, _(" Add "));
		bEdit = newtCompactButton(32, 16, _(" Edit "));
		bDelete = newtCompactButton(42, 16, _("Delete"));


		//  plexesListbox = newtListbox (2, 7, 9, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
		unallocListbox =
			newtListbox(2, 7, 7, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
		allocListbox =
			newtListbox(33, 7, 7, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
		bAlloc = newtButton(23, 7, " -> ");
		bUnalloc = newtButton(23, 11, " <- ");

		editraidForm = newtForm(NULL, NULL, 0);

		newtListboxClear(allocListbox);
		newtListboxClear(unallocListbox);
		bzero(unallocparts, sizeof(struct mountlist_itself));
		make_list_of_unallocated_raid_partitions(unallocparts, mountlist,
												 raidlist);
		for (i = 0; i < ARBITRARY_MAXIMUM; ++i) {
			keylist[i] = (void *) i;
			if (i < raidrec->subdisks) {
				asprintf(&entry, "%-17s",
						 find_dev_entry_for_raid_device_name(raidlist,
															 raidrec->
															 sd[i].
															 which_device));
				newtListboxAppendEntry(allocListbox, entry, keylist[i]);
				paranoid_free(entry);
			}
			if (i < unallocparts->entries) {
				asprintf(&entry, "%-17s", unallocparts->el[i].device);
				newtListboxAppendEntry(unallocListbox, entry, keylist[i]);
				paranoid_free(entry);
			}
		}

#define COMP(x)  newtFormAddComponent (editraidForm, x)
#define UCOMP(x) if (unallocparts->entries > 0) COMP(x)
#define ACOMP(x) if (raidrec->subdisks > 0) COMP(x)
		editraidForm = newtForm(NULL, NULL, 0);
		UCOMP(unallocListbox);
		UCOMP(bAlloc);
		ACOMP(allocListbox);
		ACOMP(bUnalloc);
		COMP(bOK);
		COMP(bCancel);
		COMP(bLevel);
		COMP(sLevel);
		if (raidrec->raidlevel != -1) {
			COMP(bStripeSize);
			COMP(sStripeSize);
		}
#undef COMP
#undef UCOMP
#undef ACOMP

		newtRefresh();
		b_res = newtRunForm(editraidForm);
		if (b_res == bOK || b_res == bCancel) {
			break;
		}

		curr_choice_a = (raidrec->subdisks > 0) ?
			newtListboxGetCurrent(allocListbox) : (void *) 1234;
		curr_choice_u = (unallocparts->entries > 0) ?
			newtListboxGetCurrent(unallocListbox) : (void *) 1234;
		for (currline_a = 0; currline_a < raidrec->subdisks; ++currline_a) {
			if (currline_a > ARBITRARY_MAXIMUM)
				break;
			if (keylist[currline_a] == curr_choice_a)
				break;
		}
		for (currline_u = 0; currline_u < unallocparts->entries;
			 ++currline_u) {
			if (currline_u > ARBITRARY_MAXIMUM)
				break;
			if (keylist[currline_u] == curr_choice_u)
				break;
		}
		if (b_res == bLevel) {
			choose_raid_level(raidrec);
		} else if (b_res == bStripeSize) {
			asprintf(&tmp, "%i", raidrec->stripesize);
			if (popup_and_get_string
				(_("Stripe size"),
				 _("Please enter the stripe size in kilobytes."), tmp)) {
				raidrec->stripesize = atoi(tmp);
			}
			paranoid_free(tmp);
		} else if ((b_res == bAlloc) || (b_res == unallocListbox)) {
			if (currline_u <= unallocparts->entries)
				add_raid_subdisk(raidlist, raidrec,
								 unallocparts->el[currline_u].device);
		} else if ((b_res == bUnalloc) || (b_res == allocListbox)) {
			if (currline_a <= raidrec->subdisks) {
				memcpy((void *) &raidrec->sd[currline_a],
					   (void *) &raidrec->sd[raidrec->subdisks - 1],
					   sizeof(struct vinum_subdisk));
				raidrec->subdisks--;
			}
		}
#if 0
	} else {
		edit_raid_subdisk(raidlist, raidrec, &raidrec->sd[currline3],
						  currline3);
	}
#endif
	newtFormDestroy(editraidForm);
	newtRefresh();
}

if (b_res == bCancel) {
	memcpy((void *) raidrec, (void *) &bkp_raidrec,
		   sizeof(struct vinum_plex));
}
newtPopWindow();
newtPopHelpLine();
}
#else
/**
 * Edit additional RAID variable number @p lino.
 * @param raidrec The RAID device record to edit the variable in.
 * @param lino The line number (starting from 0) of the variable to edit.
 * @ingroup restoreGuiVarslist
 */
void edit_varslist_entry(struct raid_device_record *raidrec, int lino)
{

	/** buffers ***********************************************************/
	char *header = NULL;
	char *comment = NULL;
	char *sz_out = NULL;

	assert(raidrec != 0);
	assert(lino >= 0);

	asprintf(&sz_out, raidrec->additional_vars.el[lino].value);
	asprintf(&header, _("Edit %s"), raidrec->additional_vars.el[lino].label);
	asprintf(&comment, _("Please set %s's value (currently '%s')"),
			raidrec->additional_vars.el[lino].label, sz_out);
	if (popup_and_get_string(header, comment, sz_out)) {
		strcpy(raidrec->additional_vars.el[lino].value, sz_out);
	}
	paranoid_free(header);
	paranoid_free(comment);
	paranoid_free(sz_out);
}
#endif


/**
 * Edit the mountlist using Newt.
 * @param mountlist The mountlist to edit.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @return 0 if the user pressed OK, 1 if they pressed Cancel.
 */
int
edit_mountlist_in_newt(char *mountlist_fname,
					   struct mountlist_itself *mountlist,
					   struct raidlist_itself *raidlist)
{

	/** newt **************************************************************/
	newtComponent myForm;
	newtComponent bAdd;
	newtComponent bEdit;
	newtComponent bDelete;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent b_res = NULL;
	newtComponent partitionsListbox;
	newtComponent headerMsg;
	newtComponent flawsLabelA;
	newtComponent flawsLabelB;
	newtComponent flawsLabelC;
	newtComponent bReload;

	/** ???? *************************************************************/
	void *curr_choice = NULL;
	void *keylist[ARBITRARY_MAXIMUM];

	/** int **************************************************************/
	int i = 0;
	int currline = 0;
	int finished = FALSE;

	/** buffers **********************************************************/
	char *tmp = NULL;
	char *flaws_str_A = NULL;
	char *flaws_str_B = NULL;
	char *flaws_str_C = NULL;

	assert(mountlist != NULL);
	assert(raidlist != NULL);

	asprintf(&flaws_str_A, "xxxxxxxxx");
	asprintf(&flaws_str_B, "xxxxxxxxx");
	asprintf(&flaws_str_C, "xxxxxxxxx");
	if (mountlist->entries > ARBITRARY_MAXIMUM) {
		log_to_screen(_("Arbitrary limits suck, man!"));
		finish(1);
	}
	newtPushHelpLine
		(_
		 ("   Please edit the mountlist to your satisfaction, then click OK or Cancel."));
	i = 4;
	bAdd = newtCompactButton(i, 17, _(" Add "));
	bEdit = newtCompactButton(i += 11, 17, _(" Edit "));
	bDelete = newtCompactButton(i += 12, 17, _("Delete"));
	bReload = newtCompactButton(i += 12, 17, _("Reload"));
	bCancel = newtCompactButton(i += 12, 17, _("Cancel"));
	bOK = newtCompactButton(i += 12, 17, _("  OK  "));
	asprintf(&tmp, "%-24s %-24s %-8s  %s", _("Device"), _("Mountpoint"),
			_("Format"), _("Size (MB)"));
	headerMsg = newtLabel(2, 1, tmp);
	flawsLabelA = newtLabel(2, 13, flaws_str_A);
	flawsLabelB = newtLabel(2, 14, flaws_str_B);
	flawsLabelC = newtLabel(2, 15, flaws_str_C);
	partitionsListbox =
		newtListbox(2, 2, 10, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
	redraw_mountlist(mountlist, keylist, partitionsListbox);
	newtOpenWindow(1, 3, 77, 18, _("Editing mountlist"));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, headerMsg, partitionsListbox,
						  flawsLabelA, flawsLabelB, flawsLabelC, bAdd,
						  bEdit, bDelete, bReload, bCancel, bOK, NULL);
	while (!finished) {
		evaluate_mountlist(mountlist, flaws_str_A, flaws_str_B,
						   flaws_str_C);
		newtLabelSetText(flawsLabelA, flaws_str_A);
		newtLabelSetText(flawsLabelB, flaws_str_B);
		newtLabelSetText(flawsLabelC, flaws_str_C);
		b_res = newtRunForm(myForm);
		/* BERLIOS: This needs to be re-written */
		if (b_res == bOK) {
			if (!evaluate_mountlist
				(mountlist, flaws_str_A, flaws_str_B, flaws_str_C)) {
				finished =
					ask_me_yes_or_no
					(_("Your mountlist might not work. Continue anyway?"));
			} else {
				finished =
					ask_me_yes_or_no
					(_
					 ("Are you sure you want to save your mountlist and continue? (No changes will be made to your partition table at this time.)"));
			}
		} else if (b_res == bCancel) {
			finished = TRUE;
		} else if (b_res == bReload) {
			if (ask_me_yes_or_no(_("Reload original mountlist?"))) {
				load_mountlist(mountlist, mountlist_fname);
				load_raidtab_into_raidlist(raidlist, RAIDTAB_FNAME);
				redraw_mountlist(mountlist, keylist, partitionsListbox);
			}
		} else {
			curr_choice = newtListboxGetCurrent(partitionsListbox);
			for (i = 0;
				 i < mountlist->entries && keylist[i] != curr_choice; i++);
			if (i == mountlist->entries && mountlist->entries > 0) {
				log_to_screen(_("I don't know what that button does!"));
			} else {
				currline = i;
				if (b_res == bAdd) {
					add_mountlist_entry(mountlist, raidlist,
										partitionsListbox, currline,
										keylist);
				} else if (b_res == bDelete) {
					delete_mountlist_entry(mountlist, raidlist,
										   partitionsListbox, currline,
										   keylist);
				} else {
					if (mountlist->entries > 0) {
						edit_mountlist_entry(mountlist, raidlist,
											 partitionsListbox, currline,
											 keylist);
					} else {
						popup_and_OK
							(_
							 ("Please add an entry. Then press ENTER to edit it."));
					}
				}
			}
		}
		paranoid_free(flaws_str_A);
		paranoid_free(flaws_str_B);
		paranoid_free(flaws_str_C);
	}
	newtFormDestroy(myForm);
	newtPopWindow();
	newtPopHelpLine();
	if (b_res == bOK) {
		log_it(_("You pushed 'OK'. I shall now continue."));
		return (0);
	} else {
		/* popup_and_OK("You pushed 'cancel'. I shall now abort."); */
		return (1);
	}
}


/**
 * Edit the mountlist.
 * @param mountlist The mountlist to edit.
 * @param raidlist The raidlist that goes with @p mountlist.
 * @return 0 if the user pressed OK, 1 if they pressed Cancel.
 */
int
edit_mountlist(char *mountlist_fname, struct mountlist_itself *mountlist,
			   struct raidlist_itself *raidlist)
{
	int res = 0;

	iamhere("entering eml");

	if (g_text_mode) {
		fatal_error("Don't call edit_mountlist() in text mode");
	} else {
		log_it
			("I'm in GUI mode, so I shall edit mountlist using edit_mountlist()");
		res = edit_mountlist_in_newt(mountlist_fname, mountlist, raidlist);
	}
	iamhere("leaving eml");
	return (res);
}


#ifndef __FreeBSD__
/**
 * Edit the additional RAID variables in @p raidrec.
 * @param raidrec The RAID device record to edit the RAID variables in.
 * @ingroup restoreGuiVarslist
 */
void edit_raidrec_additional_vars(struct raid_device_record *raidrec)
{

	/** structure *********************************************************/
	struct raid_device_record bkp_raidrec;

	/** newt **************************************************************/
	newtComponent myForm;
	newtComponent bAdd;
	newtComponent bEdit;
	newtComponent bDelete;
	newtComponent bOK;
	newtComponent bCancel;
	newtComponent b_res;
	newtComponent varsListbox;
	newtComponent headerMsg;

	/** ?? ***************************************************************/
	void *keylist[ARBITRARY_MAXIMUM], *curr_choice = NULL;

	/** buffers **********************************************************/
	char *title_of_window = NULL;

	/** int **************************************************************/
	int i = 0;
	int currline = 0;


	assert(raidrec != NULL);

	memcpy((void *) &bkp_raidrec, (void *) raidrec,
		   sizeof(struct raid_device_record));
	asprintf(&title_of_window, "Additional variables");
	newtPushHelpLine
		(_
		 ("  Edit the additional fields to your heart's content, then click OK or Cancel."));
	headerMsg =
		newtLabel(1, 1, _("Label                            Value"));
	varsListbox =
		newtListbox(1, 2, 6, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
	i = 1;
	bAdd = newtCompactButton(i, 9, _(" Add "));
	bEdit = newtCompactButton(i += 8, 9, _(" Edit "));
	bDelete = newtCompactButton(i += 9, 9, _("Delete"));
	bOK = newtCompactButton(i += 9, 9, _("  OK  "));
	bCancel = newtCompactButton(i += 9, 9, _("Cancel"));
	newtOpenWindow(17, 7, 46, 10, title_of_window);
	paranoid_free(title_of_window);

	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, headerMsg, varsListbox, bAdd, bEdit,
						  bDelete, bOK, bCancel, NULL);
	insert_essential_additionalvars(raidrec);
	redraw_varslist(&raidrec->additional_vars, keylist, varsListbox);
	for (b_res = NULL; b_res != bOK && b_res != bCancel;) {
		b_res = newtRunForm(myForm);
		curr_choice = newtListboxGetCurrent(varsListbox);
		for (currline = 0;
			 currline < raidrec->additional_vars.entries
			 && keylist[currline] != curr_choice; currline++);
		if (currline == raidrec->additional_vars.entries
			&& raidrec->additional_vars.entries > 0) {
			log_it("Warning - I don't know what this button does");
		}
		if (b_res == bOK) {		/* do nothing */
		} else if (b_res == bCancel) {	/* do nothing */
		} else if (b_res == bAdd) {
			add_varslist_entry(raidrec);
		} else if (b_res == bDelete) {
			delete_varslist_entry(raidrec, currline);
		} else {
			edit_varslist_entry(raidrec, currline);
		}
		redraw_varslist(&raidrec->additional_vars, keylist, varsListbox);
	}
	remove_essential_additionalvars(raidrec);
	newtFormDestroy(myForm);
	newtPopWindow();
	newtPopHelpLine();
	if (b_res == bCancel) {
		memcpy((void *) raidrec, (void *) &bkp_raidrec,
			   sizeof(struct raid_device_record));
	}
	return;
}
#endif


/**
 * Find the next free location to place a disk in @p disklist.
 * @param disklist The disklist to operate on.
 * @return The next free location (starting from 0).
 * @ingroup restoreGuiDisklist
 */
int find_next_free_index_in_disklist(struct list_of_disks *disklist)
{

	/** int ***************************************************************/
	int index = -1;
	int pos = 0;

  /** bool **************************************************************/
	bool done;

	assert(disklist != NULL);

	for (done = FALSE; !done;) {
		for (pos = 0;
			 pos < disklist->entries && disklist->el[pos].index <= index;
			 pos++);
		if (pos >= disklist->entries) {
			done = TRUE;
		} else {
			index = disklist->el[pos].index;
		}
	}
	return (index + 1);
}


/**
 * Locate @p device in @p raidlist.
 * @param raidlist The raidlist ot search in.
 * @param device The RAID device to search for.
 * @return The index of the device, or -1 if it could not be found.
 * @ingroup restoreGuiMountlist
 */
int
find_raid_device_in_raidlist(struct raidlist_itself *raidlist,
							 char *device)
{
	/** int ***************************************************************/
	int i = 0;
#ifdef __FreeBSD__
	char *vdev = NULL;
#endif

	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(device);

#ifdef __FreeBSD__
	for (i = 0; i < raidlist->entries; i++) {
		asprintf(&vdev, "/dev/vinum/%s", raidlist->el[i].volname);
		if (!strcmp(device, vdev)) {
			paranoid_free(vdev);
			break;
		}
		paranoid_free(vdev);
	}
#else

	for (i = 0; strcmp(raidlist->el[i].raid_device, device)
		 && i < raidlist->entries; i++);
#endif
	if (i == raidlist->entries) {
		return (-1);
	} else {
		return (i);
	}
}


/**
 * Get information about the location of ISO images from the user.
 * @param isodir_device Where to put the device (e.g. /dev/hda4) the user enters.
 * @param isodir_format Where to put the format (e.g. ext2) the user enters.
 * @param isodir_path Where to put the path (e.g. /var/cache/mondo) the user enters.
 * @param nuke_me_please Whether we're planning on nuking or not.
 * @return TRUE if OK was pressed, FALSE otherwise.
 */
bool
get_isodir_info(char *isodir_device, char *isodir_format,
				char *isodir_path, bool nuke_me_please)
{

	/** initialize ********************************************************/

	// %d no var ???
	// log_it("%d - AAA - isodir_path = %s", isodir_path);
	if (isodir_device == NULL) {
		asprintf(&isodir_device, "/dev/");
	}
	if (isodir_path == NULL) {
		asprintf(&isodir_path, "/");
	}
	if (does_file_exist("/tmp/NFS-SERVER-PATH")) {
		paranoid_free(isodir_device);
		isodir_device = last_line_of_file("/tmp/NFS-SERVER-MOUNT");
		asprintf(&isodir_format, "nfs");
		paranoid_free(isodir_path);
		isodir_path = last_line_of_file("/tmp/NFS-SERVER-PATH");
	}
	if (nuke_me_please) {
		return (TRUE);
	}

	if (popup_and_get_string
		(_("ISO Mode - device"),
		 _("On what device do the ISO files live?"), isodir_device)) {
		if (popup_and_get_string
			(_("ISO Mode - format"),
			 _
			 ("What is the disk format of the device? (Hit ENTER if you don't know.)"),
			 isodir_format)) {
			if (popup_and_get_string
				(_("ISO Mode - path"),
				 _
				 ("At what path on this device can the ISO files be found?"),
				 isodir_path)) {
				// Same pb:
				// log_it("%d - BBB - isodir_path = %s", isodir_path);
				return (TRUE);
			}
		}
	}
	return (FALSE);
}


/**
 * Create a new raidtab entry for @p device in @p raidlist.
 * @param raidlist The raidlist to add the device to.
 * @param mountlist The mountlist containing information about the user's partitions.
 * @param currline The selected line in the mountlist.
 * @param device The RAID device (e.g. /dev/md0) to use.
 * @ingroup restoreGuiMountlist
 */
void
initiate_new_raidlist_entry(struct raidlist_itself *raidlist,
							struct mountlist_itself *mountlist,
							int currline, char *device)
{
	/** structure *********************************************************/
	struct OSSWAP (raid_device_record, vinum_volume) * raidrec;

	/** int ***************************************************************/
	int pos_in_raidlist = 0;

	assert(raidlist != NULL);
	assert(mountlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(device);

	pos_in_raidlist =
		find_raid_device_in_raidlist(raidlist,
									 mountlist->el[currline].device);
	if (pos_in_raidlist >= 0) {
		fatal_error("Sorry, that RAID device already exists. Weird.");
	}
	pos_in_raidlist = raidlist->entries++;
	raidrec = &raidlist->el[pos_in_raidlist];
	initialize_raidrec(raidrec);
	strcpy(raidrec->OSSWAP(raid_device, volname),
		   OSSWAP(device, basename(device)));
#ifndef __FreeBSD__
	choose_raid_level(raidrec);
	select_raid_disks(mountlist, raidlist, raidrec, "data",
					  &raidrec->data_disks);
#endif
	edit_raidlist_entry(mountlist, raidlist, raidrec, currline);
}


#ifndef __FreeBSD__
/**
 * Insert the RAID variables not stored in the "additional RAID variables" list there too.
 * @param raidrec The RAID device record to operate on.
 * @ingroup restoreGuiVarslist
 */
void insert_essential_additionalvars(struct raid_device_record *raidrec)
{

	/** int **************************************************************/
	int items = 0;

	assert(raidrec != NULL);

	items = raidrec->additional_vars.entries;
	write_variableINT_to_raid_var_line(raidrec, items++,
									   "persistent-superblock",
									   raidrec->persistent_superblock);
	write_variableINT_to_raid_var_line(raidrec, items++, "chunk-size",
									   raidrec->chunk_size);
	raidrec->additional_vars.entries = items;
}
#endif


/**
 * Dummy function that proves that we can get to the point where Mondo is run.
 */
void nuke_mode_dummy()
{

	/** newt *************************************************************/
	newtComponent myForm;
	newtComponent b1;
	newtComponent b2;
	newtComponent b3;
	newtComponent b_res;


	newtPushHelpLine
		(_
		 ("This is where I nuke your hard drives. Mhahahahaha. No-one can stop Mojo Jojo!"));
	newtOpenWindow(24, 3, 32, 13, _("Nuking"));
	b1 = newtButton(7, 1, _("Slowly"));
	b2 = newtButton(7, 5, _("Medium"));
	b3 = newtButton(7, 9, _("Quickly"));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, b1, b2, b3, NULL);
	b_res = newtRunForm(myForm);
	newtFormDestroy(myForm);
	newtPopWindow();
	newtPopHelpLine();
}


/**
 * Redraw the disklist.
 * @param disklist The disklist to read from.
 * @param keylist The list of keys for @p listbox.
 * @param listbox The Newt listbox component to redraw.
 * @ingroup restoreGuiDisklist
 */
void
redraw_disklist(struct list_of_disks *disklist,
				void *keylist[ARBITRARY_MAXIMUM], newtComponent listbox)
{

	/** int *************************************************************/
	int i = 0;

	assert(disklist != NULL);
	assert(keylist != NULL);
	assert(listbox != NULL);

	newtListboxClear(listbox);

	for (i = 0; i < ARBITRARY_MAXIMUM; i++) {
		keylist[i] = (void *) i;
	}
	for (i = 0; i < disklist->entries; i++) {
		newtListboxAppendEntry(listbox,
							   disklist_entry_to_string(disklist, i),
							   keylist[i]);
	}
}


/**
 * Redraw the mountlist.
 * @param mountlist The mountlist to read from.
 * @param keylist The list of keys for @p listbox.
 * @param listbox The Newt listbox component to redraw.
 * @ingroup restoreGuiMountlist
 */
void
redraw_mountlist(struct mountlist_itself *mountlist,
				 void *keylist[ARBITRARY_MAXIMUM], newtComponent listbox)
{

	/** int **************************************************************/
	int i = 0;

	assert(mountlist != NULL);
	assert(keylist != NULL);
	assert(listbox != NULL);

	newtListboxClear(listbox);
//  sort_mountlist_by_device (mountlist);
	for (i = 0; i < ARBITRARY_MAXIMUM; i++) {
		keylist[i] = (void *) i;
	}
	for (i = 0; i < mountlist->entries; i++) {
		newtListboxAppendEntry(listbox,
							   mountlist_entry_to_string(mountlist, i),
							   keylist[i]);
	}
}


/**
 * Redraw the list of unallocated RAID partitions.
 * @param unallocated_raid_partitions The mountlist containing unallocated RAID partitions.
 * @param keylist The list of keys for @p listbox.
 * @param listbox The Newt listbox component to redraw.
 * @ingroup restoreGuiDisklist
 */
void redraw_unallocpartnslist(struct mountlist_itself
							  *unallocated_raid_partitions,
							  void *keylist[ARBITRARY_MAXIMUM],
							  newtComponent listbox)
{

	/** int **************************************************************/
	int i = 0;

	/** buffers **********************************************************/
	char *tmp = NULL;

	assert(unallocated_raid_partitions != NULL);
	assert(keylist != NULL);
	assert(listbox != NULL);

	newtListboxClear(listbox);
	for (i = 0; i < ARBITRARY_MAXIMUM; i++) {
		keylist[i] = (void *) i;
	}
	for (i = 0; i < unallocated_raid_partitions->entries; i++) {
		asprintf(&tmp, "%-22s %8lld",
				unallocated_raid_partitions->el[i].device,
				unallocated_raid_partitions->el[i].size / 1024);
		newtListboxAppendEntry(listbox, tmp, keylist[i]);
		paranoid_free(tmp);
	}
}


#ifndef __FreeBSD__
/**
 * Redraw the list of additional RAID variables.
 * @param additional_vars The list of additional RAID varibals.
 * @param keylist The list of keys for @p listbox.
 * @param listbox The Newt listbox component to redraw.
 * @ingroup restoreGuiVarslist
 */
void
redraw_varslist(struct additional_raid_variables *additional_vars,
				void *keylist[], newtComponent listbox)
{
	/** int *************************************************************/
	int i = 0;

	/** buffers *********************************************************/
	char *tmp;

	assert(additional_vars != NULL);
	assert(keylist != NULL);
	assert(listbox != NULL);

	newtListboxClear(listbox);

	for (i = 0; i < ARBITRARY_MAXIMUM; i++) {
		keylist[i] = (void *) i;
	}
	for (i = 0; i < additional_vars->entries; i++) {
		asprintf(&tmp, "%-32s %-8s", additional_vars->el[i].label,
				additional_vars->el[i].value);
		newtListboxAppendEntry(listbox, tmp, keylist[i]);
		paranoid_free(tmp);
	}
}


/**
 * Remove variable @p label from the RAID variables list in @p raidrec.
 * @param raidrec The RAID device record to remove the variable from.
 * @param label The variable name to remove.
 * @return The value of the variable removed.
 * @ingroup restoreUtilityGroup
 */
int read_variableINT_and_remove_from_raidvars(struct
											  OSSWAP (raid_device_record,
													  vinum_volume) *
											  raidrec, char *label)
{
	/** int ***************************************************************/
	int i = 0;
	int res = 0;


	assert(raidrec != NULL);
	assert(label != NULL);

	for (i = 0;
		 i < raidrec->additional_vars.entries
		 && strcmp(raidrec->additional_vars.el[i].label, label); i++);
	if (i == raidrec->additional_vars.entries) {
		res = -1;
	} else {
		res = atoi(raidrec->additional_vars.el[i].value);
		for (i++; i < raidrec->additional_vars.entries; i++) {
			memcpy((void *) &raidrec->additional_vars.el[i - 1],
				   (void *) &raidrec->additional_vars.el[i],
				   sizeof(struct raid_var_line));
		}
		raidrec->additional_vars.entries--;
	}
	return (res);
}
#endif


/**
 * Change all RAID devices to use @p new_dev instead of @p old_dev.
 * @param raidlist The raidlist to make the changes in.
 * @param old_dev The old name of the device (what it used to be).
 * @param new_dev The new name of the device (what it is now).
 * @ingroup restoreGuiMountlist
 */
void rejig_partition_name_in_raidlist_if_necessary(struct raidlist_itself
												   *raidlist,
												   char *old_dev,
												   char *new_dev)
{
	/** buffers ********************************************************/
	char *tmp;

	/** int ************************************************************/
	int pos = 0;
	int j = 0;

	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(old_dev);
	assert_string_is_neither_NULL_nor_zerolength(new_dev);

	pos = which_raid_device_is_using_this_partition(raidlist, old_dev);
	if (pos < 0) {
		asprintf(&tmp, "No need to rejig %s in raidlist: it's not listed.",
				old_dev);
		log_it(tmp);
		paranoid_free(tmp);
	} else {
		if ((j =
			 where_in_drivelist_is_drive(&raidlist->
										 OSSWAP(el[pos].data_disks, disks),
										 old_dev)) >= 0) {
			strcpy(raidlist->OSSWAP(el[pos].data_disks, disks).el[j].
				   device, new_dev);
		} else
			if ((j =
				 where_in_drivelist_is_drive(&raidlist->
											 OSSWAP(el[pos].spare_disks,
													spares),
											 old_dev)) >= 0) {
			strcpy(raidlist->OSSWAP(el[pos].spare_disks, spares).el[j].
				   device, new_dev);
		}
#ifndef __FreeBSD__
		else if ((j =
				  where_in_drivelist_is_drive(&raidlist->el[pos].
											  parity_disks,
											  old_dev)) >= 0) {
			strcpy(raidlist->el[pos].parity_disks.el[j].device, new_dev);
		} else
			if ((j =
				 where_in_drivelist_is_drive(&raidlist->el[pos].
											 failed_disks,
											 old_dev)) >= 0) {
			strcpy(raidlist->el[pos].failed_disks.el[j].device, new_dev);
		}
#endif
		else {
			asprintf(&tmp,
					"%s is supposed to be listed in this raid dev but it's not...",
					old_dev);
			log_it(tmp);
			paranoid_free(tmp);
		}
	}
}


#ifndef __FreeBSD__
/**
 * Remove the essential RAID variables from the "additional variables" section.
 * If they have been changed, set them in their normal locations too.
 * @param raidrec The RAID device record to operate on.
 * @ingroup restoreUtilityVarslist
 */
void remove_essential_additionalvars(struct raid_device_record *raidrec)
{

	/** int **************************************************************/
	int res = 0;

	assert(raidrec != NULL);

	res =
		read_variableINT_and_remove_from_raidvars(raidrec,
												  "persistent-superblock");
	if (res > 0) {
		raidrec->persistent_superblock = res;
	}
	res = read_variableINT_and_remove_from_raidvars(raidrec, "chunk-size");
	if (res > 0) {
		raidrec->chunk_size = res;
	}
	res = read_variableINT_and_remove_from_raidvars(raidrec, "block-size");
}


/**
 * Select the RAID disks to use in @p raidrec.
 * @param mountlist_dontedit The mountlist (will not be edited).
 * @param raidlist The raidlist to modify.
 * @param raidrec The RAID device record in @p raidlist to work on.
 * @param description_of_list The type of disks we're selecting (e.g. "data").
 * @param disklist The disklist to put the user-selected disks in.
 * @ingroup restoreGuiMountlist
 */
void
select_raid_disks(struct mountlist_itself *mountlist_dontedit,
				  struct raidlist_itself *raidlist,
				  struct raid_device_record *raidrec,
				  char *description_of_list,
				  struct list_of_disks *disklist)
{
	void *curr_choice = NULL;

	/** structures ********************************************************/
	struct raidlist_itself *bkp_raidlist = NULL;
	struct raid_device_record *bkp_raidrec = NULL;
	struct list_of_disks *bkp_disklist = NULL;
	struct mountlist_itself *unallocated_raid_partitions = NULL;

	/** newt **************************************************************/
	newtComponent myForm = NULL;
	newtComponent bAdd = NULL;
	newtComponent bDelete = NULL;
	newtComponent bOK = NULL;
	newtComponent bCancel = NULL;
	newtComponent b_res = NULL;
	newtComponent partitionsListbox = NULL;
	newtComponent headerMsg = NULL;

	/** buffers **********************************************************/
	void *keylist[ARBITRARY_MAXIMUM];
	char *tmp = NULL;
	char *help_text = NULL;
	char *title_of_window = NULL;
	char *sz_res = NULL;
	char *header_text = NULL;

  /** int **************************************************************/
	int i = 0;
	int currline = 0;

	assert(mountlist_dontedit != NULL);
	assert(raidlist != NULL);
	assert(raidrec != NULL);
	assert(description_of_list != NULL);
	assert(disklist != NULL);

	iamhere("malloc'ing");
	if (!(bkp_raidrec = malloc(sizeof(struct raid_device_record)))) {
		fatal_error("Cannot malloc space for raidrec");
	}
	if (!(bkp_disklist = malloc(sizeof(struct list_of_disks)))) {
		fatal_error("Cannot malloc space for disklist");
	}
	if (!(bkp_raidlist = malloc(sizeof(struct raidlist_itself)))) {
		fatal_error("Cannot malloc space for raidlist");
	}
	if (!
		(unallocated_raid_partitions =
		 malloc(sizeof(struct mountlist_itself)))) {
		fatal_error("Cannot malloc space for unallocated_raid_partitions");
	}

	memcpy((void *) bkp_raidlist, (void *) raidlist,
		   sizeof(struct raidlist_itself));
	memcpy((void *) bkp_raidrec, (void *) raidrec,
		   sizeof(struct raid_device_record));
	memcpy((void *) bkp_disklist, (void *) disklist,
		   sizeof(struct list_of_disks));

	iamhere("Post-malloc");
	asprintf(&help_text,
		   _
		   ("   Edit this RAID device's list of partitions. Choose OK or Cancel when done."));
	asprintf(&header_text, "%-24s    %s", _("Device"), _("Index"));
	asprintf(&title_of_window, _("%s contains..."), raidrec->raid_device);
	newtPushHelpLine(help_text);
	paranoid_free(help_text);
	for (b_res = (newtComponent) 12345; b_res != bOK && b_res != bCancel;) {
		headerMsg = newtLabel(1, 1, header_text);
		partitionsListbox =
			newtListbox(1, 2, 6, NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
		redraw_disklist(disklist, keylist, partitionsListbox);
		i = 1;
		bAdd = newtCompactButton(i, 9, _(" Add "));
		bDelete = newtCompactButton(i += 8, 9, _("Delete"));
		bOK = newtCompactButton(i += 9, 9, _("  OK  "));
		bCancel = newtCompactButton(i += 9, 9, _("Cancel"));
		newtOpenWindow(21, 7, 38, 10, title_of_window);
		myForm = newtForm(NULL, NULL, 0);
		if (disklist->entries == 0) {
			newtFormAddComponents(myForm, headerMsg, bAdd, bDelete, bOK,
								  bCancel, NULL);
		} else {
			newtFormAddComponents(myForm, headerMsg, partitionsListbox,
								  bAdd, bDelete, bOK, bCancel, NULL);
		}
		b_res = newtRunForm(myForm);
		if (b_res == bOK || b_res == bCancel) {	/* do nothing */
// That's OK. At the end of this subroutine (after this do/while loop),
// we'll throw away the changes if Cancel was pushed.
		} else {
			curr_choice = newtListboxGetCurrent(partitionsListbox);
			for (i = 0; i < disklist->entries && keylist[i] != curr_choice;
				 i++);
			if (i == disklist->entries && disklist->entries > 0) {
				log_to_screen(_("I don't know what that button does!"));
			} else {
				currline = i;
				if (b_res == bAdd) {
					log_it(_("Making list of unallocated RAID slices"));
					make_list_of_unallocated_raid_partitions
						(unallocated_raid_partitions, mountlist_dontedit,
						 raidlist);
					if (unallocated_raid_partitions->entries <= 0) {
						popup_and_OK
							(_
							 ("There are no unallocated partitions marked for RAID."));
					} else {
						log_it
							(_
							 ("Done. The user may add one or more of the above to RAID device"));
						add_disklist_entry(disklist, raidrec->raid_device,
										   unallocated_raid_partitions);
						log_it(_
							   ("I have finished adding a disklist entry."));
						redraw_disklist(disklist, keylist,
										partitionsListbox);
					}
				} else if (b_res == bDelete) {
					delete_disklist_entry(disklist, raidrec->raid_device,
										  currline);
					redraw_disklist(disklist, keylist, partitionsListbox);
				} else {
					asprintf(&tmp, _("%s's index is %d. What should it be?"),
							raidrec->raid_device,
							disklist->el[currline].index);
					asprintf(&sz_res, "%d", disklist->el[currline].index);
					if (popup_and_get_string
						(_("Set index"), tmp, sz_res)) {
						disklist->el[currline].index = atoi(sz_res);
					}
					paranoid_free(tmp);
					paranoid_free(sz_res);
					redraw_disklist(disklist, keylist, partitionsListbox);
				}
			}
		}
		newtFormDestroy(myForm);
		newtPopWindow();
	}
	paranoid_free(title_of_window);
	paranoid_free(header_text);

	newtPopHelpLine();
	if (b_res == bCancel) {
		memcpy((void *) raidlist, (void *) bkp_raidlist,
			   sizeof(struct raidlist_itself));
		memcpy((void *) raidrec, (void *) bkp_raidrec,
			   sizeof(struct raid_device_record));
		memcpy((void *) disklist, (void *) bkp_disklist,
			   sizeof(struct list_of_disks));
	}
	paranoid_free(bkp_raidrec);
	paranoid_free(bkp_disklist);
	paranoid_free(bkp_raidlist);
	paranoid_free(unallocated_raid_partitions);
}
#endif


/**
 * Ask the user which restore mode (nuke, interactive, or compare) we should use.
 * @return The mode selected: 'I' for interactive, 'N' for nuke, 'C' for compare,
 * or 'E' (or any other letter) for exit.
 */
char which_restore_mode()
{

  /** char *************************************************************/
	char output = '\0';
	char *tmp = NULL;
	size_t n = 0;

  /** newt *************************************************************/

	newtComponent b1;
	newtComponent b2;
	newtComponent b3;
	newtComponent b4;
	newtComponent b_res;
	newtComponent myForm;

	if (g_text_mode) {
		for (output = 'z'; !strchr("AICE", output); output = tmp[0]) {
			printf
				(_
				 ("Which mode - (A)utomatic, (I)nteractive, \n(C)ompare only, or (E)xit to shell?\n--> "));
			getline(&tmp, &n, stdin);
		}
		paranoid_free(tmp);
		return (output);
	}

	newtPushHelpLine
		(_
		 ("   Do you want to 'nuke' your system, restore interactively, or just compare?"));
	newtOpenWindow(24, 3, 32, 17, _("How should I restore?"));
	b1 = newtButton(7, 1, _("Automatically"));
	b2 = newtButton(7, 5, _("Interactively"));
	b3 = newtButton(7, 9, _("Compare only!"));
	b4 = newtButton(7, 13, _("Exit to shell"));
	myForm = newtForm(NULL, NULL, 0);
	newtFormAddComponents(myForm, b1, b2, b3, b4, NULL);
	b_res = newtRunForm(myForm);
	newtFormDestroy(myForm);
	newtPopWindow();
	if (b_res == b1) {
		output = 'N';
	}
	if (b_res == b2) {
		output = 'I';
	}
	if (b_res == b3) {
		output = 'C';
	}
	if (b_res == b4) {
		output = 'E';
	}
	newtPopHelpLine();
	return (output);
}
