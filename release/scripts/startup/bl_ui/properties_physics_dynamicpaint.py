# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>
import bpy

from bl_ui.properties_physics_common import (
    point_cache_ui,
    effector_weights_ui,
    )

class PhysicButtonsPanel():
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "physics"

    @classmethod
    def poll(cls, context):
        ob = context.object
        rd = context.scene.render
        return (ob and ob.type == 'MESH') and (not rd.use_game_engine) and (context.dynamic_paint)


class PHYSICS_PT_dynamic_paint(PhysicButtonsPanel, bpy.types.Panel):
    bl_label = "Dynamic Paint"

    def draw(self, context):
        layout = self.layout

        md = context.dynamic_paint
        ob = context.object

        if md:
            layout.prop(md, "dynamicpaint_type", expand=True)

            if md.dynamicpaint_type == 'CANVAS':
                canvas = md.canvas_settings
                surface = canvas.canvas_surfaces.active
                row = layout.row()
                row.template_list(canvas, "canvas_surfaces", canvas.canvas_surfaces, "active_index", rows=2)

                col = row.column(align=True)
                col.operator("dpaint.surface_slot_add", icon='ZOOMIN', text="")
                col.operator("dpaint.surface_slot_remove", icon='ZOOMOUT', text="")
                
                if surface:
                    layout.prop(surface, "name")
                    layout.prop(surface, "surface_format", expand=False)
                    
                    if surface.surface_format != "VERTEX":
                        col = layout.column()
                        col.label(text="Quality:")
                        col.prop(surface, "image_resolution")
                        col.prop(surface, "use_anti_aliasing")
                
                    col = layout.column()
                    col.label(text="Frames:")
                    split = col.split()
                
                    col = split.column(align=True)
                    col.prop(surface, "start_frame", text="Start")
                    col.prop(surface, "end_frame", text="End")
                
                    col = split.column()
                    col.prop(surface, "substeps")
                

            elif md.dynamicpaint_type == 'BRUSH':
                brush = md.brush_settings
                
                layout.prop(brush, "brush_settings_context", expand=True, icon_only=True)
                
                if (brush.brush_settings_context == "GENERAL"):
                    split = layout.split()

                    col = split.column()
                    col.prop(brush, "absolute_alpha")
                    col.prop(brush, "paint_erase")
                    col.prop(brush, "paint_wetness", text="Wetness")
                
                    col = split.column()
                    sub = col.column()
                    sub.active = (brush.paint_source != "PSYS");
                    sub.prop(brush, "use_material")
                    if brush.use_material and brush.paint_source != "PSYS":
                        col.prop(brush, "material", text="")
                        col.prop(brush, "paint_alpha", text="Alpha Factor")
                    else:
                        col.prop(brush, "paint_color", text="")
                        col.prop(brush, "paint_alpha", text="Alpha")
                
                elif (brush.brush_settings_context == "WAVE"):
                    layout.prop(brush, "wave_type")
                    if (brush.wave_type != "REFLECT"):
                        split = layout.split(percentage=0.6)
                        split.prop(brush, "wave_factor")
                else:
                    layout.label(text="-WIP-")


