#include "BPy_UnaryPredicate1D.h"

#include "BPy_Convert.h"
#include "BPy_Interface1D.h"

#include "UnaryPredicate1D/BPy_ContourUP1D.h"
#include "UnaryPredicate1D/BPy_DensityLowerThanUP1D.h"
#include "UnaryPredicate1D/BPy_EqualToChainingTimeStampUP1D.h"
#include "UnaryPredicate1D/BPy_EqualToTimeStampUP1D.h"
#include "UnaryPredicate1D/BPy_ExternalContourUP1D.h"
#include "UnaryPredicate1D/BPy_FalseUP1D.h"
#include "UnaryPredicate1D/BPy_QuantitativeInvisibilityUP1D.h"
#include "UnaryPredicate1D/BPy_ShapeUP1D.h"
#include "UnaryPredicate1D/BPy_TrueUP1D.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for UnaryPredicate1D instance  -----------*/
static int UnaryPredicate1D___init__(BPy_UnaryPredicate1D *self, PyObject *args, PyObject *kwds);
static void UnaryPredicate1D___dealloc__(BPy_UnaryPredicate1D *self);
static PyObject * UnaryPredicate1D___repr__(BPy_UnaryPredicate1D *self);

static PyObject * UnaryPredicate1D_getName( BPy_UnaryPredicate1D *self, PyObject *args);
static PyObject * UnaryPredicate1D___call__( BPy_UnaryPredicate1D *self, PyObject *args, PyObject *kwds);

/*----------------------UnaryPredicate1D instance definitions ----------------------------*/
static PyMethodDef BPy_UnaryPredicate1D_methods[] = {
	{"getName", ( PyCFunction ) UnaryPredicate1D_getName, METH_NOARGS, ""},
	{NULL, NULL, 0, NULL}
};

/*-----------------------BPy_UnaryPredicate1D type definition ------------------------------*/

PyTypeObject UnaryPredicate1D_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,							/* ob_size */
	"UnaryPredicate1D",				/* tp_name */
	sizeof( BPy_UnaryPredicate1D ),	/* tp_basicsize */
	0,							/* tp_itemsize */
	
	/* methods */
	(destructor)UnaryPredicate1D___dealloc__,	/* tp_dealloc */
	NULL,                       				/* printfunc tp_print; */
	NULL,                       				/* getattrfunc tp_getattr; */
	NULL,                       				/* setattrfunc tp_setattr; */
	NULL,										/* tp_compare */
	(reprfunc)UnaryPredicate1D___repr__,					/* tp_repr */

	/* Method suites for standard classes */

	NULL,                       /* PyNumberMethods *tp_as_number; */
	NULL,                       /* PySequenceMethods *tp_as_sequence; */
	NULL,                       /* PyMappingMethods *tp_as_mapping; */

	/* More standard operations (here for binary compatibility) */

	NULL,						/* hashfunc tp_hash; */
	(ternaryfunc)UnaryPredicate1D___call__,                       /* ternaryfunc tp_call; */
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
	BPy_UnaryPredicate1D_methods,	/* struct PyMethodDef *tp_methods; */
	NULL,                       	/* struct PyMemberDef *tp_members; */
	NULL,         					/* struct PyGetSetDef *tp_getset; */
	NULL,							/* struct _typeobject *tp_base; */
	NULL,							/* PyObject *tp_dict; */
	NULL,							/* descrgetfunc tp_descr_get; */
	NULL,							/* descrsetfunc tp_descr_set; */
	0,                          	/* long tp_dictoffset; */
	(initproc)UnaryPredicate1D___init__, /* initproc tp_init; */
	NULL,							/* allocfunc tp_alloc; */
	PyType_GenericNew,		/* newfunc tp_new; */
	
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
PyMODINIT_FUNC UnaryPredicate1D_Init( PyObject *module )
{
	if( module == NULL )
		return;

	if( PyType_Ready( &UnaryPredicate1D_Type ) < 0 )
		return;
	Py_INCREF( &UnaryPredicate1D_Type );
	PyModule_AddObject(module, "UnaryPredicate1D", (PyObject *)&UnaryPredicate1D_Type);
	
	if( PyType_Ready( &ContourUP1D_Type ) < 0 )
		return;
	Py_INCREF( &ContourUP1D_Type );
	PyModule_AddObject(module, "ContourUP1D", (PyObject *)&ContourUP1D_Type);
	
	if( PyType_Ready( &DensityLowerThanUP1D_Type ) < 0 )
		return;
	Py_INCREF( &DensityLowerThanUP1D_Type );
	PyModule_AddObject(module, "DensityLowerThanUP1D", (PyObject *)&DensityLowerThanUP1D_Type);
	
	if( PyType_Ready( &EqualToChainingTimeStampUP1D_Type ) < 0 )
		return;
	Py_INCREF( &EqualToChainingTimeStampUP1D_Type );
	PyModule_AddObject(module, "EqualToChainingTimeStampUP1D", (PyObject *)&EqualToChainingTimeStampUP1D_Type);

	if( PyType_Ready( &EqualToTimeStampUP1D_Type ) < 0 )
		return;
	Py_INCREF( &EqualToTimeStampUP1D_Type );
	PyModule_AddObject(module, "EqualToTimeStampUP1D", (PyObject *)&EqualToTimeStampUP1D_Type);
	
	if( PyType_Ready( &ExternalContourUP1D_Type ) < 0 )
		return;
	Py_INCREF( &ExternalContourUP1D_Type );
	PyModule_AddObject(module, "ExternalContourUP1D", (PyObject *)&ExternalContourUP1D_Type);
	
	if( PyType_Ready( &FalseUP1D_Type ) < 0 )
		return;
	Py_INCREF( &FalseUP1D_Type );
	PyModule_AddObject(module, "FalseUP1D", (PyObject *)&FalseUP1D_Type);
	
	if( PyType_Ready( &QuantitativeInvisibilityUP1D_Type ) < 0 )
		return;
	Py_INCREF( &QuantitativeInvisibilityUP1D_Type );
	PyModule_AddObject(module, "QuantitativeInvisibilityUP1D", (PyObject *)&QuantitativeInvisibilityUP1D_Type);
	
	if( PyType_Ready( &ShapeUP1D_Type ) < 0 )
		return;
	Py_INCREF( &ShapeUP1D_Type );
	PyModule_AddObject(module, "ShapeUP1D", (PyObject *)&ShapeUP1D_Type);
	
	if( PyType_Ready( &TrueUP1D_Type ) < 0 )
		return;
	Py_INCREF( &TrueUP1D_Type );
	PyModule_AddObject(module, "TrueUP1D", (PyObject *)&TrueUP1D_Type);
}

