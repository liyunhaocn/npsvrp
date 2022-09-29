

#include <algorithm>
#include <type_traits>
#include "Configuration.h"
#include "Utility.h"
#include "Solver.h"

namespace hust {

void Configuration::show() {

	INFO("inputPath:", inputPath);
	INFO("Pwei0:", Pwei0);
	INFO("Pwei1:", Pwei1);
	INFO("minKmax:", minKmax);
	INFO("maxKmax:", maxKmax);

	INFO("seed:", seed);
}

void Configuration::repairByCusCnt(int cusCnt) {

	outNeiSize = std::min<int>(cusCnt-1, outNeiSize);
	broadenWhenPos_0 = std::min<int>(cusCnt, broadenWhenPos_0);

	patternAdjustmentNnei = std::min<int>(cusCnt-1, patternAdjustmentNnei);
	patternAdjustmentGetM = std::min<int>(cusCnt-1, patternAdjustmentGetM);

	//mRLLocalSearchRange = { 10,50 };
	mRLLocalSearchRange[0] = std::min<int>(cusCnt-1, mRLLocalSearchRange[0]);
	mRLLocalSearchRange[1] = std::min<int>(cusCnt-1, mRLLocalSearchRange[1]);
	//mRLLSgetAllRange = std::min<int>(cusCnt, mRLLSgetAllRange);

	ruinLocalSearchNextNeiBroad = std::min<int>(cusCnt-1, ruinLocalSearchNextNeiBroad);

	ruinC_ = std::min<int>(cusCnt - 1, ruinC_);
}

static std::string getHelpInfo() {
	std::stringstream ss;
	ss << "------------------- solver DLLSMA ------------------------------" << std::endl;
	ss << "call solver by:" << std::endl;
	ss << "./DLLSMA -ins instancePath " << std::endl;
	ss << "[-time inputPath:string]" << std::endl;
	ss << "[-seed seed:unsigned]" << std::endl;
	ss << "[-pwei0 Pwei0:int]" << std::endl;
	ss << "[-pwei1 Pwei1:int]" << std::endl;
	ss << "[-tag tag:string]" << std::endl;
	ss << "[-isbreak isBreak:int]" << std::endl;
	ss << "[-psizemulpsum psizemulpsum:int]" << std::endl;
	return ss.str();
}

void Configuration::solveCommandLine(int argc, char* argv[]) {

	if (argc % 2 == 0 || argc < 3) {
		std::cerr << "argc num is wrong" << std::endl;
		exit(-1);
	}

	for (int i = 1; i < argc; i+=2) {
		std::string argvstr = std::string(argv[i]);
		if (argvstr == "-ins") {
			globalCfg->inputPath = std::string(argv[i + 1]);
		}
		else if (argvstr == "-seed") {
			globalCfg->seed = std::stol(argv[i+1], nullptr, 0);
		}
		else if (argvstr == "-tag") {
			globalCfg->tag = std::string(argv[i + 1]);
		}
		else if (argvstr == "-pwei0") {
			globalCfg->Pwei0 = std::stoi(argv[i + 1], nullptr, 0);
		}
		else if (argvstr == "-pwei1") {
			globalCfg->Pwei1 = std::stoi(argv[i + 1], nullptr, 0);
		}
		else if (argvstr == "-psizemulpsum") {
			globalCfg->psizemulpsum = std::stoi(argv[i + 1], nullptr, 0);
		}
		else {
			std::cerr << "--------------unknow argv------------:"<< argvstr << std::endl;
			std::cerr << getHelpInfo() << std::endl;
		}
	}

}

#if 0
static bool writeOneLineToFile(std::string path,std::string data) {
	
	std::ofstream rgbData;

	rgbData.open(path, std::ios::app | std::ios::out);

	if (!rgbData) {
		INFO("output file open errno");
		return false;
	}
	rgbData << data << std::endl;
	rgbData.close();
	return true;
}
#endif // 0

}