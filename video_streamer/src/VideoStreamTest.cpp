#include "VideoStreamer.hpp"
#include <ctype.h>
#include <unistd.h>

#include <stdexcept>
#include <cstdio>
#include <string>
#include <cstring>
#include <csignal>


// Headset A
static const std::string headset_A_mac_addr = "2c:26:17:eb:ae:28";
// Headset B
static const std::string headset_B_mac_addr = "2c:26:17:e9:08:3e";

static bool stereo_flag = true;
static unsigned int stereo_video_width = 0;
static unsigned int stereo_video_height = 0;

//static const int main_video_width = 1920;
//static const int main_video_height = 1080;
static const unsigned int main_video_width = 1280;
static const unsigned int main_video_height = 720;
//static const int main_video_width = 640;
//static const int main_video_height = 480;

static struct StereoViewProperty stereo_view_property = {};
static struct StereoViewProperty property1920x1080 = {
    1920.0f,
    1080.0f,
    {-10.0f, 0.0f},
    {10.0f, 0.0f},

    649.98291315,
    649.5200055,
    948.25829987,
    562.09198792,
    {
        -0.21252438,
        0.03947589,
        -0.00312653
    },
    {
        0.00009263,
        0.0000544,
    },
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    {
        0.0f,
        0.0f,
        0.0f,
    },
    {
        0.0f,
        0.0f,
    },
};

static struct StereoViewProperty property1280x720 = {
    1280.0f,
    720.0f,
    {-10.0f, 0.0f},
    {10.0f, 0.0f},

    433.3219421,
    433.013337,
    632.172199913,
    374.727991947,
    {
        -0.21252438,
        0.03947589,
        -0.00312653
    },
    {
        0.00009263,
        0.0000544,
    },
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    {
        0.0f,
        0.0f,
        0.0f,
    },
    {
        0.0f,
        0.0f,
    },
};

static struct StereoViewProperty property640x480 = {
    640.0f,
    480.0f,
    {-10.0f, 0.0f},
    {10.0f, 0.0f},

    288.881294733,
    288.675558,
    421.448133276,
    249.818661298,
    {
        -0.21252438,
        0.03947589,
        -0.00312653
    },
    {
        0.00009263,
        0.0000544,
    },
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    {
        0.0f,
        0.0f,
        0.0f,
    },
    {
        0.0f,
        0.0f,
    },
};

/*
 * references
 *   - https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
 */
bool init_options(int argc, char *argv[])
{
    printf("=======================================================\n");
    printf("dims 0: 1920x1080\n");
    printf("dims 1: 1280x720 (default)\n");
    printf("dims 2:  640x480\n");
    printf("=======================================================\n");
    int c;
    int dims = 1;
    while ((c = getopt (argc, argv, "md:")) != -1)
        switch (c)
        {
            case 'a':
                printf("A\n");
                break;
            case 'b':
                printf("B\n");
                break;
            case 'm':
                stereo_flag = false; // mono
                break;
            case 'd':
                printf("dims: %s\n", optarg);

                try
                {
                    dims = std::stoi(optarg);
                }
                catch(std::invalid_argument const& ex)
                {
                    fprintf(stderr, "std::invalid_argument::what(): %s\n", ex.what());
                    return -1;
                }
                catch(std::out_of_range const& ex)
                {
                    fprintf(stderr, "std::out_of_range::what(): %s\n", ex.what());
                    return -1;
                }
                break;
            case '?':
                if (optopt == 'd')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                return -1;
            default:
                abort ();
        }

    switch (dims)
    {
        case 0:
            stereo_video_width = 1920;
            stereo_video_height = 1080;
            stereo_view_property = property1920x1080;
            break;
        case 1:
            stereo_video_width = 1280;
            stereo_video_height = 720;
            stereo_view_property = property1280x720;
            break;
        case 2:
            stereo_video_width = 640;
            stereo_video_height = 480;
            stereo_view_property = property640x480;
            break;
    }
    printf("=======================================================\n");
    printf(" dimensions: dims %d: %dx%d\n", dims, stereo_video_width, stereo_video_height);
    printf("=======================================================\n");

    printf("=======================================================\n");
    printf(" video mode: (%s)\n", stereo_flag? "stereo": "mono" );
    printf("=======================================================\n");
    return 0;
}

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
            printf("%s", line);
        }
        pclose(pin);
    }
    return true;
}

