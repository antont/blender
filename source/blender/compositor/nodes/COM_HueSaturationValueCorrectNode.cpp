#include "COM_HueSaturationValueCorrectNode.h"

#include "COM_ConvertColourToValueProg.h"
#include "COM_ExecutionSystem.h"
#include "COM_ConvertRGBToHSVOperation.h"
#include "COM_ConvertHSVToRGBOperation.h"
#include "COM_MixBlendOperation.h"
#include "COM_SetColorOperation.h"
#include "COM_SetValueOperation.h"
#include "COM_ChangeHSVOperation.h"
#include "DNA_node_types.h"
#include "COM_HueSaturationValueCorrectOperation.h"

HueSaturationValueCorrectNode::HueSaturationValueCorrectNode(bNode *editorNode): Node(editorNode) {
}

void HueSaturationValueCorrectNode::convertToOperations(ExecutionSystem *graph, CompositorContext * context) {
    InputSocket *valueSocket = this->getInputSocket(0);
    InputSocket *colourSocket = this->getInputSocket(1);
    OutputSocket *outputSocket = this->getOutputSocket(0);
    bNode* editorsnode = getbNode();
	CurveMapping *storage = (CurveMapping*)editorsnode->storage;

    if (colourSocket->isConnected() && outputSocket->isConnected()) {
        ConvertRGBToHSVOperation * rgbToHSV = new ConvertRGBToHSVOperation();
        ConvertHSVToRGBOperation * hsvToRGB = new ConvertHSVToRGBOperation();
		HueSaturationValueCorrectOperation *changeHSV = new HueSaturationValueCorrectOperation();
        MixBlendOperation * blend = new MixBlendOperation();

        colourSocket->relinkConnections(rgbToHSV->getInputSocket(0), true, 0, graph);
        addLink(graph, rgbToHSV->getOutputSocket(), changeHSV->getInputSocket(0));
        addLink(graph, changeHSV->getOutputSocket(), hsvToRGB->getInputSocket(0));
        addLink(graph, hsvToRGB->getOutputSocket(), blend->getInputSocket(2));
        addLink(graph, rgbToHSV->getInputSocket(0)->getConnection()->getFromSocket(), blend->getInputSocket(1));
        valueSocket->relinkConnections(blend->getInputSocket(0), true, 0, graph);
        outputSocket->relinkConnections(blend->getOutputSocket());

		changeHSV->setCurveMapping(storage);

        blend->setResolutionInputSocketIndex(1);

        graph->addOperation(rgbToHSV);
        graph->addOperation(hsvToRGB);
        graph->addOperation(changeHSV);
        graph->addOperation(blend);
    }
}
