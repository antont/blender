/**
 * blenlib/BKE_library.h (mar-2001 nzc)
 *	
 * Library
 *
 * $Id$ 
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */
#ifndef BKE_LIBRARY_TYPES_H
#define BKE_LIBRARY_TYPES_H

struct ListBase;
struct ID;
struct Main;
struct Library;

void *alloc_libblock(struct ListBase *lb, short type, char *name);
void *copy_libblock(void *rt);

void id_lib_extern(struct ID *id);
void id_us_plus(struct ID *id);

int new_id(struct ListBase *lb, struct ID *id, char *name);

struct ListBase *wich_libbase(struct Main *mainlib, short type);
int set_listbasepointers(struct Main *main, struct ListBase **lb);

void free_libblock(struct ListBase *lb, void *idv);
void free_libblock_us(struct ListBase *lb, void *idv);
void free_main(struct Main *mainvar);

void splitIDname(char *name, char *left, int *nr);
void rename_id(struct ID *id, char *name);
void test_idbutton(char *name);
void all_local(Library *lib);
struct ID *find_id(char *type, char *name);
void clear_id_newpoins(void);

void IDnames_to_pupstring(char **str, char *title, char *extraops, struct ListBase *lb,struct ID* link, short *nr);
void IPOnames_to_pupstring(char **str, char *title, char *extraops, struct ListBase *lb, struct ID* link, short *nr, int blocktype);

#endif

