/* 
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
 * Contributor(s): Willian P. Germano
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
*/

#include "Image.h"

/*****************************************************************************/
/* Function:              M_Image_New                                       */
/* Python equivalent:     Blender.Image.New                                 */
/*****************************************************************************/
static PyObject *M_Image_New(PyObject *self, PyObject *args, PyObject *keywords)
{
  printf ("In Image_New() - unimplemented in 2.25\n");

  Py_INCREF(Py_None);
  return Py_None;
}

/*****************************************************************************/
/* Function:              M_Image_Get                                        */
/* Python equivalent:     Blender.Image.Get                                  */
/* Description:           Receives a string and returns the image object     */
/*                        whose name matches the string.  If no argument is  */
/*                        passed in, a list of all image names in the        */
/*                        current scene is returned.                         */
/*****************************************************************************/
static PyObject *M_Image_Get(PyObject *self, PyObject *args)
{
  char  *name = NULL;
  Image *img_iter;

	if (!PyArg_ParseTuple(args, "|s", &name))
    return (EXPP_ReturnPyObjError (PyExc_TypeError,
            "expected string argument (or nothing)"));

  img_iter = G.main->image.first;

	if (name) { /* (name) - Search image by name */

    C_Image *wanted_image = NULL;

    while ((img_iter) && (wanted_image == NULL)) {
      if (strcmp (name, img_iter->id.name+2) == 0) {
        wanted_image = (C_Image *)PyObject_NEW(C_Image, &Image_Type);
				if (wanted_image) wanted_image->image = img_iter;
      }
      img_iter = img_iter->id.next;
    }

    if (wanted_image == NULL) { /* Requested image doesn't exist */
      char error_msg[64];
      PyOS_snprintf(error_msg, sizeof(error_msg),
                      "Image \"%s\" not found", name);
      return (EXPP_ReturnPyObjError (PyExc_NameError, error_msg));
    }

    return (PyObject *)wanted_image;
	}

	else { /* () - return a list of all images in the scene */
    int index = 0;
    PyObject *img_list, *pystr;

    img_list = PyList_New (BLI_countlist (&(G.main->image)));

    if (img_list == NULL)
      return (PythonReturnErrorObject (PyExc_MemoryError,
              "couldn't create PyList"));

		while (img_iter) {
      pystr = PyString_FromString (img_iter->id.name+2);

			if (!pystr)
				return (PythonReturnErrorObject (PyExc_MemoryError,
									"couldn't create PyString"));

			PyList_SET_ITEM (img_list, index, pystr);

      img_iter = img_iter->id.next;
      index++;
		}

		return (img_list);
	}
}

/*****************************************************************************/
/* Function:              M_Image_Load                                       */
/* Python equivalent:     Blender.Image.Load                                 */
/* Description:           Receives a string and returns the image object     */
/*                        whose filename matches the string.                 */
/*****************************************************************************/
static PyObject *M_Image_Load(PyObject *self, PyObject *args)
{
  char    *fname;
  Image   *img_ptr;
  C_Image *img;

  if (!PyArg_ParseTuple(args, "s", &fname))
    return (EXPP_ReturnPyObjError (PyExc_TypeError,
            "expected string argument"));

  img = (C_Image *)PyObject_NEW(C_Image, &Image_Type);

  if (!img)
    return (EXPP_ReturnPyObjError (PyExc_MemoryError,
            "couldn't create PyObject Image_Type"));

  img_ptr = add_image(fname);
  if (!img_ptr)
    return (EXPP_ReturnPyObjError (PyExc_IOError,
            "couldn't load image"));

  img->image = img_ptr;

  return (PyObject *)img;
}

/*****************************************************************************/
/* Function:              M_Image_Init                                      */
/*****************************************************************************/
PyObject *M_Image_Init (void)
{
  PyObject  *submodule;

  submodule = Py_InitModule3("Blender.Image", M_Image_methods, M_Image_doc);

  return (submodule);
}

/*****************************************************************************/
/* Python C_Image methods:                                                  */
/*****************************************************************************/
static PyObject *Image_getName(C_Image *self)
{
  PyObject *attr = PyString_FromString(self->image->id.name+2);

  if (attr) return attr;

  return (EXPP_ReturnPyObjError (PyExc_RuntimeError,
          "couldn't get Image.name attribute"));
}

static PyObject *Image_getFilename(C_Image *self)
{
  PyObject *attr = PyString_FromString(self->image->name);

  if (attr) return attr;

  return (EXPP_ReturnPyObjError (PyExc_RuntimeError,
          "couldn't get Image.filename attribute"));
}

