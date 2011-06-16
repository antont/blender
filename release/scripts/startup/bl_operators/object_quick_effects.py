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

from mathutils import Vector
import bpy
from bpy.props import BoolProperty, EnumProperty, IntProperty, FloatProperty, FloatVectorProperty


def object_ensure_material(obj, mat_name):
    """ Use an existing material or add a new one.
    """
    mat = mat_slot = None
    for mat_slot in obj.material_slots:
        mat = mat_slot.material
        if mat:
            break
    if mat is None:
        mat = bpy.data.materials.new(mat_name)
        if mat_slot:
            mat_slot.material = mat
        else:
            obj.data.materials.append(mat)
    return mat


class QuickFur(bpy.types.Operator):
    bl_idname = "object.quick_fur"
    bl_label = "Quick Fur"
    bl_options = {'REGISTER', 'UNDO'}

    density = EnumProperty(items=(
                        ('LIGHT', "Light", ""),
                        ('MEDIUM', "Medium", ""),
                        ('HEAVY', "Heavy", "")),
                name="Fur Density",
                description="",
                default='MEDIUM')

    view_percentage = IntProperty(name="View %",
            default=10, min=1, max=100, soft_min=1, soft_max=100)

    length = FloatProperty(name="Length",
            default=0.1, min=0.001, max=100, soft_min=0.01, soft_max=10)

    def execute(self, context):
        fake_context = bpy.context.copy()
        mesh_objects = [obj for obj in context.selected_objects if obj.type == 'MESH']

        if not mesh_objects:
            self.report({'ERROR'}, "Select at least one mesh object.")
            return {'CANCELLED'}

        mat = bpy.data.materials.new("Fur Material")
        mat.strand.tip_size = 0.25
        mat.strand.blend_distance = 0.5

        for obj in mesh_objects:
            fake_context["object"] = obj
            bpy.ops.object.particle_system_add(fake_context)

            psys = obj.particle_systems[-1]
            psys.settings.type = 'HAIR'

            if self.density == 'LIGHT':
                psys.settings.count = 100
            elif self.density == 'MEDIUM':
                psys.settings.count = 1000
            elif self.density == 'HEAVY':
                psys.settings.count = 10000

            psys.settings.child_nbr = self.view_percentage
            psys.settings.hair_length = self.length
            psys.settings.use_strand_primitive = True
            psys.settings.use_hair_bspline = True
            psys.settings.child_type = 'INTERPOLATED'

            obj.data.materials.append(mat)
            obj.particle_systems[-1].settings.material = len(obj.data.materials)

        return {'FINISHED'}


