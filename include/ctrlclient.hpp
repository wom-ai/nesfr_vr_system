#ifndef CTRLCLIENT_HPP
#define CTRLCLIENT_HPP

#include <string>

enum class HeadsetCtrlCmd{
    NONE = 0,
    REGISTER,
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
    int sockfd;
    std::string hostname;
    struct CmdHeader predefined_header;

public:
    int init(const std::string &hostname, const std::string &ip_str);

    void run(void);

    int readcmd();

    int writeid();
    int writecmd();

    int deinit(void); 
};

#endif //CTRLCLIENT_HPP
