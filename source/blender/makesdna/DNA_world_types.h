/**
 * blenlib/DNA_world_types.h (mar-2001 nzc)
 *
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */
#ifndef DNA_WORLD_TYPES_H
#define DNA_WORLD_TYPES_H

#include "DNA_ID.h"

struct AnimData;
struct Ipo;
struct MTex;

#ifndef MAX_MTEX
#define MAX_MTEX	18
#endif


/**
 * World defines general modeling data such as a background fill,
 * gravity, color model, stars, etc. It mixes game-data, rendering
 * data and modeling data. */
typedef struct World {
	ID id;
	struct AnimData *adt;	/* animation data (must be immediately after id for utilities to use it) */ 
	
	short colormodel, totex;
	short texact, mistype;
	
	/* TODO - hork, zenk and ambk are not used, should remove at some point (Campbell) */
	float horr, horg, horb, hork;
	float zenr, zeng, zenb, zenk;
	float ambr, ambg, ambb, ambk;


	/* deprecated, keep around for now so old blenders don't mess
	   up rendering completely due to these being 0 */
	float exp, range;	

	/**
	 * Gravitation constant for the game world
	 */
	float gravity; // XXX moved to scene->gamedata in 2.5

	/**
	 * Radius of the activity bubble, in Manhattan length. Objects
	 * outside the box are activity-culled. */
	float activityBoxRadius; // XXX moved to scene->gamedata in 2.5
	
	short skytype;
	/**
	 * Some world modes
	 * bit 0: Do mist
	 * bit 1: Do stars
	 * bit 2: (reserved) depth of field
	 * bit 3: (gameengine): Activity culling is enabled.
	 * bit 4: ambient occlusion
	 * bit 5: (gameengine) : enable Bullet DBVT tree for view frustrum culling 
	 */
	short mode;												// partially moved to scene->gamedata in 2.5
	short occlusionRes;		/* resolution of occlusion Z buffer in pixel */	// XXX moved to scene->gamedata in 2.5
	short physicsEngine;	/* here it's aligned */					// XXX moved to scene->gamedata in 2.5
	short ticrate, maxlogicstep, physubstep, maxphystep;	// XXX moved to scene->gamedata in 2.5
	
	float misi, miststa, mistdist, misthi;
	
	float starr, starg, starb, stark;
	float starsize, starmindist;
	float stardist, starcolnoise;
	
	/* unused now: DOF */
	short dofsta, dofend, dofmin, dofmax;
	
	/* ambient occlusion */
	float aodist, aodistfac, aoenergy, aobias;
	short aomode, aosamp, aomix, aocolor;
	float ao_adapt_thresh, ao_adapt_speed_fac;
	float ao_approx_error, ao_approx_correction;
	float ao_indirect_energy, ao_env_energy, ao_pad2;
	short ao_indirect_bounces, ao_bump_method;
	short ao_samp_method, ao_gather_method, ao_approx_passes;
	
	/* assorted settings (in the middle of ambient occlusion settings for padding reasons) */
	short flag;
	
	/* ambient occlusion (contd...) */
	float *aosphere, *aotables;
	
	
	struct Ipo *ipo;			// XXX depreceated... old animation system
	struct MTex *mtex[18];		/* MAX_MTEX */
	short pr_texture, pad[3];

	/* previews */
	struct PreviewImage *preview;

} World;

/* **************** WORLD ********************* */

/* skytype */
#define WO_SKYBLEND		1
#define WO_SKYREAL		2
#define WO_SKYPAPER		4
/* while render: */
#define WO_SKYTEX		8
#define WO_ZENUP		16

/* mode */
#define WO_MIST	               1
#define WO_STARS               2
#define WO_DOF                 4
#define WO_ACTIVITY_CULLING	   8
#define WO_ENV_LIGHT   		  16
#define WO_DBVT_CULLING		  32
#define WO_AMB_OCC   		  64
#define WO_INDIRECT_LIGHT	  128

/* aomix */
#define WO_AO_ADD		0
#define WO_AO_SUB		1 /* deprecated */
#define WO_AO_ADDSUB	2 /* deprecated */
#define WO_AO_MUL		3

/* ao_samp_method - methods for sampling the AO hemi */
#define WO_LIGHT_SAMP_CONSTANT		0		/* Deprecated */
#define WO_LIGHT_SAMP_HALTON		1
#define WO_LIGHT_SAMP_HAMMERSLEY	2

/* aomode (use distances & random sampling modes) */
#define WO_LIGHT_DIST		1
#define WO_LIGHT_DEPRECATED	2
#define WO_LIGHT_CACHE		4
#define WO_LIGHT_CACHE_FILE	8

/* aocolor */
#define WO_ENV_LIGHT_WHITE		0
#define WO_ENV_LIGHT_SKY_COLOR	1
#define WO_ENV_LIGHT_SKY_TEX	2

/* ao_gather_method */
#define WO_LIGHT_GATHER_RAYTRACE	0
#define WO_LIGHT_GATHER_APPROX		1

/* ao_bump_method */
#define WO_LIGHT_BUMP_NONE		0
#define WO_LIGHT_BUMP_APPROX	1
#define WO_LIGHT_BUMP_FULL		2

/* texco (also in DNA_material_types.h) */
#define TEXCO_ANGMAP	64
#define TEXCO_H_SPHEREMAP	256
#define TEXCO_H_TUBEMAP	1024

/* mapto */
#define WOMAP_BLEND		1
#define WOMAP_HORIZ		2
#define WOMAP_ZENUP		4
#define WOMAP_ZENDOWN	8
#define WOMAP_MIST		16

/* flag */
#define WO_DS_EXPAND	(1<<0)
	/* NOTE: this must have the same value as MA_DS_SHOW_TEXS, 
	 * otherwise anim-editors will not read correctly
	 */
#define WO_DS_SHOW_TEXS	(1<<2)

#endif