class QuickExplode(bpy.types.Operator):
    bl_idname = "object.quick_explode"
    bl_label = "Quick Explode"
    bl_options = {'REGISTER', 'UNDO'}

    style = EnumProperty(items=(
                        ('EXPLODE', "Explode", ""),
                        ('BLEND', "Blend", "")),
                name="Explode Style",
                description="",
                default='EXPLODE')

    amount = IntProperty(name="Amount of pieces",
            default=100, min=2, max=10000, soft_min=2, soft_max=10000)

    frame_duration = IntProperty(name="Duration",
            default=50, min=1, max=300000, soft_min=1, soft_max=10000)

    frame_start = IntProperty(name="Start Frame",
            default=1, min=1, max=300000, soft_min=1, soft_max=10000)

    frame_end = IntProperty(name="End Frame",
            default=10, min=1, max=300000, soft_min=1, soft_max=10000)

    velocity = FloatProperty(name="Outwards Velocity",
            default=1, min=0, max=300000, soft_min=0, soft_max=10)

    fade = BoolProperty(name="Fade",
                description="Fade the pieces over time.",
                default=True)

    def execute(self, context):
        fake_context = bpy.context.copy()
        obj_act = context.active_object

        if obj_act.type != 'MESH':
            self.report({'ERROR'}, "Active object is not a mesh")
            return {'CANCELLED'}

        mesh_objects = [obj for obj in context.selected_objects
                        if obj.type == 'MESH' and obj != obj_act]
        mesh_objects.insert(0, obj_act)

        if self.style == 'BLEND' and len(mesh_objects) != 2:
            self.report({'ERROR'}, "Select two mesh objects")
            return {'CANCELLED'}
        elif not mesh_objects:
            self.report({'ERROR'}, "Select at least one mesh object")
            return {'CANCELLED'}

        for obj in mesh_objects:
            if obj.particle_systems:
                self.report({'ERROR'}, "Object %r already has a particle system" % obj.name)
                return {'CANCELLED'}

        if self.fade:
            tex = bpy.data.textures.new("Explode fade", 'BLEND')
            tex.use_color_ramp = True

            if self.style == 'BLEND':
                tex.color_ramp.elements[0].position = 0.333
                tex.color_ramp.elements[1].position = 0.666

            tex.color_ramp.elements[0].color[3] = 1.0
            tex.color_ramp.elements[1].color[3] = 0.0

        if self.style == 'BLEND':
            from_obj = mesh_objects[1]
            to_obj = mesh_objects[0]

        for obj in mesh_objects:
            fake_context["object"] = obj
            bpy.ops.object.particle_system_add(fake_context)

            settings = obj.particle_systems[-1].settings
            settings.count = self.amount
            settings.frame_start = self.frame_start
            settings.frame_end = self.frame_end - self.frame_duration
            settings.lifetime = self.frame_duration
            settings.normal_factor = self.velocity
            settings.render_type = 'NONE'

            explode = obj.modifiers.new(name='Explode', type='EXPLODE')
            explode.use_edge_cut = True

            if self.fade:
                explode.show_dead = False
                bpy.ops.mesh.uv_texture_add(fake_context)
                uv = obj.data.uv_textures[-1]
                uv.name = "Explode fade"
                explode.particle_uv = uv.name

                mat = object_ensure_material(obj, "Explode Fade")

                mat.use_transparency = True
                mat.use_transparent_shadows = True
                mat.alpha = 0.0
                mat.specular_alpha = 0.0

                tex_slot = mat.texture_slots.add()

                tex_slot.texture = tex
                tex_slot.texture_coords = 'UV'
                tex_slot.uv_layer = uv.name

                tex_slot.use_map_alpha = True

                if self.style == 'BLEND':
                    if obj == to_obj:
                        tex_slot.alpha_factor = -1.0
                        elem = tex.color_ramp.elements[1]
                        elem.color = mat.diffuse_color
                    else:
                        elem = tex.color_ramp.elements[0]
                        elem.color = mat.diffuse_color
                else:
                    tex_slot.use_map_color_diffuse = False

            if self.style == 'BLEND':
                settings.physics_type = 'KEYED'
                settings.use_emit_random = False
                settings.rotation_mode = 'NOR'

                psys = obj.particle_systems[-1]

                fake_context["particle_system"] = obj.particle_systems[-1]
                bpy.ops.particle.new_target(fake_context)
                bpy.ops.particle.new_target(fake_context)

                if obj == from_obj:
                    psys.targets[1].object = to_obj
                else:
                    psys.targets[0].object = from_obj
                    settings.normal_factor = -self.velocity
                    explode.show_unborn = False
                    explode.show_dead = True
            else:
                settings.factor_random = self.velocity
                settings.angular_velocity_factor = self.velocity / 10.0

        return {'FINISHED'}

    def invoke(self, context, event):
        self.frame_start = context.scene.frame_current
        self.frame_end = self.frame_start + self.frame_duration
        return self.execute(context)

def obj_bb_minmax(obj, min_co, max_co):
    for i in range(0, 8):
        bb_vec = Vector(obj.bound_box[i]) * obj.matrix_world

        min_co[0] = min(bb_vec[0], min_co[0])
        min_co[1] = min(bb_vec[1], min_co[1])
        min_co[2] = min(bb_vec[2], min_co[2])
        max_co[0] = max(bb_vec[0], max_co[0])
        max_co[1] = max(bb_vec[1], max_co[1])
        max_co[2] = max(bb_vec[2], max_co[2])


