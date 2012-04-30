/******************************************************************************
 *  DDF Fluid solver with Turbulence extensions
 *
 *  copyright 2009 Nils Thuerey, Tobias Pfaff
 * 
 *  DDF is free software, distributed under the GNU General Public License (GPL v2).
 *  See the file COPYING for more information.
 *
 * Basic grid class
 *
 *****************************************************************************/

#ifndef DDF_GRIDS_H
#define DDF_GRIDS_H

//#include "patches.h"
#include "globals.h"
#include "vectorbase.h"
// operators.h is included below
namespace DDF {

const int gPatchSize = 5;

class GaCallContainer;
// grid debugging
const bool gStrictGridDebug = (DDF_DEBUG==1);

// class prototypes
template<class Scalar> class Grid;
template<class Scalar, int BOUNDARY> class GridAccessor;
template<class Scalar> class GridOpTouchMemory;


// globals, allocated in globals.cpp
extern int gGridIdCounter;

// neighbor vectors (helper)
static const int nbx[27] = { -1, 0, 1,  -1, 0, 1,  -1, 0, 1,    -1, 0, 1,  -1, 0, 1,  -1, 0, 1,    -1, 0, 1,  -1, 0, 1,  -1, 0, 1 };
static const int nby[27] = { -1,-1,-1,   0, 0, 0,   1, 1, 1,    -1,-1,-1,   0, 0, 0,   1, 1, 1,    -1,-1,-1,   0, 0, 0,   1, 1, 1 };
static const int nbz[27] = { -1,-1,-1,  -1,-1,-1,  -1,-1,-1,     0, 0, 0,   0, 0, 0,   0, 0, 0,     1, 1, 1,   1, 1, 1,   1, 1, 1 };

// debug value
const bool gUseInvalidCheck = false;
static const Real gInvalidReal = 0.; // -1e14;

// helper to shorten grid funtions
//#define RETURN_ZERO {mZero=0; return mZero;}
#define RETURN_ZERO {this->setDummyZero(); return mZero;}

//*****************************************************************************
// base class with standard functionality (sizes etc.)
class GridBase {
	public:
		// cons/des
		GridBase(const std::string& name = "-unnamed-") :
			mGridId(-1),
			mSizeX(0), mSizeY(0), mSizeZ(0),
			mMinIndex(0), mMaxIndex(0),
			mName(name),
			mSanityCheckMode(2),
			mGridFlags(0)
		{ 
			mGridId = gGridIdCounter++;
			// ...
		};
		virtual ~GridBase() { };


		// standard access funcs
		nVec3i getMinIndex()           { return mMinIndex; }
		void   setMinIndex(nVec3i set) { mMinIndex = set; }
		nVec3i getMaxIndex()           { return mMaxIndex; }
		void   setMaxIndex(nVec3i set) { mMaxIndex = set; }
		//! grid size access
		nVec3i getSize()               { return nVec3i(mSizeX,mSizeY,mSizeZ); }
		nVec3i getGridSize()           { return nVec3i(mSizeX,mSizeY,mSizeZ); } // synt. sugar...
		int    getSizeX()              { return mSizeX; }
		int    getSizeY()              { return mSizeY; }
		int    getSizeZ()              { return mSizeZ; }
		size_t getNumElements()        { return mSizeX * mSizeY * mSizeZ; }
		void   setSize(nVec3i set)     { mSizeX = set[0]; mSizeY = set[1]; mSizeZ = set[2]; }

		// helper functions for 2d/3d distinctions of loop boundaries
		int    getMinZLoopValue() { if(gDim==3) { return 0;      } else { return gPatchSize/2; } }
		int    getMaxZLoopValue() { if(gDim==3) { return mSizeZ; } else { return gPatchSize/2+1; } }
		int    getMinZLoopValue(int bnd) { if(gDim==3) { return 0+bnd;      } else { return gPatchSize/2; } }
		int    getMaxZLoopValue(int bnd) { if(gDim==3) { return mSizeZ-bnd; } else { return gPatchSize/2+1; } }

		int getGridId()                   { return mGridId; }
		int getUsePatches()               { return false; }

		std::string getName() { return mName; }
		void setName(std::string set) { mName = set; }

		int getSanityCheckMode() { return mSanityCheckMode; }
		void setSanityCheckMode(int set) { mSanityCheckMode = set; }

