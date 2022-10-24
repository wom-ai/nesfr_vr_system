#include <string>
#include <json/json.h>

#include "VideoStreamer.hpp"
#include "rs2_vr_ctrl.hpp"

class NesfrVR
{
protected:
    std::atomic_bool system_on = {true};

    //put the headset's ip here
    std::string headset_ip = "192.168.0.XXX";

    Json::Value root;
    VideoStreamer *streamer_ptr = nullptr;
    RS2VRCtrl *rs2_vr_ctrl_ptr = nullptr;
public:
    NesfrVR(void);
    int initVideoStream(void);
    int run(void);
};
