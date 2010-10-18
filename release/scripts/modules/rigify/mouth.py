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
from rna_prop_ui import rna_idprop_ui_prop_get
from math import acos, pi
from mathutils import Vector
from rigify import RigifyError
from rigify_utils import copy_bone_simple

#METARIG_NAMES = ("cpy",)
RIG_TYPE = "mouth"


def mark_actions():
    for action in bpy.data.actions:
        action.tag = True

def get_unmarked_action():
    for action in bpy.data.actions:
        if action.tag != True:
            return action
    return None

def add_action(name=None):
    mark_actions()
    bpy.ops.action.new()
    action = get_unmarked_action()
    if name is not None:
        action.name = name
    return action

def addget_shape_key(obj, name="Key"):
    """ Fetches a shape key, or creates it if it doesn't exist
    """
    # Create a shapekey set if it doesn't already exist
    if obj.data.shape_keys is None:
        shape = obj.add_shape_key(name="Basis", from_mix=False)
        obj.active_shape_key_index = 0

    # Get the shapekey, or create it if it doesn't already exist
    if name in obj.data.shape_keys.keys:
        shape_key = obj.data.shape_keys.keys[name]
    else:
        shape_key = obj.add_shape_key(name=name, from_mix=False)

    return shape_key


def addget_shape_key_driver(obj, name="Key"):
    """ Fetches the driver for the shape key, or creates it if it doesn't
        already exist.
    """
    driver_path = 'keys["' + name + '"].value'
    fcurve = None
    driver = None
    new = False
    if obj.data.shape_keys.animation_data is not None:
        for driver_s in obj.data.shape_keys.animation_data.drivers:
            if driver_s.data_path == driver_path:
                fcurve = driver_s
    if fcurve is None:
        fcurve = obj.data.shape_keys.keys[name].driver_add("value")
        fcurve.driver.type = 'AVERAGE'
        new = True

    return fcurve, new


def metarig_template():
    # generated by rigify.write_meta_rig
    bpy.ops.object.mode_set(mode='EDIT')
    obj = bpy.context.active_object
    arm = obj.data
    bone = arm.edit_bones.new('Bone')
    bone.head[:] = 0.0000, 0.0000, 0.0000
    bone.tail[:] = 0.0000, 0.0000, 1.0000
    bone.roll = 0.0000
    bone.use_connect = False

    bpy.ops.object.mode_set(mode='OBJECT')
    pbone = obj.pose.bones['Bone']
    pbone['type'] = 'copy'


def metarig_definition(obj, orig_bone_name):
    bone = obj.data.bones[orig_bone_name]
    chain = []

    try:
        chain += [bone.parent.parent.name, bone.parent.name, bone.name]
    except AttributeError:
        raise RigifyError("'%s' rig type requires a chain of two parents (bone: %s)" % (RIG_TYPE, orig_bone_name))

    chain += [child.name for child in bone.children_recursive_basename]

    if len(chain) < 10:
        raise RigifyError("'%s' rig type requires a chain of 8 bones (bone: %s)" % (RIG_TYPE, orig_bone_name))

    return chain[:10]


