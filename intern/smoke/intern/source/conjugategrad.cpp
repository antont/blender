/******************************************************************************
 *  DDF Fluid solver with Turbulence extensions
 *
 *  copyright 2009 Nils Thuerey, Tobias Pfaff
 * 
 *  DDF is free software, distributed under the GNU General Public License (GPL v2).
 *  See the file COPYING for more information.
 *
 * Conjugate Gradient solver classes, used in Pressure solve
 *
 *****************************************************************************/

#include "globals.h"
#include "vectorbase.h"
#include "grid.h"
#include "operators.h"
#include "conjugategrad.h"

// oreconditioning methods:
// 1 = incomplete cholesky ala wavelet turbulence (needs 4 additional grids)
// 2 = modified IC ala Bridson (1 add. grid)

namespace DDF { 

void InitPreconditionIncompCholesky(FlagGrid *flags,
				Grid<Real> *A0, Grid<Real> *Ai, Grid<Real> *Aj, Grid<Real> *Ak,
				Grid<Real> *orgA0, Grid<Real> *orgAi, Grid<Real> *orgAj, Grid<Real> *orgAk) 
{
	// compute IC according to Golub and Van Loan

	if(!A0 || !Ai || !Aj || !Ak) {
		errFatal("InitPreconditioner","method=1 but grids PCA0,j,k not set!", SIMWORLD_GRIDERROR);
		return;
	}
	FOR_IJK_GRID(A0) {
		A0->getGlobal(i,j,k) = orgA0->getGlobal(i,j,k);
		Ai->getGlobal(i,j,k) = orgAi->getGlobal(i,j,k);
		Aj->getGlobal(i,j,k) = orgAj->getGlobal(i,j,k);
		Ak->getGlobal(i,j,k) = orgAk->getGlobal(i,j,k);
	}

	if(0) FOR_IJK_GRID(flags) { // DEBUG
			if(0) debMsg("DEB_INCOMP_CHOL mat","at "<<PRINT_IJK<<" "<<
					A0->getGlobal(i,j,k)<<" "<< Ai->getGlobal(i,j,k)<<" "<< Aj->getGlobal(i,j,k)<<" "<< Ak->getGlobal(i,j,k)<<" " );
			printf("ORGMAT debug at %d,%d,%d = %f, %f, %f, %f \n", i,j,k,
					A0->getGlobal(i,j,k), Ai->getGlobal(i,j,k), Aj->getGlobal(i,j,k), Ak->getGlobal(i,j,k) );
		} // DEBUG

	FOR_IJK_GRID(flags) {
		if (fgIsFluid(flags->getGlobal(i,j,k)) ) {
			A0->getGlobal(i,j,k) = sqrt(A0->getGlobal(i,j,k));

			// correct left and top stencil in other entries
			// for i = k+1:n
			//  if (A(i,k) != 0)
			//    A(i,k) = A(i,k) / A(k,k)
			Real invDiagonal = 1.0f / A0->getGlobal(i,j,k);
			Ai->getGlobal(i,j,k) *= invDiagonal;
			Aj->getGlobal(i,j,k) *= invDiagonal;
			Ak->getGlobal(i,j,k) *= invDiagonal;

			// correct the right and bottom stencil in other entries
			// for j = k+1:n //   for i = j:n
			//      if (A(i,j) != 0)
			//        A(i,j) = A(i,j) - A(i,k) * A(j,k)
			A0->getGlobal(i+1,j,k) -= Ai->getGlobal(i,j,k) * Ai->getGlobal(i,j,k);
			A0->getGlobal(i,j+1,k) -= Aj->getGlobal(i,j,k) * Aj->getGlobal(i,j,k);
			A0->getGlobal(i,j,k+1) -= Ak->getGlobal(i,j,k) * Ak->getGlobal(i,j,k);
		}
	}

	// invert A0 for faster computation later
	FOR_IJK_GRID(flags) {
		if ( (fgIsFluid(flags->getGlobal(i,j,k)) )  &&
			  (A0->getGlobal(i,j,k) > 0.) )
		{
			// invert
			A0->getGlobal(i,j,k) = 1. / A0->getGlobal(i,j,k);
		}
	}

	if(0) FOR_IJK_GRID(flags) { // DEBUG
			if(0) debMsg("DEB_INCOMP_CHOL mat","at "<<PRINT_IJK<<" "<<
					A0->getGlobal(i,j,k)<<" "<< Ai->getGlobal(i,j,k)<<" "<< Aj->getGlobal(i,j,k)<<" "<< Ak->getGlobal(i,j,k)<<" " );
			printf("debug at %d,%d,%d = %f, %f, %f, %f \n", i,j,k,
					A0->getGlobal(i,j,k), Ai->getGlobal(i,j,k), Aj->getGlobal(i,j,k), Ak->getGlobal(i,j,k) );
		} // DEBUG
	//exit(1);
};



void InitPreconditionModifiedIncompCholesky2(FlagGrid *flags,
				Grid<Real> *_A0, Grid<Real> *_Ai, Grid<Real> *_Aj, Grid<Real> *_Ak,
				Grid<Real> *_orgA0, Grid<Real> *_orgAi, Grid<Real> *_orgAj, Grid<Real> *_orgAk) 
{
	// compute IC according to Golub and Van Loan
	// simplify access to grids...
	Grid<Real> &Aprecond = *_A0;
	Grid<Real> &A0 = *_orgA0;
	Grid<Real> &Ai = *_orgAi;
	Grid<Real> &Aj = *_orgAj;
	Grid<Real> &Ak = *_orgAk;

	if(!_A0) {
		errFatal("InitPreconditioner","method=2 but grids PCA0 not set!", SIMWORLD_GRIDERROR);
		return;
	}

	GridOpTouchMemory<Real>(NULL, _A0, 0.);

	const Real tau = 0.97;
	const Real sigma = 0.25;
	FOR_IJK_GRID(flags) {
		if (fgIsFluid(flags->getGlobal(i,j,k)) ) {
			// compute modified incomplete cholesky
			Real e = 0.;
			e = A0(i,j,k) 
				- SQUARE(Ai(i-1,j,k) * Aprecond(i-1,j,k) )
				- SQUARE(Aj(i,j-1,k) * Aprecond(i,j-1,k) )
				- SQUARE(Ak(i,j,k-1) * Aprecond(i,j,k-1) ) ;
			e -= tau * (
					Ai(i-1,j,k) * ( Aj(i-1,j,k) + Ak(i-1,j,k) )* SQUARE( Aprecond(i-1,j,k) ) +
					Aj(i,j-1,k) * ( Ai(i,j-1,k) + Ak(i,j-1,k) )* SQUARE( Aprecond(i,j-1,k) ) +
					Ak(i,j,k-1) * ( Ai(i,j,k-1) + Aj(i,j,k-1) )* SQUARE( Aprecond(i,j,k-1) ) +
					0. );

			// stability cutoff
			if(e < sigma * A0(i,j,k))
				e = A0(i,j,k);

			Aprecond(i,j,k) = 1. / sqrt( e );
			//debMsg("Aprecond","at "<<PRINT_IJK<<" "<<Aprecond(i,j,k) );
			// */
		}
	}
};



void InitPrecondition(int method, FlagGrid *Flags,
				Grid<Real> *A0, Grid<Real> *Ai, Grid<Real> *Aj, Grid<Real> *Ak,
				Grid<Real> *orgA0, Grid<Real> *orgAi, Grid<Real> *orgAj, Grid<Real> *orgAk) 
{

	//debMsg("InitPrecondition","Method="<<method);

	switch(method) {
		case 1:  
	 		InitPreconditionIncompCholesky(Flags, A0,Ai,Aj,Ak , orgA0,orgAi,orgAj,orgAk );
			break;
		case 2:  
	 		InitPreconditionModifiedIncompCholesky2(Flags, A0,Ai,Aj,Ak , orgA0,orgAi,orgAj,orgAk );
			break;
		//default:
	 		// nothing to do ... goInitPreconditionID(mpSearch, mpResidual, mpFlags );
	};

};





void ApplyPreconditionIncompCholesky(Grid<Real> *dst, Grid<Real> *Var1, FlagGrid *flags,
				Grid<Real> *A0, Grid<Real> *Ai, Grid<Real> *Aj, Grid<Real> *Ak,
				Grid<Real> *orgA0, Grid<Real> *orgAi, Grid<Real> *orgAj, Grid<Real> *orgAk) {
	// debug out
	if(0) { FOR_IJK_GRID(dst) {
					printf("apply1 at %d,%d,%d = %f, %f \n", i,j,k, dst->getGlobal(i,j,k)*1000., Var1->getGlobal(i,j,k)*1000. );
				} } // DEBUG out
	
	FOR_IJK_GRID(dst) {
		if (fgIsFluid(flags->getGlobal(i,j,k)) ) { 
			// forward substitution
			dst->getGlobal(i,j,k) = ( Var1->getGlobal(i,j,k) 
				- ( dst->getGlobal(i-1, j, k) * Ai->getGlobal(i-1, j,k) )
				- ( dst->getGlobal(i, j-1, k) * Aj->getGlobal(i, j-1,k) )
				- ( dst->getGlobal(i, j, k-1) * Ak->getGlobal(i,j, k-1) )  )
				* A0->getGlobal(i,j,k) ; // */
		}
	}

	FOR_IJKREV_GRID(dst) {
		if (fgIsFluid(flags->getGlobal(i,j,k)) ) {
			// backw. substitution
			dst->getGlobal(i,j,k) = ( dst->getGlobal(i,j,k) 
				- ( dst->getGlobal(i+1,j,k) * Ai->getGlobal(i+0,j,k) )
				- ( dst->getGlobal(i,j+1,k) * Aj->getGlobal(i,j+0,k) )
				- ( dst->getGlobal(i,j,k+1) * Ak->getGlobal(i,j,k+0) )  )
				* A0->getGlobal(i,j,k) ;// */
		}
	}

	// debug out
	if(0) { FOR_IJK_GRID(dst) {
					printf("apply2 at %d,%d,%d = %f, %f \n", i,j,k, dst->getGlobal(i,j,k)*1000., Var1->getGlobal(i,j,k)*1000. );
				} } // DEBUG out
};


void ApplyPreconditionModifiedIncompCholesky2(Grid<Real> *dst, Grid<Real> *Var1, FlagGrid *flags,
				Grid<Real> *Aprecond, Grid<Real> *Aunused1, Grid<Real> *Aunused2, Grid<Real> *Aunused3,
				Grid<Real> *orgA0, Grid<Real> *orgAi, Grid<Real> *orgAj, Grid<Real> *orgAk) {
	
	FOR_IJK_GRID(dst) {
		if (fgIsFluid(flags->getGlobal(i,j,k)) ) { 
			const Real precond = Aprecond->getGlobal(i,j,k);
			// forward substitution
			dst->getGlobal(i,j,k) = ( Var1->getGlobal(i,j,k) 
				- ( dst->getGlobal(i-1, j, k) * orgAi->getGlobal(i-1, j,k) *precond )
				- ( dst->getGlobal(i, j-1, k) * orgAj->getGlobal(i, j-1,k) *precond )
				- ( dst->getGlobal(i, j, k-1) * orgAk->getGlobal(i,j, k-1) *precond )  )
				* precond ; // */
		}
	}

	FOR_IJKREV_GRID(dst) {
		if (fgIsFluid(flags->getGlobal(i,j,k)) ) {
			const Real precond = Aprecond->getGlobal(i,j,k);
			// backw. substitution
			dst->getGlobal(i,j,k) = ( dst->getGlobal(i,j,k) 
				- ( dst->getGlobal(i+1,j,k) * orgAi->getGlobal(i+0,j,k) *precond )
				- ( dst->getGlobal(i,j+1,k) * orgAj->getGlobal(i,j+0,k) *precond )
				- ( dst->getGlobal(i,j,k+1) * orgAk->getGlobal(i,j,k+0) *precond )  )
				* precond ;// */
		}
	}
};


void ApplyPrecondition(int method, Grid<Real> *Dst, Grid<Real> *Var1, FlagGrid *Flags,
				Grid<Real> *A0, Grid<Real> *Ai, Grid<Real> *Aj, Grid<Real> *Ak,
				Grid<Real> *orgA0, Grid<Real> *orgAi, Grid<Real> *orgAj, Grid<Real> *orgAk) {

	// debMsg("ApplyPrecondition","Method="<<method); 
	switch(method) {
		case 1:  
	 		ApplyPreconditionIncompCholesky(Dst, Var1, Flags, A0,Ai,Aj,Ak , orgA0,orgAi,orgAj,orgAk );
			break;
		case 2:  
	 		ApplyPreconditionModifiedIncompCholesky2(Dst, Var1, Flags, A0,Ai,Aj,Ak , orgA0,orgAi,orgAj,orgAk );
			break;
		default:
			// id, means PC "off"
			goPreconditionID(Dst, Var1, Flags);
	} 
}

}; // DDF
