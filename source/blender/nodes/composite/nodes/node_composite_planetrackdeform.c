/*
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
 * The Original Code is Copyright (C) 2013 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Blender Foundation,
 *                 Sergey Sharybin
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/nodes/composite/nodes/node_composite_planetrackdeform.c
 *  \ingroup cmpnodes
 */


#include "node_composite_util.h"

static bNodeSocketTemplate cmp_node_planetrackdeform_in[] = {
	{   SOCK_RGBA,  1, N_("Image"), 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, PROP_NONE},
	{   SOCK_FLOAT, 1, N_("Mask"),  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, PROP_NONE},
	{   -1, 0, ""   }
};

static bNodeSocketTemplate cmp_node_planetrackdeform_out[] = {
	{	SOCK_RGBA,   0,  N_("Image")},
	{	SOCK_FLOAT,  0,  N_("Mask")},
	{	SOCK_FLOAT,  0,  N_("Plane")},
	{	-1, 0, ""	}
};

static void init(bNodeTree *UNUSED(ntree), bNode *node)
{
	NodePlaneTrackDeformData *data = MEM_callocN(sizeof(NodePlaneTrackDeformData), "node plane track deform data");

	node->storage = data;
}

void register_node_type_cmp_planetrackdeform(void)
{
	static bNodeType ntype;

	cmp_node_type_base(&ntype, CMP_NODE_PLANETRACKDEFORM, "Plane Track Deform", NODE_CLASS_DISTORT, 0);
	node_type_socket_templates(&ntype, cmp_node_planetrackdeform_in, cmp_node_planetrackdeform_out);
	node_type_init(&ntype, init);
	node_type_storage(&ntype, "NodePlaneTrackDeformData", node_free_standard_storage, node_copy_standard_storage);

	nodeRegisterType(&ntype);
}