		// or'd flags used for debugging in the gui:
		// 1 = means hide from display cycle
		// 2 = show also values in empty region (eg for level set grids)
		// 4 = this is a velocity grid with centered instead of staggered values
		int getDisplayFlags() { return mDisplayFlags; }
		void setDisplayFlags(int set) { mDisplayFlags = set; }

		// DG TODO: add description here
		int getGridFlags() { return mGridFlags; }
		void setGridFlags(int set) { mGridFlags = set; }


		int getMaxSize() {
			int ms = mSizeX;
			if (mSizeY>ms) ms = mSizeY;
			if (mSizeZ>ms) ms = mSizeZ;
			return ms;
		}

		bool checkIndexValid(int x,int y, int z) const {
			if (x <     0) return false;
			if (y <     0) return false;
			if (z <     0) return false;
			if (x>=mSizeX) return false;
			if (y>=mSizeY) return false;
			if (z>=mSizeZ) return false;
			return true;
		}

		bool checkIndexValidWithBounds(int x,int y, int z, int bnd) const {
			if (x <     0+bnd) return false;
			if (y <     0+bnd) return false;
			if (z <     0+bnd) return false;
			if (x>=mSizeX-bnd) return false;
			if (y>=mSizeY-bnd) return false;
			if (z>=mSizeZ-bnd) return false;
			return true;
		}

		bool checkIndexInBounds(int x,int y, int z, nVec3i s, nVec3i e) {
			if (x < s[0]) return false;
			if (y < s[1]) return false;
			if (z < s[2]) return false;
			if (x>= e[0]) return false;
			if (y>= e[1]) return false;
			if (z>= e[2]) return false;
			return true;
		}

	protected:

		//! global id for grid (for debugging)
		int mGridId;

		//! grid sizes
		int mSizeX;
		int mSizeY;
		int mSizeZ;
		//! grid extent min
		nVec3i mMinIndex;
		//! grid extent max
		nVec3i mMaxIndex;

		// grid name for debugging etc.
		std::string mName;
		// debugging, sanity check modes: 0=off, 1=check for nan/inf, 2=also check for threshold (default)
		int mSanityCheckMode;
		// debug display flags for grids (or'd)
		// 1 = for debug display in gui - skip this grid?
		// 2 = show empty cells 
		int mDisplayFlags;

		// see mGridFlags defines below
		int mGridFlags;
};

// mGridFlags
#define DDF_GRID_NO_FREE 1


//*****************************************************************************
// container of a single grid
template<class Scalar>
class Grid : public GridBase {
	public:
		// cons/des
		Grid(const std::string& name="-unnamed-");
		virtual ~Grid();

		//! init memory & reset to zero
		void initGridMem(int x,int y, int z);
		void initGridMem(nVec3i size) { initGridMem(size[0], size[1], size[2]); };

		//! change size, dont reset any data
		void resizeGridMem(int set_x,int set_y, int set_z);

		// string output
		std::string toString(bool format=true);
		// print formatted in given region
		std::string printWithRoi(int* roi);

		// inlined access functions
		inline Scalar& operator() (int x,int y, int z) {
			if (gStrictGridDebug && (!checkIndexValid(x,y,z) )) {
				errFatal("Grid::operator()","Invalid access to "<<PRINT_VEC(x,y,z)  << ", grid dim: " << PRINT_VEC(mSizeX,mSizeY,mSizeZ) , SIMWORLD_GRIDERROR);
				RETURN_ZERO; 
			}
			return mpData[ ((z*mSizeY+y)*mSizeX+x) ];
		}

		// direct global access to grid 
		inline Scalar& getGlobal(int x,int y, int z) {
			if (gStrictGridDebug && (!checkIndexValid(x,y,z) )) {
				errFatal("Grid::operator()","Invalid access to "<<PRINT_VEC(x,y,z) << ", grid dim: " << PRINT_VEC(mSizeX,mSizeY,mSizeZ) , SIMWORLD_GRIDERROR);
				RETURN_ZERO; 
			}
			return mpData[ ((z*mSizeY+y)*mSizeX+x) ];
		}
		// convenience defines....
		inline Scalar& getGlobal(nVec3i p) { return getGlobal(p[0],p[1],p[2]); }
		inline Scalar& get(int x,int y, int z) { return getGlobal(x,y,z); }
		inline Scalar& get(nVec3i p) { return getGlobal(p[0],p[1],p[2]); }

