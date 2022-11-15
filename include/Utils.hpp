#ifndef UTILS_HPP
#define UTILS_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>

#include <gst/gst.h>
class NetUtils {

public:

    static bool call_nmap(const char* ip_addr)
    {
        FILE *pin = nullptr;
        char buffer[128];
        sprintf(buffer, "nmap -sn %s/24", ip_addr);
        pin = popen(buffer,"r");
        if (!pin)
            return false;
        else {
            while (!feof(pin)) {
                char *line = nullptr;
                size_t len = 0;
                ssize_t read = getline(&line, &len, pin);
                if (read)
                    printf("%s", line);
            }
            pclose(pin);
        }
        return true;
    }

    /*
     * reference
     *   - https://www.cplusplus.com/reference/cstring/strtok/
     */
    static bool find_headset_ip_by_MACAddr(const std::string &mac_addr, std::string &ip)
    {
        bool ret = false;
        FILE *pin = nullptr;
        pin = popen("arp -n","r");
        if (!pin)
            return false;

        std::vector<std::string> lines;
        while (!feof(pin)) {
            char *line = nullptr;
            size_t len = 0;
            ssize_t read = getline(&line, &len, pin);
            if (read)
                lines.push_back(line);
        }
        pclose(pin);

        // parse lines
        for (const auto line : lines) {
            char *pch;
            std::vector<std::string> tokens;
            pch = strtok ((char *)line.c_str()," ,-\n");
            while (pch != NULL)
            {
                printf ("(%s) ",pch);
                tokens.push_back(pch);
                pch = strtok (NULL, " ,-\n");
            }

    //        if (tokens.size() > 0)
    //            printf("%s\n", tokens[0].c_str());
    //
            if (tokens.size() == 5 && tokens[2].compare(mac_addr) == 0) {
                ip = tokens[0];
                printf("- found VR Headset:%s", ip.c_str());
                ret = true;
                break;
            }
            printf("\n");
        }

        return ret;
    }

    /*
     * references
     *  - https://stackoverflow.com/questions/4937529/polling-interface-names-via-siocgifconf-in-linux
     *  - http://minimonk.tistory.com/581
     *  - https://linux.die.net/man/7/netdevice
     *  - https://linux.die.net/man/3/ether_ntoa
     */
    static int getIPAddrbyHWAddr(std::string &ip_addr, const std::string &hw_addr)
    {
        struct sockaddr_in *sin;
        struct sockaddr *sa;

        struct ifconf ifcfg;
        struct ifreq ifr;
        int fd;
        int numreqs = 30;
        //fd = socket(AF_INET, SOCK_DGRAM, 0);
        //fd = socket(PF_INET, SOCK_DGRAM, 0);
        fd = socket(PF_INET, SOCK_STREAM, 0);

        memset(&ifcfg, 0, sizeof(ifcfg));
        ifcfg.ifc_buf = NULL;
        ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
        ifcfg.ifc_buf = (char *)malloc(ifcfg.ifc_len);

        ifcfg.ifc_buf = (char *)realloc(ifcfg.ifc_buf, ifcfg.ifc_len);
        if (ioctl(fd, SIOCGIFCONF, (char *)&ifcfg) < 0) {
            fprintf(stderr, "[ERROR] ioctl(SIOCGIFCONF) failed, %s(%d)\n", strerror(errno), errno);
            close (fd);
            return -1;
        }
        printf("%d : %ld \n", ifcfg.ifc_len, sizeof(struct ifreq));

        int size = ifcfg.ifc_len/sizeof(struct ifreq);
        printf("size=%d\n", size);

        struct ifreq *cur_ifr = ifcfg.ifc_req;
        for (int i = 0; i < size; i++)
        {
            //printf("[%s]\n", cur_ifr->ifr_name);
            sin = (struct sockaddr_in *)&cur_ifr->ifr_addr;
            //printf("IP    %s %d\n", inet_ntoa(sin->sin_addr), sin->sin_addr.s_addr);
            strncpy(ifr.ifr_name, cur_ifr->ifr_name, IFNAMSIZ);
            if ( ntohl(sin->sin_addr.s_addr) != INADDR_LOOPBACK) {
                ioctl(fd, SIOCGIFHWADDR, &ifr);
                sa = &ifr.ifr_hwaddr;

                if (hw_addr.compare(ether_ntoa((struct ether_addr *)sa->sa_data)) == 0) {
                    printf("[INFO] getIPAddrbyHWAddr() found %s by %s\n", inet_ntoa(sin->sin_addr), ether_ntoa((struct ether_addr *)sa->sa_data));
                    ip_addr = inet_ntoa(sin->sin_addr);
                    close (fd);
                    return 0;
                }
            }
        /*
            ioctl(fd,  SIOCGIFBRDADDR, &ifr);
            sin = (struct sockaddr_in *)&ifr.ifr_broadaddr;
            printf("BROD  %s\n", inet_ntoa(sin->sin_addr));
            ioctl(fd, SIOCGIFNETMASK, &ifr);
            sin = (struct sockaddr_in *)&ifr.ifr_addr;
            printf("MASK  %s\n", inet_ntoa(sin->sin_addr));
            ioctl(fd, SIOCGIFMTU, &ifr);
            printf("MTU   %d\n", ifr.ifr_mtu);
            printf("\n");
        */
            cur_ifr++;
        }
        close (fd);
        return -1;
    }
};