class PHYSICS_PT_dp_advanced_canvas(PhysicButtonsPanel, bpy.types.Panel):
    bl_label = "Dynamic Paint: Advanced"

    @classmethod
    def poll(cls, context):
        md = context.dynamic_paint
        return md and (md.dynamicpaint_type == 'CANVAS') and (context.dynamic_paint.canvas_settings.canvas_surfaces.active)

    def draw(self, context):
        layout = self.layout

        canvas = context.dynamic_paint.canvas_settings
        surface = canvas.canvas_surfaces.active
        ob = context.object

        layout.prop(surface, "surface_type", expand=False)
        layout.separator()

        if (surface.surface_type == "PAINT"):
            layout.label(text="Wetmap drying:")
            split = layout.split(percentage=0.8)
            split.prop(surface, "dry_speed", text="Dry Time")
            split.prop(surface, "use_dry_log", text="Slow")
            
        if (surface.surface_type != "WAVE"):
            if (surface.surface_type == "DISPLACE"):
                layout.prop(surface, "use_dissolve", text="Dissolve:")
            elif (surface.surface_type == "WEIGHT"):
                layout.prop(surface, "use_dissolve", text="Fade:")
            else:
                layout.prop(surface, "use_dissolve", text="Dissolve:")
            sub = layout.column()
            sub.active = surface.use_dissolve
            split = sub.split(percentage=0.8)
            split.prop(surface, "dissolve_speed", text="Time")
            split.prop(surface, "use_dissolve_log", text="Slow")
            
        if (surface.surface_type == "WAVE"):
            layout.prop(surface, "wave_open_borders")
            
            split = layout.split()
            
            col = split.column(align=True)
            col.prop(surface, "wave_timescale")
            col.prop(surface, "wave_speed")
            
            col = split.column(align=True)
            col.prop(surface, "wave_damping")
            col.prop(surface, "wave_spring")
            
        layout.label(text="Brush Group:")
        layout.prop(surface, "brush_group", text="")


class PHYSICS_PT_dp_canvas_output(PhysicButtonsPanel, bpy.types.Panel):
    bl_label = "Dynamic Paint: Output"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(cls, context):
        md = context.dynamic_paint
        if ((not md) or (md.dynamicpaint_type != 'CANVAS')):
            return 0
        surface = context.dynamic_paint.canvas_settings.canvas_surfaces.active
        return (surface and not (surface.surface_format=="VERTEX" and (surface.surface_type=="DISPLACE" or surface.surface_type=="WAVE")))

    def draw(self, context):
        layout = self.layout

        canvas = context.dynamic_paint.canvas_settings
        surface = canvas.canvas_surfaces.active
        ob = context.object
        
        # vertex format outputs
        if (surface.surface_format == "VERTEX"):
            if (surface.surface_type == "PAINT"):
                row = layout.row()
                row.prop_search(surface, "output_name", ob.data, "vertex_colors", text="Paintmap layer: ")
                #col = row.column(align=True)
                #col.operator("dpaint.output_add", icon='ZOOMIN', text="")
                
                row = layout.row()
                row.prop_search(surface, "output_name2", ob.data, "vertex_colors", text="Wetmap layer: ")
                #col = row.column(align=True)
                #col.operator("dpaint.output_add", icon='ZOOMIN', text="")
            if (surface.surface_type == "WEIGHT"):
                layout.prop_search(surface, "output_name", ob, "vertex_groups", text="Vertex Group: ")

        # image format outputs
        if (surface.surface_format == "IMAGE"):
            col = layout.column()
            col.label(text="UV layer:")
            col.prop_search(surface, "uv_layer", ob.data, "uv_textures", text="")
            
            col.separator()
            col = layout.column()
            col.prop(surface, "image_output_path", text="Output directory")
            col.prop(surface, "image_fileformat", text="Image Format")
            col.separator()
            if (surface.surface_type == "PAINT"):
                split = col.split()
                col = split.column()
                col.prop(surface, "do_output1", text="Output Paintmaps:")
                sub = split.column()
                sub.prop(surface, "premultiply", text="Premultiply alpha")
                sub.active = surface.do_output1
                sub = layout.column()
                sub.active = surface.do_output1
                sub.prop(surface, "output_name", text="Filename: ")
                
                col = layout.column()
                col.prop(surface, "do_output2", text="Output Wetmaps:")
                sub = col.column()
                sub.active = surface.do_output2
                sub.prop(surface, "output_name2", text="Filename: ")
            else:
                col.prop(surface, "output_name", text="Filename: ")
                if (surface.surface_type == "DISPLACE"):
                    col.prop(surface, "disp_type", text="Displace Type")
            
            layout.separator()
            layout.operator("dpaint.bake", text="Bake Image Sequence", icon='MOD_DYNAMICPAINT')
            if len(canvas.ui_info) != 0:
                layout.label(text=canvas.ui_info)
		

