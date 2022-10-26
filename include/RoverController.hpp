#include "BaseRover.hpp"

#include <atomic>
#include <mutex>
#include <memory>
#include <thread>

struct RoverDesc{
    std::string type;
    std::string name;
};

class RoverController
{
protected:
    std::atomic_bool &system_on;

    struct RoverDesc rover_desc;

    std::shared_ptr<BaseRover> rover_ptr = nullptr;

    std::shared_ptr<std::mutex> thread_mutex_ptr = nullptr;

    std::shared_ptr<std::thread> ctrl_thread_ptr  = nullptr;

    std::atomic_bool initialized = {false};

    void _ctrl_thread_func(void);

public:
    RoverController( std::atomic_bool &system_on
                    , struct RoverDesc &rover_desc
            );


    int initDevice(void);
    int deinitDevice(void);

    int initCtrl(void);
    int deinitCtrl(void);

    int initSession(void);
    int deinitSession(void);
};
