/* $Id$
   subroutines for handling RAID
*/

/**
 * @file
 * Functions for handling RAID (especially during restore).
 */

#include "my-stuff.h"
#include "mondostructures.h"
#include "newt-specific-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-string-EXT.h"
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

	if (raidno == -1) {
		asprintf(&command,
				 "grep \"linear\" /proc/mdstat > /dev/null 2> /dev/null");
	} else {
		asprintf(&command,
				 "grep \"raid%d\" /proc/mdstat > /dev/null 2> /dev/null",
				 raidno);
	}
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

	assert(raidrec != NULL);
	assert(label != NULL);

	asprintf(&sz_value, "%d", value);
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
		char *org;
		switch (raidrec->plex[i].raidlevel) {
		case -1:
			asprintf(&org, "%s", "concat");
			break;
		case 0:
			asprintf(&org, "%s", "striped");
			break;
		case 5:
			asprintf(&org, "%s", "raid5");
			break;
		}
		fprintf(fout, "  plex org %s", org);
		paranoid_free(org);

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
	if (raidrec->raid_level == -1) {
		fprintf(fout, "    raid-level            linear\n");
	} else {
		fprintf(fout, "    raid-level            %d\n",
				raidrec->raid_level);
	}
	fprintf(fout, "    chunk-size            %d\n", raidrec->chunk_size);
	fprintf(fout, "    nr-raid-disks         %d\n",
			raidrec->data_disks.entries);
	fprintf(fout, "    nr-spare-disks        %d\n",
			raidrec->spare_disks.entries);
	if (raidrec->parity_disks.entries > 0) {
		fprintf(fout, "    nr-parity-disks       %d\n",
				raidrec->parity_disks.entries);
	}

	fprintf(fout, "    persistent-superblock %d\n",
			raidrec->persistent_superblock);
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
	char *incoming = NULL;
	char *p;
	size_t n = 0;

	assert(fin != NULL);
	assert(label != NULL);
	assert(value != NULL);

	label[0] = value[0] = '\0';
	if (feof(fin)) {
		return (1);
	}

	for (getline(&incoming, &n, fin); !feof(fin);
		 getline(&incoming, &n, fin)) {
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
	paranoid_free(incoming);
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
	char *tmp1;
	int items;

	raidlist->spares.entries = 0;
	raidlist->disks.entries = 0;
	if (length_of_file(fname) < 5) {
		log_it("Raidtab is very small or non-existent. Ignoring it.");
		raidlist->entries = 0;
		return (0);
	}
	if (!(fin = fopen(fname, "r"))) {
		log_it("Cannot open raidtab");
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
	asprintf(&tmp1, "%d RAID devices in raidtab", raidlist->entries);
	log_it(tmp1);
	paranoid_free(tmp1);
	return (0);
}


#else

int load_raidtab_into_raidlist(struct raidlist_itself *raidlist,
							   char *fname)
{
	FILE *fin;
	char *label;
	char *value;
	int items;
	int v;

	assert(raidlist != NULL);
	assert_string_is_neither_NULL_nor_zerolength(fname);

	if (length_of_file(fname) < 5) {
		log_it("Raidtab is very small or non-existent. Ignoring it.");
		raidlist->entries = 0;
		return (0);
	}
	if (!(fin = fopen(fname, "r"))) {
		log_it("Cannot open raidtab");
		return (1);
	}
	items = 0;
	log_it("Loading raidtab...");
	malloc_string(label);
	malloc_string(value);
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

	malloc_string(labelB);
	malloc_string(valueB);
	assert(fin != NULL);
	assert(raidrec != NULL);
	assert_string_is_neither_NULL_nor_zerolength(label);
	assert(value != NULL);

	if (!strcmp(label, "raid-level")) {
		if (!strcmp(value, "linear")) {
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
			asprintf(&tmp,
					 "Ignoring '%s %s' pair of disk %s", labelB, valueB,
					 label);
			log_it(tmp);
			paranoid_free(tmp);
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
					(_("That sizespec is more than 1,208,925,819,614,629,174,706,176 bytes. You have a shocking amount of data. Please send a screenshot to the list :-)"));
				return size * sign * 1024 * 1024 * 1024 * 1024 * 1024 *
					1024 * 1024 * 1024;
			}
		}
	}
	return size * sign;
}

#endif




