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
#include "Mathutils.h"

#include "BLI_blenlib.h"
#include "BKE_utildefines.h"
#include "gen_utils.h"

//-------------------------DOC STRINGS ---------------------------
char V24_Point_Zero_doc[] = "() - set all values in the point to 0";
char V24_Point_toVector_doc[] = "() - create a vector representation of this point";
//-----------------------METHOD DEFINITIONS ----------------------
struct PyMethodDef Point_methods[] = {
	{"zero", (PyCFunction) V24_Point_Zero, METH_NOARGS, V24_Point_Zero_doc},
	{"toVector", (PyCFunction) V24_Point_toVector, METH_NOARGS, V24_Point_toVector_doc},
	{NULL, NULL, 0, NULL}
};
//-----------------------------METHODS----------------------------
//--------------------------Vector.toPoint()----------------------
//create a new point object to represent this vector
PyObject *V24_Point_toVector(V24_PointObject * self)
{
	float vec[3];
	int x;

	for(x = 0; x < self->size; x++){
		vec[x] = self->coord[x];
	}
	
	return V24_newVectorObject(vec, self->size, Py_NEW);
}
//----------------------------Point.zero() ----------------------
//set the point data to 0,0,0
PyObject *V24_Point_Zero(V24_PointObject * self)
{
	int x;
	for(x = 0; x < self->size; x++) {
		self->coord[x] = 0.0f;
	}
	return V24_EXPP_incr_ret((PyObject*)self);
}
//----------------------------dealloc()(internal) ----------------
//free the py_object
static void V24_Point_dealloc(V24_PointObject * self)
{
	Py_XDECREF(self->coerced_object);
	//only free py_data
	if(self->data.py_data){
		PyMem_Free(self->data.py_data);
	}
	PyObject_DEL(self);
}
//----------------------------getattr()(internal) ----------------
//object.attribute access (get)
static PyObject *V24_Point_getattr(V24_PointObject * self, char *name)
{
	if(STREQ(name,"x")){
		return PyFloat_FromDouble(self->coord[0]);
	}else if(STREQ(name, "y")){
		return PyFloat_FromDouble(self->coord[1]);
	}else if(STREQ(name, "z")){
		if(self->size > 2){
			return PyFloat_FromDouble(self->coord[2]);
		}else{
			return V24_EXPP_ReturnPyObjError(PyExc_AttributeError,
				"point.z: illegal attribute access\n");
		}
	}
	if(STREQ(name, "wrapped")){
		if(self->wrapped == Py_WRAP)
			return V24_EXPP_incr_ret((PyObject *)Py_True);
		else 
			return V24_EXPP_incr_ret((PyObject *)Py_False);
	}
	return Py_FindMethod(Point_methods, (PyObject *) self, name);
}
//----------------------------setattr()(internal) ----------------
//object.attribute access (set)
static int V24_Point_setattr(V24_PointObject * self, char *name, PyObject * v)
{
	PyObject *f = NULL;

	f = PyNumber_Float(v);
	if(f == NULL) { // parsed item not a number
		return V24_EXPP_ReturnIntError(PyExc_TypeError, 
			"point.attribute = x: argument not a number\n");
	}

	if(STREQ(name,"x")){
		self->coord[0] = (float)PyFloat_AS_DOUBLE(f);
	}else if(STREQ(name, "y")){
		self->coord[1] = (float)PyFloat_AS_DOUBLE(f);
	}else if(STREQ(name, "z")){
		if(self->size > 2){
			self->coord[2] = (float)PyFloat_AS_DOUBLE(f);
		}else{
			Py_DECREF(f);
			return V24_EXPP_ReturnIntError(PyExc_AttributeError,
				"point.z = x: illegal attribute access\n");
		}
	}else{
		Py_DECREF(f);
		return V24_EXPP_ReturnIntError(PyExc_AttributeError,
				"point.attribute = x: unknown attribute\n");
	}

	Py_DECREF(f);
	return 0;
}
//----------------------------print object (internal)-------------
//print the object to screen
static PyObject *V24_Point_repr(V24_PointObject * self)
{
	int i;
	char buffer[48], str[1024];

	BLI_strncpy(str,"[",1024);
	for(i = 0; i < self->size; i++){
		if(i < (self->size - 1)){
			sprintf(buffer, "%.6f, ", self->coord[i]);
			strcat(str,buffer);
		}else{
			sprintf(buffer, "%.6f", self->coord[i]);
			strcat(str,buffer);
		}
	}
	strcat(str, "](point)");

	return PyString_FromString(str);
}
//---------------------SEQUENCE PROTOCOLS------------------------
//----------------------------len(object)------------------------
//sequence length
static int V24_Point_len(V24_PointObject * self)
{
	return self->size;
}
//----------------------------object[]---------------------------
//sequence accessor (get)
static PyObject *V24_Point_item(V24_PointObject * self, int i)
{
	if(i < 0 || i >= self->size)
		return V24_EXPP_ReturnPyObjError(PyExc_IndexError,
		"point[attribute]: array index out of range\n");

	return PyFloat_FromDouble( (double)self->coord[i] );

}
//----------------------------object[]-------------------------
//sequence accessor (set)
static int V24_Point_ass_item(V24_PointObject * self, int i, PyObject * ob)
{
	PyObject *f = NULL;

	f = PyNumber_Float(ob);
	if(f == NULL) { // parsed item not a number
		return V24_EXPP_ReturnIntError(PyExc_TypeError, 
			"point[attribute] = x: argument not a number\n");
	}

	if(i < 0 || i >= self->size){
		Py_DECREF(f);
		return V24_EXPP_ReturnIntError(PyExc_IndexError,
			"point[attribute] = x: array assignment index out of range\n");
	}
	self->coord[i] = (float)PyFloat_AS_DOUBLE(f);
	Py_DECREF(f);
	return 0;
}
//----------------------------object[z:y]------------------------
//sequence slice (get)
static PyObject *V24_Point_slice(V24_PointObject * self, int begin, int end)
{
	PyObject *list = NULL;
	int count;

	CLAMP(begin, 0, self->size);
	CLAMP(end, 0, self->size);
	begin = MIN2(begin,end);

	list = PyList_New(end - begin);
	for(count = begin; count < end; count++) {
		PyList_SetItem(list, count - begin,
				PyFloat_FromDouble(self->coord[count]));
	}

	return list;
}
//----------------------------object[z:y]------------------------
//sequence slice (set)
static int V24_Point_ass_slice(V24_PointObject * self, int begin, int end,
			     PyObject * seq)
{
	int i, y, size = 0;
	float coord[3];
	PyObject *v, *f;

	CLAMP(begin, 0, self->size);
	CLAMP(end, 0, self->size);
	begin = MIN2(begin,end);

	size = PySequence_Length(seq);
	if(size != (end - begin)){
		return V24_EXPP_ReturnIntError(PyExc_TypeError,
			"point[begin:end] = []: size mismatch in slice assignment\n");
	}

	for (i = 0; i < size; i++) {
		v = PySequence_GetItem(seq, i);
		if (v == NULL) { // Failed to read sequence
			return V24_EXPP_ReturnIntError(PyExc_RuntimeError, 
				"point[begin:end] = []: unable to read sequence\n");
		}
		f = PyNumber_Float(v);
		if(f == NULL) { // parsed item not a number
			Py_DECREF(v);
			return V24_EXPP_ReturnIntError(PyExc_TypeError, 
				"point[begin:end] = []: sequence argument not a number\n");
		}

		coord[i] = (float)PyFloat_AS_DOUBLE(f);
		V24_EXPP_decr2(f,v);
	}
	//parsed well - now set in point
	for(y = 0; y < size; y++){
		self->coord[begin + y] = coord[y];
	}
	return 0;
}
//------------------------NUMERIC PROTOCOLS----------------------
//------------------------obj + obj------------------------------
//addition
static PyObject *V24_Point_add(PyObject * v1, PyObject * v2)
{
	int x, size;
	float coord[3];
	V24_PointObject *coord1 = NULL, *coord2 = NULL;
	V24_VectorObject *vec = NULL;

	coord1 = (V24_PointObject*)v1;
	coord2 = (V24_PointObject*)v2;

	if(!coord1->coerced_object){
		if(coord2->coerced_object){
			if(V24_VectorObject_Check(coord2->coerced_object)){  //POINT + VECTOR
				//Point translation
				vec = (V24_VectorObject*)coord2->coerced_object;
				size = coord1->size;
				if(vec->size == size){
					for(x = 0; x < size; x++){
						coord[x] = coord1->coord[x] + vec->vec[x];
					}	
				}else{
					return V24_EXPP_ReturnPyObjError(PyExc_AttributeError,
						"Point addition: arguments are the wrong size....\n");
				}
				return V24_newPointObject(coord, size, Py_NEW);
			}	
		}else{  //POINT + POINT
			size = coord1->size;
			if(coord2->size == size){
				for(x = 0; x < size; x++) {
					coord[x] = coord1->coord[x] + coord2->coord[x];
				}
			}else{
				return V24_EXPP_ReturnPyObjError(PyExc_AttributeError,
					"Point addition: arguments are the wrong size....\n");
			}
			return V24_newPointObject(coord, size, Py_NEW);
		}
	}

	return V24_EXPP_ReturnPyObjError(PyExc_AttributeError,
		"Point addition: arguments not valid for this operation....\n");
}
//------------------------obj - obj------------------------------
//subtraction
static PyObject *V24_Point_sub(PyObject * v1, PyObject * v2)
{
	int x, size;
	float coord[3];
	V24_PointObject *coord1 = NULL, *coord2 = NULL;

	coord1 = (V24_PointObject*)v1;
	coord2 = (V24_PointObject*)v2;

	if(coord1->coerced_object || coord2->coerced_object){
		return V24_EXPP_ReturnPyObjError(PyExc_AttributeError,
			"Point subtraction: arguments not valid for this operation....\n");
	}
	if(coord1->size != coord2->size){
		return V24_EXPP_ReturnPyObjError(PyExc_AttributeError,
		"Point subtraction: points must have the same dimensions for this operation\n");
	}

	size = coord1->size;
	for(x = 0; x < size; x++) {
		coord[x] = coord1->coord[x] -	coord2->coord[x];
	}

	//Point - Point = Vector
	return V24_newVectorObject(coord, size, Py_NEW);
}
//------------------------obj * obj------------------------------
//mulplication
static PyObject *V24_Point_mul(PyObject * p1, PyObject * p2)
{
	int x, size;
	float coord[3], scalar;
	V24_PointObject *coord1 = NULL, *coord2 = NULL;
	PyObject *f = NULL;
	V24_MatrixObject *mat = NULL;
	V24_QuaternionObject *quat = NULL;

	coord1 = (V24_PointObject*)p1;
	coord2 = (V24_PointObject*)p2;

	if(coord1->coerced_object){
		if (PyFloat_Check(coord1->coerced_object) || 
			PyInt_Check(coord1->coerced_object)){	// FLOAT/INT * POINT
			f = PyNumber_Float(coord1->coerced_object);
			if(f == NULL) { // parsed item not a number
				return V24_EXPP_ReturnPyObjError(PyExc_TypeError, 
					"Point multiplication: arguments not acceptable for this operation\n");
			}

			scalar = (float)PyFloat_AS_DOUBLE(f);
			size = coord2->size;
			for(x = 0; x < size; x++) {
				coord[x] = coord2->coord[x] *	scalar;
			}
			Py_DECREF(f);
			return V24_newPointObject(coord, size, Py_NEW);
		}
	}else{
		if(coord2->coerced_object){
			if (PyFloat_Check(coord2->coerced_object) || 
				PyInt_Check(coord2->coerced_object)){	// POINT * FLOAT/INT
				f = PyNumber_Float(coord2->coerced_object);
				if(f == NULL) { // parsed item not a number
					return V24_EXPP_ReturnPyObjError(PyExc_TypeError, 
						"Point multiplication: arguments not acceptable for this operation\n");
				}

				scalar = (float)PyFloat_AS_DOUBLE(f);
				size = coord1->size;
				for(x = 0; x < size; x++) {
					coord[x] = coord1->coord[x] *	scalar;
				}
				Py_DECREF(f);
				return V24_newPointObject(coord, size, Py_NEW);
			}else if(V24_MatrixObject_Check(coord2->coerced_object)){ //POINT * MATRIX
				mat = (V24_MatrixObject*)coord2->coerced_object;
				return V24_row_point_multiplication(coord1, mat);
			}else if(V24_QuaternionObject_Check(coord2->coerced_object)){  //POINT * QUATERNION
				quat = (V24_QuaternionObject*)coord2->coerced_object;
				if(coord1->size != 3){
					return V24_EXPP_ReturnPyObjError(PyExc_TypeError, 
						"Point multiplication: only 3D point rotations (with quats) currently supported\n");
				}
				return quat_rotation((PyObject*)coord1, (PyObject*)quat);
			}
		}
	}

	return V24_EXPP_ReturnPyObjError(PyExc_TypeError, 
		"Point multiplication: arguments not acceptable for this operation\n");
}
//-------------------------- -obj -------------------------------
//returns the negative of this object
static PyObject *V24_Point_neg(V24_PointObject *self)
{
	int x;
	float coord[3];

	for(x = 0; x < self->size; x++)
		coord[x] = -self->coord[x];

	return V24_newPointObject(coord, self->size, Py_NEW);
}

