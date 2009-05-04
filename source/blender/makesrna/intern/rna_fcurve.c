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
 * Contributor(s): Blender Foundation (2009), Joshua Leung
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>

#include "RNA_define.h"
#include "RNA_types.h"

#include "rna_internal.h"

#include "DNA_anim_types.h"
#include "DNA_scene_types.h"

#include "MEM_guardedalloc.h"

#ifdef RNA_RUNTIME

float FModGenFunc_amplitude_get(PointerRNA *ptr)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	return (data->coefficients) ? (float)data->coefficients[0] : 1.0f;
}

void FModGenFunc_amplitude_set(PointerRNA *ptr, float value)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	if (data->coefficients) data->coefficients[0]= value;
}

float FModGenFunc_pre_multiplier_get(PointerRNA *ptr)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	return (data->coefficients) ? (float)data->coefficients[1] : 1.0f;
}

void FModGenFunc_pre_multiplier_set(PointerRNA *ptr, float value)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	if (data->coefficients) data->coefficients[1]= value;
}

float FModGenFunc_x_offset_get(PointerRNA *ptr)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	return (data->coefficients) ? (float)data->coefficients[2] : 0.0f;
}

void FModGenFunc_x_offset_set(PointerRNA *ptr, float value)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	if (data->coefficients) data->coefficients[2]= value;
}

float FModGenFunc_y_offset_get(PointerRNA *ptr)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	return (data->coefficients) ? (float)data->coefficients[3] : 0.0f;
}

void FModGenFunc_y_offset_set(PointerRNA *ptr, float value)
{
	FModifier *fcm = (FModifier *)(ptr->data);
	FMod_Generator *data= (FMod_Generator*)(fcm->data);
	if (data->coefficients) data->coefficients[3]= value;
}

/* --------- */

StructRNA *rna_FModifierType_refine(struct PointerRNA *ptr)
{
	FModifier *fcm= (FModifier *)ptr->data;

	switch (fcm->type) {
		case FMODIFIER_TYPE_GENERATOR:
		{
			FMod_Generator *gen= (FMod_Generator *)fcm->data;
			
			switch (gen->mode) {
				case FCM_GENERATOR_POLYNOMIAL:
					return &RNA_FModifierGenerator_PolyExpanded;
				//case FCM_GENERATOR_POLYNOMIAL_FACTORISED:
				case FCM_GENERATOR_FUNCTION:
					return &RNA_FModifierGenerator_Function;
				//case FCM_GENERATOR_EXPRESSION:
				default:
					return &RNA_FModifierGenerator;
			}
		}
		case FMODIFIER_TYPE_ENVELOPE:
			return &RNA_FModifierEnvelope;
		case FMODIFIER_TYPE_CYCLES:
			return &RNA_FModifierCycles;
		case FMODIFIER_TYPE_NOISE:
			return &RNA_FModifierNoise;
		//case FMODIFIER_TYPE_FILTER:
		//	return &RNA_FModifierFilter;
		case FMODIFIER_TYPE_PYTHON:
			return &RNA_FModifierPython;
		case FMODIFIER_TYPE_LIMITS:
			return &RNA_FModifierLimits;
		default:
			return &RNA_UnknownType;
	}
}

/* ****************************** */

static void rna_DriverTarget_RnaPath_get(PointerRNA *ptr, char *value)
{
	DriverTarget *dtar= (DriverTarget *)ptr->data;

	if (dtar->rna_path)
		strcpy(value, dtar->rna_path);
	else
		strcpy(value, "");
}

static int rna_DriverTarget_RnaPath_length(PointerRNA *ptr)
{
	DriverTarget *dtar= (DriverTarget *)ptr->data;
	
	if (dtar->rna_path)
		return strlen(dtar->rna_path);
	else
		return 0;
}

static void rna_DriverTarget_RnaPath_set(PointerRNA *ptr, const char *value)
{
	DriverTarget *dtar= (DriverTarget *)ptr->data;
	
	// XXX in this case we need to be very careful, as this will require some new dependencies to be added!
	if (dtar->rna_path)
		MEM_freeN(dtar->rna_path);
	
	if (strlen(value))
		dtar->rna_path= BLI_strdup(value);
	else 
		dtar->rna_path= NULL;
}

/* ****************************** */