int read_mdstat(struct s_mdstat *mdstat, char *mdstat_file)
{
	FILE *fin;
	char *tmp;
	char *stub;
	char *incoming = NULL;
	char *p, *q, *r;
	int diskno;
	size_t n = 0;

	malloc_string(incoming);
	if (!(fin = fopen(mdstat_file, "r"))) {
		log_msg(1, "%s not found", mdstat_file);
		return (1);
	}
	mdstat->entries = 0;
	for (getline(&incoming, &n, fin); !feof(fin);
		 getline(&incoming, &n, fin)) {
		p = incoming;
		if (*p != 'm' && *(p + 1) == 'm') {
			p++;
		}
		if (strncmp(p, "md", 2)) {
			continue;
		}
// read first line --- mdN : active raidX ............
		mdstat->el[mdstat->entries].md = atoi(p + 2);
		log_msg(8, "Storing /dev/md%d's info", atoi(p + 2));
		while (*p != ':' && *p) {
			p++;
		}
		while ((*p != 'r' || *(p + 1) != 'a') && *p) {
			p++;
		}
		if (!strncmp(p, "raid", 4)) {
			mdstat->el[mdstat->entries].raidlevel = *(p + 4) - '0';
		}
		p += 4;
		while (*p != ' ' && *p) {
			p++;
		}
		while (*p == ' ' && *p) {
			p++;
		}
		for (diskno = 0; *p; diskno++) {
			asprintf(&stub, "%s", p);
			q = strchr(stub, '[');
			if (q) {
				*q = '\0';
				q++;
				r = strchr(q, ']');
				if (r) {
					*r = '\0';
				}
				mdstat->el[mdstat->entries].disks.el[diskno].index =
					atoi(q);
			} else {
				mdstat->el[mdstat->entries].disks.el[diskno].index = -1;
				q = strchr(stub, ' ');
				if (q) {
					*q = '\0';
				}
			}
			asprintf(&tmp, "/dev/%s", stub);
			paranoid_free(stub);

			log_msg(8, "/dev/md%d : disk#%d : %s (%d)",
					mdstat->el[mdstat->entries].md, diskno, tmp,
					mdstat->el[mdstat->entries].disks.el[diskno].index);
			strcpy(mdstat->el[mdstat->entries].disks.el[diskno].device,
				   tmp);
			paranoid_free(tmp);

			while (*p != ' ' && *p) {
				p++;
			}
			while (*p == ' ' && *p) {
				p++;
			}
		}
		mdstat->el[mdstat->entries].disks.entries = diskno;
// next line --- skip it
		if (!feof(fin)) {
			getline(&incoming, &n, fin);
		} else {
			continue;
		}
// next line --- the 'progress' line
		if (!feof(fin)) {
			getline(&incoming, &n, fin);
		} else {
			continue;
		}
//  log_msg(1, "Percentage line = '%s'", incoming);
		if (!(p = strchr(incoming, '\%'))) {
			mdstat->el[mdstat->entries].progress = 999;	// not found
		} else if (strstr(incoming, "DELAYED")) {
			mdstat->el[mdstat->entries].progress = -1;	// delayed (therefore, stuck at 0%)
		} else {
			for (*p = '\0'; *p != ' '; p--);
			mdstat->el[mdstat->entries].progress = atoi(p);
		}
		log_msg(8, "progress =%d", mdstat->el[mdstat->entries].progress);
		mdstat->entries++;
	}
	fclose(fin);
	paranoid_free(incoming);
	return (0);
}



int create_raidtab_from_mdstat(char *raidtab_fname, char *mdstat_fname)
{
	struct raidlist_itself *raidlist;
	struct s_mdstat *mdstat;
	int retval = 0;
	int i;

	raidlist = malloc(sizeof(struct raidlist_itself));
	mdstat = malloc(sizeof(struct s_mdstat));

	if (read_mdstat(mdstat, mdstat_fname)) {
		log_to_screen("Sorry, cannot read %s", mdstat_fname);
		return (1);
	}

	for (i = 0; i < mdstat->entries; i++) {
		sprintf(raidlist->el[i].raid_device, "/dev/md%d",
				mdstat->el[i].md);
		raidlist->el[i].raid_level = mdstat->el[i].raidlevel;
		raidlist->el[i].persistent_superblock = 1;
		raidlist->el[i].chunk_size = 4;
		memcpy((void *) &raidlist->el[i].data_disks,
			   (void *) &mdstat->el[i].disks,
			   sizeof(struct list_of_disks));
		// FIXME --- the above line does not allow for spare disks
		log_to_screen
			(_("FIXME - create_raidtab_from_mdstat does not allow for spare disks"));
	}
	raidlist->entries = i;
	retval += save_raidlist_to_raidtab(raidlist, raidtab_fname);
	return (retval);
}


/* @} - end of raidGroup */
