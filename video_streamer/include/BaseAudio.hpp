#ifndef BASE_AUDIO_HPP
#define BASE_AUDIO_HPP

#include <string>
#include <vector>

struct AudioInDesc{
    std::string type;
    std::string name;
    unsigned int bitrate;
};

struct AudioOutDesc{
    std::string type;
    std::string name;
    unsigned int bitrate;
};

class BaseAudioIn
{
public:
    virtual int init(void) = 0;
    virtual int deinit(void) = 0;
    virtual int isValid(void) = 0;
};

class BaseAudioOut
{
public:
    virtual int init(void) = 0;
    virtual int deinit(void) = 0;
    virtual int isValid(void) = 0;
};
#endif // BASE_AUDIO_HPP
