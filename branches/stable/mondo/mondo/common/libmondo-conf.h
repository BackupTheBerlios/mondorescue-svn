/* libmondo_conf.h
 *
 * $Id$
 *
 * based on parse_conf.h (c)2002-2004 Anton Kulchitsky  mailto:anton@kulchitsky.org
 * Review for mondorescue (c) 2006 Bruno Cornec <bruno@mondorescue.org>
 *   
 *     Header file of libmondo-conf (mrconf): a very small and simple
 *     library for mondorescue configuration file reading
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

#ifndef LIBMONDO_CONF_H
#define LIBMONDO_CONF_H

/* functions (public methods) */

/*initialization and closing*/
int mrconf_open(const char *filename);
void mrconf_close();

/*read integer number after string str in the current file*/
int mrconf_iread(const char *field_name);

/*read double/float number after string str in the current file*/
double mrconf_fread(const char *field_name);

/*
 * read string after string str in the current file.
 * This function allocates the string which has to be freed later on 
*/
char *mrconf_sread(const char *field_name);

#endif							/*LIBMONDO_CONF_H */
