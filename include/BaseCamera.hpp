#ifndef BASE_CAMERA_HPP
#define BASE_CAMERA_HPP

#include <string>
#include <vector>

struct CameraDesc{
    std::string type;
    std::vector<std::string> names;
    unsigned int width;
    unsigned int height;
    unsigned int framerate;
};

class BaseCamera
{
public:
    virtual int init(const int stereo_video_width, const int stereo_video_height) = 0;
    virtual int deinit(void) = 0;
    virtual int isValid(void) = 0;
    virtual int getGStreamVideoSourceStr(std::string &str) = 0;
};

#endif // BASE_CAMERA_HPP
