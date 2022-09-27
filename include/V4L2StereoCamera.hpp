#ifndef V4L2_STEREO_CAMERA_HPP
#define V4L2_STEREO_CAMERA_HPP

#include "BaseStereoCamera.hpp"
#include <vector>
#include <map>

using dev_vec = std::vector<std::string>;
using dev_map = std::map<std::string, std::string>;

class V4L2StereoCamera : public BaseStereoCamera
{
public:
    V4L2StereoCamera(void);
    virtual int init(const int stereo_video_width, const int stereo_video_height, const int mono_flag=0);
    virtual int deinit(void);
    virtual int isValid(void);
    virtual int getGStreamVideoSourceLeftStr(std::string &str);
    virtual int getGStreamVideoSourceRightStr(std::string &str);
    
private:
    const std::vector<std::string> main_camera_card_strs = {"USB Video", "USB Video: USB Video", "Video Capture 3", "Logitech StreamCam", "HD Pro Webcam C920"};
    const std::vector<std::string> stereo_camera_left_strs = {"Stereo Vision 1", "Stereo Vision 1: Stereo Vision ", "Video Capture 5",};
    const std::vector<std::string> stereo_camera_right_strs = {"Stereo Vision 2", "Stereo Vision 2: Stereo Vision ", "Video Capture 5",};

    std::string stereo_camera_left_dev_file = "";
    std::string stereo_camera_right_dev_file = "";

    int stereo_video_width = 1280;
    int stereo_video_height = 720;
};

#endif // V4L2_STEREO_CAMERA_HPP
