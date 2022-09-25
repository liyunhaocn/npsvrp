#include <time.h>
#include <iostream>

#include "hgs/Genetic.h"
#include "hgs/commandline.h"
#include "hgs/LocalSearch.h"
#include "hgs/Split.h"
#include "hgs/Params.h"
#include "hgs/Population.h"
#include "hgs/Individual.h"

#include "smart/Solver.h"
#include "smart/Goal.h"
#include "smart/Problem.h"
#include "smart/Flag.h"

namespace hust {

	bool allocGlobalMem( hust::LL seed ) {
		//bool allocGlobalMem(std::string inpath) {

		globalCfg = new hust::Configuration();
		//globalCfg->solveCommandLine(argc, argv);
		globalCfg->seed = seed;

		//globalCfg->seed = 1645192521;
		//globalCfg->seed = 1645192521;
		//globalCfg->seed = 1645199481;

		myRand = new Random(globalCfg->seed);
		myRandX = new RandomX(globalCfg->seed);

		ERROR("globalCfg->seed:", globalCfg->seed, " ins:", globalInput->example);

		globalCfg->show();

		// TODO[lyh][0]:一定要记得globalCfg用cusCnt合法化一下
		globalCfg->repairByCusCnt(globalInput->custCnt);

		yearTable = new hust::util::Array2D<int>(globalInput->custCnt + 1, globalInput->custCnt + 1, 0);

		bks = new BKS();
		gloalTimer = new Timer(globalCfg->runTimer);

		return true;
	}

	bool deallocGlobalMem() {

		delete myRand;
		delete myRandX;
		delete yearTable;
		delete globalCfg;
		delete globalInput;
		delete bks;
		delete gloalTimer;

		return true;
	}

}//namespace hust

#if 1

int main(int argc, char* argv[])
{
	try
	{
		// Reading the arguments of the program
		CommandLine commandline(argc, argv);

		// Reading the data file and initializing some data structures
		std::cout << "----- READING DATA SET FROM: " << commandline.config.pathInstance << std::endl;
		Params params(commandline);
		//std::cout << "111111" << commandline.config.pathInstance << std::endl;

		// Creating the Split and Local Search structures
		Split split(&params);
		LocalSearch localSearch(&params);

		hust::globalInput = new hust::Input(params);

		hust::allocGlobalMem(params.config.seed);
		hust::globalInput->initInput();
		
		hust::Solver smartSolver;
		//smartSolver.initSolution(0);
		//smartSolver.minimizeRN(2);
		//smartSolver.mRLLocalSearch(0, {});

		hust::Goal goal;

		goal.test();
		//goal.callSimulatedannealing();
		//goal.TwoAlgCombine();

		//goal.experOnMinRN();

		hust::deallocGlobalMem();

		return 0;
 
		// Initial population
		std::cout << "----- INSTANCE LOADED WITH " << params.nbClients << " CLIENTS AND " << params.nbVehicles << " VEHICLES" << std::endl;
		std::cout << "----- BUILDING INITIAL POPULATION" << std::endl;
		Population population(&params, &split, &localSearch);

		// Genetic algorithm
		std::cout << "----- STARTING GENETIC ALGORITHM" << std::endl;
		Genetic solver(&params, &split, &population, &localSearch);
		
		//Individual* offspring = solver.bestOfSREXAndOXCrossovers(population.getNonIdenticalParentsBinaryTournament());
		//Individual* offspring = population.getBestFound();

		//hust::Solver smartSolver;
		//smartSolver.initByArr2(offspring->chromR);
		////smartSolver.initSolution(0);
		//smartSolver.mRLLocalSearch(0, {});

		//return 0;

		solver.run(commandline.config.nbIter, commandline.config.timeLimit);
		std::cout << "----- GENETIC ALGORITHM FINISHED, TIME SPENT: " << params.getTimeElapsedSeconds() << std::endl;

		// Export the best solution, if it exist
		if (population.getBestFound() != nullptr)
		{
			population.getBestFound()->exportCVRPLibFormat(commandline.config.pathSolution);
			population.exportSearchProgress(commandline.config.pathSolution + ".PG.csv", commandline.config.pathInstance, commandline.config.seed);
			if (commandline.config.pathBKS != "")
			{
				population.exportBKS(commandline.config.pathBKS);
			}
		}
	}

	// Catch exceptions
	catch (const std::string& e)
	{
		std::cout << "EXCEPTION | " << e << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << "EXCEPTION | " << e.what() << std::endl;
	}

	// Return 0 if the program execution was successfull
	return 0;
}

#else

// Main class of the algorithm. Used to read from the parameters from the command line,
// create the structures and initial population, and run the hybrid genetic search
int main(int argc, char* argv[])
{
	try
	{
		// Reading the arguments of the program
		CommandLine commandline(argc, argv);

		// Reading the data file and initializing some data structures
		std::cout << "----- READING DATA SET FROM: " << commandline.config.pathInstance << std::endl;
		Params params(commandline);

		// Creating the Split and Local Search structures
		Split split(&params);
		LocalSearch localSearch(&params);

		// Initial population
		std::cout << "----- INSTANCE LOADED WITH " << params.nbClients << " CLIENTS AND " << params.nbVehicles << " VEHICLES" << std::endl;
		std::cout << "----- BUILDING INITIAL POPULATION" << std::endl;
		Population population(&params, &split, &localSearch);

		// Genetic algorithm
		std::cout << "----- STARTING GENETIC ALGORITHM" << std::endl;
		Genetic solver(&params, &split, &population, &localSearch);
		solver.run(commandline.config.nbIter, commandline.config.timeLimit);
		std::cout << "----- GENETIC ALGORITHM FINISHED, TIME SPENT: " << params.getTimeElapsedSeconds() << std::endl;

		// Export the best solution, if it exist
		if (population.getBestFound() != nullptr)
		{
			population.getBestFound()->exportCVRPLibFormat(commandline.config.pathSolution);
			population.exportSearchProgress(commandline.config.pathSolution + ".PG.csv", commandline.config.pathInstance, commandline.config.seed);
			if (commandline.config.pathBKS != "")
			{
				population.exportBKS(commandline.config.pathBKS);
			}
		}
	}

	// Catch exceptions
	catch (const std::string& e)
	{ 
		std::cout << "EXCEPTION | " << e << std::endl;
	}
	catch (const std::exception& e)
	{ 
		std::cout << "EXCEPTION | " << e.what() << std::endl; 
	}

	// Return 0 if the program execution was successfull
	return 0;
}
#endif