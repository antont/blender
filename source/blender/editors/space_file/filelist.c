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
 * The Original Code is Copyright (C) 2007 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */


/* global includes */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#endif   
#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_linklist.h"
#include "BLI_storage_types.h"
#include "BLI_threads.h"

#ifdef WIN32
#include "BLI_winstuff.h"
#endif

#include "BKE_utildefines.h"
#include "BKE_global.h"
#include "BKE_library.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BLO_readfile.h"

#include "DNA_space_types.h"
#include "DNA_ipo_types.h"
#include "DNA_ID.h"
#include "DNA_object_types.h"
#include "DNA_listBase.h"
#include "DNA_lamp_types.h"
#include "DNA_material_types.h"
#include "DNA_texture_types.h"
#include "DNA_world_types.h"
#include "DNA_scene_types.h"
#include "DNA_userdef_types.h"

#include "ED_datafiles.h"

#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"
#include "IMB_thumbs.h"

#include "PIL_time.h"

#include "UI_text.h"

#include "filelist.h"

/* Elubie: VERY, really very ugly and evil! Remove asap!!! */
/* for state of file */
#define ACTIVE				2

/* max length of library group name within filesel */
#define GROUP_MAX 32

static void *exec_loadimages(void *list_v);

struct FileList;

typedef struct FileImage {
	struct FileImage *next, *prev;
	int index;
	short lock;
	short done;
	struct FileList* filelist;
} FileImage;

typedef struct FileList
{
	struct direntry *filelist;
	int *fidx;

	int numfiles;
	int numfiltered;
	char dir[FILE_MAX];
	short type;
	int has_func;
	short prv_w;
	short prv_h;
	short hide_dot;
	unsigned int filter;
	short changed;
	int maxnamelen;
	ListBase loadimages;
	ListBase threads;
} FileList;

#define SPECIAL_IMG_SIZE 48
#define SPECIAL_IMG_ROWS 4
#define SPECIAL_IMG_COLS 4

#define SPECIAL_IMG_FOLDER 0
#define SPECIAL_IMG_PARENT 1
#define SPECIAL_IMG_REFRESH 2
#define SPECIAL_IMG_BLENDFILE 3
#define SPECIAL_IMG_SOUNDFILE 4
#define SPECIAL_IMG_MOVIEFILE 5
#define SPECIAL_IMG_PYTHONFILE 6
#define SPECIAL_IMG_TEXTFILE 7
#define SPECIAL_IMG_FONTFILE 8
#define SPECIAL_IMG_UNKNOWNFILE 9
#define SPECIAL_IMG_LOADING 10
#define SPECIAL_IMG_MAX SPECIAL_IMG_LOADING + 1

static ImBuf* gSpecialFileImages[SPECIAL_IMG_MAX];


/* ******************* SORT ******************* */

static int compare_name(const void *a1, const void *a2)
{
	const struct direntry *entry1=a1, *entry2=a2;

	/* type is is equal to stat.st_mode */

	if (S_ISDIR(entry1->type)){
		if (S_ISDIR(entry2->type)==0) return (-1);
	} else{
		if (S_ISDIR(entry2->type)) return (1);
	}
	if (S_ISREG(entry1->type)){
		if (S_ISREG(entry2->type)==0) return (-1);
	} else{
		if (S_ISREG(entry2->type)) return (1);
	}
	if ((entry1->type & S_IFMT) < (entry2->type & S_IFMT)) return (-1);
	if ((entry1->type & S_IFMT) > (entry2->type & S_IFMT)) return (1);
	
	/* make sure "." and ".." are always first */
	if( strcmp(entry1->relname, ".")==0 ) return (-1);
	if( strcmp(entry2->relname, ".")==0 ) return (1);
	if( strcmp(entry1->relname, "..")==0 ) return (-1);
	if( strcmp(entry2->relname, "..")==0 ) return (1);
	
	return (BLI_strcasecmp(entry1->relname,entry2->relname));
}

