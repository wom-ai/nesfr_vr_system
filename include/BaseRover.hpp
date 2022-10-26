#ifndef BASE_ROVER_HPP
#define BASE_ROVER_HPP

#include <string>

class BaseRover
{
protected:
    std::string name;

public:
    virtual int init(void) = 0;
    virtual int deinit(void) = 0;
    virtual int isValid(void) = 0;
    virtual void ctrl_thread_func (void) = 0;
};

#endif // BASE_ROVER_HPP
