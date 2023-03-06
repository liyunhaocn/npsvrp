////////////////////////////////
/// 简介:	1.	常用异常类型.
/// 
/// 备注:	1.	
////////////////////////////////

#ifndef CN_HUST_UTIL_EXCEPTION_H
#define CN_HUST_UTIL_EXCEPTION_H


#include "Flag.h"

#include <exception>


namespace hust {
namespace util {

struct NotImplementedException : public std::exception {
    virtual const char* what() const noexcept override {
        return "not implemented yet.";
    }
};

struct IndexOutOfRangeException : public std::exception {
    virtual const char* what() const noexcept override {
        return "index out of range.";
    }
};

struct DuplicateItemException : public std::exception {
    virtual const char* what() const noexcept override {
        return "duplicate item.";
    }
};

struct ItemNotExistException : public std::exception {
    virtual const char* what() const noexcept override {
        return "item not exist.";
    }
};

}
}


#endif // CN_HUST_UTIL_EXCEPTION_H
