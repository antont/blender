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

#include "logic.h" /*This must come first*/

#include "MEM_guardedalloc.h"
#include "BLI_blenlib.h"
#include "gen_utils.h"

//--------------- Python V24_BPy_Property methods declarations:---------------
static PyObject *V24_Property_getName( V24_BPy_Property * self );
static PyObject *V24_Property_setName( V24_BPy_Property * self, PyObject * value );
static PyObject *V24_Property_getData( V24_BPy_Property * self );
static PyObject *V24_Property_setData( V24_BPy_Property * self, PyObject * args );
static PyObject *V24_Property_getType( V24_BPy_Property * self );
//--------------- Python V24_BPy_Property methods table:----------------------
static PyMethodDef V24_BPy_Property_methods[] = {
	{"getName", ( PyCFunction ) V24_Property_getName, METH_NOARGS,
	 "() - return Property name"},
	{"setName", ( PyCFunction ) V24_Property_setName, METH_O,
	 "() - set the name of this Property"},
	{"getData", ( PyCFunction ) V24_Property_getData, METH_NOARGS,
	 "() - return Property data"},
	{"setData", ( PyCFunction ) V24_Property_setData, METH_VARARGS,
	 "() - set the data of this Property"},
	{"getType", ( PyCFunction ) V24_Property_getType, METH_NOARGS,
	 "() - return Property type"},
	{NULL, NULL, 0, NULL}
};
//--------------- Python TypeProperty callback function prototypes--------
static void V24_Property_dealloc( V24_BPy_Property * Property );
static PyObject *V24_Property_getAttr( V24_BPy_Property * Property, char *name );
static int V24_Property_setAttr( V24_BPy_Property * Property, char *name,
			     PyObject * v );
static PyObject *V24_Property_repr( V24_BPy_Property * Property );
static int V24_Property_compare( V24_BPy_Property * a1, V24_BPy_Property * a2 );
//--------------- Python TypeProperty structure definition----------------
PyTypeObject V24_property_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,	/* ob_size */
	"Blender Property",	/* tp_name */
	sizeof( V24_BPy_Property ),	/* tp_basicsize */
	0,			/* tp_itemsize */
	/* methods */
	( destructor ) V24_Property_dealloc,	/* tp_dealloc */
	0,			/* tp_print */
	( getattrfunc ) V24_Property_getAttr,	/* tp_getattr */
	( setattrfunc ) V24_Property_setAttr,	/* tp_setattr */
	( cmpfunc ) V24_Property_compare,	/* tp_compare */
	( reprfunc ) V24_Property_repr,	/* tp_repr */
	0,			/* tp_as_number */
	0,			/* tp_as_sequence */
	0,			/* tp_as_mapping */
	0,			/* tp_as_hash */
	0, 0, 0, 0, 0, 0,
	0,			/* tp_doc */
	0, 0, 0, 0, 0, 0,
	V24_BPy_Property_methods,	/* tp_methods */
	0,			/* tp_members */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
//--------------- Property module internal callbacks-------------------

//--------------- V24_updatePyProperty-------------------------------------
int V24_updatePyProperty( V24_BPy_Property * self )
{
	if( !self->property ) {
		return 0;	//nothing to update - not linked
	} else {
		BLI_strncpy( self->name, self->property->name, 32 );
		self->type = self->property->type;
		if( self->property->type == PROP_BOOL ) {
			if( *( ( int * ) &self->property->poin ) ) {
				self->data = V24_EXPP_incr_ret_True();
			} else {
				self->data = V24_EXPP_incr_ret_False();
			}
		} else if( self->property->type == PROP_INT ) {
			self->data = PyInt_FromLong( self->property->data );
		} else if( self->property->type == PROP_FLOAT ) {
			self->data =
				PyFloat_FromDouble( *
						    ( ( float * ) &self->
						      property->data ) );
		} else if( self->property->type == PROP_TIME ) {
			self->data =
				PyFloat_FromDouble( *
						    ( ( float * ) &self->
						      property->data ) );
		} else if( self->property->type == PROP_STRING ) {
			self->data =
				PyString_FromString( self->property->poin );
		}
		return 1;
	}
}

