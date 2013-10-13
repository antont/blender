#!/usr/bin/env python3

# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# Contributor(s): Campbell Barton
#
# #**** END GPL LICENSE BLOCK #****

# <pep8 compliant>

# this script updates XML themes once new settings are added
#
#  ./blender.bin --background -noaudio --python source/tools/check_descriptions.py

import bpy

DUPLICATE_WHITELIST = (
    # operators
    ('ACTION_OT_clean', 'GRAPH_OT_clean'),
    ('ACTION_OT_clickselect', 'GRAPH_OT_clickselect'),
    ('ACTION_OT_copy', 'GRAPH_OT_copy'),
    ('ACTION_OT_delete', 'GRAPH_OT_delete'),
    ('ACTION_OT_duplicate', 'GRAPH_OT_duplicate'),
    ('ACTION_OT_duplicate_move', 'GRAPH_OT_duplicate_move'),
    ('ACTION_OT_extrapolation_type', 'GRAPH_OT_extrapolation_type'),
    ('ACTION_OT_handle_type', 'GRAPH_OT_handle_type'),
    ('ACTION_OT_interpolation_type', 'GRAPH_OT_interpolation_type'),
    ('ACTION_OT_keyframe_insert', 'GRAPH_OT_keyframe_insert'),
    ('ACTION_OT_mirror', 'GRAPH_OT_mirror'),
    ('ACTION_OT_paste', 'GRAPH_OT_paste'),
    ('ACTION_OT_sample', 'GRAPH_OT_sample'),
    ('ACTION_OT_select_all_toggle', 'GRAPH_OT_select_all_toggle'),
    ('ACTION_OT_select_border', 'GRAPH_OT_select_border'),
    ('ACTION_OT_select_column', 'GRAPH_OT_select_column'),
    ('ACTION_OT_select_leftright', 'GRAPH_OT_select_leftright'),
    ('ACTION_OT_select_less', 'GRAPH_OT_select_less'),
    ('ACTION_OT_select_linked', 'GRAPH_OT_select_linked'),
    ('ACTION_OT_select_more', 'GRAPH_OT_select_more'),
    ('ACTION_OT_view_all', 'CLIP_OT_dopesheet_view_all', 'GRAPH_OT_view_all'),
    ('ANIM_OT_change_frame', 'CLIP_OT_change_frame'),
    ('ARMATURE_OT_armature_layers', 'POSE_OT_armature_layers'),
    ('ARMATURE_OT_autoside_names', 'POSE_OT_autoside_names'),
    ('ARMATURE_OT_bone_layers', 'POSE_OT_bone_layers'),
    ('ARMATURE_OT_extrude_forked', 'ARMATURE_OT_extrude_move'),
    ('ARMATURE_OT_select_all', 'POSE_OT_select_all'),
    ('ARMATURE_OT_select_hierarchy', 'POSE_OT_select_hierarchy'),
    ('ARMATURE_OT_select_linked', 'POSE_OT_select_linked'),
    ('CLIP_OT_cursor_set', 'UV_OT_cursor_set'),
    ('CLIP_OT_disable_markers', 'CLIP_OT_graph_disable_markers'),
    ('CLIP_OT_graph_select_border', 'MASK_OT_select_border'),
    ('CLIP_OT_view_ndof', 'IMAGE_OT_view_ndof'),
    ('CLIP_OT_view_pan', 'IMAGE_OT_view_pan', 'VIEW2D_OT_pan', 'VIEW3D_OT_view_pan'),
    ('CLIP_OT_view_zoom', 'VIEW2D_OT_zoom'),
    ('CLIP_OT_view_zoom_in', 'VIEW2D_OT_zoom_in'),
    ('CLIP_OT_view_zoom_out', 'VIEW2D_OT_zoom_out'),
    ('CONSOLE_OT_copy', 'FONT_OT_text_copy', 'TEXT_OT_copy'),
    ('CONSOLE_OT_delete', 'FONT_OT_delete', 'TEXT_OT_delete'),
    ('CONSOLE_OT_insert', 'FONT_OT_text_insert', 'TEXT_OT_insert'),
    ('CONSOLE_OT_paste', 'FONT_OT_text_paste', 'TEXT_OT_paste'),
    ('CURVE_OT_duplicate', 'MASK_OT_duplicate'),
    ('CURVE_OT_handle_type_set', 'MASK_OT_handle_type_set'),
    ('CURVE_OT_switch_direction', 'MASK_OT_switch_direction'),
    ('FONT_OT_line_break', 'TEXT_OT_line_break'),
    ('FONT_OT_move', 'TEXT_OT_move'),
    ('FONT_OT_move_select', 'TEXT_OT_move_select'),
    ('FONT_OT_text_cut', 'TEXT_OT_cut'),
    ('GRAPH_OT_properties', 'IMAGE_OT_properties', 'LOGIC_OT_properties', 'NLA_OT_properties'),
    ('LATTICE_OT_select_ungrouped', 'MESH_OT_select_ungrouped', 'PAINT_OT_vert_select_ungrouped'),
    ('NODE_OT_add_node', 'NODE_OT_add_search'),
    ('NODE_OT_move_detach_links', 'NODE_OT_move_detach_links_release'),
    ('NODE_OT_properties', 'VIEW3D_OT_properties'),
    ('NODE_OT_toolbar', 'VIEW3D_OT_toolshelf'),
    ('OBJECT_OT_duplicate_move', 'OBJECT_OT_duplicate_move_linked'),
    ('WM_OT_context_cycle_enum', 'WM_OT_context_toggle', 'WM_OT_context_toggle_enum'),
    ('WM_OT_context_set_boolean', 'WM_OT_context_set_enum', 'WM_OT_context_set_float', 'WM_OT_context_set_int', 'WM_OT_context_set_string', 'WM_OT_context_set_value'),
    )

DUPLICATE_IGNORE = {
    "",
    }


def check_duplicates():
    import rna_info

    DUPLICATE_IGNORE_FOUND = set()
    DUPLICATE_WHITELIST_FOUND = set()

    structs, funcs, ops, props = rna_info.BuildRNAInfo()

    # This is mainly useful for operators,
    # other types have too many false positives

    #for t in (structs, funcs, ops, props):
    for t in (ops, ):
        description_dict = {}
        print("")
        for k, v in t.items():
            if v.description not in DUPLICATE_IGNORE:
                id_str = ".".join([s if isinstance(s, str) else s.identifier for s in k if s])
                description_dict.setdefault(v.description, []).append(id_str)
            else:
                DUPLICATE_IGNORE_FOUND.add(v.description)
        # sort for easier viewing
        sort_ls = [(tuple(sorted(v)), k) for k, v in description_dict.items()]
        sort_ls.sort()

        for v, k in sort_ls:
            if len(v) > 1:
                if v not in DUPLICATE_WHITELIST:
                    print("found %d: %r, \"%s\"" % (len(v), v, k))
                    #print("%r," % (v,))
                else:
                    DUPLICATE_WHITELIST_FOUND.add(v)

    test = (DUPLICATE_IGNORE - DUPLICATE_IGNORE_FOUND)
    if test:
        print("Invalid 'DUPLICATE_IGNORE': %r" % test)
    test = (set(DUPLICATE_WHITELIST) - DUPLICATE_WHITELIST_FOUND)
    if test:
        print("Invalid 'DUPLICATE_WHITELIST': %r" % test)

def main():
    check_duplicates()

if __name__ == "__main__":
    main()
