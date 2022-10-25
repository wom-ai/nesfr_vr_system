#ifndef VIDEO_STREAMER_HPP
#define VIDEO_STREAMER_HPP

#include "CtrlClient.hpp"
#include "BaseStereoCamera.hpp"
#include "BaseCamera.hpp"
#include "BaseAudio.hpp"

#include <gst/gst.h>
#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <vector>

struct Offset2Df {
    float x = 0;
    float y = 0;
};

struct StereoViewProperty{
    float width;
    float height;

#if 1
    Offset2Df offset_left;
    Offset2Df offset_right;
#else
    float rotation[9];
    float translation[3];
#endif

    // for camera left
    float fx0;
    float fy0;
    float cx0;
    float cy0;

    float k0[3];

    float p0[2];

    // for camera right
    float fx1;
    float fy1;
    float cx1;
    float cy1;

    float k1[3];

    float p1[3];
};

struct CameraDesc{
    std::string type;
    std::vector<std::string> names;
    unsigned int width;
    unsigned int height;
    unsigned int framerate;
};

class VideoStreamer {
protected:
    std::atomic_bool &system_on;

    std::string headset_ip = "192.168.0.XXX";
    int stereo_flag = true;

    struct CameraDesc camera_desc_left;
    struct CameraDesc camera_desc_right;
    struct StereoViewProperty stereo_view_property;
    struct CameraDesc camera_desc_main;
    struct AudioInDesc audioin_desc;
    struct AudioOutDesc audioout_desc;

    GstElement *pipeline_stereo_left = nullptr;
    GstElement *pipeline_stereo_right = nullptr;
    GstElement *pipeline_main = nullptr;
    GstElement *pipeline_audio = nullptr;

    std::shared_ptr<BaseStereoCamera> stereo_camera_ptr = nullptr;
    std::shared_ptr<BaseCamera> camera_ptr = nullptr;

public:
    VideoStreamer(  std::atomic_bool &system_on
                    , std::string headset_ip
                    , const int stereo_flag
                    , const struct CameraDesc &camera_desc_left
                    , const struct CameraDesc &camera_desc_right
                    , const struct StereoViewProperty &stereo_view_property
                    , const struct CameraDesc &camera_desc_main
                    , const struct AudioInDesc &audioin_desc
                    , const struct AudioOutDesc &audioout_desc
                    );
    ~VideoStreamer(void);

    int run(CtrlClient &conn);

    int initDevices(void);
    int deinitDevices(void);

    int initGStreamer(void);
    int deinitGStreamer(void);

    int playStream(void);
    int stopStream(void);

    const struct StereoViewProperty &getStereoViewProperty(void) { return stereo_view_property; }
};

#endif //VIDEO_STREAMER_HPP
