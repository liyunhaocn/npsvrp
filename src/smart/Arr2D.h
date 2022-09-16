////////////////////////////////
/// 简介:	1.	使用一维数组手工计算下标实现的二维数组.
/// 
/// 备注:	1.	
////////////////////////////////

#ifndef CN_HUST_UTIL_ARR_2D_H
#define CN_HUST_UTIL_ARR_2D_H


#include <algorithm>
#include <vector>
#include <cstring>

#include "Exception.h"
#include "Math.h"

namespace hust {
namespace util {

enum ArrResetOption { AllBits0 = 0, AllBits1 = -1, SafeMaxInt = 0x3F };


template<typename T, typename IndexType = int>
class Array2D {
    static constexpr bool SafetyCheck = false;
public:
    using Index = IndexType;
    // it is always valid before copy assignment due to no reallocation.
    using Iterator = T*;
    using ConstIterator = T const *;

    explicit Array2D() : arr(nullptr), len1(0), len2(0), len(0) {}
    explicit Array2D(Index length1, Index length2) { allocate(length1, length2); }
    explicit Array2D(Index length1, Index length2, T *data)
        : arr(data), len1(length1), len2(length2), len(length1 * length2) {}
    explicit Array2D(Index length1, Index length2, const T &defaultValue) : Array2D(length1, length2) {
        std::fill(arr, arr + len, defaultValue);
    }

    Array2D(const Array2D &a) : Array2D(a.len1, a.len2) {
        if (this != &a) { copyData(a.arr); }
    }
    Array2D(Array2D &&a) : Array2D(a.len1, a.len2, a.arr) { a.arr = nullptr; }

    Array2D& operator=(const Array2D &a) {
        if (this != &a) {
            if (len != a.len) {
                clear();
                init(a.len1, a.len2);
            } else {
                len1 = a.len1;
                len2 = a.len2;
            }
            copyData(a.arr);
        }
        return *this;
    }
    Array2D& operator=(Array2D &&a) {
        if (this != &a) {
            delete[] arr;
            arr = a.arr;
            len1 = a.len1;
            len2 = a.len2;
            len = a.len;
            a.arr = nullptr;
        }
        return *this;
    }

    ~Array2D() { clear(); }

    // allocate memory if it has not been init before.
    bool init(Index length1, Index length2) {
        if (arr == nullptr) { // avoid re-init and memory leak.
            allocate(length1, length2);
            return true;
        }
        return false;
    }

    // remove all items.
    void clear() {
        delete[] arr;
        arr = nullptr;
    }

    // set all data to val. any value other than 0 or -1 is undefined behavior.
    void reset(ArrResetOption val = ArrResetOption::AllBits0) { std::memset(arr, val, sizeof(T) * len); }

    Index getFlatIndex(Index i1, Index i2) const { return (i1 * len2 + i2); }

    T* operator[](Index i1) {
        if (SafetyCheck && !math::isInRange(i1, Index(0), len1)) { throw IndexOutOfRangeException(); }
        return (arr + i1 * len2);
    }
    const T* operator[](Index i1) const {
        if (SafetyCheck && !math::isInRange(i1, Index(0), len1)) { throw IndexOutOfRangeException(); }
        return (arr + i1 * len2);
    }

    T& at(Index i) {
        if (SafetyCheck && !math::isInRange(i, Index(0), len)) { throw IndexOutOfRangeException(); }
        return arr[i];
    }
    const T& at(Index i) const {
        if (SafetyCheck && !math::isInRange(i, Index(0), len)) { throw IndexOutOfRangeException(); }
        return arr[i];
    }
    T& at(Index i1, Index i2) {
        if (SafetyCheck && !(math::isInRange(i1, Index(0), len1) && math::isInRange(i2, Index(0), len2))) { throw IndexOutOfRangeException(); }
        return arr[i1 * len2 + i2];
    }
    const T& at(Index i1, Index i2) const {
        if (SafetyCheck && !(math::isInRange(i1, Index(0), len1) && math::isInRange(i2, Index(0), len2))) { throw IndexOutOfRangeException(); }
        return arr[i1 * len2 + i2];
    }

    Iterator begin() { return arr; }
    Iterator begin(Index i1) { return arr + (i1 * len2); }
    ConstIterator begin() const { return arr; }
    ConstIterator begin(Index i1) const { return arr + (i1 * len2); }

    Iterator end() { return (arr + len); }
    Iterator end(Index i1) { return arr + (i1 * len2) + len2; }
    ConstIterator end() const { return (arr + len); }
    ConstIterator end(Index i1) const { return arr + (i1 * len2) + len2; }

    T& front() { return at(0); }
    T& front(Index i1) { return at(i1, 0); }
    const T& front() const { return at(0); }
    const T& front(Index i1) const { return at(i1, 0); }

    T& back() { return at(len - 1); }
    T& back(Index i1) { return at(i1, len - 1); }
    const T& back() const { return at(len - 1); }
    const T& back(Index i1) const { return at(i1, len - 1); }

    Index size1() const { return len1; }
    Index size2() const { return len2; }
    Index size() const { return len; }
    bool empty() const { return (len == 0); }

protected:
    // must not be called except init.
    void allocate(Index length1, Index length2) {
        len1 = length1;
        len2 = length2;
        len = length1 * length2;
        arr = new T[static_cast<size_t>(len)];
    }

    void copyData(T *data) {
        // TODO[szx][1]: what if data is shorter than arr?
        // OPT[szx][8]: use memcpy() if all callers are POD type.
        std::copy(data, data + len, arr);
    }


    T *arr;
    Index len1;
    Index len2;
    Index len;
};

}
}


#endif // CN_HUST_UTIL_ARR_2D_H