//--------------- updatePropertyData------------------------------------
int V24_updateProperyData( V24_BPy_Property * self )
{
	if( !self->property ) {
		//nothing to update - not linked
		return 0;
	} else {
		BLI_strncpy( self->property->name, self->name, 32 );
		self->property->type = self->type;
		if( PyInt_Check( self->data ) ) {
			*( ( int * ) &self->property->data ) =
				( int ) PyInt_AsLong( self->data );
		} else if( PyFloat_Check( self->data ) ) {
			*( ( float * ) &self->property->data ) =
				( float ) PyFloat_AsDouble( self->data );
		} else if( PyString_Check( self->data ) ) {
			BLI_strncpy( self->property->poin,
				     PyString_AsString( self->data ),
				     MAX_PROPSTRING );
		}
		return 1;
	}
}

//--------------- checkValidData_ptr--------------------------------
static int checkValidData_ptr( V24_BPy_Property * self )
{
	int length;
	/* test pointer to see if data was removed (oops) */
	/* WARNING!!! - MEMORY LEAK HERE, why not free this??? */
	length = MEM_allocN_len( self->property );
	if( length != sizeof( bProperty ) ) {	//data was freed
		self->property = NULL;
		return 0;
	} else {		//it's ok as far as we can tell
		return 1;
	}
}

//---------------V24_BPy_Property internal callbacks/methods------------

//--------------- dealloc-------------------------------------------
static void V24_Property_dealloc( V24_BPy_Property * self )
{
	PyMem_Free( self->name );
	PyObject_DEL( self );
}

//---------------getattr--------------------------------------------
static PyObject *V24_Property_getAttr( V24_BPy_Property * self, char *name )
{
	checkValidData_ptr( self );
	if( strcmp( name, "name" ) == 0 )
		return V24_Property_getName( self );
	else if( strcmp( name, "data" ) == 0 )
		return V24_Property_getData( self );
	else if( strcmp( name, "type" ) == 0 )
		return V24_Property_getType( self );
	else if( strcmp( name, "__members__" ) == 0 ) {
		return Py_BuildValue( "[s,s,s]", "name", "data", "type" );
	}

	return Py_FindMethod( V24_BPy_Property_methods, ( PyObject * ) self, name );
}

//--------------- setattr-------------------------------------------
static int
V24_Property_setAttr( V24_BPy_Property * self, char *name, PyObject * value )
{
	PyObject *error = NULL;

	checkValidData_ptr( self );

	if( strcmp( name, "name" ) == 0 ) {
		error = V24_Property_setName( self, value );
	} else if( strcmp( name, "data" ) == 0 ) {
		PyObject *valtuple = Py_BuildValue( "(O)", value );
		if( !valtuple )
			return V24_EXPP_ReturnIntError( PyExc_MemoryError,
						    "PropertySetAttr: couldn't create tuple" );
		
		error = V24_Property_setData( self, valtuple );
		Py_DECREF( valtuple );
	} else {
		return ( V24_EXPP_ReturnIntError
			 ( PyExc_KeyError, "attribute not found" ) );
	}
	

	if( error != Py_None )
		return -1;

	Py_DECREF( Py_None );
	return 0;
}

//--------------- repr----------------------------------------------
static PyObject *V24_Property_repr( V24_BPy_Property * self )
{
	checkValidData_ptr( self );
	if( self->property ) {
		return PyString_FromFormat( "[Property \"%s\"]",
					    self->property->name );
	} else {
		return PyString_FromFormat( "[Property \"%s\"]", self->name );
	}
}

