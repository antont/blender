#include "BPy_ColorVariationPatternShader.h"

#include "../../stroke/BasicStrokeShaders.h"
#include "../BPy_Convert.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

//------------------------INSTANCE METHODS ----------------------------------

static char ColorVariationPatternShader___doc__[] =
"Class hierarchy: :class:`StrokeShader` > :class:`ColorVariationPatternShader`\n"
"\n"
"[Color shader]\n"
"\n"
".. method:: __init__(pattern_name, stretch=True)\n"
"\n"
"   Builds a ColorVariationPatternShader object.\n"
"\n"
"   :arg pattern_name: The file name of the texture file to use as\n"
"      pattern.\n"
"   :type pattern_name: str\n"
"   :arg stretch: Tells whether the texture must be strecthed or\n"
"      repeted to fit the stroke.\n"
"   :type stretch: bool\n"
"\n"
".. method:: shade(stroke)\n"
"\n"
"   Applies a pattern to vary the original color.  The new color is the\n"
"   result of the multiplication of the pattern and the original color.\n"
"\n"
"   :arg stroke: A Stroke object.\n"
"   :type stroke: :class:`Stroke`\n";

static int ColorVariationPatternShader___init__(BPy_ColorVariationPatternShader* self, PyObject *args, PyObject *kwds)
{
	static const char *kwlist[] = {"pattern_name", "stretch", NULL};
	const char *s;
	PyObject *obj = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O!", (char **)kwlist, &s, &PyBool_Type, &obj))
		return -1;
	bool b = (!obj) ? true : bool_from_PyBool(obj);
	self->py_ss.ss = new StrokeShaders::ColorVariationPatternShader(s, b);
	return 0;
}

/*-----------------------BPy_ColorVariationPatternShader type definition ------------------------------*/

PyTypeObject ColorVariationPatternShader_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"ColorVariationPatternShader",  /* tp_name */
	sizeof(BPy_ColorVariationPatternShader), /* tp_basicsize */
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
	ColorVariationPatternShader___doc__, /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	0,                              /* tp_methods */
	0,                              /* tp_members */
	0,                              /* tp_getset */
	&StrokeShader_Type,             /* tp_base */
	0,                              /* tp_dict */
	0,                              /* tp_descr_get */
	0,                              /* tp_descr_set */
	0,                              /* tp_dictoffset */
	(initproc)ColorVariationPatternShader___init__, /* tp_init */
	0,                              /* tp_alloc */
	0,                              /* tp_new */
};

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