static int compare_date(const void *a1, const void *a2)	
{
	const struct direntry *entry1=a1, *entry2=a2;
	
	/* type is equal to stat.st_mode */

	if (S_ISDIR(entry1->type)){
		if (S_ISDIR(entry2->type)==0) return (-1);
	} else{
		if (S_ISDIR(entry2->type)) return (1);
	}
	if (S_ISREG(entry1->type)){
		if (S_ISREG(entry2->type)==0) return (-1);
	} else{
		if (S_ISREG(entry2->type)) return (1);
	}
	if ((entry1->type & S_IFMT) < (entry2->type & S_IFMT)) return (-1);
	if ((entry1->type & S_IFMT) > (entry2->type & S_IFMT)) return (1);

	/* make sure "." and ".." are always first */
	if( strcmp(entry1->relname, ".")==0 ) return (-1);
	if( strcmp(entry2->relname, ".")==0 ) return (1);
	if( strcmp(entry1->relname, "..")==0 ) return (-1);
	if( strcmp(entry2->relname, "..")==0 ) return (1);
	
	if ( entry1->s.st_mtime < entry2->s.st_mtime) return 1;
	if ( entry1->s.st_mtime > entry2->s.st_mtime) return -1;
	
	else return BLI_strcasecmp(entry1->relname,entry2->relname);
}

static int compare_size(const void *a1, const void *a2)	
{
	const struct direntry *entry1=a1, *entry2=a2;

	/* type is equal to stat.st_mode */

	if (S_ISDIR(entry1->type)){
		if (S_ISDIR(entry2->type)==0) return (-1);
	} else{
		if (S_ISDIR(entry2->type)) return (1);
	}
	if (S_ISREG(entry1->type)){
		if (S_ISREG(entry2->type)==0) return (-1);
	} else{
		if (S_ISREG(entry2->type)) return (1);
	}
	if ((entry1->type & S_IFMT) < (entry2->type & S_IFMT)) return (-1);
	if ((entry1->type & S_IFMT) > (entry2->type & S_IFMT)) return (1);

	/* make sure "." and ".." are always first */
	if( strcmp(entry1->relname, ".")==0 ) return (-1);
	if( strcmp(entry2->relname, ".")==0 ) return (1);
	if( strcmp(entry1->relname, "..")==0 ) return (-1);
	if( strcmp(entry2->relname, "..")==0 ) return (1);
	
	if ( entry1->s.st_size < entry2->s.st_size) return 1;
	if ( entry1->s.st_size > entry2->s.st_size) return -1;
	else return BLI_strcasecmp(entry1->relname,entry2->relname);
}

static int compare_extension(const void *a1, const void *a2) {
	const struct direntry *entry1=a1, *entry2=a2;
	char *sufix1, *sufix2;
	char *nil="";

	if (!(sufix1= strstr (entry1->relname, ".blend.gz"))) 
		sufix1= strrchr (entry1->relname, '.');
	if (!(sufix2= strstr (entry2->relname, ".blend.gz")))
		sufix2= strrchr (entry2->relname, '.');
	if (!sufix1) sufix1= nil;
	if (!sufix2) sufix2= nil;

	/* type is is equal to stat.st_mode */

	if (S_ISDIR(entry1->type)){
		if (S_ISDIR(entry2->type)==0) return (-1);
	} else{
		if (S_ISDIR(entry2->type)) return (1);
	}
	if (S_ISREG(entry1->type)){
		if (S_ISREG(entry2->type)==0) return (-1);
	} else{
		if (S_ISREG(entry2->type)) return (1);
	}
	if ((entry1->type & S_IFMT) < (entry2->type & S_IFMT)) return (-1);
	if ((entry1->type & S_IFMT) > (entry2->type & S_IFMT)) return (1);
	
	/* make sure "." and ".." are always first */
	if( strcmp(entry1->relname, ".")==0 ) return (-1);
	if( strcmp(entry2->relname, ".")==0 ) return (1);
	if( strcmp(entry1->relname, "..")==0 ) return (-1);
	if( strcmp(entry2->relname, "..")==0 ) return (1);
	
	return (BLI_strcasecmp(sufix1, sufix2));
}

