/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Blender Foundation (2008).
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/makesrna/intern/rna_color.c
 *  \ingroup RNA
 */


#include <stdlib.h>
#include <stdio.h>

#include "RNA_define.h"
#include "rna_internal.h"

#include "DNA_color_types.h"
#include "DNA_texture_types.h"

#include "WM_api.h"
#include "WM_types.h"

static EnumPropertyItem view_transform_items[] = {
	{0, "NONE", 0, "None", "Do not perform any color transform on display, use old non-color managed technique for display"},
	{0, NULL, 0, NULL, NULL}
};

static EnumPropertyItem color_space_items[] = {
	{0, "NONE", 0, "None", "Do not perform any color transform on load, treat colors as in scene linear space already"},
	{0, NULL, 0, NULL, NULL}
};

#ifdef RNA_RUNTIME

#include "RNA_access.h"

#include "DNA_image_types.h"
#include "DNA_material_types.h"
#include "DNA_movieclip_types.h"
#include "DNA_node_types.h"

#include "MEM_guardedalloc.h"

#include "BKE_colortools.h"
#include "BKE_depsgraph.h"
#include "BKE_image.h"
#include "BKE_movieclip.h"
#include "BKE_node.h"
#include "BKE_texture.h"

#include "ED_node.h"

#include "IMB_colormanagement.h"

static int rna_CurveMapping_curves_length(PointerRNA *ptr)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;
	int len;

	for (len = 0; len < CM_TOT; len++)
		if (!cumap->cm[len].curve)
			break;
	
	return len;
}

static void rna_CurveMapping_curves_begin(CollectionPropertyIterator *iter, PointerRNA *ptr)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;

	rna_iterator_array_begin(iter, cumap->cm, sizeof(CurveMap), rna_CurveMapping_curves_length(ptr), 0, NULL);
}

static void rna_CurveMapping_clip_set(PointerRNA *ptr, int value)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;

	if (value) cumap->flag |= CUMA_DO_CLIP;
	else cumap->flag &= ~CUMA_DO_CLIP;

	curvemapping_changed(cumap, FALSE);
}

static void rna_CurveMapping_black_level_set(PointerRNA *ptr, const float *values)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;
	cumap->black[0] = values[0];
	cumap->black[1] = values[1];
	cumap->black[2] = values[2];
	curvemapping_set_black_white(cumap, NULL, NULL);
}

static void rna_CurveMapping_white_level_set(PointerRNA *ptr, const float *values)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;
	cumap->white[0] = values[0];
	cumap->white[1] = values[1];
	cumap->white[2] = values[2];
	curvemapping_set_black_white(cumap, NULL, NULL);
}

static void rna_CurveMapping_clipminx_range(PointerRNA *ptr, float *min, float *max, float *softmin, float *softmax)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;

	*min = -100.0f;
	*max = cumap->clipr.xmax;
}

static void rna_CurveMapping_clipminy_range(PointerRNA *ptr, float *min, float *max, float *softmin, float *softmax)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;

	*min = -100.0f;
	*max = cumap->clipr.ymax;
}

static void rna_CurveMapping_clipmaxx_range(PointerRNA *ptr, float *min, float *max, float *softmin, float *softmax)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;

	*min = cumap->clipr.xmin;
	*max = 100.0f;
}

static void rna_CurveMapping_clipmaxy_range(PointerRNA *ptr, float *min, float *max, float *softmin, float *softmax)
{
	CurveMapping *cumap = (CurveMapping *)ptr->data;

	*min = cumap->clipr.ymin;
	*max = 100.0f;
}


static char *rna_ColorRamp_path(PointerRNA *ptr)
{
	char *path = NULL;
	
	/* handle the cases where a single datablock may have 2 ramp types */
	if (ptr->id.data) {
		ID *id = ptr->id.data;
		
		switch (GS(id->name)) {
			case ID_MA: /* material has 2 cases - diffuse and specular */
			{
				Material *ma = (Material *)id;
				
				if (ptr->data == ma->ramp_col)
					path = BLI_strdup("diffuse_ramp");
				else if (ptr->data == ma->ramp_spec)
					path = BLI_strdup("specular_ramp");
				break;
			}
			
			case ID_NT:
			{
				bNodeTree *ntree = (bNodeTree *)id;
				bNode *node;
				PointerRNA node_ptr;
				char *node_path;
				
				for (node = ntree->nodes.first; node; node = node->next) {
					if (ELEM3(node->type, SH_NODE_VALTORGB, CMP_NODE_VALTORGB, TEX_NODE_VALTORGB)) {
						if (node->storage == ptr->data) {
							/* all node color ramp properties called 'color_ramp'
							 * prepend path from ID to the node
							 */
							RNA_pointer_create(id, &RNA_Node, node, &node_ptr);
							node_path = RNA_path_from_ID_to_struct(&node_ptr);
							path = BLI_sprintfN("%s.color_ramp", node_path);
							MEM_freeN(node_path);
						}
					}
				}
				break;
			}
			
			default:
				/* everything else just uses 'color_ramp' */
				path = BLI_strdup("color_ramp");
				break;
		}
	}
	else {
		/* everything else just uses 'color_ramp' */
		path = BLI_strdup("color_ramp");
	}
	
	return path;
}