		// direct access with given index
		inline Scalar& operator[] (size_t index) {
			if (gStrictGridDebug && (index<0 || index>=(mSizeX*mSizeY*mSizeZ)) ) {
				errFatal("Grid::operator()","Invalid access to index "<< index << ", grid dim: " << PRINT_VEC(mSizeX,mSizeY,mSizeZ) , SIMWORLD_GRIDERROR);
				RETURN_ZERO; 
			}
			return mpData[ index ];
		}

		// direct global access to grid 
		inline void setGlobal(Scalar val, int x,int y, int z) {
			if (gStrictGridDebug && (!checkIndexValid(x,y,z) )) {
				errFatal("Grid::operator()","Invalid access to "<<PRINT_VEC(x,y,z) << ", grid dim: " << PRINT_VEC(mSizeX,mSizeY,mSizeZ) , SIMWORLD_GRIDERROR);
				return; 
			}
			mpData[ ((z*mSizeY+y)*mSizeX+x) ] = val;
		}

		inline void clearGrid()
		{
			memset(mpData, 0, sizeof(Scalar)*mSizeX*mSizeY*mSizeZ);
		}
		

		inline Scalar getInterpolated(Real x, Real y, Real z);
		inline Scalar getInterpolated(Vec3 p) { return getInterpolated(p[0],p[1],p[2]); }

		inline size_t getDataSize() { return mDataSize; }

	protected:
		//! for safely returning zero from access funcs
		static Scalar mZero; 

		// implementations ins globals.cpp
		void setDummyZero();

		// actual grid data (when usepatches=off)
		Scalar *mpData;

