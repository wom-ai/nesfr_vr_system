#include "NesfrVR.hpp"

#include "config.hpp"
#include "logging.hpp"
#include <json/json.h>
#include <cstdio>
#include <fstream>
#include <iostream>

#include <stdexcept>
#include <csignal>

#include <unistd.h>

#include "CtrlClient.hpp"

// filesystem
/*
 * references
 *  - https://stackoverflow.com/questions/48312460/c17-filesystem-is-not-a-namespace-name
 *  - https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 */
#if __GNUC__ <  8
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#else
#include <filesystem>
namespace filesystem = std::filesystem;
#endif

#ifdef __SPDLOG__
static auto console = spdlog::stdout_color_mt("Main");
#endif

static void print_build_info(void)
{
    printf(">>> %s::%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
    LOG_INFO("GCC __VERSION__={}", __VERSION__);
    LOG_INFO("CPP STANDARD __cplusplus={}", __cplusplus);
    LOG_INFO("Build Date={}", __DATE__);
    LOG_INFO("Git Branch=[{}]", __GIT_BRANCH__);
    LOG_INFO("    +-> commit {}", __GIT_COMMIT_HASH__);
    printf("<<< %s::%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
}

static int load_hw_config(Json::Value &root)
{
    const filesystem::path home_dir_path{getenv("HOME")};
    const filesystem::path cache_dir_path{CACHE_DIR_PATH};
    const filesystem::path config_file_path{home_dir_path/cache_dir_path/CONFIG_FILE_NAME};

    LOG_INFO("Trying to load {}", config_file_path.c_str());
    if (filesystem::exists(config_file_path))
    {
        // https://cplusplus.com/reference/istream/istream/tellg/
        char *buf = nullptr;
        std::ifstream is(config_file_path);
        if (!is.is_open()) {
            LOG_ERR("{} is already opened.");
            return -1;
        }

        is.seekg(0, is.end);
        int len = is.tellg();
        is.seekg(0, is.beg);

        buf = new char[len];
        is.read(buf, len);
        is.close();

        //std::cout.write(buf, len);

        const std::string rawJson = buf;
        const auto rawJsonLength = static_cast<int>(rawJson.length());
        JSONCPP_STRING err;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &root, &err)) {
            LOG_ERR("{}", config_file_path.c_str());
            return -1;
        }

        std::cout << root << std::endl;
        delete[] buf;
    }
    else
    {
        return -1;
    }

    return 0;
}

// Headset A
static const std::string headset_A_mac_addr = "2c:26:17:eb:ae:28";
// Headset B
static const std::string headset_B_mac_addr = "2c:26:17:e9:08:3e";

class InterruptException : public std::exception
{
public:
    InterruptException(int s) : S(s) {}
    int S;
};

class TerminationException : public std::exception
{
public:
    TerminationException(int s) : S(s) {}
    int S;
};

static void signal_int_handler(int s)
{
    throw InterruptException(s);
}

static void signal_term_handler(int s)
{
    throw TerminationException(s);
}

NesfrVR::NesfrVR(void)
{
}