void filelist_filter(FileList* filelist)
{
	char dir[FILE_MAX], group[GROUP_MAX];
	int num_filtered = 0;
	int i, j;
	
	if (!filelist->filelist)
		return;
	
	if (!filelist->filter) {
		if (filelist->fidx) {
			MEM_freeN(filelist->fidx);
			filelist->fidx = NULL;
		}
		filelist->fidx = (int *)MEM_callocN(filelist->numfiles*sizeof(int), "filteridx");
		for (i = 0; i < filelist->numfiles; ++i) {
			filelist->fidx[i] = i;
		}
		filelist->numfiltered = filelist->numfiles;
		return;
	}

	// How many files are left after filter ?
	for (i = 0; i < filelist->numfiles; ++i) {
		if (filelist->filelist[i].flags & filelist->filter) {
			num_filtered++;
		} 
		else if (filelist->filelist[i].type & S_IFDIR) {
			if (filelist->filter & FOLDERFILE) {
				num_filtered++;
			}
		}		
	}
	
	if (filelist->fidx) {
			MEM_freeN(filelist->fidx);
			filelist->fidx = NULL;
	}
	filelist->fidx = (int *)MEM_callocN(num_filtered*sizeof(int), "filteridx");
	filelist->numfiltered = num_filtered;

	for (i = 0, j=0; i < filelist->numfiles; ++i) {
		if (filelist->filelist[i].flags & filelist->filter) {
			filelist->fidx[j++] = i;
		}
		else if (filelist->filelist[i].type & S_IFDIR) {
			if (filelist->filter & FOLDERFILE) {
				filelist->fidx[j++] = i;
			}
		}  
	}
}

void filelist_init_icons()
{
	short x, y, k;
	ImBuf *bbuf;
	ImBuf *ibuf;
	bbuf = IMB_ibImageFromMemory((int *)datatoc_prvicons, datatoc_prvicons_size, IB_rect);
	if (bbuf) {
		for (y=0; y<SPECIAL_IMG_ROWS; y++) {
			for (x=0; x<SPECIAL_IMG_COLS; x++) {
				int tile = SPECIAL_IMG_COLS*y + x; 
				if (tile < SPECIAL_IMG_MAX) {
					ibuf = IMB_allocImBuf(SPECIAL_IMG_SIZE, SPECIAL_IMG_SIZE, 32, IB_rect, 0);
					for (k=0; k<SPECIAL_IMG_SIZE; k++) {
						memcpy(&ibuf->rect[k*SPECIAL_IMG_SIZE], &bbuf->rect[(k+y*SPECIAL_IMG_SIZE)*SPECIAL_IMG_SIZE*SPECIAL_IMG_COLS+x*SPECIAL_IMG_SIZE], SPECIAL_IMG_SIZE*sizeof(int));
					}
					gSpecialFileImages[tile] = ibuf;
				}
			}
		}
		IMB_freeImBuf(bbuf);
	}
}

void filelist_free_icons()
{
	int i;
	for (i=0; i < SPECIAL_IMG_MAX; ++i) {
		IMB_freeImBuf(gSpecialFileImages[i]);
		gSpecialFileImages[i] = NULL;
	}
}

struct FileList*	filelist_new()
{
	FileList* p = MEM_callocN( sizeof(FileList), "filelist" );
	p->filelist = 0;
	p->numfiles = 0;
	p->dir[0] = '\0';
	p->type = 0;
	p->has_func = 0;
	p->filter = 0;
	return p;
}

struct FileList*	filelist_copy(struct FileList* filelist)
{
	FileList* p = filelist_new();
	BLI_strncpy(p->dir, filelist->dir, FILE_MAX);
	p->filelist = NULL;
	p->fidx = NULL;
	p->type = filelist->type;

	return p;
}

