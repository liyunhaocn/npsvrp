
#ifndef CN_HUST_LYH_FLAG_H
#define CN_HUST_LYH_FLAG_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <iostream>
#include <list>

#include "Arr2D.h"

#define DIMACSGO 0
#define CHECKING 1

#define lyhCheckTrue(x) {				\
	if(!(x) ){							\
		std::cout						\
		<< "[" << __FILE__ << "]"		\
		<< "[line:" << __LINE__ << "]"  \
		<< "[" << __FUNCTION__ << "]: " \
		<< #x <<std::endl;				\
	}									\
}

namespace hust {

template<typename T, typename U>
using UnorderedMap = std::unordered_map<T, U>;

template<typename V>
using UnorderedSet = std::unordered_set<V>;

template<typename V>
using Vec = std::vector<V>;

template<typename V>
using List = std::list<V>;

using LL = long long int;

using DisType = LL;
extern int IntInf;
extern long long int LLInf;
extern DisType DisInf;

extern double PI_1;
extern double PI_2;
extern double PI_4;
extern double PI_8;
extern double PI_16;
extern double PI_32;

extern int vd2pi;
extern int vdpi;
extern int vd2fpi;
extern int vd4fpi;
extern int vd8fpi;
extern int vd16fpi;


extern int disMul;
extern unsigned Mod;

struct Random;
struct RandomX;
struct Configuration;
struct Input;
struct BKS;
struct Timer;

extern Random* myRand;
extern RandomX* myRandX;
extern int squIter;
extern hust::util::Array2D<int>* yearTable;
extern Configuration* globalCfg;
extern Input* globalInput;
extern BKS* bks;
extern Timer* gloalTimer;

void globalRepairSquIter();
}
#endif // !CN_HUST_LYH_FLAG_H


