#include <algorithm>
#include <time.h>
#include <iterator>
#include <unordered_set>

#include "Genetic.h"
#include "Params.h"
#include "Split.h"
#include "Population.h"
#include "LocalSearch.h"
#include "Individual.h"
#include "../smart/EAX.h"

void Genetic::run(int maxIterNonProd, int timeLimit)
{
	// Do iterations of the Genetic Algorithm, until more then maxIterNonProd consecutive iterations without improvement or a time limit (in seconds) is reached
	int nbIterNonProd = 1;
	for (int nbIter = 0; nbIterNonProd <= maxIterNonProd && !params->isTimeLimitExceeded(); nbIter++)
	{
        if(params->isTimeLimitExceeded()){
            break;
        }
        auto parent = population->getNonIdenticalParentsBinaryTournament();
        int level = 0;
        if(nbIterNonProd <= 300){
            level = 0;
        }else{
            level = 1;
        }
        Individual* offspring = bestOfSREXAndOXCrossovers(parent,level);

		localSearch->run(offspring,
                         params->penaltyCapacity,
                         params->penaltyTimeWarp);

//        if( hust::globalCfg->isHgsRuinWhenGetBKS==1){
//            smartSolver->runSimulatedannealing(offspring);
//        }
        if( params->config.ruinWhenGetBKS ==1 &&
            offspring->isFeasible && offspring->myCostSol.penalizedCost
           < population->getBestFound()->myCostSol.penalizedCost - MY_EPSILON){
            smartSolver->runSimulatedannealing(offspring);
        }
		// Check if the new individual is the best feasible individual of the population, based on penalizedCost
		bool isNewBest = population->addIndividual(offspring, true);

        // In case of infeasibility, repair the individual with a certain probability
		if (!offspring->isFeasible && params->rng() % 100 < (unsigned int) params->config.repairProbability)
		{
			// Run the Local Search again, but with penalties for infeasibilities multiplied by 10
			localSearch->run(offspring, params->penaltyCapacity * 10., params->penaltyTimeWarp * 10.);
			// If the individual is feasible now, check if it is the best feasible individual of the population, based on penalizedCost and add it to the population
			// If the individual is not feasible now, it is not added to the population
			if (offspring->isFeasible)
			{
				isNewBest = (population->addIndividual(offspring, false) || isNewBest);
			}
		}

		/* TRACKING THE NUMBER OF ITERATIONS SINCE LAST SOLUTION IMPROVEMENT */
		if (isNewBest)
		{
			//isNewBest = population->addIndividual(offspring, false);
			nbIterNonProd = 1;
		}
		else nbIterNonProd++;

		/* DIVERSIFICATION, PENALTY MANAGEMENT AND TRACES */
		// Update the penaltyTimeWarp and penaltyCapacity every 100 iterations
		if (nbIter % 100 == 0)
		{
			population->managePenalties();
		}
		// Print the state of the population every 500 iterations
		if (nbIter % 500 == 0)
		{
			population->printState(nbIter, nbIterNonProd);
		}
		// Log the current population to a .csv file every logPoolInterval iterations (if logPoolInterval is not 0)
		if (params->config.logPoolInterval > 0 && nbIter % params->config.logPoolInterval == 0)
		{
			population->exportPopulation(nbIter, params->config.pathSolution + ".log.csv");
		}

		/* FOR TESTS INVOLVING SUCCESSIVE RUNS UNTIL A TIME LIMIT: WE RESET THE ALGORITHM/POPULATION EACH TIME maxIterNonProd IS ATTAINED*/
		if (timeLimit != INT_MAX && nbIterNonProd == maxIterNonProd && params->config.doRepeatUntilTimeLimit)
		{

            INFO("ReStart");
            if(params->config.nagataMaBeforeRestart==1) {
                runMA();
            }

            if(params->isTimeLimitExceeded()){
                break;
            }

            if(params->config.ruinBeforeRestart==1){
                runRuin();
            }

            if(params->isTimeLimitExceeded()){
                break;
            }

            if(params->config.resetPopulationWithAllRandom == 1) {
                params->config.fractionGeneratedNearest = 0.00;    //0.05
                params->config.fractionGeneratedSmart = 0.00; //0.0
                params->config.fractionGeneratedFurthest = 0.00; // 0.05
                params->config.fractionGeneratedSweep = 0.00; //0.05
                params->config.fractionGeneratedRandomly = 1.00;
            }

            population->restart();
			nbIterNonProd = 1;
		}

		/* OTHER PARAMETER CHANGES*/
		// Increase the nbGranular by growNbGranularSize (and set the correlated vertices again) every certain number of iterations, if growNbGranularSize is greater than 0
		if (nbIter > 0 && params->config.growNbGranularSize != 0 && (
			(params->config.growNbGranularAfterIterations > 0 && nbIter % params->config.growNbGranularAfterIterations == 0) ||
			(params->config.growNbGranularAfterNonImprovementIterations > 0 && nbIterNonProd % params->config.growNbGranularAfterNonImprovementIterations == 0)
			))
		{
			// Note: changing nbGranular also changes how often the order is reshuffled
            int old = params->config.nbGranular;
			params->config.nbGranular += params->config.growNbGranularSize;
            params->config.nbGranular = std::min<int>(40,params->config.nbGranular);
            if(params->config.nbGranular!=old) {
                params->SetCorrelatedVertices();
            }
		}

		// Increase the minimumPopulationSize by growPopulationSize every certain number of iterations, if growPopulationSize is greater than 0
		if (nbIter > 0 && params->config.growPopulationSize != 0 && (
			(params->config.growPopulationAfterIterations > 0 && nbIter % params->config.growPopulationAfterIterations == 0) || 
			(params->config.growPopulationAfterNonImprovementIterations > 0 && nbIterNonProd % params->config.growPopulationAfterNonImprovementIterations == 0)
			))
		{
			// This will automatically adjust after some iterations
			params->config.minimumPopulationSize += params->config.growPopulationSize;
			params->config.minimumPopulationSize = std::min<int>(40, params->config.minimumPopulationSize);

		}
	}
}

