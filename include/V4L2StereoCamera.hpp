#ifndef V4L2_STEREO_CAMERA_HPP
#define V4L2_STEREO_CAMERA_HPP

#include "BaseStereoCamera.hpp"
#include "V4L2Device.hpp"

class V4L2StereoCamera : public BaseStereoCamera, V4L2Device
{
public:
    V4L2StereoCamera(void);
    virtual int init(const int stereo_video_width, const int stereo_video_height, const int mono_flag=0);
    virtual int deinit(void);
    virtual int isValid(void);
    virtual int getGStreamVideoSourceLeftStr(std::string &str);
    virtual int getGStreamVideoSourceRightStr(std::string &str);
    
private:
    const std::vector<std::string> stereo_camera_left_strs = {"Stereo Vision 1", "Stereo Vision 1: Stereo Vision ", "Video Capture 5",};
    const std::vector<std::string> stereo_camera_right_strs = {"Stereo Vision 2", "Stereo Vision 2: Stereo Vision ", "Video Capture 5",};

    std::string stereo_camera_left_dev_file = "";
    std::string stereo_camera_right_dev_file = "";

    int stereo_video_width = 1280;
    int stereo_video_height = 720;
};

#endif // V4L2_STEREO_CAMERA_HPP
