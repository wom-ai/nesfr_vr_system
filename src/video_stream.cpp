#include <gst/gst.h>
#include <unistd.h>
#include <string>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <cstring>
#include <algorithm>
#include <vector>
#include <map>
#include <linux/videodev2.h>

#include "video_stream.hpp"

using namespace std;

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
            printf("%s%s\n", "/dev/", ep->d_name);
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
        } else {
            bus_info = reinterpret_cast<const char *>(vcap.bus_info);
            card = reinterpret_cast<const char *>(vcap.card);
        }
        close(fd);

        if (err)
            continue;
        file_card_map[file.c_str()] = card;
    }
}

static bool find_dev_file_by_str(const string &id_str, string &dev_file)
{
    printf("find_dev_file_by_str((%s), (%s))\n", id_str.c_str(), dev_file.c_str());
    bool ret = false;
    for (const auto &file_card : file_card_map) {
        if (id_str.compare(file_card.second) == 0)
        {
            dev_file = file_card.first;
            printf("\t(%s) found in (%s)\n", file_card.second.c_str(), file_card.first.c_str());
            ret = true;
            break;
        }
        //printf("(%s):(%s)\n", file_card.first.c_str(), file_card.second.c_str());
    }
    return ret;
}

int main ()
{
    string main_camera_card_str = "USB Video: USB Video";
    string stereo_camera_left_str = "Stereo Vision 1: Stereo Vision ";
    string stereo_camera_right_str = "Stereo Vision 2: Stereo Vision ";

    string main_camera_dev_file;
    string stereo_camera_left_dev_file;
    string stereo_camera_right_dev_file;

    init_dev_files();

    if (find_dev_file_by_str(main_camera_card_str, main_camera_dev_file))
    {
    }
    else
    {
        perror ("Couldn't open Main Camera");
        return;
    }

    if (find_dev_file_by_str(stereo_camera_left_str, stereo_camera_left_dev_file))
    {
    }
    else
    {
        perror ("Couldn't open Stereo Camera (Left)");
        return;
    }

    if (find_dev_file_by_str(stereo_camera_right_str, stereo_camera_right_dev_file))
    {
    }
    else
    {
        perror ("Couldn't open Stereo Camera (Right)");
        return;
    }

    GError *error = NULL;
    GError *error1 = NULL;
    GError *error2 = NULL;

    gst_init (NULL, NULL);

    //put the headset's ip here
    string headset_ip = "192.168.0.230";

    //change /dev/video# to the correct numbers

    // one eye of the stereo camera
    GstElement *pipeline = gst_parse_launch
      (("v4l2src device=" + stereo_camera_left_dev_file + " ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10000").data(), &error); 

    // one eye of the stereo camera
    GstElement *pipeline1 = gst_parse_launch
      (("v4l2src device=" + stereo_camera_right_dev_file + " ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10001").data(), &error1);
    
    //main camera
    GstElement *pipeline2 = gst_parse_launch
      (("v4l2src device=" + main_camera_dev_file + " ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=60/1 ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error2);
    

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    gst_element_set_state(pipeline1, GST_STATE_PLAYING);
    gst_element_set_state(pipeline2, GST_STATE_PLAYING);

    if (error != NULL) {
        g_error("Couldn't launch the pipeline");
        return -1;
    }

    if (error1 != NULL) {
        g_error("Couldn't launch the pipeline1");
        return -1;
    }

    if (error2 != NULL) {
        g_error("Couldn't launch the pipeline2");
        return -1;
    }



    // GstBus *bus = gst_element_get_bus (pipeline);
    // GstBus *bus1 = gst_element_get_bus (pipeline1);

    
    // gst_bus_pop_filtered (bus, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    // gst_bus_pop_filtered (bus1, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    while (true) {
        sleep(10);    
    }

    // gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

    // gst_object_unref (bus1);
    gst_element_set_state (pipeline1, GST_STATE_NULL);
    gst_object_unref (pipeline1);

    gst_element_set_state (pipeline2, GST_STATE_NULL);
    gst_object_unref (pipeline2);

    return 0;
}