void Genetic::runMAOfPopulation(std::vector<Individual*> pool){

    using namespace hust;
    int poolSize = pool.size();

    if(poolSize < 2){
        return;
    }
    INFO("poolSize:", poolSize);

    auto& ords = myRandX->getMN(poolSize, poolSize);
    //myRand->shuffleVec(ords);

    auto getMinRtCostInPool = [&]() -> DisType {
        DisType bestSolInPool = DisInf;
        for (int i = 0; i <static_cast<int>(pool.size()); ++i) {
            bestSolInPool = std::min<DisType>(bestSolInPool, pool[i]->myCostSol.penalizedCost);
        }
        return bestSolInPool;
    };

    DisType bestSolInPool = getMinRtCostInPool();
    Solver pa;
    Solver pb;
    Solver pc;

    //TODO[-1]:naMA这里改10了

    for (int ct = 0; ct < 1; ct++) {
        if(params->isTimeLimitExceeded()){
            break;
        }

        hust::myRand->shuffleVec(ords);
        for (int i = 0; i < poolSize; ++i) {

            if(params->isTimeLimitExceeded()){
                break;
            }
            int paIndex = ords[i];
            int pbIndex = ords[(i + 1) % poolSize];
            pa.loadIndividual(pool[paIndex]);
            pb.loadIndividual(pool[pbIndex]);
            pc = pa;

            int eaxReturn = EAX::doTwoKindEAX(pa, pb, pc, 1);

            if(eaxReturn==-1){
                continue;
            }

            pc.exportIndividual(candidateOffsprings[7]);
            population->updateBestSolutionOverall(candidateOffsprings[7]);
            int replace = EAX::updateWhichPaPbUsePc(pa,pb,pc);
            if( replace == 1 ){
                pc.exportIndividual(pool[paIndex]);
            }else if(replace==2){
                pc.exportIndividual(pool[pbIndex]);
            }
        }

//        DisType curBest = getMinRtCostInPool();
//        if (curBest < bestSolInPool ) {
//            bestSolInPool = curBest;
//            ct = 0;
//        }
    }

    //TODO[-1]:naMA这里改10了
    bestSolInPool = getMinRtCostInPool();

    for (int ct = 0; ct < 1; ct++) {
        if(params->isTimeLimitExceeded()){
            break;
        }
        myRand->shuffleVec(ords);
        for (int i = 0; i < poolSize; ++i) {

            if(params->isTimeLimitExceeded()){
                break;
            }

            int paIndex = ords[i];
            int pbIndex = ords[(i + 1) % poolSize];
            pa.loadIndividual(pool[paIndex]);
            pb.loadIndividual(pool[pbIndex]);
            pc = pa;
            int eaxReturn = EAX::doTwoKindEAX(pa, pb, pc, 1);

            if(eaxReturn==-1){
                continue;
            }

            pc.exportIndividual(candidateOffsprings[7]);
            population->updateBestSolutionOverall(candidateOffsprings[7]);

            int replace = EAX::updateWhichPaPbUsePc(pa,pb,pc);
            if( replace == 1 ){
                pc.exportIndividual(pool[paIndex]);
            }else if(replace==2){
                pc.exportIndividual(pool[pbIndex]);
            }
        }
//TODO[lyh][eaxup]
//        DisType curBest = getMinRtCostInPool();
//        if (curBest < bestSolInPool) {
//            bestSolInPool = curBest;
//            ct = 0;
//        }
    }

    bks->bestSolFound.exportIndividual(candidateOffsprings[7]);
    population->updateBestSolutionOverall(candidateOffsprings[7]);

    INFO("getMinRtCostInPool():", getMinRtCostInPool());
}

