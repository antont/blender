/**
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
 * Contributor(s): Andrea Weikert (c) 2008 Blender Foundation.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_linklist.h"
#include "BLI_dynstr.h"

#ifdef WIN32
#include <windows.h> /* need to include windows.h so _WIN32_IE is defined  */
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400 /* minimal requirements for SHGetSpecialFolderPath on MINGW MSVC has this defined already */
#endif
#include <shlobj.h> /* for SHGetSpecialFolderPath, has to be done before BLI_winstuff because 'near' is disabled through BLI_windstuff */
#include "BLI_winstuff.h"
#endif

#include "fsmenu.h"  /* include ourselves */


/* FSMENU HANDLING */

	/* FSMenuEntry's without paths indicate seperators */
typedef struct _FSMenuEntry FSMenuEntry;
struct _FSMenuEntry {
	FSMenuEntry *next;

	char *path;
	short save;
	short xs, ys;
};

static FSMenuEntry *fsmenu_system= 0;
static FSMenuEntry *fsmenu_bookmarks= 0;
static FSMenuEntry *fsmenu_recent= 0;

static FSMenuCategory selected_category= FS_CATEGORY_SYSTEM;
static int selected_entry= 0;

void fsmenu_select_entry(FSMenuCategory category, int index)
{
	selected_category = category;
	selected_entry = index;
}

int	fsmenu_is_selected(FSMenuCategory category, int index)
{
	return (category==selected_category) && (index==selected_entry);
}

static FSMenuEntry *fsmenu_get(FSMenuCategory category)
{
	FSMenuEntry *fsmenu = NULL;

	switch(category) {
		case FS_CATEGORY_SYSTEM:
			fsmenu = fsmenu_system;
			break;
		case FS_CATEGORY_BOOKMARKS:
			fsmenu = fsmenu_bookmarks;
			break;
		case FS_CATEGORY_RECENT:
			fsmenu = fsmenu_recent;
			break;
	}
	return fsmenu;
}

static void fsmenu_set(FSMenuCategory category, FSMenuEntry *fsmenu)
{
	switch(category) {
		case FS_CATEGORY_SYSTEM:
			fsmenu_system = fsmenu;
			break;
		case FS_CATEGORY_BOOKMARKS:
			fsmenu_bookmarks = fsmenu;
			break;
		case FS_CATEGORY_RECENT:
			fsmenu_recent = fsmenu;
			break;
	}
}

int fsmenu_get_nentries(FSMenuCategory category)
{
	FSMenuEntry *fsme;
	int count= 0;

	for (fsme= fsmenu_get(category); fsme; fsme= fsme->next) 
		count++;

	return count;
}

char *fsmenu_get_entry(FSMenuCategory category, int idx)
{
	FSMenuEntry *fsme;

	for (fsme= fsmenu_get(category); fsme && idx; fsme= fsme->next)
		idx--;

	return fsme?fsme->path:NULL;
}

void	fsmenu_set_pos( FSMenuCategory category, int idx, short xs, short ys)
{
	FSMenuEntry *fsme;

	for (fsme= fsmenu_get(category); fsme && idx; fsme= fsme->next)
		idx--;

	if (fsme) {
		fsme->xs = xs;
		fsme->ys = ys;
	}
}

int	fsmenu_get_pos (FSMenuCategory category, int idx, short* xs, short* ys)
{
	FSMenuEntry *fsme;

	for (fsme= fsmenu_get(category); fsme && idx; fsme= fsme->next)
		idx--;

	if (fsme) {
		*xs = fsme->xs;
		*ys = fsme->ys;
		return 1;
	}

	return 0;
}


