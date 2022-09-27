
#include <climits>
#include "Flag.h"
#include "Utility.h"

namespace hust {

extern int IntInf = INT_MAX/10;
extern long long int LLInf = LLONG_MAX / 10;

extern double PI_1 = 3.14159265359;
extern double PI_2 = PI_1 / 2;
extern double PI_4 = PI_1 / 4;
extern double PI_8 = PI_1 / 8;
extern double PI_16 = PI_1 / 16;
extern double PI_32 = PI_1 / 32;

extern int vd2pi = 65536;
extern int vdpi = vd2pi/2;
extern int vd2fpi = vdpi/2;
extern int vd4fpi = vd2fpi/2;
extern int vd8fpi = vd4fpi/2;
extern int vd16fpi = vd8fpi/2;

// dimacs «10
extern int disMul = 10000;
extern DisType DisInf = LLInf;

//extern int disMul = 10000;
extern unsigned Mod = 1000000007;

extern Random* myRand = nullptr;
extern RandomX* myRandX = nullptr;
extern hust::util::Array2D<int>* yearTable = nullptr;
extern Configuration* globalCfg = nullptr;
extern Input* globalInput = nullptr;
extern BKS* bks = nullptr;
//extern Timer* gloalTimer = nullptr;
extern int squIter = 1;

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
