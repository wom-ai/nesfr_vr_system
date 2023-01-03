#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/media.h>
#include <linux/videodev2.h>
/*
 * references:
     - v4l-utils source code v4l-utils/utils/v4l2-ctl/v4l2-ctl-common.cpp
     - https://git.linuxtv.org/v4l-utils.git/tree/utils/v4l2-ctl/v4l2-ctl-common.cpp
 */
#include <dirent.h>

using dev_vec = std::vector<std::string>;
using dev_map = std::map<std::string, std::string>;

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
int main(){
    DIR *dp;
    struct dirent *ep;
    dev_vec files;
    dev_map links;
    dev_map cards;
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
            //printf("%s%s\n", "/dev/", ep->d_name);
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

    for (const auto &file : files) {
        int fd = open(file.c_str(), O_RDWR);
        std::string bus_info;
        std::string card;

        if (fd < 0)
            continue;
        int err = ioctl(fd, VIDIOC_QUERYCAP, &vcap);
        if (err) {
            struct media_device_info mdi;

            err = ioctl(fd, MEDIA_IOC_DEVICE_INFO, &mdi);
            if (!err) {
                if (mdi.bus_info[0])
                    bus_info = mdi.bus_info;
                else
                    bus_info = std::string("platform:") + mdi.driver;
                if (mdi.model[0])
                    card = mdi.model;
                else
                    card = mdi.driver;
            }
        } else {
            bus_info = reinterpret_cast<const char *>(vcap.bus_info);
            card = reinterpret_cast<const char *>(vcap.card);
        }
        close(fd);
        if (err)
            continue;
        if (cards[bus_info].empty())
            cards[bus_info] += card + " (" + bus_info + "):\n";
        cards[bus_info] += "\t" + file;
        if (!(links[file].empty()))
            cards[bus_info] += " <- " + links[file];
        cards[bus_info] += "\n";
    }
    for (const auto &card : cards) {
        printf("%s\n", card.second.c_str());
    }
/*
    int fd;
    struct v4l2_capability video_cap;
    //struct video_window     video_win;
    //struct video_picture   video_pic;

    if((fd = open("/dev/video0", O_RDONLY)) == -1){
        perror("cam_info: Can't open device");
        return 1;
    }

    if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1)
        perror("cam_info: Can't get capabilities");
    else {
        printf("driver:  '%s'\n", video_cap.driver);
        printf("card:    '%s'\n", video_cap.card);
        printf("bus_info:'%s'\n", video_cap.bus_info);
//        printf("Minimum size:\t%d x %d\n", video_cap.minwidth, video_cap.minheight);
//        printf("Maximum size:\t%d x %d\n", video_cap.maxwidth, video_cap.maxheight);
    }

//    if(ioctl(fd, VIDIOCGWIN, &video_win) == -1)
//        perror("cam_info: Can't get window information");
//    else
//        printf("Current size:\t%d x %d\n", video_win.width, video_win.height);
//
//    if(ioctl(fd, VIDIOCGPICT, &video_pic) == -1)
//        perror("cam_info: Can't get picture information");
//    else
//        printf("Current depth:\t%d\n", video_pic.depth);

    close(fd);
*/
    return 0;
}