bool set_stereo_camera_left(const std::string &dev_file)
{
    int n;
    FILE *pin = nullptr;
    char buffer [128];
    memset(buffer, 0, sizeof(buffer));
    n = sprintf (buffer, "v4l2-ctl --device=%s -c white_balance_temperature_auto=1", dev_file.c_str());
    printf(">> call [%s]\n", buffer);
    pin = popen("v4l2-ctl --device=/dev/video1 -l","rw");
    if (!pin)
        return false;
    else {
        while (!feof(pin)) {
            char *line = nullptr;
            size_t len = 0;
            ssize_t read = getline(&line, &len, pin);
            printf("%s", line);
        }
        pclose(pin);
    }
    return true;
}

class InterruptException : public std::exception
{
public:
    InterruptException(int s) : S(s) {}
    int S;
};


void signal_handler(int s)
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

int main (int argc, char *argv[])
{
    //put the headset's ip here
    std::string headset_ip = "192.168.0.XXX";

    if(init_options(argc, argv))
        return -1;

    init_signal();

    //set CmdHeader
    char hostname[16];
    //get host name
    memset(hostname, 0x0, sizeof(hostname));
    gethostname(hostname, sizeof(hostname));
    printf("=======================================================\n");
    printf("Device Name (=hostname): %s\n", hostname);
    printf("=======================================================\n");


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

    std::atomic_bool system_on(true);

    struct CameraDesc camera_desc_left = {
        "v4l2",
        {"Stereo Vision 1", "Stereo Vision 1: Stereo Vision ", "Video Capture 5",},
        stereo_video_width,
        stereo_video_height,
        30 /*framerate*/};
    struct CameraDesc camera_desc_right = {
        "v4l2",
        {"Stereo Vision 2", "Stereo Vision 2: Stereo Vision ", "Video Capture 5",},
        stereo_video_width,
        stereo_video_height,
        30 /*framerate*/};
    struct CameraDesc camera_desc_main = {
        "v4l2",
        {"USB Video", "USB Video: USB Video", "Video Capture 3", "Logitech StreamCam", "HD Pro Webcam C920"},
        main_video_width,
        main_video_height,
        30 /*framerate*/};
    struct AudioInDesc audioin_desc = {
        "pulsesrc",
        "alsa_input.usb-Creative_Technology_Ltd_Sound_BlasterX_G1_00141088-00.analog-stereo",
        //"alsa_input.usb-046d_HD_Pro_Webcam_C920_9D5E927F-02.analog-stereo",
        //"alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo",
        0 /*bitrate*/};
    struct AudioOutDesc audioout_desc = {
        "pulsesink",
        "",
        0 /*bitrate*/};

    VideoStreamer streamer(system_on, headset_ip, stereo_flag, camera_desc_left, camera_desc_right, stereo_view_property, camera_desc_main, audioin_desc, audioout_desc);

    if (streamer.initDevices() < 0)
    {
        perror ("gstreamer initialization failed.");
        return -1;
    }
    CtrlClient conn(hostname);

    try {
        streamer.run(conn);

    } catch (InterruptException &e) {
        fprintf(stderr, "Terminated by Interrrupt %s\n", e.what());
    } catch (std::exception &e) {
        fprintf(stderr, "[ERROR]: %s\n", e.what());
    }

    conn.deinit();
    printf("End process...\n");

    if (streamer.deinitDevices() < 0)
    {
        perror ("gstreamer deinitialization failed.");
        return -1;
    }
    printf("End properly\n");

    return 0;
}
