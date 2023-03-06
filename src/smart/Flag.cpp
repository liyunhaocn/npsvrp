
#include <climits>
#include "Flag.h"
#include "Utility.h"

namespace hust {

int IntInf = INT_MAX/10;
long long int LLInf = LLONG_MAX / 10;

// dimacs «10
int disMul = 10000;
DisType DisInf = LLInf;

//extern int disMul = 10000;
unsigned Mod = 1000000007;

Random* myRand = nullptr;
RandomX* myRandX = nullptr;
hust::util::Array2D<int>* yearTable = nullptr;
Configuration* globalCfg = nullptr;
Input* globalInput = nullptr;
BKS* bks = nullptr;
LocalSearch* hgsLocalSearch;

int squIter = 1;

void globalRepairSquIter() {
	if (squIter * 10 > IntInf) {
		squIter = 1;
		auto& lhsyear = (*yearTable);
		for (int i = 0; i < lhsyear.size1();++i) {
			for (int j = 0; j < lhsyear.size2(); ++j) {
				lhsyear[i][j] = 1;
			}
		}
	}
}

}
