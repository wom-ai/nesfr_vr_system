#include "GimbalController.hpp"
#include "RS2.hpp"

#include "logging.hpp"

#ifdef __SPDLOG__
static auto console = spdlog::stdout_color_mt("Gimbal");
#endif

GimbalController::GimbalController( std::atomic_bool &system_on
                , struct GimbalDesc &gimbal_desc
        )
    : system_on(system_on)
    , gimbal_desc(gimbal_desc)
{
    if (gimbal_desc.type.compare("RS2") == 0|| gimbal_desc.type.compare("rs2") == 0) {

        LOG_INFO("Base Gimbal type={}", gimbal_desc.type.c_str());
        LOG_INFO("            name={}", gimbal_desc.name.c_str());
        gimbal_ptr = std::make_shared<RS2>(system_on, gimbal_desc.can_ch);
    } else {
        LOG_ERR("Wrong configuration for VR Gimbal ({})", gimbal_desc.type.c_str());
        throw std::runtime_error("Wrong configuration for VR Gimbal");
    }
}

int GimbalController::initDevice(void)
{
    if (gimbal_ptr->init() < 0)
        return -1;
    initialized = {true};
    return 0;
}

int GimbalController::deinitDevice(void)
{
    if (gimbal_ptr->deinit() < 0)
        return -1;
    initialized = {false};
    return 0;
}

int GimbalController::initSession(void)
{
    return 0;
}

int GimbalController::deinitSession(void)
{
    return 0;
}
