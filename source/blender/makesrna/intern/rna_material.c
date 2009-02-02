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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Contributor(s): Blender Foundation (2008), Nathan Letwory
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <float.h>
#include <stdlib.h>

#include "RNA_define.h"
#include "RNA_types.h"

#include "rna_internal.h"

#include "DNA_material_types.h"
#include "DNA_texture_types.h"

#include "WM_types.h"

#ifdef RNA_RUNTIME

static void *rna_Material_self_get(PointerRNA *ptr)
{
	return ptr->id.data;
}

static void rna_Material_mode_halo_set(PointerRNA *ptr, int value)
{
	Material *ma= (Material*)ptr->data;
	
	if(value)
		ma->mode |= MA_HALO;
	else
		ma->mode &= ~(MA_HALO|MA_STAR|MA_HALO_XALPHA|MA_ZINV|MA_ENV);
}

static void rna_Material_mtex_begin(CollectionPropertyIterator *iter, PointerRNA *ptr)
{
	Material *ma= (Material*)ptr->data;
	rna_iterator_array_begin(iter, (void*)ma->mtex, sizeof(MTex*), MAX_MTEX, NULL);
}

static void *rna_Material_active_texture_get(PointerRNA *ptr)
{
	Material *ma= (Material*)ptr->data;
	return ma->mtex[(int)ma->texact];
}

static void rna_MaterialStrand_start_size_range(PointerRNA *ptr, float *min, float *max)
{
	Material *ma= (Material*)ptr->id.data;

	if(ma->mode & MA_STR_B_UNITS) {
		*min= 0.0001f;
		*max= 2.0f;
	}
	else {
		*min= 0.25f;
		*max= 20.0f;
	}
}

static void rna_MaterialStrand_end_size_range(PointerRNA *ptr, float *min, float *max)
{
	Material *ma= (Material*)ptr->id.data;

	if(ma->mode & MA_STR_B_UNITS) {
		*min= 0.0001f;
		*max= 1.0f;
	}
	else {
		*min= 0.25f;
		*max= 10.0f;
	}
}

#else

