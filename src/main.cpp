#include <time.h>
#include <iostream>
#include <unordered_map>

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

//		globalCfg->seed = 1645192521;
		//globalCfg->seed = 1645192521;
		//globalCfg->seed = 1645199481;

		myRand = new Random(globalCfg->seed);
		myRandX = new RandomX(globalCfg->seed);

		INFO("globalCfg->seed:", globalCfg->seed, " ins:", globalInput->example);

		globalCfg->show();

		// TODO[lyh][0]:一定要记得globalCfg用cusCnt合法化一下
		globalCfg->repairByCusCnt(globalInput->custCnt);

		yearTable = new hust::util::Array2D<int>(globalInput->custCnt + 1, globalInput->custCnt + 1, 0);

		bks = new BKS();
		//gloalTimer = new Timer(globalCfg->runTimer);
		return true;
	}

	bool deallocGlobalMem() {

		delete myRand;
		delete myRandX;
		delete yearTable;
		delete globalCfg;
		delete globalInput;
		delete bks;
		//delete gloalTimer;
        if(hgsLocalSearch!=nullptr) {
            delete hgsLocalSearch;
        }
		return true;
	}

}//namespace hust

//
 //./instances/ORTEC-VRPTW-ASYM-93ee144d-d1-n688-k38.txt -t 30 -seed 1 -veh -1 -useWallClockTime 1  -isNpsRun 0
//./instances/ORTEC-VRPTW-ASYM-92bc6975-d1-n273-k20.txt -t 30 -seed 1 -veh -1 -useWallClockTime 1  -isNpsRun 0
// ./instances/ORTEC-VRPTW-ASYM-8bc13a3f-d1-n421-k40.txt -t 273 -seed 1 -veh -1 -useWallClockTime 1
//203578
std::vector<int> getWeightByNearDelt(Params& params,int nNear){

    std::vector< std::pair<int,int> > order;
    order.reserve(params.nbClients+1);
    order.push_back({0,0});
    std::vector<int> weight(params.nbClients + 1);
    for(int v=1;v<=params.nbClients;++v){
        int vNearSumDeltCost = 0;
        int nbPair = 0;

        for(int wpos=0;wpos<nNear;++wpos){
            int w = hust::globalInput->addSTclose[v][wpos];
            for(int wjpos=0;wjpos<nNear;++wjpos){
                int wj = hust::globalInput->addSTclose[v][wjpos];
                ++nbPair;
                auto delt =
                        params.timeCost.get(w,v)
                        +params.timeCost.get(v,wj)
                        -params.timeCost.get(w,wj);

                vNearSumDeltCost +=delt;
//                    vNearSumDeltCost = std::max<int>(vNearSumDeltCost,delt);

            }
        }
        weight[v] = vNearSumDeltCost/nbPair;
//            weight[v] = vNearSumDeltCost;
    }
    return weight;
}

void getWeight(CommandLine& commandline) {

    Params params(commandline);

    hust::globalInput = new hust::Input(params);
    hust::allocGlobalMem(params.config.seed);
    hust::globalInput->initInput();
    hust::Solver smartSolver;
    hust::Goal goal;
//    // Genetic algorithm
//    INFO("----- STARTING GENETIC ALGORITHM");
//
//    goal.TwoAlgCombine();
//    goal.test();
//    smartSolver.mRLLocalSearch(0,{});
    if(params.nbMustDispatch!=-1){
        INFO("params.nbMustDispatch!=-1");
    }

    auto& P = hust::globalInput->P;

//    auto getWeightByNear = [&](int nNear)->std::vector<int>{
//
//        std::vector< std::pair<int,int> > order;
//        order.reserve(params.nbClients+1);
//        order.push_back({0,0});
//        for(int v=1;v<=params.nbClients;++v){
//            int w = hust::globalInput->addSTclose[v][nNear];
//            int cycler = hust::globalInput->getDisof2(v,w);
//            order.push_back({cycler,v});
//        }
//        std::sort(order.begin(),order.end(),
//          [&](const std::pair<int,int> x, const std::pair<int,int> y){
//              return x.first<y.first;
//          });
//        std::vector<int> weight(params.nbClients + 1);
//        int part = 5;
//        if( params.nbClients < part){
//            for(int i=0;i <= params.nbClients;++i){
//                weight[order[i].second] = i+1;
//            }
//            return weight;
//        }
//        int eachPart = params.nbClients/part;
//        for(int i=0;i <= params.nbClients;++i){
//            weight[order[i].second] = i/eachPart+1;
//        }
//        return weight;
//    };



//    auto getWeightByDistanceDepot = [&](){
//        std::vector< std::pair<int,int> > order;
//        order.reserve(params.nbClients+1);
//        order.push_back({0,0});
//        for(int v=1;v<=params.nbClients;++v){
//            int distance0 = hust::globalInput->addSTclose[v][0];
//            order.push_back({distance0,v});
//        }
//
//        std::sort(order.begin(),order.end(),
//                  [&](const std::pair<int,int> x, const std::pair<int,int> y){
//                      return x.first > y.first;
//                  });
//
//        std::vector<int> weight(params.nbClients + 1);
//        int part = 5;
//        if( params.nbClients < part){
//            for(int i=0;i <= params.nbClients;++i){
//                weight[order[i].second] = i+1;
//            }
//            return weight;
//        }
//        int eachPart = params.nbClients/part;
//        for(int i=0;i <= params.nbClients;++i){
//            weight[order[i].second] = i/eachPart+1;
//        }
//        return weight;
//    };

//    int nNear = params.nbClients/2;
//    auto weightNear = getWeightByNear(nNear);
//    auto weightDepot = getWeightByDistanceDepot();
    auto weightNearDelt =  getWeightByNearDelt(params, std::min<int>(15,params.nbClients-1));

    for( int i = 0;i <= params.nbClients;++i ){
//        P[i] = weightNearDelt[i] +hust::myRand->pick(params.maxDist);
        P[i] = weightNearDelt[i];
//        P[i] = 0;
//        P[i] += weightNear[i];
//        P[i] += weightDepot[i];
    }
    P[0] = 0;
    printf("Weight ");
    for(int i:P){
        printf("%d ",i);
    }
    printf("\n");
    //saveSolutiontoCsvFile(hust::bks->bestSolFound);
    hust::deallocGlobalMem();
}

