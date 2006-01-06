/**
 * blenlib/BKE_global.h (mar-2001 nzc)
 *
 * Global settings, handles, pointers. This is the root for finding
 * any data in Blender. This block is not serialized, but built anew
 * for every fresh Blender run.
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
#ifndef BKE_GLOBAL_H
#define BKE_GLOBAL_H

#include "DNA_listBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forwards */
struct View3D;
struct View2D;
struct SpaceIpo;
struct SpaceButs;
struct SpaceImage;
struct SpaceOops;
struct SpaceText;
struct SpaceSound;
struct SpaceAction;
struct SpaceNla;
struct Main;
struct Scene;
struct bScreen;
struct Object;
struct bSoundListener;
struct BMF_Font;
struct EditMesh;


typedef struct Global {

	/* active pointers */
	struct View3D *vd;
	struct View2D *v2d;
	struct SpaceIpo *sipo;
	struct SpaceButs *buts;
	struct SpaceImage *sima;
	struct SpaceOops *soops;
	struct SpaceSound *ssound;
	struct SpaceAction *saction;		/* __NLA */
	struct SpaceNla *snla;
	struct Main *main;
	struct Scene *scene;			/* denk aan file.c */
	struct bScreen *curscreen;
	struct Object *obedit;
	char editModeTitleExtra[64];
	
	/* fonts, allocated global data */
	struct BMF_Font *font, *fonts, *fontss;
    
	/* strings: lastsaved */
	char ima[160], sce[160], lib[160];
    
	/* totals */
	short totobj, totlamp, totobjsel, totcurve, totmesh;
	short totbone, totbonesel;
	int totvert, totedge, totface, totvertsel, totedgesel, totfacesel;
    
	short afbreek, moving;
	short qual, background;
	short winpos, displaymode;	/* used to be in Render */
	
	/**
	 * The current version of Blender.
	 */
	short version;
	short simulf, order, rt;
	int f;

	/* Editmode lists */
	struct EditMesh *editMesh;
    
	float textcurs[4][2];
    
	/* Frank's variables */
	int	save_over;

	/* Reevan's __NLA variables */
	struct	ListBase edbo;			/* Armature Editmode bones */
 
	/* Rob's variables */
	int have_quicktime;
	int ui_international;
	int charstart;
	int charmin;
	int charmax;
	struct VFont *selfont;
	struct ListBase ttfdata;

	/* libtiff flag */
	int have_libtiff;

	/* this variable is written to / read from FileGlobal->fileflags */
	int fileflags;
    
	/* save the allowed windowstate of blender when using -W or -w */
	int windowstate;

	/* Janco's playing ground */
	struct bSoundListener* listener;

	/* Test thingy for Nzc */
	int compat;      /* toggle compatibility mode for edge rendering */
	int notonlysolid;/* T-> also edge-render transparent faces       */
	
	/* confusing... G.f and G.flags */
	int flags;

} Global;

/* **************** GLOBAL ********************* */

/* G.f */
#define G_DISABLE_OK	(1 <<  0)
#define G_PLAYANIM		(1 <<  1)
/* also uses G_FILE_AUTOPLAY */
#define G_SIMULATION	(1 <<  3)
#define G_BACKBUFSEL	(1 <<  4)
#define G_PICKSEL		(1 <<  5)
#define G_DRAWNORMALS	(1 <<  6)
#define G_DRAWFACES		(1 <<  7)
#define G_FACESELECT	(1 <<  8)
#define G_DRAW_EXT		(1 <<  9)
#define G_VERTEXPAINT	(1 << 10)
#define G_ALLEDGES		(1 << 11)
#define G_DEBUG			(1 << 12)
#define G_DOSCRIPTLINKS (1 << 13)
#define G_DRAW_VNORMALS	(1 << 14)
#define G_WEIGHTPAINT	(1 << 15)	
#define G_TEXTUREPAINT	(1 << 16)
/* #define G_NOFROZEN	(1 << 17) also removed */
#define G_DRAWEDGES		(1 << 18)
#define G_DRAWCREASES	(1 << 19)
#define G_DRAWSEAMS     (1 << 20)
#define G_HIDDENEDGES   (1 << 21)

/* Measurement info Drawing */
#define G_DRAW_EDGELEN  (1 << 22) 
#define G_DRAW_FACEAREA (1 << 23)
#define G_DRAW_EDGEANG  (1 << 24)

#define G_RECORDKEYS	(1 << 25)

/* G.fileflags */

#define G_AUTOPACK               (1 << 0)
#define G_FILE_COMPRESS          (1 << 1)
#define G_FILE_AUTOPLAY          (1 << 2)
#define G_FILE_ENABLE_ALL_FRAMES (1 << 3)
#define G_FILE_SHOW_DEBUG_PROPS  (1 << 4)
#define G_FILE_SHOW_FRAMERATE    (1 << 5)
#define G_FILE_SHOW_PROFILE      (1 << 6)
#define G_FILE_LOCK              (1 << 7)
#define G_FILE_SIGN              (1 << 8)
#define G_FIle_PUBLISH			 (1 << 9)
#define G_FILE_NO_UI			 (1 << 10)
#define G_FILE_GAME_TO_IPO		 (1 << 11)
#define G_FILE_GAME_MAT			 (1 << 12)

/* G.windowstate */
#define G_WINDOWSTATE_USERDEF		0
#define G_WINDOWSTATE_BORDER		1
#define G_WINDOWSTATE_FULLSCREEN	2

/* G.simulf */
#define G_LOADFILE	2
#define G_RESTART	4
#define G_QUIT		8
#define G_SETSCENE	16

/* G.qual */
#define R_SHIFTKEY		1
#define L_SHIFTKEY		2
#define LR_SHIFTKEY 	3
#define R_ALTKEY		4
#define L_ALTKEY		8
#define LR_ALTKEY		12
#define R_CTRLKEY		16
#define L_CTRLKEY		32
#define LR_CTRLKEY  	48
#define LR_COMMANDKEY 	64

/* G.order: indicates what endianness the platform where the file was
 * written had. */
#define L_ENDIAN	1
#define B_ENDIAN	0

/* G.moving, signals drawing in (3d) window to denote transform */
#define G_TRANSFORM_OBJ		1
#define G_TRANSFORM_EDIT	2
#define G_TRANSFORM_MANIP	4

/* G.special1 */

/* Memory is allocated where? blender.c */
extern Global G;

#ifdef __cplusplus
}
#endif
	
#endif