//------------------------INSTANCE METHODS ----------------------------------

int UnaryPredicate1D___init__(BPy_UnaryPredicate1D *self, PyObject *args, PyObject *kwds)
{
	self->up1D = new UnaryPredicate1D();
	self->up1D->py_up1D = (PyObject *) self;
	return 0;
}

void UnaryPredicate1D___dealloc__(BPy_UnaryPredicate1D* self)
{
	if (self->up1D)
		delete self->up1D;
    self->ob_type->tp_free((PyObject*)self);
}


PyObject * UnaryPredicate1D___repr__(BPy_UnaryPredicate1D* self)
{
    return PyString_FromFormat("type: %s - address: %p", self->up1D->getName().c_str(), self->up1D );
}


PyObject * UnaryPredicate1D_getName( BPy_UnaryPredicate1D *self, PyObject *args)
{
	return PyString_FromString( self->up1D->getName().c_str() );
}

static PyObject * UnaryPredicate1D___call__( BPy_UnaryPredicate1D *self, PyObject *args, PyObject *kwds)
{
	PyObject *py_if1D;

	if( kwds != NULL ) {
		PyErr_SetString(PyExc_TypeError, "keyword argument(s) not supported");
		return NULL;
	}
	if( !PyArg_ParseTuple(args, "O!", &Interface1D_Type, &py_if1D) )
		return NULL;
	
	Interface1D *if1D = ((BPy_Interface1D *) py_if1D)->if1D;
	
	if( !if1D ) {
		string msg(self->up1D->getName() + " has no Interface0DIterator");
		PyErr_SetString(PyExc_RuntimeError, msg.c_str());
		return NULL;
	}
	if( typeid(*(self->up1D)) == typeid(UnaryPredicate1D) ) {
		PyErr_SetString(PyExc_TypeError, "__call__ method not properly overridden");
		return NULL;
	}
	if( self->up1D->operator()(*if1D) < 0 ) {
		if (!PyErr_Occurred()) {
			string msg(self->up1D->getName() + " __call__ method failed");
			PyErr_SetString(PyExc_RuntimeError, msg.c_str());
		}
		return NULL;
	}
	return PyBool_from_bool( self->up1D->result );
}


///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif



