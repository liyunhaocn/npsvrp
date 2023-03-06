////////////////////////////////
/// 简介:	1.	基础算术与逻辑运算.
/// 
/// 备注:	1.	算术运算可能会允许返回非精确的结果以提高计算效率.
////////////////////////////////

#ifndef CN_HUST_SMART_UTIL_MATH_H
#define CN_HUST_SMART_UTIL_MATH_H


#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdint>

#include "Flag.h"

#if _CC_MS_VC
#include <intrin.h> // for `log2()`.
#elif _CC_GNU_GCC
#include <climits> // for `log2()`.
#endif // _CC_MS_VC


namespace hust {
namespace util {

template<typename T>
struct SafeLimit { // no overflow occurs after single binary additive operation.
    static constexpr T max = (std::numeric_limits<T>::max)() / 2 - 1;
};

namespace math {

template<typename T>
bool isInRange(T num, T lb, T ub) { // check if `num \in [lb, ub)` is true.
    return (num >= lb) && (num < ub);
}

}
}
}


#endif // CN_HUST_SMART_UTIL_MATH_H
