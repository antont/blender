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
 * Contributor(s): Willian P. Germano, Joseph Gilbert
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#include "constant.h" /*This must come first */

#include "gen_utils.h"
#include "BLI_blenlib.h"

PyTypeObject V24_constant_Type;

//------------------METHOD IMPLEMENTATIONS-----------------------------
//------------------------constant.items()
//Returns a list of key:value pairs like dict.items()
PyObject* V24_constant_items(V24_BPy_constant *self)
{
	return PyDict_Items(self->dict);
}
//------------------------constant.keys()
//Returns a list of keys like dict.keys()
PyObject* V24_constant_keys(V24_BPy_constant *self)
{
	return PyDict_Keys(self->dict);
}
//------------------------constant.values()
//Returns a list of values like dict.values()
PyObject* V24_constant_values(V24_BPy_constant *self)
{
	return PyDict_Values(self->dict);
}
//------------------ATTRIBUTE IMPLEMENTATION---------------------------
//------------------TYPE_OBECT IMPLEMENTATION--------------------------
//-----------------------(internal)
//Creates a new constant object
static PyObject *new_const(void) 
{				
	V24_BPy_constant *constant;

	constant =	(V24_BPy_constant *) PyObject_NEW(V24_BPy_constant, &V24_constant_Type);
	if(constant == NULL){
		return (V24_EXPP_ReturnPyObjError(PyExc_MemoryError, 
			"couldn't create constant object"));
	}
	if((constant->dict = PyDict_New()) == NULL){
		return (V24_EXPP_ReturnPyObjError(PyExc_MemoryError,
			"couldn't create constant object's dictionary"));
	}

	return (PyObject *)constant;
}
//------------------------tp_doc
//The __doc__ string for this object
static char V24_BPy_constant_doc[] = "This is an internal subobject of armature\
designed to act as a Py_Bone dictionary.";

//------------------------tp_methods
//This contains a list of all methods the object contains
static PyMethodDef V24_BPy_constant_methods[] = {
	{"items", (PyCFunction) V24_constant_items, METH_NOARGS, 
		"() - Returns the key:value pairs from the dictionary"},
	{"keys", (PyCFunction) V24_constant_keys, METH_NOARGS, 
		"() - Returns the keys the dictionary"},
	{"values", (PyCFunction) V24_constant_values, METH_NOARGS, 
		"() - Returns the values from the dictionary"},
	{NULL, NULL, 0, NULL}
};
//------------------------mp_length
static int V24_constantLength(V24_BPy_constant *self)
{
	return 0;
}
//------------------------mp_subscript
static PyObject *V24_constantSubscript(V24_BPy_constant *self, PyObject *key)
{
	if(self->dict) {
		PyObject *v = PyDict_GetItem(self->dict, key);
		if(v) {
			return V24_EXPP_incr_ret(v);
		}
		return V24_EXPP_ReturnPyObjError( PyExc_KeyError,
				"key not found" );
	}
	return NULL;
}
//------------------------mp_ass_subscript
static int V24_constantAssSubscript(V24_BPy_constant *self, PyObject *who, PyObject *cares)
{
	return 0; /* no user assignments allowed */
}
//------------------------tp_getattro
static PyObject *V24_constant_getAttro(V24_BPy_constant * self, PyObject *value)
{
	if(self->dict) {
		PyObject *v;
		char *name = PyString_AS_STRING( value );

		if(!strcmp(name, "__members__"))
			return PyDict_Keys(self->dict);

#if 0
		if(!strcmp(name, "__methods__") || !strcmp(name, "__dict__")) {
			return PyObject_GenericGetAttr( (PyObject *)self, value );
		}
#endif

		v = PyDict_GetItemString(self->dict, name);
		if(v) {
			return V24_EXPP_incr_ret(v); /* was a borrowed ref */
		}
		return PyObject_GenericGetAttr( (PyObject *)self, value );
	}
	return (V24_EXPP_ReturnPyObjError(PyExc_RuntimeError,
					"constant object lacks a dictionary"));
}
//------------------------tp_repr
static PyObject *V24_constant_repr(V24_BPy_constant * self)
{
	char str[4096];
	PyObject *key, *value, *tempstr;
	int pos = 0;

	BLI_strncpy(str,"[Constant: ",4096);
	tempstr = PyString_FromString("name");
	value = PyDict_GetItem( self->dict, tempstr );
	Py_DECREF(tempstr);
	if(value) {
		strcat(str, PyString_AsString(value));
	} else {
		short sep = 0;
		strcat(str,"{");
		while (PyDict_Next(self->dict, &pos, &key, &value)) {
			if( sep )
				strcat (str, ", ");
			else
				sep = 1;
			strcat (str, PyString_AsString(key));
		}
		strcat(str,"}");
	}
	strcat(str, "]");
	return PyString_FromString(str);
}
//------------------------tp_dealloc
static void V24_constant_dealloc(V24_BPy_constant * self)
{
	Py_DECREF(self->dict);
	PyObject_DEL(self);
}

