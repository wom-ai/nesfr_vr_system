#ifndef V4L2_CAMERA_HPP
#define V4L2_CAMERA_HPP

#include "BaseCamera.hpp"
#include "V4L2Device.hpp"

class V4L2Camera : public BaseCamera, V4L2Device
{
public:
    V4L2Camera(const std::vector<std::string> &camera_names);
    virtual int init(const int video_width, const int video_height);
    virtual int deinit(void);
    virtual int isValid(void);
    virtual int getGStreamVideoSourceStr(std::string &str);
    
private:
    std::vector<std::string> camera_card_strs;
    std::string camera_dev_file = "";

    int video_width = 1280;
    int video_height = 720;
};

#endif // V4L2_CAMERA_HPP