//--------------- compare-------------------------------------------
//compares property.name and property.data
static int V24_Property_compare( V24_BPy_Property * a, V24_BPy_Property * b )
{
	V24_BPy_Property *py_propA, *py_propB;
	int retval = -1;

	checkValidData_ptr( a );
	checkValidData_ptr( b );
	//2 python objects
	if( !a->property && !b->property ) {
		if( a->type != b->type )
			retval = -1;
		if( BLI_streq( a->name, b->name ) ) {
			retval = PyObject_Compare( a->data, b->data );
		} else
			retval = -1;
	} else if( a->property && b->property ) {	//2 real properties
		if( a->property->type != b->property->type )
			retval = -1;
		if( BLI_streq( a->property->name, b->property->name ) ) {
			if( a->property->type == PROP_BOOL
			    || a->property->type == PROP_INT ) {
				if( a->property->data == b->property->data )
					retval = 0;
				else
					retval = -1;
			} else if( a->property->type == PROP_FLOAT
				   || a->property->type == PROP_TIME ) {
				if( *( ( float * ) &a->property->data ) ==
				    *( ( float * ) &b->property->data ) )
					retval = 0;
				else
					retval = -1;
			} else if( a->property->type == PROP_STRING ) {
				if( BLI_streq
				    ( a->property->poin, b->property->poin ) )
					retval = 0;
				else
					retval = -1;
			}
		} else
			retval = -1;
	} else {		//1 real 1 python
		if( !a->property ) {
			py_propA = a;
			py_propB = b;
		} else {
			py_propA = b;
			py_propB = a;
		}
		if( py_propB->property->type != py_propA->type )
			retval = -1;
		if( BLI_streq( py_propB->property->name, py_propA->name ) ) {
			if( py_propB->property->type == PROP_BOOL ||
			    py_propB->property->type == PROP_INT ) {
				retval = PyObject_Compare( py_propA->data,
							   PyInt_FromLong
							   ( py_propB->
							     property->
							     data ) );
			} else if( py_propB->property->type == PROP_FLOAT
				   || py_propB->property->type == PROP_TIME ) {
				retval = PyObject_Compare( py_propA->data,
							   PyFloat_FromDouble
							   ( *
							     ( ( float * )
							       &py_propB->
							       property->
							       data ) ) );
			} else if( py_propB->property->type == PROP_STRING ) {
				PyObject *tmpstr = PyString_FromString( py_propB->property->poin );
				retval = PyObject_Compare( py_propA->data, tmpstr );
				Py_DECREF(tmpstr);
			}
		} else
			retval = -1;
	}
	return retval;
}

//--------------- Property visible functions------------------------
//--------------- V24_Property_CreatePyObject---------------------------
PyObject *V24_Property_CreatePyObject( struct bProperty * Property )
{
	V24_BPy_Property *py_property;

	py_property =
		( V24_BPy_Property * ) PyObject_NEW( V24_BPy_Property,
						 &V24_property_Type );

	//set the struct flag
	py_property->property = Property;

	//allocate space for python vars
	py_property->name = PyMem_Malloc( 32 );

	if( !V24_updatePyProperty( py_property ) )
		return ( V24_EXPP_ReturnPyObjError
			 ( PyExc_AttributeError, "Property struct empty" ) );

	return ( ( PyObject * ) py_property );
}

//--------------- V24_Property_FromPyObject-----------------------------
struct bProperty *V24_Property_FromPyObject( PyObject * py_obj )
{
	V24_BPy_Property *py_property;

	py_property = ( V24_BPy_Property * ) py_obj;
	if( !py_property->property )
		return NULL;
	else
		return ( py_property->property );
}

//--------------- V24_newPropertyObject()-------------------------------
PyObject *V24_newPropertyObject( char *name, PyObject * data, int type )
{
	V24_BPy_Property *py_property;

	py_property =
		( V24_BPy_Property * ) PyObject_NEW( V24_BPy_Property,
						 &V24_property_Type );
	py_property->name = PyMem_Malloc( 32 );
	py_property->property = NULL;

	BLI_strncpy( py_property->name, name, 32 );
	py_property->data = data;
	py_property->type = (short)type;

	return ( PyObject * ) py_property;
}

//--------------- Python V24_BPy_Property methods-----------------------
//--------------- V24_BPy_Property.getName()----------------------------
static PyObject *V24_Property_getName( V24_BPy_Property * self )
{
	if( !self->property )
		return PyString_FromString( self->name );
	else
		return PyString_FromString( self->property->name );
}

//--------------- V24_BPy_Property.setName()----------------------------
static PyObject *V24_Property_setName( V24_BPy_Property * self, PyObject * value )
{
	char *name = PyString_AsString(value);

	if( !name )
		return ( V24_EXPP_ReturnPyObjError( PyExc_AttributeError,
						"expected string argument" ) );

	if( !self->property ) {
		BLI_strncpy( self->name, name, 32 );
	} else {
		BLI_strncpy( self->property->name, name, 32 );
		V24_updatePyProperty( self );
	}

	Py_RETURN_NONE;
}

