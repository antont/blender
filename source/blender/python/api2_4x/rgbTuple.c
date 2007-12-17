/* 
 * $Id: rgbTuple.c 12898 2007-12-15 21:44:40Z campbellbarton $
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

#include "rgbTuple.h" /*This must come first */

#include "gen_utils.h"

/* This file is heavily based on the old bpython Constant object code in
   Blender */

/*****************************************************************************/
/* Python V24_rgbTuple_Type callback function prototypes:                        */
/*****************************************************************************/
static PyObject *V24_rgbTuple_getAttr( V24_BPy_rgbTuple * self, char *name );
static int V24_rgbTuple_setAttr( V24_BPy_rgbTuple * self, char *name, PyObject * v );
static PyObject *V24_rgbTuple_repr( V24_BPy_rgbTuple * self );

static int V24_rgbTupleLength( void );

static PyObject *V24_rgbTupleSubscript( V24_BPy_rgbTuple * self, PyObject * key );
static int V24_rgbTupleAssSubscript( V24_BPy_rgbTuple * self, PyObject * who,
				 PyObject * cares );

static PyObject *V24_rgbTupleItem( V24_BPy_rgbTuple * self, int i );
static int V24_rgbTupleAssItem( V24_BPy_rgbTuple * self, int i, PyObject * ob );
static PyObject *V24_rgbTupleSlice( V24_BPy_rgbTuple * self, int begin, int end );
static int V24_rgbTupleAssSlice( V24_BPy_rgbTuple * self, int begin, int end,
			     PyObject * seq );

/*****************************************************************************/
/* Python V24_rgbTuple_Type Mapping Methods table:                               */
/*****************************************************************************/
static PyMappingMethods V24_rgbTupleAsMapping = {
	( inquiry ) V24_rgbTupleLength,	/* mp_length        */
	( binaryfunc ) V24_rgbTupleSubscript,	/* mp_subscript     */
	( objobjargproc ) V24_rgbTupleAssSubscript,	/* mp_ass_subscript */
};

/*****************************************************************************/
/* Python V24_rgbTuple_Type Sequence Methods table:                              */
/*****************************************************************************/
static PySequenceMethods V24_rgbTupleAsSequence = {
	( inquiry ) V24_rgbTupleLength,	/* sq_length */
	( binaryfunc ) 0,	/* sq_concat */
	( intargfunc ) 0,	/* sq_repeat */
	( intargfunc ) V24_rgbTupleItem,	/* sq_item */
	( intintargfunc ) V24_rgbTupleSlice,	/* sq_slice */
	( intobjargproc ) V24_rgbTupleAssItem,	/* sq_ass_item */
	( intintobjargproc ) V24_rgbTupleAssSlice,	/* sq_ass_slice       */
	0,0,0
};

/*****************************************************************************/
/* Python V24_rgbTuple_Type structure definition:                                */
/*****************************************************************************/
PyTypeObject V24_rgbTuple_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,	/* ob_size */
	"rgbTuple",		/* tp_name */
	sizeof( V24_BPy_rgbTuple ),			/* tp_basicsize */
	0,			/* tp_itemsize */
	/* methods */
	0,			/* tp_dealloc */
	0,			/* tp_print */
	( getattrfunc ) V24_rgbTuple_getAttr,	/* tp_getattr */
	( setattrfunc ) V24_rgbTuple_setAttr,	/* tp_setattr */
	0,			/* tp_compare */
	( reprfunc ) V24_rgbTuple_repr,	/* tp_repr */
	0,			/* tp_as_number */
	&V24_rgbTupleAsSequence,	/* tp_as_sequence */
	&V24_rgbTupleAsMapping,	/* tp_as_mapping */
	0,			/* tp_as_hash */
	0, 0, 0, 0, 0, Py_TPFLAGS_DEFAULT,
	0,			/* tp_doc */
	0, 0, 0, 0, 0, 0,
	0,			/* tp_methods */
	0,			/* tp_members */
	0,     /* struct PyGetSetDef *tp_getset; */
	0,                       /* struct _typeobject *tp_base; */
	0,                       /* PyObject *tp_dict; */
	0,                       /* descrgetfunc tp_descr_get; */
	0,                       /* descrsetfunc tp_descr_set; */
	0,                          /* long tp_dictoffset; */
	0,                       /* initproc tp_init; */
	0,                       /* allocfunc tp_alloc; */
	0,                       /* newfunc tp_new; */
	/*  Low-level free-memory routine */
	0,                       /* freefunc tp_free;  */
	/* For PyObject_IS_GC */
	0,                       /* inquiry tp_is_gc;  */
	0,                       /* PyObject *tp_bases; */
	/* method resolution order */
	0,                       /* PyObject *tp_mro;  */
	0,                       /* PyObject *tp_cache; */
	0,                       /* PyObject *tp_subclasses; */
	0,                       /* PyObject *tp_weaklist; */
	0
};

