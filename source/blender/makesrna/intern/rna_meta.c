/**
 * $Id$
 *
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
 * Contributor(s): Blender Foundation (2008), Juho Vepsalainen, Jiri Hnidek
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>

#include "RNA_define.h"

#include "rna_internal.h"

#include "DNA_mesh_types.h"
#include "DNA_meta_types.h"

#ifdef RNA_RUNTIME

#include "BLI_math.h"

#include "DNA_scene_types.h"
#include "DNA_object_types.h"

#include "BKE_mball.h"
#include "BKE_depsgraph.h"
#include "BKE_main.h"

#include "WM_types.h"
#include "WM_api.h"

static int rna_Meta_texspace_editable(PointerRNA *ptr)
{
	MetaBall *mb= (MetaBall*)ptr->data;
	return (mb->texflag & MB_AUTOSPACE)? 0: PROP_EDITABLE;
}

static void rna_Meta_texspace_loc_get(PointerRNA *ptr, float *values)
{
	MetaBall *mb= (MetaBall*)ptr->data;
	
	/* tex_space_mball() needs object.. ugh */
	
	copy_v3_v3(values, mb->loc);
}

static void rna_Meta_texspace_loc_set(PointerRNA *ptr, const float *values)
{
	MetaBall *mb= (MetaBall*)ptr->data;
	
	copy_v3_v3(mb->loc, values);
}

static void rna_Meta_texspace_size_get(PointerRNA *ptr, float *values)
{
	MetaBall *mb= (MetaBall*)ptr->data;
	
	/* tex_space_mball() needs object.. ugh */
	
	copy_v3_v3(values, mb->size);
}

static void rna_Meta_texspace_size_set(PointerRNA *ptr, const float *values)
{
	MetaBall *mb= (MetaBall*)ptr->data;
	
	copy_v3_v3(mb->size, values);
}


static void rna_MetaBall_update_data(Main *bmain, Scene *scene, PointerRNA *ptr)
{
	MetaBall *mb= ptr->id.data;
	Object *ob;

	for(ob=bmain->object.first; ob; ob= ob->id.next)
		if(ob->data == mb)
			copy_mball_properties(scene, ob);

	DAG_id_flush_update(&mb->id, OB_RECALC_DATA);
	WM_main_add_notifier(NC_GEOM|ND_DATA, mb);
}

#else