void filelist_free(struct FileList* filelist)
{
	int i;

	if (!filelist) {
		printf("Attemtping to delete empty filelist.\n");
		return;
	}

	BLI_end_threads(&filelist->threads);
	BLI_freelistN(&filelist->loadimages);
	
	if (filelist->fidx) {
		MEM_freeN(filelist->fidx);
		filelist->fidx = NULL;
	}

	for (i = 0; i < filelist->numfiles; ++i) {
		if (filelist->filelist[i].image) {			
			IMB_freeImBuf(filelist->filelist[i].image);
		}
		filelist->filelist[i].image = 0;
		if (filelist->filelist[i].relname)
			MEM_freeN(filelist->filelist[i].relname);
		filelist->filelist[i].relname = 0;
		if (filelist->filelist[i].string)
			MEM_freeN(filelist->filelist[i].string);
		filelist->filelist[i].string = 0;
	}
	
	filelist->numfiles = 0;
	free(filelist->filelist);
	filelist->filelist = 0;	
	filelist->filter = 0;
	filelist->numfiltered =0;
}

int	filelist_numfiles(struct FileList* filelist)
{
	return filelist->numfiltered;
}

const char * filelist_dir(struct FileList* filelist)
{
	return filelist->dir;
}

void filelist_setdir(struct FileList* filelist, const char *dir)
{
	BLI_strncpy(filelist->dir, dir, FILE_MAX);
}

void filelist_imgsize(struct FileList* filelist, short w, short h)
{
	filelist->prv_w = w;
	filelist->prv_h = h;
}


static void *exec_loadimages(void *list_v)
{
	FileImage* img = (FileImage*)list_v;
	struct FileList *filelist = img->filelist;

	ImBuf *imb = NULL;
	int fidx = img->index;
	
	if ( filelist->filelist[fidx].flags & IMAGEFILE ) {				
		imb = IMB_thumb_manage(filelist->dir, filelist->filelist[fidx].relname, THB_NORMAL, THB_SOURCE_IMAGE);
	} else if ( filelist->filelist[fidx].flags & MOVIEFILE ) {				
		imb = IMB_thumb_manage(filelist->dir, filelist->filelist[fidx].relname, THB_NORMAL, THB_SOURCE_MOVIE);
		if (!imb) {
			/* remember that file can't be loaded via IMB_open_anim */
			filelist->filelist[fidx].flags &= ~MOVIEFILE;
			filelist->filelist[fidx].flags |= MOVIEFILE_ICON;
		}
	}
	if (imb) {
		IMB_freeImBuf(imb);
	}
	img->done=1;
	return 0;
}

short filelist_changed(struct FileList* filelist)
{
	return filelist->changed;
}

void filelist_loadimage_timer(struct FileList* filelist)
{
	FileImage *limg = filelist->loadimages.first;
	short refresh=0;

	// as long as threads are available and there is work to do
	while (limg) {
		if (BLI_available_threads(&filelist->threads)>0) {
			if (!limg->lock) {
				limg->lock=1;
				BLI_insert_thread(&filelist->threads, limg);
			}
		}
		if (limg->done) {
			FileImage *oimg = limg;
			BLI_remlink(&filelist->loadimages, oimg);
			BLI_remove_thread(&filelist->threads, oimg);
			limg = oimg->next;
			MEM_freeN(oimg);
			refresh = 1;
		} else {
			limg= limg->next;
		}
	}
	filelist->changed=refresh;
}

void filelist_loadimage(struct FileList* filelist, int index)
{
	ImBuf *imb = NULL;
	int imgwidth = filelist->prv_w;
	int imgheight = filelist->prv_h;
	short ex, ey, dx, dy;
	float scaledx, scaledy;
	int fidx = 0;
	
	if ( (index < 0) || (index >= filelist->numfiltered) ) {
		return;
	}
	fidx = filelist->fidx[index];

	if (!filelist->filelist[fidx].image)
	{
		if (filelist->type != FILE_MAIN)
		{
			if ( (filelist->filelist[fidx].flags & IMAGEFILE) || (filelist->filelist[fidx].flags & MOVIEFILE) ) {				
				imb = IMB_thumb_read(filelist->dir, filelist->filelist[fidx].relname, THB_NORMAL);
			} 
			if (imb) {
				if (imb->x > imb->y) {
					scaledx = (float)imgwidth;
					scaledy =  ( (float)imb->y/(float)imb->x )*imgwidth;
				}
				else {
					scaledy = (float)imgheight;
					scaledx =  ( (float)imb->x/(float)imb->y )*imgheight;
				}
				ex = (short)scaledx;
				ey = (short)scaledy;
				
				dx = imgwidth - ex;
				dy = imgheight - ey;
				
				IMB_scaleImBuf(imb, ex, ey);
				filelist->filelist[fidx].image = imb;
			} else {
				/* prevent loading image twice */
				FileImage* limg = filelist->loadimages.first;
				short found= 0;
				while(limg) {
					if (limg->index == fidx) {
						found= 1;
						break;
					}
					limg= limg->next;
				}
				if (!found) {
					FileImage* limg = MEM_callocN(sizeof(struct FileImage), "loadimage");
					limg->index= fidx;
					limg->lock= 0;
					limg->filelist= filelist;
					BLI_addtail(&filelist->loadimages, limg);
				}
			}		
		}
	}
}