int NesfrVR::_initVideoStreamer(void)
{
    struct CameraDesc camera_desc_left = {
        root["video_stream_device"]["stereo_camera"]["left"]["type"].asString(),
        {/*"Stereo Vision 1", "Stereo Vision 1: Stereo Vision ", "Video Capture 5",*/},
        root["video_stream_device"]["stereo_camera"]["width"].asUInt(),
        root["video_stream_device"]["stereo_camera"]["height"].asUInt(),
        root["video_stream_device"]["stereo_camera"]["framerate"].asUInt(),
    };
    {
        const Json::Value items = root["video_stream_device"]["stereo_camera"]["left"]["names"];
        for (unsigned int i = 0; i < items.size(); i++)
            camera_desc_left.names.push_back(items[i].asString());
    }

    struct CameraDesc camera_desc_right = {
        root["video_stream_device"]["stereo_camera"]["right"]["type"].asString(),
        {/*"Stereo Vision 2", "Stereo Vision 2: Stereo Vision ", "Video Capture 5",*/},
        root["video_stream_device"]["stereo_camera"]["width"].asUInt(),
        root["video_stream_device"]["stereo_camera"]["height"].asUInt(),
        root["video_stream_device"]["stereo_camera"]["framerate"].asUInt(),
    };
    {
        const Json::Value items = root["video_stream_device"]["stereo_camera"]["right"]["names"];
        for (unsigned int i = 0; i < items.size(); i++)
            camera_desc_right.names.push_back(items[i].asString());
    }
    struct CameraDesc camera_desc_main = {
        root["video_stream_device"]["main_camera"]["type"].asString(),
        {/*"USB Video", "USB Video: USB Video", "Video Capture 3", "Logitech StreamCam", "HD Pro Webcam C920"*/},
        root["video_stream_device"]["main_camera"]["width"].asUInt(),
        root["video_stream_device"]["main_camera"]["height"].asUInt(),
        root["video_stream_device"]["main_camera"]["framerate"].asUInt(),
    };
    {
        const Json::Value items = root["video_stream_device"]["main_camera"]["names"];
        for (unsigned int i = 0; i < items.size(); i++)
            camera_desc_main.names.push_back(items[i].asString());
    }
    struct AudioInDesc audioin_desc = {
        root["video_stream_device"]["audio-in"]["type"].asString(),
        root["video_stream_device"]["audio-in"]["name"].asString(),
        root["video_stream_device"]["audio-in"]["bitrate"].asUInt(),
    };
    struct AudioOutDesc audioout_desc = {
        root["video_stream_device"]["audio-out"]["type"].asString(),
        root["video_stream_device"]["audio-out"]["name"].asString(),
        root["video_stream_device"]["audio-out"]["bitrate"].asUInt(),
    };

    const Json::Value camera_offset = root["video_stream_device"]["stereo_camera"]["camera_offset"];
    const Json::Value camera_intrinsic0 = root["video_stream_device"]["stereo_camera"]["camera_intrinsic0"];
    const Json::Value camera_intrinsic1 = root["video_stream_device"]["stereo_camera"]["camera_intrinsic1"];
    struct StereoViewProperty property = {
        (float)root["video_stream_device"]["stereo_camera"]["width"].asUInt(),
        (float)root["video_stream_device"]["stereo_camera"]["height"].asUInt(),
        {camera_offset["left_x"].asFloat(), camera_offset["left_y"].asFloat()},
        {camera_offset["right_x"].asFloat(), camera_offset["right_y"].asFloat()},

        camera_intrinsic0["fx"].asFloat(),
        camera_intrinsic0["fy"].asFloat(),
        camera_intrinsic0["cx"].asFloat(),
        camera_intrinsic0["cy"].asFloat(),
        {
            camera_intrinsic0["k1"].asFloat(),
            camera_intrinsic0["k2"].asFloat(),
            camera_intrinsic0["k3"].asFloat(),
        },
        {
            camera_intrinsic0["p1"].asFloat(),
            camera_intrinsic0["p2"].asFloat(),
        },
        camera_intrinsic1["fx"].asFloat(),
        camera_intrinsic1["fy"].asFloat(),
        camera_intrinsic1["cx"].asFloat(),
        camera_intrinsic1["cy"].asFloat(),
        {
            camera_intrinsic1["k1"].asFloat(),
            camera_intrinsic1["k2"].asFloat(),
            camera_intrinsic1["k3"].asFloat(),
        },
        {
            camera_intrinsic1["p1"].asFloat(),
            camera_intrinsic1["p2"].asFloat(),
        },
    };

    bool stereo_flag = true;
    streamer_ptr = std::make_shared<VideoStreamer>(system_on, headset_ip, stereo_flag, camera_desc_left, camera_desc_right, property, camera_desc_main, audioin_desc, audioout_desc);

    if (streamer_ptr->initDevices() < 0)
    {
        perror ("gstreamer_ptr initialization failed.");
        return -1;
    }
    return 0;
}

int NesfrVR::_deinitVideoStreamer(void)
{
    int ret = streamer_ptr->deinitDevices();
    streamer_ptr = nullptr;
    return ret;
}

int NesfrVR::_initGimbalController(void)
{
    struct GimbalDesc gimbal_desc = {
        root["gimbal"]["type"].asString(),
        root["gimbal"]["name"].asString(),
        root["gimbal"]["can_ch"].asUInt(),
    };
    gimbal_controller_ptr = std::make_shared<GimbalController>(system_on, gimbal_desc);
    return gimbal_controller_ptr->initDevice();;
}

