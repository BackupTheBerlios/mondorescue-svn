/* libmondo-raid.c                                                subroutines for handling RAID
   $Id$
.


06/29
- added create_raidtab_from_mdstat()
- changed char[MAX_STR_LEN] to char*

10/21/2003
- get_next_raidtab_line() --- correctly handle multiple spaces
  between label and value

07/03
- line 447 - changed assert()

05/08
- cleaned up some FreeBSd-specific stuff

05/05
- added Joshua Oreman's FreeBSD patches

04/25
- added a bunch of RAID utilities from mondorestore/mondo-restore.c

04/24/2003
- added some assert()'s and log_OS_error()'s

10/19/2002
- added some comments

07/24
- created
*/


/**
 * @file
 * Functions for handling RAID (especially during restore).
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-string-EXT.h"
#include "lib-common-externs.h"
#include "libmondo-raid.h"

#ifdef __FreeBSD__
/* Nonstandard library functions: */
extern void errx(int exitval, const char *fmt, ...);
extern char *strsep(char **stringp, const char *delim);
#endif

/*@unused@*/
//static char cvsid[] = "$Id$";


/**
 * @addtogroup raidGroup
 * @{
 */
/**
 * See if a particular RAID level is supported by the kernel.
 * @param raidno The RAID level (-1 through 5) to check. -1 means "linear" under Linux and
 * "concatenated" under FreeBSD. It's really the same thing, just different wording.
 * @return TRUE if it's supported, FALSE if not.
 */
bool is_this_raid_personality_registered(int raidno)
{
#ifdef __FreeBSD__
	return ((raidno == -1) || (raidno == 0) || (raidno == 1)
			|| (raidno == 5)) ? TRUE : FALSE;
#else
	/*@ buffer ********************************************************** */
	char *command;
	int res;

	command = malloc(MAX_STR_LEN * 2);
	strcpy(command, "grep \" /proc/mdstat");
	if (raidno == -1) {
		strcat(command, "linear");
	} else {
		sprintf(command + strlen(command), "raid%d", raidno);
	}
	strcat(command, "\" > /dev/null 2> /dev/null");
	log_it("Is raid %d registered? Command = '%s'", raidno, command);
	res = system(command);
	paranoid_free(command);
	if (res) {
		return (FALSE);
	} else {
		return (TRUE);
	}
#endif
}






/**
 * Search for @p device in @p disklist.
 * @param disklist The disklist to search in.
 * @param device The device to search for.
 * @return The index number of @p device, or -1 if it does not exist.
 */
int
where_in_drivelist_is_drive(struct list_of_disks *disklist, char *device)
{

	/*@ int ************************************************************* */
	int i = 0;

	assert(disklist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(device);

	for (i = 0; i < disklist->entries; i++) {
		if (!strcmp(disklist->el[i].device, device)) {
			break;
		}
	}
	if (i == disklist->entries) {
		return (-1);
	} else {
		return (i);
	}
}








/**
 * Determine which RAID device is using a particular partition.
 * @param raidlist The RAID information structure.
 * @param device The partition to find out about.
 * @return The index number of the RAID device using @p device, or -1 if there is none.
 */
int
which_raid_device_is_using_this_partition(struct raidlist_itself *raidlist,
										  char *device)
{
#ifdef __FreeBSD__
// FreeBSD-specific version of which_raid_device_is_using_this_partition()
	/*@ int ********************************************************* */
	int i = 0;

	for (i = 0; i < raidlist->entries; i++) {
		bool thisone = FALSE;
		int j, k, l;

		for (j = 0; j < raidlist->el[i].plexes; ++j) {
			for (k = 0; k < raidlist->el[i].plex[j].subdisks; ++k) {
				for (l = 0; l < raidlist->disks.entries; ++l) {
					if (!strcmp(raidlist->disks.el[l].device,
								device) &&
						!strcmp(raidlist->el[i].plex[j].sd[k].which_device,
								raidlist->disks.el[l].name))
						thisone = TRUE;
				}
			}
		}

		if (thisone) {
			break;
		}
	}
	if (i == raidlist->entries) {
		return (-1);
	} else {
		return (i);
	}
}

#else
// Linux-specific version of which_raid_device_is_using_this_partition()
// and one other function which FreeBSD doesn't use

	int current_raiddev = 0;

	assert_string_is_neither_NULL_nor_zerolength(device);
	assert(raidlist != NULL);

	for (current_raiddev = 0; current_raiddev < raidlist->entries;
		 current_raiddev++) {
		if (where_in_drivelist_is_drive
			(&raidlist->el[current_raiddev].data_disks, device) >= 0
			|| where_in_drivelist_is_drive(&raidlist->el[current_raiddev].
										   spare_disks, device) >= 0
			|| where_in_drivelist_is_drive(&raidlist->el[current_raiddev].
										   parity_disks, device) >= 0
			|| where_in_drivelist_is_drive(&raidlist->el[current_raiddev].
										   failed_disks, device) >= 0) {
			break;
		}
	}
	if (current_raiddev == raidlist->entries) {
		return (-1);
	} else {
		return (current_raiddev);
	}
}

