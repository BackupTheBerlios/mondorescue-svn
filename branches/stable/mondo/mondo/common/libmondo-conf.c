/* libmondo-conf.c
 *
 * $Id$
 *
 * Based on the work done by Anton (c)2002-2004 Anton Kulchitsky  mailto:anton@kulchitsky.org
 * Code (c)2006 Bruno Cornec <bruno@mondorescue.org>
 *   
 *     Main file of libmondo-conf (mrconf): a very small and simple
 *     library for configuration file reading
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "my-stuff.h"

/* error flags */
#define MRCONF_NO_ERROR             0x0
#define MRCONF_BAD_FILE             0x1
#define MRCONF_ALLOC_FAILED         0x2
#define MRCONF_READING_FAILED       0x3
#define MRCONF_NO_GROUP_START       0x4
#define MRCONF_NO_GROUP_END         0x5
#define MRCONF_FIELD_NOT_FOUND      0x6
#define MRCONF_FIELD_NO_VALUE       0x7
#define MRCONF_TOO_LONG_STRING      0x8
#define MRCONF_CLOSE_BUT_NOT_OPEN   0x9
#define MRCONF_OPEN_OPENED          0xA
#define MRCONF_CALL_BUT_NOT_OPEN    0xB

/*setting flags*/
#define MRCONF_FLAG_VERBOSE         0x1
#define MRCONF_FLAG_ERROR_EXIT      0x2

/*All strings of the library are here*/
#define MRCONF_STR_ERROR          _("MRCONF: Error:")
#define MRCONF_STR_WARNING        _("MRCONF: Warning:")

#define MRCONF_STR_BAD_FILE       _("cannot open the file: ")
#define MRCONF_STR_ALLOC_FAILED   _("cannot allocate the memory")
#define MRCONF_STR_READING_FAILED _("cannot read file into buffer")
#define MRCONF_STR_DEFAULT_ERROR  _("default")
#define MRCONF_STR_FIELD_NOT_FOUND   _("the field is not found, field:")
#define MRCONF_STR_FIELD_NO_VALUE   _("the field :")
#define MRCONF_STR_OPEN_OPENED _("attempt to open libmondo-conf, but it is opened: aborted")
#define MRCONF_STR_HALT _("MRCONF: Error occured: immidiate halt")

/*warnings*/
#define MRCONF_STR_SET_TO_ZERO    _("variable will be set to 0")
#define MRCONF_STR_IGNORE    _("has no value, ignoring it")
#define MRCONF_STR_CLOSE_BUT_NOT_OPEN _("attempt to close libmondo-conf but it has not been opened yet")
#define MRCONF_STR_CALL_BUT_NOT_OPEN _("attempt to use libmondo-conf when it has not been opened yet")

/*Flags of internal state*/
#define MRCONF_INTFLAG_OPEN 0x1	/*set if memory allocated */

/* Character for comments */
#define MRCONF_COMM_CHAR '#'

/*"private" members declarations*/
static size_t mrconf_filesize(const char *name);
static void mrconf_error_msg(int error_code, const char *add_line);
static void mrconf_remove_comments();
static int mrconf_check_int_flag(const int flag);
static void mrconf_set_int_flag(const int flag);
static void mrconf_drop_int_flag(const int flag);

/*global "private" variables*/
static char *buffer = NULL;		/*buffer for configuration file */
static int internal_flags = 0;	/*state of the module */
static FILE *CONF = NULL;		/* Configuration file FD */

/*if output all error and warnin messages*/
static int mrconf_flags = MRCONF_FLAG_VERBOSE;

/* 
 * Format of the configuration file is as follows:
 *
 * attribute1 = int_value
 * attribute2 = float_value
 * attribute3 = string_value
 */


/*open and read file: each call must be coupled with mrconf_close
  function: return 0 if success*/
