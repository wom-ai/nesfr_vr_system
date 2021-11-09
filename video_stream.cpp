#include <gst/gst.h>
#include <unistd.h>

int main ()
{   
    GError *error = NULL;
    GError *error1 = NULL;

    gst_init (NULL, NULL);

    GstElement *pipeline = gst_parse_launch
      ("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! rtpjpegpay ! udpsink host=192.168.0.230 port=10000", &error);

    GstElement *pipeline1 = gst_parse_launch
      ("v4l2src device=/dev/video1 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! rtpjpegpay ! udpsink host=192.168.0.230 port=10001", &error1);
    
    if (error != NULL) {
        g_error("Couldn't launch the pipeline");
        return -1;
    }

    if (error1 != NULL) {
        g_error("Couldn't launch the pipeline1");
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    gst_element_set_state(pipeline1, GST_STATE_PLAYING);

    // GstBus *bus = gst_element_get_bus (pipeline);
    // GstBus *bus1 = gst_element_get_bus (pipeline1);

    
    // gst_bus_pop_filtered (bus, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    // gst_bus_pop_filtered (bus1, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    while (true) {
        sleep(10);    
    }

    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

    gst_object_unref (bus1);
    gst_element_set_state (pipeline1, GST_STATE_NULL);
    gst_object_unref (pipeline1);

    return 0;
}