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
from rigify import RigifyError

# not used, defined for completeness
METARIG_NAMES = tuple()


def metarig_template():
    # generated by rigify.write_meta_rig
    bpy.ops.object.mode_set(mode='EDIT')
    obj = bpy.context.active_object
    arm = obj.data
    bone = arm.edit_bones.new('bonesker')
    bone.head[:] = 0.0000, 0.0000, 0.0000
    bone.tail[:] = -0.0000, 0.7382, 0.1895
    bone.roll = -0.0000
    bone.use_connect = False
    bone = arm.edit_bones.new('delta')
    bone.head[:] = -0.0497, 0.8414, 0.3530
    bone.tail[:] = -0.2511, 1.1588, 0.9653
    bone.roll = 2.6044
    bone.use_connect = False
    bone.parent = arm.edit_bones['bonesker']
    bone = arm.edit_bones.new('boney')
    bone.head[:] = 0.7940, 2.5592, 0.4134
    bone.tail[:] = 0.7940, 3.3975, 0.4890
    bone.roll = 3.1416
    bone.use_connect = False
    bone.parent = arm.edit_bones['delta']

    bpy.ops.object.mode_set(mode='OBJECT')
    pbone = obj.pose.bones['delta']
    pbone['type'] = 'delta'


def metarig_definition(obj, orig_bone_name):
    '''
    The bone given is the head, its parent is the body,
    # its only child the first of a chain with matching basenames.
    eg.
        body -> head -> neck_01 -> neck_02 -> neck_03.... etc
    '''
    arm = obj.data
    delta = arm.bones[orig_bone_name]
    children = delta.children

    if len(children) != 1:
        raise RigifyError("only 1 child supported for delta on bone '%s'" % delta.name)

    if delta.use_connect:
        raise RigifyError("bone cannot be connected to its parent '%s'" % delta.name)

    bone_definition = [delta.name, children[0].name]

    return bone_definition


def main(obj, bone_definition, base_names, options):
    '''
    Use this bone to define a delta thats applied to its child in pose mode.
    '''
    mode_orig = obj.mode
    bpy.ops.object.mode_set(mode='OBJECT')

    delta_name, child_name = bone_definition

    delta_pbone = obj.pose.bones[delta_name]

    arm = obj.data
    child_pbone = obj.pose.bones[child_name]

    delta_phead = delta_pbone.head.copy()
    delta_ptail = delta_pbone.tail.copy()
    delta_pmatrix = delta_pbone.matrix.copy()

    child_phead = child_pbone.head.copy()
    child_ptail = child_pbone.tail.copy()
    child_pmatrix = child_pbone.matrix.copy()


    children = delta_pbone.children

    bpy.ops.object.mode_set(mode='EDIT')

    delta_ebone = arm.edit_bones[delta_name]
    child_ebone = arm.edit_bones[child_name]

    delta_head = delta_ebone.head.copy()
    delta_tail = delta_ebone.tail.copy()

    child_head = child_ebone.head.copy()
    child_tail = child_ebone.tail.copy()

    #arm.edit_bones.remove(delta_ebone)
    #del delta_ebone # cant use this
    del child_pbone

    bpy.ops.object.mode_set(mode='OBJECT')


    # Move the child bone to the deltas location
    obj.animation_data_create()
    delta_pbone = obj.pose.bones[delta_name]
    # child_pbone = obj.pose.bones[child_name]

    # ------------------- drivers

    delta_pbone.rotation_mode = 'XYZ'

    rot = delta_pmatrix.invert().rotation_part() * child_pmatrix.rotation_part()
    rot = rot.invert().to_euler()

    fcurve_drivers = delta_pbone.driver_add("rotation_euler", -1)
    for i, fcurve_driver in enumerate(fcurve_drivers):
        driver = fcurve_driver.driver
        driver.type = 'AVERAGE'
        #mod = fcurve_driver.modifiers.new('GENERATOR')
        mod = fcurve_driver.modifiers[0]
        mod.poly_order = 1
        mod.coefficients[0] = rot[i]
        mod.coefficients[1] = 0.0

    # tricky, find the transform to drive the bone to this location.
    delta_head_offset = child_pmatrix.rotation_part() * (delta_phead - child_phead)

    fcurve_drivers = delta_pbone.driver_add("location", -1)
    for i, fcurve_driver in enumerate(fcurve_drivers):
        driver = fcurve_driver.driver
        driver.type = 'AVERAGE'
        #mod = fcurve_driver.modifiers.new('GENERATOR')
        mod = fcurve_driver.modifiers[0]
        mod.poly_order = 1
        mod.coefficients[0] = delta_head_offset[i]
        mod.coefficients[1] = 0.0


    bpy.ops.object.mode_set(mode='EDIT')

    bpy.ops.object.mode_set(mode=mode_orig)

    # no blendeing
    return None
