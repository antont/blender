/** \file smoke/intern/FLUID_3D.cpp
 *  \ingroup smoke
 */
//////////////////////////////////////////////////////////////////////
// This file is part of Wavelet Turbulence.
//
// Wavelet Turbulence is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Wavelet Turbulence is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Wavelet Turbulence.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2008 Theodore Kim and Nils Thuerey
//
// FLUID_3D.cpp: implementation of the FLUID_3D class.
//
//////////////////////////////////////////////////////////////////////
// Heavy parallel optimization done. Many of the old functions now
// take begin and end parameters and process only specified part of the data.
// Some functions were divided into multiple ones.
//		- MiikaH
//////////////////////////////////////////////////////////////////////

#include "FLUID_3D.h"
#include "IMAGE.h"
#include <INTERPOLATE.h>
#include "SPHERE.h"
#include <zlib.h>

#include "float.h"

#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>


#if PARALLEL==1
#include <omp.h>
#endif // PARALLEL 


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FLUID_3D::FLUID_3D(int *res, float *p0, float dtdef) :
	_xRes(res[0]), _yRes(res[1]), _zRes(res[2]), _res(0.0f)
{
	// set simulation consts
	_dt = dtdef;	// just in case. set in step from a RNA factor
	
	// start point of array
	_p0[0] = p0[0];
	_p0[1] = p0[1];
	_p0[2] = p0[2];

	_iterations = 100;
	_tempAmb = 0; 
	_heatDiffusion = 1e-3;
	_totalTime = 0.0f;
	_totalSteps = 0;
	_res = Vec3Int(_xRes,_yRes,_zRes);
	_maxRes = MAX3(_xRes, _yRes, _zRes);
	
	// initialize wavelet turbulence
	/*
	if(amplify)
		_wTurbulence = new WTURBULENCE(_res[0],_res[1],_res[2], amplify, noisetype);
	else
		_wTurbulence = NULL;
	*/
	
	// scale the constants according to the refinement of the grid
	_dx = 1.0f / (float)_maxRes;
	_constantScaling = 64.0f / _maxRes;
	_constantScaling = (_constantScaling < 1.0f) ? 1.0f : _constantScaling;
	_vorticityEps = 2.0f / _constantScaling; // Just in case set a default value

	// allocate arrays
	_totalCells   = _xRes * _yRes * _zRes;
	_slabSize = _xRes * _yRes;
	_xVelocity    = new float[_totalCells];
	_yVelocity    = new float[_totalCells];
	_zVelocity    = new float[_totalCells];
	_xVelocityOb  = new float[_totalCells];
	_yVelocityOb  = new float[_totalCells];
	_zVelocityOb  = new float[_totalCells];
	_xVelocityOld = new float[_totalCells];
	_yVelocityOld = new float[_totalCells];
	_zVelocityOld = new float[_totalCells];
	_xForce       = new float[_totalCells];
	_yForce       = new float[_totalCells];
	_zForce       = new float[_totalCells];
	_density      = new float[_totalCells];
	_densityOld   = new float[_totalCells];
	_heat         = new float[_totalCells];
	_heatOld      = new float[_totalCells];
	_obstacles    = new unsigned char[_totalCells]; // set 0 at end of step

	// For debugging purposes
	_pressure	  = new float[_totalCells];

	// For threaded version:
	_xVelocityTemp = new float[_totalCells];
	_yVelocityTemp = new float[_totalCells];
	_zVelocityTemp = new float[_totalCells];
	_densityTemp   = new float[_totalCells];
	_heatTemp      = new float[_totalCells];

	// DG TODO: check if alloc went fine

	for (int x = 0; x < _totalCells; x++)
	{
		_density[x]      = 0.0f;
		_densityOld[x]   = 0.0f;
		_heat[x]         = 0.0f;
		_heatOld[x]      = 0.0f;
		_xVelocity[x]    = 0.0f;
		_yVelocity[x]    = 0.0f;
		_zVelocity[x]    = 0.0f;
		_xVelocityOb[x]  = 0.0f;
		_yVelocityOb[x]  = 0.0f;
		_zVelocityOb[x]  = 0.0f;
		_xVelocityOld[x] = 0.0f;
		_yVelocityOld[x] = 0.0f;
		_zVelocityOld[x] = 0.0f;
		_xForce[x]       = 0.0f;
		_yForce[x]       = 0.0f;
		_zForce[x]       = 0.0f;
		_obstacles[x]    = false;
		_pressure[x]	 = 0.0f;
	}

	// boundary conditions of the fluid domain
	// set default values -> vertically non-colliding
	_domainBcFront = false;
	_domainBcTop = false;
	_domainBcLeft = false;
	_domainBcBack = _domainBcFront;
	_domainBcBottom = _domainBcTop;
	_domainBcRight	= _domainBcLeft;

	_colloPrev = 1;	// default value

	setBorderObstacles(); // walls

}

void FLUID_3D::setBorderObstacles()
{
	
	// set side obstacles
	unsigned int index;
	for (int y = 0; y < _yRes; y++)
	for (int x = 0; x < _xRes; x++)
	{
		// bottom slab
		index = x + y * _xRes;
		if(_domainBcBottom) _obstacles[index] = 1;

		// top slab
		index += _totalCells - _slabSize;
		if(_domainBcTop) _obstacles[index] = 1;
	}

	for (int z = 0; z < _zRes; z++)
	for (int x = 0; x < _xRes; x++)
	{
		// front slab
		index = x + z * _slabSize;
		if(_domainBcFront) _obstacles[index] = 1;

		// back slab
		index += _slabSize - _xRes;
		if(_domainBcBack) _obstacles[index] = 1;
	}

	for (int z = 0; z < _zRes; z++)
	for (int y = 0; y < _yRes; y++)
	{
		// left slab
		index = y * _xRes + z * _slabSize;
		if(_domainBcLeft) _obstacles[index] = 1;

		// right slab
		index += _xRes - 1;
		if(_domainBcRight) _obstacles[index] = 1;
	}
}

FLUID_3D::~FLUID_3D()
{
	if (_xVelocity) delete[] _xVelocity;
	if (_yVelocity) delete[] _yVelocity;
	if (_zVelocity) delete[] _zVelocity;
	if (_xVelocityOb) delete[] _xVelocityOb;
	if (_yVelocityOb) delete[] _yVelocityOb;
	if (_zVelocityOb) delete[] _zVelocityOb;
	if (_xVelocityOld) delete[] _xVelocityOld;
	if (_yVelocityOld) delete[] _yVelocityOld;
	if (_zVelocityOld) delete[] _zVelocityOld;
	if (_xForce) delete[] _xForce;
	if (_yForce) delete[] _yForce;
	if (_zForce) delete[] _zForce;
	if (_density) delete[] _density;
	if (_densityOld) delete[] _densityOld;
	if (_heat) delete[] _heat;
	if (_heatOld) delete[] _heatOld;
	if (_obstacles) delete[] _obstacles;
    // if (_wTurbulence) delete _wTurbulence;

	// for debugging purposes
	if(_pressure)
		delete[] _pressure;

	if (_xVelocityTemp) delete[] _xVelocityTemp;
	if (_yVelocityTemp) delete[] _yVelocityTemp;
	if (_zVelocityTemp) delete[] _zVelocityTemp;
	if (_densityTemp) delete[] _densityTemp;
	if (_heatTemp) delete[] _heatTemp;

    // printf("deleted fluid\n");
}

