/*
 * references:
 *   - https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux
 *   - https://gist.github.com/edufelipe/6108057
 *   - https://linux.die.net/man/3/getifaddrs
 */

//#include <stdio.h>
//#include <unistd.h>
//#include <string.h> /* for strncpy */
//
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/ioctl.h>
//#include <netinet/in.h>
//#include <net/if.h>
//#include <arpa/inet.h>
//

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

#include <cerrno>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INTERFACE_NAME "wlp5s0"

int check_wlp5s0_by_ifreq()
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to internet interface */
    strncpy(ifr.ifr_name, INTERFACE_NAME, IFNAMSIZ);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    /* display result */
    printf("=====================================================\n");
    printf("  %s: %s\n", INTERFACE_NAME, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    printf("=====================================================\n");

    return 0;
}


int check_wireless(const char* ifname, char* protocol) {
    int sock = -1;
    struct iwreq pwrq;
    memset(&pwrq, 0, sizeof(pwrq));
    strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 0;
    }

    if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) {
        if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);
        close(sock);
        return 1;
    }

    close(sock);
    return 0;
}

int check_all_interfaces_by_getifaddrs(void)
{
    printf("=====================================================\n");
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        char protocol[IFNAMSIZ]  = {0};

        int family = ifa->ifa_addr->sa_family;
        printf("interface %12s: address family: %2x%6s\n",
                ifa->ifa_name, family,
                (family == AF_PACKET) ? " (AF_PACKET)" :
                (family == AF_INET) ?   " (AF_INET)" :
                (family == AF_INET6) ?  " (AF_INET6)" : "");

        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            printf("\t+- %s\n", inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
            if (check_wireless(ifa->ifa_name, protocol)) {
                printf("\t+- is wireless: %s\n", protocol);
            } else {
                printf("\t+- is not wireless\n");
            }
        }
    }
    printf("=====================================================\n");

    freeifaddrs(ifaddr);
    return 0;
}

int check_wireless_interface_ip(void)
{
    printf("=====================================================\n");
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        char protocol[IFNAMSIZ]  = {0};

        int family = ifa->ifa_addr->sa_family;

        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            if (check_wireless(ifa->ifa_name, protocol)) {
                printf("interface %12s: address family: %2x%6s\n",
                    ifa->ifa_name, family,
                    (family == AF_PACKET) ? " (AF_PACKET)" :
                    (family == AF_INET) ?   " (AF_INET)" :
                    (family == AF_INET6) ?  " (AF_INET6)" : "");
                printf("\t+- %s\n", inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
                printf("\t+- is wireless: %s\n", protocol);
            }
        }
    }
    printf("=====================================================\n");

    freeifaddrs(ifaddr);
    return 0;
}

int main(int argc, char const *argv[]) {

    check_wlp5s0_by_ifreq();

    check_all_interfaces_by_getifaddrs();

    check_wireless_interface_ip();

    return 0;
 }