class PHYSICS_PT_dp_effects(PhysicButtonsPanel, bpy.types.Panel):
    bl_label = "Dynamic Paint: Effects"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(cls, context):
        md = context.dynamic_paint
        if ((not md) or (md.dynamicpaint_type != 'CANVAS')):
            return False;
        surface = context.dynamic_paint.canvas_settings.canvas_surfaces.active
        return surface and (surface.surface_type == "PAINT")

    def draw(self, context):
        layout = self.layout

        canvas = context.dynamic_paint.canvas_settings
        surface = canvas.canvas_surfaces.active

        layout.prop(surface, "effect_ui", expand=True)

        if surface.effect_ui == "SPREAD":
            layout.prop(surface, "use_spread")
            col = layout.column()
            col.active = surface.use_spread
            col.prop(surface, "spread_speed")

        elif surface.effect_ui == "DRIP":
            layout.prop(surface, "use_drip")
            col = layout.column()
            col.active = surface.use_drip
            effector_weights_ui(self, context, surface.effector_weights)

        elif surface.effect_ui == "SHRINK":
            layout.prop(surface, "use_shrink")
            col = layout.column()
            col.active = surface.use_shrink
            col.prop(surface, "shrink_speed")
			

class PHYSICS_PT_dp_cache(PhysicButtonsPanel, bpy.types.Panel):
    bl_label = "Dynamic Paint: Cache"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(cls, context):
        md = context.dynamic_paint
        return md and (md.dynamicpaint_type == 'CANVAS') and \
        (md.canvas_settings.canvas_surfaces.active) and (md.canvas_settings.canvas_surfaces.active.uses_cache)

    def draw(self, context):
        layout = self.layout

        surface = context.dynamic_paint.canvas_settings.canvas_surfaces.active
        cache = surface.point_cache
        
        point_cache_ui(self, context, cache, (cache.is_baked is False), 'DYNAMIC_PAINT')


class PHYSICS_PT_dp_advanced_brush(PhysicButtonsPanel, bpy.types.Panel):
    bl_label = "Dynamic Paint: Advanced"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(cls, context):
        md = context.dynamic_paint
        return md and (md.dynamicpaint_type == 'BRUSH')

    def draw(self, context):
        layout = self.layout

        brush = context.dynamic_paint.brush_settings
        ob = context.object
		
        split = layout.split()
        col = split.column()
        col.prop(brush, "paint_source")

        if brush.paint_source == "PSYS":
            col.prop_search(brush, "psys", ob, "particle_systems", text="")
            if brush.psys:
                col.label(text="Particle effect:")
                sub = col.column()
                sub.active = not brush.use_part_radius
                sub.prop(brush, "solid_radius", text="Solid Radius")
                col.prop(brush, "use_part_radius", text="Use Particle's Radius")
                col.prop(brush, "smooth_radius", text="Smooth radius")

        elif brush.paint_source == "DISTANCE" or brush.paint_source == "VOLDIST":
            col.prop(brush, "paint_distance", text="Paint Distance")
            split = layout.row().split()
            sub = split.column()
            sub.prop(brush, "prox_facealigned")
            sub = split.column()
            sub.prop(brush, "prox_falloff", text="Falloff")
            if brush.paint_source == "VOLDIST":
                col = layout.row().column()
                col.prop(brush, "prox_inverse")
            if brush.prox_falloff == "RAMP":
                col = layout.row().column()
                col.separator()
                col.prop(brush, "prox_ramp_alpha", text="Only Use Alpha")
                col.template_color_ramp(brush, "paint_ramp", expand=True)

def register():
    bpy.utils.register_module(__name__)


def unregister():
    bpy.utils.register_module(__name__)

if __name__ == "__main__":
    register()