void Genetic::runMA(){

    using namespace hust;
    std::map<int,std::vector<Individual*>> individualsGroupByRouterNum;

    for(auto& indiv:population->feasibleSubpopulation){
        int nbRoutes = indiv->myCostSol.nbRoutes;
        individualsGroupByRouterNum[nbRoutes].push_back(indiv);
    }

    for(auto& indiv:population->infeasibleSubpopulation){
        int nbRoutes = indiv->myCostSol.nbRoutes;
        individualsGroupByRouterNum[nbRoutes].push_back(indiv);
    }
    std::vector<Individual*> pool;

    for(auto& it:individualsGroupByRouterNum){
        INFO("it.first:",it.first);
        (void)it;
    }

    pool = individualsGroupByRouterNum.begin()->second;
    for(auto& it:individualsGroupByRouterNum){
        if(params->isTimeLimitExceeded()){
            break;
        }
        runMAOfPopulation(it.second);
    }
}

void Genetic::runRuin(){

//    auto& pop = population->feasibleSubpopulation;
//    if(pop.size()==0){
//        return;
//    }
//    auto best = pop[(params->rng()%pop.size())];
    auto best = population->getBestFound();
    if(best == nullptr){
        return;
    }
    smartSolver->loadIndividual(best);

    hust::Solver pBest = *smartSolver;
    hust::Solver s = *smartSolver;

    int iter = 1;
    int iterMax = 100;
    double temperature = 100.0;
    double j0 = temperature;
	double jf = 1;
	double c1 = pow(jf / j0, 1 / double(iterMax));
    double c2 = 0.99;
    double c = std::min<double>(c1,c2);

    temperature = j0;

    if( smartSolver->dynamicEP.size() == params->nbClients){
        return;
    }
//    int ruinNum = hust::myRand->pick(hust::globalCfg->ruinC_Min,
//                                     hust::globalCfg->ruinC_Max+1);
//        ruinNum = std::min(ruinNum,params->nbClients-1);

    int ruinNum = hust::globalCfg->ruinC_;
    while (++iter <= iterMax && !params->isTimeLimitExceeded()){

//        ruinNum = hust::myRand->pick(hust::globalCfg->ruinC_Min,
//                                         hust::globalCfg->ruinC_Max+1);
//        ruinNum = std::min(ruinNum,params->nbClients-1);

        auto sStar = s;

        hust::globalRepairSquIter();

        sStar.CVB2ruinLS(ruinNum);

        hust::bks->updateBKSAndPrint(sStar,"from ruin sStart");

        hust::DisType delt = temperature * log(double(hust::myRand->pick(1, 1000000)) / (double)1000000);

        if (sStar.RoutesCost < s.RoutesCost - delt) {
            s = sStar;

        }

        temperature *= c;
        if(temperature<1.0){
            temperature = 100.0;
        }

        if (sStar.RoutesCost < pBest.RoutesCost) {
            pBest = sStar;
            iter=0;
        }
    }

    pBest.exportIndividual(candidateOffsprings[7]);
    auto old = population->getBestFound()->myCostSol.penalizedCost;
    bool updated = population->updateBestSolutionOverall(candidateOffsprings[7]);
    if( updated == true ){
        INFO("updated,delt:",population->getBestFound()->myCostSol.penalizedCost-old);
    }
}

