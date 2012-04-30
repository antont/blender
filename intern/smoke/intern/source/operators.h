/******************************************************************************
 *  DDF Fluid solver with Turbulence extensions
 *
 *  copyright 2009 Nils Thuerey, Tobias Pfaff
 * 
 *  DDF is free software, distributed under the GNU General Public License (GPL v2).
 *  See the file COPYING for more information.
 *
 * Basic functions for applying operators to grids
 *
 *****************************************************************************/

#ifndef DDF_OPERATORS_H
#define DDF_OPERATORS_H

#include "grid.h"
#if DDF_OPENMP==1
#include <omp.h>
#endif // DDF_OPENMP==1

namespace DDF {

// globals, allocated in globals.cpp
// id's of seperate passes (for debugging)
extern int gGridPassCounter;



//*****************************************************************************
// operators without openmp

template<class Operator, class GridClass>
void applyOperatorWithBoundSimple(Operator *opOrg, GridClass *grid, int bound) 
{
	int passid = ++gGridPassCounter;
	opOrg->resetVariables();

	Operator *op = opOrg; 
	op->getGaCalls().clear();
	op->buildCallList(); 
	//FlagGrid *pFlags = op->getFlags(); // FLAGMOD
	nVec3i s(0,0,0),e(grid->getSize());
	
	for(size_t gi=0; gi<op->getGaCalls().size(); gi++) { 
		op->getGaCalls()[gi]->lockRegion(nVec3i(s), nVec3i(e), passid); 
	} 

#if DDF_DIMENSION==3
		for(int k=0+bound; k<grid->getSizeZ()-bound; k++)  {
#else // DDF_DIMENSION==3
		{ const int k=gPatchSize/2;
#endif // DDF_DIMENSION==3
			for(int j=0+bound; j<grid->getSizeY()-bound; j++)  
				for(int i=0+bound; i<grid->getSizeX()-bound; i++)  { 
					(*op)(i,j,k);
				} 
		}

	for(size_t gi=0; gi<op->getGaCalls().size(); gi++) { 
		op->getGaCalls()[gi]->unlockRegion(passid); 
	} 


	// perform operator reduction
	opOrg->reduce(*op); 

	// DEBUG - check if patch has not the right passid!?
}

template<class Operator>
void applyOperatorToGridsSimple(Operator *opOrg) 
{
	applyOperatorWithBoundSimple(opOrg, opOrg->getFlags(), 0);
}

//*****************************************************************************
// operators with openmp

template<class Operator, class GridClass>
void applyOperatorWithBoundToGrid(Operator *opOrg, GridClass *grid, int bound)
{ 
	int passid = ++gGridPassCounter;
	opOrg->resetVariables();

#	if DDF_OPENMP==1
#	pragma omp parallel default(shared) 
	{ // omp region
		const int id = omp_get_thread_num();
		const int Nthrds = omp_get_num_threads(); 
#	else
	{ // non omp
		const int id=0; // , Nthrds=1;
#	endif

	Operator myop = *opOrg;
	Operator *op = &myop;
#	pragma omp critical (BUILDCALLLIST)
	{ 
		op->setThreadId(id);
		op->resetVariables();
		op->getGaCalls().clear();
		op->buildCallList(); 
	}
	//FlagGrid *pFlags = op->getFlags(); // FLAGMOD

		int gridsize = grid->getSizeZ();
		// divide up domain along z dir
		// for 2d, parallelize along Y instead
		if(gDim==2) gridsize = grid->getSizeY();
#	if (DDF_OPENMP==1)

		const int totalSize = gridsize -2* bound;
		const int kStart = ( id * (totalSize / Nthrds) )  + bound ; 
		int kEnd   = ( (id+1) * (totalSize / Nthrds) )    + bound; 
		
		// make sure the last thread covers everything
		if(id == Nthrds-1) kEnd = gridsize -bound;

		// init region for write accesses
		nVec3i s = nVec3i( 0 );
		nVec3i e = nVec3i( grid->getSize() );
		s[2] = kStart;
		e[2] = kEnd;

		// fix access in 2D
		if(gDim==2) {
			s[1]= kStart;
			e[1]=  kEnd;

			s[2]=gPatchSize/2;
			e[2] = s[2]+1;
		}

		if(0) debMsg("T","thread "<<id<<"/"<<Nthrds<<" k="<<kStart<<"-"<<kEnd<<" , totS="<<totalSize<<" gridK="<<gridsize<<" bound="<<bound );
#	else
		// constants for single threaded version
		const int kStart = 0        + bound;
		const int kEnd   = gridsize - bound;
		nVec3i s = nVec3i( 0 );
		nVec3i e = nVec3i( grid->getSize() );
#	endif

		for(size_t gi=0; gi<op->getGaCalls().size(); gi++) { 
			op->getGaCalls()[gi]->lockRegion(nVec3i(s), nVec3i(e), passid); 
		} 

		// loop over grid
#		if DDF_DIMENSION==3
		for(int k=kStart; k<kEnd; k++)  {
			for(int j=0+bound; j<grid->getSizeY()-bound; j++)  
#		else // DDF_DIMENSION==3
		const int k=gPatchSize/2;
		for(int j=kStart; j<kEnd; j++)  {
#		endif // DDF_DIMENSION==3
				for(int i=0+bound; i<grid->getSizeX()-bound; i++)  { 
					(*op)(i,j,k);
				} 
		 } // parallel loop
		 

		for(size_t gi=0; gi<op->getGaCalls().size(); gi++) { 
			op->getGaCalls()[gi]->unlockRegion(passid); 
		} 

	// perform operator reduction
#	pragma omp critical (REDUCE)
	{ opOrg->reduce(myop); }

		//debMsg("P","id "<<id<<"/"<<Nthrds<<", pids"<<npids<<" idsum"<<idsum);
	} // omp region
}

template<class Operator, class GridClass>
void applyOperatorToGridsWithoutFlags(Operator *opOrg, GridClass *grid)
{
	applyOperatorWithBoundToGrid(opOrg, grid, 0);
}

template<class Operator>
void applyOperatorToGrids(Operator *opOrg) 
{
	applyOperatorWithBoundToGrid(opOrg, opOrg->getFlags(), 0);
}






//*****************************************************************************
// base class for grid operators
// currently only manages flag access
class GridOpBase {
	public:
		GridOpBase() : 
			mpFlags(NULL), gaFlags(), gaCalls() 
		{
			getGaCalls().clear();
			//gGridPassCounter++; debMsg("GridPass","pass="<<gGridPassCounter);
		};
		virtual ~GridOpBase() {};
		
		// init flag grid
		inline void setFlags(FlagGrid *set)  { 
			mpFlags=set; 
			if(mpFlags) {
				gaFlags.gridAccInit(mpFlags, AM_READ, gaCalls); 
			} else {
				errFatal("GridOpBase::setFlags","Flags pointer not set!", SIMWORLD_GENERICERROR);
			}
		}
		// some operators dont need a flag pointer, use this function to zero it, setFlags requires a valid one!
		inline void setNullFlagPointer() {
			mpFlags = NULL;
		}
		// virtual, not time ciritcal - important for e.g. reduction, see below
		virtual void resetVariables() { };

		// set & get flag grid access
		inline FlagGrid *getFlags()                          { return mpFlags; }
		inline GridAccessor<int,0>& getFlagAcc()             { return gaFlags; }
		inline int getThreadId()                             { return gaCalls.threadId; }
		inline std::vector<GridAccessorBase*>& getGaCalls()  { return gaCalls.calls; }
		inline void setThreadId(int set)                     { gaCalls.threadId=set; }

		// Warning - make sure copying works, e.g., dont build call list
		// before each thread has own operator copy

		// Function to be implemented:
		// void resetVariables():
		// 		- reset fields that are modified during sweep (e.g. dotproduct
		//      reset to zeroo)
		//		- this can be called multiple times, e.g., for reducing
		//      ! crucial for correct multi threading ! , dont modify such
		//      fields in constructor or buildCallList
		// void buildCallList(): 
		// 		- init grid accessors, add to gaCalls.calls
		//		- call setFlags(mpFlags) to init base op
		// void operator() (int i, int j, int k) : 
		// 		- perform actual computation
		//    - note, flag handling has to be done in operator
		//      (e.g., const int currFlag = getFlagAcc()(i,j,k); )
		// void reduce(Operator &op) : 
		// 		- reduce results from operator "op" to current one
		//    - needed for multithreaded operator execution
		// ... getValue() : 
		// 		- return result of computations

	protected:
		//! allow access to flag grid
		FlagGrid *mpFlags;
		GridAccessor<int,0> gaFlags;
		GaCallContainer gaCalls;
}; // GridOpBase

// same as GridOpBase, but allows access to the border of 
// the flag array
template<int BORDER>
class GridOpBaseFlagbord {
	public:
		GridOpBaseFlagbord() : 
			mpFlags(NULL), gaFlags(), gaCalls() 
		{
			getGaCalls().clear();
		};
		virtual ~GridOpBaseFlagbord() {};
		
		// init flag grid
		inline void setFlags(FlagGrid *set)  { 
			mpFlags=set; 
			if(mpFlags) {
				gaFlags.gridAccInit(mpFlags, AM_READ, gaCalls); 
			}
		}
		// virtual, not time ciritcal - important for e.g. reduction, see below
		virtual void resetVariables() { };

		// set & get flag grid access
		inline FlagGrid *getFlags()                          { return mpFlags; }
		inline GridAccessor<int, BORDER>& getFlagAcc()       { return gaFlags; }
		inline int getThreadId()                             { return gaCalls.threadId; }
		inline std::vector<GridAccessorBase*>& getGaCalls()  { return gaCalls.calls; }
		inline void setThreadId(int set)                     { gaCalls.threadId=set; }

	protected:
		//! allow access to flag grid
		FlagGrid *mpFlags;
		GridAccessor<int, BORDER> gaFlags;
		GaCallContainer gaCalls;
}; // GridOpBaseFlagbord



//*****************************************************************************
// test operator, does nothing
class GridOpDummy : public GridOpBase {
	public:
		GridOpDummy(FlagGrid *flags) : GridOpBase() { 
			mpFlags = flags;
			applyOperatorToGrids(this);
		};
		~GridOpDummy() { }; 
		void buildCallList() {
			setFlags(mpFlags);
		};
		inline void operator() (int i, int j, int k) { 
#if DDF_DEBUG==1
			i=j=k= 0;
#endif
		};
		void reduce(GridOpDummy &op) { };
}; // GridOpDummy */



//*****************************************************************************
// touch operator, resets memory in threaded fashion
template<class Scalar>
class GridOpTouchMemory : public GridOpBase {
	public:
		GridOpTouchMemory(FlagGrid *flags, Grid<Scalar> *gDst, Scalar val) : GridOpBase() { 
			mValue = val;
			mpDst = gDst;
			flags = NULL;
			// TODO remove mpFlags = flags;
			applyOperatorToGridsWithoutFlags(this, mpDst);
		};
		~GridOpTouchMemory() { }; 
		void resetVariables() { };
		void buildCallList() {
			gaDst.gridAccInit(mpDst, AM_WRITE, gaCalls); 
			setNullFlagPointer();
		};
		inline void operator() (int i, int j, int k) { 
			//debMsg("a","b "<<PRINT_IJK<<" = "<<mValue<<", "<<mpDst->getSize() );
			gaDst.write(i,j,k) = mValue;
		};
		void reduce(GridOpTouchMemory &op) { };
	protected:
		Grid<Scalar> *mpDst;
		GridAccessor<Scalar,0> gaDst;
		Scalar mValue;
}; // GridOpTouchMemory */


//*****************************************************************************
// copy whole grid
template<class SCALAR>
class goCopyGrid : public GridOpBase {
	public:
		goCopyGrid(Grid<SCALAR> *gDst, Grid<SCALAR> *gSrc) : GridOpBase() { 
			mpDst = gDst;
			mpSrc = gSrc;
			const bool useMemcpy = true;

			// sanity check
			if(mpSrc->getNumElements() != mpDst->getNumElements() ) {
				errFatal("goCopyGrid","Invalid grid sizes! "<< mpSrc->getNumElements() <<" vs "<< mpDst->getNumElements(), SIMWORLD_GRIDERROR );
			}

			if(useMemcpy) {
				// direct copy
				memcpy( &mpDst->getGlobal(0,0,0) ,  &mpSrc->getGlobal(0,0,0), mpDst->getNumElements() * sizeof(SCALAR) );
			} else {
				// parallel transfer 
				applyOperatorToGridsWithoutFlags(this, mpDst);
			}
		};
		~goCopyGrid() { }; 
		void resetVariables() { };
		void buildCallList() {
			gaDst.gridAccInit(mpDst, AM_WRITE, gaCalls); 
			gaSrc.gridAccInit(mpSrc, AM_READ, gaCalls); 
			setNullFlagPointer();
		};
		inline void operator() (int i, int j, int k) { 
			gaDst.write(i,j,k) = gaSrc(i,j,k);
		};
		void reduce(goCopyGrid &op) { };
	protected:
		Grid<SCALAR> *mpDst;
		Grid<SCALAR> *mpSrc;
		GridAccessor<SCALAR,0> gaDst;
		GridAccessor<SCALAR,0> gaSrc;
}; // goCopyGrid */

// copy one element of a vector to a scalar grid
template<class Scalar,int NUM>
class goCopyVec3ToScalar : public GridOpBase {
	public:
		goCopyVec3ToScalar(Grid<Scalar> *gDst, Grid<Vec3> *gSrc) : GridOpBase() { 
			mpDst = gDst;
			mpSrc = gSrc;
			applyOperatorToGridsWithoutFlags(this, mpDst);
		};
		~goCopyVec3ToScalar() { }; 
		void resetVariables() { };
		void buildCallList() {
			gaDst.gridAccInit(mpDst, AM_WRITE, gaCalls); 
			gaSrc.gridAccInit(mpSrc, AM_READ, gaCalls); 
			setNullFlagPointer();
		};
		inline void operator() (int i, int j, int k) { 
			Real spass=gaSrc(i,j,k)[NUM];			
			gaDst.write(i,j,k) = gaSrc(i,j,k)[NUM];
		};
		void reduce(goCopyVec3ToScalar &op) { };
	protected:
		Grid<Scalar> *mpDst;
		Grid<Vec3>   *mpSrc;
		GridAccessor<Scalar,0> gaDst;
		GridAccessor<Vec3,0> gaSrc;
}; // goCopyVec3ToScalar */

// copy one element of a vector from a scalar grid
template<class Scalar,int NUM>
class goCopyScalarToVec3 : public GridOpBase {
	public:
		goCopyScalarToVec3(Grid<Vec3> *gDst, Grid<Scalar> *gSrc) : GridOpBase() { 
			mpDst = gDst;
			mpSrc = gSrc;
			applyOperatorToGridsWithoutFlags(this, mpDst);
		};
		~goCopyScalarToVec3() { }; 
		void resetVariables() { };
		void buildCallList() {
			gaDst.gridAccInit(mpDst, AM_WRITE, gaCalls); 
			gaSrc.gridAccInit(mpSrc, AM_READ, gaCalls); 
			setNullFlagPointer();
		};
		inline void operator() (int i, int j, int k) { 
			gaDst.write(i,j,k)[NUM] = gaSrc(i,j,k);
		};
		void reduce(goCopyScalarToVec3 &op) { };
	protected:
		Grid<Vec3> *mpDst;
		Grid<Scalar> *mpSrc;
		GridAccessor<Vec3,0> gaDst;
		GridAccessor<Scalar,0> gaSrc;
}; // goCopyScalarToVec3 */



#if DDF_DIMENSION==3
#define LAPLACEFAC 6.
#else // DDF_DIMENSION==3
#define LAPLACEFAC 4.
#endif // DDF_DIMENSION==3


//*****************************************************************************
// compute norm (without sqrt) of a vector
class GridOpNormNosqrt : public GridOpBase{
	public:
		GridOpNormNosqrt(Grid<Real> *Var, FlagGrid *Flags, Real *result = NULL) : GridOpBase() {
			mpVar = Var;
			mpFlags = Flags;
			applyOperatorToGrids( this );
			if(result) *result = mNorm;
		};
		~GridOpNormNosqrt() {};
		virtual void resetVariables() {
			mNorm = 0.;
			//outcnt=0;
		}
		void buildCallList() {
			gaVar.gridAccInit(mpVar, AM_READ, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			const Real s = gaVar(i,j,k); 
			mNorm += s*s;
			//debMsg("GridOpNormNosqrt","at "<<PRINT_IJK<<" = "<< mNorm );
			//if(outcnt%1000==0) std::cout<<" "<<mNorm;
			//outcnt++;
		}

		void reduce(GridOpNormNosqrt &op) {
			//debMsg("GridOpNormNosqrt","reduce mNorm"<<mNorm<<" += "<<op.mNorm);
			mNorm += op.mNorm;
		}

		Real getValue() { return mNorm; }
	protected:
		//int outcnt;
		Real mNorm;
		Grid<Real> *mpVar;
		GridAccessor<Real,0> gaVar;
}; // GridOpNormNosqrt



//*****************************************************************************
// compute min and max values in grid 1 (better use goCompMinMax below...)
static inline Real _helperMinMaxNorm( Real r ) {
	return r;
};
static inline Real _helperMinMaxNorm( Vec3 r ) {
	return normNoSqrt(r);
};
template<class Scalar>
class goFindMinMax : public GridOpBase{
	public:
		goFindMinMax(FlagGrid *flags, Grid<Scalar> *grid) : GridOpBase() {
			mpVar = grid;
			mpFlags = flags;
			resetVariables();
			if(flags) {
				// use flags, if supplied to skip obstacle cells
				applyOperatorToGrids( this );
			} else {
				applyOperatorToGridsWithoutFlags(this, grid);
			}
		};
		~goFindMinMax() {};
		virtual void resetVariables() {
			//outcnt=0; mNorm = 0.;
			mMinLen =  0.5*FP_REAL_MAX;
			mMaxLen = -0.5*FP_REAL_MAX;
			mMinVal = 0.;
			mMaxVal = 0.;
			mMinPos = mMaxPos = 0;
			mInitedMin = mInitedMax = 
			mFirstInitRed = false;
		}
		void buildCallList() {
			gaVar.gridAccInit(mpVar, AM_READ, gaCalls); 
			if(!mpFlags) setNullFlagPointer();
			else setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			const Scalar s = gaVar(i,j,k); 
			const Real slen = _helperMinMaxNorm( s );
			if((!mpFlags) || (!fgIsObstacle(getFlagAcc()(i,j,k)) )) {
				// also max not for obs!?
				if((!mInitedMax) || (mMaxLen<slen)) {
					mMaxVal = s;
					mMaxLen = slen;
					mMaxPos = nVec3i(i,j,k);
					mInitedMax = true;
				}
				// minima only in fluid region
				if((!mInitedMin) || (mMinLen>slen)) {
					mMinVal = s;
					mMinLen = slen;
					mMinPos = nVec3i(i,j,k);
					mInitedMin = true;
				}
			}
			//outcnt++;
			//debMsg("goFindMinMax","at "<<PRINT_IJK<<" = "<< s <<","<<off << " min="<<mMinVal<<" at "<<mMinPos<<" max="<<mMaxVal<<" at "<<mMaxPos);
		}

		void reduce(goFindMinMax &op) {
			// used for reducing with opOrg
			if(!mFirstInitRed) { 
				mMaxVal = op.mMaxVal;
				mMaxLen = op.mMaxLen;
				mMaxPos = op.mMaxPos;
				mMinVal = op.mMinVal;
				mMinLen = op.mMinLen;
				mMinPos = op.mMinPos;
				mFirstInitRed = true;
				return;
			}
			if(mMaxLen<op.mMaxLen) {
				mMaxVal = op.mMaxVal;
				mMaxLen = op.mMaxLen;
				mMaxPos = op.mMaxPos;
			}
			if(mMinLen>op.mMinLen) {
				mMinVal = op.mMinVal;
				mMinLen = op.mMinLen;
				mMinPos = op.mMinPos;
			}
		}

		Real mMinLen;
		Real mMaxLen;
		Scalar mMinVal;
		Scalar mMaxVal;
		nVec3i mMinPos;
		nVec3i mMaxPos;
	protected:
		//int outcnt;
		bool mInitedMin, mInitedMax, mFirstInitRed;
		Grid<Scalar> *mpVar;
		GridAccessor<Scalar,0> gaVar;
}; // goFindMinMax


//*****************************************************************************
// compute min and max values in grid 2
template<class Value>
class goCompMinMax : public GridOpBase{
	public:
		goCompMinMax(Grid<Value> *Var, FlagGrid *Flags) : GridOpBase() {
			mpVar = Var;
			mpFlags = Flags;
			applyOperatorToGrids( this );
		};
		~goCompMinMax() {};
		virtual void resetVariables() {
			// todo, add debug check for min/max inits!?
			mMinAbs =  1e10;
			mMaxAbs = -1e10;
			mMin = mMax = 0.;
			mMinPos = mMaxPos = 0;
			//outcnt=0;
		}
		void buildCallList() {
			gaVar.gridAccInit(mpVar, AM_READ, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			const Value s = gaVar(i,j,k); 
			const Real sabs = normHelper(s);
			//mNorm += s*s;
			if(sabs>mMaxAbs) {
				mMaxAbs = sabs;
				mMax = s;
				mMaxPos = nVec3i(i,j,k);
			}
			if(sabs<mMinAbs) {
				mMinAbs = sabs;
				mMin = s;
				mMinPos = nVec3i(i,j,k);
			}
		}

		void reduce(goCompMinMax &op) {
			//debMsg("goCompMinMax","reduce mNorm"<<mNorm<<" += "<<op.mNorm);
			//mNorm += op.mNorm;
			if(op.mMaxAbs>mMaxAbs) {
				mMaxAbs = op.mMaxAbs;
				mMax = op.mMax;
				mMaxPos = op.mMaxPos;
			}
			if(op.mMinAbs<mMinAbs) {
				mMinAbs = op.mMinAbs;
				mMin = op.mMin;
				mMinPos = op.mMinPos;
			}
		}

		Value getMinValue() { return mMin; }
		Value getMaxValue() { return mMax; }
		Real getMinAbsValue() { return mMinAbs; }
		Real getMaxAbsValue() { return mMaxAbs; }
		Real getMinPosValue() { return mMinPos; }
		Real getMaxPosValue() { return mMaxPos; }
		string toString() {
			std::ostringstream out;
			out << "min="<<mMin<<" ("<<mMinAbs<<") at "<<mMinPos<<
				    " max="<<mMax<<" ("<<mMaxAbs<<") at "<<mMaxPos ;
			return out.str();
		}
	protected:
		//int outcnt;
		Real mMinAbs, mMaxAbs;
		Value mMin, mMax;
		nVec3i mMinPos, mMaxPos;
		Grid<Value> *mpVar;
		GridAccessor<Real,0> gaVar;
}; // goCompMinMax

//*****************************************************************************
// subtract the entries of grid 2 from grid 1, store result to destination
// grid, and sum up the total difference
template<class Value>
class GridOpDifference : public GridOpBase{
	public:
		GridOpDifference(Grid<Value>* Dst, Grid<Value>* Var1, Grid<Value>* Var2, FlagGrid *Flags) : GridOpBase() {
			mpDst = Dst;
			mpVar1 = Var1;
			mpVar2 = Var2;
			mpFlags = Flags;
			applyOperatorToGrids( this );
		}
		~GridOpDifference() {};
		void buildCallList() {
			gaDst.gridAccInit(mpDst, DDF::AM_WRITE, gaCalls); 
			gaVar1.gridAccInit(mpVar1, DDF::AM_READ, gaCalls); 
			gaVar2.gridAccInit(mpVar2, DDF::AM_READ, gaCalls);
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			Value diff;
			if (gaFlags(i,j,k) != FOBSTACLE)
				 diff = gaVar2(i,j,k)-gaVar1(i,j,k);
			else
				diff = 0;
			gaDst.write(i,j,k) = diff; 
			mDiffSum += diff;
		}

		void reduce(GridOpDifference &op) {
			mDiffSum += op.mDiffSum;
		} 

		Value getDiffSum(void) { return mDiffSum; }

	protected:
		Grid<Value> *mpDst, *mpVar1, *mpVar2;
		GridAccessor<Value,0> gaDst;
		GridAccessor<Value,0> gaVar1;
		GridAccessor<Value,0> gaVar2;
		
		Value mDiffSum;
}; // GridOpDifference */

//*****************************************************************************
// sum of two grids
template<class T>
class goSum : public GridOpBase{
	public:
		goSum(Grid<T>* dst, Grid<T>* grid1, FlagGrid *flags) : GridOpBase(),
			mpDst(dst), mpVar1(grid1) {
			mpFlags = flags;
			applyOperatorToGrids( this );
		}
		~goSum() {};
		void buildCallList() {
			gaDst.gridAccInit(mpDst, DDF::AM_READWRITE, gaCalls); 
			gaVar1.gridAccInit(mpVar1, DDF::AM_READ, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			if (gaFlags(i,j,k) != FOBSTACLE)
				gaDst.write(i,j,k) += gaVar1(i,j,k);
			else
				gaDst.write(i,j,k) = 0;
		}

		void reduce(goSum &op) {} 

	protected:
		Grid<T> *mpDst, *mpVar1;
		GridAccessor<T,0> gaDst;
		GridAccessor<T,0> gaVar1;
}; // goSum

//*****************************************************************************
// scale a grid
template<class T>
class goScale : public GridOpBase{
	public:
		goScale(Grid<T>* dst, FlagGrid *flags, Real s) : GridOpBase(),
			mpDst(dst), mScale(s) {
			mpFlags = flags;
			applyOperatorToGrids( this );
		}
		~goScale() {};
		void buildCallList() {
			gaDst.gridAccInit(mpDst, DDF::AM_WRITE, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			if (gaFlags(i,j,k) != FOBSTACLE)
				gaDst.write(i,j,k) *= mScale;
		}

		void reduce(goScale &op) {	};

	protected:
		Grid<T> *mpDst;
		Real mScale;
		GridAccessor<T,0> gaDst;		
}; // goScale



//*****************************************************************************
// multiply entries of both input grids, and store them in destination grid
class GridOpMultiplyEntries : public GridOpBase{
	public:
		GridOpMultiplyEntries(Grid<Real> *Dst, Grid<Real> *Var1, Grid<Real> *Var2, FlagGrid *Flags) : GridOpBase() {
			mpDst = Dst;
			mpVar1 = Var1;
			mpVar2 = Var2;
			mpFlags = Flags;
			applyOperatorToGrids( this );
		}
		~GridOpMultiplyEntries() {};
		void buildCallList() {
			gaDst.gridAccInit(mpDst, AM_WRITE, gaCalls); 
			gaVar1.gridAccInit(mpVar1, AM_READ, gaCalls); 
			gaVar2.gridAccInit(mpVar2, AM_READ, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			gaDst.write(i,j,k) = gaVar1(i,j,k)*gaVar2(i,j,k); 
		}

		void reduce(GridOpMultiplyEntries &op) { } 
	protected:
		Grid<Real> *mpDst, *mpVar1, *mpVar2;
		GridAccessor<Real,0> gaDst;
		GridAccessor<Real,0> gaVar1;
		GridAccessor<Real,0> gaVar2;
}; // GridOpMultiplyEntries */


//*****************************************************************************
// compute dot product of two vectors (saved in grid format)

// use current precision (single/double)
class goDotProdReal : public GridOpBase{
	public:
		goDotProdReal(Grid<Real> *Var1, Grid<Real> *Var2, FlagGrid *Flags, Real *result = NULL) : 
				GridOpBase() {
			mpVar1 = Var1;
			mpVar2 = Var2;
			mpFlags = Flags;
			mDot = 0.;
			applyOperatorToGrids( this );
			if(result) *result = mDot;
		}
		~goDotProdReal() {};
		virtual void resetVariables() {
			mDot = 0.;
		}
		void buildCallList() {
			gaVar1.gridAccInit(mpVar1, AM_READ, gaCalls); 
			gaVar2.gridAccInit(mpVar2, AM_READ, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			mDot += gaVar1(i,j,k)*gaVar2(i,j,k); 
		}

		void reduce(goDotProdReal &op) { 
			//debMsg("goDotProdReal","reduce mDot"<<mDot<<" += "<<op.mDot);
			mDot += op.mDot; 
		} 
		inline Real getValue() { return mDot; }
	protected:
		Real mDot;
		Grid<Real> *mpVar1, *mpVar2;
		GridAccessor<Real,0> gaVar1;
		GridAccessor<Real,0> gaVar2;
}; // goDotProdReal */

// use enforced double precision  (default)
class goDotProd : public GridOpBase{
	public:
		goDotProd(Grid<Real> *Var1, Grid<Real> *Var2, FlagGrid *Flags, Real *result = NULL) : 
				GridOpBase() {
			mpVar1 = Var1;
			mpVar2 = Var2;
			mpFlags = Flags;
			mDot = 0.;
			applyOperatorToGrids( this );
			if(result) *result = mDot;
		}
		~goDotProd() {};
		virtual void resetVariables() {
			mDot = 0.;
		}
		void buildCallList() {
			gaVar1.gridAccInit(mpVar1, AM_READ, gaCalls); 
			gaVar2.gridAccInit(mpVar2, AM_READ, gaCalls); 
			setFlags(mpFlags);
		};

		inline void operator() (int i, int j, int k) {
			mDot += (double)( gaVar1(i,j,k)*gaVar2(i,j,k) ); 
		}

		void reduce(goDotProd &op) { 
			//debMsg("goDotProd","reduce mDot"<<mDot<<" += "<<op.mDot);
			mDot += op.mDot; 
		} 
		inline Real getValue() { return (Real)mDot; }
	protected:
		double mDot;
		Grid<Real> *mpVar1, *mpVar2;
		GridAccessor<Real,0> gaVar1;
		GridAccessor<Real,0> gaVar2;
}; // goDotProd */

template<class T>
class fsSetConstant : public GridOpBase { 
	public:
		fsSetConstant(FlagGrid *flags, Grid<T> *dst, T value) :
				GridOpBase(), mpDst(dst), mVal(value) {
			mpFlags = flags;
			applyOperatorToGrids(this);
		};
		~fsSetConstant() { }; 
		void resetVariables() { };
		void buildCallList() {
			gaDst.gridAccInit(mpDst, AM_WRITE, gaCalls); 
			setFlags(mpFlags);
		};

		// add forces and update empty cells free surface boundaries
		inline void operator() (int i, int j, int k) { 
			gaDst.write(i,j,k) = mVal;
		};
		void reduce(fsSetConstant &op) { };

	protected:
		Grid<T> *mpDst;
		GridAccessor<T,0> gaDst;
		// value to set
		T mVal;
}; // fsSetConstant */


}; // namespace DDF

#endif // DDF_OPERATORS_H