static void rna_def_metaelement(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	static EnumPropertyItem prop_type_items[] = {
		{MB_BALL, "BALL", ICON_META_BALL, "Ball", ""},
		{MB_TUBE, "TUBE", ICON_META_TUBE, "Tube", ""},
		{MB_PLANE, "PLANE", ICON_META_PLANE, "Plane", ""},
		{MB_ELIPSOID, "ELLIPSOID", ICON_META_ELLIPSOID, "Ellipsoid", ""}, // NOTE: typo at original definition!
		{MB_CUBE, "CUBE", ICON_META_CUBE, "Cube", ""},
		{0, NULL, 0, NULL, NULL}};
	
	srna= RNA_def_struct(brna, "MetaElement", NULL);
	RNA_def_struct_sdna(srna, "MetaElem");
	RNA_def_struct_ui_text(srna, "Meta Element", "Blobby element in a MetaBall datablock");
	RNA_def_struct_ui_icon(srna, ICON_OUTLINER_DATA_META);
	
	/* enums */
	prop= RNA_def_property(srna, "type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "Type", "Metaball types");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	/* number values */
	prop= RNA_def_property(srna, "location", PROP_FLOAT, PROP_TRANSLATION);
	RNA_def_property_float_sdna(prop, NULL, "x");
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Location", "");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");

	prop= RNA_def_property(srna, "rotation", PROP_FLOAT, PROP_QUATERNION);
	RNA_def_property_float_sdna(prop, NULL, "quat");
	RNA_def_property_ui_text(prop, "Rotation", "");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");

	prop= RNA_def_property(srna, "radius", PROP_FLOAT, PROP_UNSIGNED|PROP_UNIT_LENGTH);
	RNA_def_property_float_sdna(prop, NULL, "rad");
	RNA_def_property_ui_text(prop, "Radius", "");
	RNA_def_property_range(prop, 0.0f, FLT_MAX);
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");

	prop= RNA_def_property(srna, "size_x", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "expx");
	RNA_def_property_range(prop, 0.0f, 20.0f);
	RNA_def_property_ui_text(prop, "Size X", "Size of element, use of components depends on element type");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");

	prop= RNA_def_property(srna, "size_y", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "expy");
	RNA_def_property_range(prop, 0.0f, 20.0f);
	RNA_def_property_ui_text(prop, "Size Y", "Size of element, use of components depends on element type");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");

	prop= RNA_def_property(srna, "size_z", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "expz");
	RNA_def_property_range(prop, 0.0f, 20.0f);
	RNA_def_property_ui_text(prop, "Size Z", "Size of element, use of components depends on element type");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	prop= RNA_def_property(srna, "stiffness", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "s");
	RNA_def_property_range(prop, 0.0f, 10.0f);
	RNA_def_property_ui_text(prop, "Stiffness", "Stiffness defines how much of the element to fill");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	/* flags */
	prop= RNA_def_property(srna, "negative", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", MB_NEGATIVE);
	RNA_def_property_ui_text(prop, "Negative", "Set metaball as negative one");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	prop= RNA_def_property(srna, "hide", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", MB_HIDE);
	RNA_def_property_ui_text(prop, "Hide", "Hide element");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
}

static void rna_def_metaball(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	static EnumPropertyItem prop_update_items[] = {
		{MB_UPDATE_ALWAYS, "UPDATE_ALWAYS", 0, "Always", "While editing, update metaball always"},
		{MB_UPDATE_HALFRES, "HALFRES", 0, "Half", "While editing, update metaball in half resolution"},
		{MB_UPDATE_FAST, "FAST", 0, "Fast", "While editing, update metaball without polygonization"},
		{MB_UPDATE_NEVER, "NEVER", 0, "Never", "While editing, don't update metaball at all"},
		{0, NULL, 0, NULL, NULL}};
	
	srna= RNA_def_struct(brna, "MetaBall", "ID");
	RNA_def_struct_ui_text(srna, "MetaBall", "Metaball datablock to defined blobby surfaces");
	RNA_def_struct_ui_icon(srna, ICON_META_DATA);

	prop= RNA_def_property(srna, "elements", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "elems", NULL);
	RNA_def_property_struct_type(prop, "MetaElement");
	RNA_def_property_ui_text(prop, "Elements", "Meta elements");

	prop= RNA_def_property(srna, "active_element", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "lastelem");
	RNA_def_property_ui_text(prop, "Last selected element.", "Last selected element");
	
	/* enums */
	prop= RNA_def_property(srna, "flag", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_update_items);
	RNA_def_property_ui_text(prop, "Update", "Metaball edit update behavior");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	/* number values */
	prop= RNA_def_property(srna, "wire_size", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "wiresize");
	RNA_def_property_range(prop, 0.050f, 1.0f);
	RNA_def_property_ui_text(prop, "Wire Size", "Polygonization resolution in the 3D viewport");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	prop= RNA_def_property(srna, "render_size", PROP_FLOAT, PROP_DISTANCE);
	RNA_def_property_float_sdna(prop, NULL, "rendersize");
	RNA_def_property_range(prop, 0.050f, 1.0f);
	RNA_def_property_ui_text(prop, "Render Size", "Polygonization resolution in rendering");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	prop= RNA_def_property(srna, "threshold", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "thresh");
	RNA_def_property_range(prop, 0.0f, 5.0f);
	RNA_def_property_ui_text(prop, "Threshold", "Influence of meta elements");
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");

	/* texture space */
	prop= RNA_def_property(srna, "auto_texspace", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "texflag", MB_AUTOSPACE);
	RNA_def_property_ui_text(prop, "Auto Texture Space", "Adjusts active object's texture space automatically when transforming object");
	
	prop= RNA_def_property(srna, "texspace_loc", PROP_FLOAT, PROP_TRANSLATION);
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Texture Space Location", "Texture space location");
	RNA_def_property_editable_func(prop, "rna_Meta_texspace_editable");
	RNA_def_property_float_funcs(prop, "rna_Meta_texspace_loc_get", "rna_Meta_texspace_loc_set", NULL);	
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	prop= RNA_def_property(srna, "texspace_size", PROP_FLOAT, PROP_XYZ);
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Texture Space Size", "Texture space size");
	RNA_def_property_editable_func(prop, "rna_Meta_texspace_editable");
	RNA_def_property_float_funcs(prop, "rna_Meta_texspace_size_get", "rna_Meta_texspace_size_set", NULL);
	RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");
	
	/* not supported yet
	 prop= RNA_def_property(srna, "texspace_rot", PROP_FLOAT, PROP_EULER);
	 RNA_def_property_float(prop, NULL, "rot");
	 RNA_def_property_ui_text(prop, "Texture Space Rotation", "Texture space rotation");
	 RNA_def_property_editable_func(prop, "rna_Meta_texspace_editable");
	 RNA_def_property_update(prop, 0, "rna_MetaBall_update_data");*/
	
	/* materials */
	prop= RNA_def_property(srna, "materials", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "mat", "totcol");
	RNA_def_property_struct_type(prop, "Material");
	RNA_def_property_ui_text(prop, "Materials", "");
	
	/* anim */
	rna_def_animdata_common(srna);
}

void RNA_def_meta(BlenderRNA *brna)
{
	rna_def_metaelement(brna);
	rna_def_metaball(brna);
}

#endif
