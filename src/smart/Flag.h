
#ifndef CN_HUST_LYH_FLAG_H
#define CN_HUST_LYH_FLAG_H

#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <numeric>
#include <iostream>
#include <list>
#include <sstream>

#include "Arr2D.h"
#include "../hgs/LocalSearch.h"
//#include "Utility.h"

//#define HUST_LYH_NPSRUN 0
#define CHECKING 0

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
using Map = std::map<T, U>;

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

const static double PI_1 = 3.14159265359;
const static double PI_2 = PI_1 / 2;
const static double PI_4 = PI_1 / 4;
const static double PI_8 = PI_1 / 8;
const static double PI_16 = PI_1 / 16;
const static double PI_32 = PI_1 / 32;

const static int vd2pi = 65536;
const static int vdpi = vd2pi/2;
const static int vd2fpi = vdpi/2;
const static int vd4fpi = vd2fpi/2;
const static int vd8fpi = vd4fpi/2;
const static int vd16fpi = vd8fpi/2;


extern int disMul;
extern unsigned Mod;

struct MyString {
public:
    //split
    Vec<std::string> split(std::string str, std::string s) {
        Vec<std::string> ret;
        if (s.size() == 0) {
            ret.push_back(str);
            return ret;
        }
        std::size_t pos = 0;
        while ((pos = str.find(s)) != std::string::npos) {
            if (pos > 0) {
                ret.push_back(str.substr(0, pos));
            }
            str = str.substr(pos + s.size());
        }
        if (str.size() > 0) {
            ret.push_back(str);
        }
        return ret;
    }
    //string to LL
    int str_int(std::string s) {
        std::stringstream ss;
        int ret;
        ss << s;
        ss >> ret;
        return ret;
    }

    std::string int_str(int s) {
        std::stringstream ss;
        ss << s;
        return ss.str();
    }
};

struct Random {
public:
    using Generator = SmartRandomGenerator;
    Generator rgen;
    int seed = -1;
    Random(int seed) : rgen(seed),seed(seed) {}
    Random() : rgen(generateSeed()) {}

    static int generateSeed() {
        return static_cast<int>(std::time(nullptr) + std::clock());
    }

    Generator::result_type operator()() { return rgen(); }

    // pick with probability of (numerator / denominator).
    bool isPicked(unsigned numerator, unsigned denominator) {
        return ((rgen() % denominator) < numerator);
    }

    // pick from [min, max).
    int pick(int min, int max) {
        return ((rgen() % (max - min)) + min);
    }
    // pick from [0, max).
    int pick(int max) {
        return (rgen() % max);
    }

    void shuffleVec(Vec<int>& v) {
        std::shuffle(v.begin(), v.end(), rgen);
    }
};

struct RandomX {

public:

    using Generator = SmartRandomGenerator;

    RandomX(unsigned seed) : rgen(seed) { initMpLLArr(); }
    RandomX() : rgen(generateSeed()) { initMpLLArr(); }

    RandomX(const RandomX& rhs) {
        this->mpLLArr = rhs.mpLLArr;
        this->maxRange = rhs.maxRange;
        this->rgen = rhs.rgen;
    }

    Vec< Vec<int> > mpLLArr;

    int maxRange = 1001;

    bool initMpLLArr() {
        mpLLArr = Vec< Vec<int> >(maxRange);

        for (int m = 1; m < maxRange;++m) {
            mpLLArr[m] = Vec<int>(m, 0);
            auto& arr = mpLLArr[m];
            std::iota(arr.begin(), arr.end(), 0);
        }
        return true;
    }

    static unsigned generateSeed() {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        return seed;
    }

    Generator::result_type operator()() { return rgen(); }

    // pick with probability of (numerator / denominator).
    bool isPicked(unsigned numerator, unsigned denominator) {
        return ((rgen() % denominator) < numerator);
    }

    // pick from [min, max).
    int pick(int min, int max) {
        return ((rgen() % (max - min)) + min);
    }
    // pick from [0, max).
    int pick(int max) {
        return (rgen() % max);
    }

    Vec<int>& getMN(int M, int N) {

        if (M >= maxRange) {
            mpLLArr.resize(M * 2 + 1);
            maxRange = M * 2 + 1;
        }

        Vec<int>& ve = mpLLArr[M];

        if (ve.empty()) {
            mpLLArr[M] = Vec<int>(M, 0);
            auto& arr = mpLLArr[M];
            std::iota(arr.begin(),arr.end(),0);
        }

        for (int i = 0; i < N; ++i) {
            int index = pick(i, M);
            std::swap(ve[i], ve[index]);
        }
        return ve;
    }

    RandomX& operator = (RandomX&& rhs) noexcept = delete;

    RandomX& operator = (const RandomX& rhs) = delete;

    Generator rgen;
};

// count | 1 2 3 4 ...  k   k+1   k+2   k+3  ...  n
// ------|------------------------------------------
// index | 0 1 2 3 ... k-1   k    k+1   k+2  ... n-1
// prob. | 1 1 1 1 ...  1  k/k+1 k/k+2 k/k+3 ... k/n
class Sampling {
public:
    Sampling(Random &randomNumberGenerator, int targetNumber)
            : rgen(randomNumberGenerator), targetNum(targetNumber), pickCount(0) {}

    // return 0 for not picked.
    // return an integer i \in [1, targetNum] if it is the i_th item in the picked set.
    int isPicked() {
        if ((++pickCount) <= targetNum) {
            return pickCount;
        } else {
            int i = rgen.pick(pickCount) + 1;
            return (i <= targetNum) ? i : 0;
        }
    }