		// for resizing the grid
		size_t mDataSize;
}; // Grid

// static constant fields for returning off-grid values in debug mode (gStrictGridDebug enabled)
template<class Scalar>
Scalar Grid<Scalar>::mZero; // = Scalar(0.);



//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

template<class Scalar>
Grid<Scalar>::Grid(const std::string& name) : GridBase(name) {
	mpData = NULL;
}

template<class Scalar>
Grid<Scalar>::~Grid() {
	if (mpData) delete[] mpData;
}

template<class Scalar>
void Grid<Scalar>::initGridMem(int set_x,int set_y, int set_z) {
	if (mpData) delete[] mpData;
	Scalar zero;
	zero = 0;
	if (0) debMsg("Grid::Grid","Requested size "<<PRINT_VEC(set_x,set_y,set_z) );

	// special 2D init
	if (gDim==2) {
		set_z = gPatchSize;
		// min. thickness of 2d slice should be 5 cells...
		if (gPatchSize<5) {
			errFatal("Grid::initGridMem","Invalid gPatchSize="<<gPatchSize<<" for 2d compile!",SIMWORLD_GRIDERROR);
		}
	}

	// only valid for non patch based grids / non dyn. sims
	mSizeX = set_x;
	mSizeY = set_y;
	mSizeZ = set_z;
	mMinIndex = nVec3i(0,0,0);
	mMaxIndex = nVec3i(mSizeX,mSizeY,mSizeZ);
	int numElems = mSizeX*mSizeY*mSizeZ;
	// compute patchsizes = (n-1)/gp
	nVec3i mS(mSizeX,mSizeY,mSizeZ);
	nVec3i mPs = (mS+nVec3i(-1))/gPatchSize;

	std::string patchstring[2] = { "full grid", "patchwise" };

	// not UsePatches
	mpData = new Scalar[numElems];
	mDataSize = numElems;

	for (int i=0; i<mSizeX*mSizeY*mSizeZ; i++) {
		mpData[i] = zero;
	} 
	
	if (0) debMsg("Grid::Grid","Inited size"<<mS<<", "<<patchstring[0]<<", mp"<<mPs<<", id="<<mGridId); // GRIDMEMDEB
	// inited...
}

// resize data, keep old data if new size is smaller
// parts are similar to initGridMem
template<class Scalar>
void Grid<Scalar>::resizeGridMem(int set_x,int set_y, int set_z) {
	const bool debugRealloc = false;
	size_t numElems = set_x*set_y*set_z;

	if (debugRealloc) debMsg("Grid::resizeGridMem","Inited size="<<mDataSize<<", requested size="<<numElems<<" "<<PRINT_VEC(set_x,set_y,set_z) );

	// sanity check, same as initGridMem
	if (gDim==2) {
		set_z = gPatchSize;

		if (gPatchSize<5) {
			errFatal("Grid::initGridMem","Invalid gPatchSize="<<gPatchSize<<" for 2d compile!",SIMWORLD_GRIDERROR);
		}
	}

	mSizeX = set_x;
	mSizeY = set_y;
	mSizeZ = set_z;
	mMinIndex = nVec3i(0,0,0);
	mMaxIndex = nVec3i(mSizeX,mSizeY,mSizeZ);

	if(mDataSize>=numElems) {
		// ok, keep old size

		if (debugRealloc) debMsg("Grid::resizeGridMem","Data block kept");
		return;
	}

	// reallocate
	delete [] mpData;
	// add safety buffer for future enlargements
	size_t allocSize = (numElems * 1.5);
	mpData = new Scalar[allocSize];
	mDataSize = allocSize;
	if (debugRealloc) debMsg("Grid::resizeGridMem","Data block re-allocated");
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


template<class Scalar>
std::string Grid<Scalar>::toString(bool format) 
{
	std::ostringstream out;

	/*for (int j=mMinIndex[1]; j<mMaxIndex[1]; j++) {
		for (int k=mMinIndex[2]; k<mMaxIndex[2]; k++) {
			for (int i=mMinIndex[0]; i<mMaxIndex[0]; i++) {
				// debug yzx order */

	int ks = mMinIndex[2];
	int ke = mMaxIndex[2];
	if (gDim==2) { ks = gPatchSize/2; ke = ks+1; }
	for (int k=ks; k<ke; k++) {
		for (int j=mMinIndex[1]; j<mMaxIndex[1]; j++) {
			for (int i=mMinIndex[0]; i<mMaxIndex[0]; i++) { 
				// normal zyx order */
				out << getGlobal(i,j,k);
				out <<" ";
			}
			if (format) out << std::endl;
		}
		if (format) out << std::endl;
	}

	return out.str();
}
// print only in indicated region
template<class Scalar>
std::string Grid<Scalar>::printWithRoi(int *roi) 
{
	std::ostringstream out;
	int ks = mMinIndex[2];
	int ke = mMaxIndex[2];
	if (gDim==2) { ks = gPatchSize/2; ke = ks+1; }

	if(0) debMsg("printWithRoi",this->getName()<<" size "<<this->getSize() <<" roi: "<<
			roi[0]<<"-"<< roi[1]<<", "<<
			roi[2]<<"-"<< roi[3]<<", "<<
			roi[4]<<"-"<< roi[5]<<" " );

	for (int k=ks; k<ke; k++) {
	 	if (roi[4]>=0 && roi[4]>k) continue;
	 	if (roi[5]>=0 && roi[5]<k) continue;
		for (int j=mMinIndex[1]; j<mMaxIndex[1]; j++) {
	 		if (roi[2]>=0 && roi[2]>j) continue;
			if (roi[3]>=0 && roi[3]<j) continue;
			for (int i=mMinIndex[0]; i<mMaxIndex[0]; i++) { 
	 			if (roi[0]>=0 && roi[0]>i) continue;
				if (roi[1]>=0 && roi[1]<i) continue;
				// normal zyx order */
				out << getGlobal(i,j,k);
				out <<" ";
			}
			out << std::endl;
		}
		out << std::endl;
	}
	return out.str();
}

template<class Scalar>
inline Scalar Grid<Scalar>::getInterpolated(Real x, Real y, Real z) 
{
	int xi = (int)x;
 	int yi = (int)y;
 	int zi = (int)z;
	Real s1 = x-(Real)xi, s0 = 1.-s1;
	Real t1 = y-(Real)yi, t0 = 1.-t1;
	Real f1 = z-(Real)zi, f0 = 1.-f1; 

	// clamp to border
	if (x < 0) { xi = 0; s0 = 1.0; s1 = 0.0; }
	if (y < 0) { yi = 0; t0 = 1.0; t1 = 0.0; }
	if (z < 0) { zi = 0; f0 = 1.0; f1 = 0.0; }
	if (xi >= mSizeX-1) { xi = mSizeX-2; s0 = 0.0; s1 = 1.0; }
	if (yi >= mSizeY-1) { yi = mSizeY-2; t0 = 0.0; t1 = 1.0; }
	if (zi >= mSizeZ-1) { zi = mSizeZ-2; f0 = 0.0; f1 = 1.0; }

	Scalar ret;

#define VAL getGlobal
#if DDF_DIMENSION==3
	ret = ( 
		(VAL(xi  ,yi,zi)*t0 + VAL(xi  ,yi+1,zi)*t1 )*s0 + 
		(VAL(xi+1,yi,zi)*t0 + VAL(xi+1,yi+1,zi)*t1 )*s1 )*f0 
		+ ( 
		(VAL(xi  ,yi,zi+1)*t0  + VAL(xi  ,yi+1,zi+1)*t1 )*s0 + 
		(VAL(xi+1,yi,zi+1)*t0  + VAL(xi+1,yi+1,zi+1)*t1 )*s1 )*f1 ;
#else
	ret = (VAL(xi  ,yi,zi)*t0  + VAL(xi  ,yi+1,zi)*t1 )*s0
		+  (VAL(xi+1,yi,zi)*t0  + VAL(xi+1,yi+1,zi)*t1 )*s1 ;
#endif // DDF_DIMENSION==3
#undef VAL

	return ret;
}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************



// type to precompute indices
typedef unsigned int GaccIndex;


// access modes
enum GaAccModes { 
	AM_NONE=0, 
	AM_READ=1, 
	AM_READWRITE=2, 
	AM_WRITE=3 };

// interface for simplified locking
// read/write access and regions are checked during debug
// compilation with DDF_DEBUG = 1
class GridAccessorBase {
	public:
		// cons/des
		GridAccessorBase(int accmode) { 
			mAccMode = accmode;
		};
		virtual ~GridAccessorBase() { };

		virtual void lockRegion(nVec3i s, nVec3i e, int passid) { 
			debMsg("GridAccessorBase","Invalid lockRegion called...");
			passid = 0;
		}
		virtual void unlockRegion(int passid) { 
			debMsg("GridAccessorBase","Invalid unlockRegion called...");
			passid = 0;
		}

	protected:
		//! mode, should be consistent! 0=none, 1=read, 2=read&write, 3=write only
		int mAccMode;
};


// grid accessor class, handles locking and accessing patches
template<class Scalar, int BOUNDARY>
class GridAccessor : public GridAccessorBase {
	public:
		// cons/des
		GridAccessor() : GridAccessorBase(AM_NONE),
			mSrc(NULL), mRegionStart(0), mRegionEnd(0)
		{ };

		virtual ~GridAccessor() { };

		// init access to grid, and add accessor to grid-acc call list (for looping)
		void gridAccInit(Grid<Scalar> *grid, GaAccModes acc, GaCallContainer &gacalls);
		//! write access
		Scalar& write(int x,int y, int z);
		//! init write access to region from src grid (no boundaries!)
		virtual void lockRegion(nVec3i s, nVec3i e, int passid);
		//! unlock
		virtual void unlockRegion(int passid);
		//! read access (constant!)
		inline const Scalar& operator() (int x,int y, int z) const;

		inline Scalar getInterpolated(Real x, Real y, Real z) const;

		//! read access with precomputed index
		inline const Scalar& operator() (GaccIndex index) const;
		//! compute grid acc index
		inline GaccIndex getGaccIndex(int x,int y, int z);
		//! get access to the source grid (mostly for debug checks, e.g. index ranges)
		inline Grid<Scalar>* getGaSourceGrid() { return mSrc; }

	protected:

		std::string toString() const;

		//! source grid for access
		Grid<Scalar>* mSrc;
		// locked region
		nVec3i mRegionStart, mRegionEnd;
};


class GaCallContainer {
	public:
		// ctor, dtor
		GaCallContainer() : threadId(-1), calls() { };
		virtual ~GaCallContainer() { };

		int threadId;
		std::vector<DDF::GridAccessorBase*> calls;
};






// init access to grid, and add accessor to grid-acc call list (for looping)
template<class Scalar, int BOUNDARY>
void 
GridAccessor<Scalar,BOUNDARY>::gridAccInit(Grid<Scalar> *grid, GaAccModes acc, GaCallContainer &gacalls)
{ 
	if (!grid) {
		errFatal("GridAccessor::gridAccInit","Invalid source null grid!",SIMWORLD_GRIDERROR);
		return; 
	}
	mSrc = grid;
	mAccMode = acc;

	// debug, check for double entries
	if (DDF_DEBUG==1) {
		for (size_t i=0; i<gacalls.calls.size(); i++) {
			if (gacalls.calls[i] == this) {
				errFatal("GridAccessor::gridAccInit","Debug check - double gacall entry!",SIMWORLD_GRIDERROR);
				return; 
			}
		}
	} // debug

	gacalls.calls.push_back(this);
};

//! write access
template<class Scalar, int BOUNDARY>
Scalar& 
GridAccessor<Scalar,BOUNDARY>::write(int x,int y, int z) {

	if (gStrictGridDebug) {
		if (!mSrc->checkIndexValid(x,y,z) ) {
			errFatal("GridAccessor::write","Acess to "<<PRINT_VEC(x,y,z)<<", out of bounds for grid "<<mSrc->getGridId()<<"!", SIMWORLD_GRIDERROR);
		}

		if (!mSrc->checkIndexInBounds(x,y,z, mRegionStart, mRegionEnd) ) {
			errFatal("GridAccessor::write","Acess to "<<PRINT_VEC(x,y,z)<<", out of locked region "<<
				  mRegionStart<<" to "<<mRegionEnd<<" for grid "<<mSrc->getGridId()<<"!", SIMWORLD_GRIDERROR);
		}

		if((mAccMode!=AM_WRITE) && (mAccMode!=AM_READWRITE)) {
			errFatal("GridAccessor::write","Acess to "<<PRINT_VEC(x,y,z)<<", region of grid "<<mSrc->getGridId()<<" not locked for writing!", SIMWORLD_GRIDERROR);
		} 
	}

	//return (*mSrc)(mOffset[0]+x, mOffset[1]+y, mOffset[2]+z);
	return (*mSrc)(x, y, z);
}

//! init write access to region from src grid (no boundaries!)
template<class Scalar, int BOUNDARY>
void GridAccessor<Scalar,BOUNDARY>::lockRegion(nVec3i s, nVec3i e, int passid) {
	//debMsg("GridAccessor","lockRegion #"<<tpatch->mPatchId<<", acc Mode="<<mAcc Mode<<" srcId"<<mSrc->getGridId() );

	mRegionStart = s;
	mRegionEnd   = e;
}

//! unlock
template<class Scalar, int BOUNDARY>
void GridAccessor<Scalar,BOUNDARY>::unlockRegion(int passid) { 
	// nothing to do now...
}

//! read access (constant!)
template<class Scalar, int BOUNDARY>
const Scalar& 
GridAccessor<Scalar,BOUNDARY>:: operator() (int x,int y, int z) const {
	return (*mSrc)(x, y, z);
}


//! read access with gacc index
template<class Scalar, int BOUNDARY>
const Scalar& 
GridAccessor<Scalar,BOUNDARY>:: operator() (GaccIndex index) const {
	return (*mSrc)(index);
}
template<class Scalar, int BOUNDARY>
Scalar GridAccessor<Scalar,BOUNDARY>::getInterpolated(Real x, Real y, Real z) const
{
	return mSrc->getInterpolated(x,y,z);
}

		//const Scalar& operator() (GaccIndex index) const;
//! compute grid acc index
template<class Scalar, int BOUNDARY> GaccIndex 
GridAccessor<Scalar,BOUNDARY>::getGaccIndex(int x,int y, int z) {
	return (( (z) *mSrc->getSizeY()+ (y) ) *mSrc->getSizeX()+ (x) );
}

template<class Scalar, int BOUNDARY>
std::string 
GridAccessor<Scalar,BOUNDARY>::toString() const {
	std::ostringstream out;
	out <<"accessor: "<<sizeof(Scalar)<<" region="<<mRegionStart<<" to "<<mRegionEnd<<" ";
	// grid info?

	return out.str();
}



//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//typedef Grid<int> FlagGrid;
#define FlagGrid Grid<int>

//! obstacle flag types
typedef enum FluidSolverFlagsT { 
	FNONE=0,
	FFLUID=1, 
	FOBSTACLE=2,
	FEMPTY=4, 
	FINFLOW=8, 
	FOUTFLOW=16, 
	FPARTICLESOURCE=512, 
	FDENSITYSOURCE=1024, 
	F_TO_FLUID=(1<<16),  // bit flag for changing types
	F_RESEED=(1<<17),    // reseed particles here
	FINVALID=-1 
} FluidSolverFlags;

// analogous to IS_OBS, IS_FLUID
inline bool fgIsObstacle(int flag) {
	return ((flag & FOBSTACLE)!=0);
}
inline bool fgIsFluid(int flag) {
	return ((flag & FFLUID)!=0);
}
inline bool fgIsEmpty(int flag) {
	return ((flag & FEMPTY)!=0);
}
inline bool fgIsInflow(int flag) {
	return ((flag & FINFLOW)!=0);
}
inline bool fgIsOutflow(int flag) {
	return ((flag & FOUTFLOW)!=0);
}

}; // namespace DDF

#include "operators.h"
#endif // DDF_GRIDS_H