Individual* Genetic::crossoverOX(std::pair<const Individual*, const Individual*> parents)
{
	// Picking the start and end of the crossover zone
//	int start = params->rng() % params->nbClients;
//	int end = params->rng() % params->nbClients;
//	while (end == start)
//	{
//		end = params->rng() % params->nbClients;
//	}

// TODO[lyh][ox]:
    int start = params->rng() % params->nbClients;
    int minWidth = params->nbClients*0.10;
    int maxWidth = params->nbClients*0.90;

    minWidth = std::max<int>(minWidth,1);
    maxWidth = std::min<int>(maxWidth,params->nbClients-1);

    int end = (start + (params->rng() % maxWidth)) % params->nbClients;
    int width = end>=start?end-start+1:end+params->nbClients-start+1;

    while (width>maxWidth || width<minWidth){
        end = (start + (params->rng() % maxWidth)) % params->nbClients;
        width = end>=start?end-start+1:end+params->nbClients-start+1;
    }

	doOXcrossover(candidateOffsprings[2], parents, start, end);
	doOXcrossover(candidateOffsprings[3], parents, start, end);

	// Return the best individual of the two, based on penalizedCost
	return candidateOffsprings[2]->myCostSol.penalizedCost < candidateOffsprings[3]->myCostSol.penalizedCost
		? candidateOffsprings[2]
		: candidateOffsprings[3];
}

void Genetic::doOXcrossover(Individual* result, std::pair<const Individual*, const Individual*> parents, int start, int end)
{
	// Frequency vector to track the clients which have been inserted already
	std::vector<bool> freqClient = std::vector<bool>(params->nbClients + 1, false);

	// Copy in place the elements from start to end (possibly "wrapping around" the end of the array)
	int j = start;
	while (j % params->nbClients != (end + 1) % params->nbClients)
	{
		result->chromT[j % params->nbClients] = parents.first->chromT[j % params->nbClients];
		// Mark the client as copied
		freqClient[result->chromT[j % params->nbClients]] = true;
		j++;
	}

	// Fill the remaining elements in the order given by the second parent
	for (int i = 1; i <= params->nbClients; i++)
	{
		// Check if the next client is already copied in place
		int temp = parents.second->chromT[(end + i) % params->nbClients];
		// If the client is not yet copied in place, copy in place now
		if (freqClient[temp] == false)
		{
			result->chromT[j % params->nbClients] = temp;
			j++;
		}
	}

	// Completing the individual with the Split algorithm
	split->generalSplit(result, params->nbVehicles);
}