// init direct access functions from blender
void FLUID_3D::initBlenderRNA(float *alpha, float *beta, float *dt_factor, float *vorticity, int *borderCollision)
{
	_alpha = alpha;
	_beta = beta;
	_dtFactor = dt_factor;
	_vorticityRNA = vorticity;
	_borderColli = borderCollision;
}

//////////////////////////////////////////////////////////////////////
// step simulation once
//////////////////////////////////////////////////////////////////////
void FLUID_3D::step(float dt)
{
	// DG: TODO for the moment redo border for every timestep since it's been deleted every time by moving obstacles
	setBorderCollisions();


	// set delta time by dt_factor
	_dt = (*_dtFactor) * dt;
	// set vorticity from RNA value
	_vorticityEps = (*_vorticityRNA)/_constantScaling;

#if PARALLEL==1
	int threadval = 1;
	threadval = omp_get_max_threads();

	int stepParts = 1;
	float partSize = _zRes;

	stepParts = threadval*2;	// Dividing parallelized sections into numOfThreads * 2 sections
	partSize = (float)_zRes/stepParts;	// Size of one part;

  if (partSize < 4) {stepParts = threadval;					// If the slice gets too low (might actually slow things down, change it to larger
					partSize = (float)_zRes/stepParts;}
  if (partSize < 4) {stepParts = (int)(ceil((float)_zRes/4.0f));	// If it's still too low (only possible on future systems with +24 cores), change it to 4
					partSize = (float)_zRes/stepParts;}
#else
	int zBegin=0;
	int zEnd=_zRes;
#endif


#if PARALLEL==1
	#pragma omp parallel
	{
	#pragma omp for schedule(static,1)
	for (int i=0; i<stepParts; i++)
	{
		int zBegin = (int)((float)i*partSize + 0.5f);
		int zEnd = (int)((float)(i+1)*partSize + 0.5f);
#endif

		wipeBoundariesSL(zBegin, zEnd);
		addVorticity(zBegin, zEnd);
		addBuoyancy(_heat, _density, zBegin, zEnd);
		addForce(zBegin, zEnd);

#if PARALLEL==1
	}	// end of parallel
	#pragma omp barrier

	#pragma omp single
	{
#endif
	/*
	* addForce() changed Temp values to preserve thread safety
	* (previous functions in per thread loop still needed
	*  original velocity data)
	*
	* So swap temp values to velocity
	*/
	SWAP_POINTERS(_xVelocity, _xVelocityTemp);
	SWAP_POINTERS(_yVelocity, _yVelocityTemp);
	SWAP_POINTERS(_zVelocity, _zVelocityTemp);
#if PARALLEL==1
	}	// end of single

	#pragma omp barrier

	#pragma omp for
	for (int i=0; i<2; i++)
	{
		if (i==0)
		{
#endif
		project();
#if PARALLEL==1
		}
		else
		{
#endif
		diffuseHeat();
#if PARALLEL==1
		}
	}

	#pragma omp barrier

	#pragma omp single
	{
#endif
	/*
	* For thread safety use "Old" to read
	* "current" values but still allow changing values.
	*/
	SWAP_POINTERS(_xVelocity, _xVelocityOld);
	SWAP_POINTERS(_yVelocity, _yVelocityOld);
	SWAP_POINTERS(_zVelocity, _zVelocityOld);
	SWAP_POINTERS(_density, _densityOld);
	SWAP_POINTERS(_heat, _heatOld);

	advectMacCormackBegin(0, _zRes);

#if PARALLEL==1
	}	// end of single

	#pragma omp barrier


	#pragma omp for schedule(static,1)
	for (int i=0; i<stepParts; i++)
	{

		int zBegin = (int)((float)i*partSize + 0.5f);
		int zEnd = (int)((float)(i+1)*partSize + 0.5f);
#endif

	advectMacCormackEnd1(zBegin, zEnd);

#if PARALLEL==1
	}	// end of parallel

	#pragma omp barrier

	#pragma omp for schedule(static,1)
	for (int i=0; i<stepParts; i++)
	{

		int zBegin = (int)((float)i*partSize + 0.5f);
		int zEnd = (int)((float)(i+1)*partSize + 0.5f);
#endif

		advectMacCormackEnd2(zBegin, zEnd);

		artificialDampingSL(zBegin, zEnd);

		// Using forces as temp arrays

#if PARALLEL==1
	}
	}



	for (int i=1; i<stepParts; i++)
	{
		int zPos=(int)((float)i*partSize + 0.5f);
		
		artificialDampingExactSL(zPos);

	}
#endif

	/*
	* swap final velocity back to Velocity array
	* from temp xForce storage
	*/
	SWAP_POINTERS(_xVelocity, _xForce);
	SWAP_POINTERS(_yVelocity, _yForce);
	SWAP_POINTERS(_zVelocity, _zForce);




	_totalTime += _dt;
	_totalSteps++;		

	for (int i = 0; i < _totalCells; i++)
	{
		_xForce[i] = _yForce[i] = _zForce[i] = 0.0f;
	}

}


// Set border collision model from RNA setting

void FLUID_3D::setBorderCollisions() {


	_colloPrev = *_borderColli;		// saving the current value

	// boundary conditions of the fluid domain
	if (_colloPrev == 0)
	{
		// No collisions
		_domainBcFront = false;
		_domainBcTop = false;
		_domainBcLeft = false;
	}
	else if (_colloPrev == 2)
	{
		// Collide with all sides
		_domainBcFront = true;
		_domainBcTop = true;
		_domainBcLeft = true;
	}
	else
	{
		// Default values: Collide with "walls", but not top and bottom
		_domainBcFront = true;
		_domainBcTop = false;
		_domainBcLeft = true;
	}

	_domainBcBack = _domainBcFront;
	_domainBcBottom = _domainBcTop;
	_domainBcRight	= _domainBcLeft;



	// set side obstacles
	setBorderObstacles();
}

//////////////////////////////////////////////////////////////////////
// helper function to dampen co-located grid artifacts of given arrays in intervals
// (only needed for velocity, strength (w) depends on testcase...
//////////////////////////////////////////////////////////////////////


