/*
* references
 *  - https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
 */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "CtrlClient.hpp"

#define MAX 80
#define PORT 12345
void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}

CtrlClient::CtrlClient(const std::string &hostname, const std::string &interface_name)
{
    this->hostname = hostname;
    this->interface_name = interface_name;
    memset(predefined_header.name, 0x0, sizeof(predefined_header.name));
    strncpy(predefined_header.name, hostname.c_str(), sizeof(predefined_header.name));
}

int CtrlClient::init(void)
{
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "[ERROR] socket creation failed...\n");
        return -1;
    }
    else
        printf("Socket successfully created..\n");
/*
    int on = 1;
    if (ioctl(sockfd, FIONBIO, (char *)&on) < 0)
    {
        fprintf(stderr, "[ERROR] ioctl() failed, %s(%d)\n", strerror(errno), errno);
        close(sockfd);
        return -1;
    }
    */
    /*
     * references
     *  - https://stackoverflow.com/questions/47286590/how-to-detect-internet-connectivity-on-a-non-gateway-interface-linuxa
     *  - https://man7.org/linux/man-pages/man7/netdevice.7.html
     */

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);

    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        close(sockfd);
        sockfd = -1;
        return -1;
    }

    return 0;
}

int CtrlClient::conn(const std::string &server_ip_str)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server_ip_str.c_str());
    servaddr.sin_port = htons(PORT);
   
    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        fprintf(stderr, "[ERROR] connection with the server failed, %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    else
        printf("connected to the server..\n");
    return 0;
}

int CtrlClient::deinit(void)
{
    if (sockfd)
        close(sockfd);
    sockfd = -1;
    return 0;
}

struct CmdHeader CtrlClient::build_header(const unsigned int cmd, const unsigned int data_size)
{
    struct CmdHeader header = predefined_header;
    header.cmd = cmd;
    header.data_size = data_size;
    return header;
}

int CtrlClient::_read(void *buf, size_t len)
{
    int ret;
    fd_set set;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;

    while(true) {
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        ret = select(sockfd + 1, &set, NULL, NULL, &timeout);

        if (ret < 0) {
            fprintf(stderr, "[ERROR]: select() failed, %s(%d)\n", strerror(errno), errno);
            break;
        } else if (ret && FD_ISSET(sockfd, &set)) {
            ret = recv(sockfd, buf, len, 0);
            printf("[INFO] receive command from server\n");
            break;
        } else {
            //fprintf(stderr, "[WARN]: select() timeout.\n");
            continue;
        }
    }
    return ret;
}

int CtrlClient::readcmd(struct RemoteCtrlCmdMsg &msg)
{
    return _read((void *)&msg, sizeof(msg));
}

int CtrlClient::_write(const void *buf, size_t len)
{
    int ret = send(sockfd, buf, len, 0);
    return ret;
}

int CtrlClient::write_cmd(HeadsetCtrlCmdMsg &msg)
{
    return _write((const void *)&msg, sizeof(msg));
}

int CtrlClient::write_id()
{
    HeadsetCtrlCmdMsg msg = { build_header((unsigned int)HeadsetCtrlCmd::REGISTER, 0), 0, 0, 0,};
    return _write((const void *)&msg, sizeof(msg));
}

int CtrlClient::write_streamstate(const unsigned int play)
{
    HeadsetCtrlCmdMsg msg = { build_header((unsigned int)HeadsetCtrlCmd::PUT_STREAM_STATE, 0), (int)play, 0, 0,};
    return _write((const void *)&msg, sizeof(msg));
}


int CtrlClient::write_data(const void *data_ptr, unsigned int data_size)
{
    return _write(data_ptr, data_size);
}
