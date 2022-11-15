#ifndef BASE_GIMBAL_HPP
#define BASE_GIMBAL_HPP

#include <string>

class BaseGimbal
{
protected:
    std::string name;

public:
    virtual int init(void) = 0;
    virtual int deinit(void) = 0;
    virtual int isValid(void) = 0;
};

#endif // BASE_GIMBAL_HPP