static PyObject *Image_setName(C_Image *self, PyObject *args)
{
  char *name;
  char buf[21];

  if (!PyArg_ParseTuple(args, "s", &name))
    return (EXPP_ReturnPyObjError (PyExc_TypeError,
            "expected string argument"));
  
  PyOS_snprintf(buf, sizeof(buf), "%s", name);
  
  rename_id(&self->image->id, buf);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *Image_setXRep(C_Image *self, PyObject *args)
{
  short value;

  if (!PyArg_ParseTuple(args, "h", &value))
    return (EXPP_ReturnPyObjError (PyExc_TypeError,
            "expected int argument in [1,16]"));

  if (value >= EXPP_IMAGE_REP_MIN || value <= EXPP_IMAGE_REP_MAX)
    self->image->xrep = value;
  else
    return (EXPP_ReturnPyObjError (PyExc_ValueError,
            "expected int argument in [1,16]"));

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *Image_setYRep(C_Image *self, PyObject *args)
{
  short value;

  if (!PyArg_ParseTuple(args, "h", &value))
    return (EXPP_ReturnPyObjError (PyExc_TypeError,
            "expected int argument in [1,16]"));

  if (value >= EXPP_IMAGE_REP_MIN || value <= EXPP_IMAGE_REP_MAX)
    self->image->yrep = value;
  else
    return (EXPP_ReturnPyObjError (PyExc_ValueError,
            "expected int argument in [1,16]"));

  Py_INCREF(Py_None);
  return Py_None;
}

/*****************************************************************************/
/* Function:    ImageDeAlloc                                                */
/* Description: This is a callback function for the C_Image type. It is     */
/*              the destructor function.                                     */
/*****************************************************************************/
static void ImageDeAlloc (C_Image *self)
{
  PyObject_DEL (self);
}

/*****************************************************************************/
/* Function:    ImageGetAttr                                                */
/* Description: This is a callback function for the C_Image type. It is     */
/*              the function that accesses C_Image member variables and     */
/*              methods.                                                     */
/*****************************************************************************/
static PyObject* ImageGetAttr (C_Image *self, char *name)
{
  PyObject *attr = Py_None;

  if (strcmp(name, "name") == 0)
    attr = PyString_FromString(self->image->id.name+2);
  else if (strcmp(name, "filename") == 0)
    attr = PyString_FromString(self->image->name);
  else if (strcmp(name, "xrep") == 0)
    attr = PyInt_FromLong(self->image->xrep);
  else if (strcmp(name, "yrep") == 0)
    attr = PyInt_FromLong(self->image->yrep);

  else if (strcmp(name, "__members__") == 0)
    attr = Py_BuildValue("[s,s,s,s]",
                    "name", "filename", "xrep", "yrep");

  if (!attr)
    return (EXPP_ReturnPyObjError (PyExc_MemoryError,
                            "couldn't create PyObject"));

  if (attr != Py_None) return attr; /* attribute found, return its value */

  /* not an attribute, search the methods table */
  return Py_FindMethod(C_Image_methods, (PyObject *)self, name);
}

/*****************************************************************************/
/* Function:    ImageSetAttr                                                */
/* Description: This is a callback function for the C_Image type. It is the */
/*              function that changes Image Data members values. If this    */
/*              data is linked to a Blender Image, it also gets updated.    */
/*****************************************************************************/
static int ImageSetAttr (C_Image *self, char *name, PyObject *value)
{
  PyObject *valtuple; 
  PyObject *error = NULL;

/* We're playing a trick on the Python API users here.  Even if they use
 * Image.member = val instead of Image.setMember(value), we end up using the
 * function anyway, since it already has error checking, clamps to the right
 * interval and updates the Blender Image structure when necessary. */

  valtuple = Py_BuildValue("(N)", value); /* the set* functions expect a tuple */

  if (!valtuple)
    return EXPP_ReturnIntError(PyExc_MemoryError,
                  "ImageSetAttr: couldn't create PyTuple");

  if (strcmp (name, "name") == 0)
    error = Image_setName (self, valtuple);
  else if (strcmp (name, "xrep") == 0)
    error = Image_setXRep (self, valtuple);
  else if (strcmp (name, "yrep") == 0)
    error = Image_setYRep (self, valtuple);
  else { /* Error: no such member in the Image Data structure */
    Py_DECREF(value);
    Py_DECREF(valtuple);
    return (EXPP_ReturnIntError (PyExc_KeyError,
            "attribute not found or immutable"));
  }

  Py_DECREF(valtuple);

  if (error != Py_None) return -1;

  Py_DECREF(Py_None); /* incref'ed by the called set* function */
  return 0; /* normal exit */
}

/*****************************************************************************/
/* Function:    ImageCompare                                                 */
/* Description: This is a callback function for the C_Image type. It         */
/*              compares two Image_Type objects. Only the "==" and "!="      */
/*              comparisons are meaninful. Returns 0 for equality and -1 if  */
/*              they don't point to the same Blender Image struct.           */
/*              In Python it becomes 1 if they are equal, 0 otherwise.       */
/*****************************************************************************/
static int ImageCompare (C_Image *a, C_Image *b)
{
	Image *pa = a->image, *pb = b->image;
	return (pa == pb) ? 0:-1;
}

/*****************************************************************************/
/* Function:    ImagePrint                                                  */
/* Description: This is a callback function for the C_Image type. It        */
/*              builds a meaninful string to 'print' image objects.         */
/*****************************************************************************/
static int ImagePrint(C_Image *self, FILE *fp, int flags)
{ 
  fprintf(fp, "[Image \"%s\"]", self->image->id.name+2);
  return 0;
}

/*****************************************************************************/
/* Function:    ImageRepr                                                   */
/* Description: This is a callback function for the C_Image type. It        */
/*              builds a meaninful string to represent image objects.       */
/*****************************************************************************/
static PyObject *ImageRepr (C_Image *self)
{
  return PyString_FromString(self->image->id.name+2);
}