struct ImBuf * filelist_getimage(struct FileList* filelist, int index)
{
	ImBuf* ibuf = NULL;
	int fidx = 0;	
	if ( (index < 0) || (index >= filelist->numfiltered) ) {
		return NULL;
	}
	fidx = filelist->fidx[index];
	ibuf = filelist->filelist[fidx].image;

	return ibuf;
}

struct ImBuf * filelist_geticon(struct FileList* filelist, int index)
{
	ImBuf* ibuf= NULL;
	struct direntry *file= NULL;
	int fidx = 0;	
	if ( (index < 0) || (index >= filelist->numfiltered) ) {
		return NULL;
	}
	fidx = filelist->fidx[index];
	file = &filelist->filelist[fidx];
	if (file->type & S_IFDIR) {
			if ( strcmp(filelist->filelist[fidx].relname, "..") == 0) {
				ibuf = gSpecialFileImages[SPECIAL_IMG_PARENT];
			} else if  ( strcmp(filelist->filelist[fidx].relname, ".") == 0) {
				ibuf = gSpecialFileImages[SPECIAL_IMG_REFRESH];
			} else {
		ibuf = gSpecialFileImages[SPECIAL_IMG_FOLDER];
			}
	} else {
		ibuf = gSpecialFileImages[SPECIAL_IMG_UNKNOWNFILE];
	}

	if (file->flags & BLENDERFILE) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_BLENDFILE];
	} else if ( (file->flags & MOVIEFILE) || (file->flags & MOVIEFILE_ICON) ) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_MOVIEFILE];
	} else if (file->flags & SOUNDFILE) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_SOUNDFILE];
	} else if (file->flags & PYSCRIPTFILE) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_PYTHONFILE];
	} else if (file->flags & FTFONTFILE) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_FONTFILE];
	} else if (file->flags & TEXTFILE) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_TEXTFILE];
	} else if (file->flags & IMAGEFILE) {
		ibuf = gSpecialFileImages[SPECIAL_IMG_LOADING];
	}

	return ibuf;
}

struct direntry * filelist_file(struct FileList* filelist, int index)
{
	int fidx = 0;
	
	if ( (index < 0) || (index >= filelist->numfiltered) ) {
		return NULL;
	}
	fidx = filelist->fidx[index];

	return &filelist->filelist[fidx];
}

int filelist_find(struct FileList* filelist, char *file)
{
	int index = -1;
	int i;
	int fidx = -1;
	
	if (!filelist->fidx) 
		return fidx;

	
	for (i = 0; i < filelist->numfiles; ++i) {
		if ( strcmp(filelist->filelist[i].relname, file) == 0) {
			index = i;
			break;
		}
	}

	for (i = 0; i < filelist->numfiltered; ++i) {
		if (filelist->fidx[i] == index) {
			fidx = i;
			break;
		}
	}
	return fidx;
}

void filelist_hidedot(struct FileList* filelist, short hide)
{
	filelist->hide_dot = hide;
}

void filelist_setfilter(struct FileList* filelist, unsigned int filter)
{
	filelist->filter = filter;
}

int	filelist_maxnamelen(struct FileList* filelist)
{
	return filelist->maxnamelen;
}

