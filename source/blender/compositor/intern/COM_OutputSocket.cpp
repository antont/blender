#include "COM_Socket.h"
#include "COM_Node.h"
#include "COM_SocketConnection.h"
#include "COM_NodeOperation.h"
#include <stdio.h>

OutputSocket::OutputSocket(DataType datatype) :Socket(datatype) {
    this->groupInput = NULL;
    this->inputSocketDataTypeDeterminatorIndex = -1;
}
OutputSocket::OutputSocket(DataType datatype, int inputSocketDataTypeDeterminatorIndex) :Socket(datatype) {
    this->groupInput = NULL;
    this->inputSocketDataTypeDeterminatorIndex = inputSocketDataTypeDeterminatorIndex;
}

OutputSocket::OutputSocket(OutputSocket *from): Socket(from->getDataType()) {
	this->groupInput = NULL;
	this->inputSocketDataTypeDeterminatorIndex = from->getInputSocketDataTypeDeterminatorIndex();	
}

OutputSocket::~OutputSocket() {
    if (this->groupInput != NULL && !this->isInsideOfGroupNode()) {
        delete this->groupInput;
    }
    this->groupInput = NULL;
}

int OutputSocket::isOutputSocket() const { return true; }
const int OutputSocket::isConnected() const { return this->connections.size()!=0; }

void OutputSocket::determineResolution(unsigned int resolution[], unsigned int preferredResolution[]) {
    NodeBase* node = this->getNode();
    if (node->isOperation()) {
        NodeOperation* operation = (NodeOperation*)node;
        operation->determineResolution(resolution, preferredResolution);
        operation->setResolution(resolution);
    } else {
        printf("ERROR is not an operation\n");
    }
}

void OutputSocket::determineActualDataType() {
	DataType actualDatatype = this->getNode()->determineActualDataType(this);

	/** @todo: set the channel info needs to be moved after integration with OCIO */
	this->channelinfo[0].setNumber(0);
	this->channelinfo[1].setNumber(1);
	this->channelinfo[2].setNumber(2);
	this->channelinfo[3].setNumber(3);
	switch (actualDatatype) {
	case COM_DT_VALUE:
		this->channelinfo[0].setType(COM_CT_Value);
		break;
	case COM_DT_VECTOR:
		this->channelinfo[0].setType(COM_CT_X);
		this->channelinfo[1].setType(COM_CT_Y);
		this->channelinfo[2].setType(COM_CT_Z);
		break;
	case COM_DT_COLOR:
		this->channelinfo[0].setType(COM_CT_ColorComponent);
		this->channelinfo[1].setType(COM_CT_ColorComponent);
		this->channelinfo[2].setType(COM_CT_ColorComponent);
		this->channelinfo[3].setType(COM_CT_Alpha);
		break;
	default:
		break;
	}

    this->setActualDataType(actualDatatype);
    this->fireActualDataType();
}

void OutputSocket::addConnection(SocketConnection *connection) {
    this->connections.push_back(connection);
}

void OutputSocket::fireActualDataType() {
    unsigned int index;
    for (index = 0 ; index < this->connections.size();index ++) {
        SocketConnection *connection = this->connections[index];
        connection->getToSocket()->notifyActualInputType(this->getActualDataType());
    }
}
void OutputSocket::relinkConnections(OutputSocket *relinkToSocket, bool single) {
	if (isConnected()) {
		if (single) {
			SocketConnection *connection = this->connections[0];
			connection->setFromSocket(relinkToSocket);
			relinkToSocket->addConnection(connection);
			relinkToSocket->setActualDataType(this->getActualDataType());
			this->connections.erase(this->connections.begin());
		} else {
			unsigned int index;
			for (index = 0 ; index < this->connections.size();index ++) {
				SocketConnection *connection = this->connections[index];
				connection->setFromSocket(relinkToSocket);
				relinkToSocket->addConnection(connection);
			}
			relinkToSocket->setActualDataType(this->getActualDataType());
			this->connections.clear();
		}
	}
}
void OutputSocket::removeFirstConnection() {
    SocketConnection *connection = this->connections[0];
    InputSocket* inputSocket = connection->getToSocket();
    if (inputSocket != NULL) {
        inputSocket->setConnection(NULL);
    }
    this->connections.erase(this->connections.begin());
}

void OutputSocket::clearConnections() {
    while (this->isConnected()) {
        removeFirstConnection();
    }
}

WriteBufferOperation* OutputSocket::findAttachedWriteBufferOperation() const {
	unsigned int index;
	for (index = 0 ; index < this->connections.size();index++) {
		SocketConnection* connection = this->connections[index];
		NodeBase* node = connection->getToNode();
		if (node->isOperation()) {
			NodeOperation* operation = (NodeOperation*)node;
			if (operation->isWriteBufferOperation()) {
				return (WriteBufferOperation*)operation;
			}
		}
	}
	return NULL;
}

ChannelInfo* OutputSocket::getChannelInfo(const int channelnumber) {
	return &this->channelinfo[channelnumber];
}