static void rna_def_material_mtex(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	static EnumPropertyItem prop_texture_coordinates_items[] = {
		{TEXCO_GLOB, "GLOBAL", "Global", "Uses global coordinates for the texture coordinates."},
		{TEXCO_OBJECT, "OBJECT", "Object", "Uses linked object's coordinates for texture coordinates."},
		{TEXCO_UV, "UV", "UV", "Uses UV coordinates for texture coordinates."},
		{TEXCO_ORCO, "ORCO", "Orco", "Uses the original undeformed coordinates of the object."},
		{TEXCO_STRAND, "STRAND", "Strand", "Uses normalized strand texture coordinate (1D)."},
		{TEXCO_STICKY, "STICKY", "Sticky", "Uses mesh's sticky coordinates for the texture coordinates."},
		{TEXCO_WINDOW, "WINDOW", "Window", "Uses screen coordinates as texture coordinates."},
		{TEXCO_NORM, "NORMAL", "Normal", "Uses normal vector as texture coordinates."},
		{TEXCO_REFL, "REFLECTION", "Reflection", "Uses reflection vector as texture coordinates."},
		{TEXCO_STRESS, "STRESS", "Stress", "Uses the difference of edge lengths compared to original coordinates of the mesh."},
		{TEXCO_SPEED, "TANGENT", "Tangent", "Uses the optional tangent vector as texture coordinates."},

		{0, NULL, NULL, NULL}};

	static EnumPropertyItem prop_mapping_items[] = {
		{MTEX_FLAT, "FLAT", "Flat", "Maps X and Y coordinates directly."},
		{MTEX_CUBE, "CUBE", "Cube", "Maps using the normal vector."},
		{MTEX_TUBE, "TUBE", "Tube", "Maps with Z as central axis."},
		{MTEX_SPHERE, "SPHERE", "Sphere", "Maps with Z as central axis."},
		{0, NULL, NULL, NULL}};

	static EnumPropertyItem prop_normal_map_space_items[] = {
		{MTEX_NSPACE_CAMERA, "CAMERA", "Camera", ""},
		{MTEX_NSPACE_WORLD, "WORLD", "World", ""},
		{MTEX_NSPACE_OBJECT, "OBJECT", "Object", ""},
		{MTEX_NSPACE_TANGENT, "TANGENT", "Tangent", ""},
		{0, NULL, NULL, NULL}};

	srna= RNA_def_struct(brna, "MaterialTextureSlot", "TextureSlot");
	RNA_def_struct_sdna(srna, "MTex");
	RNA_def_struct_ui_text(srna, "Material Texture Slot", "Texture slot for textures in a Material datablock.");

	prop= RNA_def_property(srna, "texture_coordinates", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "texco");
	RNA_def_property_enum_items(prop, prop_texture_coordinates_items);
	RNA_def_property_ui_text(prop, "Texture Coordinates", "");

	prop= RNA_def_property(srna, "object", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "object");
	RNA_def_property_struct_type(prop, "Object");
	RNA_def_property_ui_text(prop, "Object", "Object to use for mapping with Object texture coordinates.");

	prop= RNA_def_property(srna, "uv_layer", PROP_STRING, PROP_NONE);
	RNA_def_property_string_sdna(prop, NULL, "uvname");
	RNA_def_property_ui_text(prop, "UV Layer", "UV layer to use for mapping with UV texture coordinates.");

	prop= RNA_def_property(srna, "from_dupli", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "texflag", MTEX_DUPLI_MAPTO);
	RNA_def_property_ui_text(prop, "From Dupli", "Dupli's instanced from verts, faces or particles, inherit texture coordinate from their parent (only for UV and Orco texture coordinates).");

	/* XXX: MTex.mapto and MTex.maptoneg */

	/* XXX: MTex.proj[xyz] */

	prop= RNA_def_property(srna, "mapping", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_mapping_items);
	RNA_def_property_ui_text(prop, "Mapping", "");

	/* XXX: MTex.colormodel, pmapto, pmaptoneg */

	prop= RNA_def_property(srna, "normal_map_space", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "normapspace");
	RNA_def_property_enum_items(prop, prop_normal_map_space_items);
	RNA_def_property_ui_text(prop, "Normal Map Space", "");

	/* XXX: MTex.which_output */

	/* XXX: MTex.k */

	prop= RNA_def_property(srna, "displacement_factor", PROP_FLOAT, PROP_VECTOR);
	RNA_def_property_float_sdna(prop, NULL, "dispfac");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Displacement Factor", "Amount texture displaces the surface.");

	prop= RNA_def_property(srna, "warp_factor", PROP_FLOAT, PROP_VECTOR);
	RNA_def_property_float_sdna(prop, NULL, "warpfac");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Warp Factor", "Amount texture affects color values.");
}

static void rna_def_material_colors(StructRNA *srna)
{
	PropertyRNA *prop;

	static EnumPropertyItem prop_type_items[] = {
		{MA_RGB, "RGB", "RGB", ""},
		// {MA_CMYK, "CMYK", "CMYK", ""}, 
		// {MA_YUV, "YUV", "YUV", ""},
		{MA_HSV, "HSV", "HSV", ""},
		{0, NULL, NULL, NULL}};
	
	prop= RNA_def_property(srna, "color_model", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "colormodel");
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "Color Model", "Color model to display color values with in the user interface.");
	
	prop= RNA_def_property(srna, "diffuse_color", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "r");
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Diffuse Color", "");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING_DRAW, NULL);
	
	prop= RNA_def_property(srna, "specular_color", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "specr");
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Specular Color", "Specular color of the material.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING_DRAW, NULL);
	
	prop= RNA_def_property(srna, "mirror_color", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "mirr");
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Mirror Color", "Mirror color of the material.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "alpha", PROP_FLOAT, PROP_NONE);
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Alpha", "Alpha transparency of the material.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING_DRAW, NULL);
	
	/* Color bands */
	prop= RNA_def_property(srna, "diffuse_ramp", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "ramp_col");
	RNA_def_property_struct_type(prop, "ColorRamp");
	RNA_def_property_ui_text(prop, "Diffuse Ramp", "Color ramp used to affect diffuse shading.");

	prop= RNA_def_property(srna, "specular_ramp", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "ramp_spec");
	RNA_def_property_struct_type(prop, "ColorRamp");
	RNA_def_property_ui_text(prop, "Specular Ramp", "Color ramp used to affect specular shading.");
}

