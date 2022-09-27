#include "V4L2StereoCamera.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <dirent.h> 

#include <algorithm>
#include <string>
#include <cstring>

static bool configure_stereo_camera(const std::string &dev_file)
{
    int n;
    FILE *pin = nullptr;
    char buffer [128];
    char line[1024];
    memset(buffer, 0, sizeof(buffer));
    n = sprintf (buffer, "v4l2-ctl --device=%s -l", dev_file.c_str());
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
    n = sprintf (buffer, "v4l2-ctl --device=%s -c white_balance_temperature_auto=0", dev_file.c_str());
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
    n = sprintf (buffer, "v4l2-ctl --device=%s -C white_balance_temperature_auto", dev_file.c_str());
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

/*
 * references
 *   - https://git.linuxtv.org/v4l-utils.git/tree/utils/v4l2-ctl/v4l2-ctl-common.cpp
 */
using dev_vec = std::vector<std::string>;
using dev_map = std::map<std::string, std::string>;
static dev_map file_card_map;

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

static bool is_v4l_dev(const char *name)
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

static int calc_node_val(const char *s)
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

static bool sort_on_device_name(const std::string &s1, const std::string &s2)
{
    int n1 = calc_node_val(s1.c_str());
    int n2 = calc_node_val(s2.c_str());

    return n1 < n2;
}

static void init_dev_files(void)
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
        if (is_v4l_dev(ep->d_name))
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

    std::sort(files.begin(), files.end(), sort_on_device_name);

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

/*
static bool find_dev_file_by_strs(const std::vector<string> &id_strs, string &dev_file)
{
    string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("find_dev_file_by_strs(%s)\n", temp.c_str());

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
*/

static bool find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("find_and_remove_dev_file_by_strs(%s)\n", temp.c_str());

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

static bool find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file, std::string &found_id_str)
{
    std::string temp = "";
    for (const auto id_str : id_strs) {
        temp += "<";
        temp += id_str;
        temp += ">,";
    }
    printf("find_and_remove_dev_file_by_strs(%s)\n", temp.c_str());

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
        printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}

V4L2StereoCamera::V4L2StereoCamera(void)
{
}

int V4L2StereoCamera::init(const int stereo_video_width, const int stereo_video_height, const int mono_flag)
{
    this->stereo_video_width = stereo_video_width;
    this->stereo_video_height = stereo_video_height;

    init_dev_files();

    // left
    if (find_and_remove_dev_file_by_strs(stereo_camera_left_strs, stereo_camera_left_dev_file)) {
        printf(">> Stereo Camera (Left) (%s)\n", stereo_camera_left_dev_file.c_str());
        if (!configure_stereo_camera(stereo_camera_left_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Stereo Camera (Left)\n");
            return -1;
        }
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Stereo Camera (Left)\n");
        return -1;
    }

    if (mono_flag)
        return 0;

    // right
    if (find_and_remove_dev_file_by_strs(stereo_camera_right_strs, stereo_camera_right_dev_file)) {
        printf(">> Stereo Camera (Right) (%s)\n", stereo_camera_right_dev_file.c_str());
        if (!configure_stereo_camera(stereo_camera_right_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Stereo Camera (Right)\n");
            return -1;
        }
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Stereo Camera (Right)\n");
        return -1;
    }
    return 0;
}

int V4L2StereoCamera::deinit(void)
{
    return 0;
}

int V4L2StereoCamera::isValid(void)
{
    return 0;
}

int V4L2StereoCamera::getGStreamVideoSourceLeftStr(std::string &str)
{
    str = stereo_camera_left_dev_file;
    if (str.empty())
        return -1;
    return 0;
}

int V4L2StereoCamera::getGStreamVideoSourceRightStr(std::string &str)
{
    str = stereo_camera_right_dev_file;
    if (str.empty())
        return -1;
    return 0;
}