static void rna_FCurve_RnaPath_get(PointerRNA *ptr, char *value)
{
	FCurve *fcu= (FCurve *)ptr->data;

	if (fcu->rna_path)
		strcpy(value, fcu->rna_path);
	else
		strcpy(value, "");
}

static int rna_FCurve_RnaPath_length(PointerRNA *ptr)
{
	FCurve *fcu= (FCurve *)ptr->data;
	
	if (fcu->rna_path)
		return strlen(fcu->rna_path);
	else
		return 0;
}

static void rna_FCurve_RnaPath_set(PointerRNA *ptr, const char *value)
{
	FCurve *fcu= (FCurve *)ptr->data;

	if (fcu->rna_path)
		MEM_freeN(fcu->rna_path);
	
	if (strlen(value))
		fcu->rna_path= BLI_strdup(value);
	else 
		fcu->rna_path= NULL;
}

#else

static void rna_def_fmodifier_generator_common(StructRNA *srna)
{
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_mode_items[] = {
		{FCM_GENERATOR_POLYNOMIAL, "POLYNOMIAL", "Expanded Polynomial", ""},
		{FCM_GENERATOR_POLYNOMIAL_FACTORISED, "POLYNOMIAL_FACTORISED", "Factorised Polynomial", ""},
		{FCM_GENERATOR_FUNCTION, "FUNCTION", "Built-In Function", ""},
		{FCM_GENERATOR_EXPRESSION, "EXPRESSION", "Expression", ""},
		{0, NULL, NULL, NULL}};
		
	/* struct wrapping settings */
	RNA_def_struct_sdna_from(srna, "FMod_Generator", "data");	
	
	/* settings */
	prop= RNA_def_property(srna, "additive", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FCM_GENERATOR_ADDITIVE);
	RNA_def_property_ui_text(prop, "Additive", "Values generated by this modifier are applied on top of the existing values instead of overwriting them.");
	
		// XXX this has a special validation func
	prop= RNA_def_property(srna, "mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_mode_items);
	RNA_def_property_ui_text(prop, "Mode", "Type of generator to use.");
}

/* this is a temporary dummy generator-modifier wrapping (to be discarded) */
static void rna_def_fmodifier_generator(BlenderRNA *brna)
{
	StructRNA *srna;
	
	srna= RNA_def_struct(brna, "FModifierGenerator", "FModifier");
	RNA_def_struct_ui_text(srna, "Generator F-Curve Modifier", "Deterministically generates values for the modified F-Curve.");
	
	/* define common props */
	rna_def_fmodifier_generator_common(srna);
}

static void rna_def_fmodifier_generator_polyexpanded(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "FModifierGenerator_PolyExpanded", "FModifier");
	RNA_def_struct_ui_text(srna, "Expanded Polynomial Generator", "Generates values for the modified F-Curve using expanded polynomial expresion.");
	
	/* define common props */
	rna_def_fmodifier_generator_common(srna);
	
	/* order of the polynomial */
		// XXX this has a special validation func
	prop= RNA_def_property(srna, "poly_order", PROP_INT, PROP_NONE);
	RNA_def_property_ui_text(prop, "Polynomial Order", "The highest power of 'x' for this polynomial. (i.e. the number of coefficients - 1)");
	
	/* coefficients array */
	//prop= RNA_def_property(srna, "coefficients", PROP_FLOAT, PROP_NONE);
	//RNA_def_property_ui_text(prop, "Coefficients", "Coefficients for 'x' (starting from lowest power).");
}