int NesfrVR::_deinitGimbalController(void)
{
    int ret = gimbal_controller_ptr->deinitDevice();
    gimbal_controller_ptr = nullptr;
    return ret;
}

int NesfrVR::_initRoverController(void)
{

    struct RoverDesc rover_desc = {
        root["base_rover"]["type"].asString(),
        root["base_rover"]["name"].asString(),
    };
    rover_controller_ptr = std::make_shared<RoverController>(system_on, rover_desc);
    return rover_controller_ptr->initDevice();
}

int NesfrVR::_deinitRoverController(void)
{
    int ret = rover_controller_ptr->deinitDevice();
    rover_controller_ptr = nullptr;
    return ret;
}

int NesfrVR::_playAudioGuide(const std::string filename, const double volume)
{
#ifdef _AUDIO_GUIDE_
    static const filesystem::path home_dir_path{getenv("HOME")};
    const filesystem::path data_dir_path{home_dir_path/DATA_DIR_PATH/filename.c_str()};
    if (audio_player_ptr && audio_player_ptr->playOggFile(data_dir_path.string(), volume))
        return -1;
#endif
    return 0;
}

int NesfrVR::_mainLoop(CtrlClient &conn)
{
    //streamer_ptr->warmUpStream();

    while (system_on) {
        if (conn.init()) {
            return -1;
        }
        if (conn.conn(headset_ip)) {
            LOG_WARN("[WARN] connectto() failed retry after 1 sec\n");

            if (_playAudioGuide("CouldntConnectTheVRHeadset.ogg") < 0)
                return -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            continue;
        }

        if (streamer_ptr->initGStreamer() < 0)
        {
            LOG_ERR("gstreamer initialization failed.");
            return -1;
        }

        HeadsetCtrlCmdMsg reg_msg = { conn.build_header((unsigned int)HeadsetCtrlCmd::REGISTER, 0), device_options, 0, 0,};
        if (conn.write_cmd(reg_msg) < 0) {
            fprintf(stderr, "[ERROR] write_id() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }
        {
            HeadsetCtrlCmdMsg msg = { conn.build_header((unsigned int)HeadsetCtrlCmd::PUT_STEREO_CAMERA_PROPERTY, sizeof(StereoViewProperty)), 0, 0, 0,};
            if (conn.write_cmd(msg) < 0) {
                fprintf(stderr, "[ERROR] write_cmd() failed, %s(%d)\n", strerror(errno), errno);
                return -1;
            }

            struct StereoViewProperty stereo_view_property = streamer_ptr->getStereoViewProperty();
            if (conn.write_data((const void*)&stereo_view_property, sizeof(StereoViewProperty)) < 0) {
                fprintf(stderr, "[ERROR] write_data(HeadsetCtrlCmd::PUT_STEREO_CAMERA_PROPERTY) failed, %s(%d)\n", strerror(errno), errno);
                return -1;
            }
        }
        if (rover_controller_ptr)
        {
            std::vector<struct DestinationDesc> destination_array;
            rover_controller_ptr->getDestinationArray(destination_array);
            int size = destination_array.size()*sizeof(struct DestinationDesc);
            HeadsetCtrlCmdMsg msg = { conn.build_header((unsigned int)HeadsetCtrlCmd::PUT_DESTINATION_ARRAY, size), 0, 0, 0,};
            if (conn.write_cmd(msg) < 0) {
                fprintf(stderr, "[ERROR] write_cmd() failed, %s(%d)\n", strerror(errno), errno);
                return -1;
            }

            if (conn.write_data((const void*)destination_array.data(), size) < 0) {
                fprintf(stderr, "[ERROR] write_data(HeadsetCtrlCmd::PUT_DESTINATION_ARRAY) failed, %s(%d)\n", strerror(errno), errno);
                return -1;
            }
        }

        if (conn.write_streamstate(stream_state) < 0) {
            fprintf(stderr, "[ERROR] write_streamstate() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        while (system_on) {
            struct RemoteCtrlCmdMsg msg;
            int ret = conn.readcmd(msg);
            if (ret < 0) {
                fprintf(stderr, "[ERROR] read failed, %s(%d)\n", strerror(errno), errno);
            } else if ( ret == 0) {
                LOG_ERR("connection closed\n");
                {
                    LOG_INFO(">>> STOP\n");
                    stream_state = 0;
                    streamer_ptr->stopStream();
                }
                conn.deinit();
                break;
            } else {
                switch (msg.cmd) {
                    case  RemoteCtrlCmd::PLAY:
                        {
                            LOG_INFO(">>> PLAY\n");
                            stream_state = 1;
                            if (streamer_ptr->playStream() < 0) {
                                LOG_ERR("streamer_ptr->playStream() failed");
                                return -1;
                            }
                            if (conn.write_streamstate(stream_state) < 0) {
                                fprintf(stderr, "[ERROR] writeid failed, %s(%d)\n", strerror(errno), errno);
                                return -1;
                            }
                            break;
                        }
                    case RemoteCtrlCmd::STOP:
                        {
                            LOG_INFO(">>> STOP\n");
                            stream_state = 0;
                            if (streamer_ptr->stopStream() < 0) {
                                LOG_ERR("streamer_ptr->stopStream() failed");
                                return -1;
                            }
                            if (conn.write_streamstate(stream_state) < 0) {
                                fprintf(stderr, "[ERROR] writeid failed, %s(%d)\n", strerror(errno), errno);
                                return -1;
                            }
                            break;
                        }
                    case RemoteCtrlCmd::NONE:
                    default:
                        break;
                }
            }
            usleep(100);
        }
        if (streamer_ptr->deinitGStreamer() < 0)
        {
            LOG_ERR("gstreamer deinitialization failed.");
            return -1;
        }
    }
    return 0;
}

int NesfrVR::_init(void)
{

#ifdef _AUDIO_GUIDE_
    audio_player_ptr = std::make_shared<AudioPlayer>(root["video_stream_device"]["audio-out"]["type"].asString(), root["video_stream_device"]["audio-out"]["name"].asString());
#endif

#ifdef _AUDIO_GUIDE_
    {
        const filesystem::path home_dir_path{getenv("HOME")};
        const filesystem::path data_dir_path{home_dir_path/DATA_DIR_PATH/"Intro.wav"};
        if (audio_player_ptr && audio_player_ptr->playWavFile(data_dir_path.string(), 0.5))
            return -1;
    }
#endif

    if (root.isMember("version") && root["version"].asUInt() == CONFIG_VERSION) {
        LOG_INFO("Version matched.");
    } else {
        LOG_ERR("HW configuraton file's version is not compatible with this application.");
        if (root.isMember("version")) {
            LOG_ERR("\tConfig File version={}, Application version={}.", root["version"].asUInt(), CONFIG_VERSION);
        } else {
            LOG_ERR("\tNo version info in the config file.");
        }

        return -1;
    }

    std::string ip_addr;
    if (NetUtils::getIPAddrbyHWAddr(interface_name, ip_addr, root["network"]["mac_addr"].asString()) < 0) {
        LOG_ERR("getIPAddrbyHWAddr() falied");
        return -1;
    }
    LOG_INFO("ip_addr = {}", ip_addr.c_str());

    if (NetUtils::findHeadsetIPAddrbyHWAddr(headset_ip, headset_A_mac_addr) < 0) {
        LOG_ERR("Couldn't find VR Headset");

        while(system_on) {
            if (!NetUtils::call_nmap(ip_addr.c_str())) {
                fprintf(stderr, "[ERROR] Couldn't call nmap.\n");
                return -1;
            }
            if (NetUtils::findHeadsetIPAddrbyHWAddr(headset_ip, headset_A_mac_addr) < 0) {
                LOG_ERR("Couldn't find VR Headset");
                if (_playAudioGuide("CouldntFindAnyVRHeadset.ogg") < 0)
                    return -1;

                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            break;
        }
    }
    printf(">> VR Headset ip: %s\n", headset_ip.c_str());

    // initialize
    if (root.isMember("video_stream_device")) {
        if (_initVideoStreamer()) {
            LOG_ERR("_initVideoStreamer() failed");
            _playAudioGuide("VideoStreamerInitFailed.ogg");
            return -1;
        }
        if (_playAudioGuide("VideoStreamerIsReady.ogg") < 0)
            return -1;
    } else {
        LOG_ERR("No Video Streame Device configuration.");
        return -1;
    }

    if (root.isMember("gimbal")) {
        LOG_INFO("Gimbal found");
        if (_initGimbalController()) {
            LOG_ERR("_initGimbalController() failed");
            _playAudioGuide("GimbalControllerInitFailed.ogg");
            return -1;
        }
        device_options |= DEVICE_OPTION_GIMBAL;

        if (_playAudioGuide("GimbalControllerIsReady.ogg") < 0)
            return -1;
    } else {
        LOG_WARN("No Gimbal configuration.");
    }

    if (root.isMember("base_rover")) {
        LOG_INFO("Base Rover found");
        if (_initRoverController()) {
            LOG_ERR("_initRoverController() failed");
            _playAudioGuide("RoverControllerInitFailed.ogg");
            return -1;
        }
        device_options |= DEVICE_OPTION_ROVER;

        if (_playAudioGuide("RoverIsReady.ogg") < 0)
            return -1;
    } else {
        LOG_WARN("No Base Rover configuration.");
    }

    if (_playAudioGuide("VrSystemIsReady.ogg") < 0)
        return -1;

    return 0;
}

int NesfrVR::_deinit(void)
{
    // finalize
    if (root.isMember("video_stream_device")) {
        LOG_INFO("Video Stream - deinit");
        if (_deinitVideoStreamer() < 0)
        {
            LOG_ERR("_deinitVideoStreamer() failed.");
            return -1;
        }
    }
    if (root.isMember("gimbal")) {
        LOG_INFO("Gimbal - deinit");
        if (_deinitGimbalController()) {
            LOG_ERR("_deinitGimbalController() failed");
            return -1;
        }
    }
    if (root.isMember("base_rover")) {
        LOG_INFO("Base Rover - deinit");
        if (_deinitRoverController()) {
            LOG_ERR("_deinitRoverController() failed");
            return -1;
        }
    }

    return 0;
}

int NesfrVR::run(void)
{
    print_build_info();

    system_on = {true};

    //set CmdHeader
    char hostname[16];
    //get host name
    memset(hostname, 0x0, sizeof(hostname));
    gethostname(hostname, sizeof(hostname));
    printf("=======================================================\n");
    printf("Device Name (=hostname): %s\n", hostname);
    printf("=======================================================\n");

    if (load_hw_config(root) < 0) {
        LOG_ERR("No HW configuration.");
        return -1;
    }

    if (_init() < 0)
        return -1;

    CtrlClient conn(hostname, interface_name);

    try {
        // main loop
        _mainLoop(conn);
        stop();
    } catch (InterruptException &e) {
        LOG_WARN("Terminated by Interrrupt Signal {}\n", e.what());
        stop();
    } catch (TerminationException &e) {
        LOG_WARN("Terminated by Termination Signal {}\n", e.what());
        stop();
    } catch (std::exception &e) {
        LOG_ERR("[ERROR]: %s\n", e.what());
        stop();
    }

    streamer_ptr->deinitGStreamer();
    conn.deinit();

    if (_deinit() < 0)
        return -1;

    if (_playAudioGuide("VrSystemIsShutDown.ogg") < 0)
        return -1;

    return 0;
}

int NesfrVR::stop(void)
{
    system_on = {false};
    return 0;
}

#ifdef NESFR_VR_MAIN_FUNC
void init_signal(void)
{
    printf(">>> %s::%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
#if 0
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
#else
    std::signal(SIGINT, signal_int_handler);
    std::signal(SIGHUP, signal_int_handler);
    std::signal(SIGTERM, signal_term_handler);
#endif
    printf("<<< %s::%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
}

int main(int argc, char *argv[])
{
    init_signal();

    NesfrVR nesfrvr;
    int ret = 0;
    try {
        ret = nesfrvr.run();
    } catch (InterruptException &e) {
        LOG_WARN("Terminated by Interrrupt Signal {}\n", e.what());
        nesfrvr.stop();
    } catch (TerminationException &e) {
        LOG_WARN("Terminated by Termination Signal {}\n", e.what());
        nesfrvr.stop();
    } catch (std::exception &e) {
        LOG_ERR("[ERROR]: %s\n", e.what());
        nesfrvr.stop();
    }

    LOG_INFO("System is shut down");

    return ret;
}
#endif // NESFR_VR_MAIN_FUNC