static void rna_def_material_diffuse(StructRNA *srna)
{
	PropertyRNA *prop;

	static EnumPropertyItem prop_diff_shader_items[] = {
		{MA_DIFF_LAMBERT, "LAMBERT", "Lambert", ""},
		{MA_DIFF_ORENNAYAR, "OREN_NAYAR", "Oren-Nayar", ""},
		{MA_DIFF_TOON, "TOON", "Toon", ""},
		{MA_DIFF_MINNAERT, "MINNAERT", "Minnaert", ""},
		{MA_DIFF_FRESNEL, "FRESNEL", "Fresnel", ""},
		{0, NULL, NULL, NULL}};
	
	prop= RNA_def_property(srna, "diffuse_shader", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "diff_shader");
	RNA_def_property_enum_items(prop, prop_diff_shader_items);
	RNA_def_property_ui_text(prop, "Diffuse Shader Model", "");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "diffuse_reflection", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "ref");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Diffuse Reflection", "Amount of diffuse reflection.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING_DRAW, NULL);
	
	prop= RNA_def_property(srna, "roughness", PROP_FLOAT, PROP_NONE);
	RNA_def_property_range(prop, 0.0f, 3.14f);
	RNA_def_property_ui_text(prop, "Roughness", "Oren-Nayar Roughness");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "params1_4", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "param");
	RNA_def_property_array(prop, 4);
	RNA_def_property_range(prop, 0.0f, 5.0f);
	RNA_def_property_ui_text(prop, "Params 1-4", "Parameters used for diffuse and specular Toon, and diffuse Fresnel shaders. Check documentation for details.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "darkness", PROP_FLOAT, PROP_NONE);
	RNA_def_property_range(prop, 0.0f, 2.0f);
	RNA_def_property_ui_text(prop, "Darkness", "Minnaert darkness.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
}

static void rna_def_material_raymirror(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	static EnumPropertyItem prop_fadeto_mir_items[] = {
		{MA_RAYMIR_FADETOSKY, "FADE_TO_SKY", "Fade to Sky Color", ""},
		{MA_RAYMIR_FADETOMAT, "FADE_TO_MATERIAL", "Fade to Material Color", ""},
		{0, NULL, NULL, NULL}};

	srna= RNA_def_struct(brna, "MaterialRaytraceMirror", NULL);
	RNA_def_struct_sdna(srna, "Material");
	RNA_def_struct_nested(brna, srna, "Material");
	RNA_def_struct_ui_text(srna, "Material Raytrace Mirror", "Raytraced reflection settings for a Material datablock.");

	prop= RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_RAYMIRROR); /* use bitflags */
	RNA_def_property_ui_text(prop, "Enabled", "Enable raytraced reflections.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
		
	prop= RNA_def_property(srna, "reflect", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "ray_mirror");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Reflect", "Sets the amount mirror reflection for raytrace.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "fresnel", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "fresnel_mir");
	RNA_def_property_range(prop, 0.0f, 5.0f);
	RNA_def_property_ui_text(prop, "Fresnel", "Power of Fresnel for mirror reflection.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "fresnel_fac", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "fresnel_mir_i");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Fresnel Factor", "Blending factor for Fresnel.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "gloss", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "gloss_mir");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Gloss", "The shininess of the reflection. Values < 1.0 give diffuse, blurry reflections.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "gloss_anisotropic", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "aniso_gloss_mir");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Gloss Anisotropic", "The shape of the reflection, from 0.0 (circular) to 1.0 (fully stretched along the tangent.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
		
	prop= RNA_def_property(srna, "gloss_samples", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "samp_gloss_mir");
	RNA_def_property_range(prop, 0, 1024);
	RNA_def_property_ui_text(prop, "Gloss Samples", "Number of cone samples averaged for blurry reflections.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "gloss_threshold", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "adapt_thresh_mir");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Gloss Threshold", "Threshold for adaptive sampling. If a sample contributes less than this amount (as a percentage), sampling is stopped.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "depth", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "ray_depth");
	RNA_def_property_range(prop, 0, 10);
	RNA_def_property_ui_text(prop, "Depth", "Maximum allowed number of light inter-reflections.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "distance", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "dist_mir");
	RNA_def_property_range(prop, 0.0f, 10000.0f);
	RNA_def_property_ui_text(prop, "Max Dist", "Maximum distance of reflected rays. Reflections further than this range fade to sky color or material color.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "fade_to", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "fadeto_mir");
	RNA_def_property_enum_items(prop, prop_fadeto_mir_items);
	RNA_def_property_ui_text(prop, "End Fade-out", "The color that rays with no intersection within the Max Distance take. Material color can be best for indoor scenes, sky color for outdoor.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
}

static void rna_def_material_raytra(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna= RNA_def_struct(brna, "MaterialRaytraceTransparency", NULL);
	RNA_def_struct_sdna(srna, "Material");
	RNA_def_struct_nested(brna, srna, "Material");
	RNA_def_struct_ui_text(srna, "Material Raytrace Transparency", "Raytraced refraction settings for a Material datablock.");

	prop= RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_RAYTRANSP); /* use bitflags */
	RNA_def_property_ui_text(prop, "Enabled", "Enables raytracing for transparent refraction rendering.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "ior", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "ang");
	RNA_def_property_range(prop, 1.0f, 3.0f);
	RNA_def_property_ui_text(prop, "IOR", "Sets angular index of refraction for raytraced refraction.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "fresnel", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "fresnel_tra");
	RNA_def_property_range(prop, 0.0f, 5.0f);
	RNA_def_property_ui_text(prop, "Fresnel", "Power of Fresnel for transparency (Ray or ZTransp).");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "fresnel_fac", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "fresnel_tra_i");
	RNA_def_property_range(prop, 1.0f, 5.0f);
	RNA_def_property_ui_text(prop, "Fresnel Factor", "Blending factor for Fresnel.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "gloss", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "gloss_tra");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Gloss", "The clarity of the refraction. Values < 1.0 give diffuse, blurry refractions.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "gloss_samples", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "samp_gloss_tra");
	RNA_def_property_range(prop, 0, 1024);
	RNA_def_property_ui_text(prop, "Gloss Samples", "Number of cone samples averaged for blurry refractions.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "gloss_threshold", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "adapt_thresh_tra");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Gloss Threshold", "Threshold for adaptive sampling. If a sample contributes less than this amount (as a percentage), sampling is stopped.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "depth", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "ray_depth_tra");
	RNA_def_property_range(prop, 0, 10);
	RNA_def_property_ui_text(prop, "Depth", "Maximum allowed number of light inter-refractions.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "filter", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "filter");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Filter", "Amount to blend in the material's diffuse color in raytraced transparency (simulating absorption).");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "limit", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "tx_limit");
	RNA_def_property_range(prop, 0.0f, 100.0f);
	RNA_def_property_ui_text(prop, "Limit", "Maximum depth for light to travel through the transparent material before becoming fully filtered (0.0 is disabled).");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "falloff", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "tx_falloff");
	RNA_def_property_range(prop, 0.1f, 10.0f);
	RNA_def_property_ui_text(prop, "Falloff", "Falloff power for transmissivity filter effect (1.0 is linear).");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "specular_opacity", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "spectra");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Specular Opacity", "Makes specular areas opaque on transparent materials.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
}

static void rna_def_material_halo(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna= RNA_def_struct(brna, "MaterialHalo", NULL);
	RNA_def_struct_sdna(srna, "Material");
	RNA_def_struct_nested(brna, srna, "Material");
	RNA_def_struct_ui_text(srna, "Material Halo", "Halo particle effect settings for a Material datablock.");

	prop= RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO); /* use bitflags */
	RNA_def_property_ui_text(prop, "Enabled", "Enables halo rendering of material.");
	RNA_def_property_boolean_funcs(prop, NULL, "rna_Material_mode_halo_set");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING_DRAW, NULL);
	
	prop= RNA_def_property(srna, "size", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "hasize");
	RNA_def_property_range(prop, 0.0f, 100.0f);
	RNA_def_property_ui_text(prop, "Size", "Sets the dimension of the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "hardness", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "har");
	RNA_def_property_range(prop, 0, 127);
	RNA_def_property_ui_text(prop, "Hardness", "Sets the hardness of the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "add", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "add");
	RNA_def_property_range(prop, 0.0f, 1.0f);
	RNA_def_property_ui_text(prop, "Add", "Sets the strength of the add effect.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "rings", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "ringc");
	RNA_def_property_range(prop, 0, 24);
	RNA_def_property_ui_text(prop, "Rings", "Sets the number of rings rendered over the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "line_number", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "linec");
	RNA_def_property_range(prop, 0, 250);
	RNA_def_property_ui_text(prop, "Line Number", "Sets the number of star shaped lines rendered over the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "star_tips", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "starc");
	RNA_def_property_range(prop, 3, 50);
	RNA_def_property_ui_text(prop, "Star Tips", "Sets the number of points on the star shaped halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "seed", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "seed1");
	RNA_def_property_range(prop, 0, 255);
	RNA_def_property_ui_text(prop, "Seed", "Randomizes ring dimension and line location.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "flare_mode", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO_FLARE); /* use bitflags */
	RNA_def_property_ui_text(prop, "Flare", "Renders halo as a lensflare.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "flare_size", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "flaresize");
	RNA_def_property_range(prop, 0.1f, 25.0f);
	RNA_def_property_ui_text(prop, "Flare Size", "Sets the factor by which the flare is larger than the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "flare_subsize", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "subsize");
	RNA_def_property_range(prop, 0.1f, 25.0f);
	RNA_def_property_ui_text(prop, "Flare Subsize", "Sets the dimension of the subflares, dots and circles.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "flare_boost", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "flareboost");
	RNA_def_property_range(prop, 0.1f, 10.0f);
	RNA_def_property_ui_text(prop, "Flare Boost", "Gives the flare extra strength.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "flare_seed", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "seed2");
	RNA_def_property_range(prop, 0, 255);
	RNA_def_property_ui_text(prop, "Flare Seed", "Specifies an offset in the flare seed table.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "flares_sub", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "flarec");
	RNA_def_property_range(prop, 1, 32);
	RNA_def_property_ui_text(prop, "Flares Sub", "Sets the number of subflares.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "ring", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO_RINGS); /* use bitflags */
	RNA_def_property_ui_text(prop, "Rings", "Renders rings over halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "lines", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO_LINES); /* use bitflags */
	RNA_def_property_ui_text(prop, "Lines", "Renders star shaped lines over halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "star", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_STAR); /* use bitflags */
	RNA_def_property_ui_text(prop, "Star", "Renders halo as a star.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "use_texture", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALOTEX); /* use bitflags */
	RNA_def_property_ui_text(prop, "Use Texture", "Gives halo a texture.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "use_vertex_normal", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALOPUNO); /* use bitflags */
	RNA_def_property_ui_text(prop, "Use Vertex Normal", "Uses the vertex normal to specify the dimension of the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "xalpha", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO_XALPHA); /* use bitflags */
	RNA_def_property_ui_text(prop, "Extreme Alpha", "Uses extreme alpha.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "shaded", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO_SHADE); /* use bitflags */
	RNA_def_property_ui_text(prop, "Shaded", "Lets halo receive light and shadows.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
	
	prop= RNA_def_property(srna, "soft", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_HALO_SOFT); /* use bitflags */
	RNA_def_property_ui_text(prop, "Soft", "Softens the halo.");
	RNA_def_property_update(prop, NC_MATERIAL|ND_SHADING, NULL);
}

static void rna_def_material_sss(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna= RNA_def_struct(brna, "MaterialSubsurfaceScattering", NULL);
	RNA_def_struct_sdna(srna, "Material");
	RNA_def_struct_nested(brna, srna, "Material");
	RNA_def_struct_ui_text(srna, "Material Subsurface Scattering", "Diffuse subsurface scattering settings for a Material datablock.");

	prop= RNA_def_property(srna, "radius", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "sss_radius");
	RNA_def_property_range(prop, 0.001, FLT_MAX);
	RNA_def_property_ui_range(prop, 0.001, 10000, 1, 3);
	RNA_def_property_ui_text(prop, "Radius", "Mean red/green/blue scattering path length.");

	prop= RNA_def_property(srna, "color", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_float_sdna(prop, NULL, "sss_col");
	RNA_def_property_ui_text(prop, "Color", "Scattering color.");

	prop= RNA_def_property(srna, "error_tolerance", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_error");
	RNA_def_property_ui_range(prop, 0.0001, 10, 1, 3);
	RNA_def_property_ui_text(prop, "Error Tolerance", "Error tolerance (low values are slower and higher quality).");

	prop= RNA_def_property(srna, "scale", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_scale");
	RNA_def_property_ui_range(prop, 0.001, 1000, 1, 3);
	RNA_def_property_ui_text(prop, "Scale", "Object scale factor.");

	prop= RNA_def_property(srna, "ior", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_ior");
	RNA_def_property_ui_range(prop, 0.1, 2, 1, 3);
	RNA_def_property_ui_text(prop, "IOR", "Index of refraction (higher values are denser).");

	prop= RNA_def_property(srna, "color_factor", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_colfac");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Color Factor", "Blend factor for SSS colors.");

	prop= RNA_def_property(srna, "texture_factor", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_texfac");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Texture Factor", "Texture scatting blend factor.");

	prop= RNA_def_property(srna, "front", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_front");
	RNA_def_property_range(prop, 0, 2);
	RNA_def_property_ui_text(prop, "Front", "Front scattering weight.");

	prop= RNA_def_property(srna, "back", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sss_back");
	RNA_def_property_range(prop, 0, 10);
	RNA_def_property_ui_text(prop, "Back", "Back scattering weight.");

	prop= RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "sss_flag", MA_DIFF_SSS);
	RNA_def_property_ui_text(prop, "Enabled", "Enable diffuse subsurface scatting effects in a material.");
}

void rna_def_material_specularity(StructRNA *srna)
{
	PropertyRNA *prop;

	prop= RNA_def_property(srna, "specularity", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "spec");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Specularity", "");

	/* XXX: this field is also used for Halo hardness. should probably be fixed in DNA */
	prop= RNA_def_property(srna, "specular_hardness", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "har");
	RNA_def_property_range(prop, 1, 511);
	RNA_def_property_ui_text(prop, "Specular Hardness", "");

	prop= RNA_def_property(srna, "specular_refraction", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "refrac");
	RNA_def_property_range(prop, 1, 10);
	RNA_def_property_ui_text(prop, "Specular Refraction", "");

	/* XXX: evil "param" field also does specular stuff */

	prop= RNA_def_property(srna, "specular_slope", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "rms");
	RNA_def_property_range(prop, 0, 0.4);
	RNA_def_property_ui_text(prop, "Specular Slope", "The standard deviation of surface slope.");
}

void rna_def_material_strand(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;

	srna= RNA_def_struct(brna, "MaterialStrand", NULL);
	RNA_def_struct_sdna(srna, "Material");
	RNA_def_struct_nested(brna, srna, "Material");
	RNA_def_struct_ui_text(srna, "Material Strand", "Strand settings for a Material datablock.");

	prop= RNA_def_property(srna, "tangent_shading", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_TANGENT_STR);
	RNA_def_property_ui_text(prop, "Tangent Shading", "Uses direction of strands as normal for tangent-shading.");
	
	prop= RNA_def_property(srna, "surface_diffuse", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_STR_SURFDIFF);
	RNA_def_property_ui_text(prop, "Surface Diffuse", "Make diffuse shading more similar to shading the surface.");

	prop= RNA_def_property(srna, "blend_distance", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "strand_surfnor");
	RNA_def_property_range(prop, 0, 10);
	RNA_def_property_ui_text(prop, "Blend Distance", "Distance in Blender units over which to blend in the surface normal.");

	prop= RNA_def_property(srna, "blender_units", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "mode", MA_STR_B_UNITS);
	RNA_def_property_ui_text(prop, "Blender Units", "Use Blender units for widths instead of pixels.");

	prop= RNA_def_property(srna, "start_size", PROP_FLOAT, PROP_UNSIGNED);
	RNA_def_property_float_sdna(prop, NULL, "strand_sta");
	RNA_def_property_float_funcs(prop, NULL, NULL, "rna_MaterialStrand_start_size_range");
	RNA_def_property_ui_text(prop, "Start Size", "Start size of strands in pixels Blender units.");

	prop= RNA_def_property(srna, "end_size", PROP_FLOAT, PROP_UNSIGNED);
	RNA_def_property_float_sdna(prop, NULL, "strand_end");
	RNA_def_property_float_funcs(prop, NULL, NULL, "rna_MaterialStrand_end_size_range");
	RNA_def_property_ui_text(prop, "End Size", "Start size of strands in pixels or Blender units.");

	prop= RNA_def_property(srna, "min_size", PROP_FLOAT, PROP_UNSIGNED);
	RNA_def_property_float_sdna(prop, NULL, "strand_min");
	RNA_def_property_range(prop, 0.001, 10);
	RNA_def_property_ui_text(prop, "Minimum Size", "Minimum size of strands in pixels.");

	prop= RNA_def_property(srna, "shape", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "strand_ease");
	RNA_def_property_range(prop, -0.9, 0.9);
	RNA_def_property_ui_text(prop, "Shape", "Positive values make strands rounder, negative makes strands spiky.");

	prop= RNA_def_property(srna, "width_fade", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "strand_widthfade");
	RNA_def_property_range(prop, 0, 2);
	RNA_def_property_ui_text(prop, "Width Fade", "Transparency along the width of the strand.");

	prop= RNA_def_property(srna, "uv_layer", PROP_STRING, PROP_NONE);
	RNA_def_property_string_sdna(prop, NULL, "strand_uvname");
	RNA_def_property_ui_text(prop, "UV Layer", "Name of UV layer to override.");
}

void RNA_def_material(BlenderRNA *brna)
{
	StructRNA *srna= NULL;
	PropertyRNA *prop= NULL;

	srna= RNA_def_struct(brna, "Material", "ID");
	RNA_def_struct_ui_text(srna, "Material", "Material datablock to defined the appearance of geometric objects for rendering.");

	prop= RNA_def_property(srna, "ambient", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "amb");
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Ambient", "Amount of global ambient color the material receives.");

	prop= RNA_def_property(srna, "emit", PROP_FLOAT, PROP_NONE);
	RNA_def_property_range(prop, 0, 2);
	RNA_def_property_ui_text(prop, "Emit", "Amount of light to emit.");

	prop= RNA_def_property(srna, "translucency", PROP_FLOAT, PROP_NONE);
	RNA_def_property_range(prop, 0, 1);
	RNA_def_property_ui_text(prop, "Translucency", "Amount of diffuse shading on the back side.");
		
	prop= RNA_def_property(srna, "cubic", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "shade_flag", MA_CUBIC);
	RNA_def_property_ui_text(prop, "Cubic", "Use cubic interpolation for diffuse values, for smoother transitions.");
	
	prop= RNA_def_property(srna, "object_color", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "shade_flag", MA_OBCOLOR);
	RNA_def_property_ui_text(prop, "Object Color", "Modulate the result with a per-object color.");

	prop= RNA_def_property(srna, "shadow_ray_bias", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "sbias");
	RNA_def_property_range(prop, 0, 0.25);
	RNA_def_property_ui_text(prop, "Shadow Ray Bias", "Shadow raytracing bias to prevent terminator problems on shadow boundary.");

	prop= RNA_def_property(srna, "shadow_buffer_bias", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "lbias");
	RNA_def_property_range(prop, 0, 10);
	RNA_def_property_ui_text(prop, "Shadow Buffer Bias", "Factor to multiply shadow buffer bias with (0 is ignore.)");

	prop= RNA_def_property(srna, "shadow_casting_alpha", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "shad_alpha");
	RNA_def_property_range(prop, 0.001, 1);
	RNA_def_property_ui_text(prop, "Shadow Casting Alpha", "Shadow casting alpha, only in use for Irregular Shadowbuffer.");

	/* nested structs */
	prop= RNA_def_property(srna, "raytrace_mirror", PROP_POINTER, PROP_NEVER_NULL);
	RNA_def_property_struct_type(prop, "MaterialRaytraceMirror");
	RNA_def_property_pointer_funcs(prop, "rna_Material_self_get", NULL, NULL);
	RNA_def_property_ui_text(prop, "Raytrace Mirror", "Raytraced reflection settings for the material.");

	prop= RNA_def_property(srna, "raytrace_transparency", PROP_POINTER, PROP_NEVER_NULL);
	RNA_def_property_struct_type(prop, "MaterialRaytraceTransparency");
	RNA_def_property_pointer_funcs(prop, "rna_Material_self_get", NULL, NULL);
	RNA_def_property_ui_text(prop, "Raytrace Transparency", "Raytraced reflection settings for the material.");

	prop= RNA_def_property(srna, "halo", PROP_POINTER, PROP_NEVER_NULL);
	RNA_def_property_struct_type(prop, "MaterialHalo");
	RNA_def_property_pointer_funcs(prop, "rna_Material_self_get", NULL, NULL);
	RNA_def_property_ui_text(prop, "Halo", "Halo settings for the material.");

	prop= RNA_def_property(srna, "subsurface_scattering", PROP_POINTER, PROP_NEVER_NULL);
	RNA_def_property_struct_type(prop, "MaterialSubsurfaceScattering");
	RNA_def_property_pointer_funcs(prop, "rna_Material_self_get", NULL, NULL);
	RNA_def_property_ui_text(prop, "Subsurface Scattering", "Subsurface scattering settings for the material.");

	prop= RNA_def_property(srna, "strand", PROP_POINTER, PROP_NEVER_NULL);
	RNA_def_property_struct_type(prop, "MaterialStrand");
	RNA_def_property_pointer_funcs(prop, "rna_Material_self_get", NULL, NULL);
	RNA_def_property_ui_text(prop, "Strand", "Strand settings for the material.");

	/* nodetree */
	prop= RNA_def_property(srna, "node_tree", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "nodetree");
	RNA_def_property_ui_text(prop, "Node Tree", "Node tree for node based materials.");

	/* common */
	rna_def_animdata_common(srna);
	rna_def_mtex_common(srna, "rna_Material_mtex_begin", "rna_Material_active_texture_get", "MaterialTextureSlot");

	prop= RNA_def_property(srna, "script_link", PROP_POINTER, PROP_NEVER_NULL);
	RNA_def_property_pointer_sdna(prop, NULL, "scriptlink");
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_ui_text(prop, "Script Link", "Scripts linked to this material.");

	/* XXX: does Material.septex get RNA? */
	
	rna_def_material_colors(srna);
	rna_def_material_diffuse(srna);
	rna_def_material_specularity(srna);

	/* nested structs */
	rna_def_material_raymirror(brna);
	rna_def_material_raytra(brna);
	rna_def_material_halo(brna);
	rna_def_material_sss(brna);
	rna_def_material_mtex(brna);
	rna_def_material_strand(brna);
}

void rna_def_mtex_common(StructRNA *srna, const char *begin, const char *activeget, const char *structname)
{
	PropertyRNA *prop;

	/* mtex */
	prop= RNA_def_property(srna, "textures", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_struct_type(prop, structname);
	RNA_def_property_collection_funcs(prop, begin, "rna_iterator_array_next", "rna_iterator_array_end", "rna_iterator_array_dereference_get", 0, 0, 0, 0);
	RNA_def_property_ui_text(prop, "Textures", "Texture slots defining the mapping and influence of textures.");

	prop= RNA_def_property(srna, "active_texture", PROP_POINTER, PROP_NONE);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_struct_type(prop, structname);
	RNA_def_property_pointer_funcs(prop, activeget, NULL, NULL);
	RNA_def_property_ui_text(prop, "Active Texture", "Active texture slot being displayed.");

	prop= RNA_def_property(srna, "active_texture_index", PROP_INT, PROP_UNSIGNED);
	RNA_def_property_int_sdna(prop, NULL, "texact");
	RNA_def_property_range(prop, 0, MAX_MTEX-1);
	RNA_def_property_ui_text(prop, "Active Texture Index", "Index of active texture slot.");
}

#endif