void doDynamicWithEjection(Params& params){

    hust::globalInput = new hust::Input(params);
    hust::allocGlobalMem(params.config.seed);
    hust::globalInput->initInput();

    hust::DynamicGoal dynamicGoal(&params);

//    std::vector<int> weightNew = getWeightByNearDelt(params,15);
//
//    for(int i = 1;i <= params.nbClients;++i){
//        params.P[i] = weightNew[i] - params.P[i];
//    }
//    auto& P = params.P;
    dynamicGoal.test();

    //saveSolutiontoCsvFile(hust::bks->bestSolFound);
    hust::deallocGlobalMem();
}

void smallInstance(CommandLine& commandline) {

    Params params(commandline);

    hust::globalInput = new hust::Input(params);
    hust::allocGlobalMem(params.config.seed);
    hust::globalInput->initInput();
    hust::Goal goal;

    hust::Solver smartSolver;
    smartSolver.initSolution(1);
    smartSolver.simulatedannealing(hust::IntInf,50.0,hust::globalCfg->ruinC_);

    hust::bks->bestSolFound.printDimacs();

}

void smartOnly(CommandLine& commandline){

    Params params(commandline);
    if(params.nbClients==1){

        printf("Route #1: 1\n");
        printf("Cost %d\n", params.timeCost.get(0,1) + params.timeCost.get(0,1));
        fflush(stdout);
        return;
    }

    if(params.nbMustDispatch == params.nbClients || params.nbMustDispatch == -1){
        ;
    }
    else if( params.nbMustDispatch != params.nbClients ){
        doDynamicWithEjection(params);
        return;
    }

    hust::globalInput = new hust::Input(params);
    hust::allocGlobalMem(params.config.seed);
    hust::globalInput->initInput();
    hust::Goal goal;
    // Genetic algorithm
    INFO("----- STARTING GENETIC ALGORITHM");
    goal.TwoAlgCombine();
//    goal.test();
    hust::bks->bestSolFound.printDimacs();
    //saveSolutiontoCsvFile(hust::bks->bestSolFound);
    hust::deallocGlobalMem();
}

void setParams(Params& params){

//    params.config.fractionGeneratedNearest = 0.20;
//    params.config. fractionGeneratedSmart = 0.20;
//    params.config. fractionGeneratedFurthest = 00.20;
//    params.config. fractionGeneratedSweep = 0.20;
//    params.config. fractionGeneratedRandomly = 0.10;

    if(params.nbClients <= 240){
        params.config.nbIter = 4000;
        params.config.fractionGeneratedNearest = 0.05;	//0.05
        params.config.fractionGeneratedSmart = 0.00; //0.0
        params.config.fractionGeneratedFurthest = 0.05; // 0.05
        params.config.fractionGeneratedSweep = 0.05; //0.05
        params.config.fractionGeneratedRandomly = 0.85;
    }

    if(params.nbClients >= 450){
        params.config.nbIter = 6000;
        params.config.fractionGeneratedNearest = 0.05;	//0.05
        params.config.fractionGeneratedSmart = 0.00; //0.0
        params.config.fractionGeneratedFurthest = 0.050; // 0.05
        params.config.fractionGeneratedSweep = 0.05; //0.05
        params.config.fractionGeneratedRandomly = 0.85;
    }
//    else{
//        params.config.useDynamicParameters = true;
//    }

}