void FLUID_3D::artificialDampingSL(int zBegin, int zEnd) {
	const float w = 0.9;

	memmove(_xForce+(_slabSize*zBegin), _xVelocityTemp+(_slabSize*zBegin), sizeof(float)*_slabSize*(zEnd-zBegin));
	memmove(_yForce+(_slabSize*zBegin), _yVelocityTemp+(_slabSize*zBegin), sizeof(float)*_slabSize*(zEnd-zBegin));
	memmove(_zForce+(_slabSize*zBegin), _zVelocityTemp+(_slabSize*zBegin), sizeof(float)*_slabSize*(zEnd-zBegin));


	if(_totalSteps % 4 == 1) {
		for (int z = zBegin+1; z < zEnd-1; z++)
			for (int y = 1; y < _res[1]-1; y++)
				for (int x = 1+(y+z)%2; x < _res[0]-1; x+=2) {
					const int index = x + y*_res[0] + z * _slabSize;
					_xForce[index] = (1-w)*_xVelocityTemp[index] + 1./6. * w*(
							_xVelocityTemp[index+1] + _xVelocityTemp[index-1] +
							_xVelocityTemp[index+_res[0]] + _xVelocityTemp[index-_res[0]] +
							_xVelocityTemp[index+_slabSize] + _xVelocityTemp[index-_slabSize] );

					_yForce[index] = (1-w)*_yVelocityTemp[index] + 1./6. * w*(
							_yVelocityTemp[index+1] + _yVelocityTemp[index-1] +
							_yVelocityTemp[index+_res[0]] + _yVelocityTemp[index-_res[0]] +
							_yVelocityTemp[index+_slabSize] + _yVelocityTemp[index-_slabSize] );

					_zForce[index] = (1-w)*_zVelocityTemp[index] + 1./6. * w*(
							_zVelocityTemp[index+1] + _zVelocityTemp[index-1] +
							_zVelocityTemp[index+_res[0]] + _zVelocityTemp[index-_res[0]] +
							_zVelocityTemp[index+_slabSize] + _zVelocityTemp[index-_slabSize] );
				}
	}

	if(_totalSteps % 4 == 3) {
		for (int z = zBegin+1; z < zEnd-1; z++)
			for (int y = 1; y < _res[1]-1; y++)
				for (int x = 1+(y+z+1)%2; x < _res[0]-1; x+=2) {
					const int index = x + y*_res[0] + z * _slabSize;
					_xForce[index] = (1-w)*_xVelocityTemp[index] + 1./6. * w*(
							_xVelocityTemp[index+1] + _xVelocityTemp[index-1] +
							_xVelocityTemp[index+_res[0]] + _xVelocityTemp[index-_res[0]] +
							_xVelocityTemp[index+_slabSize] + _xVelocityTemp[index-_slabSize] );

					_yForce[index] = (1-w)*_yVelocityTemp[index] + 1./6. * w*(
							_yVelocityTemp[index+1] + _yVelocityTemp[index-1] +
							_yVelocityTemp[index+_res[0]] + _yVelocityTemp[index-_res[0]] +
							_yVelocityTemp[index+_slabSize] + _yVelocityTemp[index-_slabSize] );

					_zForce[index] = (1-w)*_zVelocityTemp[index] + 1./6. * w*(
							_zVelocityTemp[index+1] + _zVelocityTemp[index-1] +
							_zVelocityTemp[index+_res[0]] + _zVelocityTemp[index-_res[0]] +
							_zVelocityTemp[index+_slabSize] + _zVelocityTemp[index-_slabSize] );
				}

	}
}



void FLUID_3D::artificialDampingExactSL(int pos) {
	const float w = 0.9;
	int index, x,y,z;
	

	size_t posslab;

	for (z=pos-1; z<=pos; z++)
	{
	posslab=z * _slabSize;

	if(_totalSteps % 4 == 1) {
			for (y = 1; y < _res[1]-1; y++)
				for (x = 1+(y+z)%2; x < _res[0]-1; x+=2) {
					index = x + y*_res[0] + posslab;
					/*
					* Uses xForce as temporary storage to allow other threads to read
					* old values from xVelocityTemp
					*/
					_xForce[index] = (1-w)*_xVelocityTemp[index] + 1./6. * w*(
							_xVelocityTemp[index+1] + _xVelocityTemp[index-1] +
							_xVelocityTemp[index+_res[0]] + _xVelocityTemp[index-_res[0]] +
							_xVelocityTemp[index+_slabSize] + _xVelocityTemp[index-_slabSize] );

					_yForce[index] = (1-w)*_yVelocityTemp[index] + 1./6. * w*(
							_yVelocityTemp[index+1] + _yVelocityTemp[index-1] +
							_yVelocityTemp[index+_res[0]] + _yVelocityTemp[index-_res[0]] +
							_yVelocityTemp[index+_slabSize] + _yVelocityTemp[index-_slabSize] );

					_zForce[index] = (1-w)*_zVelocityTemp[index] + 1./6. * w*(
							_zVelocityTemp[index+1] + _zVelocityTemp[index-1] +
							_zVelocityTemp[index+_res[0]] + _zVelocityTemp[index-_res[0]] +
							_zVelocityTemp[index+_slabSize] + _zVelocityTemp[index-_slabSize] );
					
				}
	}

	if(_totalSteps % 4 == 3) {
			for (y = 1; y < _res[1]-1; y++)
				for (x = 1+(y+z+1)%2; x < _res[0]-1; x+=2) {
					index = x + y*_res[0] + posslab;

					/*
					* Uses xForce as temporary storage to allow other threads to read
					* old values from xVelocityTemp
					*/
					_xForce[index] = (1-w)*_xVelocityTemp[index] + 1./6. * w*(
							_xVelocityTemp[index+1] + _xVelocityTemp[index-1] +
							_xVelocityTemp[index+_res[0]] + _xVelocityTemp[index-_res[0]] +
							_xVelocityTemp[index+_slabSize] + _xVelocityTemp[index-_slabSize] );

					_yForce[index] = (1-w)*_yVelocityTemp[index] + 1./6. * w*(
							_yVelocityTemp[index+1] + _yVelocityTemp[index-1] +
							_yVelocityTemp[index+_res[0]] + _yVelocityTemp[index-_res[0]] +
							_yVelocityTemp[index+_slabSize] + _yVelocityTemp[index-_slabSize] );

					_zForce[index] = (1-w)*_zVelocityTemp[index] + 1./6. * w*(
							_zVelocityTemp[index+1] + _zVelocityTemp[index-1] +
							_zVelocityTemp[index+_res[0]] + _zVelocityTemp[index-_res[0]] +
							_zVelocityTemp[index+_slabSize] + _zVelocityTemp[index-_slabSize] );
					
				}

	}
	}
}

//////////////////////////////////////////////////////////////////////
// copy out the boundary in all directions
//////////////////////////////////////////////////////////////////////
void FLUID_3D::copyBorderAll(float* field, int zBegin, int zEnd)
{
	int index, x, y, z;
	int zSize = zEnd-zBegin;
	int _blockTotalCells=_slabSize * zSize;

	if (zBegin==0)
	for (int y = 0; y < _yRes; y++)
		for (int x = 0; x < _xRes; x++)
		{
			// front slab
			index = x + y * _xRes;
			field[index] = field[index + _slabSize];
    }

	if (zEnd==_zRes)
	for (y = 0; y < _yRes; y++)
		for (x = 0; x < _xRes; x++)
		{

			// back slab
			index = x + y * _xRes + _blockTotalCells - _slabSize;
			field[index] = field[index - _slabSize];
    }

	for (z = 0; z < zSize; z++)
		for (x = 0; x < _xRes; x++)
    {
			// bottom slab
			index = x + z * _slabSize;
			field[index] = field[index + _xRes];

			// top slab
			index += _slabSize - _xRes;
			field[index] = field[index - _xRes];
    }

	for (z = 0; z < zSize; z++)
		for (y = 0; y < _yRes; y++)
    {
			// left slab
			index = y * _xRes + z * _slabSize;
			field[index] = field[index + 1];

			// right slab
			index += _xRes - 1;
			field[index] = field[index - 1];
		}
}

//////////////////////////////////////////////////////////////////////
// wipe boundaries of velocity and density
//////////////////////////////////////////////////////////////////////
void FLUID_3D::wipeBoundaries(int zBegin, int zEnd)
{
	setZeroBorder(_xVelocity, _res, zBegin, zEnd);
	setZeroBorder(_yVelocity, _res, zBegin, zEnd);
	setZeroBorder(_zVelocity, _res, zBegin, zEnd);
	setZeroBorder(_density, _res, zBegin, zEnd);
}

