#include "Nesfr7ROS2.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "VRInput.hpp"

// FIXME
#include "VRState.h"
#define VR_UDP_PORT_LEFT        10004

#include <netinet/in.h>
#include <unistd.h>

static void init_udp(int &sockfd, uint16_t port)
{
    struct sockaddr_in addr;
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        throw "Cannot open VR!";
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family    = AF_INET; // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if ( bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0 )
    {
        throw "Cannot open VR!";
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

Nesfr7ROS2::Nesfr7ROS2(std::atomic_bool &system_on)
    : system_on(system_on)
{
}

Nesfr7ROS2::~Nesfr7ROS2(void)
{
    system_on = {false};
}

int Nesfr7ROS2::init(void) {
    try {
        init_udp(udp_sock_fd, VR_UDP_PORT_LEFT);
    } catch (std::exception &e) {
        fprintf(stderr, "[ERROR] %s\n", e.what());
        return -1;
    }
    return 0;
}

int Nesfr7ROS2::deinit(void) {

    if (udp_sock_fd != -1)
        close(udp_sock_fd);
    return 0;
}

int Nesfr7ROS2::isValid(void) {
    return 1;
}

int Nesfr7ROS2::waitForVRInput(void)
{
    struct left_control_data_t left_control_data = {
        0,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        false,
        false,
        0,
        0,
    };

    const int timeout_usec = 500000;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_usec; // 100 milliseconds

    FD_ZERO(&set); /* clear the set */
    FD_SET(udp_sock_fd, &set); /* add our file descriptor to the set */

    int retval = select(udp_sock_fd + 1, &set, NULL, NULL, &timeout);
    if (retval == -1) {
        perror("select()");
        return -1;
    }
    else if (retval && FD_ISSET(udp_sock_fd, &set)) {
        //printf("Data is available now.\n");
    }
    else {
        fprintf(stderr, "[VRJoystick] No data within timeout limit (%d usecs).\n", timeout_usec);
        return -1;
    }
    ssize_t nbytes = read(udp_sock_fd, (char *)&left_control_data, sizeof(struct left_control_data_t));

    // FIXME
    if (nbytes != sizeof(struct left_control_data_t)) {
        fprintf(stderr, "[WARN] %s:%d read() failed, %s(%d)\n", __FUNCTION__, __LINE__, strerror(errno), errno);
        return -1;
    }

    return 0;
}

void Nesfr7ROS2::ctrl_thread_func (void)
{
    try {
        rclcpp::init(0, nullptr);

        auto node = rclcpp::Node::make_shared("nesfr_vr_controller");

        auto publisher = node->create_publisher<geometry_msgs::msg::Twist>("/cmd_val", 10);
       
        geometry_msgs::msg::Twist twist_msg;

        while(system_on && rclcpp::ok()) {

            if (is_active) {
                if (waitForVRInput() < 0) {
                    continue;
                }

                publisher->publish(twist_msg);
            } else {
                // Not Active
            }
            rclcpp::spin_some(node);
        }

        rclcpp::shutdown();

        printf("[INFO] %s():%d - Thread End\n", __FUNCTION__, __LINE__);
    } catch (std::exception &e) {
        fprintf(stderr, "[ERROR]: %s\n", e.what());
        system_on = {false};
    }
}