class QuickSmoke(bpy.types.Operator):
    bl_idname = "object.quick_smoke"
    bl_label = "Quick Smoke"
    bl_options = {'REGISTER', 'UNDO'}

    style = EnumProperty(items=(
                        ('STREAM', "Stream", ""),
                        ('PUFF', "Puff", ""),
                        ('FIRE', "Fire", "")),
                name="Smoke Style",
                description="",
                default='STREAM')

    show_flows = BoolProperty(name="Render Smoke Objects",
                description="Keep the smoke objects visible during rendering.",
                default=False)

    def execute(self, context):
        fake_context = bpy.context.copy()
        mesh_objects = [obj for obj in context.selected_objects if obj.type == 'MESH']
        min_co = Vector((100000.0, 100000.0, 100000.0))
        max_co = -min_co

        if not mesh_objects:
            self.report({'ERROR'}, "Select at least one mesh object.")
            return {'CANCELLED'}

        for obj in mesh_objects:
            fake_context["object"] = obj
            # make each selected object a smoke flow
            bpy.ops.object.modifier_add(fake_context, type='SMOKE')
            obj.modifiers[-1].smoke_type = 'FLOW'

            psys = obj.particle_systems[-1]
            if self.style == 'PUFF':
                psys.settings.frame_end = psys.settings.frame_start
                psys.settings.emit_from = 'VOLUME'
                psys.settings.distribution = 'RAND'
            elif self.style == 'FIRE':
                psys.settings.effector_weights.gravity = -1
                psys.settings.lifetime = 5
                psys.settings.count = 100000

                obj.modifiers[-2].flow_settings.initial_velocity = True
                obj.modifiers[-2].flow_settings.temperature = 2

            psys.settings.use_render_emitter = self.show_flows
            if not self.show_flows:
                obj.draw_type = 'WIRE'

            # store bounding box min/max for the domain object
            obj_bb_minmax(obj, min_co, max_co)

        # add the smoke domain object
        bpy.ops.mesh.primitive_cube_add()
        obj = context.active_object
        obj.name = "Smoke Domain"

        # give the smoke some room above the flows
        obj.location = 0.5 * (max_co + min_co) + Vector((0.0, 0.0, 1.0))
        obj.scale = 0.5 * (max_co - min_co) + Vector((1.0, 1.0, 2.0))

        # setup smoke domain
        bpy.ops.object.modifier_add(type='SMOKE')
        obj.modifiers[-1].smoke_type = 'DOMAIN'
        if self.style == 'FIRE':
            obj.modifiers[-1].domain_settings.use_dissolve_smoke = True
            obj.modifiers[-1].domain_settings.dissolve_speed = 20
            obj.modifiers[-1].domain_settings.use_high_resolution = True

        # create a volume material with a voxel data texture for the domain
        bpy.ops.object.material_slot_add()

        mat = bpy.data.materials.new("Smoke Domain Material")
        obj.material_slots[0].material = mat
        mat.type = 'VOLUME'
        mat.volume.density = 0
        mat.volume.density_scale = 5

        mat.texture_slots.add()
        mat.texture_slots[0].texture = bpy.data.textures.new("Smoke Density", 'VOXEL_DATA')
        mat.texture_slots[0].texture.voxel_data.domain_object = obj
        mat.texture_slots[0].use_map_color_emission = False
        mat.texture_slots[0].use_map_density = True

        # for fire add a second texture for emission and emission color
        if self.style == 'FIRE':
            mat.volume.emission = 5
            mat.texture_slots.add()
            mat.texture_slots[1].texture = bpy.data.textures.new("Smoke Heat", 'VOXEL_DATA')
            mat.texture_slots[1].texture.voxel_data.domain_object = obj
            mat.texture_slots[1].texture.use_color_ramp = True

            ramp = mat.texture_slots[1].texture.color_ramp

            elem = ramp.elements.new(0.333)
            elem.color[0] = elem.color[3] = 1
            elem.color[1] = elem.color[2] = 0

            elem = ramp.elements.new(0.666)
            elem.color[0] = elem.color[1] = elem.color[3] = 1
            elem.color[2] = 0

            mat.texture_slots[1].use_map_emission = True
            mat.texture_slots[1].blend_type = 'MULTIPLY'

        return {'FINISHED'}


