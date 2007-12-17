/* 
 * $Id: Scene.h 12898 2007-12-15 21:44:40Z campbellbarton $
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
 * This is a new part of Blender.
 *
 * Contributor(s): Willian P. Germano
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#ifndef V24_EXPP_SCENE_H
#define V24_EXPP_SCENE_H

#include <Python.h>
#include "DNA_scene_types.h"

/* The Scene PyType Object defined in Scene.c */
extern PyTypeObject V24_Scene_Type;
extern PyTypeObject V24_SceneObSeq_Type;

#define V24_BPy_Scene_Check(v) \
    ((v)->ob_type == &V24_Scene_Type)
#define V24_BPy_SceneObSeq_Check(v) \
    ((v)->ob_type == &V24_SceneObSeq_Type)

/*---------------------------Python V24_BPy_Scene structure definition----------*/
typedef struct {
	PyObject_HEAD 
	Scene * scene; /* libdata must be second */
} V24_BPy_Scene;
/*---------------------------Python V24_BPy_Scene visible prototypes-----------*/
/* Python V24_Scene_Type helper functions needed by Blender (the Init function) and Object modules. */


/* Scene object sequence, iterate on the scene object listbase*/
typedef struct {
	PyObject_VAR_HEAD /* required python macro   */
	V24_BPy_Scene *bpyscene; /* link to the python scene so we can know if its been removed */
	Base *iter; /* so we can iterate over the objects */
	int mode; /*0:all objects, 1:selected objects, 2:user context*/
} V24_BPy_SceneObSeq;


PyObject *V24_Scene_Init( void );
PyObject *V24_Scene_CreatePyObject( Scene * scene );
/*Scene *V24_Scene_FromPyObject( PyObject * pyobj );*/  /* not used yet */

#endif				/* EXPP_SCENE_H */