static char *rna_ColorRampElement_path(PointerRNA *ptr)
{
	PointerRNA ramp_ptr;
	PropertyRNA *prop;
	char *path = NULL;
	int index;
	
	/* helper macro for use here to try and get the path
	 *	- this calls the standard code for getting a path to a texture...
	 */

#define COLRAMP_GETPATH                                                       \
{                                                                             \
	prop = RNA_struct_find_property(&ramp_ptr, "elements");                   \
	if (prop) {                                                               \
		index = RNA_property_collection_lookup_index(&ramp_ptr, prop, ptr);   \
		if (index >= 0) {                                                     \
			char *texture_path = rna_ColorRamp_path(&ramp_ptr);               \
			path = BLI_sprintfN("%s.elements[%d]", texture_path, index);      \
			MEM_freeN(texture_path);                                          \
		}                                                                     \
	}                                                                         \
} (void)0

	/* determine the path from the ID-block to the ramp */
	/* FIXME: this is a very slow way to do it, but it will have to suffice... */
	if (ptr->id.data) {
		ID *id = ptr->id.data;
		
		switch (GS(id->name)) {
			case ID_MA: /* 2 cases for material - diffuse and spec */
			{
				Material *ma = (Material *)id;
				
				/* try diffuse first */
				if (ma->ramp_col) {
					RNA_pointer_create(id, &RNA_ColorRamp, ma->ramp_col, &ramp_ptr);
					COLRAMP_GETPATH;
				}
				/* try specular if not diffuse */
				if (!path && ma->ramp_spec) {
					RNA_pointer_create(id, &RNA_ColorRamp, ma->ramp_spec, &ramp_ptr);
					COLRAMP_GETPATH;
				}
			}
			break;
				
			case ID_NT:
			{
				bNodeTree *ntree = (bNodeTree *)id;
				bNode *node;
				
				for (node = ntree->nodes.first; node; node = node->next) {
					if (ELEM3(node->type, SH_NODE_VALTORGB, CMP_NODE_VALTORGB, TEX_NODE_VALTORGB)) {
						RNA_pointer_create(id, &RNA_ColorRamp, node->storage, &ramp_ptr);
						COLRAMP_GETPATH;
					}
				}
			}
			break;
				
			default: /* everything else should have a "color_ramp" property */
			{
				/* create pointer to the ID block, and try to resolve "color_ramp" pointer */
				RNA_id_pointer_create(id, &ramp_ptr);
				if (RNA_path_resolve(&ramp_ptr, "color_ramp", &ramp_ptr, &prop)) {
					COLRAMP_GETPATH;
				}
			}
		}
	}
	
	/* cleanup the macro we defined */
#undef COLRAMP_GETPATH
	
	return path;
}

static void rna_ColorRamp_update(Main *bmain, Scene *UNUSED(scene), PointerRNA *ptr)
{
	if (ptr->id.data) {
		ID *id = ptr->id.data;
		
		switch (GS(id->name)) {
			case ID_MA:
			{
				Material *ma = ptr->id.data;
				
				DAG_id_tag_update(&ma->id, 0);
				WM_main_add_notifier(NC_MATERIAL | ND_SHADING_DRAW, ma);
			}
			break;
			case ID_NT:
			{
				bNodeTree *ntree = (bNodeTree *)id;
				bNode *node;

				for (node = ntree->nodes.first; node; node = node->next) {
					if (ELEM3(node->type, SH_NODE_VALTORGB, CMP_NODE_VALTORGB, TEX_NODE_VALTORGB)) {
						ED_node_generic_update(bmain, ntree, node);
					}
				}
			}
			break;
			case ID_TE:
			{
				Tex *tex = ptr->id.data;

				DAG_id_tag_update(&tex->id, 0);
				WM_main_add_notifier(NC_TEXTURE, tex);
			}
			break;
			default:
				break;
		}
	}
}

