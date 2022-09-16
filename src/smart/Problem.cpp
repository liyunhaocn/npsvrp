
#include <string>
#include <istream>
#include <functional>

#include "Problem.h"
#include "Utility.h"
#include "../hgs/Params.h"

namespace hust {

Input::Input(Params& para):datas(para.cli),para(para) {

	this->custCnt = para.nbClients;
	this->example = para.instanceName;

	//initInput();
	//initDetail();
}

bool Input::initInput() {

	//readDimacsInstance(globalCfg->inputPath);
	//readDimacsBKS();

	P = Vec<int>(custCnt + 1, 1);

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

	auto canlinkDir = [&](int v, int w) ->bool {

		DisType av = getDisof2(0, v);
		DisType aw = av + datas[v].serviceDuration + getDisof2(v, w);
		DisType ptw = std::max<DisType>(0, aw - datas[w].latestArrival);
		DisType an = aw + datas[w].serviceDuration + getDisof2(w, 0);
		ptw += std::max<DisType>(0, an - datas[0].latestArrival);
		return ptw == 0;
	};

	auto canLinkNode = [&](int v, int w) ->bool {

		if (!canlinkDir(v, w) && !canlinkDir(w, v)) {
			return false;
		}
		return true;
	};

	for (int v = 0; v <= custCnt; ++v) {

		int idx = 0;
		for (int w = 0; w <= custCnt; ++w) {
			if (w != v) {
				addSTclose[v][idx] = w;
				++idx;
			}
		}

		auto cmp = [&](const int a, const int b) {

			//TODO[-1] 这里的比较方式进行了修改
			//return disOf[v][a] < disOf[v][b];
			//return disOf[v][a] + datas[a].serviceDuration <
			//	disOf[v][b] + datas[b].serviceDuration;

			//if (disOf[a][v] == disOf[b][v]) {
			//	return datas[a].latestArrival < datas[b].latestArrival;
			//}
			//else {
			//	return disOf[a][v] < disOf[b][v];
			//}
			//return true;

			//int aLinkv = canLinkNode(a, v);
			//int bLinkv = canLinkNode(b, v);
			//if ((aLinkv && bLinkv) || (!aLinkv && !bLinkv)) {
			//	return disOf[v][a] + datas[a].serviceDuration <
			//		disOf[v][b] + datas[b].serviceDuration;
			//}
			//else {
			//	return aLinkv ? true : false;
			//}

			int aLinkv = canLinkNode(a, v);
			int bLinkv = canLinkNode(b, v);
			if ((aLinkv && bLinkv) || (!aLinkv && !bLinkv)) {
				return getDisof2(a, v) < getDisof2(b, v);
			}
			else {
				return aLinkv ? true : false;
			}
		};
		std::sort(addSTclose[v], addSTclose[v] + addSTclose.size2(), cmp);
	}

	addSTJIsxthcloseOf = util::Array2D<int>(custCnt + 1, custCnt + 1, -1);

	for (int v = 0; v <= custCnt; ++v) {
		for (std::size_t wpos = 0; wpos < addSTclose.size2(); ++wpos) {
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

	for (std::size_t v = 0; v <= custCnt; ++v) {

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

#if 0
bool Input::readDimacsInstance(std::string& instanciaPath) {

	//debug(instanciaPath.c_str());
	FILE* file = fopen(instanciaPath.c_str(), "r");

	if (!file) {
		std::cout << instanciaPath << "ERROR: Instance path wrong." << std::endl;
		exit(EXIT_FAILURE);
	}

	char name[64];
	
	fscanf(file, "%s\n", name);
	this->example = std::string(name);
	fscanf(file, "%*[^\n]\n");
	fscanf(file, "%*[^\n]\n");
	fscanf(file, "%d %lld\n", &this->vehicleCnt, &this->Q);
	fscanf(file, "%*[^\n]\n");
	fscanf(file, "%*[^\n]\n");

	this->Q *= disMul;
	std::string line = "";

	this->datas = Vec<Data>(303);

	int index = 0;
	int id = -1, coordx = -1, coordy = -1, demand = -1;
	int ready_time = -1, due_date = -1, service_time = -1;
	int readArgNum = 0;
	while ((readArgNum = fscanf(file, "%d %d %d %d %d %d %d\n", &id, &coordx, &coordy, &demand, &ready_time, &due_date, &service_time)) == 7) {

		if (index >= datas.size()) {
			int newSize = datas.size() + datas.size() / 2;
			datas.resize(newSize);
		}

		this->datas[index].CUSTNO = id;
		this->datas[index].XCOORD = coordx * disMul;
		this->datas[index].YCOORD = coordy * disMul;
		this->datas[index].demand = demand * disMul;
		this->datas[index].earliestArrival = ready_time * disMul;
		this->datas[index].latestArrival = due_date * disMul;
		this->datas[index].serviceDuration = service_time * disMul;

		if (index > 0) {
			auto& dt = datas[index];
			dt.polarAngle = CircleSector::positive_mod
			(32768. * atan2(dt.YCOORD - datas[0].YCOORD, dt.XCOORD - datas[0].XCOORD) / PI_1);
		}
		++index;
	}
	custCnt = index - 1;
	fclose(file);
	return true;
}
#endif //0

int Input::partition(int* arr, int start, int end, std::function<bool(int, int)>cmp) {
	//int index = ( [start, end] (void)  //我试图利用随机法，但是这不是快排，外部输入不能保证end-start!=0，所以可能发生除零异常
	//              {return random()%(end-start)+start;} )(); 
	//std::swap(arr[start], arr[end]);

	int small = start - 1;
	for (int index = start; index < end; ++index) {
		if (cmp(arr[index] , arr[end])) {
			++small;
			if (small != index)
				std::swap(arr[small], arr[index]);
		}
	}
	++small;
	std::swap(arr[small], arr[end]);
	return small;
}

}
