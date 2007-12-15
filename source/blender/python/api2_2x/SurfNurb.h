/*
 * $Id$
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
 * Contributor(s): Stephen Swaney
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifndef EXPP_SURFNURB_H
#define EXPP_SURFNURB_H

#include <Python.h>
#include "DNA_curve_types.h"

extern PyTypeObject V24_SurfNurb_Type;

#define V24_BPy_SurfNurb_Check(v)  ((v)->ob_type == &V24_SurfNurb_Type)	/* for type checking */

/* Python V24_BPy_SurfNurb structure definition */
typedef struct {
	PyObject_HEAD
	Nurb * nurb;

	/* iterator stuff */
	/* internal ptrs to point data.  do not free */
	BPoint *bp;
	BezTriple *bezt;
	int atEnd;		/* iter exhausted flag  */
	int nextPoint;

} V24_BPy_SurfNurb;


/*
 *  prototypes
 */

PyObject *V24_SurfNurb_Init( void );
PyObject *V24_SurfNurb_CreatePyObject( Nurb * bzt );
Nurb *V24_SurfNurb_FromPyObject( PyObject * pyobj );

PyObject *V24_SurfNurb_getPoint( V24_BPy_SurfNurb * self, int index );
PyObject *V24_SurfNurb_pointAtIndex( Nurb * nurb, int index );

#endif				/* EXPP_SURFNURB_H */
