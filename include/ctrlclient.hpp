#ifndef CTRLCLIENT_HPP
#define CTRLCLIENT_HPP

#include <string>

enum class HeadsetCtrlCmd{
    NONE = 0,
    REGISTER,
    STREAM_STATE,
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
    struct CmdHeader predefined_header;

public:
    int init(const std::string &hostname);
    int conn(const std::string &ip_str);
    void run(void);

    int _read(void *buf, size_t len);
    int readcmd(struct RemoteCtrlCmdMsg &msg);


    int _write(const void *buf, size_t len);
    int write_id(void);
    int write_cmd(void);
    int write_streamstate(const int play);

    int deinit(void); 
};

#endif //CTRLCLIENT_HPP
