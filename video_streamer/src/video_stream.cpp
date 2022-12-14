#include <gst/gst.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <map>
#include <csignal>

#include "video_stream.hpp"
#include "CtrlClient.hpp"


using dev_vec = std::vector<std::string>;
using dev_map = std::map<std::string, std::string>;

static bool is_stereo = true;
static int stereo_video_width = 0;
static int stereo_video_height = 0;

//static const int main_video_width = 1920;
//static const int main_video_height = 1080;
static const int main_video_width = 1280;
static const int main_video_height = 720;
//static const int main_video_width = 640;
//static const int main_video_height = 480;

static const std::vector<std::string> main_camera_card_strs = {"USB Video", "USB Video: USB Video", "Video Capture 3", "Logitech StreamCam", "HD Pro Webcam C920"};
static const std::vector<std::string> stereo_camera_left_strs = {"Stereo Vision 1", "Stereo Vision 1: Stereo Vision ", "Video Capture 5",};
static const std::vector<std::string> stereo_camera_right_strs = {"Stereo Vision 2", "Stereo Vision 2: Stereo Vision ", "Video Capture 5",};

// Headset A
static const std::string headset_A_mac_addr = "2c:26:17:eb:ae:28";
// Headset B
static const std::string headset_B_mac_addr = "2c:26:17:e9:08:3e";

static dev_map file_card_map;

static const char *prefixes[] = {
    "video",
//    "radio",
//    "vbi",
//    "swradio",
//    "v4l-subdev",
//    "v4l-touch",
//    "media",
    nullptr
};

//put the headset's ip here
std::string headset_ip = "192.168.0.XXX";

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
                is_stereo = false; // mono
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
            break;
        case 1:
            stereo_video_width = 1280;
            stereo_video_height = 720;
            break;
        case 2:
            stereo_video_width = 640;
            stereo_video_height = 480;
            break;
    }
    printf("=======================================================\n");
    printf(" dimensions: dims %d: %dx%d\n", dims, stereo_video_width, stereo_video_height);
    printf("=======================================================\n");

    printf("=======================================================\n");
    printf(" video mode: (%s)\n", is_stereo? "stereo": "mono" );
    printf("=======================================================\n");
    return 0;
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

/*
 * references
 *   - https://git.linuxtv.org/v4l-utils.git/tree/utils/v4l2-ctl/v4l2-ctl-common.cpp
 */
static bool is_v4l_dev(const char *name)
{
    for (unsigned i = 0; prefixes[i]; i++) {
        unsigned l = strlen(prefixes[i]);

        if (!memcmp(name, prefixes[i], l)) {
            if (isdigit(name[l]))
                return true;
        }
    }
    return false;
}

static int calc_node_val(const char *s)
{
    int n = 0;

    s = std::strrchr(s, '/') + 1;

    for (unsigned i = 0; prefixes[i]; i++) {
        unsigned l = strlen(prefixes[i]);

        if (!memcmp(s, prefixes[i], l)) {
            n = i << 8;
            n += atol(s + l);
            return n;
        }
    }
    return 0;
}

static bool sort_on_device_name(const std::string &s1, const std::string &s2)
{
    int n1 = calc_node_val(s1.c_str());
    int n2 = calc_node_val(s2.c_str());

    return n1 < n2;
}

static void init_dev_files(void)
{
    DIR *dp;
    struct dirent *ep;
    dev_vec files;
    dev_map links;
    struct v4l2_capability vcap;

    dp = opendir("/dev");
    if (dp == nullptr) {
        perror ("Couldn't open the directory");
        return;
    }

    while ((ep = readdir(dp)))
        if (is_v4l_dev(ep->d_name))
        {
            files.push_back(std::string("/dev/") + ep->d_name);
        }
    closedir(dp);

    /* Find device nodes which are links to other device nodes */
    for (auto iter = files.begin();
            iter != files.end(); ) {
        char link[64+1];
        int link_len;
        std::string target;

        link_len = readlink(iter->c_str(), link, 64);
        if (link_len < 0) {	/* Not a link or error */
            iter++;
            continue;
        }
        link[link_len] = '\0';

        /* Only remove from files list if target itself is in list */
        if (link[0] != '/')	/* Relative link */
            target = std::string("/dev/");
        target += link;
        if (find(files.begin(), files.end(), target) == files.end()) {
            iter++;
            continue;
        }

        /* Move the device node from files to links */
        if (links[target].empty())
            links[target] = *iter;
        else
            links[target] += ", " + *iter;
        iter = files.erase(iter);
    }

    std::sort(files.begin(), files.end(), sort_on_device_name);

    printf("-[v4l_devs]----------------------------------\n");
    for (const auto &file : files) {
        int fd = open(file.c_str(), O_RDWR);
        std::string bus_info;
        std::string card;

        if (fd < 0)
            continue;
        int err = ioctl(fd, VIDIOC_QUERYCAP, &vcap);
        if (err) {
        } else {
            bus_info = reinterpret_cast<const char *>(vcap.bus_info);
            card = reinterpret_cast<const char *>(vcap.card);
        }
        close(fd);

        if (err)
            continue;
        if (!(vcap.device_caps & V4L2_CAP_VIDEO_CAPTURE))
            continue;
        printf("  %s\n", file.c_str());
        file_card_map[file.c_str()] = card;
    }
    printf("--------------------------------------------\n");
}