void FLUID_3D::wipeBoundariesSL(int zBegin, int zEnd)
{
	
	/////////////////////////////////////
	// setZeroBorder to all:
	/////////////////////////////////////

	/////////////////////////////////////
	// setZeroX
	/////////////////////////////////////

	const int slabSize = _xRes * _yRes;
	int index, x,y,z;

	for (z = zBegin; z < zEnd; z++)
		for (y = 0; y < _yRes; y++)
		{
			// left slab
			index = y * _xRes + z * slabSize;
			_xVelocity[index] = 0.0f;
			_yVelocity[index] = 0.0f;
			_zVelocity[index] = 0.0f;
			_density[index] = 0.0f;

			// right slab
			index += _xRes - 1;
			_xVelocity[index] = 0.0f;
			_yVelocity[index] = 0.0f;
			_zVelocity[index] = 0.0f;
			_density[index] = 0.0f;
		}

	/////////////////////////////////////
	// setZeroY
	/////////////////////////////////////

	for (z = zBegin; z < zEnd; z++)
		for (x = 0; x < _xRes; x++)
		{
			// bottom slab
			index = x + z * slabSize;
			_xVelocity[index] = 0.0f;
			_yVelocity[index] = 0.0f;
			_zVelocity[index] = 0.0f;
			_density[index] = 0.0f;

			// top slab
			index += slabSize - _xRes;
			_xVelocity[index] = 0.0f;
			_yVelocity[index] = 0.0f;
			_zVelocity[index] = 0.0f;
			_density[index] = 0.0f;

		}

	/////////////////////////////////////
	// setZeroZ
	/////////////////////////////////////


	const int totalCells = _xRes * _yRes * _zRes;

	index = 0;
	if (zBegin == 0)
	for (y = 0; y < _yRes; y++)
		for (x = 0; x < _xRes; x++, index++)
		{
			// front slab
			_xVelocity[index] = 0.0f;
			_yVelocity[index] = 0.0f;
			_zVelocity[index] = 0.0f;
			_density[index] = 0.0f;
    }

	if (zEnd == _zRes)
	{
		index=0;
		int indexx=0;
		const int cellsslab = totalCells - slabSize;

		for (y = 0; y < _yRes; y++)
			for (x = 0; x < _xRes; x++, index++)
			{

				// back slab
				indexx = index + cellsslab;
				_xVelocity[indexx] = 0.0f;
				_yVelocity[indexx] = 0.0f;
				_zVelocity[indexx] = 0.0f;
				_density[indexx] = 0.0f;
			}
	}

}

//////////////////////////////////////////////////////////////////////
// add forces to velocity field
//////////////////////////////////////////////////////////////////////
void FLUID_3D::addForce(int zBegin, int zEnd)
{
	int begin=zBegin * _slabSize;
	int end=begin + (zEnd - zBegin) * _slabSize;

	for (int i = begin; i < end; i++)
	{
		_xVelocityTemp[i] = _xVelocity[i] + _dt * _xForce[i];
		_yVelocityTemp[i] = _yVelocity[i] + _dt * _yForce[i];
		_zVelocityTemp[i] = _zVelocity[i] + _dt * _zForce[i];
	}
}

