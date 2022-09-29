
#include "EAX.h"
#include "Goal.h"

namespace hust {

Goal::Goal() {

	eaxTabuTable = Vec<Vec<bool>>
		(globalCfg->popSize, Vec<bool>(globalCfg->popSize,false));

}

DisType Goal::getMinRtCostInPool(int rn) {
	DisType bestSolInPool = DisInf;
	auto& pool = ppool[rn];
	for (int i = 0; i < pool.size(); ++i) {
		bestSolInPool = std::min<DisType>(bestSolInPool, pool[i].RoutesCost);
	}
	return bestSolInPool;
}

DisType Goal::doTwoKindEAX(Solver& pa, Solver& pb, int kind) {

	int retState = 0; // 0 表示没有成功更新最优解，1表示更新了最优解 -1表示这两个解无法进行eax

	EAX eax(pa, pb);
	eax.generateCycles();

	if (eax.abCycleSet.size() <= 1) {
		return -1;
	}
	if (kind == 1 && eax.abCycleSet.size() < 4) {
		return -1;
	}

	Solver paBest = pa;

	int contiNotRepair = 1;

	for (int ch = 1; ch <= globalCfg->naEaxCh && !globalInput->para.isTimeLimitExceeded(); ++ch) {

		Solver pc = pa;
		int eaxState = 0;
		if (kind == 0) {
			eaxState = eax.doNaEAX(pa, pb, pc);
		}
		else {
			eaxState = eax.doPrEAX(pa, pb, pc);
		}

		if (eaxState == -1) {
			//break;
			eax.generateCycles();
			continue;
		}

		//++genSol;
		if (eax.repairSolNum == 0) {
			++contiNotRepair;
			if (contiNotRepair >= 3) {
				eax.generateCycles();
			}
			continue;
		}
		else {
			contiNotRepair = 1;
		}

		//++repSol;
		auto newCus = EAX::getDiffCusofPb(pa, pc);

		if (newCus.size() > 0) {
			//pc.mRLLocalSearch(0, {});
			pc.mRLLocalSearch(1, newCus);
			//auto cus1 = EAX::getDiffCusofPb(pa, pc);
			//if (cus1.size() == 0) {
			//	//debug("pa is same as pa");
			//}
		}
		else {
			//debug("pa is same as pa");
		}

		if (pc.RoutesCost < paBest.RoutesCost) {
			paBest = pc;
			
			bool isup = bks->updateBKSAndPrint(pc," eax ls, kind:" + std::to_string(kind));
			if (isup) {
				ch = 1;
				retState = 1;
				//INFO("bestSolFound cost:", bestSolFound.RoutesCost, ", kind, "choosecyIndex:", eax.choosecyIndex, "chooseuId:", eax.unionIndex);
			}
		}

	}

	if (paBest.RoutesCost < pa.RoutesCost) {
		if (paBest.RoutesCost < pb.RoutesCost) {
			auto diffwithPa = EAX::getDiffCusofPb(paBest, pa);
			auto diffwithPb = EAX::getDiffCusofPb(paBest, pb);
			if (diffwithPa.size() <= diffwithPb.size()) {
				pa = paBest;
			}
			else {
				//INFO("replace with pb,kind:", kind);
				pb = paBest;
			}
		}
		else {
			pa = paBest;
		}
	}
	return paBest.RoutesCost;
}

bool Goal::perturbOneSol(Solver& sol) {

	//auto before = sol.RoutesCost;
	Solver sclone = sol;

	bool isPerOnePerson = false;
	for (int i = 0; i < 10; ++i) {

		//尝试使用 100度的模拟退火进行扰动
		//sclone.Simulatedannealing(0,100,100.0,globalCfg->ruinC_);
		
		if (myRand->pick(2)==0) {
			int perkind = myRand->pick(5);
			int clearEPkind = myRand->pick(6);
			int ruinCusNum = std::min<int>(globalInput->custCnt/2, globalCfg->ruinC_);
			sclone.perturbBaseRuin(perkind, ruinCusNum, clearEPkind);
		}
		else{
			int step = myRand->pick(sclone.input.custCnt * 0.2, sclone.input.custCnt * 0.3);
			sclone.patternAdjustment(step);
		}
		 
		auto diff = EAX::getDiffCusofPb(sol, sclone);
		//INFO("i:",i,"diff.size:",diff.size());
		if (diff.size() > sol.input.custCnt * 0.2) {
			sclone.mRLLocalSearch(1, diff);
			sol = sclone;
			isPerOnePerson = true;
			break;
		}
		else {
			sclone = sol;
		}
	}
	return isPerOnePerson;
}

int Goal::naMA(int rn) { // 1 代表更新了最优解 0表示没有

	auto& pool = ppool[rn];

	auto& ords = myRandX->getMN(globalCfg->popSize, globalCfg->popSize);
	//myRand->shuffleVec(ords);

	DisType bestSolInPool = getMinRtCostInPool(curSearchRN);

	//TODO[-1]:naMA这里改10了
	for (int ct = 0; ct < 10; ct++) {
		myRand->shuffleVec(ords);
		for (int i = 0; i < globalCfg->popSize; ++i) {
			int paIndex = ords[i];
			int pbIndex = ords[(i + 1) % globalCfg->popSize];
			Solver& pa = pool[paIndex];
			Solver& pb = pool[pbIndex];

#if CHECKING
			
			if (pa.verify() < 0) {
				ERROR("pa.verify():", pa.verify());
			}

			if (pb.verify() < 0) {
				ERROR("pb.verify():", pb.verify());
			}
#endif // CHECKING

			doTwoKindEAX(pa, pb, 0);
		}
		
		DisType curBest = getMinRtCostInPool(curSearchRN);
		if (curBest < bestSolInPool ) {
			bestSolInPool = curBest;
			ct = 0;
		}
	}

	//TODO[-1]:naMA这里改10了
	bestSolInPool = getMinRtCostInPool(curSearchRN);
	for (int ct = 0; ct < 10; ct++) {
		myRand->shuffleVec(ords);
		for (int i = 0; i < globalCfg->popSize; ++i) {
			int paIndex = ords[i];
			int pbIndex = ords[(i + 1) % globalCfg->popSize];
			Solver& pa = pool[paIndex];
			Solver& pb = pool[pbIndex];
			doTwoKindEAX(pa, pb, 1);
		}

		DisType curBest = getMinRtCostInPool(curSearchRN);
		if (curBest < bestSolInPool) {
			bestSolInPool = curBest;
			ct = 0;
		}
	}

	for (int i = 0; i < globalCfg->popSize; ++i) {
		if (bks->bestSolFound.rts.cnt == pool[i].rts.cnt) {
			auto pa = bks->bestSolFound;
			auto pb = pool[i];
			doTwoKindEAX(pa, pb, 0);
			doTwoKindEAX(pa, pb, 1);
		}
	}

	INFO("curSearchRN:",curSearchRN,"getMinRtCostInPool():", getMinRtCostInPool(curSearchRN));
	return 0;
}

int Goal::gotoRNPop(int rn) {

	if (ppool.count(rn) == 0) {
		return poprnLowBound;
		throw std::string(LYH_FILELINEADDS("rn > poprnUpBound || rn < poprnLowBound"));
	}
	
	if (rn > poprnUpBound || rn < poprnLowBound) {
		return poprnLowBound;
		INFO(("rn > poprnUpBound || rn < poprnLowBound"));
		throw std::string(LYH_FILELINEADDS("rn > poprnUpBound || rn < poprnLowBound"));
	}

	//TODO[0]:Lmax和ruinLmax的定义
	globalCfg->ruinLmax = globalInput->custCnt / rn;
	//globalCfg->ruinC_ = (globalCfg->ruinLmax + 1)/2;
	//globalCfg->ruinC_ = (globalCfg->ruinLmax + 1);
	//globalCfg->ruinC_ = 15;
	//globalCfg->ruinC_ = std::max<int>(globalCfg->ruinC_, (globalCfg->ruinLmax + 1) / 2);

	auto& pool = ppool[rn];

	if (rn == poprnLowBound) { //r如果是
		return rn;
	}

	for (int pIndex = 0; pIndex < globalCfg->popSize; ++pIndex) {

		auto& sol = pool[pIndex];
		//if (sol.rts.cnt != rn) {

		bool isAdj = false;

		//TODO[-1]:从刚才搜索的位置跳
		int downRn = -1;
		DisType minRc = DisInf;
		
		for (int i = poprnUpBound; i >= poprnLowBound; --i) {
			if (ppool[i][pIndex].rts.cnt == i) {
				if (ppool[i][pIndex].RoutesCost < minRc) {
					minRc = ppool[i][pIndex].RoutesCost;
					downRn = i;
				}
			}
		}

#if CHECKING
		if (downRn == -1) {
			ERROR("downRn:", downRn);
			ERROR("rn:", rn);
		}
#endif // CHECKING

		if (downRn == rn) {

#if CHECKING
			if (sol.rts.cnt != rn) {
				ERROR("sol.rts.cnt != rn");
				ERROR("sol.rts.cnt:", sol.rts.cnt);
				ERROR("rn:",rn);
				ERROR("rn:",rn);
			}
#endif // CHECKING

			sol.patternAdjustment(100);
			isAdj = true;
		}
		else {
			sol = ppool[downRn][pIndex];
			isAdj = sol.adjustRN(rn);
		}

		if (!isAdj) {

			sol = ppool[poprnLowBound][pIndex];
			
			isAdj = sol.adjustRN(rn);
		}

#if CHECKING
		if (sol.rts.cnt != rn) {
			ERROR("downRn:", downRn);
			ERROR("isAdj:", isAdj);
			ERROR("rn:", rn);
			ERROR("sol.rts.cnt:", sol.rts.cnt);
			ERROR("sol.rts.cnt:", sol.rts.cnt);
			throw std::string(LYH_FILELINEADDS("sol.rts.cnt != rn"));
		}
#endif // CHECKING

		updateppol(sol, pIndex);
		bks->updateBKSAndPrint(sol, "adjust from cur + ls");
	//}
	}
	return rn;
}

bool Goal::fillPopulation(int rn) {

	auto& pool = ppool[rn];

	if (pool.size() == 0 ) {
		pool.resize(globalCfg->popSizeMax);
	}

	return true;
}

int Goal::callSimulatedannealing() {

	//int ourTarget = globalCfg->lkhRN;
	int ourTarget = 0;

	Solver st;

	st.initSolution(0);
	st.adjustRN(ourTarget);

	st.mRLLocalSearch(0, {});
	st.Simulatedannealing(0,1000, 20.0, 1);
	bks->updateBKSAndPrint(st, "Simulatedannealing(0,1000, 20.0, 1)");

	st.Simulatedannealing(1, 1000, 20.0, globalCfg->ruinC_);
	bks->updateBKSAndPrint(st,"Simulatedannealing(1, 1000, 20.0, globalCfg->ruinC_)");

	//saveSlnFile();
	return true;
}

bool Goal::experOnMinRN() {
	//globalInput->initDetail();

	Solver sol;
	sol.initSolution(3);
	
	bool succeed = sol.minimizeRN(2);
	if (succeed) {
		INFO("succeed to delete");
	}
	else {
		INFO("fail to delete");
	}
	saveSolutiontoCsvFile(sol);

	return true;
}

void Goal::updateppol(Solver& sol, int index) {

	int tar = sol.rts.cnt;

	if (sol.rts.cnt < poprnLowBound || sol.rts.cnt > poprnUpBound) {
		return;
		throw std::string(LYH_FILELINEADDS("rn > poprnUpBound || rn < poprnLowBound"));
	}

	if (ppool[tar].size() == 0) {
		throw std::string(LYH_FILELINEADDS("ppool[tar].size() == 0"));
	}

	if (sol.RoutesCost < ppool[tar][index].RoutesCost) {
		INFO("update ppool rn:", tar, "index:", index);
		ppool[tar][index] = sol;
	}

};

void Goal::getTheRangeMostHope() {

	Solver sol;
	sol.initSolution(0);

	int adjBig = std::min<int>(globalInput->vehicleCnt, sol.rts.cnt + 15);

	if (adjBig > sol.rts.cnt) {
		sol.adjustRN(adjBig);
	}
	
	globalCfg->ruinLmax = globalInput->custCnt / sol.rts.cnt;
	//globalCfg->ruinC_ = (globalCfg->ruinLmax + 1);
	
	int& mRLLocalSearchRange1 = globalCfg->mRLLocalSearchRange[1];
	mRLLocalSearchRange1 = 40;

	sol.Simulatedannealing(1, 1000, 100.0, globalCfg->ruinC_);
	
	if (globalInput->custCnt < sol.rts.cnt * 25 ) {
		//short route
		globalCfg->popSizeMin = 2;
		globalCfg->popSizeMax = 6;
		globalCfg->popSize = globalCfg->popSizeMin;
		globalCfg->neiSizeMax = 25;
	}
	else {//long route
		globalCfg->popSizeMin = 4;
		globalCfg->popSizeMax = 50;
		globalCfg->popSize = globalCfg->popSizeMin;
		globalCfg->neiSizeMax = 35;
	}

	Vec<Solver> poolt(globalCfg->popSizeMax);
	poolt[0] = sol;
	updateppol(sol, 0);

	for (int i = 1; i < globalCfg->popSizeMax; ++i) {
		int kind = (i == 4 ? 4 : i % 4);
		//int kind = (i % 4);
		poolt[i].initSolution(kind);
		//poolt[i].initSolution(1);
		//DEBUG("poolt[i].rts.cnt:", poolt[i].rts.cnt);
		//DEBUG("globalInput->custCnt:", globalInput->custCnt);
		int adjBig = std::min<int>(globalInput->vehicleCnt, poolt[i].rts.cnt + 15);

		poolt[i].adjustRN(adjBig);

		if (i <= 4 ) {
			//poolt[i].mRLLocalSearch(0, {});
			globalCfg->ruinLmax = globalInput->custCnt / poolt[i].rts.cnt;
			//globalCfg->ruinC_ = (globalCfg->ruinLmax + 1);
			// TODO[lyh][Goal][-1]:这里记得还原注释
			//poolt[i].Simulatedannealing(1, 500, 100.0, globalCfg->ruinC_);
			updateppol(poolt[i], i);
		}
		bks->updateBKSAndPrint(poolt[i], " poolt[i] init");
	}

	bks->resetBksAtRn();
	mRLLocalSearchRange1 = globalCfg->neiSizeMin;

	Vec <Vec<Solver>> soles(globalCfg->popSizeMax);

	int glbound = IntInf;

	for (int peopleIndex = 0; peopleIndex < globalCfg->popSizeMax; ++peopleIndex) {
		auto& sol = poolt[peopleIndex];
		soles[peopleIndex].push_back(sol);

		//sol.Simulatedannealing(0, 2, 1.0, globalCfg->ruinC_);
		if (sol.rts.cnt < 2) {
			sol.adjustRN(5);
		}

		glbound = std::min<int>(glbound, poolt[peopleIndex].rts.cnt);
		//glbound = std::min<int>(glbound, poolt[0].rts.cnt);
		// TODO[lyh][Goal][-1]:bound
		int bound = (peopleIndex == 0 ? 2 : glbound);
		//int bound = (peopleIndex == 0 ? sol.rts.cnt-1 : glbound);
		while (sol.rts.cnt > bound && !globalInput->para.isTimeLimitExceeded()) {
		//while (sol.rts.cnt > 2) {
			soles[peopleIndex].push_back(sol);
			bool isDel = sol.minimizeRN(sol.rts.cnt - 1);
			if (!isDel) {
				break;
			}
			bks->updateBKSAndPrint(sol, " poolt[0] mRLS(0, {})");
		}
	}
	
	poprnUpBound = 0;
	poprnLowBound = IntInf;
	for (int i = 0; i < globalCfg->popSizeMax; ++i) {
		//DEBUG(i);
		poprnUpBound = std::max<int>(poprnUpBound, soles[i].front().rts.cnt);
		poprnLowBound = std::min<int>(poprnLowBound, soles[i].back().rts.cnt);
	}

	poprnUpBound = std::min<int>(poprnLowBound + 15, globalInput->custCnt);
	poprnUpBound = std::max<int>(poprnUpBound,bks->bestSolFound.rts.cnt);
	poprnLowBound = std::min<int>(poprnLowBound,bks->bestSolFound.rts.cnt);

	//TODO[-1]:这个很重要 考虑了vehicleCnt！！！
	poprnUpBound = std::min<int>(poprnUpBound, globalInput->vehicleCnt);

	INFO("poprnLowBound:",poprnLowBound,"poprnUpBound:", poprnUpBound);

	if (poprnLowBound > globalInput->vehicleCnt) {
		throw std::string("!!!!!this alg finshed! even cant get a sol sat vehicleCnt");
	}
	INFO("soles.size():", soles.size());

	for (int rn = poprnLowBound; rn <= poprnUpBound; ++rn) {
		fillPopulation(rn);
	}

	// 所有解
	for (int popIndex = 0;popIndex< globalCfg->popSizeMax;++popIndex) {
		auto& deorsoles = soles[popIndex];
		for (auto& sol : deorsoles) {
			if (sol.rts.cnt >= poprnLowBound
				&& sol.rts.cnt <= poprnUpBound) {
				ppool[sol.rts.cnt][popIndex] = sol;
			}
		}
	}

	std::queue<int> alreadyBound;

	for (int i = 0; i < globalCfg->popSizeMax; ++i) {
		if (ppool[poprnLowBound][i].rts.cnt == poprnLowBound) {
			alreadyBound.push(i);
		}
	}
	if (bks->bestSolFound.rts.cnt == poprnLowBound) {
		alreadyBound.push(globalCfg->popSizeMax);
	}

	INFO("alreadyBound.size():", alreadyBound.size());
	lyhCheckTrue(alreadyBound.size() > 0);
	
	for (int i = 0; i < globalCfg->popSizeMax; ++i) {
		auto& sol = ppool[poprnLowBound][i];
		if (sol.rts.cnt != poprnLowBound) {
			int index = alreadyBound.front();
			alreadyBound.pop();
			alreadyBound.push(i);
			alreadyBound.push(index);
			sol = (index == globalCfg->popSizeMax ? bks->bestSolFound : ppool[poprnLowBound][index]);
		}
	}

	for (int i = 0; i < globalCfg->popSizeMax && !globalInput->para.isTimeLimitExceeded(); ++i) {
		auto& sol = ppool[poprnLowBound][i];
		sol.patternAdjustment(100);
	}

	globalCfg->popSize = globalCfg->popSizeMax;
//	for (int i = poprnUpBound ; i >= poprnLowBound; --i) {
	for (int i = poprnLowBound  ; i <= poprnUpBound; ++i) {
		curSearchRN = gotoRNPop(i);
	}
	//globalCfg->popSize = globalCfg->popSizeMin;
}

int Goal::TwoAlgCombine() {

	getTheRangeMostHope();

	std::queue<int>qunext;

	int iterFillqu = 0;

	globalCfg->popSize = globalCfg->popSizeMin;

	auto fillqu = [&]() -> void {

		Vec<int> rns;
		int bksRN = bks->bestSolFound.rts.cnt;
		if (bksRN == poprnUpBound) {
			rns = { bksRN, bksRN - 1 ,bksRN - 2 };
		}
		else if (bksRN == poprnLowBound) {
			rns = { bksRN, bksRN + 1 ,bksRN + 2 };
		}
		else {
			rns = { bksRN,bksRN + 1 ,bksRN - 1 };
		}

		std::sort(rns.begin(), rns.end(), [&](int x, int y) {
			return bks->bksAtRn[x] < bks->bksAtRn[y];
		});

		int n = rns.size();
		for (int i = 0; i < n ; ++i) {
		//for (int i = 0; i < 3; ++i) {
			qunext.push(rns[i]);
		}

		for (int rn : rns) {
			auto& pool = ppool[rn];
			int pn = pool.size();
			for (int i = 0; i +1 < pn; ++i) {
				int rdi = myRand->pick(i+1,pn);
				std::swap(pool[i], pool[rdi]);
			}
		}

	};
	
	auto getNextRnSolGO = [&]() -> int {

		//return bks->bestSolFound.rts.cnt;

		if(qunext.size() == 0) {

			fillqu();

			globalCfg->popSize *= 3;
			globalCfg->popSize = std::min<int>(globalCfg->popSize, globalCfg->popSizeMax);

			globalCfg->ruinC_ += 3;
			if (globalCfg->ruinC_ > globalCfg->ruinC_Max) {
				globalCfg->ruinC_ = globalCfg->ruinC_Min;
			}

			int& mRLLocalSearchRange1 = globalCfg->mRLLocalSearchRange[1];
			mRLLocalSearchRange1 += 5;
			if (mRLLocalSearchRange1 > globalCfg->neiSizeMax) {
				mRLLocalSearchRange1 = globalCfg->neiSizeMin;
			}
			//mRLLocalSearchRange1 = std::min<int>(globalCfg->neiSizeMax, mRLLocalSearchRange1);

		}

		INFO("globalCfg->popSize:", globalCfg->popSize);
		INFO("qunext.size():", qunext.size());
		auto q = qunext;
		while (!q.empty()) {
			INFO("q.front():", q.front());
			q.pop();
		}

		int ret = qunext.front();
		qunext.pop();

		return ret;
	};

	int rnn = getNextRnSolGO();
	curSearchRN = gotoRNPop(rnn);

	DisType bksLastLoop = bks->bestSolFound.RoutesCost;
	int contiNotDown = 1;

#if CHECKING
	for (int rn = poprnLowBound; rn <= poprnUpBound; ++rn) {
		for (int i = 0; i < globalCfg->popSizeMax; ++i) {
			if (ppool[rn][i].rts.cnt != rn) {
				ERROR("rn:", rn);
				ERROR("i:", i);
				ERROR("i:", i);
			}
		}
	}
#endif // CHECKING

	while (!globalInput->para.isTimeLimitExceeded()) {
		
#if CHECKING
		for (int rn = poprnLowBound; rn <= poprnUpBound; ++rn) {
			for (int i = 0; i < globalCfg->popSizeMax; ++i) {
				if (ppool[rn][i].rts.cnt != rn) {
					ERROR("rn:", rn);
					ERROR("i:", i);
				}
			}
		}
#endif // CHECKING

		int& neiSize = globalCfg->mRLLocalSearchRange[1];

		INFO("curSearchRN:", curSearchRN,
			"popSize:",globalCfg->popSize,
			"globalCfg->ruinC_:", globalCfg->ruinC_,
			"neiSize:", neiSize);

		bksLastLoop = bks->bksAtRn[curSearchRN];

		auto& pool = ppool[curSearchRN];
		globalRepairSquIter();

		naMA(curSearchRN);

		int outpeopleNum = std::min<int>(5,globalCfg-> popSize);
		auto& arr = myRandX->getMN(globalCfg->popSize, outpeopleNum);

		for (int i = 0; i < outpeopleNum && !globalInput->para.isTimeLimitExceeded(); ++i) {
			int index = arr[i];
			Solver& sol = pool[index];
			Solver clone = sol;
			//clone.Simulatedannealing(0, 50, 30.0, globalCfg->ruinC_);
			//bks->updateBKSAndPrint(clone, " pool sol simulate 0");
			//updateppol(sol, index);
			//clone = sol;
			clone.Simulatedannealing(1, 100, 50.0, globalCfg->ruinC_);
			bks->updateBKSAndPrint(clone, " pool sol simulate 1");
			updateppol(sol, i);
		}

		Solver& sol = bks->bestSolFound;
		Solver clone = sol;
		//clone.Simulatedannealing(0, 100, 50.0, globalCfg->ruinC_);
		//bks->updateBKSAndPrint(clone, " bks ruin simulate 0");
		//updateppol(sol, 0);
		//clone = sol;
		clone.Simulatedannealing(1, 500, 100.0, globalCfg->ruinC_);
		bks->updateBKSAndPrint(clone, " bks ruin simulate 1");
		updateppol(sol, 0);
		
		if (bks->bksAtRn[curSearchRN] < bksLastLoop) {
			contiNotDown = 1;
		}
		else {
			++contiNotDown;
		}

		int plangoto = -1;
		if (contiNotDown >= 2) {
			plangoto = getNextRnSolGO();
		}

		if (plangoto != -1) {
			curSearchRN = gotoRNPop(plangoto);
			if (curSearchRN == plangoto) {
				INFO("jump succeed curSearchRN", curSearchRN);
			}
			else {
				INFO("jump fail curSearchRN", curSearchRN, "plangoto:", plangoto);
			}
		}
	}

	INFO(globalInput->para.getTimeElapsedSeconds());

	return true;
}

void Goal::test(Genetic* hgsSolver) {

	std::pair<Solver,Solver> parent;
	auto& pa = parent.first;
	auto& pb = parent.second;
	parent.first.initSolution(0);
	parent.second.initSolution(1);

	int adjBig = std::min<int>(globalInput->vehicleCnt, pa.rts.cnt + 10);

	pa.adjustRN(adjBig);

	globalCfg->ruinLmax = globalInput->custCnt / pa.rts.cnt;
	//globalCfg->ruinC_ = (globalCfg->ruinLmax + 1);
	int& mRLLocalSearchRange1 = globalCfg->mRLLocalSearchRange[1];
	mRLLocalSearchRange1 = 40;
	//pa.Simulatedannealing(1, 1000, 100.0, globalCfg->ruinC_);

	//while (!globalInput->para.isTimeLimitExceeded()) {
	//	auto paIndividual =  pa.exportIndividual();
	//	auto pbIndividual = pb.exportIndividual();
	//	Individual* offspring = hgsSolver->crossoverOX({&paIndividual,&pbIndividual });
	//	
	//	Solver pc;
	//	pc.loadSolutionByArr2D(offspring->chromR);
	//	if (pc.penalty > 0) {
	//		if (pc.repair()) {
	//			pc.mRLLocalSearch(0,{});
	//			if (pc.RoutesCost < pa.RoutesCost) {
	//				pa = pc;
	//			}
	//		}
	//	}
	//	else {
	//		pc.mRLLocalSearch(0, {});
	//		pa.patternAdjustment();
	//		pb.patternAdjustment();
	//	}
	//}

}

}
