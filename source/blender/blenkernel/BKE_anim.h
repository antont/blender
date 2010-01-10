/**
 * blenlib/BKE_anim.h (mar-2001 nzc);
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#ifndef BKE_ANIM_H
#define BKE_ANIM_H

#define MAX_DUPLI_RECUR 8

struct Path;
struct Object;
struct PartEff;
struct Scene;
struct ListBase;
struct bAnimVizSettings;
struct bMotionPath;
struct bPoseChannel;

#include "DNA_object_types.h"

/* ---------------------------------------------------- */
/* Animation Visualisation */

void animviz_settings_init(struct bAnimVizSettings *avs);

void animviz_free_motionpath_cache(struct bMotionPath *mpath);
void animviz_free_motionpath(struct bMotionPath *mpath);

struct bMotionPath *animviz_verify_motionpaths(struct Scene *scene, struct Object *ob, struct bPoseChannel *pchan);
void animviz_calc_motionpaths(struct Scene *scene, struct Object *ob);

/* ---------------------------------------------------- */
/* Curve Paths */

void free_path(struct Path *path);
void calc_curvepath(struct Object *ob);
int interval_test(int min, int max, int p1, int cycl);
int where_on_path(struct Object *ob, float ctime, float *vec, float *dir, float *quat, float *radius);

/* ---------------------------------------------------- */
/* Dupli-Geometry */

struct ListBase *object_duplilist(struct Scene *sce, struct Object *ob);
void free_object_duplilist(struct ListBase *lb);
int count_duplilist(struct Object *ob);

#endif

