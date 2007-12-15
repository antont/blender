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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * This is a new part of Blender.
 *
 * Contributor(s): Jacques Guignot
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#ifndef EXPP_IPOCURVE_H
#define EXPP_IPOCURVE_H

#include <Python.h>
#include "DNA_curve_types.h"	/* declaration of IpoCurve */

/*****************************************************************************/
/* Python V24_C_IpoCurve structure definition:                                   */
/*****************************************************************************/
typedef struct {
	PyObject_HEAD		/* required macro */
	IpoCurve * ipocurve;
	char wrapped;
} V24_C_IpoCurve;

extern PyTypeObject V24_IpoCurve_Type;

#define V24_BPy_IpoCurve_Check(v)  ((v)->ob_type == &V24_IpoCurve_Type)	/* for type checking */

PyObject *V24_IpoCurve_Init( void );
PyObject *V24_IpoCurve_CreatePyObject( IpoCurve * ipo );
IpoCurve *V24_IpoCurve_FromPyObject( PyObject * pyobj );
char *getIpoCurveName( IpoCurve * icu );


#endif				/* EXPP_IPOCURVE_H */
