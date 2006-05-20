/*
 * Test file to test the configuration file handling
 *
 * $Id$
 *
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "my-stuff.h"
#include "libmondo-string.h"

main() {
	const char delims[3] = ": \n";
	
	int  lastpos = 0;
	char *token  = NULL;
	char *string = NULL;

	asprintf(&string, "%s", "md0 : active raid10 hda1[0] hda12[3] hda6[2] hda5[1]\n");
	fprintf(stdout, "string=|%s|\n", string);
	token = mr_strtok(string, delims, &lastpos);
	while (lastpos > 0) {
		fprintf(stdout, "token=|%s|, lastpos=%d\n", token, lastpos);
		paranoid_free(token);
		token = mr_strtok(string, delims, &lastpos);
	}
	paranoid_free(string);
	exit(0);
}