static void rna_def_fmodifier_generator_function(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_type_items[] = {
		{0, "SIN", "Sine", ""},
		{1, "COS", "Cosine", ""},
		{2, "TAN", "Tangent", ""},
		{3, "SQRT", "Square Root", ""},
		{4, "LN", "Natural Logarithm", ""},
		{0, NULL, NULL, NULL}};
		
	
	srna= RNA_def_struct(brna, "FModifierGenerator_Function", "FModifier");
	RNA_def_struct_ui_text(srna, "Built-In Function Generator", "Generates values for modified F-Curve using Built-In Function.");
	
	/* common settings */
	rna_def_fmodifier_generator_common(srna);
	
	/* type */
	prop= RNA_def_property(srna, "func_type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "Type", "Type of Built-In function to use as generator.");
	
	/* coefficients */
	prop= RNA_def_property(srna, "amplitude", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_funcs(prop, "FModGenFunc_amplitude_get", "FModGenFunc_amplitude_set", NULL);
	RNA_def_property_ui_text(prop, "Amplitude", "Scale factor for y-values generated by the function.");
	
	prop= RNA_def_property(srna, "pre_multiplier", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_funcs(prop, "FModGenFunc_pre_multiplier_get", "FModGenFunc_pre_multiplier_set", NULL);
	RNA_def_property_ui_text(prop, "PreMultiplier", "Scale factor for x-value inputs to function.");
	
	prop= RNA_def_property(srna, "x_offset", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_funcs(prop, "FModGenFunc_x_offset_get", "FModGenFunc_x_offset_set", NULL);
	RNA_def_property_ui_text(prop, "X Offset", "Offset for x-value inputs to function.");
	
	prop= RNA_def_property(srna, "y_offset", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_funcs(prop, "FModGenFunc_y_offset_get", "FModGenFunc_y_offset_set", NULL);
	RNA_def_property_ui_text(prop, "Y Offset", "Offset for y-values generated by the function.");
}

/* --------- */

static void rna_def_fmodifier_envelope(BlenderRNA *brna)
{
	StructRNA *srna;
	//PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "FModifierEnvelope", "FModifier");
	RNA_def_struct_ui_text(srna, "Envelope F-Curve Modifier", "Scales the values of the modified F-Curve.");
	RNA_def_struct_sdna_from(srna, "FMod_Envelope", "data");
}

/* --------- */

static void rna_def_fmodifier_cycles(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_type_items[] = {
		{FCM_EXTRAPOLATE_NONE, "NONE", "No Cycles", "Don't do anything."},
		{FCM_EXTRAPOLATE_CYCLIC, "REPEAT", "Repeat Motion", "Repeat keyframe range as-is."},
		{FCM_EXTRAPOLATE_CYCLIC_OFFSET, "REPEAT_OFFSET", "Repeat with Offset", "Repeat keyframe range, but with offset based on gradient between values"},
		{FCM_EXTRAPOLATE_MIRROR, "MIRROR", "Repeat Mirrored", "Alternate between forward and reverse playback of keyframe range"},
		{0, NULL, NULL, NULL}};
	
	srna= RNA_def_struct(brna, "FModifierCycles", "FModifier");
	RNA_def_struct_ui_text(srna, "Cycles F-Curve Modifier", "Repeats the values of the modified F-Curve.");
	RNA_def_struct_sdna_from(srna, "FMod_Cycles", "data");
	
	/* before */
	prop= RNA_def_property(srna, "before_mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "Before Mode", "Cycling mode to use before first keyframe.");
	
	prop= RNA_def_property(srna, "before_cycles", PROP_FLOAT, PROP_NONE);
	RNA_def_property_ui_text(prop, "Before Cycles", "Maximum number of cycles to allow before first keyframe. (0 = infinite)");
	
	
	/* after */
	prop= RNA_def_property(srna, "after_mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "After Mode", "Cycling mode to use after last keyframe.");
	
	prop= RNA_def_property(srna, "after_cycles", PROP_FLOAT, PROP_NONE);
	RNA_def_property_ui_text(prop, "After Cycles", "Maximum number of cycles to allow after last keyframe. (0 = infinite)");
}

/* --------- */

static void rna_def_fmodifier_python(BlenderRNA *brna)
{
	StructRNA *srna;
	//PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "FModifierPython", "FModifier");
	RNA_def_struct_ui_text(srna, "Python F-Curve Modifier", "Performs user-defined operation on the modified F-Curve.");
	RNA_def_struct_sdna_from(srna, "FMod_Python", "data");
}

/* --------- */

static void rna_def_fmodifier_limits(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "FModifierLimits", "FModifier");
	RNA_def_struct_ui_text(srna, "Limits F-Curve Modifier", "Limits the time/value ranges of the modified F-Curve.");
	RNA_def_struct_sdna_from(srna, "FMod_Limits", "data");
	
	prop= RNA_def_property(srna, "use_minimum_x", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FCM_LIMIT_XMIN);
	RNA_def_property_ui_text(prop, "Minimum X", "Use the minimum X value.");
	
	prop= RNA_def_property(srna, "use_minimum_y", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FCM_LIMIT_YMIN);
	RNA_def_property_ui_text(prop, "Minimum Y", "Use the minimum Y value.");
	
	prop= RNA_def_property(srna, "use_maximum_x", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FCM_LIMIT_XMAX);
	RNA_def_property_ui_text(prop, "Maximum X", "Use the maximum X value.");
	
	prop= RNA_def_property(srna, "use_maximum_y", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FCM_LIMIT_YMAX);
	RNA_def_property_ui_text(prop, "Maximum Y", "Use the maximum Y value.");
	
	prop= RNA_def_property(srna, "minimum_x", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "rect.xmin");
	RNA_def_property_ui_text(prop, "Minimum X", "Lowest X value to allow.");
	
	prop= RNA_def_property(srna, "minimum_y", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "rect.ymin");
	RNA_def_property_ui_text(prop, "Minimum Y", "Lowest Y value to allow.");
	
	prop= RNA_def_property(srna, "maximum_x", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "rect.xmax");
	RNA_def_property_ui_text(prop, "Maximum X", "Highest X value to allow.");
	
	prop= RNA_def_property(srna, "maximum_y", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "rect.ymax");
	RNA_def_property_ui_text(prop, "Maximum Y", "Highest Y value to allow.");
}

/* --------- */

static void rna_def_fmodifier_noise(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_modification_items[] = {
		{FCM_NOISE_MODIF_REPLACE, "REPLACE", "Replace", ""},
		{FCM_NOISE_MODIF_ADD, "ADD", "Add", ""},
		{FCM_NOISE_MODIF_SUBTRACT, "SUBTRACT", "Subtract", ""},
		{FCM_NOISE_MODIF_MULTIPLY, "MULTIPLY", "Multiply", ""},
		{0, NULL, NULL, NULL}};
	
	srna= RNA_def_struct(brna, "FModifierNoise", "FModifier");
	RNA_def_struct_ui_text(srna, "Noise F-Curve Modifier", "Gives randomness to the modified F-Curve.");
	RNA_def_struct_sdna_from(srna, "FMod_Noise", "data");
	
	prop= RNA_def_property(srna, "modification", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_modification_items);
	RNA_def_property_ui_text(prop, "Modification", "Method of modifying the existing F-Curve.");
	
	prop= RNA_def_property(srna, "size", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "size");
	RNA_def_property_ui_text(prop, "Size", "Scaling (in time) of the noise");
	
	prop= RNA_def_property(srna, "strength", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "strength");
	RNA_def_property_ui_text(prop, "Strength", "Amplitude of the noise - the amount that it modifies the underlying curve");
	
	prop= RNA_def_property(srna, "phase", PROP_FLOAT, PROP_NONE);
	RNA_def_property_float_sdna(prop, NULL, "phase");
	RNA_def_property_ui_text(prop, "Phase", "A random seed for the noise effect");
	
	prop= RNA_def_property(srna, "depth", PROP_INT, PROP_NONE);
	RNA_def_property_int_sdna(prop, NULL, "depth");
	RNA_def_property_ui_text(prop, "Depth", "Amount of fine level detail present in the noise");

}


/* --------- */

void rna_def_fmodifier(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_type_items[] = {
		{FMODIFIER_TYPE_NULL, "NULL", "Invalid", ""},
		{FMODIFIER_TYPE_GENERATOR, "GENERATOR", "Generator", ""},
		{FMODIFIER_TYPE_ENVELOPE, "ENVELOPE", "Envelope", ""},
		{FMODIFIER_TYPE_CYCLES, "CYCLES", "Cycles", ""},
		{FMODIFIER_TYPE_NOISE, "NOISE", "Noise", ""},
		{FMODIFIER_TYPE_FILTER, "FILTER", "Filter", ""},
		{FMODIFIER_TYPE_PYTHON, "PYTHON", "Python", ""},
		{FMODIFIER_TYPE_LIMITS, "LIMITS", "Limits", ""},
		{0, NULL, NULL, NULL}};
		
		
	/* base struct definition */
	srna= RNA_def_struct(brna, "FModifier", NULL);
	RNA_def_struct_refine_func(srna, "rna_FModifierType_refine");
	RNA_def_struct_ui_text(srna, "FCurve Modifier", "Modifier for values of F-Curve.");
	
#if 0 // XXX not used yet
	/* name */
	prop= RNA_def_property(srna, "name", PROP_STRING, PROP_NONE);
	RNA_def_struct_name_property(srna, prop);
	RNA_def_property_ui_text(prop, "Name", "Short description of F-Curve Modifier.");
#endif // XXX not used yet
	
	/* type */
	prop= RNA_def_property(srna, "type", PROP_ENUM, PROP_NONE);
	RNA_def_property_clear_flag(prop, PROP_EDITABLE);
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "Type", "F-Curve Modifier Type");
	
	/* settings */
	prop= RNA_def_property(srna, "expanded", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FMODIFIER_FLAG_EXPANDED);
	RNA_def_property_ui_text(prop, "Expanded", "F-Curve Modifier's panel is expanded in UI.");
	
	prop= RNA_def_property(srna, "muted", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_clear_flag(prop, PROP_EDITABLE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FMODIFIER_FLAG_MUTED);
	RNA_def_property_ui_text(prop, "Muted", "F-Curve Modifier will not be evaluated.");
	
		// XXX this is really an internal flag, but it may be useful for some tools to be able to access this...
	prop= RNA_def_property(srna, "disabled", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_clear_flag(prop, PROP_EDITABLE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FMODIFIER_FLAG_DISABLED);
	RNA_def_property_ui_text(prop, "Disabled", "F-Curve Modifier has invalid settings and will not be evaluated.");
	
		// TODO: setting this to true must ensure that all others in stack are turned off too...
	prop= RNA_def_property(srna, "active", PROP_BOOLEAN, PROP_NONE);
	RNA_def_property_boolean_sdna(prop, NULL, "flag", FMODIFIER_FLAG_ACTIVE);
	RNA_def_property_ui_text(prop, "Active", "F-Curve Modifier is the one being edited ");
}	

/* *********************** */

void rna_def_drivertarget(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "DriverTarget", NULL);
	RNA_def_struct_ui_text(srna, "Driver Target", "Variable from some source/target for driver relationship.");
	
	/* Variable Name */
	prop= RNA_def_property(srna, "name", PROP_STRING, PROP_NONE);
	RNA_def_struct_name_property(srna, prop);
	RNA_def_property_ui_text(prop, "Name", "Name to use in scripted expressions/functions.");
	
	/* Target Properties */
	prop= RNA_def_property(srna, "target", PROP_POINTER, PROP_NONE);
	RNA_def_property_pointer_sdna(prop, NULL, "id");
	RNA_def_property_ui_text(prop, "Object", "Object the specific property used can be found from");
	
	prop= RNA_def_property(srna, "rna_path", PROP_STRING, PROP_NONE);
	RNA_def_property_string_funcs(prop, "rna_DriverTarget_RnaPath_get", "rna_DriverTarget_RnaPath_length", "rna_DriverTarget_RnaPath_set");
	RNA_def_property_ui_text(prop, "RNA Path", "RNA Path (from Object) to property used");
	
	prop= RNA_def_property(srna, "array_index", PROP_INT, PROP_NONE);
	RNA_def_property_ui_text(prop, "RNA Array Index", "Index to the specific property used (if applicable)");
}

void rna_def_channeldriver(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_type_items[] = {
		{DRIVER_TYPE_AVERAGE, "AVERAGE", "Averaged Value", ""},
		{DRIVER_TYPE_PYTHON, "SCRIPTED", "Scripted Expression", ""},
		{DRIVER_TYPE_ROTDIFF, "ROTDIFF", "Rotational Difference", ""},
		{0, NULL, NULL, NULL}};

	srna= RNA_def_struct(brna, "Driver", NULL);
	RNA_def_struct_sdna(srna, "ChannelDriver");
	RNA_def_struct_ui_text(srna, "Driver", "Driver for the value of a setting based on an external value.");

	/* Enums */
	prop= RNA_def_property(srna, "type", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_type_items);
	RNA_def_property_ui_text(prop, "Type", "Driver types.");

	/* String values */
	prop= RNA_def_property(srna, "expression", PROP_STRING, PROP_NONE);
	RNA_def_property_ui_text(prop, "Expression", "Expression to use for Scripted Expression.");

	/* Collections */
	prop= RNA_def_property(srna, "targets", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "targets", NULL);
	RNA_def_property_struct_type(prop, "DriverTarget");
	RNA_def_property_ui_text(prop, "Target Variables", "Properties acting as targets for this driver.");
}

/* *********************** */

void rna_def_fcurve(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	static EnumPropertyItem prop_mode_extend_items[] = {
		{FCURVE_EXTRAPOLATE_CONSTANT, "CONSTANT", "Constant", ""},
		{FCURVE_EXTRAPOLATE_LINEAR, "LINEAR", "Linear", ""},
		{0, NULL, NULL, NULL}};
	static EnumPropertyItem prop_mode_color_items[] = {
		{FCURVE_COLOR_AUTO_RAINBOW, "AUTO_RAINBOW", "Automatic Rainbow", ""},
		{FCURVE_COLOR_AUTO_RGB, "AUTO_RGB", "Automatic XYZ to RGB", ""},
		{FCURVE_COLOR_CUSTOM, "CUSTOM", "User Defined", ""},
		{0, NULL, NULL, NULL}};

	srna= RNA_def_struct(brna, "FCurve", NULL);
	RNA_def_struct_ui_text(srna, "F-Curve", "F-Curve defining values of a period of time.");

	/* Enums */
	prop= RNA_def_property(srna, "extrapolation", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_sdna(prop, NULL, "extend");
	RNA_def_property_enum_items(prop, prop_mode_extend_items);
	RNA_def_property_ui_text(prop, "Extrapolation", "");

	/* Pointers */
	prop= RNA_def_property(srna, "driver", PROP_POINTER, PROP_NONE);
	RNA_def_property_clear_flag(prop, PROP_EDITABLE);
	RNA_def_property_ui_text(prop, "Driver", "Channel Driver (only set for Driver F-Curves)");
	
	/* Path + Array Index */
	prop= RNA_def_property(srna, "rna_path", PROP_STRING, PROP_NONE);
	RNA_def_property_string_funcs(prop, "rna_FCurve_RnaPath_get", "rna_FCurve_RnaPath_length", "rna_FCurve_RnaPath_set");
	RNA_def_property_ui_text(prop, "RNA Path", "RNA Path to property affected by F-Curve.");
	
	prop= RNA_def_property(srna, "array_index", PROP_INT, PROP_NONE);
	RNA_def_property_ui_text(prop, "RNA Array Index", "Index to the specific property affected by F-Curve if applicable.");
	
	/* Color */
	prop= RNA_def_property(srna, "color_mode", PROP_ENUM, PROP_NONE);
	RNA_def_property_enum_items(prop, prop_mode_color_items);
	RNA_def_property_ui_text(prop, "Color Mode", "Method used to determine color of F-Curve in Graph Editor.");
	
	prop= RNA_def_property(srna, "color", PROP_FLOAT, PROP_COLOR);
	RNA_def_property_array(prop, 3);
	RNA_def_property_ui_text(prop, "Color", "Color of the F-Curve in the Graph Editor.");
	
	/* Collections */
	prop= RNA_def_property(srna, "sampled_points", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "fpt", "totvert");
	RNA_def_property_struct_type(prop, "CurvePoint"); // XXX FPoints not BPoints here! FPoints are much smaller!
	RNA_def_property_ui_text(prop, "Sampled Points", "Sampled animation data");

	prop= RNA_def_property(srna, "keyframe_points", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_collection_sdna(prop, NULL, "bezt", "totvert");
	RNA_def_property_struct_type(prop, "BezierCurvePoint");
	RNA_def_property_ui_text(prop, "Keyframes", "User-editable keyframes");
	
	prop= RNA_def_property(srna, "modifiers", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_struct_type(prop, "FModifier");
	RNA_def_property_ui_text(prop, "Modifiers", "Modifiers affecting the shape of the F-Curve.");
}

/* *********************** */

void RNA_def_fcurve(BlenderRNA *brna)
{
	rna_def_fcurve(brna);
	
	rna_def_drivertarget(brna);
	rna_def_channeldriver(brna);
	
	rna_def_fmodifier(brna);
	
	rna_def_fmodifier_generator(brna);
		rna_def_fmodifier_generator_polyexpanded(brna);
		rna_def_fmodifier_generator_function(brna);
	rna_def_fmodifier_envelope(brna);
	rna_def_fmodifier_cycles(brna);
	rna_def_fmodifier_python(brna);
	rna_def_fmodifier_limits(brna);
	rna_def_fmodifier_noise(brna);
}


#endif
