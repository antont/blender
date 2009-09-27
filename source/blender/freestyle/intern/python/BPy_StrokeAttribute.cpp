#include "BPy_StrokeAttribute.h"

#include "BPy_Convert.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for StrokeAttribute instance  -----------*/
static int StrokeAttribute___init__(BPy_StrokeAttribute *self, PyObject *args, PyObject *kwds);
static void StrokeAttribute___dealloc__(BPy_StrokeAttribute *self);
static PyObject * StrokeAttribute___repr__(BPy_StrokeAttribute *self);

static PyObject * StrokeAttribute_getColorR( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getColorG( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getColorB( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getColorRGB( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getAlpha( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getThicknessR( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getThicknessL( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getThicknessRL( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_isVisible( BPy_StrokeAttribute *self );
static PyObject * StrokeAttribute_getAttributeReal( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_getAttributeVec2f( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_getAttributeVec3f( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_isAttributeAvailableReal( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_isAttributeAvailableVec2f( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_isAttributeAvailableVec3f( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setColor( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setAlpha( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setThickness( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setVisible( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setAttributeReal( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setAttributeVec2f( BPy_StrokeAttribute *self, PyObject *args );
static PyObject * StrokeAttribute_setAttributeVec3f( BPy_StrokeAttribute *self, PyObject *args );


/*----------------------StrokeAttribute instance definitions ----------------------------*/
static PyMethodDef BPy_StrokeAttribute_methods[] = {
	{"getColorR", ( PyCFunction ) StrokeAttribute_getColorR, METH_NOARGS, "Returns the R color component. "},
	{"getColorG", ( PyCFunction ) StrokeAttribute_getColorG, METH_NOARGS, "Returns the G color component. "},
	{"getColorB", ( PyCFunction ) StrokeAttribute_getColorB, METH_NOARGS, "Returns the B color component. "},
	{"getColorRGB", ( PyCFunction ) StrokeAttribute_getColorRGB, METH_NOARGS, "Returns the RGB color components."},
	{"getAlpha", ( PyCFunction ) StrokeAttribute_getAlpha, METH_NOARGS, "Returns the alpha color component."},
	{"getThicknessR", ( PyCFunction ) StrokeAttribute_getThicknessR, METH_NOARGS, "Returns the thickness on the right of the vertex when following the stroke. "},
	{"getThicknessL", ( PyCFunction ) StrokeAttribute_getThicknessL, METH_NOARGS, "Returns the thickness on the left of the vertex when following the stroke."},
	{"getThicknessRL", ( PyCFunction ) StrokeAttribute_getThicknessRL, METH_NOARGS, "Returns the thickness on the right and on the left of the vertex when following the stroke. "},
	{"isVisible", ( PyCFunction ) StrokeAttribute_isVisible, METH_NOARGS, "Returns true if the strokevertex is visible, false otherwise"},
	{"getAttributeReal", ( PyCFunction ) StrokeAttribute_getAttributeReal, METH_VARARGS, "(name) Returns an attribute of type real specified by name."},
	{"getAttributeVec2f", ( PyCFunction ) StrokeAttribute_getAttributeVec2f, METH_VARARGS, "(name) Returns an attribute of type Vec2f specified by name."},
	{"getAttributeVec3f", ( PyCFunction ) StrokeAttribute_getAttributeVec3f, METH_VARARGS, "(name) Returns an attribute of type Vec3f specified by name."},
	{"isAttributeAvailableReal", ( PyCFunction ) StrokeAttribute_isAttributeAvailableReal, METH_VARARGS, "(name) Checks whether the real attribute specified by name is available"},
	{"isAttributeAvailableVec2f", ( PyCFunction ) StrokeAttribute_isAttributeAvailableVec2f, METH_VARARGS, "(name) Checks whether the Vec2f attribute specified by name is available"},
	{"isAttributeAvailableVec3f", ( PyCFunction ) StrokeAttribute_isAttributeAvailableVec3f, METH_VARARGS, "(name) Checks whether the Vec3f attribute specified by name is available"},
	{"setColor", ( PyCFunction ) StrokeAttribute_setColor, METH_VARARGS, "(float a)Sets the attribute's alpha value. "},
	{"setAlpha", ( PyCFunction ) StrokeAttribute_setAlpha, METH_VARARGS, "(float a) Sets the attribute's alpha value."},
	{"setThickness", ( PyCFunction ) StrokeAttribute_setThickness, METH_VARARGS, ""},
	{"setVisible", ( PyCFunction ) StrokeAttribute_setVisible, METH_VARARGS, ""},
	{"setAttributeReal", ( PyCFunction ) StrokeAttribute_setAttributeReal, METH_VARARGS, "(name, float att) Adds a user defined attribute of type real. If there is no attribute of specified by name, it is added. Otherwise, the new value replaces the old one."},
	{"setAttributeVec2f", ( PyCFunction ) StrokeAttribute_setAttributeVec2f, METH_VARARGS, "(name, float att) Adds a user defined attribute of type Vec2f. If there is no attribute of specified by name, it is added. Otherwise, the new value replaces the old one."},
	{"setAttributeVec3f", ( PyCFunction ) StrokeAttribute_setAttributeVec3f, METH_VARARGS, "(name, float att) Adds a user defined attribute of type Vec4f. If there is no attribute of specified by name, it is added. Otherwise, the new value replaces the old one."},
	{NULL, NULL, 0, NULL}
};



/*-----------------------BPy_StrokeAttribute type definition ------------------------------*/

PyTypeObject StrokeAttribute_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"StrokeAttribute",              /* tp_name */
	sizeof(BPy_StrokeAttribute),    /* tp_basicsize */
	0,                              /* tp_itemsize */
	(destructor)StrokeAttribute___dealloc__, /* tp_dealloc */
	0,                              /* tp_print */
	0,                              /* tp_getattr */
	0,                              /* tp_setattr */
	0,                              /* tp_reserved */
	(reprfunc)StrokeAttribute___repr__, /* tp_repr */
	0,                              /* tp_as_number */
	0,                              /* tp_as_sequence */
	0,                              /* tp_as_mapping */
	0,                              /* tp_hash  */
	0,                              /* tp_call */
	0,                              /* tp_str */
	0,                              /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"StrokeAttribute objects",      /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	BPy_StrokeAttribute_methods,    /* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	0,                              /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	(initproc)StrokeAttribute___init__, /* tp_init */
	0,                              /* tp_alloc */
	PyType_GenericNew,              /* tp_new */
};

//-------------------MODULE INITIALIZATION--------------------------------
int StrokeAttribute_Init( PyObject *module )
{
	if( module == NULL )
		return -1;

	if( PyType_Ready( &StrokeAttribute_Type ) < 0 )
		return -1;
	Py_INCREF( &StrokeAttribute_Type );
	PyModule_AddObject(module, "StrokeAttribute", (PyObject *)&StrokeAttribute_Type);
	return 0;
}

//------------------------INSTANCE METHODS ----------------------------------

int StrokeAttribute___init__(BPy_StrokeAttribute *self, PyObject *args, PyObject *kwds)
{

	PyObject *obj1 = 0, *obj2 = 0 , *obj3 = 0, *obj4 = 0, *obj5 = 0 , *obj6 = 0;

    if (! PyArg_ParseTuple(args, "|OOOOOO", &obj1, &obj2, &obj3, &obj4, &obj5, &obj6) )
        return -1;

	if( !obj1 || !obj2 || !obj3 ){
		
		self->sa = new StrokeAttribute();
		
	} else if( 	BPy_StrokeAttribute_Check(obj1) && 
				BPy_StrokeAttribute_Check(obj2) &&
				PyFloat_Check(obj3) ) {	
		
			self->sa = new StrokeAttribute(	*( ((BPy_StrokeAttribute *) obj1)->sa ),
											*( ((BPy_StrokeAttribute *) obj2)->sa ),
											PyFloat_AsDouble( obj3 ) );	
										
	} else if( 	obj4 && obj5 && obj6 ) {
	
			self->sa = new StrokeAttribute(	PyFloat_AsDouble( obj1 ),
											PyFloat_AsDouble( obj2 ),
											PyFloat_AsDouble( obj3 ),
											PyFloat_AsDouble( obj4 ),
											PyFloat_AsDouble( obj5 ),
											PyFloat_AsDouble( obj6 ) );

	} else {
		PyErr_SetString(PyExc_TypeError, "invalid arguments");
		return -1;
	}

	self->borrowed = 0;

	return 0;

}

void StrokeAttribute___dealloc__(BPy_StrokeAttribute* self)
{
	if( self->sa && !self->borrowed )
		delete self->sa;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject * StrokeAttribute___repr__(BPy_StrokeAttribute* self)
{
	stringstream repr("StrokeAttribute:");
	repr << " r: " << self->sa->getColorR()
		 << " g: " << self->sa->getColorG()
		 << " b: " << self->sa->getColorB()
		 << " a: " << self->sa->getAlpha()
		 << " - R: " << self->sa->getThicknessR() 
		 << " L: " << self->sa->getThicknessL();

	return PyUnicode_FromFormat( repr.str().c_str() );
}


PyObject *StrokeAttribute_getColorR( BPy_StrokeAttribute *self ) {
	return PyFloat_FromDouble( self->sa->getColorR() );	
}

PyObject *StrokeAttribute_getColorG( BPy_StrokeAttribute *self ) {
	return PyFloat_FromDouble( self->sa->getColorG() );	
}

PyObject *StrokeAttribute_getColorB( BPy_StrokeAttribute *self ) {
	return PyFloat_FromDouble( self->sa->getColorB() );	
}

PyObject *StrokeAttribute_getColorRGB( BPy_StrokeAttribute *self ) {
	Vec3f v( self->sa->getColorRGB() );
	return Vector_from_Vec3f( v );	
}

PyObject *StrokeAttribute_getAlpha( BPy_StrokeAttribute *self ) {
	return PyFloat_FromDouble( self->sa->getAlpha() );	
}

PyObject *StrokeAttribute_getThicknessR( BPy_StrokeAttribute *self ) {
	return PyFloat_FromDouble( self->sa->getThicknessR() );	
}
PyObject *StrokeAttribute_getThicknessL( BPy_StrokeAttribute *self ) {
	return PyFloat_FromDouble( self->sa->getThicknessL() );	
}
PyObject *StrokeAttribute_getThicknessRL( BPy_StrokeAttribute *self ) {
	Vec2f v( self->sa->getThicknessRL() );
	return Vector_from_Vec2f( v );
}

PyObject *StrokeAttribute_isVisible( BPy_StrokeAttribute *self ) {
	return PyBool_from_bool( self->sa->isVisible() );	
}


PyObject *StrokeAttribute_getAttributeReal( BPy_StrokeAttribute *self, PyObject *args ) {
	char *attr;

	if(!( PyArg_ParseTuple(args, "s", &attr) ))
		return NULL;

	double a = self->sa->getAttributeReal( attr );
	return PyFloat_FromDouble( a );
}

PyObject *StrokeAttribute_getAttributeVec2f( BPy_StrokeAttribute *self, PyObject *args ) {
	char *attr;

	if(!( PyArg_ParseTuple(args, "s", &attr) ))
		return NULL;

	Vec2f a = self->sa->getAttributeVec2f( attr );
	return Vector_from_Vec2f( a );
}


PyObject *StrokeAttribute_getAttributeVec3f( BPy_StrokeAttribute *self, PyObject *args ) {
	char *attr;

	if(!( PyArg_ParseTuple(args, "s", &attr) ))
		return NULL;

	Vec3f a = self->sa->getAttributeVec3f( attr );
	return Vector_from_Vec3f( a );
}

PyObject *StrokeAttribute_isAttributeAvailableReal( BPy_StrokeAttribute *self, PyObject *args ) {
	char *attr;

	if(!( PyArg_ParseTuple(args, "s", &attr) ))
		return NULL;

	return PyBool_from_bool( self->sa->isAttributeAvailableReal( attr ) );
}

PyObject *StrokeAttribute_isAttributeAvailableVec2f( BPy_StrokeAttribute *self, PyObject *args ) {
	char *attr;

	if(!( PyArg_ParseTuple(args, "s", &attr) ))
		return NULL;

	return PyBool_from_bool( self->sa->isAttributeAvailableVec2f( attr ) );
}

PyObject *StrokeAttribute_isAttributeAvailableVec3f( BPy_StrokeAttribute *self, PyObject *args ) {
	char *attr;

	if(!( PyArg_ParseTuple(args, "s", &attr) ))
		return NULL;

	return PyBool_from_bool( self->sa->isAttributeAvailableVec3f( attr ) );
}


PyObject * StrokeAttribute_setColor( BPy_StrokeAttribute *self, PyObject *args ) {
	PyObject *obj1 = 0, *obj2 = 0, *obj3 = 0 ;

	if(!( PyArg_ParseTuple(args, "O|OO", &obj1, &obj2, &obj3) ))
		return NULL;

	if( obj1 && !obj2 && !obj3 ){

		Vec3f *v = Vec3f_ptr_from_PyObject(obj1);
		if( !v ) {
			PyErr_SetString(PyExc_TypeError, "argument 1 must be a 3D vector (either a list of 3 elements or Vector)");
			return NULL;
		}
		self->sa->setColor( *v );
		delete v;
		
	} else if( 	obj1 && obj2 && obj3 ){

		self->sa->setColor(	PyFloat_AsDouble(obj1),
							PyFloat_AsDouble(obj2),
							PyFloat_AsDouble(obj3) );

	} else {
		PyErr_SetString(PyExc_TypeError, "invalid arguments");
		return NULL;
	}
	
	Py_RETURN_NONE;
}

PyObject * StrokeAttribute_setAlpha( BPy_StrokeAttribute *self, PyObject *args ){
	float f = 0;

	if(!( PyArg_ParseTuple(args, "f", &f) ))
		return NULL;
	
	self->sa->setAlpha( f );
	Py_RETURN_NONE;
}

PyObject * StrokeAttribute_setThickness( BPy_StrokeAttribute *self, PyObject *args )  {
	PyObject *obj1 = 0, *obj2 = 0;

	if(!( PyArg_ParseTuple(args, "O|O", &obj1, &obj2) ))
		return NULL;

	if( obj1 && !obj2 ){
		
		Vec2f *v = Vec2f_ptr_from_PyObject(obj1);
		if( !v ) {
			PyErr_SetString(PyExc_TypeError, "argument 1 must be a 2D vector (either a list of 2 elements or Vector)");
			return NULL;
		}
		self->sa->setThickness( *v );
		delete v;
				
	} else if( 	obj1 && obj2 ){
					
		self->sa->setThickness(	PyFloat_AsDouble(obj1),
								PyFloat_AsDouble(obj2) );

	} else {
		PyErr_SetString(PyExc_TypeError, "invalid arguments");
		return NULL;
	}
	
	Py_RETURN_NONE;
}

PyObject * StrokeAttribute_setVisible( BPy_StrokeAttribute *self, PyObject *args ) {
	PyObject *py_b;

	if(!( PyArg_ParseTuple(args, "O", &py_b) ))
		return NULL;

	self->sa->setVisible( bool_from_PyBool(py_b) );

	Py_RETURN_NONE;
}


PyObject * StrokeAttribute_setAttributeReal( BPy_StrokeAttribute *self, PyObject *args ) {
	char *s = 0;
	double d = 0;

	if(!( PyArg_ParseTuple(args, "sd", &s, &d) ))
		return NULL;

	self->sa->setAttributeReal( s, d );
	Py_RETURN_NONE;
}

PyObject * StrokeAttribute_setAttributeVec2f( BPy_StrokeAttribute *self, PyObject *args ) {
	char *s;
	PyObject *obj = 0;

	if(!( PyArg_ParseTuple(args, "sO", &s, &obj) ))
		return NULL;
	Vec2f *v = Vec2f_ptr_from_PyObject(obj);
	if( !v ) {
		PyErr_SetString(PyExc_TypeError, "argument 2 must be a 2D vector (either a list of 2 elements or Vector)");
		return NULL;
	}
	self->sa->setAttributeVec2f( s, *v );
	delete v;

	Py_RETURN_NONE;
}

PyObject * StrokeAttribute_setAttributeVec3f( BPy_StrokeAttribute *self, PyObject *args ) {
	char *s;
	PyObject *obj = 0;

	if(!( PyArg_ParseTuple(args, "sO", &s, &obj) ))
		return NULL;
	Vec3f *v = Vec3f_ptr_from_PyObject(obj);
	if( !v ) {
		PyErr_SetString(PyExc_TypeError, "argument 2 must be a 3D vector (either a list of 3 elements or Vector)");
		return NULL;
	}
	self->sa->setAttributeVec3f( s, *v );
	delete v;

	Py_RETURN_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