void filelist_readdir(struct FileList* filelist)
{
	char wdir[FILE_MAX];
	int finished = 0;
	int i;

	if (!filelist) return;
	filelist->fidx = 0;
	filelist->filelist = 0;

	BLI_getwdN(wdir);	 
	
	BLI_cleanup_dir(G.sce, filelist->dir);
	BLI_hide_dot_files(filelist->hide_dot);
	filelist->numfiles = BLI_getdir(filelist->dir, &(filelist->filelist));

	chdir(wdir);
	filelist_setfiletypes(filelist, G.have_quicktime);
	filelist_filter(filelist);
	
	if (!filelist->threads.first) {
		BLI_init_threads(&filelist->threads, exec_loadimages, 2);
	}

	filelist->maxnamelen = 0;
	for (i=0; (i < filelist->numfiles); ++i)
	{
		struct direntry* file = filelist_file(filelist, i);	
		int len = UI_GetStringWidth(G.font, file->relname,0)+UI_GetStringWidth(G.font, file->size,0);
		if (len > filelist->maxnamelen) filelist->maxnamelen = len;
	}
}

int filelist_empty(struct FileList* filelist)
{	
	return filelist->filelist == 0;
}

void filelist_parent(struct FileList* filelist)
{
	BLI_parent_dir(filelist->dir);
	BLI_make_exist(filelist->dir);
	filelist_readdir(filelist);
}

void filelist_setfiletypes(struct FileList* filelist, short has_quicktime)
{
	struct direntry *file;
	int num;

	file= filelist->filelist;

	for(num=0; num<filelist->numfiles; num++, file++) {
		file->flags= 0;
		file->type= file->s.st_mode;	/* restore the mess below */ 

			/* Don't check extensions for directories */ 
		if (file->type & S_IFDIR)
			continue;
				
		
		
		if(BLO_has_bfile_extension(file->relname)) {
			file->flags |= BLENDERFILE;
			if(filelist->type==FILE_LOADLIB) {		
				char name[FILE_MAXDIR+FILE_MAXFILE];
				BLI_strncpy(name, filelist->dir, sizeof(name));
				strcat(name, file->relname);
				
				/* prevent current file being used as acceptable dir */
				if (BLI_streq(G.main->name, name)==0) {
					file->type &= ~S_IFMT;
					file->type |= S_IFDIR;
				}
			}
		} else if(BLI_testextensie(file->relname, ".py")) {
				file->flags |= PYSCRIPTFILE;
		} else if(BLI_testextensie(file->relname, ".txt")) {
				file->flags |= TEXTFILE;
		} else if( BLI_testextensie(file->relname, ".ttf")
					|| BLI_testextensie(file->relname, ".ttc")
					|| BLI_testextensie(file->relname, ".pfb")
					|| BLI_testextensie(file->relname, ".otf")
					|| BLI_testextensie(file->relname, ".otc")) {
				file->flags |= FTFONTFILE;			
		} else if (has_quicktime){
			if(		BLI_testextensie(file->relname, ".int")
				||  BLI_testextensie(file->relname, ".inta")
				||  BLI_testextensie(file->relname, ".jpg")
#ifdef WITH_OPENJPEG
				||  BLI_testextensie(file->relname, ".jp2")
#endif
				||	BLI_testextensie(file->relname, ".jpeg")
				||	BLI_testextensie(file->relname, ".tga")
				||	BLI_testextensie(file->relname, ".rgb")
				||	BLI_testextensie(file->relname, ".rgba")
				||	BLI_testextensie(file->relname, ".bmp")
				||	BLI_testextensie(file->relname, ".png")
				||	BLI_testextensie(file->relname, ".iff")
				||	BLI_testextensie(file->relname, ".lbm")
				||	BLI_testextensie(file->relname, ".gif")
				||	BLI_testextensie(file->relname, ".psd")
				||	BLI_testextensie(file->relname, ".tif")
				||	BLI_testextensie(file->relname, ".tiff")
				||	BLI_testextensie(file->relname, ".pct")
				||	BLI_testextensie(file->relname, ".pict")
				||	BLI_testextensie(file->relname, ".pntg") //macpaint
				||	BLI_testextensie(file->relname, ".qtif")
				||	BLI_testextensie(file->relname, ".sgi")
				||	BLI_testextensie(file->relname, ".hdr")
#ifdef WITH_DDS
				||	BLI_testextensie(file->relname, ".dds")
#endif
#ifdef WITH_OPENEXR
				||	BLI_testextensie(file->relname, ".exr")
#endif
			    ) {
				file->flags |= IMAGEFILE;			
			}
			else if(BLI_testextensie(file->relname, ".avi")
				||	BLI_testextensie(file->relname, ".flc")
				||	BLI_testextensie(file->relname, ".mov")
				||	BLI_testextensie(file->relname, ".movie")
				||	BLI_testextensie(file->relname, ".mp4")
				||	BLI_testextensie(file->relname, ".m4v")
				||	BLI_testextensie(file->relname, ".mv")) {
				file->flags |= MOVIEFILE;			
			}
			else if(BLI_testextensie(file->relname, ".wav")) {
				file->flags |= SOUNDFILE;
			}
		} else { // no quicktime
			if(BLI_testextensie(file->relname, ".int")
				||	BLI_testextensie(file->relname, ".inta")
				||	BLI_testextensie(file->relname, ".jpg")
				||  BLI_testextensie(file->relname, ".jpeg")
#ifdef WITH_OPENJPEG
				||  BLI_testextensie(file->relname, ".jp2")
#endif
				||	BLI_testextensie(file->relname, ".tga")
				||	BLI_testextensie(file->relname, ".rgb")
				||	BLI_testextensie(file->relname, ".rgba")
				||	BLI_testextensie(file->relname, ".bmp")
				||	BLI_testextensie(file->relname, ".png")
				||	BLI_testextensie(file->relname, ".iff")
				||	BLI_testextensie(file->relname, ".tif")
				||	BLI_testextensie(file->relname, ".tiff")
				||	BLI_testextensie(file->relname, ".hdr")
#ifdef WITH_DDS
				||	BLI_testextensie(file->relname, ".dds")
#endif
#ifdef WITH_OPENEXR
				||	BLI_testextensie(file->relname, ".exr")
#endif
				||	BLI_testextensie(file->relname, ".lbm")
				||	BLI_testextensie(file->relname, ".sgi")) {
				file->flags |= IMAGEFILE;			
			}
			else if(BLI_testextensie(file->relname, ".avi")
				||	BLI_testextensie(file->relname, ".mp4")
				||	BLI_testextensie(file->relname, ".mv")) {
				file->flags |= MOVIEFILE;			
			}
			else if(BLI_testextensie(file->relname, ".wav")) {
				file->flags |= SOUNDFILE;
			}
		}
	}
}

