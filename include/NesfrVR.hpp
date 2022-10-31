#ifndef NESFR_VR_HPP
#define NESFR_VR_HPP

#include <string>
#include <json/json.h>
#include <memory>

#include "VideoStreamer.hpp"
#include "rs2_vr_ctrl.hpp"
#include "RoverController.hpp"

#define DEVICE_OPTION_VIDEO     0x0001
#define DEVICE_OPTION_GIMBAL    0x0002
#define DEVICE_OPTION_ROVER     0x0004

class NesfrVR
{
protected:
    std::atomic_bool system_on = {true};

    //put the headset's ip here
    std::string headset_ip = "192.168.0.XXX";

    Json::Value root;
    std::shared_ptr<VideoStreamer>      streamer_ptr = nullptr;
    std::shared_ptr<RS2VRCtrl>          rs2_vr_ctrl_ptr = nullptr;
    std::shared_ptr<RoverController>    rover_controller_ptr = nullptr;

    int device_options = 0x0;
    int stream_state = 0;
public:
    NesfrVR(void);
    int _initVideoStream(void);
    int _initGimbalCtrl(void);
    int _initRoverController(void);
    int _deinitRoverController(void);
    int run(void);

    int _mainLoop(CtrlClient &conn);
};
#endif // NESFR_VR_HPP
