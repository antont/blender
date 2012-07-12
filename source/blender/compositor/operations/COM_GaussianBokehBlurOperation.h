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

#ifndef _COM_BokehGaussianBokehBlurOperation_h
#define _COM_GaussianBokehBlurOperation_h
#include "COM_NodeOperation.h"
#include "COM_BlurBaseOperation.h"
#include "COM_QualityStepHelper.h"

class GaussianBokehBlurOperation : public BlurBaseOperation {
private:
	float *m_gausstab;
	int m_radx, m_rady;
	void updateGauss(MemoryBuffer **memoryBuffers);

public:
	GaussianBokehBlurOperation();
	void initExecution();
	void *initializeTileData(rcti *rect, MemoryBuffer **memoryBuffers);
	/**
	 * the inner loop of this program
	 */
	void executePixel(float *color, int x, int y, MemoryBuffer * inputBuffers[], void *data);
	
	/**
	 * Deinitialize the execution
	 */
	void deinitExecution();
	
	bool determineDependingAreaOfInterest(rcti *input, ReadBufferOperation *readOperation, rcti *output);
};

class GaussianBokehBlurReferenceOperation : public NodeOperation, public QualityStepHelper {
private:
	SocketReader * m_inputImage;
	SocketReader * m_inputSize;
	float *m_gausstab;
	NodeBlurData *m_data;
	
	void updateGauss();

	static const int radxf = 256.0f;
	static const int radyf = 256.0f;
	static const int radx = 256;
	static const int rady = 256;
	
public:
	GaussianBokehBlurReferenceOperation();
	void initExecution();
	void *initializeTileData(rcti *rect, MemoryBuffer **memoryBuffers);
	/**
	 * the inner loop of this program
	 */
	void executePixel(float *color, int x, int y, MemoryBuffer * inputBuffers[], void *data);
	
	/**
	 * Deinitialize the execution
	 */
	void deinitExecution();
	
	bool determineDependingAreaOfInterest(rcti *input, ReadBufferOperation *readOperation, rcti *output);

	void setData(NodeBlurData *data) { this->m_data = data; }
};

#endif