//------------------------coerce(obj, obj)-----------------------
//coercion of unknown types to type V24_PointObject for numeric protocols
/*Coercion() is called whenever a math operation has 2 operands that
 it doesn't understand how to evaluate. 2+Matrix for example. We want to 
 evaluate some of these operations like: (vector * 2), however, for math
 to proceed, the unknown operand must be cast to a type that python math will
 understand. (e.g. in the case above case, 2 must be cast to a vector and 
 then call vector.multiply(vector, scalar_cast_as_vector)*/
static int V24_Point_coerce(PyObject ** p1, PyObject ** p2)
{
	if(V24_VectorObject_Check(*p2) || PyFloat_Check(*p2) || PyInt_Check(*p2) ||
			V24_MatrixObject_Check(*p2) || V24_QuaternionObject_Check(*p2)) {
		PyObject *coerced = V24_EXPP_incr_ret(*p2);
		*p2 = V24_newPointObject(NULL,3,Py_NEW);
		((V24_PointObject*)*p2)->coerced_object = coerced;
		Py_INCREF (*p1);
		return 0;
	}

	return V24_EXPP_ReturnIntError(PyExc_TypeError, 
		"point.coerce(): unknown operand - can't coerce for numeric protocols");
}
//-----------------PROTOCOL DECLARATIONS--------------------------
static PySequenceMethods V24_Point_SeqMethods = {
	(inquiry) V24_Point_len,						/* sq_length */
	(binaryfunc) 0,								/* sq_concat */
	(intargfunc) 0,								/* sq_repeat */
	(intargfunc) V24_Point_item,					/* sq_item */
	(intintargfunc) V24_Point_slice,				/* sq_slice */
	(intobjargproc) V24_Point_ass_item,				/* sq_ass_item */
	(intintobjargproc) V24_Point_ass_slice,			/* sq_ass_slice */
};
static PyNumberMethods V24_Point_NumMethods = {
	(binaryfunc) V24_Point_add,						/* __add__ */
	(binaryfunc) V24_Point_sub,						/* __sub__ */
	(binaryfunc) V24_Point_mul,						/* __mul__ */
	(binaryfunc) 0,								/* __div__ */
	(binaryfunc) 0,								/* __mod__ */
	(binaryfunc) 0,								/* __divmod__ */
	(ternaryfunc) 0,							/* __pow__ */
	(unaryfunc) V24_Point_neg,						/* __neg__ */
	(unaryfunc) 0,								/* __pos__ */
	(unaryfunc) 0,								/* __abs__ */
	(inquiry) 0,								/* __nonzero__ */
	(unaryfunc) 0,								/* __invert__ */
	(binaryfunc) 0,								/* __lshift__ */
	(binaryfunc) 0,								/* __rshift__ */
	(binaryfunc) 0,								/* __and__ */
	(binaryfunc) 0,								/* __xor__ */
	(binaryfunc) 0,								/* __or__ */
	(coercion)  V24_Point_coerce,					/* __coerce__ */
	(unaryfunc) 0,								/* __int__ */
	(unaryfunc) 0,								/* __long__ */
	(unaryfunc) 0,								/* __float__ */
	(unaryfunc) 0,								/* __oct__ */
	(unaryfunc) 0,								/* __hex__ */

};
//------------------PY_OBECT DEFINITION--------------------------
PyTypeObject V24_point_Type = {
	PyObject_HEAD_INIT(NULL) 
	0,											/*ob_size */
	"point",									/*tp_name */
	sizeof(V24_PointObject),						/*tp_basicsize */
	0,											/*tp_itemsize */
	(destructor) V24_Point_dealloc,					/*tp_dealloc */
	(printfunc) 0,								/*tp_print */
	(getattrfunc) V24_Point_getattr,				/*tp_getattr */
	(setattrfunc) V24_Point_setattr,				/*tp_setattr */
	0,											/*tp_compare */
	(reprfunc) V24_Point_repr,						/*tp_repr */
	&V24_Point_NumMethods,							/*tp_as_number */
	&V24_Point_SeqMethods,							/*tp_as_sequence */
};
//------------------------V24_newPointObject (internal)-------------
//creates a new point object
/*pass Py_WRAP - if point is a WRAPPER for data allocated by BLENDER
 (i.e. it was allocated elsewhere by MEM_mallocN())
  pass Py_NEW - if point is not a WRAPPER and managed by PYTHON
 (i.e. it must be created here with PyMEM_malloc())*/
PyObject *V24_newPointObject(float *coord, int size, int type)
{
	V24_PointObject *self;
	int x;

	V24_point_Type.ob_type = &PyType_Type;
	self = PyObject_NEW(V24_PointObject, &V24_point_Type);
	self->data.blend_data = NULL;
	self->data.py_data = NULL;
	if(size > 3 || size < 2)
		return NULL;
	self->size = size;
	self->coerced_object = NULL;

	if(type == Py_WRAP){
		self->data.blend_data = coord;
		self->coord = self->data.blend_data;
		self->wrapped = Py_WRAP;
	}else if (type == Py_NEW){
		self->data.py_data = PyMem_Malloc(size * sizeof(float));
		self->coord = self->data.py_data;
		if(!coord) { //new empty
			for(x = 0; x < size; x++){
				self->coord[x] = 0.0f;
			}
		}else{
			for(x = 0; x < size; x++){
				self->coord[x] = coord[x];
			}
		}
		self->wrapped = Py_NEW;
	}else{ //bad type
		return NULL;
	}
	return (PyObject *) self;
}
