#ifndef UTILS_HPP
#define UTILS_HPP

#include <gst/gst.h>

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