/**
 * Write an @c int variable to a list of RAID variables.
 * @param raidrec The RAID device record to write to.
 * @param lino The variable index number to modify/create.
 * @param label The label to write.
 * @param value The value to write.
 */
void
write_variableINT_to_raid_var_line(struct raid_device_record *raidrec,
								   int lino, char *label, int value)
{
	/*@ buffers ***************************************************** */
	char *sz_value;

	malloc_string(sz_value);
	assert(raidrec != NULL);
	assert(label != NULL);

	sprintf(sz_value, "%d", value);
	strcpy(raidrec->additional_vars.el[lino].label, label);
	strcpy(raidrec->additional_vars.el[lino].value, sz_value);
	paranoid_free(sz_value);
}
#endif








#ifdef __FreeBSD__
/**
 * Add a disk to a RAID plex.
 * @param p The plex to add the device to.
 * @param device_to_add The device to add to @p p.
 */
void add_disk_to_raid_device(struct vinum_plex *p, char *device_to_add)
{
	strcpy(p->sd[p->subdisks].which_device, device_to_add);
	++p->subdisks;

}
#else
/**
 * Add a disk to a RAID device.
 * @param disklist The disklist to add the device to.
 * @param device_to_add The device to add to @p disklist.
 * @param index The index number of the disklist entry we're creating.
 */