//--------------- V24_BPy_Property.getData()----------------------------
static PyObject *V24_Property_getData( V24_BPy_Property * self )
{
	PyObject *attr = NULL;

	if( !self->property ) {
		attr = V24_EXPP_incr_ret( self->data );
	} else {
		if( self->property->type == PROP_BOOL ) {
			if( self->property->data )
				attr = V24_EXPP_incr_ret_True();
			else
				attr = V24_EXPP_incr_ret_False();
		} else if( self->property->type == PROP_INT ) {
			attr = PyInt_FromLong( self->property->data );
		} else if( self->property->type == PROP_FLOAT ||
			   self->property->type == PROP_TIME ) {
			attr = PyFloat_FromDouble( *
						   ( ( float * ) &self->
						     property->data ) );
		} else if( self->property->type == PROP_STRING ) {
			attr = PyString_FromString( self->property->poin );
		}
	}
	if( attr )
		return attr;

	return ( V24_EXPP_ReturnPyObjError( PyExc_RuntimeError,
					"couldn't get Property.data attribute" ) );
}

//--------------- V24_BPy_Property.setData()----------------------------
static PyObject *V24_Property_setData( V24_BPy_Property * self, PyObject * args )
{
	PyObject *data;
	char *type_str = NULL;
	int type = -1;
	short *p_type = NULL;

	if( !PyArg_ParseTuple( args, "O|s", &data, &type_str ) )
		return ( V24_EXPP_ReturnPyObjError( PyExc_AttributeError,
						"expected object  and optional string argument" ) );

	if( !PyInt_Check( data ) && !PyFloat_Check( data )
	    && !PyString_Check( data ) )
		return ( V24_EXPP_ReturnPyObjError
			 ( PyExc_RuntimeError,
			   "float, int, or string expected as data" ) );

	//parse property name
	if( type_str ) {
		if( BLI_streq( type_str, "BOOL" ) )
			type = PROP_BOOL;
		else if( BLI_streq( type_str, "INT" ) )
			type = PROP_INT;
		else if( BLI_streq( type_str, "FLOAT" ) )
			type = PROP_FLOAT;
		else if( BLI_streq( type_str, "TIME" ) )
			type = PROP_TIME;
		else if( BLI_streq( type_str, "STRING" ) )
			type = PROP_STRING;
		else
			return ( V24_EXPP_ReturnPyObjError( PyExc_RuntimeError,
							"BOOL, INT, FLOAT, TIME or STRING expected" ) );
	}
	//get pointer to type
	if( self->property )
		p_type = &self->property->type;
	else
		p_type = &self->type;

	//set the type
	if( PyInt_Check( data ) ) {
		if( type == -1 || type == PROP_INT )
			*p_type = PROP_INT;
		else
			*p_type = PROP_BOOL;
	} else if( PyFloat_Check( data ) ) {
		if( type == -1 || type == PROP_FLOAT )
			*p_type = PROP_FLOAT;
		else
			*p_type = PROP_TIME;
	} else if( PyString_Check( data ) ) {
		if( type == -1 || type == PROP_STRING )
			*p_type = PROP_STRING;
	} else {
		return ( V24_EXPP_ReturnPyObjError( PyExc_RuntimeError,
						"cant set unknown data type" ) );
	}

	//set the data
	if( self->property ) {
		if( PyInt_Check( data ) ) {
			*( ( int * ) &self->property->data ) =
				( int ) PyInt_AsLong( data );
		} else if( PyFloat_Check( data ) ) {
			*( ( float * ) &self->property->data ) =
				( float ) PyFloat_AsDouble( data );
		} else if( PyString_Check( data ) ) {
			BLI_strncpy( self->property->poin,
				     PyString_AsString( data ),
				     MAX_PROPSTRING );
		}
		V24_updatePyProperty( self );
	} else {
		self->data = data;
	}
	Py_RETURN_NONE;
}

//--------------- V24_BPy_Property.getType()----------------------------
static PyObject *V24_Property_getType( V24_BPy_Property * self )
{
	int type;

	if( self->property )
		type = self->property->type;
	else
		type = self->type;

	if( type == PROP_BOOL )
		return PyString_FromString( "BOOL" );
	else if( type == PROP_INT )
		return PyString_FromString( "INT" );
	else if( type == PROP_FLOAT )
		return PyString_FromString( "FLOAT" );
	else if( type == PROP_STRING )
		return PyString_FromString( "STRING" );
	else if( type == PROP_TIME )
		return PyString_FromString( "TIME" );
	Py_RETURN_NONE;
}
