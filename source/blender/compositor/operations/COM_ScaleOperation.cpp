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

#include "COM_ScaleOperation.h"

ScaleOperation::ScaleOperation() : NodeOperation() {
    this->addInputSocket(COM_DT_COLOR);
    this->addInputSocket(COM_DT_VALUE);
    this->addInputSocket(COM_DT_VALUE);
    this->addOutputSocket(COM_DT_COLOR);
    this->setResolutionInputSocketIndex(0);
    this->inputOperation = NULL;
    this->inputXOperation = NULL;
    this->inputYOperation = NULL;
}
void ScaleOperation::initExecution() {
	this->inputOperation = this->getInputSocketReader(0);
	this->inputXOperation = this->getInputSocketReader(1);
	this->inputYOperation = this->getInputSocketReader(2);
    this->centerX = this->getWidth()/2.0;
    this->centerY = this->getHeight()/2.0;
}

void ScaleOperation::deinitExecution() {
    this->inputOperation = NULL;
    this->inputXOperation = NULL;
    this->inputYOperation = NULL;
}


void ScaleOperation::executePixel(float *color,float x, float y, MemoryBuffer *inputBuffers[]) {
	float scaleX[4];
	float scaleY[4];

	this->inputXOperation->read(scaleX, x, y, inputBuffers);
	this->inputYOperation->read(scaleY, x, y, inputBuffers);

	const float scx = scaleX[0];
	const float scy = scaleY[0];

	float nx = this->centerX+ (x - this->centerX) / scx;
	float ny = this->centerY+ (y - this->centerY) / scy;
	this->inputOperation->read(color, nx, ny, inputBuffers);
}

bool ScaleOperation::determineDependingAreaOfInterest(rcti *input, ReadBufferOperation *readOperation, rcti *output) {
    rcti newInput;
	float scaleX[4];
	float scaleY[4];

	this->inputXOperation->read(scaleX, 0, 0, NULL);
	this->inputYOperation->read(scaleY, 0, 0, NULL);

	const float scx = scaleX[0];
	const float scy = scaleY[0];

	newInput.xmax = this->centerX+ (input->xmax - this->centerX) / scx;
	newInput.xmin = this->centerX+ (input->xmin - this->centerX) / scx;
	newInput.ymax = this->centerY+ (input->ymax - this->centerY) / scy;
	newInput.ymin = this->centerY+ (input->ymin - this->centerY) / scy;

    return NodeOperation::determineDependingAreaOfInterest(&newInput, readOperation, output);
}

