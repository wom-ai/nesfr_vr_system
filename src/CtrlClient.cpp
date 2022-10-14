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

CtrlClient::CtrlClient(const std::string &hostname)
{
    this->hostname = hostname;
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
    return 0;
}
int CtrlClient::conn(const std::string &ip_str)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_str.c_str());
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
    return 0;
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
    HeadsetCtrlCmdMsg msg = { predefined_header, HeadsetCtrlCmd::REGISTER, 0, 0, 0};
    return _write((const void *)&msg, sizeof(msg));
}

int CtrlClient::write_streamstate(const int play)
{
    HeadsetCtrlCmdMsg msg = { predefined_header, HeadsetCtrlCmd::STREAM_STATE, play, 0, 0};
    return _write((const void *)&msg, sizeof(msg));
}
