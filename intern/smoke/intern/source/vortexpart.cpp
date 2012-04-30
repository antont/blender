/******************************************************************************
 *  DDF Fluid solver with Turbulence extensions
 *
 *  copyright 2009 Nils Thuerey, Tobias Pfaff
 * 
 *  DDF is free software, distributed under the GNU General Public License (GPL v2).
 *  See the file COPYING for more information.
 *
 * Vortex particle classes
 *
 *****************************************************************************/


#include "vortexpart.h"

// get rid of MSVC ambiguities be enforcing single precision
#ifdef WIN32
#define pow powf
#endif

#if DDF_DIMENSION==2
#define FOR_KERNEL(rd) for (int j=-(rd),k=0,idx=0;j<=rd;++j) for (int i=-(rd);i<=rd;++i,++idx)
#else
#define FOR_KERNEL(rd) for (int k=-(rd),idx=0;k<=rd;++k) for (int j=-(rd);j<=rd;++j) for (int i=-(rd);i<=rd;++i,++idx)
#endif

using namespace std;
namespace DDF {

const Real PI = (Real)M_PI; 
const Real MININV=1e-8,MININV2=1e-16;
inline Real SQR(DDF::Real x, DDF::Real y, DDF::Real z) { return x*x+y*y+z*z; }
inline Real SQRn(DDF::Real x, DDF::Real y, DDF::Real z) { DDF::Real v=x*x+y*y+z*z; if(v<MININV2){x+=MININV;return x*x+y*y+z*z;} return v; }

#ifdef DEBUG_VORTEXPATH
std::list<LineElement> gVortexPath = std::list<LineElement>();
#endif

// ******************************************************************
//
// VORTEX PARTICLE CONSTANTS
//
// ******************************************************************

const Real VortexParticleGaussian::msLogCutoff = sqrt(3.);  // 3 is for cutoff at 1e-3 of the exponential term

// workaround for missing erf function in win32 MSVC
#ifndef WIN32
const static Real VPGaussian_msAreaMult_erf = erf( VortexParticleGaussian::msLogCutoff);
#else
const static Real VPGaussian_msAreaMult_erf = 0.9953222650; // hard coded value for 1e-4 cutoff
#endif

#if DDF_DIMENSION==3
const Real VortexParticleGaussian::msAreaMult = pow(PI, 1.5) * VPGaussian_msAreaMult_erf * exp(-1.);
#else
const Real VortexParticleGaussian::msAreaMult = 2.* PI * exp(-1.);
#endif
// move particles away from obstacles
const Real VortexParticle::msRepulsion = .5;
// attenuate particle near obstacles, remove amount of strength per timestep
const Real VortexParticle::msAttenuation = 0.1;
// if set to true, magnitude is preserved, otherwise decay is allowed
const bool VortexParticle::msStretchingPreserve = false;

// Default values, may be changed via plugin
Real VortexParticle::msRadiusCascade = 2.;	 // split into particles of new_radius = radius/msRadiusCascade. If this param is small, many weak particles form
Real VortexParticle::msDissipateRadius = 1.; // dissipate particles smaller than this radius
Real VortexParticle::msDecayTime = 450.0;    // factor to time before splitting
Real VortexParticle::msInitialTime = 20.5;   // factor to time before entering inertial subrange
Vec3 VortexParticle::msU0 = Vec3(0.);        // velocity scale (inflow speed etc.)
RandomStream VorticitySystem::randStream(13322223);

// ******************************************************************
//
// GENERAL PARTICLE FUNCTIONS
//
// ******************************************************************

// irad is related to the kernel distribution. orad is the cutoff radius
VortexParticle::VortexParticle(const Vec3& p, const Vec3& s, Real irad, Real orad)
	: mPos(p),mStrength(s),mForce(0.),mFlags(0),mIRad(irad),mORad(orad), mAttenuate(0.)
{
	// time before entering inertial subrange
	Real meantime = msInitialTime * pow(norm(msU0),-1./3.);
	mTime = meantime * (1. + .5*(2.*VorticitySystem::rnd()-1.) );
}

inline void VortexParticle::advance(const Vec3& v)
{
#if DEBUG_VORTEXPATH==1
	LineElement le;
	le.p0 = mPos;
	mPos += v;
	le.p1 = mPos;
	Real c = std::max(std::max(fabs(mStrength.x),fabs(mStrength.y)),fabs(mStrength.z));
	le.r = fabs(mStrength.x)/c;
	le.g = fabs(mStrength.y)/c;
	le.b = fabs(mStrength.z)/c;
	le.a = 1.0;
	le.s = mIRad;
	gVortexPath.push_front(le);
#else
	mPos += v;
#endif
}
		
// calculate velocity gradient matrix at particles position by trilinear interpolation
inline Vec3 velGradient(Grid<Vec3>* grid, const Vec3& pos, int idx)
{
	Vec3 res;
	for (int n=0;n<3;n++)
	{
		// translate grid (MAC)
		Vec3 cpos (pos);
		cpos[n] += .5; cpos[idx] -= 0.5;
		nVec3i id(vec2I(cpos)), nid(id+1), nd(id), nnd(nid);
		Vec3 a (cpos-vec2R(id)), na (-a+1.);
		nd[n]++; nnd[n]++;
				
		// trilinear interpolation
		res[n] = na.x * na.y * na.z * ( (*grid)( nd.x, nd.y, nd.z)[idx]-(*grid)( id.x, id.y, id.z)[idx]) +
					na.x * na.y *  a.z * ( (*grid)( nd.x, nd.y,nnd.z)[idx]-(*grid)( id.x, id.y,nid.z)[idx]) +
					na.x *  a.y * na.z * ( (*grid)( nd.x,nnd.y, nd.z)[idx]-(*grid)( id.x,nid.y, id.z)[idx]) +
					na.x *  a.y *  a.z * ( (*grid)( nd.x,nnd.y,nnd.z)[idx]-(*grid)( id.x,nid.y,nid.z)[idx]) +
					 a.x * na.y * na.z * ( (*grid)(nnd.x, nd.y, nd.z)[idx]-(*grid)(nid.x, id.y, id.z)[idx]) +
					 a.x * na.y *  a.z * ( (*grid)(nnd.x, nd.y,nnd.z)[idx]-(*grid)(nid.x, id.y,nid.z)[idx]) +
					 a.x *  a.y * na.z * ( (*grid)(nnd.x,nnd.y, nd.z)[idx]-(*grid)(nid.x,nid.y, id.z)[idx]) +
					 a.x *  a.y *  a.z * ( (*grid)(nnd.x,nnd.y,nnd.z)[idx]-(*grid)(nid.x,nid.y,nid.z)[idx]);
	}
	return res;
}

// perform vortex stretching
void VortexParticle::vortexStretch(Grid<Vec3>* pVel, Real dt, Real mult)
{
	Vec3 spos = mPos*mult;
	// gradient
	Vec3 s;
	s.x = dot(mStrength,velGradient(pVel,spos,0));
	s.y = dot(mStrength,velGradient(pVel,spos,1));
	s.z = dot(mStrength,velGradient(pVel,spos,2));
		
	Vec3 nstr = mStrength + dt * s;
	Real oldStr2 = normNoSqrt(mStrength);
	Real newStr2 = normNoSqrt(nstr);
	
	// do not allow the magnitude to grow
	if (newStr2 > MININV2 && (newStr2 > oldStr2 || msStretchingPreserve))
		nstr *= sqrt(oldStr2 / newStr2);

	// apply vorticity update
	mStrength = nstr;
}

// reset the timer for decay; used after particle splitting etc.
void VortexParticle::setDecayTimer()
{ 
	Real meantime = msDecayTime * pow(norm(msU0),-1./3.) * ( pow(mIRad,-2./3.) - pow(mIRad * msRadiusCascade,-2./3.)); 
	mTime = meantime * (1. + .5*(2.*VorticitySystem::rnd()-1.) );
}

string VortexParticle::toString() {
	std::ostringstream out;
	out<< " pos="<<mPos<<" str="<<mStrength<<" radi="<<mIRad<<","<<mORad;
	return out.str();
}


// ******************************************************************
//
// GAUSSIAN PARTICLE  [psi = a*b/2/exp(-r^2/b)]
//
// Kernel used in the paper with b := 2 sqrt(sigma)
//
// ******************************************************************

// ATTENTION : For the 2D kernel, some derivations are obsolete. Better doublecheck before using 2D !!

static float gAvgStr=0., gAvgStrCnt=0.;
VortexParticleGaussian::VortexParticleGaussian(const Vec3& pos, const Vec3& strength, Real irad)
	: VortexParticle(pos,strength,irad, msLogCutoff*irad)
{
	mSigma = irad*irad; // w is zero at b=irad^2

	// measure average strengths
	float str = norm(strength);
	gAvgStr += str;
	gAvgStrCnt += 1.;

	printf("\n-> new particle : radius %g/%g strength [%g %g %g], avg %g,%g\n ",mIRad,mORad,strength.x,strength.y,strength.z, (gAvgStr/gAvgStrCnt), gAvgStrCnt);	
}

// Overlay desired vorticity of all kernels in temporary global grid
// using eq.13 in the paper
void VortexParticleGaussian::buildReference(Grid<Vec3>* pTemp, FlagGrid* flags, Real mult, bool doForces)
{
	// precompute frequently used constants
	int irad = 1+(int)ceilf(mIRad*mult);
	Real mult2=mult*mult, irad2 = mIRad*mIRad*mult2, isig=-1.0/(mSigma*mult2);
	Vec3 spos = mPos*mult;
	nVec3i apos = vec2I(spos);
	Vec3 subpos = vec2R(apos)-spos;
	if (gDim==2) {subpos.z = 0; apos.z = (int)mPos.z;}
	Vec3 str = mStrength * (2.0 / mSigma);
	Vec3 ndir(mStrength);
	Real strength = normalize(ndir);

	// construct kernel
	int totalSum=0, obsSum=0,freeSum=0;
	Vec3 center(0.);
	FOR_KERNEL(irad)
	{
		int x=apos.x+i, y=apos.y+j, z=apos.z+k;
		Vec3 pa(subpos.x+i,subpos.y+j,subpos.z+k);
		Real r2= SQR(pa.x,pa.y,pa.z);
		if (r2 < irad2)
		{
			if (flags->checkIndexValid(x,y,z))
			{
				// Evaluate Kernel
				Real dz= dot(pa,ndir), rho2 = r2 - dz*dz;
				Real gauss = (-rho2 + mSigma) * exp(isig * r2);
				if (gauss>0)
					(*pTemp)(x,y,z) += str * gauss;
				
				// Calculate overlap with air or obstacles
				if (!doForces) continue;
				if (fgIsEmpty(flags->getGlobal(x,y,z))) 
					freeSum++;
				if (fgIsObstacle(flags->getGlobal(x,y,z)))
				{
					obsSum++;
					center += Vec3(x,y,z);
				}
				// Delete particles at inflow
				if ((flags->getGlobal(x,y,z) & FINFLOW) != 0) {markDelete();}
				totalSum++;
			}
			else {markDelete();}
		}
	}

	mForce=0.;
	mAttenuate=0.;
	// Calculate repulsion force and delete invalid particles
	if (doForces)
	{
		// Delete below dissipation radius
		if (totalSum <= 1 || mIRad <= msDissipateRadius) 
			{markDelete();  }
		else {
			Real obsPart = (Real)obsSum/(Real)totalSum, freePart = (Real)freeSum/(Real)totalSum;
			// Delete particles in the air or in obstacles
			if (freePart > 0.5 || obsPart > 0.7)
				{markDelete();}
			else if (obsSum != 0 || freeSum != 0)
			{
				// Derive origin of repulsion force					
				Vec3 diff = spos - (center/obsSum);
				normalize(diff);
				mForce = obsPart * diff;
				mAttenuate = (obsPart + freePart) * msAttenuation;
#if DDF_DIMENSION==2
				mForce.z = 0.;
#endif
			}
			/*std::cout << "totalSum " << totalSum << " obsPart " << obsPart << " freePart " << freePart << std::endl;
			std::cout << "force : " << mForce << " attenuate " << mAttenuate << std::endl;*/
		}		
	}
}

// merge current particle with particle v
// total vorticity and energy per volume are conserved
bool VortexParticleGaussian::merge(VortexParticle* v)
{
#if DDF_DIMENSION==2
	Real aA = mIRad*mIRad, bA = v->getRadius()*v->getRadius();
#else
	Real aA = mIRad*mIRad*mIRad, bA = v->getRadius()*v->getRadius()*v->getRadius();
#endif
	//printf("\n* ----------------  MERGING\n[%g %g %g] r%g + [%g %g %g] r%g",mStrength.x,mStrength.y,mStrength.z,mIRad,v->getStrength().x,v->getStrength().y,v->getStrength().z,v->getRadius());

	// derive new magnitude by conservation laws
	const Vec3& aS = mStrength, bS = v->getStrength();
	Real midRatio = 0.5 * (norm(aS)/mIRad + norm(bS)/v->getRadius());
	Real magnitude = norm (aA * aS + bA * bS);

	// new radius and volume
	Real r = pow ( magnitude/midRatio, 1./(1.+(Real)gDim));
	Real A = (gDim==2) ? (r*r) : (r*r*r);
	
	// obtain direction by weighted average
	mStrength = aS * (aA/A) + bS * (bA/A);
	mIRad = r;
	mORad = msLogCutoff * mIRad;
	mSigma = r*r;

	// average timers, enter inertial subrange
	if ((v->getFlags() & FInertial) != 0) mFlags |= FInertial;
	mTime = 0.5 * (mTime + v->getTime());

	//printf(" -> [%g %g %g] r%g\n\n",mStrength.x,mStrength.y,mStrength.z,mIRad);
	return true;
}

// Split particle
// total vorticity and energy per volume are conserved
VortexParticle* VortexParticleGaussian::split()
{
	// obtain new radius depending on the chosen cascade level
	Real bMult = 1.0 / (msRadiusCascade*msRadiusCascade);
	// adapt strength using Kolmogorov law
	Real aMult = sqrt(0.5) * pow(msRadiusCascade,(Real)gDim-5./6.);

	// set new radii and strength
	mSigma *= bMult;
	mIRad = sqrt(mSigma);
	mORad = msLogCutoff * mIRad;
	mStrength *= aMult;
	
	VortexParticle *p = new VortexParticleGaussian(mPos, mStrength, mIRad);
	p->getFlags() |= FInertial; // after splitting, always enter inertial subrange

	return p;
}

// velocity field induces by the kernel
// (eq. 12 in the paper)
// helper function for applyForce()
inline Vec3 gaussianKernel(const Vec3& pa, Real isig, Real magnitude,const Vec3& sdir)
{
	Real r2 = SQR(pa.x,pa.y,pa.z);
	Real z = dot(pa,sdir), z2=z*z;
	Real rho2 = r2-z2;
	if (rho2 > MININV2 && MININV2)
	{
		Vec3 ephi = cross(sdir,pa);
		Real s = magnitude * sqrt(rho2/r2) * exp(r2*isig);
		return ephi*s;
	}
	return Vec3(0.);
}

// Apply the kernel to the velocity field
void VortexParticleGaussian::applyForce(
		Grid<Vec3>* pVel,Grid<Vec3>* pVort,Grid<Vec3>* pTemp, FlagGrid* flags, 
		Real mult, Real fadein, bool doForces, Grid<Vec3>* velHelper)
{
	// precalculate frequently used constants
	int irad = 1+(int)ceilf(mIRad*mult);
	Real mult2=mult*mult, irad2 = mIRad*mIRad*mult2;
	Vec3 spos = mPos*mult;
	nVec3i apos = vec2I(spos);
	Vec3 subpos = vec2R(apos)-spos;
	if (gDim==2) {subpos.z = 0; apos.z = (int)mPos.z;}
	Vec3 ndir(mStrength);
	Real strength=normalize(ndir);
	if (strength==0) return;
	
	// integrate vorticity field of the kernel to obtain weight w_k
	Real summRef=0,summDiff=0;
	FOR_KERNEL(irad)
	{
		int x=apos.x+i, y=apos.y+j, z=apos.z+k;
		if (flags->checkIndexValid(x,y,z))
		{
			Real r2= SQR(subpos.x+i,subpos.y+j,subpos.z+k);
			if (r2 < irad2)
			{
				Real wRef = dot((*pTemp)(x,y,z),ndir);
				Real wCur = dot((*pVort)(x,y,z),ndir);
				
				summRef += wRef;
				summDiff += wRef-wCur;
			}
 		}
	}
	
	// don't correct backwards, will cause oscillations
	if (summRef >0 && summDiff > 0)
	{
		Real percentage = summDiff/summRef;
		// Only evaluate if deviation from measured vorticity is not neglectable to save comp. time
		if (percentage > 0.05) 
		{
			if (percentage > 1.) percentage = 1.; // no jumps > 100%
			Real magnitude = percentage*strength;
		
			// particle fade-in
			//debMsg("VortexParticleGaussian","Fadein="<< fadein ); // debug
			// NT test if ((mFlags & FInertial)==0 && mTime < fadein * mIRad)
			if (mTime < fadein * mIRad)
				magnitude *= mTime / (fadein*mIRad);
			
			// precalculate
			int orad = 1+(int)ceilf(mORad*mult);
			Real orad2=mORad*mORad*mult2, isig=-1.0/(mSigma*mult2);
		
			// apply kernel
			FOR_KERNEL(orad)
			{
				int x=apos.x+i, y=apos.y+j, z=apos.z+k;
				if (flags->checkIndexValid(x,y,z) && !fgIsObstacle(flags->getGlobal(x,y,z)))
				{
					Vec3 pa(subpos.x+i,subpos.y+j,subpos.z+k), v(0.);	
					if (SQR(pa.x,pa.y,pa.z) < orad2)
					{
						// x direction				
						if (flags->checkIndexValid(x-1,y,z) && fgIsFluid(flags->getGlobal(x-1,y,z)))
						{
							Vec3 vp = gaussianKernel(Vec3(pa.x-.5,pa.y,pa.z), isig, magnitude, ndir);
							v.x = vp.x;					
						}
						// y direction				
						if (flags->checkIndexValid(x,y-1,z) && fgIsFluid(flags->getGlobal(x,y-1,z)))
						{
							Vec3 vp = gaussianKernel(Vec3(pa.x,pa.y-.5,pa.z), isig, magnitude, ndir);
							v.y = vp.y;					
						}
		#if DDF_DIMENSION==3
						// z direction				
						if (flags->checkIndexValid(x,y,z-1) && fgIsFluid(flags->getGlobal(x,y,z-1)))
						{
							Vec3 vp = gaussianKernel(Vec3(pa.x,pa.y,pa.z-.5), isig, magnitude, ndir);
							v.z = vp.z;
						}
		#endif
						(*pVel)(x,y,z) +=v;
						if(velHelper)	velHelper->getGlobal(x,y,z) += v;
					}
				}
			}
		}
	}
	
	// process attenuation for particles trapped in obstacle
	// shrink radius and strength, such that str/R = const
	if (doForces)
	{
		if (mAttenuate > 1e-4)
		{
			Real perc = 1.-mAttenuate;
			scale(pow(perc,1./((Real)gDim+1.)));
		}
	}
}

// uniform scaling of the particle
void VortexParticleGaussian::scale(Real fac)
{
	mIRad *= fac;
	mORad *= fac;
	mSigma *= fac*fac;
	mStrength *= fac;
}


// ******************************************************************
//
// VORTICITY SYSTEM
//
// ******************************************************************

// apply the particles velocity kernel onto the grid
void VorticitySystem::applyForces(Grid<Vec3>* pVel, Grid<Vec3>* pVort, Grid<Vec3>* pTemp,FlagGrid* flags, Real mult, bool doForces,
		Grid<Vec3>* velHelper)
{
	std::cout << "----------------------" << mParts.size() <<" parts------------------------" << std::endl;

	const VList::iterator it_end = mParts.end();
	for (VList::iterator it=mParts.begin(); it != it_end; ++it) 
	{
		(*it)->buildReference(pTemp, flags, mult, doForces);
	}	
	for (VList::iterator it=mParts.begin(); it != it_end; ++it) 	
	{
		if ((*it)->isDeleted()) continue;
		(*it)->applyForce(pVel,pVort,pTemp,flags,
				mult, msFadeIn, doForces,velHelper);
	}	
}

// perform merge, split and dissipation of particles
void VorticitySystem::merge(FlagGrid* flags, Grid<Real>* ndist, Grid<Vec3>* vel,Real dt, Real mergeDist)
{
	Real mergeDist2 = mergeDist*mergeDist;

	// Update timer, split
	for (VList::iterator it=mParts.begin(); it != mParts.end(); ++it) 
	{
		VortexParticle *p = *it;
		if (p->isDeleted()) continue;
		p->getTime() -= dt;
		if (VortexParticle::doDecay() && p->getTime() <= 0)
		{
			// dissipatation
			if (p->getRadius() <= VortexParticle::msDissipateRadius)
				{p->markDelete(); }
			// particle splitting
			else if ((p->getFlags() & VortexParticle::FInertial) != 0)
			{
				//printf("split before : %g,%g,%g / r%g\n",p->getStrength().x,p->getStrength().y,p->getStrength().z,p->getRadius());
				VortexParticle* pn = p->split();
				pn->setDecayTimer();
				pn->getFlags() |= VortexParticle::FInertial;
				
				// slightly modify new particle position, so that they don't lump together
				Vec3 ndir (rnd(),rnd(),rnd());
				Real nDP1 = sqrt(p->getRadius()) * (0.5+1.*rnd());
				Real nDP3 = 3. * dt*norm(mpFsolver->interpolateVpVelocity(p->getPos(), vel));
				ndir *= std::max(std::min(nDP1,nDP3),(Real)1.2) / norm(ndir);
				if (gDim==2) ndir.z=0;
				pn->advance(ndir);
				p->advance(-ndir);
				
				// slightly modify new particle axis, so that they don't lump together				
#if DDF_DIMENSION==3
				Vec3 nang (rnd(),rnd(),rnd());
				Real str = norm(p->getStrength());
				nang *= 0.1 * str / norm(nang);
				Vec3 pnew = (p->getStrength()+nang);
				Real pnewn = norm(pnew);
				if (pnewn > MININV) 
					p->getStrength() = pnew * (str / pnewn);
				pnew = (pn->getStrength()-nang);
				pnewn = norm(pnew);
				if (pnewn > MININV) 
					pn->getStrength() = pnew * (str / pnewn);
#endif
				mParts.insert(it,pn);

				//printf("split 1: %g,%g,%g / r%g\n",p->getStrength().x,p->getStrength().y,p->getStrength().z,p->getRadius());
				//printf("split 2 : %g,%g,%g / r%g\n",pn->getStrength().x,pn->getStrength().y,pn->getStrength().z,pn->getRadius());				
			}
			p->getFlags() |= VortexParticle::FInertial;
			p->setDecayTimer();
		}
		else if(VortexParticle::doDecay() && (p->getFlags() & VortexParticle::FInertial) == 0)
		{
			// transfer to inertial range if particle leaves near-wall region
			nVec3i pos(vec2I(p->getPos()+.5));			
			if ((*ndist).getGlobal(pos.x,pos.y,pos.z) == 0)
			{
				Real timeLeft = p->getTime();
				p->getFlags() |= VortexParticle::FInertial;
				p->setDecayTimer();
				p->getTime() = 0.5* (p->getTime() + timeLeft);
			}
		}
	}

	// Particle merging
	const VList::iterator it_end = mParts.end();	
	for (VList::iterator it=mParts.begin(); it != it_end; ++it) 
	{
		VortexParticle* p1 = *it;
		if (!p1->canMerge()) continue;
		VList::iterator it2=it;
		for (++it2; it2 != it_end; ++it2) 	
		{
			VortexParticle* p2 = *it2;
			Real d2 = normNoSqrt(p1->getPos()-p2->getPos());
			// Merge if distance smaller then merge radius
			if (p2->canMerge() &&
				d2 < (p1->getMergeR2()+ p2->getMergeR2())*mergeDist2 && 
				d2 < 4.*min(p1->getMergeR2(), p2->getMergeR2())*mergeDist2)
			{
				// Merging probability scales with axis alignment of the two particles
				Real angle = acos(dot(p1->getStrength(),p2->getStrength())/(norm(p1->getStrength())*norm(p2->getStrength())));
				Real prob = 1.-angle/PI;
				Real rand = rnd();
				if (rand >= prob) continue;
			
				Real dist = norm(p1->getPos()-p2->getPos());
				if (p1->merge(p2))
				{
					p2->markDelete(); cout << "MRG - " << p1->getPos() << " d " << dist << endl;
					break;
				}
			}
		}
	}

	// remove particles marked for deletation
	for (VList::iterator it=mParts.begin(); it != mParts.end();) 
		if ((*it)->isDeleted())
		{
			delete *it;
			it = mParts.erase(it);		
		}
		else
			++it;
}

// particle advection and vortex stretching
void VorticitySystem::advectParticles(Grid<Vec3>* pVel, FlagGrid* flags, Real dt, Real mult)
{
	const int numParticles = mParts.size();

#if DEBUG_VORTEXPATH==1
	for (std::list<LineElement>::iterator it=gVortexPath.begin(); it != gVortexPath.end();)
	{
		it->a -= 0.015;
		if (it->a < 0)
			it = gVortexPath.erase(it);
		else
			++it;
	}
#endif
	
	{
		int i=0;
		const VList::iterator it_end = mParts.end(); 
		for (VList::iterator it=mParts.begin(); it != it_end;++i) 
		{
			VortexParticle& p = *(*it);
			// Second-order Heun/RK2 step
			Vec3 pos1 = p.getPos();
			Vec3 v1 = mpFsolver->interpolateVpVelocity(pos1*mult, pVel);
			Vec3 pos2 = pos1 + v1*dt;
			Vec3 v2 = mpFsolver->interpolateVpVelocity(pos2*mult, pVel);
			Vec3 v = (v1+v2)*0.5;
			// remove deleted particles (only used if no merge plugin is invoked)
			if (p.isDeleted() )
			{
				delete *it;
				it = mParts.erase(it);
			}
			else
			{
				// update vorticity (vortex stretching)
#if DDF_DIMENSION==3
				p.vortexStretch(pVel, dt, mult);
#endif
				// update position
				v += dt*p.getForce() * VortexParticle::msRepulsion;
				p.advance(v*dt);
				++it;
			}
		}
	}
}


// constructor
VorticitySystem::VorticitySystem(FluidSolver *fs) : 
		mpFsolver(fs)
{ 
	VortexParticle::msU0 = fs->getParams()->mU0;

	// init defaults
	msFadeIn = 2.;
};

// destructor
VorticitySystem::~VorticitySystem()
{
	for (VList::iterator it=mParts.begin(); it != mParts.end();) {
		delete *it;
		it = mParts.erase(it);
	}
}

} // namespace DDF