void filelist_swapselect(struct FileList* filelist)
{
	struct direntry *file;
	int num, act= 0;
	
	file= filelist->filelist;
	for(num=0; num<filelist->numfiles; num++, file++) {
		if(file->flags & ACTIVE) {
			act= 1;
			break;
		}
	}
	file= filelist->filelist+2;
	for(num=2; num<filelist->numfiles; num++, file++) {
		if(act) file->flags &= ~ACTIVE;
		else file->flags |= ACTIVE;
	}
}

void filelist_settype(struct FileList* filelist, int type)
{
	filelist->type = type;
}

short filelist_gettype(struct FileList* filelist)
{
	return filelist->type;
}

void filelist_sort(struct FileList* filelist, short sort)
{
	struct direntry *file;
	int num;/*  , act= 0; */

	switch(sort) {
	case FILE_SORTALPHA:
		qsort(filelist->filelist, filelist->numfiles, sizeof(struct direntry), compare_name);	
		break;
	case FILE_SORTDATE:
		qsort(filelist->filelist, filelist->numfiles, sizeof(struct direntry), compare_date);	
		break;
	case FILE_SORTSIZE:
		qsort(filelist->filelist, filelist->numfiles, sizeof(struct direntry), compare_size);	
		break;
	case FILE_SORTEXTENS:
		qsort(filelist->filelist, filelist->numfiles, sizeof(struct direntry), compare_extension);	
	}

	file= filelist->filelist;
	for(num=0; num<filelist->numfiles; num++, file++) {
		file->flags &= ~HILITE;
	}
	filelist_filter(filelist);
}
