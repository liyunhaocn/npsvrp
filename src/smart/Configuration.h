
#ifndef CN_HUST_LYH_CFG_H
#define CN_HUST_LYH_CFG_H

#include "Flag.h"

// $(SolutionDir)$(Platform)\$(Configuration)
// $(SolutionDir)\dimacsVRP

namespace hust {

struct Configuration {

	LL seed = -1;
	std::string inputPath = "";
	std::string outputPath = "./results/";

	int squContiIter = 100;
	int squMinContiIter = 100;
	int squMaxContiIter = 199;
	int squIterStepUp = 10;

	int weightUpStep = 1;

	int Irand = 400;
	int neiMoveKind = 16;

	int Pwei0 = 1;
	int Pwei1 = 2;

	int minKmax = 1;
	int maxKmax = 4;

	int yearTabuLen = 10;
	int yearTabuRand = 10;

	// TODO[0]:看需不需要频繁构造 1000 stepC2_8_7 需要20s C1_6_6 需要6s
	int ejectLSMaxIter = 100;

	//int yearTabuLen = 20;
	//int yearTabuRand = 10;

	int popSize = 80;
	int popSizeMin = 40;
	// TODO[lyh][config.h][-1]:popSizeMax 原来是 50;
	int popSizeMax = 80;
	//int repairExitStep = 50;
	int repairExitStep = 50;

	//int initFindPosPqSize = 20;
	//int findBestPosForRuinPqSize = 10;
	//int findBestPosInSolPqSize = 3;//64

	int naEaxCh = 20;
	//int naEaxCh = 20;

	//patternAdjustment参数
	int patternAdjustmentNnei = 60;
	int patternAdjustmentGetM = 10;

	// mRLLocalSearchRange
	//TODO[-1]:这里改成了40

	double broaden = 1.2;
	//int broadenWhenPos_0 = 50;
	int broadenWhenPos_0 = 20;
	int outNeiSize = 50;

	int neiSizeMin = 20;
	int neiSizeMax = 50;
	Vec<int> mRLLocalSearchRange = { 10,20 };
	//int mRLLSgetAllRange = 50;

	// ruinLocalSearch
	int ruinLocalSearchNextNeiBroad = 5;

	int ruinSplitRate = 0; // %100 means ruinSplitRate%
	
	int ruinLmax = 20;
	//(ruinLmax+1)/2
	int ruinC_ = 15;
	int ruinC_Min = 5;
	int ruinC_Max = 20;

	int ruinWinkacRate = 97; // 100
	//TODO[-1]:为初始化设置了眨眼
	int initWinkacRate = 20; // 100
	int abcyWinkacRate = 99; // 100
    int rateOfDynamicInAndOut = 5;

	std::string tag = "";

	int psizemulpsum = 0;

	void show();

	void repairByCusCnt(int cusCnt);

	void solveCommandLine(int argc, char* argv[]);

};

}

#endif // !CN_HUST_LYH_CFG_H
