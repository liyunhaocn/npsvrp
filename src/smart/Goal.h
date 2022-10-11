#ifndef CN_HUST_LYH_GOAL_H
#define CN_HUST_LYH_GOAL_H

#include "Solver.h"
#include "../hgs/Genetic.h"

namespace hust {

struct EjectPool{

    int sumCost = 0;
    ConfSet container;
    Params* params;
    EjectPool(Params* params):params(params), container(params->nbClients+1){}
    ~EjectPool() { }

    void insert(int v){
        sumCost += params->P[v];
        container.ins(v);
    }

    void remove(int v){
        if(container.pos[v]==-1){
            ERROR("container.pos[v]==-1");
        }
        sumCost -= params->P[v];
        container.removeVal(v);
    }

    int randomPeek() {
        if( container.cnt == 0 ){
            ERROR("container.cnt == 0");
        }
        int index = myRand->pick(container.cnt);
        return container.ve[index];
    }
};

struct DynamicGoal{
    Params* params;
    Solver bksSolution;
    Vec<int> customers_no_need_dispatch;
    EjectPool ep;
    DynamicGoal(Params* params);
    void test();
};

struct Goal {

	Vec<Vec<bool>> eaxTabuTable;

	int poprnLowBound = 0;
	int poprnUpBound = 0;

	UnorderedMap<int, Vec<Solver>> ppool;
	//Vec<Vec<Solver>> ppool;
	Params* para;
    Vec<Solver> population;
	int curSearchRN = -1;

	Goal();

	void updateppol(Solver& sol, int index);

	DisType getMinRtCostInPool(int rn);

	DisType doTwoKindEAX(Solver& pa, Solver& pb, int kind);

	bool perturbOneSol(Solver& sol);

	int naMA(int rn);

	//Vec<int> getNotTabuPaPb();
	//Vec<int> getpairOfPaPb();

	int gotoRNPop(int rn);

	bool fillPopulation(int rn);

	int callSimulatedannealing();

	bool experOnMinRN();

	int TwoAlgCombine();

	void getTheRangeMostHope();

	void test();
};


}

#endif // !CN_HUST_LYH_GOAL_H