class AudioPlayer {
private:
    std::string dev_type;
    std::string dev_name;

public:
    AudioPlayer(const std::string dev_type, const std::string dev_name)
        : dev_type(dev_type)
        , dev_name(dev_name) {}

    int playOggFile(const std::string &path, const double volume=1.0) {

        gst_init(nullptr, nullptr);

        printf("[INFO] %s:%d path=%s\n", __FUNCTION__, __LINE__, path.c_str());
        GError *error = NULL;
        std::string audiosink = (dev_type + " device=" + dev_name);
        std::string launch_script = ("filesrc location=" + path + " ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! volume volume=" + std::to_string(volume) + " ! level  ! " + audiosink);

        printf("[INFO] %s:%d gst_parse_launch(%s)\n", __FUNCTION__, __LINE__, launch_script.c_str());
        GstElement *pipeline = gst_parse_launch (launch_script.c_str(), &error);
        if (error) {
            g_printerr("[ERROR] Unable to build pipeline for *.ogg: %s\n", error->message);
            g_clear_error (&error);
            return -1;
        }

        GstStateChangeReturn ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set the pipeline to the playing state for %s.\n", path.c_str());
            gst_object_unref (pipeline);
            return -1;
        }

        GstBus *bus = gst_element_get_bus (pipeline);;
        GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (msg != NULL) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("[ERROR] received from element %s: %s\n",
                            GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("[ERROR] Debugging information: %s\n",
                            debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    break;
                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    break;
                default:
                    /* We should not reach here because we only asked for ERRORs and EOS */
                    g_printerr ("[ERROR] Unexpected message received.\n");
                    break;
            }
            gst_message_unref (msg);
        }

        /* Free resources */
        gst_object_unref (bus);
        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (pipeline);

        //gst_deinit();
        return 0;
    }

    //filesrc location=$FILE_LOC0 ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! volume volume=1.2 ! level  ! autoaudiosink
    int playWavFile(const std::string &path, const double volume=1.0) {

        gst_init(nullptr, nullptr);

        printf("[INFO] %s:%d path=%s\n", __FUNCTION__, __LINE__, path.c_str());
        GError *error = NULL;
        std::string audiosink = (dev_type + " device=" + dev_name);
        std::string launch_script = ("filesrc location=" + path + " ! wavparse ! audioconvert ! audioresample ! volume volume=" + std::to_string(volume) + " ! level  ! " + audiosink);

        printf("[INFO] %s:%d gst_parse_launch(%s)\n", __FUNCTION__, __LINE__, launch_script.c_str());
        GstElement *pipeline = gst_parse_launch (launch_script.c_str(), &error);
        if (error) {
            g_printerr("[ERROR] Unable to build pipeline for *.wav: %s\n", error->message);
            g_clear_error (&error);
            return -1;
        }

        GstStateChangeReturn ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set the pipeline to the playing state for %s.\n", path.c_str());
            gst_object_unref (pipeline);
            return -1;
        }

        GstBus *bus = gst_element_get_bus (pipeline);;
        GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        // FIXME: handle SIGXXX Exceptions
        if (msg != NULL) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("[ERROR] received from element %s: %s\n",
                            GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("[ERROR] Debugging information: %s\n",
                            debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    break;
                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    break;
                default:
                    /* We should not reach here because we only asked for ERRORs and EOS */
                    g_printerr ("[ERROR] Unexpected message received.\n");
                    break;
            }
            gst_message_unref (msg);
        }

        /* Free resources */
        gst_object_unref (bus);
        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (pipeline);

        //gst_deinit();
        return 0;
    }

};
#endif // UTILS_HPP