void Genetic::doOXcrossoverStar(Individual* result, std::pair<const Individual*, const Individual*> parents)
{
	// Frequency vector to track the clients which have been inserted already
	std::vector<bool> freqClient = std::vector<bool>(params->nbClients + 1, false);
    std::fill(result->chromT.begin(), result->chromT.end(),-1);

	int paNbRoute = parents.first->myCostSol.nbRoutes;
	int nbStartIndexeschromT = (params->rng()%paNbRoute) + 1;
    nbStartIndexeschromT = std::min<int>(6,nbStartIndexeschromT);
	auto& arr = hust::myRandX->getMN(params->nbClients, nbStartIndexeschromT);
	std::sort(arr.begin(),arr.begin()+nbStartIndexeschromT);
	for (int i = params->rng() % 2 ; i < nbStartIndexeschromT;i+=2){
        int begin = arr[i];
        int end = arr[(i + 1)%nbStartIndexeschromT];
		for (int j = begin;; ++j) {
            int t = j%params->nbClients;
			result->chromT[t] = parents.first->chromT[t];
			freqClient[result->chromT[t]] = true;
            if(t == end){
                break;
            }
		}
	}

//    int start = params->rng() % params->nbClients;
//    int minWidth = params->nbClients*0.10;
//    int maxWidth = params->nbClients*0.60;
//    minWidth = std::max<int>(minWidth,1);
//    maxWidth = std::min<int>(maxWidth,params->nbClients-1);
//    int nbGetFromFirstParent = hust::myRand->pick(minWidth,maxWidth+1);
//    auto& arr = hust::myRandX->getMN(params->nbClients, nbGetFromFirstParent);
//    std::sort(arr.begin(),arr.begin()+nbGetFromFirstParent);
//	for (int i = 0 ; i < nbGetFromFirstParent ; ++i){
//        result->chromT[i] = parents.first->chromT[i];
//        freqClient[result->chromT[i]] = true;
//	}

	std::vector<int> reamin;
	for (int c :parents.second->chromT ) {
		if (freqClient[c] == false) {
			reamin.push_back(c);
		}
	}
	
//	int resultChromTIndex = params->rng()%params->nbClients;
	int resultChromTIndex = 0;

	// Fill the remaining elements in the order given by the second parent
	for (int c : reamin) {
		while (result->chromT[resultChromTIndex % params->nbClients] != -1) {
			++resultChromTIndex;
			resultChromTIndex %= params->nbClients;
		}
		result->chromT[resultChromTIndex] = c;
		++resultChromTIndex;
		resultChromTIndex %= params->nbClients;
	}
	// Completing the individual with the Split algorithm
	split->generalSplit(result, params->nbVehicles);
}

Individual* Genetic::crossoverOXStar(std::pair<const Individual*, const Individual*> parents)
{

	// Create two individuals using OX
	doOXcrossoverStar(candidateOffsprings[4], parents);
	doOXcrossoverStar(candidateOffsprings[5], parents);

	// Return the best individual of the two, based on penalizedCost
	return candidateOffsprings[4]->myCostSol.penalizedCost < candidateOffsprings[5]->myCostSol.penalizedCost
		? candidateOffsprings[4]
		: candidateOffsprings[5];
}

