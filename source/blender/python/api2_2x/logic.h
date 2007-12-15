/* 
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * This is a new part of Blender.
 *
 * Contributor(s): Joseph Gilbert
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#ifndef EXPP_LOGIC_H
#define EXPP_LOGIC_H

#include <Python.h>
#include "DNA_property_types.h"

extern PyTypeObject V24_property_Type;

#define V24_BPy_Property_Check(v)       ((v)->ob_type == &V24_property_Type)

//--------------------------Python V24_BPy_Property structure definition.----
typedef struct {
	PyObject_HEAD
		//reference to property data if object linked
	bProperty * property;
	//list of vars that define the property
	char *name;
	PyObject *data;
	short type;
} V24_BPy_Property;

//------------------------------visible prototypes-----------------------
PyObject *V24_Property_CreatePyObject( struct bProperty *prop );
bProperty *V24_Property_FromPyObject( PyObject * py_obj );
PyObject *V24_newPropertyObject( char *name, PyObject * data, int type );
int V24_updatePyProperty( V24_BPy_Property * self );
int V24_updateProperyData( V24_BPy_Property * self );

#endif
