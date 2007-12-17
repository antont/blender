/* 
 * $Id: Texture.h 12898 2007-12-15 21:44:40Z campbellbarton $
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
 * Contributor(s): Alex Mole
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

/* based on Image.h */

#ifndef V24_EXPP_TEXTURE_H
#define V24_EXPP_TEXTURE_H

#include <Python.h>
#include "DNA_texture_types.h"

/*****************************************************************************/
/* Python V24_BPy_Texture structure definition                                   */
/*****************************************************************************/

typedef struct {
	PyObject_HEAD 
	Tex * texture; /* libdata must be second */
} V24_BPy_Texture;

extern PyTypeObject V24_Texture_Type;

#define V24_BPy_Texture_Check(v)  ((v)->ob_type == &V24_Texture_Type)


/*****************************************************************************/
/* Module Blender.Texture - public functions                                 */
/*****************************************************************************/

PyObject *V24_Texture_Init( void );
PyObject *V24_Texture_CreatePyObject( struct Tex *tex );
Tex *V24_Texture_FromPyObject( PyObject * pyobj );


#endif				/* EXPP_TEXTURE_H */
