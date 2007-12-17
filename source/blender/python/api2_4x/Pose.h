/* 
 * $Id: 
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
 * Contributor(s): Joseph Gilbert
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#ifndef V24_EXPP_POSE_H
#define V24_EXPP_POSE_H

#include <Python.h>
#include "DNA_action_types.h"
#include "DNA_object_types.h"

//-------------------TYPE CHECKS---------------------------------
#define BPy_Pose_Check(v) ((v)->ob_type == &V24_Pose_Type)
#define BPy_PoseBone_Check(v) ((v)->ob_type == &V24_PoseBone_Type)
#define BPy_PoseBonesDict_Check(v) ((v)->ob_type == &V24_PoseBonesDict_Type)
//-------------------TYPEOBJECT----------------------------------
extern PyTypeObject V24_Pose_Type;
extern PyTypeObject V24_PoseBone_Type;
extern PyTypeObject V24_PoseBonesDict_Type;
//-------------------STRUCT DEFINITION----------------------------
typedef struct {
	PyObject_HEAD 
	PyObject *bonesMap;
	ListBase *bones;  
} V24_BPy_PoseBonesDict;

typedef struct {
	PyObject_HEAD
	bPose *pose;
	char name[24];   //because poses have not names :(
	V24_BPy_PoseBonesDict *Bones; 
} V24_BPy_Pose;

typedef struct {
	PyObject_HEAD
	bPoseChannel *posechannel;
	
} V24_BPy_PoseBone;

//-------------------VISIBLE PROTOTYPES-------------------------
PyObject *V24_Pose_Init(void);
PyObject *V24_PyPose_FromPose(bPose *pose, char *name);
PyObject *V24_PyPoseBone_FromPosechannel(bPoseChannel *pchan);
Object *V24_Object_FromPoseChannel(bPoseChannel *curr_pchan);
#endif
