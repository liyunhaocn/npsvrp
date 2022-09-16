
#ifndef CN_HUST_LYH_EAX_H
#define CN_HUST_LYH_EAX_H

#include "Solver.h"

namespace hust {

struct EAX
{
public:

	enum class Owner { UNKNOWN = -1, Pa, Pb, COMMON };

	struct Edge {
		int a = -1;
		int b = -1;
		Edge() :a(-1), b(-1) {}
		Edge(int aa, int bb) :a(aa), b(bb) {}
		~Edge() {}
	};

	struct RichEdge {

		int index = -1; // �ڴ洢�ռ�� ID
		Edge e;
		int code = -1; // �ߵĹ�ϣ��
		Owner owner = Owner::UNKNOWN; // �ߵ�����Ȩ���� 0 pa 1 pb 2 pc -1 δ֪
		//int baseRouteID, goalRouteID; // ��˫���е�·�� ID
		bool visited = false; // �Ƿ��ѷ���
		//int cycleID=-1; // �ñ߱����ֵ��� AB-Cycle �� ID

		RichEdge() :
			index(-1)
			, e(-1, -1)
			, code(-1)
			, owner(Owner::UNKNOWN)
			//baseRouteID(-1), goalRouteID(-1),
			, visited(false)
			//,cycleID(-1)
		{ }

		/* �������� */
		bool set(
			int index,
			Edge e,
			int code, // �ߵĹ�ϣ��
			Owner owner, // �ߵ�����Ȩ���� 0 pa 1 pb 2 pc -1 δ֪
			int visited, // �Ƿ��ѷ���
			int cycleID)
		{

			this->index = index;
			this->e = e;
			this->code = code;
			this->owner = owner;
			//baseRouteID(-1), goalRouteID(-1),
			this->visited = visited;
			//this->cycleID = cycleID;
			return true;
		}
	};

	/* ��֧�ֽڵ��� */
	int supportNumNodes = -1;

	int richEdgeCurSize = 0; // `richEdges` ��һ�η���� ID
	Vec<RichEdge> richEdges = {}; // �洢˫�����б���Ϣ
	Vec<Vec<int>> adjEdgeTable; // ��ǰ GAB ��ÿ���ڵ���ڽ�rich edge�б�

	// ӳ�� `code` �� `richEdges` �� ID
	UnorderedMap<int, int>	papbEMap; // ����

	ConfSet paPriE; // pa ����
	ConfSet pbPriE;// pb ���� 
	Vec<Vec<int>> abCycleSet; // һ�ηֽ� GAB �õ��� AB-Cycle ����
	int generSolNum = -1; //�����˶��ٽ�
	int repairSolNum = -1; //�����˶��ٽ�
	//Vec<bool> visited; // ���һ���ڵ��Ƿ��ѷ���
	int GabEsize = 0;
	
	int eaxCusCnt = -1;
	int eaxRCnt = -1;
	int subCyNum = 0;
	int subCyCusNum = 0;
	
	Vec< Vec<int> > unionArr;
	Vec< Vec<int> > abCyAdj;
	UnorderedSet<int> tabuCyIds;

	int unionIndex = -1;
	int choosecyIndex = -1;

	EAX(Solver& pa, Solver& pb);

	/* �ӱߵ���ϣ�� */
	int toCode(int i, int j);

	/* �ӹ�ϣ�뵽�� */
	Edge toEdge(int code);

	/* ˫�ױ߼����� */
	bool classifyEdges(Solver& pa, Solver& pb);

	/* �ֽ� GAB, ��� AB-Cycle */
	bool generateCycles();

	/* �����Ƹ���Ŀͻ��ڵ�������Ϣ */
	//�Ը���Ӧ�ø��� AB-Cycle; Ŀ��·����Ϊ `params.preprocess.numRoutes`
	bool applyOneCycle(int& cycleIndex, Solver& pc);

	/* �Ը���Ӧ�ø��� eSet ����; */
	bool applyCycles(const Vec<int>& cyclesIndexes, Solver& pc);

	//int sameCnt = 0;

	Solver::Position findBestPosRemoveSubtour(Solver& pc, int w, int wj, DisType deInSub);

	int removeSubring(Solver& pc);

	UnorderedSet<int> getCusInOneCycle(int cyIndex);

	bool isInter(const UnorderedSet<int>& a, const UnorderedSet<int>& b);

	void getUnionArr();

	int doNaEAX(Solver& pa, Solver& pb, Solver& pc);

	int doPrEAX(Solver& pa, Solver& pb, Solver& pc);

	static int getabCyNum(Solver& pa, Solver& pb);
	
	static Vec<int> getDiffCusofPb(Solver& pa, Solver& pb);

};

}
#endif // !CN_HUST_LYH_EAX_H
