/*
 * Test file to test the configuration file handling
 *
 * $Id$
 *
 */

#include <stdio.h>
#include "my-stuff.h"
#include "libmondo-conf.h"

int g_main_pid = 0;
int g_buffer_pid = 0;

main() {
	int ret = 0;
	int i = 0;
	double f = 0.0;
	char *s = NULL;
	
	if ((ret = mrconf_open("mondo.conf")) != 0) {
		fprintf(stderr,"Unable to open conf file (%d)\n",ret); 
		exit(-1);
	}
	if ((i = mrconf_iread("testinteger")) == 0) {
		fprintf(stderr,"Unable to get integer\n");
		exit(-1);
	}
	fprintf(stdout, "Integer : ***%d***\n",i);
	if ((f = mrconf_fread("testfloat")) == 0.0) {
		fprintf(stderr,"Unable to get float\n");
		exit(-1);
	}
	fprintf(stdout, "Float : ***%f***\n",f);
	if (! (s = mrconf_sread("teststring"))) {
		fprintf(stderr,"Unable to get string\n");
		exit(-1);
	}
	fprintf(stdout, "String : ***%s***\n",s);
	paranoid_free(s);
	mrconf_close();
	exit(0);
}