def deform(obj, definitions, base_names, options):
    bpy.ops.object.mode_set(mode='EDIT')

    eb = obj.data.edit_bones
    bb = obj.data.bones
    pb = obj.pose.bones

    jaw = definitions[1]

    # Options
    req_options = ["mesh"]
    for option in req_options:
        if option not in options:
            raise RigifyError("'%s' rig type requires a '%s' option (bone: %s)" % (RIG_TYPE, option, base_names[definitions[0]]))

    meshes = options["mesh"].replace(" ", "").split(",")

    # Lip DEF
    lip1 = copy_bone_simple(obj.data, definitions[2], "DEF-" + base_names[definitions[2]]).name
    lip2 = copy_bone_simple(obj.data, definitions[3], "DEF-" + base_names[definitions[3]]).name
    lip3 = copy_bone_simple(obj.data, definitions[4], "DEF-" + base_names[definitions[4]]).name
    lip4 = copy_bone_simple(obj.data, definitions[5], "DEF-" + base_names[definitions[5]]).name
    lip5 = copy_bone_simple(obj.data, definitions[6], "DEF-" + base_names[definitions[6]]).name
    lip6 = copy_bone_simple(obj.data, definitions[7], "DEF-" + base_names[definitions[7]]).name
    lip7 = copy_bone_simple(obj.data, definitions[8], "DEF-" + base_names[definitions[8]]).name
    lip8 = copy_bone_simple(obj.data, definitions[9], "DEF-" + base_names[definitions[9]]).name

    # Mouth corner spread bones (for driving corrective shape keys)
    spread_l_1 = copy_bone_simple(obj.data, definitions[6], "MCH-" + base_names[definitions[6]] + ".spread_1").name
    spread_l_2 = copy_bone_simple(obj.data, definitions[6], "MCH-" + base_names[definitions[6]] + ".spread_2").name
    eb[spread_l_1].tail = eb[definitions[5]].head
    eb[spread_l_2].tail = eb[definitions[5]].head
    eb[spread_l_1].roll = 0
    eb[spread_l_2].roll = 0
    eb[spread_l_1].use_connect = False
    eb[spread_l_2].use_connect = False
    eb[spread_l_1].parent = eb[definitions[6]]
    eb[spread_l_2].parent = eb[definitions[6]]

    spread_r_1 = copy_bone_simple(obj.data, definitions[2], "MCH-" + base_names[definitions[2]] + ".spread_1").name
    spread_r_2 = copy_bone_simple(obj.data, definitions[2], "MCH-" + base_names[definitions[2]] + ".spread_2").name
    eb[spread_r_1].tail = eb[definitions[3]].head
    eb[spread_r_2].tail = eb[definitions[3]].head
    eb[spread_r_1].roll = 0
    eb[spread_r_2].roll = 0
    eb[spread_r_1].use_connect = False
    eb[spread_r_2].use_connect = False
    eb[spread_r_1].parent = eb[definitions[2]]
    eb[spread_r_2].parent = eb[definitions[2]]



    # Jaw open bones (for driving corrective shape keys)
    jopen1 = copy_bone_simple(obj.data, jaw, "MCH-"+base_names[jaw]+".track1", parent=True).name
    eb[jopen1].use_connect = False
    eb[jopen1].head = eb[jaw].tail
    eb[jopen1].tail = eb[jopen1].head + Vector((0, 0, eb[jaw].length/4))

    jopen2 = copy_bone_simple(obj.data, jopen1, "MCH-"+base_names[jaw]+".track2").name
    eb[jopen2].parent = eb[jaw]


    bpy.ops.object.mode_set(mode='OBJECT')

    # Constrain DEF bones to ORG bones
    con = pb[lip1].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[2]

    con = pb[lip2].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[3]

    con = pb[lip3].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[4]

    con = pb[lip4].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[5]

    con = pb[lip5].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[6]

    con = pb[lip6].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[7]

    con = pb[lip7].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[8]

    con = pb[lip8].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = definitions[9]

    # Constraint mouth corner spread bones
    con = pb[spread_l_1].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lip4

    con = pb[spread_l_2].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = spread_l_1

    con = pb[spread_l_2].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lip6

    con = pb[spread_r_1].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lip2

    con = pb[spread_r_2].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = spread_r_1

    con = pb[spread_r_2].constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = lip8


    # Corrective shape keys for the corners of the mouth.
    bpy.ops.object.mode_set(mode='EDIT')

    # Calculate the rotation difference between the bones
    rotdiff_l = acos((eb[lip5].head - eb[lip4].head).normalize().dot((eb[lip5].head - eb[lip6].head).normalize()))
    rotdiff_r = acos((eb[lip1].head - eb[lip2].head).normalize().dot((eb[lip1].head - eb[lip8].head).normalize()))

    bpy.ops.object.mode_set(mode='OBJECT')


    # Left side shape key
    for mesh_name in meshes:
        mesh_obj = bpy.data.objects[mesh_name]
        shape_key_name = "COR-" + base_names[definitions[6]] + ".spread"

        # Add/get the shape key
        shape_key = addget_shape_key(mesh_obj, name=shape_key_name)

        # Add/get the shape key driver
        fcurve, is_new_driver = addget_shape_key_driver(mesh_obj, name=shape_key_name)
        driver = fcurve.driver

        # Get the variable, or create it if it doesn't already exist
        var_name = base_names[definitions[6]]
        if var_name in driver.variables:
            var = driver.variables[var_name]
        else:
            var = driver.variables.new()
            var.name = var_name

        # Set up the variable
        var.type = "ROTATION_DIFF"
        var.targets[0].id = obj
        var.targets[0].bone_target = spread_l_1
        var.targets[1].id = obj
        var.targets[1].bone_target = spread_l_2

        # Set fcurve offset
        if is_new_driver:
            mod = fcurve.modifiers[0]
            if rotdiff_l != pi:
                mod.coefficients[0] = -rotdiff_l / (pi-rotdiff_l)
                mod.coefficients[1] = 1 / (pi-rotdiff_l)

    # Right side shape key
    for mesh_name in meshes:
        mesh_obj = bpy.data.objects[mesh_name]
        shape_key_name = "COR-" + base_names[definitions[2]] + ".spread"

        # Add/get the shape key
        shape_key = addget_shape_key(mesh_obj, name=shape_key_name)

        # Add/get the shape key driver
        fcurve, is_new_driver = addget_shape_key_driver(mesh_obj, name=shape_key_name)
        driver = fcurve.driver

        # Get the variable, or create it if it doesn't already exist
        var_name = base_names[definitions[2]]
        if var_name in driver.variables:
            var = driver.variables[var_name]
        else:
            var = driver.variables.new()
            var.name = var_name

        # Set up the variable
        var.type = "ROTATION_DIFF"
        var.targets[0].id = obj
        var.targets[0].bone_target = spread_r_1
        var.targets[1].id = obj
        var.targets[1].bone_target = spread_r_2

        # Set fcurve offset
        if is_new_driver:
            mod = fcurve.modifiers[0]
            if rotdiff_r != pi:
                mod.coefficients[0] = -rotdiff_r / (pi-rotdiff_r)
                mod.coefficients[1] = 1 / (pi-rotdiff_r)

    # Jaw open corrective shape key
    for mesh_name in meshes:
        mesh_obj = bpy.data.objects[mesh_name]
        shape_key_name = "COR-" + base_names[definitions[4]] + ".jaw_open"

        # Add/get the shape key
        shape_key = addget_shape_key(mesh_obj, name=shape_key_name)

        # Add/get the shape key driver
        fcurve, is_new_driver = addget_shape_key_driver(mesh_obj, name=shape_key_name)
        driver = fcurve.driver

        # Get the variable, or create it if it doesn't already exist
        var_name = base_names[definitions[4]]
        if var_name in driver.variables:
            var = driver.variables[var_name]
        else:
            var = driver.variables.new()
            var.name = var_name

        # Set up the variable
        var.type = "LOC_DIFF"
        var.targets[0].id = obj
        var.targets[0].bone_target = jopen1
        var.targets[1].id = obj
        var.targets[1].bone_target = jopen2

        # Set fcurve offset
        if is_new_driver:
            mod = fcurve.modifiers[0]
            mod.coefficients[0] = 0.0
            mod.coefficients[1] = 1.0 / bb[jaw].length

    return (None,)




