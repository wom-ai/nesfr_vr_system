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
#include "rs2_vr_ctrl.hpp"
#include "PIDCtrl.hpp"
#include "VRState.h"
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
auto console = spdlog::stdout_color_mt("Main");
#endif

static void build_info(void)
{
    LOG_INFO("GCC __VERSION__={}", __VERSION__);
    LOG_INFO("CPP STANDARD __cplusplus={}", __cplusplus);
    LOG_INFO("Build Date={}", __DATE__);
    LOG_INFO("Git Branch=[{}]", __GIT_BRANCH__);
    LOG_INFO("    +-> commit {}", __GIT_COMMIT_HASH__);
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

/*
 * reference
 *   - https://www.cplusplus.com/reference/cstring/strtok/
 */
static bool find_headset_ip_by_MACAddr(const std::string &mac_addr, std::string &ip)
{
    bool ret = false;
    FILE *pin = nullptr;
    pin = popen("arp -n","r");
    if (!pin)
        return false;

    std::vector<std::string> lines;
    while (!feof(pin)) {
        char *line = nullptr;
        size_t len = 0;
        ssize_t read = getline(&line, &len, pin);
        if (read)
            lines.push_back(line);
    }
    pclose(pin);

    // parse lines
    for (const auto line : lines) {
        char *pch;
        std::vector<std::string> tokens;
        pch = strtok ((char *)line.c_str()," ,-\n");
        while (pch != NULL)
        {
            printf ("(%s) ",pch);
            tokens.push_back(pch);
            pch = strtok (NULL, " ,-\n");
        }

//        if (tokens.size() > 0)
//            printf("%s\n", tokens[0].c_str());
//
        if (tokens.size() == 5 && tokens[2].compare(headset_A_mac_addr) == 0) {
            ip = tokens[0];
            printf("- found VR Headset:%s", ip.c_str());
            ret = true;
            break;
        }
        printf("\n");
    }

    return ret;
}

class InterruptException : public std::exception
{
public:
    InterruptException(int s) : S(s) {}
    int S;
};

static void signal_handler(int s)
{
    throw InterruptException(s);
}

void init_signal(void)
{
#if 0
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
#else
    std::signal(SIGINT, signal_handler);
#endif
}
static bool call_nmap(void)
{
    FILE *pin = nullptr;
    pin = popen("nmap -sn 192.168.0.0/24","r");
    if (!pin)
        return false;
    else {
        while (!feof(pin)) {
            char *line = nullptr;
            size_t len = 0;
            ssize_t read = getline(&line, &len, pin);
            if (read)
                printf("%s", line);
        }
        pclose(pin);
    }
    return true;
}
NesfrVR::NesfrVR(void)
{
}

int NesfrVR::initVideoStream(void)
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
    streamer_ptr = new VideoStreamer(system_on, headset_ip, stereo_flag, camera_desc_left, camera_desc_right, property, camera_desc_main, audioin_desc, audioout_desc);

    if (streamer_ptr->initDevices() < 0)
    {
        perror ("gstreamer_ptr initialization failed.");
        return -1;
    }
    return 0;
}

int NesfrVR::run(void)
{
    build_info();

    init_signal();

    //set CmdHeader
    char hostname[16];
    //get host name
    memset(hostname, 0x0, sizeof(hostname));
    gethostname(hostname, sizeof(hostname));
    printf("=======================================================\n");
    printf("Device Name (=hostname): %s\n", hostname);
    printf("=======================================================\n");

    CtrlClient conn(hostname);
    const uint8_t can_ch_num = 0;
    CANComm can_comm(can_ch_num);
    RS2Ctrl rs2_ctrl(can_comm);

    if (!find_headset_ip_by_MACAddr(headset_A_mac_addr, headset_ip)) {
        fprintf(stderr, "[ERROR] Couldn't find VR Headset.\n");

        if (!call_nmap()) {
            fprintf(stderr, "[ERROR] Couldn't call nmap.\n");
            return -1;
        }
        if (!find_headset_ip_by_MACAddr(headset_A_mac_addr, headset_ip)) {
            fprintf(stderr, "[ERROR] Couldn't find VR Headset.\n");
            return -1;
        }
    }
    printf(">> VR Headset ip: %s\n", headset_ip.c_str());

    if (load_hw_config(root) < 0) {
        LOG_ERR("No HW configuration.");
        return -1;
    }

    if (root.isMember("version") && root["version"].asUInt() == CONFIG_VERSION) {
        LOG_INFO("Version matched.");
    } else {
        LOG_ERR("HW configuraton file's version is not compatible with this application.");
        if (root.isMember("version")) {
            LOG_ERR("\tFile version={}, Application version={}.", root["version"].asUInt(), CONFIG_VERSION);
        } else {
            LOG_ERR("\tNo version info in the config file.");
        }

        return -1;
    }

    // initialize
    if (root.isMember("gimbal")) {
        LOG_INFO("Gimbal found");
        rs2_vr_ctrl_ptr = new RS2VRCtrl(system_on, rs2_ctrl);
        rs2_vr_ctrl_ptr->init();
    } else {
        LOG_WARN("No Gimbal configuration.");
    }

    if (root.isMember("base_rover")) {
        LOG_INFO("Base Rover found");
    } else {
        LOG_WARN("No Base Rover configuration.");
    }

    if (root.isMember("video_stream_device")) {
        initVideoStream();
    } else {
        LOG_ERR("No Video Streame Device configuration.");
        return -1;
    }

    // main loop
    try {
        streamer_ptr->run(conn);

    } catch (InterruptException &e) {
        fprintf(stderr, "Terminated by Interrrupt %s\n", e.what());
    } catch (std::exception &e) {
        fprintf(stderr, "[ERROR]: %s\n", e.what());
    }

    // finalize
    if (root.isMember("gimbal")) {
        LOG_INFO("Gimbal - deinit");
    }

    if (root.isMember("base_rover")) {
        LOG_INFO("Base Rover - deinit");
    }
    if (root.isMember("video_stream_device")) {
        LOG_INFO("Video Stream - deinit");
        if (streamer_ptr->deinitDevices() < 0)
        {
            perror ("gstreamer_ptr deinitialization failed.");
            return -1;
        }
    }

    conn.deinit();
    return 0;
}

#ifdef NESFR_VR_MAIN_FUNC
int main(int argc, char *argv[])
{
    NesfrVR nesfrvr;
    return nesfrvr.run();
}
#endif // NESFR_VR_MAIN_FUNC