    // return -1 for no need to replace any item.
    // return an integer i \in [0, targetNum) as the index to be replaced in the picked set.
    int replaceIndex() {
        if (pickCount < targetNum) {
            return pickCount++;
        } else {
            int i = rgen.pick(++pickCount);
            return (i < targetNum) ? i : -1;
        }
    }

    void reset() {
        pickCount = 0;
    }

protected:
    Random &rgen;
    int targetNum;
    int pickCount;
};

struct Timer {
public:

    using Millisecond = std::chrono::milliseconds;
    using Microsecond = std::chrono::microseconds;
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Clock = std::chrono::high_resolution_clock;

    static constexpr double MillisecondsPerSecond = 1000;

    Timer(const Millisecond& duration, const TimePoint& st = Clock::now(), std::string name = "")
            : startTime(st), endTime(startTime + duration), duration(duration), name(name) {}

    Timer(LL duration, const TimePoint& st = Clock::now(), std::string name = "")
            : startTime(st), endTime(startTime + Millisecond(duration * 1000)), duration(Millisecond(duration * 1000)), name(name) {}

    static Millisecond durationInMillisecond(const TimePoint& start, const TimePoint& end) {
        return std::chrono::duration_cast<Millisecond>(end - start);
    }

    static double durationInSecond(const TimePoint& start, const TimePoint& end) {
        return std::chrono::duration_cast<Millisecond>(end - start).count() / MillisecondsPerSecond;
    }

    static Microsecond durationInMicrosecond(const TimePoint& start, const TimePoint& end) {
        return std::chrono::duration_cast<Microsecond>(end - start);
    }

    static Millisecond toMillisecond(double second) {
        return Millisecond(static_cast<int>(second * MillisecondsPerSecond));
    }

    // there is no need to free the pointer. the format of the format string is
    // the same as std::strftime() in http://en.cppreference.com/w/cpp/chrono/c/strftime.
    static const char* getLocalTime(const char* format = "%Y-%m-%d(%a)%H:%M:%S") {
        static constexpr int DateBufSize = 64;
        static char buf[DateBufSize];
        time_t t = time(NULL);
        tm* date = localtime(&t);
        strftime(buf, DateBufSize, format, date);
        return buf;
    }
    static const char* getTightLocalTime() { return getLocalTime("%Y%m%d%H%M%S"); }

    bool isTimeOut() const {
        return (Clock::now() > endTime);
    }

    TimePoint getNow() {
        return (Clock::now());
    }

    Millisecond restMilliseconds() const {
        return durationInMillisecond(Clock::now(), endTime);
    }

    double restSeconds() const {
        return durationInSecond(Clock::now(), endTime);
    }

    Millisecond elapsedMilliseconds() const {
        return durationInMillisecond(startTime, Clock::now());
    }

    double elapsedSeconds() const {
        return durationInSecond(startTime, Clock::now());
    }

    const TimePoint& getStartTime() const { return startTime; }
    const TimePoint& getEndTime() const { return endTime; }

    void disp() {
        std::cout << name << "run:" << elapsedSeconds() << "s "
                  << elapsedMilliseconds().count() << "ms" << std::endl;
    }

    double getRunTime() {
        return durationInSecond(startTime, Clock::now());
    }

    bool reStart() {
        startTime = Clock::now();
        endTime = startTime + duration;
        return true;
    }

    bool setLenUseSecond(LL s) {
        duration = Millisecond(s * 1000);
        return true;
    }
protected:
    TimePoint startTime;
    TimePoint endTime;
    Millisecond duration;
    std::string name;
};

struct Union {

    Vec<int> fa;
    Vec<int> rank;
    Union(int maxSize) {
        fa = Vec<int>(maxSize);
        rank = Vec<int>(maxSize);
        int n = fa.size();
        for (int i = 0; i < n; ++i)
        {
            fa[i] = i;
            rank[i] = 1;
        }
    }


    int find(int x)
    {
        return x == fa[x] ? x : (fa[x] = find(fa[x]));
    }

    inline void merge(int i, int j)
    {
        int x = find(i), y = find(j);
        if (rank[x] <= rank[y])
            fa[x] = y;
        else
            fa[y] = x;
        if (rank[x] == rank[y] && x != y)
            rank[y]++;
    }

};

struct Configuration;
struct Input;
struct BKS;

extern Random* myRand;
extern RandomX* myRandX;
extern int squIter;
extern hust::util::Array2D<int>* yearTable;
extern Configuration* globalCfg;
extern Input* globalInput;
extern BKS* bks;
extern LocalSearch* hgsLocalSearch;
//extern Timer* gloalTimer;
struct ProbControl {

        Vec<int> data;
        Vec<int> sum;

        ProbControl(int maxSize) {
            data.resize(maxSize, 2);
            sum.resize(maxSize, 0);
        }

        void resetData() {
            std::fill(data.begin(), data.end(), 2);
        }

        int getIndexBasedData() {

            int n = data.size();

            auto maxEleIter = std::max_element(data.begin(), data.begin()+n);

            if ((*maxEleIter) >= 40) {
                for (auto& i : data) {
                    i = (i >> 2) + 2;
                }
            }

            sum[0] = data[0];
            for (int i = 1; i < n; ++i) {
                sum[i] = sum[i - 1] + data[i];
            }

            int allSum = sum[n-1];
            int rd = myRand->pick(1, allSum + 1);
            auto index = std::lower_bound(sum.begin(), sum.begin() + n, rd) - sum.begin();
            return index;
        }
    };

void globalRepairSquIter();
}
#endif // !CN_HUST_LYH_FLAG_H