static Params::Config getConfig(Params::Config& defaultCfg,int index) {

    std::vector<Params::Config> configs(Params::ConfigType::nbConfigType,defaultCfg);

    configs[Params::ConfigType::initSmallTolerate].maxToleratedCapacityViolation = 20;
    configs[Params::ConfigType::initSmallTolerate].maxToleratedTimeWarp = 20;

    configs[Params::ConfigType::smallNBGranular].nbGranular = 20;

    configs[Params::ConfigType::smallNBIter].nbIter = 5000;

    configs[Params::ConfigType::bigNBIter].nbIter = 20000;

    configs[Params::ConfigType::smallTargetFeasible].targetFeasible = 0.2;

    configs[Params::ConfigType::bigTargetFeasible].targetFeasible = 0.5;

    configs[Params::ConfigType::growNbGranularSize].nbGranular = 5;
    configs[Params::ConfigType::growNbGranularSize].growNbGranularSize = 5;
    configs[Params::ConfigType::growNbGranularSize].growNbGranularAfterNonImprovementIterations = 500;

    configs[Params::ConfigType::growPopulationSize].minimumPopulationSize = 10;
    configs[Params::ConfigType::growPopulationSize].growPopulationSize = 5;
    configs[Params::ConfigType::growPopulationSize].growPopulationAfterNonImprovementIterations = 500;

    return configs[index];
}

int getConfigIndex(std::string key) {

    std::unordered_map<std::string, int> mp = {
        {"initSmallTolerate",0},
        {"smallNBGranular", 1},
        {"smallNBIter", 2},
        {"bigNBIter", 3},
        {"smallTargetFeasible", 4},
        {"bigTargetFeasible", 5},
        {"growNbGranularSize", 6},
        {"growPopulationSize", 7},
        {"nbConfigType", 8}
    };
    if (mp.count(key) > 0) {
        return mp[key];
    }
    return -1;

}

void hgsAndSmart(CommandLine& commandline) {

    Params params(commandline);

//	if (params.nbMustDispatch == 0) {
//		printf("Cost 0\n");
//		fflush(stdout);
//		return;
//	}

    if(params.nbClients==1) {
        printf("Route #1: 1\n");
        printf("Cost %d\n", params.timeCost.get(0,1) + params.timeCost.get(0,1));
        fflush(stdout);
        return;
    }
    if(params.nbMustDispatch == params.nbClients || params.nbMustDispatch == -1){
        ;
    }
    else if( params.nbMustDispatch != params.nbClients ){
        doDynamicWithEjection(params);
        return;
    }

    hust::globalInput = new hust::Input(params);
    hust::allocGlobalMem(params.config.seed);
    hust::globalInput->initInput();
    hust::Goal goal;

    // setParams(params);

    INFO("params.nbMustDispatch:",params.nbMustDispatch," params.nbClients:", params.nbClients);
    Split split(&params);
    LocalSearch localSearch(&params);
    INFO("----- INSTANCE LOADED WITH ", params.nbClients, " CLIENTS AND ", params.nbVehicles," VEHICLES");
    hust::Solver smartSolver;

    Population population(&params, &split, &localSearch,&smartSolver);
    Genetic solver(&params, &split, &population, &localSearch, &smartSolver);

    INFO("params.config.timeLimit:",params.config.timeLimit);

    solver.run(params.config.nbIter, params.config.timeLimit);
//    solver.runMA();
//    solver.runRuin();
    population.getBestFound()->printCVRPLibFormat();
    hust::deallocGlobalMem();
}

void setCommandLine(CommandLine& commandLine){
    commandLine.config.nbIter = 10000;
    commandLine.config.minimumPopulationSize = 30;

//    commandLine.config.resetPopulationWithAllRandom = 1;
//    commandLine.config.ruinBeforeRestart = 1;
//    commandLine.config.ruinWhenGetBKS = 1;
//    commandLine.config.nagataMaBeforeRestart = 1;
//    commandLine.config.ruinBeforeRestart = 1;
}

int main(int argc, char* argv[])
{
//	try
//	{
		// Reading the arguments of the program
		CommandLine commandline(argc, argv);
        INFO("----- READING DATA SET FROM: ", commandline.config.pathInstance);

        setCommandLine(commandline);

        if( commandline.config.call == "getWeight"){
            getWeight(commandline);
        }else if(commandline.config.call == "hgsAndSmart"){
            INFO("commandline.config.call == hgsAndSmart");
            hgsAndSmart(commandline);
        }
        else if(commandline.config.call == "smallInstance"){
            INFO("commandline.config.call == smallInstance");
            smallInstance(commandline);
        }
        else{
            ERROR("No this function call:",commandline.config.call);
        }

//	}
//	catch (const std::string& e)
//	{
//		std::cerr << "EXCEPTION | " << e << std::endl;
//	}
//	catch (const std::exception& e)
//	{
//		std::cerr << "EXCEPTION | " << e.what() << std::endl;
//	}
	return 0;
}

