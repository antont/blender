#include "BPy_orientedViewEdgeIterator.h"

#include "../BPy_Convert.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for orientedViewEdgeIterator instance  -----------*/
static int orientedViewEdgeIterator___init__(BPy_orientedViewEdgeIterator *self, PyObject *args);
static PyObject * orientedViewEdgeIterator_iternext(BPy_orientedViewEdgeIterator *self);

static PyObject * orientedViewEdgeIterator_getObject(BPy_orientedViewEdgeIterator *self);


/*----------------------orientedViewEdgeIterator instance definitions ----------------------------*/
static PyMethodDef BPy_orientedViewEdgeIterator_methods[] = {
	{"getObject", ( PyCFunction ) orientedViewEdgeIterator_getObject, METH_NOARGS, "() Get object referenced by the iterator"},
	{NULL, NULL, 0, NULL}
};

/*-----------------------BPy_orientedViewEdgeIterator type definition ------------------------------*/

PyTypeObject orientedViewEdgeIterator_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,							/* ob_size */
	"orientedViewEdgeIterator",				/* tp_name */
	sizeof( BPy_orientedViewEdgeIterator ),	/* tp_basicsize */
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
	PyObject_SelfIter,				/* getiterfunc tp_iter; */
	(iternextfunc)orientedViewEdgeIterator_iternext,	/* iternextfunc tp_iternext; */

  /*** Attribute descriptor and subclassing stuff ***/
	BPy_orientedViewEdgeIterator_methods,	/* struct PyMethodDef *tp_methods; */
	NULL,                       	/* struct PyMemberDef *tp_members; */
	NULL,         					/* struct PyGetSetDef *tp_getset; */
	&Iterator_Type,				/* struct _typeobject *tp_base; */
	NULL,							/* PyObject *tp_dict; */
	NULL,							/* descrgetfunc tp_descr_get; */
	NULL,							/* descrsetfunc tp_descr_set; */
	0,                          	/* long tp_dictoffset; */
	(initproc)orientedViewEdgeIterator___init__,                       	/* initproc tp_init; */
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

//------------------------INSTANCE METHODS ----------------------------------

int orientedViewEdgeIterator___init__(BPy_orientedViewEdgeIterator *self, PyObject *args )
{	
	PyObject *obj = 0;

	if (!( PyArg_ParseTuple(args, "|O", &obj) ))
	    return -1;

	if( !obj )
		self->ove_it = new ViewVertexInternal::orientedViewEdgeIterator();
	else if( BPy_orientedViewEdgeIterator_Check(obj) )
		self->ove_it = new ViewVertexInternal::orientedViewEdgeIterator(*( ((BPy_orientedViewEdgeIterator *) obj)->ove_it ));
	else {
		PyErr_SetString(PyExc_TypeError, "invalid argument");
		return -1;
	}
	
	self->py_it.it = self->ove_it;
	self->reversed = 0;
	
	return 0;
}

PyObject * orientedViewEdgeIterator_iternext( BPy_orientedViewEdgeIterator *self ) {
	ViewVertex::directedViewEdge *dve;
	if (self->reversed) {
		if (self->ove_it->isBegin()) {
			PyErr_SetNone(PyExc_StopIteration);
			return NULL;
		}
		self->ove_it->decrement();
		dve = self->ove_it->operator->();
	} else {
		if (self->ove_it->isEnd()) {
			PyErr_SetNone(PyExc_StopIteration);
			return NULL;
		}
		dve = self->ove_it->operator->();
		self->ove_it->increment();
	}
	return BPy_directedViewEdge_from_directedViewEdge( *dve );
}

PyObject * orientedViewEdgeIterator_getObject( BPy_orientedViewEdgeIterator *self) {
	return BPy_directedViewEdge_from_directedViewEdge( self->ove_it->operator*() );
}

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