void add_disk_to_raid_device(struct list_of_disks *disklist,
							 char *device_to_add, int index)
{
	int items;

	assert(disklist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(device_to_add);
	items = disklist->entries;
	strcpy(disklist->el[items].device, device_to_add);
	disklist->el[items].index = index;
	items++;
	disklist->entries = items;
}
#endif


/**
 * Save the additional RAID variables to a stream.
 * @param vars The RAID variable list to save.
 * @param fout The FILE pointer to save them to.
 */
void
save_additional_vars_to_file(struct additional_raid_variables *vars,
							 FILE * fout)
{
	int i;

	assert(vars != NULL);
	assert(fout != NULL);

	for (i = 0; i < vars->entries; i++) {
		fprintf(fout, "    %-21s %s\n", vars->el[i].label,
				vars->el[i].value);
	}
}


/**
 * Save a raidlist structure to disk in raidtab format.
 * @param raidlist The raidlist to save.
 * @param fname The file to save it to.
 * @return 0, always.
 * @bug Return value is redundant.
 */
int save_raidlist_to_raidtab(struct raidlist_itself *raidlist, char *fname)
{
	FILE *fout;
	int current_raid_device;
#ifdef __FreeBSD__
	int i;
#else
// Linux
#endif

	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(fname);

	if (raidlist->entries <= 0) {
		unlink(fname);
		log_it("Deleting raidtab (no RAID devs anyway)");
		return (0);
	}
	if (!(fout = fopen(fname, "w"))) {
		log_OS_error("Failed to save raidlist");
		return (1);
	}
	fprintf(fout, "# Generated by Mondo Rescue\n");

#ifdef __FreeBSD__
	for (i = 0; i < raidlist->disks.entries; ++i) {
		fprintf(fout, "drive %s device %s\n", raidlist->disks.el[i].name,
				raidlist->disks.el[i].device);
	}
	for (i = 0; i < (raidlist->spares.entries); ++i) {
		fprintf(fout, "drive %s device %s hotspare\n",
				raidlist->spares.el[i].name,
				raidlist->spares.el[i].device);
	}
#endif

	for (current_raid_device = 0; current_raid_device < raidlist->entries;
		 current_raid_device++) {
		save_raidrec_to_file(&raidlist->el[current_raid_device], fout);
	}
	paranoid_fclose(fout);
	return (0);
}


/**
 * Save an individual RAID device record to a stream.
 * @param raidrec The RAID device record to save.
 * @param fout The stream to save it to.
 */
void save_raidrec_to_file(struct
#ifdef __FreeBSD__
						  vinum_volume
#else
						  raid_device_record
#endif
						  * raidrec, FILE * fout)
{
#ifdef __FreeBSD__
	int i, j;

	fprintf(fout, "\nvolume %s\n", raidrec->volname);
	for (i = 0; i < raidrec->plexes; ++i) {
		char org[24];
		switch (raidrec->plex[i].raidlevel) {
		case -1:
			strcpy(org, "concat");
			break;
		case 0:
			strcpy(org, "striped");
			break;
		case 5:
			strcpy(org, "raid5");
			break;
		}
		fprintf(fout, "  plex org %s", org);
		if (raidrec->plex[i].raidlevel != -1) {
			fprintf(fout, " %ik", raidrec->plex[i].stripesize);
		}
		fprintf(fout, "\n");

		for (j = 0; j < raidrec->plex[i].subdisks; ++j) {
			fprintf(fout, "    sd drive %s size 0\n",
					raidrec->plex[i].sd[j].which_device);
		}
	}
#else
	assert(raidrec != NULL);
	assert(fout != NULL);

	fprintf(fout, "raiddev %s\n", raidrec->raid_device);
	if (raidrec->raid_level == -2) {
		fprintf(fout, "    raid-level            multipath\n");
	} else if (raidrec->raid_level == -1) {
		fprintf(fout, "    raid-level            linear\n");
	} else {
		fprintf(fout, "    raid-level            %d\n",
				raidrec->raid_level);
	}
	fprintf(fout, "    nr-raid-disks         %d\n",
			raidrec->data_disks.entries);
	if (raidrec->spare_disks.entries > 0) {
		fprintf(fout, "    nr-spare-disks        %d\n",
				raidrec->spare_disks.entries);
	}
	if (raidrec->parity_disks.entries > 0) {
		fprintf(fout, "    nr-parity-disks       %d\n",
				raidrec->parity_disks.entries);
	}
	fprintf(fout, "    persistent-superblock %d\n",
			raidrec->persistent_superblock);
	if (raidrec->chunk_size > -1) {
	  fprintf(fout, "    chunk-size            %d\n", raidrec->chunk_size);
	}
	if (raidrec->parity > -1) {
	  switch(raidrec->parity) {
	  case 0:
	    fprintf(fout, "    parity-algorithm      left-asymmetric\n");
	    break;
	  case 1:
	    fprintf(fout, "    parity-algorithm      right-asymmetric\n");
	    break;
	  case 2:
	    fprintf(fout, "    parity-algorithm      left-symmetric\n");
	    break;
	  case 3:
	    fprintf(fout, "    parity-algorithm      right-symmetric\n");
	    break;
	  default:
	    fatal_error("Unknown RAID parity algorithm.");
	    break;
	  }
	}
	save_additional_vars_to_file(&raidrec->additional_vars, fout);
	fprintf(fout, "\n");
	save_disklist_to_file("raid-disk", &raidrec->data_disks, fout);
	save_disklist_to_file("spare-disk", &raidrec->spare_disks, fout);
	save_disklist_to_file("parity-disk", &raidrec->parity_disks, fout);
	save_disklist_to_file("failed-disk", &raidrec->failed_disks, fout);
	fprintf(fout, "\n");
#endif
}

/**
 * Retrieve the next line from a raidtab stream.
 * @param fin The file to read the input from.
 * @param label Where to put the line's label.
 * @param value Where to put the line's value.
 * @return 0 if the line was read and stored successfully, 1 if we're at end of file.
 */
int get_next_raidtab_line(FILE * fin, char *label, char *value)
{
	char *incoming;
	char *p;

	malloc_string(incoming);
	assert(fin != NULL);
	assert(label != NULL);
	assert(value != NULL);

	label[0] = value[0] = '\0';
	if (feof(fin)) {
		paranoid_free(incoming);
		return (1);
	}
	for (fgets(incoming, MAX_STR_LEN - 1, fin); !feof(fin);
		 fgets(incoming, MAX_STR_LEN - 1, fin)) {
		strip_spaces(incoming);
		p = strchr(incoming, ' ');
		if (strlen(incoming) < 3 || incoming[0] == '#' || !p) {
			continue;
		}
		*(p++) = '\0';
		while (*p == ' ') {
			p++;
		}
		strcpy(label, incoming);
		strcpy(value, p);
		paranoid_free(incoming);
		return (0);
	}
	return (1);
}



/**
 * Load a raidtab file into a raidlist structure.
 * @param raidlist The raidlist to fill.
 * @param fname The file to read from.
 * @return 0 for success, 1 for failure.
 */
#ifdef __FreeBSD__
int load_raidtab_into_raidlist(struct raidlist_itself *raidlist,
							   char *fname)
{
	FILE *fin;
	char *tmp;
	int items;

	malloc_string(tmp);
	raidlist->spares.entries = 0;
	raidlist->disks.entries = 0;
	if (length_of_file(fname) < 5) {
		log_it("Raidtab is very small or non-existent. Ignoring it.");
		raidlist->entries = 0;
		paranoid_free(tmp);
		return (0);
	}
	if (!(fin = fopen(fname, "r"))) {
		log_it("Cannot open raidtab");
		paranoid_free(tmp);
		return (1);
	}
	items = 0;
	log_it("Loading raidtab...");
	while (!feof(fin)) {
		int argc;
		char **argv = get_next_vinum_conf_line(fin, &argc);
		if (!argv)
			break;
		if (!strcmp(argv[0], "drive")) {
			char *drivename, *devname;
			if (argc < 4)
				continue;
			drivename = argv[1];
			devname = get_option_val(argc, argv, "device");
			if (!devname)
				continue;

			if (get_option_state(argc, argv, "hotspare")) {
				strcpy(raidlist->spares.el[raidlist->spares.entries].name,
					   drivename);
				strcpy(raidlist->spares.el[raidlist->spares.entries].
					   device, devname);
				raidlist->spares.el[raidlist->spares.entries].index =
					raidlist->disks.entries;
				raidlist->spares.entries++;
			} else {
				strcpy(raidlist->disks.el[raidlist->disks.entries].name,
					   drivename);
				strcpy(raidlist->disks.el[raidlist->disks.entries].device,
					   devname);
				raidlist->disks.el[raidlist->disks.entries].index =
					raidlist->disks.entries;
				raidlist->disks.entries++;
			}
		} else if (!strcmp(argv[0], "volume")) {
			char *volname;
			if (argc < 2)
				continue;
			volname = argv[1];
			strcpy(raidlist->el[raidlist->entries].volname, volname);
			raidlist->el[raidlist->entries].plexes = 0;
			raidlist->entries++;
		} else if (!strcmp(argv[0], "plex")) {
			int raidlevel, stripesize;
			char *org = 0;
			char **tmp = 0;
			if (argc < 3)
				continue;
			org = get_option_val(argc, argv, "org");
			if (!org)
				continue;
			if (strcmp(org, "concat")) {
				tmp = get_option_vals(argc, argv, "org", 2);
				if (tmp && tmp[1]) {
					stripesize = (int) (size_spec(tmp[1]) / 1024);
				} else
					stripesize = 279;
			} else
				stripesize = 0;

			if (!strcmp(org, "concat")) {
				raidlevel = -1;
			} else if (!strcmp(org, "striped")) {
				raidlevel = 0;
			} else if (!strcmp(org, "raid5")) {
				raidlevel = 5;
			} else
				continue;

			raidlist->el[raidlist->entries - 1].plex
				[raidlist->el[raidlist->entries - 1].plexes].raidlevel =
				raidlevel;
			raidlist->el[raidlist->entries -
						 1].plex[raidlist->el[raidlist->entries -
											  1].plexes].stripesize =
				stripesize;
			raidlist->el[raidlist->entries -
						 1].plex[raidlist->el[raidlist->entries -
											  1].plexes].subdisks = 0;
			raidlist->el[raidlist->entries - 1].plexes++;
		} else if ((!strcmp(argv[0], "sd"))
				   || (!strcmp(argv[0], "subdisk"))) {
			char *drive = 0;
			if (argc < 3)
				continue;
			drive = get_option_val(argc, argv, "drive");
			if (!drive)
				continue;

			strcpy(raidlist->el[raidlist->entries - 1].plex
				   [raidlist->el[raidlist->entries - 1].plexes - 1].sd
				   [raidlist->el[raidlist->entries - 1].plex
					[raidlist->el[raidlist->entries - 1].plexes -
					 1].subdisks].which_device, drive);
			raidlist->el[raidlist->entries -
						 1].plex[raidlist->el[raidlist->entries -
											  1].plexes - 1].subdisks++;
		}
	}
	fclose(fin);
	log_it("Raidtab loaded successfully.");
	sprintf(tmp, "%d RAID devices in raidtab", raidlist->entries);
	log_it(tmp);
	paranoid_free(tmp);
	return (0);
}


#else

int load_raidtab_into_raidlist(struct raidlist_itself *raidlist,
							   char *fname)
{
	FILE *fin;
	char *tmp;
	char *label;
	char *value;
	int items;
	int v;

	malloc_string(tmp);
	malloc_string(label);
	malloc_string(value);
	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(fname);

	if (length_of_file(fname) < 5) {
		log_it("Raidtab is very small or non-existent. Ignoring it.");
		raidlist->entries = 0;
		paranoid_free(tmp);
		paranoid_free(label);
		paranoid_free(value);
		return (0);
	}
	if (!(fin = fopen(fname, "r"))) {
		log_it("Cannot open raidtab");
		paranoid_free(tmp);
		paranoid_free(label);
		paranoid_free(value);
		return (1);
	}
	items = 0;
	log_it("Loading raidtab...");
	get_next_raidtab_line(fin, label, value);
	while (!feof(fin)) {
		log_msg(1, "Looking for raid record #%d", items);
		initialize_raidrec(&raidlist->el[items]);
		v = 0;
		/* find the 'raiddev' entry, indicating the start of the block of info */
		while (!feof(fin) && strcmp(label, "raiddev")) {
			strcpy(raidlist->el[items].additional_vars.el[v].label, label);
			strcpy(raidlist->el[items].additional_vars.el[v].value, value);
			v++;
			get_next_raidtab_line(fin, label, value);
			log_it(tmp);
		}
		raidlist->el[items].additional_vars.entries = v;
		if (feof(fin)) {
			log_msg(1, "No more records.");
			continue;
		}
		log_msg(2, "Record #%d (%s) found", items, value);
		strcpy(raidlist->el[items].raid_device, value);
		for (get_next_raidtab_line(fin, label, value);
			 !feof(fin) && strcmp(label, "raiddev");
			 get_next_raidtab_line(fin, label, value)) {
			process_raidtab_line(fin, &raidlist->el[items], label, value);
		}
		items++;
	}
	paranoid_fclose(fin);
	raidlist->entries = items;
	log_msg(1, "Raidtab loaded successfully.");
	log_msg(1, "%d RAID devices in raidtab", items);
	paranoid_free(tmp);
	paranoid_free(label);
	paranoid_free(value);
	return (0);
}
#endif








#ifndef __FreeBSD__
/**
 * Process a single line from the raidtab and store the results into @p raidrec.
 * @param fin The stream to read the line from.
 * @param raidrec The RAID device record to update.
 * @param label Where to put the label processed.
 * @param value Where to put the value processed.
 */
void
process_raidtab_line(FILE * fin,
					 struct raid_device_record *raidrec,
					 char *label, char *value)
{

	/*@ add mallocs * */
	char *tmp;
	char *labelB;
	char *valueB;

	struct list_of_disks *disklist;
	int index;
	int v;

	malloc_string(tmp);
	malloc_string(labelB);
	malloc_string(valueB);
	assert(fin != NULL);
	assert(raidrec != NULL);
	assert_string_is_neither_NULL_nor_zerolength(label);
	assert(value != NULL);

	if (!strcmp(label, "raid-level")) {
		if (!strcmp(value, "multipath")) {
			raidrec->raid_level = -2;
		} else if (!strcmp(value, "linear")) {
			raidrec->raid_level = -1;
		} else {
			raidrec->raid_level = atoi(value);
		}
	} else if (!strcmp(label, "nr-raid-disks")) {	/* ignore it */
	} else if (!strcmp(label, "nr-spare-disks")) {	/* ignore it */
	} else if (!strcmp(label, "nr-parity-disks")) {	/* ignore it */
	} else if (!strcmp(label, "nr-failed-disks")) {	/* ignore it */
	} else if (!strcmp(label, "persistent-superblock")) {
		raidrec->persistent_superblock = atoi(value);
	} else if (!strcmp(label, "chunk-size")) {
		raidrec->chunk_size = atoi(value);
	} else if (!strcmp(label, "parity-algorithm")) {
		if (!strcmp(value, "left-asymmetric")) {
			raidrec->parity = 0;
		} else if (!strcmp(value, "right-asymmetric")) {
			raidrec->parity = 1;
		} else if (!strcmp(value, "left-symmetric")) {
			raidrec->parity = 2;
		} else if (!strcmp(value, "right-symmetric")) {
			raidrec->parity = 3;
		} else {
			log_msg(1, "Unknown RAID parity algorithm '%s'\n.", value);
		}
	} else if (!strcmp(label, "device")) {
		get_next_raidtab_line(fin, labelB, valueB);
		if (!strcmp(labelB, "raid-disk")) {
			disklist = &raidrec->data_disks;
		} else if (!strcmp(labelB, "spare-disk")) {
			disklist = &raidrec->spare_disks;
		} else if (!strcmp(labelB, "parity-disk")) {
			disklist = &raidrec->parity_disks;
		} else if (!strcmp(labelB, "failed-disk")) {
			disklist = &raidrec->failed_disks;
		} else {
			disklist = NULL;
		}
		if (!disklist) {
			sprintf(tmp,
					"Ignoring '%s %s' pair of disk %s", labelB, valueB,
					label);
			log_it(tmp);
		} else {
			index = atoi(valueB);
			add_disk_to_raid_device(disklist, value, index);
		}
	} else {
		v = raidrec->additional_vars.entries;
		strcpy(raidrec->additional_vars.el[v].label, label);
		strcpy(raidrec->additional_vars.el[v].value, value);
		raidrec->additional_vars.entries = ++v;
	}
	paranoid_free(tmp);
	paranoid_free(labelB);
	paranoid_free(valueB);
}
#endif


/**
 * Save a disklist to a stream in raidtab format.
 * @param listname One of "raid-disk", "spare-disk", "parity-disk", or "failed-disk".
 * @param disklist The disklist to save to @p fout.
 * @param fout The stream to write to.
 */
void
save_disklist_to_file(char *listname,
					  struct list_of_disks *disklist, FILE * fout)
{
	int i;

	assert_string_is_neither_NULL_nor_zerolength(listname);
	assert(disklist != NULL);
	assert(fout != NULL);

	for (i = 0; i < disklist->entries; i++) {
		fprintf(fout, "    device                %s\n",
				disklist->el[i].device);
		fprintf(fout, "    %-21s %d\n", listname, disklist->el[i].index);
	}
}





#ifdef __FreeBSD__
/**
 * Add a new plex to a volume. The index of the plex will be <tt>v-\>plexes - 1</tt>.
 * @param v The volume to operate on.
 * @param raidlevel The RAID level of the new plex.
 * @param stripesize The stripe size (chunk size) of the new plex.
 */
void add_plex_to_volume(struct vinum_volume *v, int raidlevel,
						int stripesize)
{
	v->plex[v->plexes].raidlevel = raidlevel;
	v->plex[v->plexes].stripesize = stripesize;
	v->plex[v->plexes].subdisks = 0;
	++v->plexes;
}

/**
 * For internal use only.
 */
char **get_next_vinum_conf_line(FILE * f, int *argc)
{
	int cnt = 0;
	static char *argv[64];
	char **ap;
	char *line = (char *) malloc(MAX_STR_LEN);
	if (!line)
		errx(1,
			 "unable to allocate %i bytes of memory for `char *line' at %s:%i",
			 MAX_STR_LEN, __FILE__, __LINE__);
	(void) fgets(line, MAX_STR_LEN, f);
	if (feof(f)) {
		log_it("[GNVCL] Uh... I reached the EOF.");
		return 0;
	}

	for (ap = argv; (*ap = strsep(&line, " \t")) != NULL;)
		if (**ap != '\0') {
			if (++ap >= &argv[64])
				break;
			cnt++;
		}

	if (strchr(argv[cnt - 1], '\n')) {
		*(strchr(argv[cnt - 1], '\n')) = '\0';
	}

	if (argc)
		*argc = cnt;
	return argv;
}

/**
 * For internal use only.
 */
char *get_option_val(int argc, char **argv, char *option)
{
	int i;
	for (i = 0; i < (argc - 1); ++i) {
		if (!strcmp(argv[i], option)) {
			return argv[i + 1];
		}
	}
	return 0;
}

/**
 * For internal use only.
 */
char **get_option_vals(int argc, char **argv, char *option, int nval)
{
	int i, j;
	static char **ret;
	ret = (char **) malloc(nval * sizeof(char *));
	for (i = 0; i < (argc - nval); ++i) {
		if (!strcmp(argv[i], option)) {
			for (j = 0; j < nval; ++j) {
				ret[j] = (char *) malloc(strlen(argv[i + j + 1]) + 1);
				strcpy(ret[j], argv[i + j + 1]);
			}
			return ret;
		}
	}
	return 0;
}

/**
 * For internal use only.
 */
bool get_option_state(int argc, char **argv, char *option)
{
	int i;
	for (i = 0; i < argc; ++i)
		if (!strcmp(argv[i], option))
			return TRUE;

	return FALSE;
}

/**
 * Taken from Vinum source -- for internal use only.
 */
long long size_spec(char *spec)
{
	u_int64_t size;
	char *s;
	int sign = 1;				/* -1 if negative */

	size = 0;
	if (spec != NULL) {			/* we have a parameter */
		s = spec;
		if (*s == '-') {		/* negative, */
			sign = -1;
			s++;				/* skip */
		}
		if ((*s >= '0') && (*s <= '9')) {	/* it's numeric */
			while ((*s >= '0') && (*s <= '9'))	/* it's numeric */
				size = size * 10 + *s++ - '0';	/* convert it */
			switch (*s) {
			case '\0':
				return size * sign;

			case 'B':
			case 'b':
			case 'S':
			case 's':
				return size * sign * 512;

			case 'K':
			case 'k':
				return size * sign * 1024;

			case 'M':
			case 'm':
				return size * sign * 1024 * 1024;

			case 'G':
			case 'g':
				return size * sign * 1024 * 1024 * 1024;

			case 'T':
			case 't':
				log_it
					("Ok, I'm scared... Someone did a TERABYTE+ size-spec");
				return size * sign * 1024 * 1024 * 1024 * 1024;

			case 'P':
			case 'p':
				log_it
					("If I was scared last time, I'm freaked out now. Someone actually has a PETABYTE?!?!?!?!");
				return size * sign * 1024 * 1024 * 1024 * 1024 * 1024;

			case 'E':
			case 'e':
				log_it
					("Okay, I'm REALLY freaked out. Who could devote a whole EXABYTE to their data?!?!");
				return size * sign * 1024 * 1024 * 1024 * 1024 * 1024 *
					1024;

			case 'Z':
			case 'z':
				log_it
					("WHAT!?!? A ZETABYTE!?!? You've GOT to be kidding me!!!");
				return size * sign * 1024 * 1024 * 1024 * 1024 * 1024 *
					1024 * 1024;

			case 'Y':
			case 'y':
				log_it
					("Oh my gosh. You actually think a YOTTABYTE will get you anywhere? What're you going to do with 1,208,925,819,614,629,174,706,176 bytes?!?!");
				popup_and_OK
					("That sizespec is more than 1,208,925,819,614,629,174,706,176 bytes. You have a shocking amount of data. Please send a screenshot to the list :-)");
				return size * sign * 1024 * 1024 * 1024 * 1024 * 1024 *
					1024 * 1024 * 1024;
			}
		}
	}
	return size * sign;
}

#endif




int parse_mdstat(struct raidlist_itself *raidlist, char *device_prefix) {

  const char delims[] = " ";

  FILE   *fin;
  int    res = 0, row, i, index_min;
  size_t len = 0;
  char   *token, *string = NULL, *pos, type, *strtmp;

  // open file
  if (!(fin = fopen(MDSTAT_FILE, "r"))) {
    log_msg(1, "Could not open %s.\n", MDSTAT_FILE);
    return 1;
  }
  // initialise record, build progress and row counters
  raidlist->entries = 0;
  raidlist->el[raidlist->entries].progress = 999;
  row = 1;
  // skip first output row - contains registered RAID levels
  res = getline(&string, &len, fin);
  // parse the rest
  while ( !feof_unlocked(fin) ) {
    res = getline(&string, &len, fin);
    if (res <= 0) break;
    // trim leading spaces
    pos = string;
    while (*pos == ' ') *pos++;
    memmove(string, pos, strlen(string));
    // if we have newline after only spaces, this is a blank line, update
    // counters, otherwise do normal parsing
    if (*string == '\n') {
      row = 1;
      raidlist->entries++;
      raidlist->el[raidlist->entries].progress = 999;
    } else {
      switch (row) {
      case 1:  // device information
	// check whether last line of record and if so skip
	pos = strcasestr(string, "unused devices: ");
	if (pos == string) {
	  //raidlist->entries--;
	  break;
	}
	// tokenise string
	token = strtok (string, delims);
	// get RAID device name
	asprintf(&strtmp,"%s%s", device_prefix, token);
	strcpy(raidlist->el[raidlist->entries].raid_device, strtmp);
	paranoid_free(strtmp);
	// skip ':' and status
	token = strtok (NULL, delims);
	token = strtok (NULL, delims);
	if (!strcmp(token, "inactive")) {
	  log_msg(1, "RAID device '%s' inactive.\n",
		 raidlist->el[raidlist->entries].raid_device);
	  paranoid_free(string);
	  return 1;
	}
	// get RAID level
	token = strtok (NULL, delims);
	if (!strcmp(token, "multipath")) {
	  raidlist->el[raidlist->entries].raid_level = -2;
	} else if (!strcmp(token, "linear")) {
	  raidlist->el[raidlist->entries].raid_level = -1;
	} else if (!strcmp(token, "raid0")) {
	  raidlist->el[raidlist->entries].raid_level = 0;
	} else if (!strcmp(token, "raid1")) {
	  raidlist->el[raidlist->entries].raid_level = 1;
	} else if (!strcmp(token, "raid4")) {
	  raidlist->el[raidlist->entries].raid_level = 4;
	} else if (!strcmp(token, "raid5")) {
	  raidlist->el[raidlist->entries].raid_level = 5;
	} else if (!strcmp(token, "raid6")) {
	  raidlist->el[raidlist->entries].raid_level = 6;
	} else if (!strcmp(token, "raid10")) {
	  raidlist->el[raidlist->entries].raid_level = 10;
	} else {
	  log_msg(1, "Unknown RAID level '%s'.\n", token);
	  paranoid_free(string);
	  return 1;
	}
	// get RAID devices (type, index, device)
	// Note: parity disk for RAID4 is last normal disk, there is no '(P)'
	raidlist->el[raidlist->entries].data_disks.entries = 0;
	raidlist->el[raidlist->entries].spare_disks.entries = 0;
	raidlist->el[raidlist->entries].failed_disks.entries = 0;
	while((token = strtok (NULL, delims))) {
	  if ((pos = strstr(token, "("))) {
	    type = *(pos+1);
	  } else {
	    type = ' ';
	  }
	  pos = strstr(token, "[");
	  *pos = '\0';
	  switch(type) {
	  case ' ': // normal data disks
	    raidlist->el[raidlist->entries].data_disks.el[raidlist->el[raidlist->entries].data_disks.entries].index = atoi(pos + 1);
	    asprintf(&strtmp,"%s%s", device_prefix, token);
	    strcpy(raidlist->el[raidlist->entries].data_disks.el[raidlist->el[raidlist->entries].data_disks.entries].device, strtmp);
	    paranoid_free(strtmp);
	    raidlist->el[raidlist->entries].data_disks.entries++;
	    break;
	  case 'S': // spare disks
	    raidlist->el[raidlist->entries].spare_disks.el[raidlist->el[raidlist->entries].spare_disks.entries].index = atoi(pos + 1);
	    asprintf(&strtmp,"%s%s", device_prefix, token);
	    strcpy(raidlist->el[raidlist->entries].spare_disks.el[raidlist->el[raidlist->entries].spare_disks.entries].device, strtmp);
	    paranoid_free(strtmp);
	    raidlist->el[raidlist->entries].spare_disks.entries++;
	    break;
	  case 'F': // failed disks
	    raidlist->el[raidlist->entries].failed_disks.el[raidlist->el[raidlist->entries].failed_disks.entries].index = atoi(pos + 1);
	    asprintf(&strtmp,"%s%s", device_prefix, token);
	    strcpy(raidlist->el[raidlist->entries].failed_disks.el[raidlist->el[raidlist->entries].failed_disks.entries].device, strtmp);
	    paranoid_free(strtmp);
	    raidlist->el[raidlist->entries].failed_disks.entries++;
	    log_it("At least one failed disk found in RAID array.\n");
	    break;
	  default: // error
	    log_msg(1, "Unknown device type '%c'\n", type);
	    paranoid_free(string);
	    return 1;
	    break;
	  }
	}
	// adjust index for each device so that it starts with 0 for every type
	index_min = 99;
	for (i=0; i<raidlist->el[raidlist->entries].data_disks.entries;i++) {
	  if (raidlist->el[raidlist->entries].data_disks.el[i].index < index_min) {
	    index_min = raidlist->el[raidlist->entries].data_disks.el[i].index;
	  }
	}
	if (index_min > 0) {
	  for (i=0; i<raidlist->el[raidlist->entries].data_disks.entries;i++) {
	    raidlist->el[raidlist->entries].data_disks.el[i].index = raidlist->el[raidlist->entries].data_disks.el[i].index - index_min;	
	  }
	}
	index_min = 99;
	for (i=0; i<raidlist->el[raidlist->entries].spare_disks.entries;i++) {
	  if (raidlist->el[raidlist->entries].spare_disks.el[i].index < index_min) {
	    index_min = raidlist->el[raidlist->entries].spare_disks.el[i].index;
	  }
	}
	if (index_min > 0) {
	  for (i=0; i<raidlist->el[raidlist->entries].spare_disks.entries;i++) {
	    raidlist->el[raidlist->entries].spare_disks.el[i].index = raidlist->el[raidlist->entries].spare_disks.el[i].index - index_min;	
	  }
	}
	index_min = 99;
	for (i=0; i<raidlist->el[raidlist->entries].failed_disks.entries;i++) {
	  if (raidlist->el[raidlist->entries].failed_disks.el[i].index < index_min) {
	    index_min = raidlist->el[raidlist->entries].failed_disks.el[i].index;
	  }
	}
	if (index_min > 0) {
	  for (i=0; i<raidlist->el[raidlist->entries].failed_disks.entries;i++) {
	    raidlist->el[raidlist->entries].failed_disks.el[i].index = raidlist->el[raidlist->entries].failed_disks.el[i].index - index_min;	
	  }
	}
	break;
      case 2:  // config information
	// check for persistent super block
	if (strcasestr(string, "super non-persistent")) {
	  raidlist->el[raidlist->entries].persistent_superblock = 0;
	} else {
	  raidlist->el[raidlist->entries].persistent_superblock = 1;
	}
	// extract chunk size
	if (!(pos = strcasestr(string, "k chunk"))) {
	  raidlist->el[raidlist->entries].chunk_size = -1;
	} else {
	  while (*pos != ' ') {
	    *pos--;
	    if (pos < string) {
	      log_it("String underflow!\n");
	      paranoid_free(string);
	      return 1;
	    }
	  }
	  raidlist->el[raidlist->entries].chunk_size = atoi(pos + 1);
	}
	// extract parity if present
	if ((pos = strcasestr(string, "algorithm"))) {
	  raidlist->el[raidlist->entries].parity = atoi(pos + 9);
	} else {
	  raidlist->el[raidlist->entries].parity = -1;
	}
	break;
      case 3:  // optional build status information
	if (!(pos = strchr(string, '\%'))) {
	  if (strcasestr(string, "delayed")) {
	    raidlist->el[raidlist->entries].progress = -1;	// delayed (therefore, stuck at 0%)
	  } else {
	    raidlist->el[raidlist->entries].progress = 999;	// not found
	  }
	} else {
	  while (*pos != ' ') {
	    *pos--;
	    if (pos < string) {
	      printf("ERROR: String underflow!\n");
	      paranoid_free(string);
	      return 1;
	    }
	  }
	  raidlist->el[raidlist->entries].progress = atoi(pos);
	}
	break;
      default: // error
	log_msg(1, "Row %d should not occur in record!\n", row);
	break;
      }
      row++;
    }
  }
  // close file
  fclose(fin);
  // free string
  paranoid_free(string);
  // return success
  return 0;

}




int create_raidtab_from_mdstat(char *raidtab_fname)
{
	struct raidlist_itself *raidlist;
	int retval = 0;

	raidlist = malloc(sizeof(struct raidlist_itself));

	// FIXME: Prefix '/dev/' should really be dynamic!
	if (parse_mdstat(raidlist, "/dev/")) {
		log_to_screen("Sorry, cannot read %s", MDSTAT_FILE);
		return (1);
	}

	retval += save_raidlist_to_raidtab(raidlist, raidtab_fname);
	return (retval);
}



/* @} - end of raidGroup */