//////////////////////////////////////////////////////////////////////
// project into divergence free field
//////////////////////////////////////////////////////////////////////
void FLUID_3D::project()
{
	int x, y, z;
	size_t index;

	VectorXf p(_totalCells);

	ArrayXd A0(_totalCells);
	ArrayXd Ai(_totalCells);
	ArrayXd Aj(_totalCells);
	ArrayXd Ak(_totalCells);

	A0.setZero(_totalCells);
	Ai.setZero(_totalCells);
	Aj.setZero(_totalCells);
	Ak.setZero(_totalCells);
	p.setZero(_totalCells);


	ArrayXd gti(_totalCells); // gridToIndex
	unsigned int linearIndex = 0;

	for (z = 0; z < _zRes; z++)
		for (y = 0; y < _yRes; y++)
			for (x = 0; x < _xRes; x++)
				if(!_obstacles[FINDEX(x, y, z)])
				{
					gti[FINDEX(x, y, z)] = linearIndex;
					linearIndex++;
				}


	VectorXf b(linearIndex);
	b.setZero(linearIndex);
	SparseMatrix<float,RowMajor> A(linearIndex, linearIndex);
	A.reserve(VectorXi::Constant(linearIndex, 7));

	// float *_pressure = new float[_totalCells];
	float *_divergence   = new float[_totalCells];

	memset(_pressure, 0, sizeof(float)*_totalCells);
	memset(_divergence, 0, sizeof(float)*_totalCells);
	
	// set VELOCITY and PRESSURE inside of obstacles cells to zero
	setObstacleBoundaries(_pressure, 0, _zRes);
	
	// copy out the boundaries
	if(!_domainBcLeft)  setNeumannX(_xVelocity, _res, 0, _zRes);
	else setZeroX(_xVelocity, _res, 0, _zRes); 

	if(!_domainBcFront)   setNeumannY(_yVelocity, _res, 0, _zRes);
	else setZeroY(_yVelocity, _res, 0, _zRes); 

	if(!_domainBcTop) setNeumannZ(_zVelocity, _res, 0, _zRes);
	else setZeroZ(_zVelocity, _res, 0, _zRes);

	// calculate divergence
	index = _slabSize + _xRes + 1;
	for (z = 1; z < _zRes - 1; z++, index += 2 * _xRes)
		for (y = 1; y < _yRes - 1; y++, index += 2)
			for (x = 1; x < _xRes - 1; x++, index++)
			{
				
				if(_obstacles[index])
				{
					_divergence[index] = 0.0f;
					continue;
				}
				
				float xright = _xVelocity[index + 1];
				float xleft  = _xVelocity[index - 1];
				float yup    = _yVelocity[index + _xRes];
				float ydown  = _yVelocity[index - _xRes];
				float ztop   = _zVelocity[index + _slabSize];
				float zbottom = _zVelocity[index - _slabSize];

				if(_obstacles[index+1]) xright = - _xVelocity[index]; // DG: +=
				if(_obstacles[index-1]) xleft  = - _xVelocity[index];
				if(_obstacles[index+_xRes]) yup    = - _yVelocity[index];
				if(_obstacles[index-_xRes]) ydown  = - _yVelocity[index];
				if(_obstacles[index+_slabSize]) ztop    = - _zVelocity[index];
				if(_obstacles[index-_slabSize]) zbottom = - _zVelocity[index];

				if(_obstacles[index+1])			xright	+= _xVelocityOb[index + 1];
				if(_obstacles[index-1])			xleft	+= _xVelocityOb[index - 1];
				if(_obstacles[index+_xRes])		yup		+= _yVelocityOb[index + _xRes];
				if(_obstacles[index-_xRes])		ydown	+= _yVelocityOb[index - _xRes];
				if(_obstacles[index+_slabSize]) ztop    += _zVelocityOb[index + _slabSize];
				if(_obstacles[index-_slabSize]) zbottom += _zVelocityOb[index - _slabSize];

				_divergence[index] = -_dx * 0.5f * (
						xright - xleft +
						yup - ydown +
						ztop - zbottom );

				// Pressure is zero anyway since now a local array is used
				_pressure[index] = 0.0f;
			}

	float scale = 1.0; // DG TODO: make this global and incooperate this into other functions
#if USE_NEW_CG == 1
	for (z = 0; z < _zRes; z++)
		for (y = 0; y < _yRes; y++)
			for (x = 0; x < _xRes; x++)
	{
				if(!_obstacles[FINDEX(x, y, z)])
				{
					// +x
					if(x < _xRes -1)
						{ if(!_obstacles[FINDEX(x + 1, y, z)]) A0[FINDEX(x, y, z)] += scale; }
					else
						A0[FINDEX(x, y, z)] += scale;

					// -x
					if(x > 0)
						{ if(!_obstacles[FINDEX(x - 1, y, z)]) A0[FINDEX(x, y, z)] += scale; }
					else
						A0[FINDEX(x, y, z)] += scale;

					// +y
					if(y < _yRes - 1)
						{ if(!_obstacles[FINDEX(x, y + 1, z)]) A0[FINDEX(x, y, z)] += scale; }
					else
						A0[FINDEX(x, y, z)] += scale;

					// -y
					if(y > 0)
						{ if(!_obstacles[FINDEX(x, y - 1, z)]) A0[FINDEX(x, y, z)] += scale; }
					else
						A0[FINDEX(x, y, z)] += scale;

					// +z
					if(z < _zRes - 1)
						{ if(!_obstacles[FINDEX(x, y, z + 1)]) A0[FINDEX(x, y, z)] += scale; }
					else
						A0[FINDEX(x, y, z)] += scale;

					// -z
					if(z > 0)
						{ if(!_obstacles[FINDEX(x, y, z - 1)]) A0[FINDEX(x, y, z)] += scale; }
					else
						A0[FINDEX(x, y, z)] += scale;
				}

	}

	for (z = 0; z < _zRes; z++)
		for (y = 0; y < _yRes; y++)
			for (x = 0; x < _xRes; x++)
	{
				if(!_obstacles[FINDEX(x, y, z)])
				{
					// x
					if(x < _xRes - 1)
						{ if(!_obstacles[FINDEX(x + 1, y, z)]) Ai[FINDEX(x, y, z)] = -scale; }
					else
						Ai[FINDEX(x, y, z)] = -scale;

					// y
					if(y < _yRes - 1)
						{ if(!_obstacles[FINDEX(x, y + 1, z)]) Aj[FINDEX(x, y, z)] = -scale; }
					else
						Aj[FINDEX(x, y, z)] = -scale;

					// z
					if(z < _zRes - 1)
						{ if(!_obstacles[FINDEX(x, y, z + 1)]) Ak[FINDEX(x, y, z)] = -scale; }
					else
						Ak[FINDEX(x, y, z)] = -scale;
				}

	}

	unsigned int rowCount = 0;
	for (z = 0; z < _zRes; z++)
		for (y = 0; y < _yRes; y++)
			for (x = 0; x < _xRes; x++)
		if (!_obstacles[FINDEX(x,y,z)])
		{
			rowCount = gti(FINDEX(x, y, z));

			/*
			if((x > 0) && (Ai[FINDEX(x - 1, y, z)] < 0))
				A.insert(rowCount, gti(FINDEX(x - 1, y, z))) = Ai[FINDEX(x - 1, y, z)];
			if((y > 0) && (Aj[FINDEX(x, y - 1, z)] < 0))
				A.insert(rowCount, gti(FINDEX(x, y - 1, z))) = Aj[FINDEX(x, y - 1, z)];
			if((z > 0) && (Ak[FINDEX(x, y, z - 1)] < 0))
				A.insert(rowCount, gti(FINDEX(x, y, z - 1))) = Ak[FINDEX(x, y, z - 1)];
			*/
		
			if(A0[FINDEX(x, y, z)] > 0)
				A.insert(rowCount, gti(FINDEX(x, y, z))) = A0[FINDEX(x, y, z)];

			if((x + 1 < _xRes) && (Ai[FINDEX(x, y, z)] < 0))
				A.insert(rowCount, gti(FINDEX(x + 1, y, z))) = Ai[FINDEX(x, y, z)];
			if((y + 1 < _yRes) && (Aj[FINDEX(x, y, z)] < 0))
				A.insert(rowCount, gti(FINDEX(x, y + 1, z))) = Aj[FINDEX(x, y, z)];
			if((z + 1 < _zRes) && (Ak[FINDEX(x, y, z)] < 0))
				A.insert(rowCount, gti(FINDEX(x, y, z + 1))) = Ak[FINDEX(x, y, z)];

			// rowCount++;
		}

	for (z = 0; z < _zRes; z++)
		for (y = 0; y < _yRes; y++)
			for (x = 0; x < _xRes; x++)
		if (!_obstacles[FINDEX(x,y,z)])
			b[gti(FINDEX(x, y, z))] = _divergence[FINDEX(x,y,z)];
#endif
	// copyBorderAll(_pressure, 0, _zRes);

	// solve Poisson equation
#if USE_NEW_CG == 1
	A.makeCompressed();

	VectorXf result(linearIndex);
	result.setZero(linearIndex);
	ConjugateGradient< SparseMatrix<float, RowMajor>, Upper > solver;
	solver.setMaxIterations(300);
	solver.setTolerance(1e-06);

	solver.compute(A);
	if(solver.info() != Success) 
	{
	  // decomposition failed
		printf("Solver error: Cannot create decomposition: %d\n", solver.info());
	}
	else
	{
		result = solver.solve(b);
		if(solver.info() != Success) 
		{
		  // solving failed
			printf("Solver error: Cannot solve equation: %d\n", solver.info());
		}
		else
			printf("Solver finished.\n");

		std::cout << "#iterations:     " << solver.iterations() << std::endl;
		std::cout << "estimated error: " << solver.error()      << std::endl;
	}
#else
	solvePressurePre(_pressure, _divergence, _obstacles);
#endif

#if 0
	for(unsigned int i = 0; i < _xRes * _yRes * _zRes; i++)
	{
		float value = (_pressure[i] - result[i]);
		if(value > 0.01)
			printf("error: p: %f, b: %f\n", _pressure[i], result[i]);
	}
#endif

#if 1
	{
		float maxvalue = 0;
		for(unsigned int i = 0; i < _xRes * _yRes * _zRes; i++)
		{
			if(_pressure[i] > maxvalue)
				maxvalue = _pressure[i];

			/* HACK: Animated collision object sometimes result in a non converging solvePressurePre() */ 
			if(_pressure[i] > _dx * _dt)
				_pressure[i] = _dx * _dt;
			else if(_pressure[i] < -_dx * _dt)
				_pressure[i] = -_dx * _dt;

			// if(_obstacle[i] && _pressure[i] != 0.0)
			// 	printf("BAD PRESSURE i\n");

			// if(_pressure[i]>1)
			// 	printf("index: %d\n", i);
		}
		// printf("Max pressure: %f, dx: %f\n", maxvalue, _dx);
	}
#endif
	// DG TODO: check this function, for now this is done in the next function
	// setObstaclePressure(_pressure, 0, _zRes);

	// project out solution
	float invDx = 1.0f / _dx;
	index = _slabSize + _xRes + 1;
	for (z = 1; z < _zRes - 1; z++, index += 2 * _xRes)
		for (y = 1; y < _yRes - 1; y++, index += 2)
			for (x = 1; x < _xRes - 1; x++, index++)
			{
				float vMask[3] = {1.0f, 1.0f, 1.0f}, vObst[3] = {0, 0, 0};
				float vR = 0.0f, vL = 0.0f, vT = 0.0f, vB = 0.0f, vD = 0.0f, vU = 0.0f;
#if USE_NEW_CG == 1
				float pC = result[gti(FINDEX(x, y, z))]; // center
				float pR = result[gti(FINDEX(x + 1, y, z))]; // right
				float pL = result[gti(FINDEX(x - 1, y, z))]; // left
				float pU = result[gti(FINDEX(x, y + 1, z))]; // top
				float pD = result[gti(FINDEX(x, y - 1, z))]; // bottom
				float pT = result[gti(FINDEX(x, y, z + 1))]; // Up
				float pB = result[gti(FINDEX(x, y, z - 1))]; // Down
#else

				float pC = _pressure[index]; // center
				float pR = _pressure[index + 1]; // right
				float pL = _pressure[index - 1]; // left
				float pU = _pressure[index + _xRes]; // Up
				float pD = _pressure[index - _xRes]; // Down
				float pT = _pressure[index + _slabSize]; // top
				float pB = _pressure[index - _slabSize]; // bottom
#endif
				if(!_obstacles[index])
				{
					// DG TODO: What if obstacle is left + right and one is moving?
					if(_obstacles[index+1])			{ pR = pC; vObst[0] = _xVelocityOb[index + 1];			vMask[0] = 0; }
					if(_obstacles[index-1])			{ pL = pC; vObst[0]	= _xVelocityOb[index - 1];			vMask[0] = 0; }
					if(_obstacles[index+_xRes])		{ pU = pC; vObst[1]	= _yVelocityOb[index + _xRes];		vMask[1] = 0; }
					if(_obstacles[index-_xRes])		{ pD = pC; vObst[1]	= _yVelocityOb[index - _xRes];		vMask[1] = 0; }
					if(_obstacles[index+_slabSize]) { pT = pC; vObst[2] = _zVelocityOb[index + _slabSize];	vMask[2] = 0; }
					if(_obstacles[index-_slabSize]) { pB = pC; vObst[2]	= _zVelocityOb[index - _slabSize];	vMask[2] = 0; }

					_xVelocity[index] -= 0.5f * (pR - pL) * invDx;
					_yVelocity[index] -= 0.5f * (pU  - pD) * invDx;
					_zVelocity[index] -= 0.5f * (pT - pB) * invDx;

					_xVelocity[index] = (vMask[0] * _xVelocity[index]) + vObst[0];
					_yVelocity[index] = (vMask[1] * _yVelocity[index]) + vObst[1];
					_zVelocity[index] = (vMask[2] * _zVelocity[index]) + vObst[2];
				}
				else
				{
					_xVelocity[index] = _xVelocityOb[index];
					_yVelocity[index] = _yVelocityOb[index];
					_zVelocity[index] = _zVelocityOb[index];
				}
			}

	// setObstacleVelocity(0, _zRes);

	// if (_pressure) delete[] _pressure;
	if (_divergence) delete[] _divergence;
}

