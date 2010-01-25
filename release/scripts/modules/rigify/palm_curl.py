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
#  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

import bpy
from rigify import get_layer_dict
from rigify_utils import copy_bone_simple, get_side_name
from rna_prop_ui import rna_idprop_ui_prop_get

# not used, defined for completeness
METARIG_NAMES = tuple()


def metarig_template():
    # generated by rigify.write_meta_rig
    bpy.ops.object.mode_set(mode='EDIT')
    obj = bpy.context.active_object
    arm = obj.data
    bone = arm.edit_bones.new('hand')
    bone.head[:] = 0.0004, -0.0629, 0.0000
    bone.tail[:] = 0.0021, -0.0209, 0.0000
    bone.roll = 0.0000
    bone.connected = False
    bone = arm.edit_bones.new('palm.03')
    bone.head[:] = -0.0000, 0.0000, 0.0000
    bone.tail[:] = 0.0025, 0.0644, -0.0065
    bone.roll = -3.1396
    bone.connected = False
    bone.parent = arm.edit_bones['hand']
    bone = arm.edit_bones.new('palm.02')
    bone.head[:] = 0.0252, -0.0000, 0.0000
    bone.tail[:] = 0.0324, 0.0627, -0.0065
    bone.roll = -3.1357
    bone.connected = False
    bone.parent = arm.edit_bones['hand']
    bone = arm.edit_bones.new('palm.01')
    bone.head[:] = 0.0504, 0.0000, 0.0000
    bone.tail[:] = 0.0703, 0.0508, -0.0065
    bone.roll = -3.1190
    bone.connected = False
    bone.parent = arm.edit_bones['hand']
    bone = arm.edit_bones.new('palm.04')
    bone.head[:] = -0.0252, 0.0000, 0.0000
    bone.tail[:] = -0.0286, 0.0606, -0.0065
    bone.roll = 3.1386
    bone.connected = False
    bone.parent = arm.edit_bones['hand']
    bone = arm.edit_bones.new('palm.05')
    bone.head[:] = -0.0504, 0.0000, 0.0000
    bone.tail[:] = -0.0669, 0.0534, -0.0065
    bone.roll = 3.1239
    bone.connected = False
    bone.parent = arm.edit_bones['hand']
    bone = arm.edit_bones.new('thumb')
    bone.head[:] = 0.0682, -0.0148, 0.0000
    bone.tail[:] = 0.1063, 0.0242, -0.0065
    bone.roll = -3.0929
    bone.connected = False
    bone.parent = arm.edit_bones['hand']

    bpy.ops.object.mode_set(mode='OBJECT')
    pbone = obj.pose.bones['palm.01']
    pbone['type'] = 'palm_curl'


def metarig_definition(obj, orig_bone_name):
    '''
    The bone given is the first in an array of siblings with a matching basename
    sorted with pointer first, little finger last.
    eg.
        [pointer, middle, ring, pinky... ] # any number of fingers
    '''
    arm = obj.data

    palm_bone = arm.bones[orig_bone_name]
    palm_parent = palm_bone.parent
    palm_base = palm_bone.basename
    bone_definition = [bone.name for bone in palm_parent.children if bone.basename == palm_base]
    bone_definition.sort()
    bone_definition.reverse()

    return [palm_parent.name] + bone_definition


def deform(obj, definitions, base_names, options):
    for org_bone_name in definitions[1:]:
        bpy.ops.object.mode_set(mode='EDIT')

        # Create deform bone.
        bone = copy_bone_simple(obj.data, org_bone_name, "DEF-%s" % base_names[org_bone_name], parent=True)
        
        # Store name before leaving edit mode
        bone_name = bone.name
        
        # Leave edit mode
        bpy.ops.object.mode_set(mode='OBJECT')
        
        # Get the pose bone
        bone = obj.pose.bones[bone_name]
        
        # Constrain to the original bone
        # XXX. Todo, is this needed if the bone is connected to its parent?
        con = bone.constraints.new('COPY_TRANSFORMS')
        con.name = "copy_loc"
        con.target = obj
        con.subtarget = org_bone_name


