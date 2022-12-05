#ifndef NESFR7_ROS2_HPP
#define NESFR7_ROS2_HPP

#include "BaseRover.hpp"
#include <memory>
#include <atomic>
#include <mutex>

class Nesfr7ROS2 : public BaseRover
{
protected:
    std::atomic_bool &system_on;

    std::atomic_bool is_active = {false};

    std::shared_ptr<std::mutex> thread_mutex_ptr = nullptr;

    int udp_sock_fd = -1;
    fd_set set;
public:
    Nesfr7ROS2(std::atomic_bool &system_on);
    ~Nesfr7ROS2();

    virtual int init(void);
    virtual int deinit(void);

    virtual int isValid(void);

    virtual void setActive(bool flag) { is_active = {flag}; }

    virtual bool isActive(void) { return is_active; }

    virtual void ctrl_thread_func (void);

    virtual int waitForVRInput(void);
};
#endif // NESFR7_ROS2_HPP