void rna_ColorRamp_eval(struct ColorBand *coba, float position, float color[4])
{
	do_colorband(coba, position, color);
}

static CBData *rna_ColorRampElement_new(struct ColorBand *coba, ReportList *reports, float position)
{
	CBData *element = colorband_element_add(coba, position);

	if (element == NULL)
		BKE_reportf(reports, RPT_ERROR, "Unable to add element to colorband (limit %d)", MAXCOLORBAND);

	return element;
}

static void rna_ColorRampElement_remove(struct ColorBand *coba, ReportList *reports, CBData *element)
{
	int index = (int)(element - coba->data);
	if (colorband_element_remove(coba, index) == 0)
		BKE_report(reports, RPT_ERROR, "Element not found in element collection or last element");

}

static void rna_Scopes_update(Main *UNUSED(bmain), Scene *UNUSED(scene), PointerRNA *ptr)
{
	Scopes *s = (Scopes *)ptr->data;
	s->ok = 0;
}

static int rna_ColorManagedDisplaySettings_display_device_get(struct PointerRNA *ptr)
{
	ColorManagedDisplaySettings *display = (ColorManagedDisplaySettings *) ptr->data;

	return IMB_colormanagement_display_get_named_index(display->display_device);
}

static void rna_ColorManagedDisplaySettings_display_device_set(struct PointerRNA *ptr, int value)
{
	ColorManagedDisplaySettings *display = (ColorManagedDisplaySettings *) ptr->data;
	const char *name = IMB_colormanagement_display_get_indexed_name(value);

	if (name) {
		BLI_strncpy(display->display_device, name, sizeof(display->display_device));
	}
}

static EnumPropertyItem *rna_ColorManagedDisplaySettings_display_device_itemf(bContext *C, PointerRNA *ptr, PropertyRNA *UNUSED(prop), int *free)
{
	EnumPropertyItem *items = NULL;
	int totitem = 0;

	IMB_colormanagement_display_items_add(&items, &totitem);
	RNA_enum_item_end(&items, &totitem);

	*free = TRUE;

	return items;
}

static int rna_ColorManagedViewSettings_view_transform_get(PointerRNA *ptr)
{
	ColorManagedViewSettings *view = (ColorManagedViewSettings *) ptr->data;

	return IMB_colormanagement_view_get_named_index(view->view_transform);
}

static void rna_ColorManagedViewSettings_view_transform_set(PointerRNA *ptr, int value)
{
	ColorManagedViewSettings *view = (ColorManagedViewSettings *) ptr->data;

	const char *name = IMB_colormanagement_view_get_indexed_name(value);

	if (name) {
		BLI_strncpy(view->view_transform, name, sizeof(view->view_transform));
	}
}

static EnumPropertyItem* rna_ColorManagedViewSettings_view_transform_itemf(bContext *C, PointerRNA *ptr, PropertyRNA *UNUSED(prop), int *free)
{
	wmWindow *win = CTX_wm_window(C);
	EnumPropertyItem *items = NULL;
	ColorManagedDisplaySettings *display_settings = &win->display_settings;
	int totitem = 0;

	RNA_enum_item_add(&items, &totitem, &view_transform_items[0]);
	IMB_colormanagement_view_items_add(&items, &totitem, display_settings->display_device);
	RNA_enum_item_end(&items, &totitem);

	*free = TRUE;
	return items;
}

static int rna_ColorManagedColorspaceSettings_colorspace_get(struct PointerRNA *ptr)
{
	ColorManagedColorspaceSettings *colorspace = (ColorManagedColorspaceSettings *) ptr->data;

	return IMB_colormanagement_colorspace_get_named_index(colorspace->name);
}

static void rna_ColorManagedColorspaceSettings_colorspace_set(struct PointerRNA *ptr, int value)
{
	ColorManagedColorspaceSettings *colorspace = (ColorManagedColorspaceSettings *) ptr->data;
	const char *name = IMB_colormanagement_colorspace_get_indexed_name(value);

	if (name) {
		BLI_strncpy(colorspace->name, name, sizeof(colorspace->name));
	}
}

