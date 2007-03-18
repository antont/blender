/**
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

#ifndef BIF_TRANSFORM_H
#define BIF_TRANSFORM_H

/* ******************** Macros & Prototypes *********************** */

/* MODE AND NUMINPUT FLAGS */
#define TFM_INIT			-1
#define TFM_DUMMY			0
#define TFM_TRANSLATION		1
#define TFM_ROTATION		2
#define TFM_RESIZE			3
#define TFM_TOSPHERE		4
#define TFM_SHEAR			5
#define TFM_WARP			7
#define TFM_SHRINKFATTEN	8
#define TFM_TILT			9
#define TFM_LAMP_ENERGY		10
#define TFM_TRACKBALL		11
#define TFM_PUSHPULL		12
#define TFM_CREASE			13
#define TFM_MIRROR			14
#define TFM_BONESIZE		15
#define TFM_BONE_ENVELOPE	16
#define TFM_CURVE_SHRINKFATTEN		17
#define TFM_BONE_ROLL		18

/* TRANSFORM CONTEXTS */
#define CTX_NONE			0
#define CTX_TEXTURE			1
#define CTX_EDGE			2
#define CTX_NO_PET			4
#define CTX_TWEAK			8

void initTransform(int mode, int context);
void Transform(void);
void Mirror(short mode);

/* Standalone call to get the transformation center corresponding to the current situation */
void calculateTransformCenter(int centerMode, float *vec);

struct TransInfo;
struct ScrArea;

struct TransInfo * BIF_GetTransInfo(void);
void BIF_setSingleAxisConstraint(float vec[3], char *text);
void BIF_setDualAxisConstraint(float vec1[3], float vec2[3], char *text);
void BIF_setLocalAxisConstraint(char axis, char *text);
void BIF_setLocalLockConstraint(char axis, char *text);
void BIF_drawConstraint(void);
void BIF_drawPropCircle(void);
void BIF_drawSnap(void);
void BIF_getPropCenter(float *center);

void BIF_TransformSetUndo(char *str);

void BIF_selectOrientation(void);

/* view3d manipulators */
void initManipulator(int mode);
void ManipulatorTransform();

int BIF_do_manipulator(struct ScrArea *sa);
void BIF_draw_manipulator(struct ScrArea *sa);

#endif