class QuickFluid(bpy.types.Operator):
    bl_idname = "object.quick_fluid"
    bl_label = "Quick Fluid"
    bl_options = {'REGISTER', 'UNDO'}

    style = EnumProperty(items=(
                        ('INFLOW', "Inflow", ""),
                        ('BASIC', "Basic", "")),
                name="Fluid Style",
                description="",
                default='BASIC')

    initial_velocity = FloatVectorProperty(name="Initial Velocity",
        description="Initial velocity of the fluid",
        default=(0.0, 0.0, 0.0), min=-100.0, max=100.0, subtype='VELOCITY')

    show_flows = BoolProperty(name="Render Fluid Objects",
                description="Keep the fluid objects visible during rendering.",
                default=False)

    start_baking = BoolProperty(name="Start Fluid Bake",
                description="Start baking the fluid immediately after creating the domain object.",
                default=False)

    def execute(self, context):
        fake_context = bpy.context.copy()
        mesh_objects = [obj for obj in context.selected_objects if (obj.type == 'MESH' and not 0 in obj.dimensions)]
        min_co = Vector((100000, 100000, 100000))
        max_co = Vector((-100000, -100000, -100000))

        if not mesh_objects:
            self.report({'ERROR'}, "Select at least one mesh object.")
            return {'CANCELLED'}

        for obj in mesh_objects:
            fake_context["object"] = obj
            # make each selected object a fluid
            bpy.ops.object.modifier_add(fake_context, type='FLUID_SIMULATION')

            # fluid has to be before constructive modifiers, so it might not be the last modifier
            for mod in obj.modifiers:
                if mod.type == 'FLUID_SIMULATION':
                    break

            if self.style == 'INFLOW':
                mod.settings.type = 'INFLOW'
                mod.settings.inflow_velocity = self.initial_velocity.copy()
            else:
                mod.settings.type = 'FLUID'
                mod.settings.initial_velocity = self.initial_velocity.copy()

            obj.hide_render = not self.show_flows
            if not self.show_flows:
                obj.draw_type = 'WIRE'

            # store bounding box min/max for the domain object
            obj_bb_minmax(obj, min_co, max_co)

        # add the fluid domain object
        bpy.ops.mesh.primitive_cube_add()
        obj = context.active_object
        obj.name = "Fluid Domain"

        # give the fluid some room below the flows and scale with initial velocity
        v = 0.5 * self.initial_velocity
        obj.location = 0.5 * (max_co + min_co) + Vector((0.0, 0.0, -1.0)) + v
        obj.scale = 0.5 * (max_co - min_co) + Vector((1.0, 1.0, 2.0)) + Vector((abs(v[0]), abs(v[1]), abs(v[2])))

        # setup smoke domain
        bpy.ops.object.modifier_add(type='FLUID_SIMULATION')
        obj.modifiers[-1].settings.type = 'DOMAIN'

        # make the domain smooth so it renders nicely
        bpy.ops.object.shade_smooth()

        # create a ray-transparent material for the domain
        bpy.ops.object.material_slot_add()

        mat = bpy.data.materials.new("Fluid Domain Material")
        obj.material_slots[0].material = mat

        mat.specular_intensity = 1
        mat.specular_hardness = 100
        mat.use_transparency = True
        mat.alpha = 0.0
        mat.transparency_method = 'RAYTRACE'
        mat.raytrace_transparency.ior = 1.33
        mat.raytrace_transparency.depth = 4

        if self.start_baking:
            bpy.ops.fluid.bake()

        return {'FINISHED'}
