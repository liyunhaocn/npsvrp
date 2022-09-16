////////////////////////////////
/// usage : 1.	utilities.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef CN_HUST_LYH_UTILITY_H
#define CN_HUST_LYH_UTILITY_H

#pragma warning(disable:4996)

#include <algorithm>
#include <chrono>
#include <initializer_list>
#include <vector>
#include <map>
#include <random>
#include <iostream>
#include <iomanip>

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sstream>

#include "Flag.h"

#if DIMACSGO

#define INFO(...);

#else

#define INFO(...) hust::println_("[INFO]:",## __VA_ARGS__);

#endif // DIMACSGO

#define DEBUG(...) hust::println_("[DEBUG]:",## __VA_ARGS__);
#define ERROR(...) hust::printlnerr_("[ERROR]:",## __VA_ARGS__);

namespace hust {

static void println_() { std::cout << std::endl; }
template<typename T, typename ... Types>
static void println_(const T& firstArg, const Types&... args) {

    //cout << "size of args: " << sizeof...(args) << endl;
    std::cout << firstArg << " ";
    println_(args...);
}


static void printlnerr_() { std::cerr << std::endl; }
template<typename T, typename ... Types>
static void printlnerr_(const T& firstArg, const Types&... args) {

    std::cerr << firstArg << " ";
    printlnerr_(args...);
}

template<typename T>
static void printve(T arr) {
    std::cout << "[ ";
    for (auto& i : arr) {
        std::cout << i << ",";
    }
    std::cout << "]" << std::endl;
}

template<typename T>
static Vec<int> putEleInVec(T arr) {
    Vec<int> ret;
    ret.reserve(arr.size());
    for (const auto& i : arr) {
        ret.push_back(i);
    }
    return ret;
}

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

// This is a Xorshift random number generators, also called shift-register generators, which is a pseudorandom number generators.
// It generates the next number in the sequence by repeatedly taking the 'exclusive or' (the ^ operator) of a number with a bit-shifted version of itself.
// For more information, see: https://en.wikipedia.org/wiki/Xorshift
struct XorShift128 {

    // This random number generator uses 4 numbers.
    // Those numbers are used to repeatedly take the 'exclusive or' of a number with a bit-shifted version of itself.
    unsigned state_[4]{};

public:
    typedef unsigned result_type;

    // Constructor of the Xorshift random number generator, given a seed stored as state_[0]
    XorShift128(const int seed = 42)
    {
        state_[0] = seed;
        state_[1] = 123456789;
        state_[2] = 362436069;
        state_[3] = 521288629;
    }

    // Return the min unsigned integer value
    static constexpr size_t min()
    {
        return 0;
    }

    // Return the max unsigned integer value
    static constexpr size_t max()
    {
        return UINT_MAX;
    }

    // Defines the operator '()'. So a new random number will be returned when rng() is called on the XorShift128 instance rng.
    unsigned operator()()
    {
        // Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs"
        unsigned t = state_[3];

        // Perform a contrived 32-bit shift.
        unsigned s = state_[0];
        state_[3] = state_[2];
        state_[2] = state_[1];
        state_[1] = s;

        t ^= t << 11;
        t ^= t >> 8;

        // Return the new random number
        return state_[0] = t ^ s ^ (s >> 19);
    }
};

struct Random {
public:
    using Generator = XorShift128;
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
        std::shuffle(v.begin(), v.end(), myRand->rgen);
    }
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

}


#endif // CN_HUST_LYH_UTILITY_H
