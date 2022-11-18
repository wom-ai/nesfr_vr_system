#ifndef NESFR_VR_HPP
#define NESFR_VR_HPP

#include <string>
#include <json/json.h>
#include <memory>

#include "VideoStreamer.hpp"
#include "GimbalController.hpp"
#include "RoverController.hpp"
#include "Utils.hpp"

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
    std::shared_ptr<GimbalController>   gimbal_controller_ptr = nullptr;
    std::shared_ptr<RoverController>    rover_controller_ptr = nullptr;

    std::shared_ptr<AudioPlayer>        audio_player_ptr = nullptr;
    int device_options = 0x0;
    int stream_state = 0;

    std::string ip_addr;
    std::string interface_name;
public:
    NesfrVR(void);
    int run(void);
    int stop(void);

private:
    int _init(void);
    int _deinit(void);

    int _initVideoStreamer(void);
    int _deinitVideoStreamer(void);

    int _initGimbalController(void);
    int _deinitGimbalController(void);

    int _initRoverController(void);
    int _deinitRoverController(void);

    int _playAudioGuide(const std::string filename, const double volume=1.0);

    int _mainLoop(CtrlClient &conn);
};
#endif // NESFR_VR_HPP