def control(obj, definitions, base_names, options):
    bpy.ops.object.mode_set(mode='EDIT')

    eb = obj.data.edit_bones
    bb = obj.data.bones
    pb = obj.pose.bones

    head_e = eb[definitions[0]]
    jaw_e = eb[definitions[1]]
    jaw = definitions[1]

    # Head lips
    hlip1 = copy_bone_simple(obj.data, definitions[2], "MCH-"+base_names[definitions[2]]+".head").name
    hlip2 = copy_bone_simple(obj.data, definitions[3], "MCH-"+base_names[definitions[3]]+".head").name
    hlip3 = copy_bone_simple(obj.data, definitions[4], "MCH-"+base_names[definitions[4]]+".head").name
    hlip4 = copy_bone_simple(obj.data, definitions[5], "MCH-"+base_names[definitions[5]]+".head").name
    hlip5 = copy_bone_simple(obj.data, definitions[6], "MCH-"+base_names[definitions[6]]+".head").name
    hlip6 = copy_bone_simple(obj.data, definitions[7], "MCH-"+base_names[definitions[7]]+".head").name
    hlip7 = copy_bone_simple(obj.data, definitions[8], "MCH-"+base_names[definitions[8]]+".head").name
    hlip8 = copy_bone_simple(obj.data, definitions[9], "MCH-"+base_names[definitions[9]]+".head").name

    eb[hlip1].parent = head_e
    eb[hlip2].parent = head_e
    eb[hlip3].parent = head_e
    eb[hlip4].parent = head_e
    eb[hlip5].parent = head_e
    eb[hlip6].parent = head_e
    eb[hlip7].parent = head_e
    eb[hlip8].parent = head_e

    # Jaw lips
    jlip1 = copy_bone_simple(obj.data, definitions[2], "MCH-"+base_names[definitions[2]]+".jaw").name
    jlip2 = copy_bone_simple(obj.data, definitions[3], "MCH-"+base_names[definitions[3]]+".jaw").name
    jlip3 = copy_bone_simple(obj.data, definitions[4], "MCH-"+base_names[definitions[4]]+".jaw").name
    jlip4 = copy_bone_simple(obj.data, definitions[5], "MCH-"+base_names[definitions[5]]+".jaw").name
    jlip5 = copy_bone_simple(obj.data, definitions[6], "MCH-"+base_names[definitions[6]]+".jaw").name
    jlip6 = copy_bone_simple(obj.data, definitions[7], "MCH-"+base_names[definitions[7]]+".jaw").name
    jlip7 = copy_bone_simple(obj.data, definitions[8], "MCH-"+base_names[definitions[8]]+".jaw").name
    jlip8 = copy_bone_simple(obj.data, definitions[9], "MCH-"+base_names[definitions[9]]+".jaw").name

    eb[jlip1].parent = jaw_e
    eb[jlip2].parent = jaw_e
    eb[jlip3].parent = jaw_e
    eb[jlip4].parent = jaw_e
    eb[jlip5].parent = jaw_e
    eb[jlip6].parent = jaw_e
    eb[jlip7].parent = jaw_e
    eb[jlip8].parent = jaw_e

    # Control lips
    lip1 = copy_bone_simple(obj.data, definitions[2], base_names[definitions[2]]).name
    lip2 = copy_bone_simple(obj.data, definitions[3], base_names[definitions[3]]).name
    lip3 = copy_bone_simple(obj.data, definitions[4], base_names[definitions[4]]).name
    lip4 = copy_bone_simple(obj.data, definitions[5], base_names[definitions[5]]).name
    lip5 = copy_bone_simple(obj.data, definitions[6], base_names[definitions[6]]).name
    lip6 = copy_bone_simple(obj.data, definitions[7], base_names[definitions[7]]).name
    lip7 = copy_bone_simple(obj.data, definitions[8], base_names[definitions[8]]).name
    lip8 = copy_bone_simple(obj.data, definitions[9], base_names[definitions[9]]).name

    eb[lip1].parent = eb[hlip1]
    eb[lip2].parent = eb[hlip2]
    eb[lip3].parent = eb[hlip3]
    eb[lip4].parent = eb[hlip4]
    eb[lip5].parent = eb[hlip5]
    eb[lip6].parent = eb[hlip6]
    eb[lip7].parent = eb[hlip7]
    eb[lip8].parent = eb[hlip8]

    # Jaw open tracker
    jopent = copy_bone_simple(obj.data, jaw_e.name, "MCH-"+base_names[jaw_e.name]+".track", parent=True).name
    eb[jopent].use_connect = False
    eb[jopent].tail = jaw_e.tail + Vector((0.0, 0.0, jaw_e.length))
    eb[jopent].head = jaw_e.tail

    bpy.ops.object.mode_set(mode='OBJECT')

    # Add mouth open action if it doesn't already exist
    action_name = "mouth_open"
    if action_name in bpy.data.actions:
        open_action = bpy.data.actions[action_name]
    else:
        open_action = add_action(name=action_name)

    # Add close property (useful when making the animation in the action)
    prop_name = "open_action"
    prop = rna_idprop_ui_prop_get(pb[lip1], prop_name, create=True)
    pb[lip1][prop_name] = 1.0
    prop["soft_min"] = 0.0
    prop["soft_max"] = 1.0
    prop["min"] = 0.0
    prop["max"] = 1.0

    open_driver_path = pb[lip1].path_from_id() + '["open_action"]'


    # Constraints

    # Jaw open tracker stretches to jaw tip
    con = pb[jopent].constraints.new('STRETCH_TO')
    con.target = obj
    con.subtarget = jaw
    con.head_tail = 1.0
    con.rest_length = bb[jopent].length
    con.volume = 'NO_VOLUME'

    # Head lips to jaw lips
    influence = [0.02, 0.1, 0.35, 0.25, 0.0]

    con = pb[hlip1].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip1
    con.influence = influence[2]

    con = pb[hlip2].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip2
    con.influence = influence[1]

    con = pb[hlip3].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip3
    con.influence = influence[0]

    con = pb[hlip4].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip4
    con.influence = influence[1]

    con = pb[hlip5].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip5
    con.influence = influence[2]

    con = pb[hlip6].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip6
    con.influence = 1.0 - influence[3]

    con = pb[hlip7].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip7
    con.influence = 1.0 - influence[4]

    con = pb[hlip8].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = jlip8
    con.influence = 1.0 - influence[3]

    # ORG bones to lips
    con = pb[definitions[2]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip1

    con = pb[definitions[3]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip2

    con = pb[definitions[4]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip3

    con = pb[definitions[5]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip4

    con = pb[definitions[6]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip5

    con = pb[definitions[7]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip6

    con = pb[definitions[8]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip7

    con = pb[definitions[9]].constraints.new('COPY_TRANSFORMS')
    con.target = obj
    con.subtarget = lip8

    # Action constraints for open mouth
    con = pb[lip1].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip2].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip3].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip4].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip5].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip6].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip7].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path

    con = pb[lip8].constraints.new('ACTION')
    con.target = obj
    con.subtarget = jopent
    con.action = open_action
    con.transform_channel = 'SCALE_Y'
    con.frame_start = 0
    con.frame_end = 60
    con.min = 0.0
    con.max = 1.0
    con.target_space = 'LOCAL'
    fcurve = con.driver_add("influence")
    driver = fcurve.driver
    driver.type = 'AVERAGE'
    var = driver.variables.new()
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = open_driver_path


    # Set layers
    layer = list(bb[definitions[2]].layers)
    bb[lip1].layers = layer
    bb[lip2].layers = layer
    bb[lip3].layers = layer
    bb[lip4].layers = layer
    bb[lip5].layers = layer
    bb[lip6].layers = layer
    bb[lip7].layers = layer
    bb[lip8].layers = layer


    return (None,)




def main(obj, bone_definition, base_names, options):
    # Create control rig
    control(obj, bone_definition, base_names, options)
    # Create deform rig
    deform(obj, bone_definition, base_names, options)

    return (None,)




def make_lip_stretch_bone(obj, name, bone1, bone2, roll_alpha):
    eb = obj.data.edit_bones
    pb = obj.pose.bones

    # Create the bone, pointing from bone1 to bone2
    bone_e = copy_bone_simple(obj.data, bone1, name, parent=True)
    bone_e.use_connect = False
    bone_e.tail = eb[bone2].head
    bone = bone_e.name

    # Align the bone roll with the average direction of bone1 and bone2
    vec = bone_e.y_axis.cross(((1.0-roll_alpha)*eb[bone1].y_axis) + (roll_alpha*eb[bone2].y_axis)).normalize()

    ang = acos(vec * bone_e.x_axis)

    bone_e.roll += ang
    c1 = vec * bone_e.x_axis
    bone_e.roll -= (ang*2)
    c2 = vec * bone_e.x_axis

    if c1 > c2:
        bone_e.roll += (ang*2)

    bpy.ops.object.mode_set(mode='OBJECT')
    bone_p = pb[bone]

    # Constrains
    con = bone_p.constraints.new('COPY_LOCATION')
    con.target = obj
    con.subtarget = bone1

    con = bone_p.constraints.new('DAMPED_TRACK')
    con.target = obj
    con.subtarget = bone2

    con = bone_p.constraints.new('STRETCH_TO')
    con.target = obj
    con.subtarget = bone2
    con.volume = 'NO_VOLUME'

    bpy.ops.object.mode_set(mode='EDIT')

    return bone