//------------------TYPE_OBECT DEFINITION------------------------------
static PyMappingMethods V24_constantAsMapping = {
	(inquiry) V24_constantLength,					// mp_length 
	(binaryfunc) V24_constantSubscript,				// mp_subscript
	(objobjargproc) V24_constantAssSubscript,		// mp_ass_subscript
};

PyTypeObject V24_constant_Type = {
	PyObject_HEAD_INIT(NULL)		//tp_head
	0,								//tp_internal
	"Constant",						//tp_name
	sizeof(V24_BPy_constant),			//tp_basicsize
	0,								//tp_itemsize
	(destructor)V24_constant_dealloc,	//tp_dealloc
	0,								//tp_print
	0,								//tp_getattr
	0,								//tp_setattr
	0,								//tp_compare
	(reprfunc) V24_constant_repr,		//tp_repr
	0,								//tp_as_number
	0,								//tp_as_sequence
	&V24_constantAsMapping,				//tp_as_mapping
	0,								//tp_hash
	0,								//tp_call
	0,								//tp_str
	(getattrofunc)V24_constant_getAttro,	//tp_getattro
	0,								//tp_setattro
	0,								//tp_as_buffer
	Py_TPFLAGS_DEFAULT,				//tp_flags
	V24_BPy_constant_doc,				//tp_doc
	0,								//tp_traverse
	0,								//tp_clear
	0,								//tp_richcompare
	0,								//tp_weaklistoffset
	0,								//tp_iter
	0,								//tp_iternext
	V24_BPy_constant_methods,			//tp_methods
	0,								//tp_members
	0,								//tp_getset
	0,								//tp_base
	0,								//tp_dict
	0,								//tp_descr_get
	0,								//tp_descr_set
	0,								//tp_dictoffset
	0,								//tp_init
	0,								//tp_alloc
	0,								//tp_new
	0,								//tp_free
	0,								//tp_is_gc
	0,								//tp_bases
	0,								//tp_mro
	0,								//tp_cache
	0,								//tp_subclasses
	0,								//tp_weaklist
	0								//tp_del
};
//------------------VISIBLE PROTOTYPE IMPLEMENTATION-------------------
//Creates a default empty constant
PyObject *V24_PyConstant_New(void)
{				
	return new_const();
}
//Inserts a key:value pair into the constant and then returns 0/1
int V24_PyConstant_Insert(V24_BPy_constant *self, char *name, PyObject *value)
{
	PyType_Ready( &V24_constant_Type );
	return V24_EXPP_dict_set_item_str(self->dict, name, value);
}
//This is a helper function for generating constants......
PyObject *V24_PyConstant_NewInt(char *name, int value)
{
        PyObject *constant = V24_PyConstant_New();

		if (constant)
		{                
			V24_PyConstant_Insert((V24_BPy_constant*)constant, "name", PyString_FromString(name));
			V24_PyConstant_Insert((V24_BPy_constant*)constant, "value", PyInt_FromLong(value));
		}
		return constant;
}
//This is a helper function for generating constants......
PyObject *V24_PyConstant_NewString(char *name, char *value)
{
		PyObject *constant = V24_PyConstant_New();

		if (constant)
		{
			V24_PyConstant_Insert((V24_BPy_constant*)constant, "name", PyString_FromString(name));
			V24_PyConstant_Insert((V24_BPy_constant*)constant, "value", PyString_FromString(value));
		}
		return constant;
}
