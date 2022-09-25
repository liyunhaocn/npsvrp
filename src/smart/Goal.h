#ifndef CN_HUST_LYH_GOAL_H
#define CN_HUST_LYH_GOAL_H

#include "Solver.h"

namespace hust {

struct Goal {

	Vec<Vec<bool>> eaxTabuTable;

	int poprnLowBound = 0;
	int poprnUpBound = 0;

	UnorderedMap<int, Vec<Solver>> ppool;
	//Vec<Vec<Solver>> ppool;

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
