/* 
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
 * Contributor(s): Michel Selten
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#include <Python.h>

#include <DNA_ID.h>

int StringEqual (char * string1, char * string2);
char * GetIdName (ID *id);
PyObject * PythonReturnErrorObject (PyObject * type, char * error_msg);
PyObject * PythonIncRef (PyObject *object);
PyObject * EXPP_incr_ret (PyObject *object);
char * event_to_name (short event);
float EXPP_ClampFloat (float value, float min, float max);
int EXPP_ReturnIntError (PyObject *type, char *error_msg);
PyObject *EXPP_ReturnPyObjError (PyObject * type, char * error_msg);

/* The following functions may need to be moved to the respective BKE or */
/* DNA modules. */

struct Object * GetObjectByName (char * name);

