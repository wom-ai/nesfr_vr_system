#ifndef GIMBAL_CONTROLLER_HPP
#define GIMBAL_CONTROLLER_HPP

#include "BaseGimbal.hpp"

#include <atomic>
#include <mutex>
#include <memory>
#include <thread>

struct GimbalDesc{
    std::string type;
    std::string name;
    unsigned int can_ch;
};

class GimbalController
{
protected:
    std::atomic_bool &system_on;

    struct GimbalDesc gimbal_desc;

    std::shared_ptr<BaseGimbal> gimbal_ptr = nullptr;

    std::shared_ptr<std::mutex> thread_mutex_ptr = nullptr;

    std::atomic_bool initialized = {false};

public:
    GimbalController( std::atomic_bool &system_on
                    , struct GimbalDesc &gimbal_desc
            );


    int initDevice(void);
    int deinitDevice(void);

    int initCtrl(void);
    int deinitCtrl(void);

    int initSession(void);
    int deinitSession(void);
};
#endif // GIMBAL_CONTROLLER_HPP