//////////////////////////////////////////////////////////////////////
// calculate the obstacle velocity at boundary
//////////////////////////////////////////////////////////////////////
void FLUID_3D::setObstacleVelocity(int zBegin, int zEnd)
{
	
	// completely TODO <-- who wrote this and what is here TODO? DG

	const size_t index_ = _slabSize + _xRes + 1;

	//int vIndex=_slabSize + _xRes + 1;

	int bb=0;
	int bt=0;

	if (zBegin == 0) {bb = 1;}
	if (zEnd == _zRes) {bt = 1;}

	// tag remaining obstacle blocks
	for (int z = zBegin + bb; z < zEnd - bt; z++)
	{
		size_t index = index_ +(z-1)*_slabSize;

		for (int y = 1; y < _yRes - 1; y++, index += 2)
		{
			for (int x = 1; x < _xRes - 1; x++, index++)
		{
			if (!_obstacles[index])
			{
				// if(_obstacles[index+1]) xright = - _xVelocityOb[index]; 
				if((_obstacles[index - 1] & 8) && abs(_xVelocityOb[index - 1]) > FLT_EPSILON )
				{
					// printf("velocity x!\n");
					_xVelocity[index]  = _xVelocityOb[index - 1];
					_xVelocity[index - 1]  = _xVelocityOb[index - 1];
				}
				// if(_obstacles[index+_xRes]) yup    = - _yVelocityOb[index];
				if((_obstacles[index - _xRes] & 8) && abs(_yVelocityOb[index - _xRes]) > FLT_EPSILON)
				{
					// printf("velocity y!\n");
					_yVelocity[index]  = _yVelocityOb[index - _xRes];
					_yVelocity[index - _xRes]  = _yVelocityOb[index - _xRes];
				}
				// if(_obstacles[index+_slabSize]) ztop    = - _zVelocityOb[index];
				if((_obstacles[index - _slabSize] & 8) && abs(_zVelocityOb[index - _slabSize]) > FLT_EPSILON)
				{
					// printf("velocity z!\n");
					_zVelocity[index] = _zVelocityOb[index - _slabSize];
					_zVelocity[index - _slabSize] = _zVelocityOb[index - _slabSize];
				}
			}
			else
			{
				_density[index] = 0;
			}
			//vIndex++;
		}	// x loop
		//vIndex += 2;
		}	// y loop
		//vIndex += 2 * _xRes;
	}	// z loop
}

//////////////////////////////////////////////////////////////////////
// diffuse heat
//////////////////////////////////////////////////////////////////////
void FLUID_3D::diffuseHeat()
{
	SWAP_POINTERS(_heat, _heatOld);

	copyBorderAll(_heatOld, 0, _zRes);
	solveHeat(_heat, _heatOld, _obstacles);

	// zero out inside obstacles
	for (int x = 0; x < _totalCells; x++)
		if (_obstacles[x])
			_heat[x] = 0.0f;

}

//////////////////////////////////////////////////////////////////////
// stamp an obstacle in the _obstacles field
//////////////////////////////////////////////////////////////////////
void FLUID_3D::addObstacle(OBSTACLE* obstacle)
{
	int index = 0;
	for (int z = 0; z < _zRes; z++)
		for (int y = 0; y < _yRes; y++)
			for (int x = 0; x < _xRes; x++, index++)
				if (obstacle->inside(x * _dx, y * _dx, z * _dx)) {
					_obstacles[index] = true;
        }
}