/*****************************************************************************/
/* Function:              V24_rgbTuple_New                                       */
/*****************************************************************************/
PyObject *V24_rgbTuple_New( float *rgb[3] )
{
	V24_BPy_rgbTuple *rgbTuple = PyObject_NEW( V24_BPy_rgbTuple, &V24_rgbTuple_Type );

	if( rgbTuple == NULL )
		return V24_EXPP_ReturnPyObjError( PyExc_MemoryError,
					      "couldn't create rgbTuple object" );

	rgbTuple->rgb[0] = rgb[0];
	rgbTuple->rgb[1] = rgb[1];
	rgbTuple->rgb[2] = rgb[2];

	return ( PyObject * ) rgbTuple;
}

/*****************************************************************************/
/* Functions:      V24_rgbTuple_getCol and V24_rgbTuple_setCol                       */
/* Description:    These functions get/set rgb color triplet values.  The    */
/*                 get function returns a tuple, the set one accepts three   */
/*                 floats (separated or in a tuple) as arguments.            */
/*****************************************************************************/
PyObject *V24_rgbTuple_getCol( V24_BPy_rgbTuple * self )
{
	return Py_BuildValue( "[fff]", *(self->rgb[0]),
			 			*(self->rgb[1]), *(self->rgb[2]));
}

int V24_rgbTuple_setCol( V24_BPy_rgbTuple * self, PyObject * args )
{
	int ok = 0;
	int i;
	float num[3]={0,0,0};

	/*
	 * since V24_rgbTuple_getCol() returns a list, be sure we accept a list
	 * as valid input
	 */

	if( PyObject_Length( args ) == 3 ) {
		if ( PyList_Check ( args ) ) {
			ok = 1;
			for( i = 0; i < 3; ++i ) {
				PyObject *tmp = PyList_GET_ITEM( args, i );
				if( !PyNumber_Check ( tmp ) ) {
					ok = 0;
					break;
				}
				num[i] = (float)PyFloat_AsDouble( tmp );
			}
		} else
			ok = PyArg_ParseTuple( args, "fff", &num[0], &num[1], &num[2] );
	} else
		ok = PyArg_ParseTuple( args, "|(fff)", &num[0], &num[1], &num[2] );

	if( !ok )
		return V24_EXPP_ReturnIntError( PyExc_TypeError,
					      "expected [f,f,f], (f,f,f) or f,f,f as arguments (or nothing)" );

	for( i = 0; i < 3; ++i )
		*( self->rgb[i] ) = V24_EXPP_ClampFloat( num[i], 0.0, 1.0 );

	return 0;
}

/*****************************************************************************/
/* Function:    V24_rgbTuple_getAttr                                             */
/* Description: This is a callback function for the V24_BPy_rgbTuple type. It is */
/*              the function that accesses V24_BPy_rgbTuple member variables and */
/*              methods.                                                     */
/*****************************************************************************/
static PyObject *V24_rgbTuple_getAttr( V24_BPy_rgbTuple * self, char *name )
{
	int i;

	if( strcmp( name, "__members__" ) == 0 )
		return Py_BuildValue( "[s,s,s]", "R", "G", "B" );

	else if( !strcmp( name, "R" ) || !strcmp( name, "r" ) )
		i = 0;
	else if( !strcmp( name, "G" ) || !strcmp( name, "g" ) )
		i = 1;
	else if( !strcmp( name, "B" ) || !strcmp( name, "b" ) )
		i = 2;
	else
		return ( V24_EXPP_ReturnPyObjError( PyExc_AttributeError,
						"attribute not found" ) );

	return PyFloat_FromDouble( (double)(*( self->rgb[i] )) );
}

/*****************************************************************************/
/* Function:    V24_rgbTuple_setAttr                                             */
/* Description: This is a callback function for the V24_BPy_rgbTuple type. It is */
/*              the function that changes V24_BPy_rgbTuple member variables.     */
/*****************************************************************************/
static int V24_rgbTuple_setAttr( V24_BPy_rgbTuple * self, char *name, PyObject * v )
{
	float value;

	if( !PyArg_Parse( v, "f", &value ) )
		return V24_EXPP_ReturnIntError( PyExc_TypeError,
					    "expected float argument" );

	value = V24_EXPP_ClampFloat( value, 0.0, 1.0 );

	if( !strcmp( name, "R" ) || !strcmp( name, "r" ) )
		*( self->rgb[0] ) = value;

	else if( !strcmp( name, "G" ) || !strcmp( name, "g" ) )
		*( self->rgb[1] ) = value;

	else if( !strcmp( name, "B" ) || !strcmp( name, "b" ) )
		*( self->rgb[2] ) = value;

	else
		return ( V24_EXPP_ReturnIntError( PyExc_AttributeError,
					      "attribute not found" ) );

	return 0;
}

/*****************************************************************************/
/* Section:    rgbTuple as Mapping                                           */
/*             These functions provide code to access rgbTuple objects as    */
/*             mappings.                                                     */
/*****************************************************************************/
static int V24_rgbTupleLength( void )
{
	return 3;
}

