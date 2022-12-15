#ifndef ROVER_CONTROLLER_HPP
#define ROVER_CONTROLLER_HPP

#include "BaseRover.hpp"

#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>

struct RoverDesc{
    std::string type;
    std::string name;
};

struct DestinationDesc{
    char name[64];
    char desc[64];
    double x;
    double y;
    double z;
    double w;
    char reserved[32];
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

    std::vector<struct DestinationDesc> destination_array = {
        {{'d','u','m','m','y','0','0'}, {'t','h','i','s','i','s',' ','d','u','m','m','y'}, 0.0, 0.0, 0.0, 0.0, {0,}},
        {{'d','u','m','m','y','0','1'}, {'t','h','i','s','i','s',' ','d','u','m','m','y'}, 0.0, 0.0, 0.0, 0.0, {0,}},
        {{'d','u','m','m','y','0','2'}, {'t','h','i','s','i','s',' ','d','u','m','m','y'}, 0.0, 0.0, 0.0, 0.0, {0,}},
        {{'d','u','m','m','y','0','3'}, {'t','h','i','s','i','s',' ','d','u','m','m','y'}, 0.0, 0.0, 0.0, 0.0, {0,}},
    };

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

    void getDestinationArray(std::vector<struct DestinationDesc> &destination_array) const;
};
#endif // ROVER_CONTROLLER_HPP