int mrconf_open(const char *filename) {
	size_t length;				/*length of the buffer/file */

	/* check if libmondo-conf is already opened? */
	if (mrconf_check_int_flag(MRCONF_INTFLAG_OPEN)) {
		mrconf_error_msg(MRCONF_OPEN_OPENED, NULL);
		return MRCONF_OPEN_OPENED;
	}

	length = mrconf_filesize(filename);
	CONF = fopen(filename, "r");

	/*if file is empty or not exist => error */
	if (length == 0) {
		mrconf_error_msg(MRCONF_BAD_FILE, filename);
		return (MRCONF_BAD_FILE);
	}

	/*creating and reading buffer for file content */

	/*allocate memory for the buffers */
	buffer = (char *) malloc(sizeof(char) * (length + 1));

	if (buffer == NULL) {
		mrconf_error_msg(MRCONF_ALLOC_FAILED, "");
		return (MRCONF_ALLOC_FAILED);
	}

	/*set flag that module is in "open" state */
	mrconf_set_int_flag(MRCONF_INTFLAG_OPEN);

	/*reading file in buffer (skip all 0 characters) */

	(void) fread(buffer, sizeof(char), length, CONF);
	buffer[length] = (char) 0;	/*finalize the string */

	if (ferror(CONF)) {
		mrconf_error_msg(MRCONF_READING_FAILED, "");
		return (MRCONF_READING_FAILED);
	}

	/* finally we have to remove all comment lines */
	mrconf_remove_comments();

	return MRCONF_NO_ERROR;
}

/*release all memory and prepare to the next possiable config file*/
void mrconf_close() {
	/* if not opened => error */
	if (!mrconf_check_int_flag(MRCONF_INTFLAG_OPEN)) {
		mrconf_error_msg(MRCONF_CLOSE_BUT_NOT_OPEN, NULL);
	}
	paranoid_free(buffer);
	fclose(CONF);

	/*set flag that module is in "close" state */
	mrconf_drop_int_flag(MRCONF_INTFLAG_OPEN);
}

/*read field value after string str in the current file*/
static char *mrconf_read(const char *field_name) {
	char *p = NULL;				/*pointer to the field */

	/* check if libmondo-conf is not yet opened? */
	if (!mrconf_check_int_flag(MRCONF_INTFLAG_OPEN)) {
		mrconf_error_msg(MRCONF_CALL_BUT_NOT_OPEN, NULL);
		return NULL;
	}

	/*read the number */
	p = strstr(buffer, field_name);
	if (p == NULL) {
		mrconf_error_msg(MRCONF_FIELD_NOT_FOUND, field_name);
		return NULL;
	} else {
		p += strlen(field_name);
		while ((*p != '\n') && (*p != '\0') && (*p != '=')) {
				p++;
		}
		if (*p != '=') {
			mrconf_error_msg(MRCONF_FIELD_NO_VALUE, field_name);
			return NULL;
		} else {
			/* points after the = sign */
			p++;
		}
	}
	/* skip initial spaces and tabs after = */
	while ((*p == ' ') || (*p == '\t')) {
			p++;
	}

	return p;
}

/*read integer number after string str in the current file*/
int mrconf_iread(const char *field_name) {
	char *p = NULL;				/*pointer to the field */
	int ret = 0;				/*return value */

	p = mrconf_read(field_name);
	if (p != NULL) {
		ret = atoi(p);
		}
	return ret;
}

/*read float/double number after string str in the current file*/
double mrconf_fread(const char *field_name) {
	char *p = NULL;				/*pointer to the field */
	double ret = 0.0;				/*return value */

	p = mrconf_read(field_name);
	if (p != NULL) {
		ret = atof(p);
	}
	return ret;
}

/*
  reads string outstr after string str in the current file (between
  "..."), not more than maxlength simbols: cannot check if outstr has
  enough length! It must be at least maxlength+1 ! Returns number of
  read chars
*/
char *mrconf_sread(const char *field_name) {
	char *p = NULL;				/*pointer to the field */
	char *q = NULL;				/*pointer to the field */
	char *r = NULL;				/*pointer to the field */
	char *ret = NULL;			/*return value */
	int size = 0;				/*size of returned string */
	int i = 0;

	ret = NULL;

	p = mrconf_read(field_name);
	if (p == NULL) {
		return(p);
	}
	asprintf(&q, p);

	/* trunk at first \n */
	r = index(q,'\n');

	size = r-q+1;
	/*copy filtered data to the buffer */
	ret = (char *) malloc(sizeof(char) * (size));
	if (ret == NULL) {
		mrconf_error_msg(MRCONF_ALLOC_FAILED, "");
		return (NULL);
	}
	while (i < size - 1) {
		ret[i] = *p;
		i++;
		p++;
	}

	ret[i] = (char) 0;		/*and set its length */
	paranoid_free(q);

	return ret;
}

