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

#ifndef _COM_Node_h
#define _COM_Node_h

#include "COM_NodeBase.h"
#include "COM_InputSocket.h"
#include "COM_OutputSocket.h"
#include "COM_CompositorContext.h"
#include "DNA_node_types.h"
#include "BKE_text.h"
#include "COM_ExecutionSystem.h"
#include <vector>
#include <string>

using namespace std;

class Node;
class NodeOperation;
class ExecutionSystem;

typedef vector<Node *> NodeList;
typedef NodeList::iterator NodeIterator;
typedef pair<NodeIterator, NodeIterator> NodeRange;

/**
 * My node documentation.
 */
class Node : public NodeBase {
private:
	/**
	 * @brief stores the reference to the SDNA bNode struct
	 */
	bNode *m_editorNode;

	/**
	 * @brief Is this node part of the active group
	 */
	bool m_inActiveGroup;

public:
	Node(bNode *editorNode, bool create_sockets = true);
	
	/**
	 * @brief get the reference to the SDNA bNode struct
	 */
	bNode *getbNode();
	
	/**
	 * @brief Is this node in the active group (the group that is being edited)
	 * @param isInActiveGroup
	 */
	void setIsInActiveGroup(bool value) { this->m_inActiveGroup = value; }
	
	/**
	 * @brief Is this node part of the active group
	 * the active group is the group that is currently being edited. When no group is edited, 
	 * the active group will be the main tree (all nodes that are not part of a group will be active)
	 * @return bool [false:true]
	 */
	inline bool isInActiveGroup() { return this->m_inActiveGroup; }

	/**
	 * @brief convert node to operation
	 *
	 * @todo this must be described furter
	 *
	 * @param system the ExecutionSystem where the operations need to be added
	 * @param context reference to the CompositorContext
	 */
	virtual void convertToOperations(ExecutionSystem *system, CompositorContext *context) = 0;
	
	/**
	 * this method adds a SetValueOperation as input of the input socket.
	 * This can only be used from the convertToOperation method. all other usages are not allowed
	 */
	void addSetValueOperation(ExecutionSystem *graph, InputSocket *inputsocket, int editorNodeInputSocketIndex);
	
	/**
	 * this method adds a SetColorOperation as input of the input socket.
	 * This can only be used from the convertToOperation method. all other usages are not allowed
	 */
	void addSetColorOperation(ExecutionSystem *graph, InputSocket *inputsocket, int editorNodeInputSocketIndex);
	
	/**
	 * this method adds a SetVectorOperation as input of the input socket.
	 * This can only be used from the convertToOperation method. all other usages are not allowed
	 */
	void addSetVectorOperation(ExecutionSystem *graph, InputSocket *inputsocket, int editorNodeInputSocketIndex);
	
	/**
	 * Creates a new link between an outputSocket and inputSocket and registrates the link to the graph
	 * @return the new created link
	 */
	SocketConnection *addLink(ExecutionSystem *graph, OutputSocket *outputSocket, InputSocket *inputsocket);
	
	/**
	 * is this node a group node.
	 */
	virtual bool isGroupNode() const { return false; }
	/**
	 * is this node a proxy node.
	 */
	virtual bool isProxyNode() const { return false; }
	
	/**
	 * @brief find the InputSocket by bNodeSocket
	 *
	 * @param socket
	 */
	InputSocket *findInputSocketBybNodeSocket(bNodeSocket *socket);
	
	/**
	 * @brief find the OutputSocket by bNodeSocket
	 *
	 * @param socket
	 */
	OutputSocket *findOutputSocketBybNodeSocket(bNodeSocket *socket);
protected:
	
	Node();
	
	void addPreviewOperation(ExecutionSystem *system, InputSocket *inputSocket);
	void addPreviewOperation(ExecutionSystem *system, OutputSocket *outputSocket);
	
	bNodeSocket *getEditorInputSocket(int editorNodeInputSocketIndex);
	bNodeSocket *getEditorOutputSocket(int editorNodeOutputSocketIndex);
private:
};

#endif
