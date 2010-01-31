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
from math import radians, pi
from rigify import RigifyError, ORG_PREFIX
from rigify_utils import bone_class_instance, copy_bone_simple, add_pole_target_bone, add_stretch_to, blend_bone_list, get_side_name, get_base_name
from rna_prop_ui import rna_idprop_ui_prop_get
from Mathutils import Vector

METARIG_NAMES = "shoulder", "arm", "forearm", "hand"


def metarig_template():
    # generated by rigify.write_meta_rig
    bpy.ops.object.mode_set(mode='EDIT')
    obj = bpy.context.active_object
    arm = obj.data
    bone = arm.edit_bones.new('shoulder')
    bone.head[:] = 0.0000, -0.0425, 0.0000
    bone.tail[:] = 0.0942, -0.0075, 0.0333
    bone.roll = -0.2227
    bone.connected = False
    bone = arm.edit_bones.new('upper_arm')
    bone.head[:] = 0.1066, -0.0076, -0.0010
    bone.tail[:] = 0.2855, 0.0206, -0.0104
    bone.roll = 1.6152
    bone.connected = False
    bone.parent = arm.edit_bones['shoulder']
    bone = arm.edit_bones.new('forearm')
    bone.head[:] = 0.2855, 0.0206, -0.0104
    bone.tail[:] = 0.4550, -0.0076, -0.0023
    bone.roll = 1.5153
    bone.connected = True
    bone.parent = arm.edit_bones['upper_arm']
    bone = arm.edit_bones.new('hand')
    bone.head[:] = 0.4550, -0.0076, -0.0023
    bone.tail[:] = 0.5423, -0.0146, -0.0131
    bone.roll = -3.0083
    bone.connected = True
    bone.parent = arm.edit_bones['forearm']

    bpy.ops.object.mode_set(mode='OBJECT')
    pbone = obj.pose.bones['upper_arm']
    pbone['type'] = 'arm_biped'


def metarig_definition(obj, orig_bone_name):
    mt = bone_class_instance(obj, METARIG_NAMES) # meta
    mt.arm = orig_bone_name
    mt.update()

    mt.shoulder_p = mt.arm_p.parent

    if not mt.shoulder_p:
        raise RigifyError("could not find '%s' parent, skipping:" % orig_bone_name)

    mt.shoulder = mt.shoulder_p.name

    # We could have some bones attached, find the bone that has this as its 2nd parent
    hands = []
    for pbone in obj.pose.bones:
        index = pbone.parent_index(mt.arm_p)
        if index == 2 and pbone.bone.connected and pbone.bone.parent.connected:
            hands.append(pbone)

    if len(hands) != 1:
        raise RigifyError("Found %s possible hands attached to this arm, expected 1 from bone: %s" % ([pbone.name for pbone in hands], orig_bone_name))

    # first add the 2 new bones
    mt.hand_p = hands[0]
    mt.hand = mt.hand_p.name

    mt.forearm_p = mt.hand_p.parent
    mt.forearm = mt.forearm_p.name

    return mt.names()


def ik(obj, definitions, base_names, options):

    arm = obj.data

    mt = bone_class_instance(obj, METARIG_NAMES)
    mt.shoulder, mt.arm, mt.forearm, mt.hand = definitions
    mt.update()

    ik = bone_class_instance(obj, ["pole", "pole_vis", "hand_vis"])
    ik_chain = mt.copy(to_fmt="MCH-%s_ik", base_names=base_names, exclude_attrs=["shoulder"])

    # IK needs no parent_index
    ik_chain.hand_e.connected = False
    ik_chain.hand_e.parent = None
    ik_chain.hand_e.local_location = False
    ik_chain.rename("hand", get_base_name(base_names[mt.hand]) + "_ik" + get_side_name(mt.hand))

    ik_chain.arm_e.connected = False
    ik_chain.arm_e.parent = mt.shoulder_e

    # Add the bone used for the arms poll target
    #ik.pole = add_pole_target_bone(obj, mt.forearm, get_base_name(base_names[mt.forearm]) + "_target" + get_side_name(mt.forearm), mode='ZAVERAGE')
    ik.pole = add_pole_target_bone(obj, mt.forearm, "elbow_target" + get_side_name(mt.forearm), mode='ZAVERAGE')

    ik.update()
    ik.pole_e.local_location = False

    # option: elbow_parent
    elbow_parent_name = options.get("elbow_parent", "")

    if elbow_parent_name:
        try:
            elbow_parent_e = arm.edit_bones[ORG_PREFIX + elbow_parent_name]
        except:
            # TODO, old/new parent mapping
            raise RigifyError("parent bone from property 'arm_biped_generic.elbow_parent' not found '%s'" % elbow_parent_name)
        ik.pole_e.parent = elbow_parent_e

    # update bones after this!
    ik.hand_vis = add_stretch_to(obj, mt.hand, ik_chain.hand, "VIS-%s_ik" % base_names[mt.hand])
    ik.pole_vis = add_stretch_to(obj, mt.forearm, ik.pole, "VIS-%s_ik" % base_names[mt.forearm])

    ik.update()
    ik.hand_vis_e.restrict_select = True
    ik.pole_vis_e.restrict_select = True

    bpy.ops.object.mode_set(mode='OBJECT')

    mt.update()
    ik.update()
    ik_chain.update()

    # Set IK dof
    ik_chain.forearm_p.ik_dof_x = True
    ik_chain.forearm_p.ik_dof_y = False
    ik_chain.forearm_p.ik_dof_z = False

    con = ik_chain.forearm_p.constraints.new('IK')
    con.target = obj
    con.subtarget = ik_chain.hand
    con.pole_target = obj
    con.pole_subtarget = ik.pole

    con.use_tail = True
    con.use_stretch = True
    con.use_target = True
    con.use_rotation = False
    con.chain_length = 2
    con.pole_angle = -pi/2

    # last step setup layers
    if "ik_layer" in options:
        layer = [n==options["ik_layer"] for n in range(0,32)]
    else:
        layer = list(mt.arm_b.layer)
    ik_chain.hand_b.layer = layer
    ik.hand_vis_b.layer   = layer
    ik.pole_b.layer       = layer
    ik.pole_vis_b.layer   = layer

    bpy.ops.object.mode_set(mode='EDIT')
    # don't blend the shoulder
    return [None] + ik_chain.names()


