/*
 * Copyright 2011, Blender Foundation.
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
 * Contributor: 
 *		Jeroen Bakker 
 *		Monique Dewanchand
 */

#include "COM_BlurNode.h"
#include "DNA_scene_types.h"
#include "DNA_node_types.h"
#include "COM_GaussianXBlurOperation.h"
#include "COM_GaussianYBlurOperation.h"
#include "COM_ExecutionSystem.h"
#include "COM_GaussianBokehBlurOperation.h"

BlurNode::BlurNode(bNode *editorNode): Node(editorNode) {
}

void BlurNode::convertToOperations(ExecutionSystem *graph, CompositorContext * context) {
    bNode* editorNode = this->getbNode();
	NodeBlurData * data = (NodeBlurData*)editorNode->storage;
	const bNodeSocket *sock = this->getInputSocket(1)->getbNodeSocket();
	const float size = ((const bNodeSocketValueFloat*)sock->default_value)->value;
	if (!data->bokeh) {
		GaussianXBlurOperation *operationx = new GaussianXBlurOperation();
		operationx->setData(data);
		operationx->setQuality(context->getQuality());
		this->getInputSocket(0)->relinkConnections(operationx->getInputSocket(0), true, 0, graph);
		operationx->setSize(size);
		graph->addOperation(operationx);
		GaussianYBlurOperation *operationy = new GaussianYBlurOperation();
		operationy->setData(data);
		operationy->setQuality(context->getQuality());
		this->getOutputSocket(0)->relinkConnections(operationy->getOutputSocket());
		operationy->setSize(size);
		graph->addOperation(operationy);
		addLink(graph, operationx->getOutputSocket(), operationy->getInputSocket(0));
		addPreviewOperation(graph, operationy->getOutputSocket(), 5);
	} else {
		GaussianBokehBlurOperation *operation = new GaussianBokehBlurOperation();
		operation->setData(data);
		this->getInputSocket(0)->relinkConnections(operation->getInputSocket(0), true, 0, graph);
		operation->setSize(size);
		operation->setQuality(context->getQuality());
		graph->addOperation(operation);
		this->getOutputSocket(0)->relinkConnections(operation->getOutputSocket());
		addPreviewOperation(graph, operation->getOutputSocket(), 5);
	}
}