/*
static bool find_dev_file_by_strs(const std::vector<string> &id_strs, string &dev_file)
{
    string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("find_dev_file_by_strs(%s)\n", temp.c_str());

    bool ret = false;
    for (const auto &file_card : file_card_map) {
        //printf("(%s)\n{%s)\n", id_str.c_str(), file_card.second.c_str());
        bool match = false;
        for (const auto id_str : id_strs)
            match |= (file_card.second.compare(id_str) == 0);
        if (match)
        {
            dev_file = file_card.first;
            printf("  (%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            break;
        }
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}
*/

static bool find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("find_dev_file_by_strs(%s)\n", temp.c_str());

    printf("\n-[v4l_devs]-------------------------------------\n");
    for (const auto &file_card : file_card_map)
        printf("  [%s]:%s\n", file_card.first.c_str(), file_card.second.c_str());
    printf("------------------------------------------------\n\n");

    bool ret = false;
    //for (const auto &file_card : file_card_map) {
    for (auto iter = file_card_map.begin();
            iter != file_card_map.end(); ) {

        auto file_card = *iter;

        //printf("(%s)\n{%s)\n", id_str.c_str(), file_card.second.c_str());
        bool match = false;
        for (const auto id_str : id_strs)
            match |= (file_card.second.compare(id_str) == 0);
        if (match)
        {
            dev_file = file_card.first;
            printf("  (%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            file_card_map.erase(iter);
            break;
        }
        else
            iter++;
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}

static bool find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file, std::string &found_id_str)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("find_dev_file_by_strs(%s)\n", temp.c_str());

    printf("\n-[v4l_devs]-------------------------------------\n");
    for (const auto &file_card : file_card_map)
        printf("  [%s]:%s\n", file_card.first.c_str(), file_card.second.c_str());
    printf("------------------------------------------------\n\n");

    bool ret = false;
    //for (const auto &file_card : file_card_map) {
    for (auto iter = file_card_map.begin();
            iter != file_card_map.end(); ) {

        auto file_card = *iter;

        //printf("(%s)\n{%s)\n", id_str.c_str(), file_card.second.c_str());
        bool match = false;
        for (const auto id_str : id_strs)
            match |= (file_card.second.compare(id_str) == 0);
        if (match)
        {
            dev_file = file_card.first;
            printf("  (%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            file_card_map.erase(iter);
            found_id_str = file_card.second.c_str();
            break;
        }
        else
            iter++;
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
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

bool configure_stereo_camera(const std::string &dev_file)
{
    int n;
    FILE *pin = nullptr;
    char buffer [128];
    char line[1024];
    memset(buffer, 0, sizeof(buffer));
    n = sprintf (buffer, "v4l2-ctl --device=%s -l", dev_file.c_str());
    printf(">> call [%s]\n", buffer);
    pin = popen(buffer,"r");
    if (!pin)
        return false;
    else {
        while (fgets(line, 1024, pin)) {
            printf(">> %s", line);
        }
        pclose(pin);
    }

    memset(buffer, 0, sizeof(buffer));
    n = sprintf (buffer, "v4l2-ctl --device=%s -c white_balance_temperature_auto=0", dev_file.c_str());
    printf(">> call [%s]\n", buffer);
    pin = popen(buffer,"r");
    if (!pin)
        return false;
    else {
        while (fgets(line, 1024, pin)) {
            printf(">> %s", line);
        }
        pclose(pin);
    }

    memset(buffer, 0, sizeof(buffer));
    n = sprintf (buffer, "v4l2-ctl --device=%s -C white_balance_temperature_auto", dev_file.c_str());
    printf(">> call [%s]\n", buffer);
    pin = popen(buffer,"r");
    if (!pin)
        return false;
    else {
        while (fgets(line, 1024, pin)) {
            printf(">> %s", line);
        }
        pclose(pin);
    }
    return true;
}

int init_gstreamer(   GstElement *&pipeline_stereo_left
                    , GstElement *&pipeline_stereo_right
                    , GstElement *&pipeline_main
                    , GstElement *&pipeline_audio
                    )
{
    std::string main_camera_dev_file;
    std::string stereo_camera_left_dev_file;
    std::string stereo_camera_right_dev_file;

    GError *error = NULL;
    GError *error1 = NULL;
    GError *error2 = NULL;
    GError *error_audio = NULL;

    gst_init (NULL, NULL);

    size_t size = 128;
    char buf[size];

    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=30/1 ", stereo_video_width, stereo_video_height);
    std::string stereo_video_conf_str = buf;

    // one eye of the stereo camera
    if (find_and_remove_dev_file_by_strs(stereo_camera_left_strs, stereo_camera_left_dev_file)) {
        printf(">> Stereo Camera (Left) (%s)\n", stereo_camera_left_dev_file.c_str());
        if (!configure_stereo_camera(stereo_camera_left_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Stereo Camera (Left)\n");
            return -1;
        }
        pipeline_stereo_left = gst_parse_launch
          (("v4l2src device=" + stereo_camera_left_dev_file + " ! image/jpeg, " + stereo_video_conf_str+ "  ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10000").data(), &error);
        //gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Stereo Camera (Left)\n");
    }

    // mono mode
    if (is_stereo)
    {
        // one eye of the stereo camera
        if (find_and_remove_dev_file_by_strs(stereo_camera_right_strs, stereo_camera_right_dev_file)) {
            printf(">> Stereo Camera (Right) (%s)\n", stereo_camera_right_dev_file.c_str());
            if (!configure_stereo_camera(stereo_camera_right_dev_file))
            {
                fprintf(stderr, "[ERROR] Couldn't configure Stereo Camera (Right)\n");
                return -1;
            }
            pipeline_stereo_right = gst_parse_launch
              (("v4l2src device=" + stereo_camera_right_dev_file + " ! image/jpeg, " + stereo_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10001").data(), &error1);
            //gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
        } else {
            fprintf(stderr, "[ERROR] Couldn't open Stereo Camera (Right)\n");
        }
    }

    //main camera
#if 0
    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=60/1 ", main_video_width, main_video_height);
    std::string main_video_conf_str = buf;

    if (find_and_remove_dev_file_by_strs(main_camera_card_strs, main_camera_dev_file)) {
        printf(">> Main Camera (%s)\n", main_camera_dev_file.c_str());
        pipeline_main = gst_parse_launch
          (("v4l2src device=" + main_camera_dev_file + " ! image/jpeg, " + main_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error2);
        //gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Main Camera\n");
    }

    //audio in
//    pipeline_audio = gst_parse_launch
//        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! alawenc ! rtpgstpay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);

//    pipeline_audio = gst_parse_launch
//        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! alawenc ! rtppcmapay !application/x-rtp, payload=8, clock-rate=8000 ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);

    pipeline_audio = gst_parse_launch
        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! rtpL16pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);

//    pipeline_audio = gst_parse_launch
//        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! rtpL8pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
#else
    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=30/1 ", main_video_width, main_video_height);
    std::string main_video_conf_str = buf;

    if (find_and_remove_dev_file_by_strs(main_camera_card_strs, main_camera_dev_file)) {
        printf(">> Main Camera (%s)\n", main_camera_dev_file.c_str());
        pipeline_main = gst_parse_launch
          (("v4l2src device=" + main_camera_dev_file + " ! image/jpeg, " + main_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error2);
        //gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Main Camera\n");
    }

    pipeline_audio = gst_parse_launch
        (("pulsesrc device=alsa_input.usb-046d_HD_Pro_Webcam_C920_9D5E927F-02.analog-stereo ! rtpL16pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
#endif
    //gst_element_set_state(pipeline_audio, GST_STATE_PLAYING);

    if (error != NULL) {
        g_error("Couldn't launch the pipeline_stereo_left");
        return -1;
    }

    if (error1 != NULL) {
        g_error("Couldn't launch the pipeline_stereo_right");
        return -1;
    }

    if (error2 != NULL) {
        g_error("Couldn't launch the pipeline_main");
        return -1;
    }

    if (error_audio != NULL) {
        g_error("Couldn't launch the pipeline_audio");
        return -1;
    }

    return 0;
}

int deinit_gstreamer( GstElement *&pipeline_stereo_left
                    , GstElement *&pipeline_stereo_right
                    , GstElement *&pipeline_main
                    , GstElement *&pipeline_audio
                    )
{
    // gst_object_unref (bus);
    if (pipeline_stereo_left) {
        gst_element_set_state (pipeline_stereo_left, GST_STATE_NULL);
        gst_object_unref (pipeline_stereo_left);
    }

    // gst_object_unref (bus1);
    if (pipeline_stereo_right) {
        gst_element_set_state (pipeline_stereo_right, GST_STATE_NULL);
        gst_object_unref (pipeline_stereo_right);
    }

    if (pipeline_main) {
        gst_element_set_state (pipeline_main, GST_STATE_NULL);
        gst_object_unref (pipeline_main);
    }

    if (pipeline_audio) {
        gst_element_set_state (pipeline_audio, GST_STATE_NULL);
        gst_object_unref (pipeline_audio);
    }

    return 0;
}

int main_loop(    CtrlClient &conn
                , GstElement *&pipeline_stereo_left
                , GstElement *&pipeline_stereo_right
                , GstElement *&pipeline_main
                , GstElement *&pipeline_audio
                )
{
    int stream_state = 0;

    while (true) {
        if (conn.init()) {
            return -1;
        }
        if (conn.conn(headset_ip)) {
            fprintf(stderr, "[WARN] connectto() failed retry after 1 sec\n");
            sleep(1);
            continue;
        }

        if (conn.write_id() < 0) {
            fprintf(stderr, "[ERROR] write_id() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        if (conn.write_streamstate(stream_state) < 0) {
            fprintf(stderr, "[ERROR] write_streamstate() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        while (true) {
            struct RemoteCtrlCmdMsg msg;
            int ret = conn.readcmd(msg);
            if (ret < 0) {
                fprintf(stderr, "[ERROR] read failed, %s(%d)\n", strerror(errno), errno);
            } else if ( ret == 0) {
                fprintf(stderr, "connection closed\n");
                {
                    printf(">>> STOP\n");
                    stream_state = 0;
                    if (pipeline_stereo_left) gst_element_set_state(pipeline_stereo_left, GST_STATE_PAUSED);
                    if (pipeline_stereo_right) gst_element_set_state(pipeline_stereo_right, GST_STATE_PAUSED);
                    if (pipeline_main) gst_element_set_state(pipeline_main, GST_STATE_PAUSED);
                    if (pipeline_audio) gst_element_set_state(pipeline_audio, GST_STATE_PAUSED);
                }
                conn.deinit();
                break;
            } else {
                switch (msg.cmd) {
                    case  RemoteCtrlCmd::PLAY:
                        {
                            printf(">>> PLAY\n");
                            stream_state = 1;
                            if (pipeline_stereo_left) gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
                            if (pipeline_stereo_right) gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
                            if (pipeline_main) gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
                            if (pipeline_audio) gst_element_set_state(pipeline_audio, GST_STATE_PLAYING);
                            if (conn.write_streamstate(stream_state) < 0) {
                                fprintf(stderr, "[ERROR] writeid failed, %s(%d)\n", strerror(errno), errno);
                                return -1;
                            }
                            break;
                        }
                    case RemoteCtrlCmd::STOP:
                        {
                            printf(">>> STOP\n");
                            stream_state = 0;
                            if (pipeline_stereo_left) gst_element_set_state(pipeline_stereo_left, GST_STATE_PAUSED);
                            if (pipeline_stereo_right) gst_element_set_state(pipeline_stereo_right, GST_STATE_PAUSED);
                            if (pipeline_main) gst_element_set_state(pipeline_main, GST_STATE_PAUSED);
                            if (pipeline_audio) gst_element_set_state(pipeline_audio, GST_STATE_PAUSED);
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
    }
    return 0;
}

int main (int argc, char *argv[])
{
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

    init_dev_files();

    GstElement *pipeline_stereo_left = nullptr;
    GstElement *pipeline_stereo_right = nullptr;
    GstElement *pipeline_main = nullptr;
    GstElement *pipeline_audio = nullptr;


    if (init_gstreamer( pipeline_stereo_left,
                        pipeline_stereo_right,
                        pipeline_main,
                        pipeline_audio) < 0)
    {
        perror ("gstreamer initialization failed.");
        return -1;
    }

    // GstBus *bus = gst_element_get_bus (pipeline_stereo_left);
    // GstBus *bus1 = gst_element_get_bus (pipeline_stereo_right);

    // gst_bus_pop_filtered (bus, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    // gst_bus_pop_filtered (bus1, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    CtrlClient conn(hostname);

    try {
        main_loop(  conn
                    , pipeline_stereo_left
                    , pipeline_stereo_right
                    , pipeline_main
                    , pipeline_audio);

    } catch (InterruptException &e) {
        fprintf(stderr, "Terminated by Interrrupt %s\n", e.what());
    } catch (std::exception &e) {
        fprintf(stderr, "[ERROR]: %s\n", e.what());
    }

    conn.deinit();
    printf("End process...\n");

    if (deinit_gstreamer(   pipeline_stereo_left,
                            pipeline_stereo_right,
                            pipeline_main,
                            pipeline_audio) < 0)
    {
        perror ("gstreamer deinitialization failed.");
        return -1;
    }
    printf("End properly\n");

    return 0;
}
