#include "COM_MixOverlayOperation.h"
#include "COM_InputSocket.h"
#include "COM_OutputSocket.h"

MixOverlayOperation::MixOverlayOperation(): MixBaseOperation() {
}

void MixOverlayOperation::executePixel(float* outputValue, float x, float y, MemoryBuffer *inputBuffers[]) {
    float inputColor1[4];
    float inputColor2[4];
    float value;

	inputValueOperation->read(&value, x, y, inputBuffers);
	inputColor1Operation->read(&inputColor1[0], x, y, inputBuffers);
	inputColor2Operation->read(&inputColor2[0], x, y, inputBuffers);

    if (this->useValueAlphaMultiply()) {
        value *= inputColor2[3];
    }

    float valuem= 1.0f-value;

    if(inputColor1[0] < 0.5f) {
            outputValue[0] = inputColor1[0] * (valuem + 2.0f*value*inputColor2[0]);
    } else {
            outputValue[0] = 1.0f - (valuem + 2.0f*value*(1.0f - inputColor2[0])) * (1.0f - inputColor1[0]);
    }
    if(inputColor1[1] < 0.5f) {
            outputValue[1] = inputColor1[1] * (valuem + 2.0f*value*inputColor2[1]);
    } else {
            outputValue[1] = 1.0f - (valuem + 2.0f*value*(1.0f - inputColor2[1])) * (1.0f - inputColor1[1]);
    }
    if(inputColor1[2] < 0.5f) {
            outputValue[2] = inputColor1[2] * (valuem + 2.0f*value*inputColor2[2]);
    } else {
            outputValue[2] = 1.0f - (valuem + 2.0f*value*(1.0f - inputColor2[2])) * (1.0f - inputColor1[2]);
    }
    outputValue[3] = inputColor1[3];
}

