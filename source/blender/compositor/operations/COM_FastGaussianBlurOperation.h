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

#ifndef _COM_FastGaussianBlurOperation_h
#define _COM_FastGaussianBlurOperation_h

#include "COM_NodeOperation.h"
#include "DNA_node_types.h"

class FastGaussianBlurOperation: public NodeOperation {
private:
	SocketReader *inputOperation;
	NodeBlurData *data; 
	float size;
	float sx;
	float sy;
public:
	FastGaussianBlurOperation();
	bool determineDependingAreaOfInterest(rcti *input, ReadBufferOperation *readOperation, rcti *output);
	void executePixel(float *color, int x, int y, MemoryBuffer *inputBuffers[], void *data);
	
	void initExecution();
	void deinitExecution();
	void setdata(NodeBlurData *data){this->data = data;}
	void setSize(float size){this->size = size;} 
	void IIR_gauss(MemoryBuffer *src, float sigma, int channel, int xy);
	void* initializeTileData(rcti *rect, MemoryBuffer **memoryBuffers);
	void deinitializeTileData(rcti *rect, MemoryBuffer **memoryBuffers, void *data);
	
};
#endif