//////////////////////////////////////////////////////////////////////
// calculate the obstacle directional types
//////////////////////////////////////////////////////////////////////
void FLUID_3D::setObstaclePressure(float *_pressure, int zBegin, int zEnd)
{

	// completely TODO <-- who wrote this and what is here TODO? DG

	const size_t index_ = _slabSize + _xRes + 1;

	//int vIndex=_slabSize + _xRes + 1;

	int bb=0;
	int bt=0;

	if (zBegin == 0) {bb = 1;}
	if (zEnd == _zRes) {bt = 1;}

	// tag remaining obstacle blocks
	for (int z = zBegin + bb; z < zEnd - bt; z++)
	{
		size_t index = index_ +(z-1)*_slabSize;

		for (int y = 1; y < _yRes - 1; y++, index += 2)
		{
			for (int x = 1; x < _xRes - 1; x++, index++)
		{
			// could do cascade of ifs, but they are a pain
			if (_obstacles[index] /* && !(_obstacles[index] & 8) DG TODO TEST THIS CONDITION */)
			{
				const int top   = _obstacles[index + _slabSize];
				const int bottom= _obstacles[index - _slabSize];
				const int up    = _obstacles[index + _xRes];
				const int down  = _obstacles[index - _xRes];
				const int left  = _obstacles[index - 1];
				const int right = _obstacles[index + 1];

				// unused
				// const bool fullz = (top && bottom);
				// const bool fully = (up && down);
				//const bool fullx = (left && right);

				/*
				_xVelocity[index] =
				_yVelocity[index] =
				_zVelocity[index] = 0.0f;
				*/
				_pressure[index] = 0.0f;

				// average pressure neighbors
				float pcnt = 0.;
				if (left && !right) {
					_pressure[index] += _pressure[index + 1];
					pcnt += 1.;
				}
				if (!left && right) {
					_pressure[index] += _pressure[index - 1];
					pcnt += 1.;
				}
				if (up && !down) {
					_pressure[index] += _pressure[index - _xRes];
					pcnt += 1.;
				}
				if (!up && down) {
					_pressure[index] += _pressure[index + _xRes];
					pcnt += 1.;
				}
				if (top && !bottom) {
					_pressure[index] += _pressure[index - _slabSize];
					pcnt += 1.;
				}
				if (!top && bottom) {
					_pressure[index] += _pressure[index + _slabSize];
					pcnt += 1.;
				}
				
				if(pcnt > 0.000001f)
				 	_pressure[index] /= pcnt;

				// TODO? set correct velocity bc's
				// velocities are only set to zero right now
				// this means it's not a full no-slip boundary condition
				// but a "half-slip" - still looks ok right now
			}
			//vIndex++;
		}	// x loop
		//vIndex += 2;
		}	// y loop
		//vIndex += 2 * _xRes;
	}	// z loop
}

void FLUID_3D::setObstacleBoundaries(float *_pressure, int zBegin, int zEnd)
{
	// cull degenerate obstacles , move to addObstacle?

	// r = b - Ax
	const size_t index_ = _slabSize + _xRes + 1;

	int bb=0;
	int bt=0;

	if (zBegin == 0) {bb = 1;}
	if (zEnd == _zRes) {bt = 1;}

	for (int z = zBegin + bb; z < zEnd - bt; z++)
	{
		size_t index = index_ +(z-1)*_slabSize;
		
		for (int y = 1; y < _yRes - 1; y++, index += 2)
		{
			for (int x = 1; x < _xRes - 1; x++, index++)
			{
				if (_obstacles[index] != EMPTY)
				{
					const int top   = _obstacles[index + _slabSize];
					const int bottom= _obstacles[index - _slabSize];
					const int up    = _obstacles[index + _xRes];
					const int down  = _obstacles[index - _xRes];
					const int left  = _obstacles[index - 1];
					const int right = _obstacles[index + 1];

					int counter = 0;
					if (up)    counter++;
					if (down)  counter++;
					if (left)  counter++;
					if (right) counter++;
					if (top)  counter++;
					if (bottom) counter++;

					// DG TODO: Do not shrink obstacles for now
					// if (counter < 3)
					//	_obstacles[index] = EMPTY;
				}
				if (_obstacles[index])
				{
					_xVelocity[index] =
					_yVelocity[index] =
					_zVelocity[index] = 0.0f;
					_pressure[index] = 0.0f;
				}
				//vIndex++;
			}	// x-loop
			//vIndex += 2;
		}	// y-loop
		//vIndex += 2* _xRes;
	}	// z-loop
}

//////////////////////////////////////////////////////////////////////
// add buoyancy forces
//////////////////////////////////////////////////////////////////////
void FLUID_3D::addBuoyancy(float *heat, float *density, int zBegin, int zEnd)
{
	int index = zBegin*_slabSize;

	for (int z = zBegin; z < zEnd; z++)
		for (int y = 0; y < _yRes; y++)
			for (int x = 0; x < _xRes; x++, index++)
			{
				_zForce[index] += *_alpha * density[index] + (*_beta * (heat[index] - _tempAmb)); // DG: was _yForce, changed for Blender
			}
}

//////////////////////////////////////////////////////////////////////
// add vorticity to the force field
//////////////////////////////////////////////////////////////////////
void FLUID_3D::addVorticity(int zBegin, int zEnd)
{
	//int x,y,z,index;
	if(_vorticityEps<=0.) return;

	int _blockSize=zEnd-zBegin;
	int _blockTotalCells = _slabSize * (_blockSize+2);

	float *_xVorticity, *_yVorticity, *_zVorticity, *_vorticity;

	int bb=0;
	int bt=0;
	int bb1=-1;
	int bt1=-1;

	if (zBegin == 0) {bb1 = 1; bb = 1; _blockTotalCells-=_blockSize;}
	if (zEnd == _zRes) {bt1 = 1;bt = 1; _blockTotalCells-=_blockSize;}

	_xVorticity = new float[_blockTotalCells];
	_yVorticity = new float[_blockTotalCells];
	_zVorticity = new float[_blockTotalCells];
	_vorticity = new float[_blockTotalCells];

	memset(_xVorticity, 0, sizeof(float)*_blockTotalCells);
	memset(_yVorticity, 0, sizeof(float)*_blockTotalCells);
	memset(_zVorticity, 0, sizeof(float)*_blockTotalCells);
	memset(_vorticity, 0, sizeof(float)*_blockTotalCells);

	//const size_t indexsetupV=_slabSize;
	const size_t index_ = _slabSize + _xRes + 1;

	// calculate vorticity
	float gridSize = 0.5f / _dx;
	//index = _slabSize + _xRes + 1;


	size_t vIndex=_xRes + 1;

	for (int z = zBegin + bb1; z < (zEnd - bt1); z++)
	{
		size_t index = index_ +(z-1)*_slabSize;
		vIndex = index-(zBegin-1+bb)*_slabSize;

		for (int y = 1; y < _yRes - 1; y++, index += 2)
		{
			for (int x = 1; x < _xRes - 1; x++, index++)
			{

				int up    = _obstacles[index + _xRes] ? index : index + _xRes;
				int down  = _obstacles[index - _xRes] ? index : index - _xRes;
				float dy  = (up == index || down == index) ? 1.0f / _dx : gridSize;
				int out   = _obstacles[index + _slabSize] ? index : index + _slabSize;
				int in    = _obstacles[index - _slabSize] ? index : index - _slabSize;
				float dz  = (out == index || in == index) ? 1.0f / _dx : gridSize;
				int right = _obstacles[index + 1] ? index : index + 1;
				int left  = _obstacles[index - 1] ? index : index - 1;
				float dx  = (right == index || left == index) ? 1.0f / _dx : gridSize;

				_xVorticity[vIndex] = (_zVelocity[up] - _zVelocity[down]) * dy + (-_yVelocity[out] + _yVelocity[in]) * dz;
				_yVorticity[vIndex] = (_xVelocity[out] - _xVelocity[in]) * dz + (-_zVelocity[right] + _zVelocity[left]) * dx;
				_zVorticity[vIndex] = (_yVelocity[right] - _yVelocity[left]) * dx + (-_xVelocity[up] + _xVelocity[down])* dy;

				_vorticity[vIndex] = sqrtf(_xVorticity[vIndex] * _xVorticity[vIndex] +
						_yVorticity[vIndex] * _yVorticity[vIndex] +
						_zVorticity[vIndex] * _zVorticity[vIndex]);

				vIndex++;
			}
			vIndex+=2;
		}
		//vIndex+=2*_xRes;
	}

	// calculate normalized vorticity vectors
	float eps = _vorticityEps;
	
	//index = _slabSize + _xRes + 1;
	vIndex=_slabSize + _xRes + 1;

	for (int z = zBegin + bb; z < (zEnd - bt); z++)
	{
		size_t index = index_ +(z-1)*_slabSize;
		vIndex = index-(zBegin-1+bb)*_slabSize;

		for (int y = 1; y < _yRes - 1; y++, index += 2)
		{
			for (int x = 1; x < _xRes - 1; x++, index++)
			{
				//

				if (!_obstacles[index])
				{
					float N[3];

					int up    = _obstacles[index + _xRes] ? vIndex : vIndex + _xRes;
					int down  = _obstacles[index - _xRes] ? vIndex : vIndex - _xRes;
					float dy  = (up == vIndex || down == vIndex) ? 1.0f / _dx : gridSize;
					int out   = _obstacles[index + _slabSize] ? vIndex : vIndex + _slabSize;
					int in    = _obstacles[index - _slabSize] ? vIndex : vIndex - _slabSize;
					float dz  = (out == vIndex || in == vIndex) ? 1.0f / _dx : gridSize;
					int right = _obstacles[index + 1] ? vIndex : vIndex + 1;
					int left  = _obstacles[index - 1] ? vIndex : vIndex - 1;
					float dx  = (right == vIndex || left == vIndex) ? 1.0f / _dx : gridSize;
					N[0] = (_vorticity[right] - _vorticity[left]) * dx;
					N[1] = (_vorticity[up] - _vorticity[down]) * dy;
					N[2] = (_vorticity[out] - _vorticity[in]) * dz;

					float magnitude = sqrtf(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);
					if (magnitude > 0.0f)
					{
						magnitude = 1.0f / magnitude;
						N[0] *= magnitude;
						N[1] *= magnitude;
						N[2] *= magnitude;

						_xForce[index] += (N[1] * _zVorticity[vIndex] - N[2] * _yVorticity[vIndex]) * _dx * eps;
						_yForce[index] -= (N[0] * _zVorticity[vIndex] - N[2] * _xVorticity[vIndex]) * _dx * eps;
						_zForce[index] += (N[0] * _yVorticity[vIndex] - N[1] * _xVorticity[vIndex]) * _dx * eps;
					}
					}	// if
					vIndex++;
					}	// x loop
				vIndex+=2;
				}		// y loop
			//vIndex+=2*_xRes;
		}				// z loop
				
	if (_xVorticity) delete[] _xVorticity;
	if (_yVorticity) delete[] _yVorticity;
	if (_zVorticity) delete[] _zVorticity;
	if (_vorticity) delete[] _vorticity;
}