static EnumPropertyItem *rna_ColorManagedColorspaceSettings_colorspace_itemf(bContext *C, PointerRNA *ptr, PropertyRNA *UNUSED(prop), int *free)
{
	EnumPropertyItem *items = NULL;
	int totitem = 0;

	RNA_enum_item_add(&items, &totitem, &color_space_items[0]);
	IMB_colormanagement_colorspace_items_add(&items, &totitem);
	RNA_enum_item_end(&items, &totitem);

	*free = TRUE;

	return items;
}

static void rna_ColorManagedColorspaceSettings_reload_update(Main *UNUSED(bmain), Scene *UNUSED(scene), PointerRNA *ptr)
{
	ID *id = ptr->id.data;

	if (GS(id->name) == ID_IM) {
		Image *ima = (Image *) id;

		BKE_image_signal(ima, NULL, IMA_SIGNAL_RELOAD);
		WM_main_add_notifier(NC_IMAGE | ND_DISPLAY, &ima->id);
	}
	else if (GS(id->name) == ID_MC) {
		MovieClip *clip = (MovieClip *) id;

		BKE_movieclip_reload(clip);
		WM_main_add_notifier(NC_MOVIECLIP | ND_DISPLAY, &clip->id);
	}
}

/* this function only exists because #curvemap_evaluateF uses a 'const' qualifier */
float rna_CurveMap_evaluateF(struct CurveMap *cuma, float value)
{
	return curvemap_evaluateF(cuma, value);
}

#else