static PyObject *V24_rgbTupleSubscript( V24_BPy_rgbTuple * self, PyObject * key )
{
	char *name = NULL;
	int i;

	if( PyNumber_Check( key ) )
		return V24_rgbTupleItem( self, ( int ) PyInt_AsLong( key ) );

	if( !PyArg_ParseTuple( key, "s", &name ) )
		return V24_EXPP_ReturnPyObjError( PyExc_TypeError,
					      "expected int or string argument" );

	if( !strcmp( name, "R" ) || !strcmp( name, "r" ) )
		i = 0;
	else if( !strcmp( name, "G" ) || !strcmp( name, "g" ) )
		i = 1;
	else if( !strcmp( name, "B" ) || !strcmp( name, "b" ) )
		i = 2;
	else
		return V24_EXPP_ReturnPyObjError( PyExc_AttributeError, name );

	return PyFloat_FromDouble( (double)(*( self->rgb[i] )) );
}

static int V24_rgbTupleAssSubscript( V24_BPy_rgbTuple * self, PyObject * key,
				 PyObject * v )
{
	char *name = NULL;
	int i;

	if( !PyNumber_Check( v ) )
		return V24_EXPP_ReturnIntError( PyExc_TypeError,
					    "value to assign must be a number" );

	if( PyNumber_Check( key ) )
		return V24_rgbTupleAssItem( self, ( int ) PyInt_AsLong( key ), v );

	if( !PyArg_Parse( key, "s", &name ) )
		return V24_EXPP_ReturnIntError( PyExc_TypeError,
					    "expected int or string argument" );

	if( !strcmp( name, "R" ) || !strcmp( name, "r" ) )
		i = 0;
	else if( !strcmp( name, "G" ) || !strcmp( name, "g" ) )
		i = 1;
	else if( !strcmp( name, "B" ) || !strcmp( name, "b" ) )
		i = 2;
	else
		return V24_EXPP_ReturnIntError( PyExc_AttributeError, name );

	*( self->rgb[i] ) = V24_EXPP_ClampFloat( (float)PyFloat_AsDouble( v ), 0.0, 1.0 );

	return 0;
}

/*****************************************************************************/
/* Section:    rgbTuple as Sequence                                          */
/*             These functions provide code to access rgbTuple objects as    */
/*             sequences.                                                    */
/*****************************************************************************/
static PyObject *V24_rgbTupleItem( V24_BPy_rgbTuple * self, int i )
{
	if( i < 0 || i >= 3 )
		return V24_EXPP_ReturnPyObjError( PyExc_IndexError,
					      "array index out of range" );

	return PyFloat_FromDouble( (long)(*( self->rgb[i] )) );
}

static PyObject *V24_rgbTupleSlice( V24_BPy_rgbTuple * self, int begin, int end )
{
	PyObject *list;
	int count;

	if( begin < 0 )
		begin = 0;
	if( end > 3 )
		end = 3;
	if( begin > end )
		begin = end;

	list = PyList_New( end - begin );

	for( count = begin; count < end; count++ )
		PyList_SetItem( list, count - begin,
				PyFloat_FromDouble( *( self->rgb[count] ) ) );

	return list;
}

static int V24_rgbTupleAssItem( V24_BPy_rgbTuple * self, int i, PyObject * ob )
{
	if( i < 0 || i >= 3 )
		return V24_EXPP_ReturnIntError( PyExc_IndexError,
					    "array assignment index out of range" );

	if( !PyNumber_Check( ob ) )
		return V24_EXPP_ReturnIntError( PyExc_IndexError,
					    "color component must be a number" );
/* XXX this check above is probably ... */
	*( self->rgb[i] ) =
		V24_EXPP_ClampFloat( (float)PyFloat_AsDouble( ob ), 0.0, 1.0 );

	return 0;
}

static int V24_rgbTupleAssSlice( V24_BPy_rgbTuple * self, int begin, int end,
			     PyObject * seq )
{
	int count;

	if( begin < 0 )
		begin = 0;
	if( end > 3 )
		end = 3;
	if( begin > end )
		begin = end;

	if( !PySequence_Check( seq ) )
		return V24_EXPP_ReturnIntError( PyExc_TypeError,
					    "illegal argument type for built-in operation" );

	if( PySequence_Length( seq ) != ( end - begin ) )
		return V24_EXPP_ReturnIntError( PyExc_TypeError,
					    "size mismatch in slice assignment" );

	for( count = begin; count < end; count++ ) {
		float value;
		PyObject *ob = PySequence_GetItem( seq, count );

		if( !PyArg_Parse( ob, "f", &value ) ) {
			Py_DECREF( ob );
			return -1;
		}

		*( self->rgb[count] ) = V24_EXPP_ClampFloat( value, 0.0, 1.0 );

		Py_DECREF( ob );
	}

	return 0;
}

/*****************************************************************************/
/* Function:    V24_rgbTuple_repr                                                */
/* Description: This is a callback function for the V24_BPy_rgbTuple type. It    */
/*              builds a meaninful string to represent rgbTuple objects.     */
/*****************************************************************************/
static PyObject *V24_rgbTuple_repr( V24_BPy_rgbTuple * self )
{
	float r, g, b;

	r = *( self->rgb[0] );
	g = *( self->rgb[1] );
	b = *( self->rgb[2] );

	return PyString_FromFormat( "[%f, %f, %f]", r, g, b );
}
