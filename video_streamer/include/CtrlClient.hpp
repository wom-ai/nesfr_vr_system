#ifndef CTRLCLIENT_HPP
#define CTRLCLIENT_HPP

#include <string>

#define CTRL_MSG_VERSION 1

enum class HeadsetCtrlCmd{
    NONE = 0,
    REGISTER,
    PUT_STREAM_STATE,
    PUT_STEREO_CAMERA_PROPERTY,
    PUT_DESTINATION_ARRAY,
};

enum class RemoteCtrlCmd{
    NONE = 0,
    PLAY,
    STOP,
};

struct CmdHeader {
    unsigned int version = CTRL_MSG_VERSION;
    char name[16];
    unsigned int cmd;
    unsigned int data_size;
};

struct HeadsetCtrlCmdMsg {
    struct CmdHeader header;
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
    std::string interface_name;

public:
    struct CmdHeader predefined_header;

    CtrlClient(const std::string &hostname, const std::string &interface_name="wlan0");

    int init(void);
    int deinit(void);

    int conn(const std::string &server_ip_str);
    void run(void);

    struct CmdHeader build_header(const unsigned int cmd, const unsigned int data_size);

    int _read(void *buf, size_t len);
    int readcmd(struct RemoteCtrlCmdMsg &msg);

    int _write(const void *buf, size_t len);
    int write_id(void);
    int write_cmd(HeadsetCtrlCmdMsg &msg);
    int write_streamstate(const unsigned int play);

    int write_data(const void *data_ptr, unsigned int size);
};

#endif //CTRLCLIENT_HPP
