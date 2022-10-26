////////////////////////////////
/// usage : 1.	utilities.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef CN_HUST_LYH_UTILITY_H
#define CN_HUST_LYH_UTILITY_H

//#pragma warning(disable:4996)

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
#include <climits>

#include "Flag.h"
#include "../hgs/xorshift128.h"

#define LYH_FILELINEADDS(str) ("[" + std::string(__FILE__) + "]" + "[line:" + std::to_string(__LINE__) + "]:" + str)
#define LYH_FILELINE() ("[" + std::string(__FILE__) + "]" + "[line:" + std::to_string(__LINE__) + "]:")

#if 0
#define INFO(...) hust::println_("[INFO]" + LYH_FILELINE() + ":" ,## __VA_ARGS__)
#else
#define INFO(...);
#endif // 1

#define DEBUG(...) hust::println_("[DEBUG]" + LYH_FILELINE() + ":" ,## __VA_ARGS__)
#define ERROR(...) hust::println_("[ERROR]" + LYH_FILELINE() + ":" ,## __VA_ARGS__)

namespace hust {

static void println_() { std::cerr << std::endl; }
template<typename T, typename ... Types>
static void println_(const T& firstArg, const Types&... args) {

    //cout << "size of args: " << sizeof...(args) << endl;
    std::cerr << firstArg << " ";
    println_(args...);
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

}


#endif // CN_HUST_LYH_UTILITY_H
