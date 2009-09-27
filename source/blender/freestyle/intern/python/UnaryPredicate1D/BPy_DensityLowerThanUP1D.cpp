#include "BPy_DensityLowerThanUP1D.h"

#include "../../stroke/AdvancedPredicates1D.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for DensityLowerThanUP1D instance  -----------*/
static int DensityLowerThanUP1D___init__(BPy_DensityLowerThanUP1D* self, PyObject *args);

/*-----------------------BPy_DensityLowerThanUP1D type definition ------------------------------*/

PyTypeObject DensityLowerThanUP1D_Type = {
	PyObject_HEAD_INIT(NULL)
	"DensityLowerThanUP1D",         /* tp_name */
	sizeof(BPy_DensityLowerThanUP1D), /* tp_basicsize */
	0,                              /* tp_itemsize */
	0,                              /* tp_dealloc */
	0,                              /* tp_print */
	0,                              /* tp_getattr */
	0,                              /* tp_setattr */
	0,                              /* tp_reserved */
	0,                              /* tp_repr */
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
	"DensityLowerThanUP1D objects", /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	0,                              /* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	&UnaryPredicate1D_Type,         /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	(initproc)DensityLowerThanUP1D___init__, /* tp_init */
	0,                              /* tp_alloc */
	0,                              /* tp_new */
};

//------------------------INSTANCE METHODS ----------------------------------

int DensityLowerThanUP1D___init__( BPy_DensityLowerThanUP1D* self, PyObject *args )
{
	double d1, d2 = 2.0;

	if( !PyArg_ParseTuple(args, "d|d", &d1, &d2) )
		return -1;
	
	self->py_up1D.up1D = new Predicates1D::DensityLowerThanUP1D(d1,d2);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
