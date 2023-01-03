#ifndef BASE_STEREO_CAMERA_HPP
#define BASE_STEREO_CAMERA_HPP

#include <string>

class BaseStereoCamera
{
public:
    virtual int init(const int stereo_video_width, const int stereo_video_height, const int mono_flag=0) = 0;
    virtual int deinit(void) = 0;
    virtual int isValid(void) = 0;
    virtual int getGStreamVideoSourceLeftStr(std::string &str) = 0;
    virtual int getGStreamVideoSourceRightStr(std::string &str) = 0;
};

#endif // BASE_STEREOCAMERA_HPP
