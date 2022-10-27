#include "RoverController.hpp"
#include "Nesfr4.hpp"

#include "logging.hpp"

#ifdef __SPDLOG__
static auto console = spdlog::stdout_color_mt("Rover");
#endif

RoverController::RoverController( std::atomic_bool &system_on
                , struct RoverDesc &rover_desc
        )
    : system_on(system_on)
    , rover_desc(rover_desc)
{
    if (rover_desc.type.compare("NESFR4") == 0|| rover_desc.type.compare("Nesfr4") == 0) {

        LOG_INFO("Basee Rover type={}", rover_desc.type.c_str());
        LOG_INFO("            name={}", rover_desc.name.c_str());
        rover_ptr = std::make_shared<Nesfr4>(system_on);

    } else if (rover_desc.type.compare("NESFR7_ROS2") == 0|| rover_desc.type.compare("Nesfr7_ROS2") == 0){

    } else {
        LOG_ERR("Wrong configuration for VR Rover ({})", rover_desc.type.c_str());
        throw std::runtime_error("Wrong configuration for VR Rover");
    }
}

void RoverController::_ctrl_thread_func(void)
{
    rover_ptr->ctrl_thread_func();
}

int RoverController::initDevice(void)
{
    if (rover_ptr->init() < 0)
        return -1;
    ctrl_thread_ptr = std::make_shared<std::thread>(&RoverController::_ctrl_thread_func, this);
    initialized = {true};
    return 0;
}

int RoverController::deinitDevice(void)
{
    if (ctrl_thread_ptr)
        ctrl_thread_ptr->join();
    ctrl_thread_ptr = nullptr;
    if (rover_ptr->deinit() < 0)
        return -1;
    initialized = {false};
    return 0;
}

int RoverController::initSession(void)
{
    return 0;
}

int RoverController::deinitSession(void)
{
    return 0;
}
