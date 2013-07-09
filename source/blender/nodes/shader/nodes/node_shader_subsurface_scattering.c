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
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "../node_shader_util.h"

/* **************** OUTPUT ******************** */

static bNodeSocketTemplate sh_node_subsurface_scattering_in[] = {
	{	SOCK_RGBA, 1, N_("Color"),		0.8f, 0.8f, 0.8f, 1.0f, 0.0f, 1.0f},
	{	SOCK_FLOAT, 1, N_("Scale"),		1.0, 0.0f, 0.0f, 0.0f, 0.0f, 1000000.0f},
	{	SOCK_VECTOR, 1, N_("Radius"),	1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f},
	//{	SOCK_FLOAT, 1, N_("IOR"),		1.3f, 0.0f, 0.0f, 0.0f, 1.0f, 1000.0f},
	{	SOCK_VECTOR, 1, N_("Normal"),	0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, PROP_NONE, SOCK_HIDE_VALUE},
	{	-1, 0, ""	}
};

static bNodeSocketTemplate sh_node_subsurface_scattering_out[] = {
	{	SOCK_SHADER, 0, N_("BSSRDF")},
	{	-1, 0, ""	}
};

static int node_shader_gpu_subsurface_scattering(GPUMaterial *mat, bNode *UNUSED(node), bNodeExecData *UNUSED(execdata), GPUNodeStack *in, GPUNodeStack *out)
{
	if (!in[1].link)
		in[1].link = GPU_builtin(GPU_VIEW_NORMAL);

	return GPU_stack_link(mat, "node_subsurface_scattering", in, out);
}

/* node type definition */
void register_node_type_sh_subsurface_scattering(void)
{
	static bNodeType ntype;

	sh_node_type_base(&ntype, SH_NODE_SUBSURFACE_SCATTERING, "Subsurface Scattering", NODE_CLASS_SHADER, 0);
	node_type_compatibility(&ntype, NODE_NEW_SHADING);
	node_type_socket_templates(&ntype, sh_node_subsurface_scattering_in, sh_node_subsurface_scattering_out);
	node_type_size_preset(&ntype, NODE_SIZE_MIDDLE);
	node_type_init(&ntype, NULL);
	node_type_storage(&ntype, "", NULL, NULL);
	node_type_gpu(&ntype, node_shader_gpu_subsurface_scattering);

	nodeRegisterType(&ntype);
}