def main(obj, bone_definition, base_names, options):
    arm = obj.data

    children = bone_definition[1:]

    # Make a copy of the pinky
    # simply assume the pinky has the lowest name
    pinky_ebone = arm.edit_bones[children[0]]
    ring_ebone = arm.edit_bones[children[1]]

    # FIXME, why split the second one?
    base_name = base_names[pinky_ebone.name].rsplit('.', 2)[0]

    control_ebone = copy_bone_simple(arm, pinky_ebone.name, base_name + get_side_name(base_names[pinky_ebone.name]), parent=True)
    control_name = control_ebone.name

    offset = (pinky_ebone.head - ring_ebone.head)

    control_ebone.translate(offset)
    
    deform(obj, bone_definition, base_names, options)

    bpy.ops.object.mode_set(mode='OBJECT')

    arm = obj.data
    control_pbone = obj.pose.bones[control_name]
    pinky_pbone = obj.pose.bones[children[0]]

    control_pbone.rotation_mode = 'YZX'
    control_pbone.lock_rotation = False, True, True
    control_pbone.lock_location = True, True, True

    driver_fcurves = pinky_pbone.driver_add("rotation_euler")


    controller_path = control_pbone.path_to_id()

    # add custom prop
    control_pbone["spread"] = 0.0
    prop = rna_idprop_ui_prop_get(control_pbone, "spread", create=True)
    prop["soft_min"] = -1.0
    prop["soft_max"] = 1.0
    prop["min"] = -1.0
    prop["max"] = 1.0


    # *****
    driver = driver_fcurves[0].driver
    driver.type = 'AVERAGE'

    var = driver.variables.new()
    var.name = "x"
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = controller_path + ".rotation_euler[0]"


    # *****
    driver = driver_fcurves[1].driver
    driver.expression = "-x/4.0"

    var = driver.variables.new()
    var.name = "x"
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = controller_path + ".rotation_euler[0]"


    # *****
    driver = driver_fcurves[2].driver
    driver.expression = "(1.0-cos(x))-s"

    for fcurve in driver_fcurves:
        fcurve.modifiers.remove(0) # grr dont need a modifier

    var = driver.variables.new()
    var.name = "x"
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = controller_path + ".rotation_euler[0]"

    var = driver.variables.new()
    var.name = "s"
    var.targets[0].id_type = 'OBJECT'
    var.targets[0].id = obj
    var.targets[0].data_path = controller_path + '["spread"]'


    for i, child_name in enumerate(children):
        child_pbone = obj.pose.bones[child_name]
        child_pbone.rotation_mode = 'YZX'

        if child_name != children[-1] and child_name != children[0]:

            # this is somewhat arbitrary but seems to look good
            inf = i / (len(children) + 1)
            inf = 1.0 - inf
            inf = ((inf * inf) + inf) / 2.0

            # used for X/Y constraint
            inf_minor = inf * inf

            con = child_pbone.constraints.new('COPY_ROTATION')
            con.name = "Copy Z Rot"
            con.target = obj
            con.subtarget = children[0] # also pinky_pbone
            con.owner_space = con.target_space = 'LOCAL'
            con.use_x, con.use_y, con.use_z = False, False, True
            con.influence = inf

            con = child_pbone.constraints.new('COPY_ROTATION')
            con.name = "Copy XY Rot"
            con.target = obj
            con.subtarget = children[0] # also pinky_pbone
            con.owner_space = con.target_space = 'LOCAL'
            con.use_x, con.use_y, con.use_z = True, True, False
            con.influence = inf_minor


    child_pbone = obj.pose.bones[children[-1]]
    child_pbone.rotation_mode = 'QUATERNION'

    # fix at the end since there is some trouble with tx info not being updated otherwise
    def x_direction():
        # NOTE: the direction of the Z rotation depends on which side the palm is on.
        # we could do a simple side-of-x test but better to work out the direction
        # the hand is facing.
        from Mathutils import Vector
        from math import degrees
        child_pbone_01 = obj.pose.bones[children[0]].bone
        child_pbone_02 = obj.pose.bones[children[1]].bone

        rel_vec = child_pbone_01.head - child_pbone_02.head
        x_vec = child_pbone_01.matrix.rotation_part() * Vector(1.0, 0.0, 0.0)

        return degrees(rel_vec.angle(x_vec)) > 90.0

    if x_direction(): # flip
        driver.expression = "-(%s)" % driver.expression


    # last step setup layers
    arm.bones[control_name].layer = list(arm.bones[bone_definition[1]].layer)


    # no blending the result of this
    return None