void fsmenu_insert_entry(FSMenuCategory category, char *path, int sorted, short save)
{
	FSMenuEntry *prev;
	FSMenuEntry *fsme;
	FSMenuEntry *fsmenu;

	fsmenu = fsmenu_get(category);
	prev= fsme= fsmenu;

	for (; fsme; prev= fsme, fsme= fsme->next) {
		if (fsme->path) {
			if (BLI_streq(path, fsme->path)) {
				return;
			} else if (sorted && strcmp(path, fsme->path)<0) {
				break;
			}
		} else {
			// if we're bookmarking this, file should come 
			// before the last separator, only automatically added
			// current dir go after the last sep.
			if (save) {
				break;
			}
		}
	}
	
	fsme= MEM_mallocN(sizeof(*fsme), "fsme");
	fsme->path= BLI_strdup(path);
	fsme->save = save;

	if (prev) {
		fsme->next= prev->next;
		prev->next= fsme;
	} else {
		fsme->next= fsmenu;
		fsmenu_set(category, fsme);
	}
}

void fsmenu_remove_entry(FSMenuCategory category, int idx)
{
	FSMenuEntry *prev= NULL, *fsme= NULL;
	FSMenuEntry *fsmenu = fsmenu_get(category);

	for (fsme= fsmenu; fsme && idx; prev= fsme, fsme= fsme->next)		
		idx--;

	if (fsme) {
		/* you should only be able to remove entries that were 
		   not added by default, like windows drives.
		   also separators (where path == NULL) shouldn't be removed */
		if (fsme->save && fsme->path) {

			/* remove fsme from list */
			if (prev) {
				prev->next= fsme->next;
			} else {
				fsmenu= fsme->next;
				fsmenu_set(category, fsmenu);
			}
			/* free entry */
			MEM_freeN(fsme->path);
			MEM_freeN(fsme);
		}
	}
}

void fsmenu_write_file(const char *filename)
{
	FSMenuEntry *fsme= NULL;

	FILE *fp = fopen(filename, "w");
	if (!fp) return;

	for (fsme= fsmenu_get(FS_CATEGORY_BOOKMARKS); fsme; fsme= fsme->next) {
		if (fsme->path && fsme->save) {
			fprintf(fp, "%s\n", fsme->path);
		}
	}
	fclose(fp);
}

void fsmenu_read_file(const char *filename)
{
	char line[256];
	FILE *fp;

	#ifdef WIN32
	/* Add the drive names to the listing */
	{
		__int64 tmp;
		char folder[256];
		char tmps[4];
		int i;
			
		tmp= GetLogicalDrives();
		
		for (i=2; i < 26; i++) {
			if ((tmp>>i) & 1) {
				tmps[0]='a'+i;
				tmps[1]=':';
				tmps[2]='\\';
				tmps[3]=0;
				
				fsmenu_insert_entry(FS_CATEGORY_SYSTEM, tmps, 1, 0);
			}
		}

		/* Adding Desktop and My Documents */
		SHGetSpecialFolderPath(0, folder, CSIDL_PERSONAL, 0);
		fsmenu_insert_entry(FS_CATEGORY_BOOKMARKS, folder, 1, 0);
		SHGetSpecialFolderPath(0, folder, CSIDL_DESKTOPDIRECTORY, 0);
		fsmenu_insert_entry(FS_CATEGORY_BOOKMARKS, folder, 1, 0);
	}
#endif

	fp = fopen(filename, "r");
	if (!fp) return;

	while ( fgets ( line, 256, fp ) != NULL ) /* read a line */
	{
		int len = strlen(line);
		if (len>0) {
			if (line[len-1] == '\n') {
				line[len-1] = '\0';
			}
			fsmenu_insert_entry(FS_CATEGORY_BOOKMARKS, line, 0, 1);
		}
	}
	fclose(fp);
}

static void fsmenu_free_category(FSMenuCategory category)
{
	FSMenuEntry *fsme= fsmenu_get(category);

	while (fsme) {
		FSMenuEntry *n= fsme->next;

		if (fsme->path) MEM_freeN(fsme->path);
		MEM_freeN(fsme);

		fsme= n;
	}
}

void fsmenu_free(void)
{
	fsmenu_free_category(FS_CATEGORY_SYSTEM);
	fsmenu_free_category(FS_CATEGORY_BOOKMARKS);
	fsmenu_free_category(FS_CATEGORY_RECENT);
}

