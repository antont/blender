#include "BPy_Interface0DIterator.h"

#include "../BPy_Convert.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for Interface0DIterator instance  -----------*/
static int Interface0DIterator___init__(BPy_Interface0DIterator *self, PyObject *args);

static PyObject * Interface0DIterator_t( BPy_Interface0DIterator *self );
static PyObject * Interface0DIterator_u( BPy_Interface0DIterator *self );

static PyObject * Interface0DIterator_getObject(BPy_Interface0DIterator *self);

/*----------------------Interface0DIterator instance definitions ----------------------------*/
static PyMethodDef BPy_Interface0DIterator_methods[] = {
	{"t", ( PyCFunction ) Interface0DIterator_t, METH_NOARGS, "（ ）Returns the curvilinear abscissa."},
	{"u", ( PyCFunction ) Interface0DIterator_u, METH_NOARGS, "（ ）Returns the point parameter in the curve 0<=u<=1."},
	{"getObject", ( PyCFunction ) Interface0DIterator_getObject, METH_NOARGS, "() Get object referenced by the iterator"},
	{NULL, NULL, 0, NULL}
};

/*-----------------------BPy_Interface0DIterator type definition ------------------------------*/

PyTypeObject Interface0DIterator_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,							/* ob_size */
	"Interface0DIterator",				/* tp_name */
	sizeof( BPy_Interface0DIterator ),	/* tp_basicsize */
	0,							/* tp_itemsize */
	
	/* methods */
	NULL,	/* tp_dealloc */
	NULL,                       				/* printfunc tp_print; */
	NULL,                       				/* getattrfunc tp_getattr; */
	NULL,                       				/* setattrfunc tp_setattr; */
	NULL,										/* tp_compare */
	NULL,					/* tp_repr */

	/* Method suites for standard classes */

	NULL,                       /* PyNumberMethods *tp_as_number; */
	NULL,                       /* PySequenceMethods *tp_as_sequence; */
	NULL,                       /* PyMappingMethods *tp_as_mapping; */

	/* More standard operations (here for binary compatibility) */

	NULL,						/* hashfunc tp_hash; */
	NULL,                       /* ternaryfunc tp_call; */
	NULL,                       /* reprfunc tp_str; */
	NULL,                       /* getattrofunc tp_getattro; */
	NULL,                       /* setattrofunc tp_setattro; */

	/* Functions to access object as input/output buffer */
	NULL,                       /* PyBufferProcs *tp_as_buffer; */

  /*** Flags to define presence of optional/expanded features ***/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 		/* long tp_flags; */

	NULL,                       /*  char *tp_doc;  Documentation string */
  /*** Assigned meaning in release 2.0 ***/
	/* call function for all accessible objects */
	NULL,                       /* traverseproc tp_traverse; */

	/* delete references to contained objects */
	NULL,                       /* inquiry tp_clear; */

  /***  Assigned meaning in release 2.1 ***/
  /*** rich comparisons ***/
	NULL,                       /* richcmpfunc tp_richcompare; */

  /***  weak reference enabler ***/
	0,                          /* long tp_weaklistoffset; */

  /*** Added in release 2.2 ***/
	/*   Iterators */
	NULL,                       /* getiterfunc tp_iter; */
	NULL,                       /* iternextfunc tp_iternext; */

  /*** Attribute descriptor and subclassing stuff ***/
	BPy_Interface0DIterator_methods,	/* struct PyMethodDef *tp_methods; */
	NULL,                       	/* struct PyMemberDef *tp_members; */
	NULL,         					/* struct PyGetSetDef *tp_getset; */
	&Iterator_Type,				/* struct _typeobject *tp_base; */
	NULL,							/* PyObject *tp_dict; */
	NULL,							/* descrgetfunc tp_descr_get; */
	NULL,							/* descrsetfunc tp_descr_set; */
	0,                          	/* long tp_dictoffset; */
	(initproc)Interface0DIterator___init__,                       	/* initproc tp_init; */
	NULL,							/* allocfunc tp_alloc; */
	NULL,		/* newfunc tp_new; */
	
	/*  Low-level free-memory routine */
	NULL,                       /* freefunc tp_free;  */
	
	/* For PyObject_IS_GC */
	NULL,                       /* inquiry tp_is_gc;  */
	NULL,                       /* PyObject *tp_bases; */
	
	/* method resolution order */
	NULL,                       /* PyObject *tp_mro;  */
	NULL,                       /* PyObject *tp_cache; */
	NULL,                       /* PyObject *tp_subclasses; */
	NULL,                       /* PyObject *tp_weaklist; */
	NULL
};

//-------------------MODULE INITIALIZATION--------------------------------


//------------------------INSTANCE METHODS ----------------------------------

int Interface0DIterator___init__(BPy_Interface0DIterator *self, PyObject *args )
{	
	PyObject *obj = 0;

	if (!( PyArg_ParseTuple(args, "O!", &Interface0DIterator_Type, &obj) ))
	    return -1;

	self->if0D_it = new Interface0DIterator(*( ((BPy_Interface0DIterator *) obj)->if0D_it ));
	self->py_it.it = self->if0D_it;
	
	return 0;
}

PyObject * Interface0DIterator_t( BPy_Interface0DIterator *self ) {
	return PyFloat_FromDouble( self->if0D_it->t() );
}

PyObject * Interface0DIterator_u( BPy_Interface0DIterator *self ) {
	return PyFloat_FromDouble( self->if0D_it->u() );
}

PyObject * Interface0DIterator_getObject(BPy_Interface0DIterator *self) {
	Interface0D &if0D = self->if0D_it->operator*();
	if (typeid(if0D) == typeid(CurvePoint)) {
		return BPy_CurvePoint_from_CurvePoint_ptr(dynamic_cast<CurvePoint*>(&if0D));
	} else if (typeid(if0D) == typeid(StrokeVertex)) {
		return BPy_StrokeVertex_from_StrokeVertex_ptr(dynamic_cast<StrokeVertex*>(&if0D));
	} else if (typeid(if0D) == typeid(SVertex)) {
		return BPy_SVertex_from_SVertex_ptr(dynamic_cast<SVertex*>(&if0D));
	} else if (typeid(if0D) == typeid(ViewVertex)) {
		return BPy_ViewVertex_from_ViewVertex_ptr(dynamic_cast<ViewVertex*>(&if0D));
	} else if (typeid(if0D) == typeid(NonTVertex)) {
		return BPy_NonTVertex_from_NonTVertex_ptr(dynamic_cast<NonTVertex*>(&if0D));
	} else if (typeid(if0D) == typeid(TVertex)) {
		return BPy_TVertex_from_TVertex_ptr(dynamic_cast<TVertex*>(&if0D));
	} else {
		cerr << "Warning: cast to " << if0D.getExactTypeName() << " not implemented" << endl;
		return BPy_Interface0D_from_Interface0D(if0D);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