void FLUID_3D::advectMacCormackBegin(int zBegin, int zEnd)
{
	Vec3Int res = Vec3Int(_xRes,_yRes,_zRes);

	if(!_domainBcLeft) copyBorderX(_xVelocityOld, res, zBegin, zEnd);
	else setZeroX(_xVelocityOld, res, zBegin, zEnd);

	if(!_domainBcFront) copyBorderY(_yVelocityOld, res, zBegin, zEnd);
	else setZeroY(_yVelocityOld, res, zBegin, zEnd); 

	if(!_domainBcTop) copyBorderZ(_zVelocityOld, res, zBegin, zEnd);
	else setZeroZ(_zVelocityOld, res, zBegin, zEnd);
}

//////////////////////////////////////////////////////////////////////
// Advect using the MacCormack method from the Selle paper
//////////////////////////////////////////////////////////////////////
void FLUID_3D::advectMacCormackEnd1(int zBegin, int zEnd)
{
	Vec3Int res = Vec3Int(_xRes,_yRes,_zRes);

	const float dt0 = _dt / _dx;

	int begin=zBegin * _slabSize;
	int end=begin + (zEnd - zBegin) * _slabSize;
	for (int x = begin; x < end; x++)
		_xForce[x] = 0.0;

	// advectFieldMacCormack1(dt, xVelocity, yVelocity, zVelocity, oldField, newField, res)

	advectFieldMacCormack1(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _densityOld, _densityTemp, res, zBegin, zEnd);
	advectFieldMacCormack1(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _heatOld, _heatTemp, res, zBegin, zEnd);
	advectFieldMacCormack1(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _xVelocityOld, _xVelocity, res, zBegin, zEnd);
	advectFieldMacCormack1(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _yVelocityOld, _yVelocity, res, zBegin, zEnd);
	advectFieldMacCormack1(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _zVelocityOld, _zVelocity, res, zBegin, zEnd);

	// Have to wait untill all the threads are done -> so continuing in step 3
}

//////////////////////////////////////////////////////////////////////
// Advect using the MacCormack method from the Selle paper
//////////////////////////////////////////////////////////////////////
void FLUID_3D::advectMacCormackEnd2(int zBegin, int zEnd)
{
	const float dt0 = _dt / _dx;
	Vec3Int res = Vec3Int(_xRes,_yRes,_zRes);

	// use force array as temp arrays
	float* t1 = _xForce;

	// advectFieldMacCormack2(dt, xVelocity, yVelocity, zVelocity, oldField, newField, tempfield, temp, res, obstacles)

	advectFieldMacCormack2(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _densityOld, _density, _densityTemp, t1, res, _obstacles, zBegin, zEnd);
	advectFieldMacCormack2(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _heatOld, _heat, _heatTemp, t1, res, _obstacles, zBegin, zEnd);
	advectFieldMacCormack2(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _xVelocityOld, _xVelocityTemp, _xVelocity, t1, res, _obstacles, zBegin, zEnd);
	advectFieldMacCormack2(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _yVelocityOld, _yVelocityTemp, _yVelocity, t1, res, _obstacles, zBegin, zEnd);
	advectFieldMacCormack2(dt0, _xVelocityOld, _yVelocityOld, _zVelocityOld, _zVelocityOld, _zVelocityTemp, _zVelocity, t1, res, _obstacles, zBegin, zEnd);

	if(!_domainBcLeft) copyBorderX(_xVelocityTemp, res, zBegin, zEnd);
	else setZeroX(_xVelocityTemp, res, zBegin, zEnd);				

	if(!_domainBcFront) copyBorderY(_yVelocityTemp, res, zBegin, zEnd);
	else setZeroY(_yVelocityTemp, res, zBegin, zEnd); 

	if(!_domainBcTop) copyBorderZ(_zVelocityTemp, res, zBegin, zEnd);
	else setZeroZ(_zVelocityTemp, res, zBegin, zEnd);

	setZeroBorder(_density, res, zBegin, zEnd);
	setZeroBorder(_heat, res, zBegin, zEnd);
#if 0
	{
	const size_t index_ = _slabSize + _xRes + 1;
	int bb=0;
	int bt=0;

	if (zBegin == 0) {bb = 1;}
	if (zEnd == _zRes) {bt = 1;}
	
	for (int z = zBegin + bb; z < zEnd - bt; z++)
	{
		size_t index = index_ +(z-1)*_slabSize;

		for (int y = 1; y < _yRes - 1; y++, index += 2)
		{
			for (int x = 1; x < _xRes - 1; x++, index++)
			{
				// clean custom velocities from moving obstacles again
				if (_obstacles[index])
				{
					_xVelocity[index] =
					_yVelocity[index] =
					_zVelocity[index] = 0.0f;
				}
			}
		}
	}
	}
#endif

	/*int begin=zBegin * _slabSize;
	int end=begin + (zEnd - zBegin) * _slabSize;
  for (int x = begin; x < end; x++)
    _xForce[x] = _yForce[x] = 0.0f;*/
}

