#include <string>
#include <json/json.h>
#include <memory>

#include "VideoStreamer.hpp"
#include "rs2_vr_ctrl.hpp"
#include "RoverController.hpp"

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