Individual* Genetic::crossoverSREX(std::pair<const Individual*, const Individual*> parents)
{
	// Get the number of routes of both parents
	int nOfRoutesA = parents.first->myCostSol.nbRoutes;
	int nOfRoutesB = parents.second->myCostSol.nbRoutes;

	// Picking the start index of routes to replace of parent A
	// We like to replace routes with a large overlap of tasks, so we choose adjacent routes (they are sorted on polar angle)
	int startA = params->rng() % nOfRoutesA;
	int nOfMovedRoutes = std::min(nOfRoutesA, nOfRoutesB) == 1 ? 1 : params->rng() % (std::min(nOfRoutesA - 1, nOfRoutesB - 1)) + 1; // Prevent not moving any routes
	int startB = startA < nOfRoutesB ? startA : 0;

	std::unordered_set<int> clientsInSelectedA;
	for (int r = 0; r < nOfMovedRoutes; r++)
	{
		// Insert the first 
		clientsInSelectedA.insert(parents.first->chromR[(startA + r) % nOfRoutesA].begin(),
			parents.first->chromR[(startA + r) % nOfRoutesA].end());
	}

	std::unordered_set<int> clientsInSelectedB;
	for (int r = 0; r < nOfMovedRoutes; r++)
	{
		clientsInSelectedB.insert(parents.second->chromR[(startB + r) % nOfRoutesB].begin(),
			parents.second->chromR[(startB + r) % nOfRoutesB].end());
	}

	bool improved = true;
	while (improved)
	{
		// Difference for moving 'left' in parent A
		const int differenceALeft = static_cast<int>(std::count_if(parents.first->chromR[(startA - 1 + nOfRoutesA) % nOfRoutesA].begin(),
			parents.first->chromR[(startA - 1 + nOfRoutesA) % nOfRoutesA].end(),
			[&clientsInSelectedB](int c) { return clientsInSelectedB.find(c) == clientsInSelectedB.end(); }))
			- static_cast<int>(std::count_if(parents.first->chromR[(startA + nOfMovedRoutes - 1) % nOfRoutesA].begin(),
				parents.first->chromR[(startA + nOfMovedRoutes - 1) % nOfRoutesA].end(),
				[&clientsInSelectedB](int c) { return clientsInSelectedB.find(c) == clientsInSelectedB.end(); }));

		// Difference for moving 'right' in parent A
		const int differenceARight = static_cast<int>(std::count_if(parents.first->chromR[(startA + nOfMovedRoutes) % nOfRoutesA].begin(),
			parents.first->chromR[(startA + nOfMovedRoutes) % nOfRoutesA].end(),
			[&clientsInSelectedB](int c) { return clientsInSelectedB.find(c) == clientsInSelectedB.end(); }))
			- static_cast<int>(std::count_if(parents.first->chromR[startA].begin(),
				parents.first->chromR[startA].end(),
				[&clientsInSelectedB](int c) { return clientsInSelectedB.find(c) == clientsInSelectedB.end(); }));

		// Difference for moving 'left' in parent B
		const int differenceBLeft = static_cast<int>(std::count_if(parents.second->chromR[(startB - 1 + nOfMovedRoutes) % nOfRoutesB].begin(),
			parents.second->chromR[(startB - 1 + nOfMovedRoutes) % nOfRoutesB].end(),
			[&clientsInSelectedA](int c) { return clientsInSelectedA.find(c) != clientsInSelectedA.end(); }))
			- static_cast<int>(std::count_if(parents.second->chromR[(startB - 1 + nOfRoutesB) % nOfRoutesB].begin(),
				parents.second->chromR[(startB - 1 + nOfRoutesB) % nOfRoutesB].end(),
				[&clientsInSelectedA](int c) { return clientsInSelectedA.find(c) != clientsInSelectedA.end(); }));

		// Difference for moving 'right' in parent B
		const int differenceBRight = static_cast<int>(std::count_if(parents.second->chromR[startB].begin(),
			parents.second->chromR[startB].end(),
			[&clientsInSelectedA](int c) { return clientsInSelectedA.find(c) != clientsInSelectedA.end(); }))
			- static_cast<int>(std::count_if(parents.second->chromR[(startB + nOfMovedRoutes) % nOfRoutesB].begin(),
				parents.second->chromR[(startB + nOfMovedRoutes) % nOfRoutesB].end(),
				[&clientsInSelectedA](int c) { return clientsInSelectedA.find(c) != clientsInSelectedA.end(); }));

		const int bestDifference = std::min({ differenceALeft, differenceARight, differenceBLeft, differenceBRight });

		if (bestDifference < 0)
		{
			if (bestDifference == differenceALeft)
			{
				for (int c : parents.first->chromR[(startA + nOfMovedRoutes - 1) % nOfRoutesA])
				{
					clientsInSelectedA.erase(clientsInSelectedA.find(c));
				}
				startA = (startA - 1 + nOfRoutesA) % nOfRoutesA;
				for (int c : parents.first->chromR[startA])
				{
					clientsInSelectedA.insert(c);
				}
			}
			else if (bestDifference == differenceARight)
			{
				for (int c : parents.first->chromR[startA])
				{
					clientsInSelectedA.erase(clientsInSelectedA.find(c));
				}
				startA = (startA + 1) % nOfRoutesA;
				for (int c : parents.first->chromR[(startA + nOfMovedRoutes - 1) % nOfRoutesA])
				{
					clientsInSelectedA.insert(c);
				}
			}
			else if (bestDifference == differenceBLeft)
			{
				for (int c : parents.second->chromR[(startB + nOfMovedRoutes - 1) % nOfRoutesB])
				{
					clientsInSelectedB.erase(clientsInSelectedB.find(c));
				}
				startB = (startB - 1 + nOfRoutesB) % nOfRoutesB;
				for (int c : parents.second->chromR[startB])
				{
					clientsInSelectedB.insert(c);
				}
			}
			else if (bestDifference == differenceBRight)
			{
				for (int c : parents.second->chromR[startB])
				{
					clientsInSelectedB.erase(clientsInSelectedB.find(c));
				}
				startB = (startB + 1) % nOfRoutesB;
				for (int c : parents.second->chromR[(startB + nOfMovedRoutes - 1) % nOfRoutesB])
				{
					clientsInSelectedB.insert(c);
				}
			}
		}
		else
		{
			improved = false;
		}
	}

	// Identify differences between route sets
	std::unordered_set<int> clientsInSelectedANotB;
	std::copy_if(clientsInSelectedA.begin(), clientsInSelectedA.end(),
		std::inserter(clientsInSelectedANotB, clientsInSelectedANotB.end()),
		[&clientsInSelectedB](int c) { return clientsInSelectedB.find(c) == clientsInSelectedB.end(); });

	std::unordered_set<int> clientsInSelectedBNotA;
	std::copy_if(clientsInSelectedB.begin(), clientsInSelectedB.end(),
		std::inserter(clientsInSelectedBNotA, clientsInSelectedBNotA.end()),
		[&clientsInSelectedA](int c) { return clientsInSelectedA.find(c) == clientsInSelectedA.end(); });

	// Replace selected routes from parent A with routes from parent B
	for (int r = 0; r < nOfMovedRoutes; r++)
	{
		int indexA = (startA + r) % nOfRoutesA;
		int indexB = (startB + r) % nOfRoutesB;
		candidateOffsprings[0]->chromR[indexA].clear();
		candidateOffsprings[1]->chromR[indexA].clear();

		for (int c : parents.second->chromR[indexB])
		{
			candidateOffsprings[0]->chromR[indexA].push_back(c);
			if (clientsInSelectedBNotA.find(c) == clientsInSelectedBNotA.end())
			{
				candidateOffsprings[1]->chromR[indexA].push_back(c);
			}
		}
	}

	// Move routes from parent A that are kept
	for (int r = nOfMovedRoutes; r < nOfRoutesA; r++)
	{
		int indexA = (startA + r) % nOfRoutesA;
		candidateOffsprings[0]->chromR[indexA].clear();
		candidateOffsprings[1]->chromR[indexA].clear();

		for (int c : parents.first->chromR[indexA])
		{
			if (clientsInSelectedBNotA.find(c) == clientsInSelectedBNotA.end())
			{
				candidateOffsprings[0]->chromR[indexA].push_back(c);
			}
			candidateOffsprings[1]->chromR[indexA].push_back(c);
		}
	}

	// Delete any remaining routes that still lived in offspring
	for (int r = nOfRoutesA; r < params->nbVehicles; r++)
	{
		candidateOffsprings[0]->chromR[r].clear();
		candidateOffsprings[1]->chromR[r].clear();
	}

	// Step 3: Insert unplanned clients (those that were in the removed routes of A but not the inserted routes of B)
	insertUnplannedTasks(candidateOffsprings[0], clientsInSelectedANotB);
	insertUnplannedTasks(candidateOffsprings[1], clientsInSelectedANotB);

	candidateOffsprings[0]->evaluateCompleteCost();
	candidateOffsprings[1]->evaluateCompleteCost();

	return candidateOffsprings[0]->myCostSol.penalizedCost < candidateOffsprings[1]->myCostSol.penalizedCost
		? candidateOffsprings[0]
		: candidateOffsprings[1];
}