def fk(obj, definitions, base_names, options):

    arm = obj.data

    mt = bone_class_instance(obj, METARIG_NAMES)
    mt.shoulder, mt.arm, mt.forearm, mt.hand = definitions
    mt.update()

    ex = bone_class_instance(obj, ["socket", "hand_delta"])
    fk_chain = mt.copy(base_names=base_names)

    # shoulder is used as a hinge
    fk_chain.rename("shoulder", "MCH-%s_hinge" % base_names[mt.arm])
    fk_chain.shoulder_e.translate(Vector(0.0, fk_chain.shoulder_e.length / 2, 0.0))

    # upper arm constrains to this.
    ex.socket_e = copy_bone_simple(arm, mt.arm, "MCH-%s_socket" % base_names[mt.arm])
    ex.socket = ex.socket_e.name
    ex.socket_e.connected = False
    ex.socket_e.parent = mt.shoulder_e
    ex.socket_e.length *= 0.5

    # insert the 'MCH-delta_hand', between the forearm and the hand
    # copies forarm rotation
    ex.hand_delta_e = copy_bone_simple(arm, fk_chain.hand, "MCH-delta_%s" % base_names[mt.hand], parent=True)
    ex.hand_delta = ex.hand_delta_e.name
    ex.hand_delta_e.length *= 0.5
    ex.hand_delta_e.connected = False
    if "hand_roll" in options:
        ex.hand_delta_e.roll += radians(options["hand_roll"])

    fk_chain.hand_e.connected = False
    fk_chain.hand_e.parent = ex.hand_delta_e

    bpy.ops.object.mode_set(mode='OBJECT')

    mt.update()
    ex.update()
    fk_chain.update()

    # Set rotation modes and axis locks
    fk_chain.forearm_p.rotation_mode = 'XYZ'
    fk_chain.forearm_p.lock_rotation = (False, True, True)
    fk_chain.hand_p.rotation_mode = 'ZXY'
    fk_chain.arm_p.lock_location = True, True, True

    con = fk_chain.arm_p.constraints.new('COPY_LOCATION')
    con.target = obj
    con.subtarget = ex.socket

    fk_chain.hand_p.lock_location = True, True, True
    con = ex.hand_delta_p.constraints.new('COPY_ROTATION')
    con.target = obj
    con.subtarget = fk_chain.forearm

    def hinge_setup():
        # Hinge constraint & driver
        con = fk_chain.shoulder_p.constraints.new('COPY_ROTATION')
        con.name = "hinge"
        con.target = obj
        con.subtarget = mt.shoulder
        driver_fcurve = con.driver_add("influence", 0)
        driver = driver_fcurve.driver


        controller_path = fk_chain.arm_p.path_to_id()
        # add custom prop
        fk_chain.arm_p["hinge"] = 0.0
        prop = rna_idprop_ui_prop_get(fk_chain.arm_p, "hinge", create=True)
        prop["soft_min"] = 0.0
        prop["soft_max"] = 1.0


        # *****
        driver = driver_fcurve.driver
        driver.type = 'AVERAGE'

        var = driver.variables.new()
        var.name = "hinge"
        var.targets[0].id_type = 'OBJECT'
        var.targets[0].id = obj
        var.targets[0].data_path = controller_path + '["hinge"]'

        mod = driver_fcurve.modifiers[0]
        mod.poly_order = 1
        mod.coefficients[0] = 1.0
        mod.coefficients[1] = -1.0

    hinge_setup()

    # last step setup layers
    if "fk_layer" in options:
        layer = [n==options["fk_layer"] for n in range(0,32)]
    else:
        layer = list(mt.arm_b.layer)
    fk_chain.arm_b.layer     = layer
    fk_chain.forearm_b.layer = layer
    fk_chain.hand_b.layer    = layer

    # Forearm was getting wrong roll somehow.  Hack to fix that.
    bpy.ops.object.mode_set(mode='EDIT')
    fk_chain.update()
    mt.update()
    fk_chain.forearm_e.roll = mt.forearm_e.roll
    bpy.ops.object.mode_set(mode='OBJECT')

    bpy.ops.object.mode_set(mode='EDIT')
    return None, fk_chain.arm, fk_chain.forearm, fk_chain.hand


