#include "VideoStreamer.hpp"
#include "CtrlClient.hpp"
#include "V4L2StereoCamera.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <cstring>

/*
 * references
 *   - https://git.linuxtv.org/v4l-utils.git/tree/utils/v4l2-ctl/v4l2-ctl-common.cpp
 */
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

VideoStreamer::VideoStreamer(   std::atomic_bool &system_on
                                , const std::string headset_ip
                                , const int stereo_flag
                                , const int stereo_video_width
                                , const int stereo_video_height
                                , const int main_video_width
                                , const int main_video_height)
    : system_on(system_on)
    , headset_ip(headset_ip)
    , stereo_flag(stereo_flag)
    , stereo_video_width(stereo_video_width)
    , stereo_video_height(stereo_video_height)
    , main_video_width(main_video_width)
    , main_video_height(main_video_height)
{
    stereo_camera_ptr = std::make_shared<V4L2StereoCamera>();
}

VideoStreamer::~VideoStreamer(void)
{
}

int VideoStreamer::initGStreamer(void)
{
    std::string main_camera_dev_file;
    std::string stereo_camera_left_dev_file;
    std::string stereo_camera_right_dev_file;

    GError *error = NULL;
    GError *error1 = NULL;
    GError *error2 = NULL;
    GError *error_audio = NULL;

    init_dev_files();
    gst_init (NULL, NULL);

    size_t size = 128;
    char buf[size];

    if (stereo_camera_ptr->init(stereo_video_width, stereo_video_height, !stereo_flag) < 0) {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=30/1 ", stereo_video_width, stereo_video_height);
    std::string stereo_video_conf_str = buf;

    // one eye of the stereo camera
    if (stereo_camera_ptr->getGStreamVideoSourceLeftStr(stereo_camera_left_dev_file) == 0) {
        printf(">> Stereo Camera (Left) (%s)\n", stereo_camera_left_dev_file.c_str());
        pipeline_stereo_left = gst_parse_launch
          (("v4l2src device=" + stereo_camera_left_dev_file + " ! image/jpeg, " + stereo_video_conf_str+ "  ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10000").data(), &error);
        //gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
    } else {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // mono mode
    if (stereo_flag)
    {
        // one eye of the stereo camera
        if (stereo_camera_ptr->getGStreamVideoSourceRightStr(stereo_camera_right_dev_file) == 0) {
            printf(">> Stereo Camera (Right) (%s)\n", stereo_camera_right_dev_file.c_str());
            pipeline_stereo_right = gst_parse_launch
              (("v4l2src device=" + stereo_camera_right_dev_file + " ! image/jpeg, " + stereo_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10001").data(), &error1);
            //gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
        } else {
            fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
            return -1;
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

int VideoStreamer::deinitGStreamer(void)
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

int VideoStreamer::run(CtrlClient &conn)
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
