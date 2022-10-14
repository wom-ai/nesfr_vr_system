#ifndef CTRLCLIENT_HPP
#define CTRLCLIENT_HPP

#include <string>

enum class HeadsetCtrlCmd{
    NONE = 0,
    REGISTER,
    STREAM_STATE,
    STEREO_CAMERA_PROPERTY,
};

enum class RemoteCtrlCmd{
    NONE = 0,
    PLAY,
    STOP,
};

struct CmdHeader {
    char name[16];
};

struct HeadsetCtrlCmdMsg {
    struct CmdHeader header;
    HeadsetCtrlCmd cmd;
    int param0;
    int param1;
    int param2;
};

struct RemoteCtrlCmdMsg {
    struct CmdHeader header;
    RemoteCtrlCmd cmd;
    int param0;
    int param1;
    int param2;
};

class CtrlClient {
private: 
    int sockfd = 0;
    std::string hostname;

public:
    struct CmdHeader predefined_header;

    CtrlClient(const std::string &hostname);

    int init(void);
    int deinit(void);

    int conn(const std::string &ip_str);
    void run(void);

    int _read(void *buf, size_t len);
    int readcmd(struct RemoteCtrlCmdMsg &msg);


    int _write(const void *buf, size_t len);
    int write_id(void);
    int write_cmd(HeadsetCtrlCmdMsg &msg);
    int write_streamstate(const int play);
};

#endif //CTRLCLIENT_HPP