/*removes all comments from the buffer*/
static void mrconf_remove_comments() {
	char *tmp_buf;				/*temporary buffer without comments */
	size_t length				/*initial length */ ;
	size_t i;					/*iterator */
	size_t k;					/*conditioned iterator for tmp_buffer */

	length = strlen(buffer);

	/*sizing the new chain */
	k = 0;
	i = 0;
	while (i < length) {
		if (buffer[i] != MRCONF_COMM_CHAR) {
			k++;
			i++;
		} else {
			while ((buffer[i] != '\n') && (buffer[i] != (char) 0)) {
				i++;
			}
		}
	}
	/* k is new buffer length now */
	tmp_buf = (char *) malloc(sizeof(char) * (k + 1));
	if (tmp_buf == NULL) {
		mrconf_error_msg(MRCONF_ALLOC_FAILED, "");
		return;
	}

	k = 0;
	i = 0;
	while (i < length) {
		if (buffer[i] != MRCONF_COMM_CHAR) {
			tmp_buf[k++] = buffer[i++];
		} else {
			while ((buffer[i] != '\n') && (buffer[i] != (char) 0)) {
				i++;
			}
		}
	}
	tmp_buf[k] = (char) 0;		/*and set its length */

	paranoid_free(buffer);
	/*copy filtered data to the buffer */
	buffer = tmp_buf;
}

static int mrconf_check_int_flag(const int flag) {
	return (flag & internal_flags);
}

static void mrconf_set_int_flag(const int flag) {
	internal_flags = flag | internal_flags;
}

static void mrconf_drop_int_flag(const int flag) {
	internal_flags = (~flag) & internal_flags;
}


/*local function to define size of a file. Return 0 for mistake*/
static size_t mrconf_filesize(const char *name) {
	FILE *F = fopen(name, "r");	/*file to open */
	size_t length;				/*number to return */

	if (F == NULL) {
		return 0;
	}

	fseek(F, 0, SEEK_END);		/*set position to the end of file */
	length = ftell(F);			/*define number of position=> this is its
								   length */
	fclose(F);

	return length;
}

/*output error message*/
static void mrconf_error_msg(int error_code, const char *add_line) {
	if ((mrconf_flags & MRCONF_FLAG_VERBOSE)) {	/*if verbose mode */
		switch (error_code) {
		case MRCONF_BAD_FILE:
			log_msg(4, "%s %s %s\n", MRCONF_STR_ERROR, MRCONF_STR_BAD_FILE,
				   add_line);
			break;

		case MRCONF_ALLOC_FAILED:
			log_msg(4, "%s %s\n", MRCONF_STR_ERROR, MRCONF_STR_ALLOC_FAILED);
			break;

		case MRCONF_READING_FAILED:
			log_msg(4, "%s %s\n", MRCONF_STR_ERROR, MRCONF_STR_READING_FAILED);
			break;

		case MRCONF_FIELD_NOT_FOUND:
			log_msg(4, "%s %s \"%s\"\n", MRCONF_STR_ERROR,
				   MRCONF_STR_FIELD_NOT_FOUND, add_line);
			log_msg(4, "%s %s\n", MRCONF_STR_WARNING, MRCONF_STR_SET_TO_ZERO);
			break;

		case MRCONF_FIELD_NO_VALUE:
			log_msg(4, "%s %s \"%s\"\n", MRCONF_STR_ERROR,
				   MRCONF_STR_FIELD_NO_VALUE, add_line);
			log_msg(4, "%s %s\n", MRCONF_STR_WARNING, MRCONF_STR_IGNORE);
			break;

		case MRCONF_CLOSE_BUT_NOT_OPEN:
			log_msg(4, "%s %s\n", MRCONF_STR_WARNING,
				   MRCONF_STR_CLOSE_BUT_NOT_OPEN);
			break;

		case MRCONF_CALL_BUT_NOT_OPEN:
			log_msg(4, "%s %s\n", MRCONF_STR_WARNING,
				   MRCONF_STR_CALL_BUT_NOT_OPEN);
			break;

		case MRCONF_OPEN_OPENED:
			log_msg(4, "%s %s\n", MRCONF_STR_ERROR, MRCONF_STR_OPEN_OPENED);
			break;

		default:
			log_msg(4, "%s %s\n", MRCONF_STR_ERROR, MRCONF_STR_DEFAULT_ERROR);
			break;
		}
	}

	/* if the flag is set to ERROR_EXIT then any error leads to halt */
	if (mrconf_flags & MRCONF_FLAG_ERROR_EXIT) {
		if (mrconf_check_int_flag(MRCONF_INTFLAG_OPEN))
			mrconf_close();
		log_msg(4, "%s\n", MRCONF_STR_HALT);
		exit(error_code);
	}
}