void Genetic::insertUnplannedTasks(Individual* offspring, std::unordered_set<int> unplannedTasks)
{
	// Initialize some variables
	int newDistanceToInsert = INT_MAX;		// TODO:
	int newDistanceFromInsert = INT_MAX;	// TODO:
	int distanceDelta = INT_MAX;			// TODO:

	// Loop over all unplannedTasks
	for (int c : unplannedTasks)
	{
		// Get the earliest and laster possible arrival at the client
		int earliestArrival = params->cli[c].earliestArrival;
		int latestArrival = params->cli[c].latestArrival;

		int bestDistance = INT_MAX;
		std::pair<int, int> bestLocation;

		// Loop over all routes
		for (int r = 0; r < params->nbVehicles; r++)
		{
			// Go to the next route if this route is empty
			if (offspring->chromR[r].empty())
			{
				continue;
			}

			newDistanceFromInsert = params->timeCost.get(c, offspring->chromR[r][0]);
			if (earliestArrival + newDistanceFromInsert < params->cli[offspring->chromR[r][0]].latestArrival)
			{
				distanceDelta = params->timeCost.get(0, c) + newDistanceToInsert
					- params->timeCost.get(0, offspring->chromR[r][0]);
				if (distanceDelta < bestDistance)
				{
					bestDistance = distanceDelta;
					bestLocation = { r, 0 };
				}
			}

			for (int i = 1; i < static_cast<int>(offspring->chromR[r].size()); i++)
			{
				newDistanceToInsert = params->timeCost.get(offspring->chromR[r][i - 1], c);
				newDistanceFromInsert = params->timeCost.get(c, offspring->chromR[r][i]);
				if (params->cli[offspring->chromR[r][i - 1]].earliestArrival + newDistanceToInsert < latestArrival
					&& earliestArrival + newDistanceFromInsert < params->cli[offspring->chromR[r][i]].latestArrival)
				{
					distanceDelta = newDistanceToInsert + newDistanceFromInsert
						- params->timeCost.get(offspring->chromR[r][i - 1], offspring->chromR[r][i]);
					if (distanceDelta < bestDistance)
					{
						bestDistance = distanceDelta;
						bestLocation = { r, i };
					}
				}
			}

			newDistanceToInsert = params->timeCost.get(offspring->chromR[r].back(), c);
			if (params->cli[offspring->chromR[r].back()].earliestArrival + newDistanceToInsert < latestArrival)
			{
				distanceDelta = newDistanceToInsert + params->timeCost.get(c, 0)
					- params->timeCost.get(offspring->chromR[r].back(), 0);
				if (distanceDelta < bestDistance)
				{
					bestDistance = distanceDelta;
					bestLocation = { r, static_cast<int>(offspring->chromR[r].size()) };
				}
			}
		}

		offspring->chromR[bestLocation.first].insert(
			offspring->chromR[bestLocation.first].begin() + bestLocation.second, c);
	}
}

