#include <gst/gst.h>
#include <unistd.h>
#include <string>

using namespace std;

int main ()
{   
    GError *error = NULL;
    GError *error1 = NULL;
    GError *error2 = NULL;

    gst_init (NULL, NULL);

    //put the headset's ip here
    string headset_ip = "192.168.0.230";

    //change /dev/video# to the correct numbers

    // one eye of the stereo camera
    GstElement *pipeline = gst_parse_launch
      (("v4l2src device=/dev/video4 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10000").data(), &error); 

    // one eye of the stereo camera
    GstElement *pipeline1 = gst_parse_launch
      (("v4l2src device=/dev/video2 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10001").data(), &error1);
    
    //main camera
    GstElement *pipeline2 = gst_parse_launch
      (("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=60/1 ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error2);
    

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