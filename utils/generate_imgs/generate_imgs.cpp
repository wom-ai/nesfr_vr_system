#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include <stdio.h>

#include <unistd.h>


#include <dirent.h>


using namespace cv;

using namespace std;


int saved_index = 0;
int end_index = 40;


GstClockTime calc_diff(GstClockTime time, GstClockTime time1)
{
	GstClockTime diff;
	
	if (time > time1)
		diff = time - time1;
	else 
		diff = time1 - time;

	return diff;		
}

int
main (int argc, char *argv[])
{
	string path = "video0/";


	//figure out the index of the last saved image so we don't overwrite it
	DIR *dir = opendir("./video0");
	struct dirent* entry;

	char const* digits = "0123456789";

	while (entry = readdir(dir))
	{
		string img(entry->d_name);

		cout << img << endl;

		if (img == "." || img == "..")
			continue;

		auto first_digit_index = img.find_first_of(digits);
		auto last_digit_index = img.find_first_of(".");

		int number = stoi(img.substr(first_digit_index, last_digit_index));

		if (number > saved_index)
			saved_index = number;
	}


	cout << "started" << endl;

	sleep(5);

	cout << "woke up" << endl;





	GstElement *pipeline, *appsink;
	GstSample *sample; 
	GError *error = NULL;
	
	gst_init (&argc, &argv);

	pipeline = gst_parse_launch("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! jpegdec ! videoconvert ! video/x-raw, format=\"BGR\" ! appsink name=appsink", &error);


	// pipeline = gst_parse_launch("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! jpegdec ! appsink name=appsink", &error);
		
//	pipeline = gst_parse_launch("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=60/1 ! jpegdec ! xvimagesink", &error);

	GstElement *pipeline1, *appsink1;
	GstSample *sample1; 
	GError *error1 = NULL;

	pipeline1 = gst_parse_launch("v4l2src device=/dev/video2 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! jpegdec ! videoconvert ! video/x-raw, format=\"BGR\" ! appsink name=appsink", &error);
	// pipeline1 = gst_parse_launch("v4l2src device=/dev/video2 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! jpegdec !appsink name=appsink", &error1);


	if (error != NULL) {
    	g_print ("could not construct pipeline: %s\n", error->message);
    	g_error_free (error);
    	exit (-1);
  	}

  	if (error1 != NULL) {
    	g_print ("could not construct pipeline: %s\n", error1->message);
    	g_error_free (error1);
    	exit (-1);
  	}

	appsink = gst_bin_get_by_name (GST_BIN (pipeline), "appsink");

	appsink1 = gst_bin_get_by_name (GST_BIN (pipeline1), "appsink");

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	gst_element_set_state(pipeline1, GST_STATE_PLAYING);
	




	GError *error2 = NULL;
	GstElement *pipeline2 = gst_parse_launch("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=60/1 ! jpegdec ! xvimagesink", &error2);

	if (error2 != NULL) {
    	g_print ("could not construct pipeline: %s\n", error2->message);
    	g_error_free (error2);
    	exit (-1);
  	}

  	gst_element_set_state(pipeline2, GST_STATE_PLAYING);


//	GstBus *bus = gst_element_get_bus (pipeline);
//	gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);	
	
	//g_print("1\n");
	
	// namedWindow("webcam", WINDOW_AUTOSIZE);

	int cycle_count = 0;
	int saved_count = saved_index;

	GstSample *prev_sample = NULL;
	GstSample *prev_sample1 = NULL;

	// pair<GstBuffer*, GstClockTime> prev_frame;
	// pair<GstBuffer*, GstClockTime> prev_frame1;


	while (TRUE) {	

		cycle_count++;
	
		sample = gst_app_sink_pull_sample (GST_APP_SINK (appsink));
		
	
		if (sample == NULL) {
			g_print("Rip, no frame\n");
			continue;
		}
		


		GstBuffer *buffer = gst_sample_get_buffer (sample);


		sample1 = gst_app_sink_pull_sample (GST_APP_SINK (appsink1));
		
	
		if (sample1 == NULL) {
			g_print("Rip, no frame1\n");
			continue;
		}
		
		GstBuffer *buffer1 = gst_sample_get_buffer (sample1);

		GstClockTime buffer_t = buffer->pts;
		GstClockTime buffer_t1 = buffer1->pts;

		GstClockTime diff = calc_diff(buffer_t, buffer_t1);


		if (cycle_count % 30 == 10)
		{
			GstBuffer *save_buffer = buffer;
			GstBuffer *save_buffer1 = buffer1;

			if (diff > pow(10, 7))
			{
				if (buffer_t > buffer_t1)
				{
					save_buffer = gst_sample_get_buffer (prev_sample); 
				} else {
					save_buffer1 = gst_sample_get_buffer (prev_sample1);
				}
			}

			diff = calc_diff(save_buffer->pts, save_buffer1->pts);

			if (diff < pow(10, 7))
			{
				GstMapInfo map;
				gst_buffer_map ( save_buffer, &map, GST_MAP_READ);
				
				GstMapInfo map1;
				gst_buffer_map ( save_buffer1, &map1, GST_MAP_READ);

				Mat img = Mat(Size(1920, 1080), CV_8UC3, (char *) map.data, Mat::AUTO_STEP);

				Mat img1 = Mat(Size(1920, 1080), CV_8UC3, (char *) map1.data, Mat::AUTO_STEP);

				imwrite("video0/image" + to_string(saved_count) +".png", img);
				imwrite("video2/image" + to_string(saved_count) +".png", img1);


				gst_buffer_unmap(save_buffer, &map);
				gst_buffer_unmap(save_buffer1, &map1);

				

				cout << "saved imgs: " << saved_count << endl; 
				cout << "diff in ns: " << GST_TIME_AS_NSECONDS(diff) << endl; 

				saved_count++;

				if (saved_count >= end_index)
					break;
			}
			
		}

		if (prev_sample)
			gst_sample_unref(prev_sample);
		
		if (prev_sample1)
			gst_sample_unref(prev_sample1);

		prev_sample = sample;
		prev_sample1 = sample1;


		
		// gst_buffer_unmap(buffer, &map);
		
		
	}
	
	gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    gst_element_set_state(pipeline1, GST_STATE_NULL);
    gst_object_unref(pipeline1);
//	gst_object_unref(bus);	

    return 0;
}
