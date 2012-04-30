/******************************************************************************
 *  DDF Fluid solver with Turbulence extensions
 *
 *  copyright 2009 Nils Thuerey, Tobias Pfaff
 * 
 *  DDF is free software, distributed under the GNU General Public License (GPL v2).
 *  See the file COPYING for more information.
 *
 * Class to setup the fluid solver (alternative to Parser/lexer)
 *
 *****************************************************************************/


#include "solverinit.h"
#include "paramset.h"
#include "solverplugin.h"
#include "waveletnoise.h"
#include <map>

// global solver lists for GLUTGUI
extern map<string, DDF::FluidSolver*> gFluidSolvers;
extern DDF::FluidSolver *gpFsolver;
extern map<string, WAVELETNOISE::WaveletNoiseField*> gNoiseFields;

namespace DDF {
#if DDF_GLUTGUI == 1
extern ParamSet gGlutGuiParams;
#endif

// declaration from fileio.cpp
extern nVec3i getUniversalGridSize(const string& file);

//******************************************************************************
// PluginArgument class

template<> PluginArgument::PluginArgument(const IntArg& pe) {
	mParam.AddInt(pe.mName, pe.mValue);
}
template<> PluginArgument::PluginArgument(const RealArg& pe) {
	mParam.AddFloat(pe.mName, pe.mValue);
}
template<> PluginArgument::PluginArgument(const StringArg& pe) {
	mParam.AddString(pe.mName, pe.mValue);
}
template<> PluginArgument::PluginArgument(const VecArg& pe) {
	mParam.AddVector(pe.mName, pe.mValue);
}

PluginArgument operator+ (const PluginArgument& t, const PluginArgument& s)
{
	PluginArgument n;
	n.mParam.AddSet(t.mParam);
	n.mParam.AddSet(s.mParam);
	return n;
}

//******************************************************************************
// SolverObject class

SolverObject::SolverObject(const string& name, const nVec3i& gridSize)
{
	init(name, gridSize);
}

SolverObject::SolverObject(const string& name, const std::string& flagGridFile)
{
	nVec3i size = getUniversalGridSize(flagGridFile);
	init(name, size);
	addInitPlugin ("load-universal", StringArg("grid","flags") + StringArg("file",flagGridFile));
}

SolverObject::SolverObject(const std::string& name, const SolverObject& source, int multiplier)
{
	nVec3i size = source.getParams().getGridSize() * multiplier;
	init(name, size);
	getParams().setMultiplier(multiplier);
	getParams().adaptInit(&source.getParams());
}

SolverObject::SolverObject(const string& name, Grid<int> *flags)
{
	nVec3i size = flags->getSize();
	init(name, size);
	createIntGrid("flags", flags);	
}

SolverObject::~SolverObject()
{
	delete mSolver;
}

// initialize default solver params
void SolverObject::init(const string& name, const nVec3i& size)
{
	ParamSet par;
	par.AddInt("host-vorticity-system", 1);
	par.AddFloat("timestep", 1.0);
	par.AddVector("gridsize", vec2R(size));
	par.AddFloat("cg-max-iter-fac", 0.30);
	par.AddFloat("cg-accuracy", 0.001);
		// setup plugin lists
	mInitPlugins.clear();
	mPlugins.clear();
	mEndPlugins.clear();
	mStarted = false;

	// create solver
	SolverParams* solverParam = new SolverParams("p_" + name);
	solverParam->initFromParamSet(par);
	mSolver = new FluidSolver("s_" + name);
	mSolver->setParams(solverParam);

	// create default grids
	createIntGrid("flags");
	createVec3Grid("vel-curr",false);
	createRealGrid("pressure",false);

	// globally register solver
	gFluidSolvers[mSolver->getName()] = mSolver;
	if (gpFsolver == NULL) gpFsolver = mSolver;

	// setup GUI params
	 #if DDF_GLUTGUI == 1
	gGlutGuiParams.AddString("solvername", gpFsolver->getName());
	#endif
}

void SolverObject::forceInit() {
	if (!mStarted) {
		// start solver
		mSolver->initFluid();
		mSolver->addInitPlugins(mInitPlugins);
		mSolver->addPlugins(mPlugins);
		mSolver->addEndPlugins(mEndPlugins);
		mSolver->runInitPlugins();
		mSolver->setSolverInited(true);
		mStarted = true;
	}
}

void SolverObject::addStandardSolverGrids()
{
	createVec3Grid("vec-temp",true);
	createRealGrid("real-temp",true);
	createRealGrid("temp1",true);
	createRealGrid("temp2",true);
	createRealGrid("tmp",true);	
	createRealGrid("residual",true);
	createRealGrid("rhs",true);
	createRealGrid("search",true);
	createRealGrid("A0",true);
	createRealGrid("Ai",true);
	createRealGrid("Aj",true);
	createRealGrid("Ak",true);
}

void SolverObject::finalize()
{
	mSolver->finalize();
}

bool SolverObject::performStep()
{
	if (!mStarted) forceInit();
	for(int i=0; i< mSolver->getParams()->mStepFactor; i++)
		mSolver->simulateFluid();
	
	return mSolver->getParams()->getQuit();
}

void SolverObject::createIntGrid(const std::string& name, int gridFlags)
{
	const nVec3i gridsize = mSolver->getParams()->getGridSize();	
	Grid<int> *grid = new Grid<int>(name);
	grid->initGridMem(gridsize[0], gridsize[1], gridsize[2]);
	mSolver->getParams()->mGridsInt[name] = grid;
	grid->setGridFlags(gridFlags);
	grid->setDisplayFlags(/* hideInGui */ 1);
}

void SolverObject::createRealGrid(const std::string& name, int gridFlags)
{
	const nVec3i gridsize = mSolver->getParams()->getGridSize();	
	Grid<Real> *grid = new Grid<Real>(name);
	grid->initGridMem(gridsize[0], gridsize[1], gridsize[2]);
	mSolver->getParams()->mGridsReal[name] = grid;
	grid->setGridFlags(gridFlags);
	grid->setDisplayFlags(/* hideInGui */ 1);
}

void SolverObject::createVec3Grid(const std::string& name, int gridFlags)
{
	const nVec3i gridsize = mSolver->getParams()->getGridSize();	
	Grid<Vec3> *grid = new Grid<Vec3>(name);
	grid->initGridMem(gridsize[0], gridsize[1], gridsize[2]);
	mSolver->getParams()->mGridsVec3[name] = grid;
	grid->setGridFlags(gridFlags);
	grid->setDisplayFlags(/* hideInGui */ 1);
}

/* Use existing arrays */
void SolverObject::createIntGrid(const std::string& name, Grid<int> *grid)
{
	grid->setName(name);
	mSolver->getParams()->mGridsInt[grid->getName()] = grid;
}

void SolverObject::createRealGrid(const std::string& name, Grid<Real> *grid)
{
	grid->setName(name);
	mSolver->getParams()->mGridsReal[grid->getName()] = grid;
}

void SolverObject::createVec3Grid(const std::string& name, Grid<Vec3> *grid)
{
	grid->setName(name);
	mSolver->getParams()->mGridsVec3[grid->getName()] = grid;
}

void SolverObject::createNoiseField(const string& name, const Vec3& posOffset, const Vec3& posScale, Real valOffset, Real valScale, Real timeAnim)
{
	// set params
	ParamSet p;
	p.AddVector("pos-offset", posOffset);
	p.AddVector("pos-scale", posScale);
	p.AddFloat("val-offset", valOffset);
	p.AddFloat("val-scale", valScale);
	p.AddFloat("time-anim", timeAnim);
	p.AddBool("clamp",true);
	p.AddFloat("clamp-neg",0.);
	p.AddFloat("clamp-pos",1.);
	
	WAVELETNOISE::WaveletNoiseField *f = new WAVELETNOISE::WaveletNoiseField(name, p, getParams().getGridSize());
	p.ReportUnused();
	
	// register globally
	gNoiseFields[name] = f;
	getParams().addNoiseField(f, name);
}

SolverPlugin* SolverObject::createPlugin(const std::string& name, const PluginArgument& p)
{
	SolverPlugin* plugin = SolverPlugin::makePlugin(name);
	if (!plugin)
		errFatal("SolverObject::createPlugin","Can't create plugin " << name, SIMWORLD_ERRPARSE);
	
	plugin->setPluginParams(&getParams());
	plugin->SolverPlugin::parseParams(p.mParam);
	if (!plugin->parseParams(p.mParam))
		errFatal("SolverObject::createPlugin","Error parsing arguments of plugin " << name, SIMWORLD_ERRPARSE);
	
	p.mParam.ReportUnused();
	return plugin;
}

void SolverObject::addPlugin(const std::string& name, const PluginArgument& p) {
	mPlugins.push_back(createPlugin(name, p));
}
void SolverObject::addInitPlugin(const std::string& name, const PluginArgument& p) {
	mInitPlugins.push_back(createPlugin(name, p));
}
void SolverObject::addEndPlugin(const std::string& name, const PluginArgument& p) {
	mEndPlugins.push_back(createPlugin(name, p));
}

} // DDF