static void rna_def_curvemappoint(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	static EnumPropertyItem prop_handle_type_items[] = {
		{0, "AUTO", 0, "Auto Handle", ""},
		{CUMA_VECTOR, "VECTOR", 0, "Vector Handle", ""},
		{0, NULL, 0, NULL, NULL}
	};

	srna = RNA_def_struct(brna, "CurveMapPoint", NULL);
	RNA_def_struct_ui_text(srna, "CurveMapPoint", "Point of a curve used for a curve mapping");

	prop = RNA_def_property(srna, "location", PROP_FLOAT, PROP_XYZ);
	RNA_def_property_float_sdna(prop, NULL, "x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Location", "X/Y coordinates of the curve point");

	prop = RNA_def_property(srna, "handle_type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
	RNA_def_property_enum_items(prop, prop_handle_type_items);
	RNA_def_property_ui_text(prop, "Handle Type", "Curve interpolation at this point: Bezier or vector");

	prop = RNA_def_property(srna, "select", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", CUMA_SELECT);
	RNA_def_property_ui_text(prop, "Select", "Selection state of the curve point");
}

static void rna_def_curvemap_points_api(BlenderRNA *brna, PropertyRNA *cprop)
{
	StructRNA *srna;
	PropertyRNA *parm;
	FunctionRNA *func;

	RNA_def_property_srna(cprop, "CurveMapPoints");
	srna = RNA_def_struct(brna, "CurveMapPoints", NULL);
	RNA_def_struct_sdna(srna, "CurveMap");
	RNA_def_struct_ui_text(srna, "Curve Map Point", "Collection of Curve Map Points");

	func = RNA_def_function(srna, "new", "curvemap_insert");
	RNA_def_function_ui_description(func, "Add point to CurveMap");
	parm = RNA_def_float(func, "position", 0.0f, -FLT_MAX, FLT_MAX, "Position", "Position to add point", -FLT_MAX, FLT_MAX);
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm = RNA_def_float(func, "value", 0.0f, -FLT_MAX, FLT_MAX, "Value", "Value of point", -FLT_MAX, FLT_MAX);
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm = RNA_def_pointer(func, "point", "CurveMapPoint", "", "New point");
	RNA_def_function_return(func, parm);

	func = RNA_def_function(srna, "remove", "curvemap_remove_point");
	RNA_def_function_ui_description(func, "Delete point from CurveMap");
	parm = RNA_def_pointer(func, "point", "CurveMapPoint", "", "PointElement to remove");
	RNA_def_property_flag(parm, PROP_REQUIRED | PROP_NEVER_NULL);
}

static void rna_def_curvemap(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop, *parm;
	FunctionRNA *func;

	static EnumPropertyItem prop_extend_items[] = {
		{0, "HORIZONTAL", 0, "Horizontal", ""},
		{CUMA_EXTEND_EXTRAPOLATE, "EXTRAPOLATED", 0, "Extrapolated", ""},
		{0, NULL, 0, NULL, NULL}
	};

	srna = RNA_def_struct(brna, "CurveMap", NULL);
	RNA_def_struct_ui_text(srna, "CurveMap", "Curve in a curve mapping");

	prop = RNA_def_property(srna, "extend", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
	RNA_def_property_enum_items(prop, prop_extend_items);
	RNA_def_property_ui_text(prop, "Extend", "Extrapolate the curve or extend it horizontally");

	prop = RNA_def_property(srna, "points", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "curve", "totpoint");
	RNA_def_property_struct_type(prop, "CurveMapPoint");
	RNA_def_property_ui_text(prop, "Points", "");
	rna_def_curvemap_points_api(brna, prop);

	func = RNA_def_function(srna, "evaluate", "rna_CurveMap_evaluateF");
	RNA_def_function_ui_description(func, "Evaluate curve at given location");
	parm = RNA_def_float(func, "position", 0.0f, -FLT_MAX, FLT_MAX, "Position", "Position to evaluate curve at", -FLT_MAX, FLT_MAX);
	RNA_def_property_flag(parm, PROP_REQUIRED);
	parm = RNA_def_float(func, "value", 0.0f, -FLT_MAX, FLT_MAX, "Value", "Value of curve at given location", -FLT_MAX, FLT_MAX);
	RNA_def_function_return(func, parm);
}

static void rna_def_curvemapping(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	FunctionRNA *func;

	srna = RNA_def_struct(brna, "CurveMapping", NULL);
	RNA_def_struct_ui_text(srna, "CurveMapping",
	                       "Curve mapping to map color, vector and scalar values to other values using "
	                       "a user defined curve");
	
	prop = RNA_def_property(srna, "use_clip", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", CUMA_DO_CLIP);
	RNA_def_property_ui_text(prop, "Clip", "Force the curve view to fit a defined boundary");
	RNA_def_property_boolean_funcs(prop, NULL, "rna_CurveMapping_clip_set");

	prop = RNA_def_property(srna, "clip_min_x", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "clipr.xmin");
	RNA_def_property_range(prop, -100.0f, 100.0f);
	RNA_def_property_ui_text(prop, "Clip Min X", "");
	RNA_def_property_float_funcs(prop, NULL, NULL, "rna_CurveMapping_clipminx_range");

	prop = RNA_def_property(srna, "clip_min_y", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "clipr.ymin");
	RNA_def_property_range(prop, -100.0f, 100.0f);
	RNA_def_property_ui_text(prop, "Clip Min Y", "");
	RNA_def_property_float_funcs(prop, NULL, NULL, "rna_CurveMapping_clipminy_range");

	prop = RNA_def_property(srna, "clip_max_x", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "clipr.xmax");
	RNA_def_property_range(prop, -100.0f, 100.0f);
	RNA_def_property_ui_text(prop, "Clip Max X", "");
	RNA_def_property_float_funcs(prop, NULL, NULL, "rna_CurveMapping_clipmaxx_range");

	prop = RNA_def_property(srna, "clip_max_y", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "clipr.ymax");
	RNA_def_property_range(prop, -100.0f, 100.0f);
	RNA_def_property_ui_text(prop, "Clip Max Y", "");
	RNA_def_property_float_funcs(prop, NULL, NULL, "rna_CurveMapping_clipmaxy_range");

	prop = RNA_def_property(srna, "curves", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_funcs(prop, "rna_CurveMapping_curves_begin", "rna_iterator_array_next",
	                                  "rna_iterator_array_end", "rna_iterator_array_get",
	                                  "rna_CurveMapping_curves_length", NULL, NULL, NULL);
	RNA_def_property_struct_type(prop, "CurveMap");
	RNA_def_property_ui_text(prop, "Curves", "");

	prop = RNA_def_property(srna, "black_level", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "black");
	RNA_def_property_range(prop, -1000.0f, 1000.0f);
	RNA_def_property_ui_text(prop, "Black Level", "For RGB curves, the color that black is mapped to");
	RNA_def_property_float_funcs(prop, NULL, "rna_CurveMapping_black_level_set", NULL);

	prop = RNA_def_property(srna, "white_level", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "white");
	RNA_def_property_range(prop, -1000.0f, 1000.0f);
	RNA_def_property_ui_text(prop, "White Level", "For RGB curves, the color that white is mapped to");
	RNA_def_property_float_funcs(prop, NULL, "rna_CurveMapping_white_level_set", NULL);

	func = RNA_def_function(srna, "update", "curvemapping_changed_all");
	RNA_def_function_ui_description(func, "Update curve mapping after making changes");
}

static void rna_def_color_ramp_element(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna = RNA_def_struct(brna, "ColorRampElement", NULL);
	RNA_def_struct_sdna(srna, "CBData");
	RNA_def_struct_path_func(srna, "rna_ColorRampElement_path");
	RNA_def_struct_ui_text(srna, "Color Ramp Element", "Element defining a color at a position in the color ramp");
	
	prop = RNA_def_property(srna, "color", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "r");
	RNA_def_property_array(prop, 4);
	RNA_def_property_ui_text(prop, "Color", "Set color of selected color stop");
	RNA_def_property_update(prop, 0, "rna_ColorRamp_update");
	
	prop = RNA_def_property(srna, "position", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "pos");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Position", "Set position of selected color stop");
	RNA_def_property_update(prop, 0, "rna_ColorRamp_update");
}

static void rna_def_color_ramp_element_api(BlenderRNA *brna, PropertyRNA *cprop)
{
	StructRNA *srna;
	PropertyRNA *parm;
	FunctionRNA *func;

	RNA_def_property_srna(cprop, "ColorRampElements");
	srna = RNA_def_struct(brna, "ColorRampElements", NULL);
	RNA_def_struct_sdna(srna, "ColorBand");
	RNA_def_struct_path_func(srna, "rna_ColorRampElement_path");
	RNA_def_struct_ui_text(srna, "Color Ramp Elements", "Collection of Color Ramp Elements");

	/* TODO, make these functions generic in texture.c */
	func = RNA_def_function(srna, "new", "rna_ColorRampElement_new");
	RNA_def_function_ui_description(func, "Add element to ColorRamp");
	RNA_def_function_flag(func, FUNC_USE_REPORTS);
	parm = RNA_def_float(func, "position", 0.0f, 0.0f, 1.0f, "Position", "Position to add element", 0.0f, 1.0f);
	RNA_def_property_flag(parm, PROP_REQUIRED);
	/* return type */
	parm = RNA_def_pointer(func, "element", "ColorRampElement", "", "New element");
	RNA_def_function_return(func, parm);

	func = RNA_def_function(srna, "remove", "rna_ColorRampElement_remove");
	RNA_def_function_ui_description(func, "Delete element from ColorRamp");
	RNA_def_function_flag(func, FUNC_USE_REPORTS);
	parm = RNA_def_pointer(func, "element", "ColorRampElement", "", "Element to remove");
	RNA_def_property_flag(parm, PROP_REQUIRED | PROP_NEVER_NULL);
}

static void rna_def_color_ramp(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	FunctionRNA *func;

	static EnumPropertyItem prop_interpolation_items[] = {
		{1, "EASE", 0, "Ease", ""},
		{3, "CARDINAL", 0, "Cardinal", ""},
		{0, "LINEAR", 0, "Linear", ""},
		{2, "B_SPLINE", 0, "B-Spline", ""},
		{4, "CONSTANT", 0, "Constant", ""},
		{0, NULL, 0, NULL, NULL}
	};
	
	srna = RNA_def_struct(brna, "ColorRamp", NULL);
	RNA_def_struct_sdna(srna, "ColorBand");
	RNA_def_struct_path_func(srna, "rna_ColorRamp_path");
	RNA_def_struct_ui_text(srna, "Color Ramp", "Color ramp mapping a scalar value to a color");
	
	prop = RNA_def_property(srna, "elements", PROP_COLLECTION, PROP_COLOR);
	RNA_def_property_collection_sdna(prop, NULL, "data", "tot");
	RNA_def_property_struct_type(prop, "ColorRampElement");
	RNA_def_property_ui_text(prop, "Elements", "");
	RNA_def_property_update(prop, 0, "rna_ColorRamp_update");
	rna_def_color_ramp_element_api(brna, prop);

	prop = RNA_def_property(srna, "interpolation", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "ipotype");
	RNA_def_property_enum_items(prop, prop_interpolation_items);
	RNA_def_property_ui_text(prop, "Interpolation", "");
	RNA_def_property_update(prop, 0, "rna_ColorRamp_update");

#if 0 /* use len(elements) */
	prop = RNA_def_property(srna, "total", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "tot");
	/* needs a function to do the right thing when adding elements like colorband_add_cb() */
	RNA_def_property_clear_flag(prop, PROP_EDITABLE);
	RNA_def_property_range(prop, 0, 31); /* MAXCOLORBAND = 32 */
	RNA_def_property_ui_text(prop, "Total", "Total number of elements");
	RNA_def_property_update(prop, 0, "rna_ColorRamp_update");
#endif
	
	func = RNA_def_function(srna, "evaluate", "rna_ColorRamp_eval");
	RNA_def_function_ui_description(func, "Evaluate ColorRamp");
	prop = RNA_def_float(func, "position", 1.0f, 0.0f, 1.0f, "Position", "Evaluate ColorRamp at position", 0.0f, 1.0f);
	RNA_def_property_flag(prop, PROP_REQUIRED);
	/* return */
	prop = RNA_def_float_color(func, "color", 4, NULL, -FLT_MAX, FLT_MAX, "Color", "Color at given position",
	                           -FLT_MAX, FLT_MAX);
	RNA_def_property_flag(prop, PROP_THICK_WRAP);
	RNA_def_function_output(func, prop);
}

static void rna_def_histogram(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_mode_items[] = {
		{HISTO_MODE_LUMA, "LUMA", 0, "Luma", "Luma"},
		{HISTO_MODE_RGB, "RGB", 0, "RGB", "Red Green Blue"},
		{HISTO_MODE_R, "R", 0, "R", "Red"},
		{HISTO_MODE_G, "G", 0, "G", "Green"},
		{HISTO_MODE_B, "B", 0, "B", "Blue"},
		{HISTO_MODE_ALPHA, "A", 0, "A", "Alpha"},
		{0, NULL, 0, NULL, NULL}
	};

	srna = RNA_def_struct(brna, "Histogram", NULL);
	RNA_def_struct_ui_text(srna, "Histogram", "Statistical view of the levels of color in an image");
	
	prop = RNA_def_property(srna, "mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "mode");
	RNA_def_property_enum_items(prop, prop_mode_items);
	RNA_def_property_ui_text(prop, "Mode", "Channels to display when drawing the histogram");

	prop = RNA_def_property(srna, "show_line", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", HISTO_FLAG_LINE);
	RNA_def_property_ui_text(prop, "Show Line", "Display lines rather then filled shapes");
	RNA_def_property_ui_icon(prop, ICON_IPO, 0);
}

static void rna_def_scopes(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	static EnumPropertyItem prop_wavefrm_mode_items[] = {
		{SCOPES_WAVEFRM_LUMA, "LUMA", ICON_COLOR, "Luma", ""},
		{SCOPES_WAVEFRM_RGB, "RGB", ICON_COLOR, "Red Green Blue", ""},
		{SCOPES_WAVEFRM_YCC_601, "YCBCR601", ICON_COLOR, "YCbCr (ITU 601)", ""},
		{SCOPES_WAVEFRM_YCC_709, "YCBCR709", ICON_COLOR, "YCbCr (ITU 709)", ""},
		{SCOPES_WAVEFRM_YCC_JPEG, "YCBCRJPG", ICON_COLOR, "YCbCr (Jpeg)", ""},
		{0, NULL, 0, NULL, NULL}
	};

	srna = RNA_def_struct(brna, "Scopes", NULL);
	RNA_def_struct_ui_text(srna, "Scopes", "Scopes for statistical view of an image");
	
	prop = RNA_def_property(srna, "use_full_resolution", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, "Scopes", "sample_full", 1);
	RNA_def_property_ui_text(prop, "Full Sample", "Sample every pixel of the image");
	RNA_def_property_update(prop, 0, "rna_Scopes_update");
	
	prop = RNA_def_property(srna, "accuracy", PROP_FLOAT, PROP_PERCENTAGE);
	RNA_def_property_float_sdna(prop, "Scopes", "accuracy");
	RNA_def_property_range(prop, 0.0, 100.0);
	RNA_def_property_ui_range(prop, 0.0, 100.0, 10, 1);
	RNA_def_property_ui_text(prop, "Accuracy", "Proportion of original image source pixel lines to sample");
	RNA_def_property_update(prop, 0, "rna_Scopes_update");

	prop = RNA_def_property(srna, "histogram", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, "Scopes", "hist");
	RNA_def_property_struct_type(prop, "Histogram");
	RNA_def_property_ui_text(prop, "Histogram", "Histogram for viewing image statistics");

	prop = RNA_def_property(srna, "waveform_mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, "Scopes", "wavefrm_mode");
	RNA_def_property_enum_items(prop, prop_wavefrm_mode_items);
	RNA_def_property_ui_text(prop, "Waveform Mode", "");
	RNA_def_property_update(prop, 0, "rna_Scopes_update");

	prop = RNA_def_property(srna, "waveform_alpha", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, "Scopes", "wavefrm_alpha");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Waveform Opacity", "Opacity of the points");

	prop = RNA_def_property(srna, "vectorscope_alpha", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, "Scopes", "vecscope_alpha");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Vectorscope Opacity", "Opacity of the points");
}

static void rna_def_colormanage(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	static EnumPropertyItem display_device_items[] = {
		{0, "DEFAULT", 0, "Default", ""},
		{0, NULL, 0, NULL, NULL}
	};

	/* ** Display Settings  **  */
	srna = RNA_def_struct(brna, "ColorManagedDisplaySettings", NULL);
	RNA_def_struct_ui_text(srna, "ColorManagedDisplaySettings", "Color management specific to display device");

	prop= RNA_def_property(srna, "display_device", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, display_device_items);
	RNA_def_property_enum_funcs(prop, "rna_ColorManagedDisplaySettings_display_device_get",
	                                  "rna_ColorManagedDisplaySettings_display_device_set",
	                                  "rna_ColorManagedDisplaySettings_display_device_itemf");
	RNA_def_property_ui_text(prop, "Display Device", "Display device name");
	RNA_def_property_update(prop, NC_WINDOW, NULL);

	/* ** View Settings  **  */
	srna = RNA_def_struct(brna, "ColorManagedViewSettings", NULL);
	RNA_def_struct_ui_text(srna, "ColorManagedViewSettings", "Color management settings used for displaying images on the display");

	prop = RNA_def_property(srna, "use_global_settings", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", COLORMANAGE_VIEW_USE_GLOBAL);
	RNA_def_property_ui_text(prop, "Use Global Settings", "Use global display settings instead of per-space setting");
	RNA_def_property_update(prop, NC_WINDOW, NULL);

	prop= RNA_def_property(srna, "view_transform", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, view_transform_items);
	RNA_def_property_enum_funcs(prop, "rna_ColorManagedViewSettings_view_transform_get",
	                                  "rna_ColorManagedViewSettings_view_transform_set",
	                                  "rna_ColorManagedViewSettings_view_transform_itemf");
	RNA_def_property_ui_text(prop, "View Transform", "View transform used for this image editor");
	RNA_def_property_update(prop, NC_WINDOW, NULL);

	prop = RNA_def_property(srna, "exposure", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, NULL, "exposure");
	RNA_def_property_range(prop, -10.0f, 10.0f);
	RNA_def_property_float_default(prop, 0.0f);
	RNA_def_property_ui_text(prop, "Exposure", "Exposure (stops) applied on displaying image buffers");
	RNA_def_property_update(prop, NC_WINDOW, NULL);

	prop = RNA_def_property(srna, "gamma", PROP_FLOAT, PROP_FACTOR);
	RNA_def_property_float_sdna(prop, NULL, "gamma");
	RNA_def_property_float_default(prop, 1.0f);
	RNA_def_property_range(prop, 0.0f, 5.0f);
	RNA_def_property_ui_text(prop, "Gamma", "Amount f gamma modification for displaying image buffers");
	RNA_def_property_update(prop, NC_WINDOW, NULL);

	/* ** Colorspace **  */
	srna = RNA_def_struct(brna, "ColorManagedColorspaceSettings", NULL);
	RNA_def_struct_ui_text(srna, "ColorManagedColorspaceSettings", "Input color space settings");

	prop= RNA_def_property(srna, "name", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, color_space_items);
	RNA_def_property_enum_funcs(prop, "rna_ColorManagedColorspaceSettings_colorspace_get",
	                                  "rna_ColorManagedColorspaceSettings_colorspace_set",
	                                  "rna_ColorManagedColorspaceSettings_colorspace_itemf");
	RNA_def_property_ui_text(prop, "Color Space", "Input color space name");
	RNA_def_property_update(prop, NC_WINDOW, "rna_ColorManagedColorspaceSettings_reload_update");
}

void RNA_def_color(BlenderRNA *brna)
{
	rna_def_curvemappoint(brna);
	rna_def_curvemap(brna);
	rna_def_curvemapping(brna);
	rna_def_color_ramp_element(brna);
	rna_def_color_ramp(brna);
	rna_def_histogram(brna);
	rna_def_scopes(brna);
	rna_def_colormanage(brna);
}

#endif