def deform(obj, definitions, base_names, options):
    bpy.ops.object.mode_set(mode='EDIT')

    # Create upper arm bones: two bones, each half of the upper arm.
    uarm1 = copy_bone_simple(obj.data, definitions[1], "DEF-%s.01" % base_names[definitions[1]], parent=True)
    uarm2 = copy_bone_simple(obj.data, definitions[1], "DEF-%s.02" % base_names[definitions[1]], parent=True)
    uarm1.connected = False
    uarm2.connected = False
    uarm2.parent = uarm1
    center = uarm1.center
    uarm1.tail = center
    uarm2.head = center

    # Create forearm bones: two bones, each half of the forearm.
    farm1 = copy_bone_simple(obj.data, definitions[2], "DEF-%s.01" % base_names[definitions[2]], parent=True)
    farm2 = copy_bone_simple(obj.data, definitions[2], "DEF-%s.02" % base_names[definitions[2]], parent=True)
    farm1.connected = False
    farm2.connected = False
    farm2.parent = farm1
    center = farm1.center
    farm1.tail = center
    farm2.head = center

    # Create twist bone
    twist = copy_bone_simple(obj.data, definitions[2], "MCH-arm_twist")
    twist.connected = False
    twist.parent = obj.data.edit_bones[definitions[3]]
    twist.length /= 2

    # Create hand bone
    hand = copy_bone_simple(obj.data, definitions[3], "DEF-%s" % base_names[definitions[3]], parent=True)

    # Store names before leaving edit mode
    uarm1_name = uarm1.name
    uarm2_name = uarm2.name
    farm1_name = farm1.name
    farm2_name = farm2.name
    twist_name = twist.name
    hand_name = hand.name

    # Leave edit mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # Get the pose bones
    uarm1 = obj.pose.bones[uarm1_name]
    uarm2 = obj.pose.bones[uarm2_name]
    farm1 = obj.pose.bones[farm1_name]
    farm2 = obj.pose.bones[farm2_name]
    twist = obj.pose.bones[twist_name]
    hand = obj.pose.bones[hand_name]

    # Upper arm constraints
    con = uarm1.constraints.new('DAMPED_TRACK')
    con.name = "trackto"
    con.target = obj
    con.subtarget = definitions[2]

    con = uarm1.constraints.new('COPY_SCALE')
    con.name = "trackto"
    con.target = obj
    con.subtarget = definitions[1]

    con = uarm2.constraints.new('COPY_ROTATION')
    con.name = "copy_rot"
    con.target = obj
    con.subtarget = definitions[1]

    # Forearm constraints
    con = farm1.constraints.new('COPY_ROTATION')
    con.name = "copy_rot"
    con.target = obj
    con.subtarget = definitions[2]

    con = farm1.constraints.new('COPY_SCALE')
    con.name = "copy_rot"
    con.target = obj
    con.subtarget = definitions[2]

    con = farm2.constraints.new('COPY_ROTATION')
    con.name = "copy_rot"
    con.target = obj
    con.subtarget = twist.name

    con = farm2.constraints.new('DAMPED_TRACK')
    con.name = "trackto"
    con.target = obj
    con.subtarget = definitions[3]

    # Hand constraint
    con = hand.constraints.new('COPY_ROTATION')
    con.name = "copy_rot"
    con.target = obj
    con.subtarget = definitions[3]

    bpy.ops.object.mode_set(mode='EDIT')
    return (uarm1_name, uarm2_name, farm1_name, farm2_name, hand_name)


def main(obj, bone_definition, base_names, options):
    bones_fk = fk(obj, bone_definition, base_names, options)
    bones_ik = ik(obj, bone_definition, base_names, options)
    bones_deform = deform(obj, bone_definition, base_names, options)

    bpy.ops.object.mode_set(mode='OBJECT')
    blend_bone_list(obj, bone_definition, bones_fk, bones_ik, target_bone=bones_ik[3], target_prop="ik", blend_default=0.0)
