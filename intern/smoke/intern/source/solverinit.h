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

#ifndef DDFSOLVERINIT_H

#include "fluidsolver.h"
#include "paramset.h"

namespace DDF
{
	template<class T> class PluginArgumentElement {
		public:
		PluginArgumentElement(const std::string& name, const T& value) : mName(name), mValue(value) {}
		
		std::string mName;
		T mValue;
	};

	// shortcuts
	typedef PluginArgumentElement<string>  StringArg;
	typedef PluginArgumentElement<int>     IntArg;
	typedef PluginArgumentElement<Real>    RealArg;
	typedef PluginArgumentElement<Vec3>    VecArg;

	// helper class to easily link plugin arguments
	class PluginArgument {
		public:
		PluginArgument() {};
		template<class T> PluginArgument(const PluginArgumentElement<T>& pe);
		
		ParamSet mParam;
	};
	PluginArgument operator+ (const PluginArgument&, const PluginArgument&);
	
	
	// Wrapper class for a solver
	class SolverObject
	{
		public:
		SolverObject(const std::string& name, const nVec3i& gridSize);
		SolverObject(const std::string& name, const std::string& flagGridFile);
		SolverObject(const std::string& name, const SolverObject& source, int multiplier);
		SolverObject(const string& name, Grid<int> *flags);
		~SolverObject();

		bool performStep();
		void finalize();
		
		void createIntGrid(const std::string& name, int gridFlags = 0);
		void createRealGrid(const std::string& name, int gridFlags = 0);
		void createVec3Grid(const std::string& name, int gridFlags = 0);
		void createIntGrid(const std::string& name, Grid<int> *grid);
		void createRealGrid(const std::string& name, Grid<Real> *grid);
		void createVec3Grid(const std::string& name, Grid<Vec3> *grid);
		void createNoiseField(const std::string& name, const Vec3& posOffset, const Vec3& posScale, Real valOffset, Real valScale, Real timeAnim);
		void addPlugin(const std::string& name, const PluginArgument& arg = PluginArgument());
		void addInitPlugin(const std::string& name, const PluginArgument& arg = PluginArgument());
		void addEndPlugin(const std::string& name, const PluginArgument& arg = PluginArgument());
		SolverParams& getParams() { return *(mSolver->getParams()); }
		const SolverParams& getParams() const { return *(mSolver->getParams()); }
		void forceInit();
		void addStandardSolverGrids();

		protected:
		void init(const std::string& name, const nVec3i& gridSize);
		SolverPlugin* createPlugin(const std::string& name, const PluginArgument&);
		
		FluidSolver* mSolver;
		std::vector<SolverPlugin*> mInitPlugins, mPlugins, mEndPlugins;
		bool mStarted;
	};
};

#endif
