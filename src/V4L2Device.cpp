#include "V4L2Device.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <dirent.h>

#include <functional>

bool V4L2Device::_configure_camera(const std::string &dev_file)
{
    FILE *pin = nullptr;
    char buffer [128];
    char line[1024];
    memset(buffer, 0, sizeof(buffer));
    if (sprintf (buffer, "v4l2-ctl --device=%s -l", dev_file.c_str()) < 0)
        return false;

    printf(">> call [%s]\n", buffer);
    pin = popen(buffer,"r");
    if (!pin)
        return false;
    else {
        while (fgets(line, 1024, pin)) {
            printf(">> %s", line);
        }
        pclose(pin);
    }

    memset(buffer, 0, sizeof(buffer));
    if (sprintf (buffer, "v4l2-ctl --device=%s -c white_balance_temperature_auto=0", dev_file.c_str()) < 0)
        return false;
    printf(">> call [%s]\n", buffer);
    pin = popen(buffer,"r");
    if (!pin)
        return false;
    else {
        while (fgets(line, 1024, pin)) {
            printf(">> %s", line);
        }
        pclose(pin);
    }

    memset(buffer, 0, sizeof(buffer));
    if (sprintf (buffer, "v4l2-ctl --device=%s -C white_balance_temperature_auto", dev_file.c_str()) < 0)
        return false;
    printf(">> call [%s]\n", buffer);
    pin = popen(buffer,"r");
    if (!pin)
        return false;
    else {
        while (fgets(line, 1024, pin)) {
            printf(">> %s", line);
        }
        pclose(pin);
    }
    return true;
}


static const char *prefixes[] = {
    "video",
//    "radio",
//    "vbi",
//    "swradio",
//    "v4l-subdev",
//    "v4l-touch",
//    "media",
    nullptr
};

bool V4L2Device::_is_v4l_dev(const char *name)
{
    for (unsigned i = 0; prefixes[i]; i++) {
        unsigned l = strlen(prefixes[i]);

        if (!memcmp(name, prefixes[i], l)) {
            if (isdigit(name[l]))
                return true;
        }
    }
    return false;
}

int V4L2Device::_calc_node_val(const char *s)
{
    int n = 0;

    s = std::strrchr(s, '/') + 1;

    for (unsigned i = 0; prefixes[i]; i++) {
        unsigned l = strlen(prefixes[i]);

        if (!memcmp(s, prefixes[i], l)) {
            n = i << 8;
            n += atol(s + l);
            return n;
        }
    }
    return 0;
}

bool V4L2Device::_sort_on_device_name(const std::string &s1, const std::string &s2)
{
    int n1 = _calc_node_val(s1.c_str());
    int n2 = _calc_node_val(s2.c_str());

    return n1 < n2;
}


