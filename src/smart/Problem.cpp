
#include <string>
#include <istream>
#include <functional>

#include "Problem.h"
#include "Utility.h"
#include "../hgs/Params.h"

namespace hust {

Input::Input(Params& para):para(para),datas(para.cli),P(para.P) {

	this->custCnt = para.nbClients;
	this->example = para.instanceName;
	this->vehicleCnt = para.nbVehicles;
	this->Q = para.vehicleCapacity;

	//int eq = 0;
	//int neq = 0;
	//for (int i = 0; i <= custCnt; ++i) {
	//	for (int j = 0; j <= custCnt; ++j) {
	//		auto dis1 = para.timeCost.get(i,j);
	//		auto dis2 = para.timeCost.get(j,i);
	//		if (dis1 == dis2) {
	//			++eq;
	//		}
	//		else {
	//			++neq;
	//			INFO("dis1!=dis2:", "abs(dis1-dis2)", abs(dis1 - dis2));
	//		}
	//	}
	//}
	//INFO(" eq:",eq);
	//INFO("neq:",neq);

}

bool Input::initInput() {

//	P = Vec<int>(custCnt + 1, 1);
	double sumq = 0;
	for (int i = 1; i <= custCnt; ++i) {
		sumq += datas[i].demand;
	}
	Qbound = ceil(double(sumq) / Q);
	Qbound = std::max<int>(Qbound, 2);

	sectorClose = util::Array2D<int>(custCnt + 1, custCnt,0);

	for (int v = 0; v <= custCnt; ++v) {

		int idx = 0;

		for (int pos = 0; pos <= custCnt; ++pos) {
			if (pos != v) {
				sectorClose[v][idx] = pos;
				++idx;
			}
		}
		auto cmp = [&](const int a, const int b) {
			return abs(datas[a].polarAngle - datas[v].polarAngle)
				< abs(datas[b].polarAngle - datas[v].polarAngle);
		};
		//TODO[-1]:这里的排序比较浪费时间，去掉可以节省一般的初始化时间
		std::sort(sectorClose[v], sectorClose[v] + sectorClose.size2(), cmp);
	}

	addSTclose = util::Array2D<int> (custCnt + 1, custCnt,0);

//	auto canlinkDir = [&](int v, int w) ->bool {
//
//		DisType av = getDisof2(0, v);
//		DisType aw = av + datas[v].serviceDuration + getDisof2(v, w);
//		DisType ptw = std::max<DisType>(0, aw - datas[w].latestArrival);
//		DisType an = aw + datas[w].serviceDuration + getDisof2(w, 0);
//		ptw += std::max<DisType>(0, an - datas[0].latestArrival);
//		return ptw == 0;
//	};

//	auto canLinkNode = [&](int v, int w) ->bool {
//
//		if (!canlinkDir(v, w) && !canlinkDir(w, v)) {
//			return false;
//		}
//		return true;
//	};

	for (int v = 0; v <= custCnt; ++v) {
        int idx = 0;
        for (auto it:para.orderProximities[v]) {
            addSTclose[v][idx] = it.second;
            ++idx;
        }
	}

	addSTJIsxthcloseOf = util::Array2D<int>(custCnt + 1, custCnt + 1, -1);

	for (int v = 0; v <= custCnt; ++v) {
		for (int wpos = 0; wpos < static_cast<int>(addSTclose.size2()); ++wpos) {
			int w = addSTclose[v][wpos];
			addSTJIsxthcloseOf[v][w] = wpos;
		}
	}

	int deNeiSize = globalCfg->outNeiSize;
	deNeiSize = std::min(custCnt - 1, deNeiSize);

	auto iInNeicloseOf = Vec< Vec<int> >
		(custCnt + 1, Vec<int>());
	for (int i = 0; i < custCnt + 1; ++i) {
		iInNeicloseOf[i].reserve(custCnt);
	}

	for (int v = 0; v <= custCnt; ++v) {
		for (int wpos = 0; wpos < deNeiSize; ++wpos) {
			int w = addSTclose[v][wpos];
			iInNeicloseOf[w].push_back(v);
		}
	}

	iInNeicloseOfUnionNeiCloseOfI = Vec< Vec<int> >(custCnt + 1);

	for (int v = 0; v <= custCnt; ++v) {

		iInNeicloseOfUnionNeiCloseOfI[v] = Vec<int>
			(addSTclose[v], addSTclose[v] + deNeiSize);
		for (int w : iInNeicloseOf[v]) {
			if (addSTJIsxthcloseOf[v][w] >= deNeiSize) {
				iInNeicloseOfUnionNeiCloseOfI[v].push_back(w);
			}
		}
	}

	return true;
}

}