Individual* Genetic::bestOfSREXAndOXCrossovers(std::pair<const Individual*, const Individual*> parents,int level)
{
	// Create two individuals, one with OX and one with SREX
    std::vector<Individual*> offsprings(5, nullptr);
    offsprings[0] = crossoverSREX(parents);
    offsprings[1] = crossoverOX(parents);

    if(level==1 && params->config.useEaxAndOXStar==1){
        offsprings[2] = crossoverOXStar(parents);
        offsprings[3] = hust::EAX::doEaxWithoutRepair(parents, candidateOffsprings[6]);
//        smartSolver->loadIndividual(parents.first);
//        smartSolver->perturbBaseRuin(hust::globalCfg->ruinC_*2);
//        smartSolver->exportIndividual(candidateOffsprings[7]);
//        split->generalSplit(candidateOffsprings[7],params->nbClients);
//        offsprings[4] = candidateOffsprings[7];
    }

    Individual* best = nullptr;
    for(auto& offs:offsprings){
        if(offs!= nullptr){
            if( best == nullptr){
                best = offs;
            }else{
                if(offs ->myCostSol.penalizedCost < best->myCostSol.penalizedCost ){
                    best = offs;
                }
            }
        }
    }
	//Return the best individual, based on penalizedCost
	return best;
}

Genetic::Genetic(Params* params, Split* split, Population* population, LocalSearch* localSearch,hust::Solver* smartSolver) 
	: params(params), split(split), population(population), localSearch(localSearch), smartSolver(smartSolver)
{
	// After initializing the parameters of the Genetic object, also generate new individuals in the array candidateOffsprings
	std::generate(candidateOffsprings.begin(), candidateOffsprings.end(), [&]{ return new Individual(params); });
}

Genetic::~Genetic(void)
{
	// Destruct the Genetic object by deleting all the individuals of the candidateOffsprings
	for (Individual* candidateOffspring : candidateOffsprings)
	{
		delete candidateOffspring;
	}
}