void V4L2Device::_init_dev_files(void)
{
    DIR *dp;
    struct dirent *ep;
    dev_vec files;
    dev_map links;
    struct v4l2_capability vcap;

    dp = opendir("/dev");
    if (dp == nullptr) {
        perror ("Couldn't open the directory");
        return;
    }

    while ((ep = readdir(dp)))
        if (_is_v4l_dev(ep->d_name))
        {
            files.push_back(std::string("/dev/") + ep->d_name);
        }
    closedir(dp);

    /* Find device nodes which are links to other device nodes */
    for (auto iter = files.begin();
            iter != files.end(); ) {
        char link[64+1];
        int link_len;
        std::string target;

        link_len = readlink(iter->c_str(), link, 64);
        if (link_len < 0) {	/* Not a link or error */
            iter++;
            continue;
        }
        link[link_len] = '\0';

        /* Only remove from files list if target itself is in list */
        if (link[0] != '/')	/* Relative link */
            target = std::string("/dev/");
        target += link;
        if (find(files.begin(), files.end(), target) == files.end()) {
            iter++;
            continue;
        }

        /* Move the device node from files to links */
        if (links[target].empty())
            links[target] = *iter;
        else
            links[target] += ", " + *iter;
        iter = files.erase(iter);
    }

    /*
     * reference
     *  - https://stackoverflow.com/questions/29286439/how-to-use-sort-in-c-with-custom-sort-member-function
     *  - https://en.cppreference.com/w/cpp/utility/functional/bind
     */

#if 0
    std::sort(files.begin(), files.end(), [&, this](const std::string &s1, const std::string &s2)
        {
            int n1 = this->_calc_node_val(s1.c_str());
            int n2 = this->_calc_node_val(s2.c_str());

            return n1 < n2;
        }
    );
#else
    //auto _sorter = std::mem_fn(&V4L2Device::_sort_on_device_name);
    auto sorter = std::bind(&V4L2Device::_sort_on_device_name, this, std::placeholders::_1, std::placeholders::_2);
    std::sort(files.begin(), files.end(), sorter);
#endif

    printf("-[v4l_devs]----------------------------------\n");
    for (const auto &file : files) {
        int fd = open(file.c_str(), O_RDWR);
        std::string bus_info;
        std::string card;

        if (fd < 0)
            continue;
        int err = ioctl(fd, VIDIOC_QUERYCAP, &vcap);
        if (err) {
        } else {
            bus_info = reinterpret_cast<const char *>(vcap.bus_info);
            card = reinterpret_cast<const char *>(vcap.card);
        }
        close(fd);

        if (err)
            continue;
        if (!(vcap.device_caps & V4L2_CAP_VIDEO_CAPTURE))
            continue;
        printf("  %s\n", file.c_str());
        file_card_map[file.c_str()] = card;
    }
    printf("--------------------------------------------\n");
}

bool V4L2Device::_find_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("%s:%s(%s)\n", __FILE__, __FUNCTION__, temp.c_str());

    bool ret = false;
    for (const auto &file_card : file_card_map) {
        //printf("(%s)\n{%s)\n", id_str.c_str(), file_card.second.c_str());
        bool match = false;
        for (const auto id_str : id_strs)
            match |= (file_card.second.compare(id_str) == 0);
        if (match)
        {
            dev_file = file_card.first;
            printf("  (%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            break;
        }
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}

bool V4L2Device::_find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("%s:%s(%s)\n", __FILE__, __FUNCTION__, temp.c_str());

    printf("\n-[v4l_devs]-------------------------------------\n");
    for (const auto &file_card : file_card_map)
        printf("  [%s]:%s\n", file_card.first.c_str(), file_card.second.c_str());
    printf("------------------------------------------------\n\n");

    bool ret = false;
    //for (const auto &file_card : file_card_map) {
    for (auto iter = file_card_map.begin();
            iter != file_card_map.end(); ) {

        auto file_card = *iter;

        //printf("(%s)\n{%s)\n", id_str.c_str(), file_card.second.c_str());
        bool match = false;
        for (const auto id_str : id_strs)
            match |= (file_card.second.compare(id_str) == 0);
        if (match)
        {
            dev_file = file_card.first;
            printf("  (%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            file_card_map.erase(iter);
            break;
        }
        else
            iter++;
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}

bool V4L2Device::_find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file, std::string &found_id_str)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("%s:%s(%s)\n", __FILE__, __FUNCTION__, temp.c_str());

    printf("\n-[v4l_devs]-------------------------------------\n");
    for (const auto &file_card : file_card_map)
        printf("  [%s]:%s\n", file_card.first.c_str(), file_card.second.c_str());
    printf("------------------------------------------------\n\n");

    bool ret = false;
    //for (const auto &file_card : file_card_map) {
    for (auto iter = file_card_map.begin();
            iter != file_card_map.end(); ) {

        auto file_card = *iter;

        //printf("(%s)\n{%s)\n", id_str.c_str(), file_card.second.c_str());
        bool match = false;
        for (const auto id_str : id_strs)
            match |= (file_card.second.compare(id_str) == 0);
        if (match)
        {
            dev_file = file_card.first;
            printf("  (%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            file_card_map.erase(iter);
            found_id_str = file_card.second.c_str();
            break;
        }
        else
            iter++;
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}
