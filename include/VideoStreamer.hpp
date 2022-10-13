#ifndef VIDEO_STREAMER_HPP
#define VIDEO_STREAMER_HPP

#include "CtrlClient.hpp"
#include "BaseStereoCamera.hpp"
#include "BaseCamera.hpp"

#include <gst/gst.h>
#include <atomic>
#include <vector>
#include <string>
#include <memory>

class VideoStreamer {
protected:
    std::atomic_bool &system_on;

    std::string headset_ip = "192.168.0.XXX";
    int stereo_flag = true;

    struct CameraDesc camera_desc_left;
    struct CameraDesc camera_desc_right;
    struct CameraDesc camera_desc_main;

    GstElement *pipeline_stereo_left = nullptr;
    GstElement *pipeline_stereo_right = nullptr;
    GstElement *pipeline_main = nullptr;
    GstElement *pipeline_audio = nullptr;

public:
    VideoStreamer(  std::atomic_bool &system_on
                    , std::string headset_ip
                    , const int stereo_flag
                    , const struct CameraDesc &camera_desc_left
                    , const struct CameraDesc &camera_desc_right
                    , const struct CameraDesc &camera_desc_main
                    );
    ~VideoStreamer(void);

    int run(CtrlClient &conn);

    int initGStreamer(void);
    int deinitGStreamer(void);

    std::shared_ptr<BaseStereoCamera> stereo_camera_ptr = nullptr;
    std::shared_ptr<BaseCamera> camera_ptr = nullptr;
};

#endif //VIDEO_STREAMER_HPP
