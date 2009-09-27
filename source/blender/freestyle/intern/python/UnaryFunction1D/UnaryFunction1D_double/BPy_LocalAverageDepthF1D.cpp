#include "BPy_LocalAverageDepthF1D.h"

#include "../../../stroke/AdvancedFunctions1D.h"
#include "../../BPy_Convert.h"
#include "../../BPy_IntegrationType.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for LocalAverageDepthF1D instance  -----------*/
static int LocalAverageDepthF1D___init__(BPy_LocalAverageDepthF1D* self, PyObject *args);

/*-----------------------BPy_LocalAverageDepthF1D type definition ------------------------------*/

PyTypeObject LocalAverageDepthF1D_Type = {
	PyObject_HEAD_INIT(NULL)
	"LocalAverageDepthF1D",         /* tp_name */
	sizeof(BPy_LocalAverageDepthF1D), /* tp_basicsize */
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
	"LocalAverageDepthF1D objects", /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	0,                              /* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	&UnaryFunction1DDouble_Type,    /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	(initproc)LocalAverageDepthF1D___init__, /* tp_init */
	0,                              /* tp_alloc */
	0,                              /* tp_new */
};

//------------------------INSTANCE METHODS ----------------------------------

int LocalAverageDepthF1D___init__( BPy_LocalAverageDepthF1D* self, PyObject *args)
{
	PyObject *obj = 0;
	double d;

	if( !PyArg_ParseTuple(args, "d|O!", &d, &IntegrationType_Type, &obj) )
		return -1;
	
	IntegrationType t = ( obj ) ? IntegrationType_from_BPy_IntegrationType(obj) : MEAN;
	self->py_uf1D_double.uf1D_double = new Functions1D::LocalAverageDepthF1D(d,t);
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
