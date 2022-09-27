#ifndef VIDEO_STREAMER_HPP
#define VIDEO_STREAMER_HPP

#include <gst/gst.h>
#include <atomic>
#include <vector>
#include <string>
#include "CtrlClient.hpp"

class VideoStreamer {
protected:
    std::atomic_bool &system_on;

    std::string headset_ip = "192.168.0.XXX";
    bool is_stereo = true;
    int stereo_video_width = 1280;
    int stereo_video_height = 720;

    int main_video_width = 1280;
    int main_video_height = 720;

    const std::vector<std::string> main_camera_card_strs = {"USB Video", "USB Video: USB Video", "Video Capture 3", "Logitech StreamCam", "HD Pro Webcam C920"};
    const std::vector<std::string> stereo_camera_left_strs = {"Stereo Vision 1", "Stereo Vision 1: Stereo Vision ", "Video Capture 5",};
    const std::vector<std::string> stereo_camera_right_strs = {"Stereo Vision 2", "Stereo Vision 2: Stereo Vision ", "Video Capture 5",};

    GstElement *pipeline_stereo_left = nullptr;
    GstElement *pipeline_stereo_right = nullptr;
    GstElement *pipeline_main = nullptr;
    GstElement *pipeline_audio = nullptr;

public:
    VideoStreamer(std::atomic_bool &system_on, std::string headset_ip, int stereo_video_width, int stereo_video_height, int main_video_width, int main_video_height);
    ~VideoStreamer(void);

    int run(CtrlClient &conn);

    int initGStreamer(void);
    int deinitGStreamer(void);
};

#endif //VIDEO_STREAMER_HPP
