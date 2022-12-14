#include "VideoStreamer.hpp"
#include "CtrlClient.hpp"
#include "V4L2StereoCamera.hpp"
#include "V4L2Camera.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <cstring>

VideoStreamer::VideoStreamer(  std::atomic_bool &system_on
                , std::string headset_ip
                , const int stereo_flag
                , const struct CameraDesc &camera_desc_left
                , const struct CameraDesc &camera_desc_right
                , const struct StereoViewProperty &stereo_view_property
                , const struct CameraDesc &camera_desc_main
                , const struct AudioInDesc &audio_in_desc
                , const struct AudioOutDesc &audio_out_desc
                )
    : system_on(system_on)
    , headset_ip(headset_ip)
    , stereo_flag(stereo_flag)
    , camera_desc_left(camera_desc_left)
    , camera_desc_right(camera_desc_right)
    , stereo_view_property(stereo_view_property)
    , camera_desc_main(camera_desc_main)
    , audio_in_desc(audio_in_desc)
    , audio_out_desc(audio_out_desc)
{
    stereo_camera_ptr = std::make_shared<V4L2StereoCamera>(camera_desc_left.names, camera_desc_right.names);
    camera_ptr = std::make_shared<V4L2Camera>(camera_desc_main.names);
}

VideoStreamer::~VideoStreamer(void)
{
}

int VideoStreamer::initDevices(void)
{
    printf("[INFO] %s\n", __FUNCTION__);
    if (stereo_camera_ptr->init(camera_desc_left.width, camera_desc_left.height, !stereo_flag) < 0) {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (camera_ptr->init(camera_desc_main.width, camera_desc_main.height) < 0) {
        fprintf(stderr, "[WARN] %s:%d\n", __FUNCTION__, __LINE__);
    }
    return 0;
}

int VideoStreamer::deinitDevices(void)
{
    printf("[INFO] %s\n", __FUNCTION__);
    return 0;
}

int VideoStreamer::initGStreamer(void)
{
    printf("[INFO] %s\n", __FUNCTION__);
    //gst_debug_set_threshold_for_name ("*", GST_LEVEL_WARNING);

    std::string main_camera_dev_file;
    std::string stereo_camera_left_dev_file;
    std::string stereo_camera_right_dev_file;

    GError *error = NULL;
    GError *error_audio = NULL;

    gst_init (NULL, NULL);

    size_t size = 128;
    char buf[size];

    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=%d/1 ", camera_desc_left.width, camera_desc_left.height, camera_desc_left.framerate);
    std::string stereo_video_conf_str = buf;

    // one eye of the stereo camera
    if (stereo_camera_ptr->getGStreamVideoSourceLeftStr(stereo_camera_left_dev_file) == 0) {
        if (pipeline_stereo_left) {
            fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
            return -1;
        }

        printf(">> Stereo Camera (Left) (%s)\n", stereo_camera_left_dev_file.c_str());

        pipeline_stereo_left = gst_parse_launch
          (("v4l2src device=" + stereo_camera_left_dev_file + " ! image/jpeg, " + stereo_video_conf_str+ "  ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10000").data(), &error);
        //gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
        if (!pipeline_stereo_left) {
            if (error) {
                gst_printerr ("[ERROR] pipeline_stereo_left could not be constructed: %s.\n", GST_STR_NULL (error->message));
                g_clear_error (&error);
            } else {
                gst_printerr ("[ERROR] pipeline_stereo_left could not be constructed.\n");
            }
            return -1;
        } else if (error) {
            gst_printerr ("[WARNING] erroneous pipeline_stereo_left: %s\n", GST_STR_NULL (error->message));
            g_clear_error (&error);
            gst_object_unref (pipeline_stereo_left);
            pipeline_stereo_left = nullptr;
            return -1;
        }
    } else {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // mono mode
    if (stereo_flag)
    {
        // one eye of the stereo camera
        if (stereo_camera_ptr->getGStreamVideoSourceRightStr(stereo_camera_right_dev_file) == 0) {
            if (pipeline_stereo_right) {
                fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
                return -1;
            }

            printf(">> Stereo Camera (Right) (%s)\n", stereo_camera_right_dev_file.c_str());
            pipeline_stereo_right = gst_parse_launch
              (("v4l2src device=" + stereo_camera_right_dev_file + " ! image/jpeg, " + stereo_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10001").data(), &error);
            //gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
            if (!pipeline_stereo_right) {
                if (error) {
                    gst_printerr ("[ERROR] pipeline_stereo_right could not be constructed: %s.\n", GST_STR_NULL (error->message));
                    g_clear_error (&error);
                } else {
                    gst_printerr ("[ERROR] pipeline_stereo_right could not be constructed.\n");
                }
                return -1;
            } else if (error) {
                gst_printerr ("[WARNING] erroneous pipeline_stereo_right: %s\n", GST_STR_NULL (error->message));
                g_clear_error (&error);
                gst_object_unref (pipeline_stereo_right);
                pipeline_stereo_right = nullptr;
                return -1;
            }
        } else {
            fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
            return -1;
        }
    }

    //main camera
    if (camera_ptr->isValid()) {
        sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=%d/1 ", camera_desc_main.width, camera_desc_main.height, camera_desc_main.framerate);
        std::string main_video_conf_str = buf;

        if (camera_ptr->getGStreamVideoSourceStr(main_camera_dev_file) == 0) {
            if (pipeline_main) {
                fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
                return -1;
            }

            printf(">> Main Camera (%s)\n", main_camera_dev_file.c_str());
            pipeline_main = gst_parse_launch
              (("v4l2src device=" + main_camera_dev_file + " ! image/jpeg, " + main_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error);
            //gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
            if (!pipeline_main) {
                if (error) {
                    gst_printerr ("[ERROR] pipeline_main could not be constructed: %s.\n", GST_STR_NULL (error->message));
                    g_clear_error (&error);
                } else {
                    gst_printerr ("[ERROR] pipeline_main could not be constructed.\n");
                }
                return -1;
            } else if (error) {
                gst_printerr ("[WARNING] erroneous pipeline_main: %s\n", GST_STR_NULL (error->message));
                g_clear_error (&error);
                gst_object_unref (pipeline_main);
                pipeline_main = nullptr;
                return -1;
            }
        } else {
            fprintf(stderr, "[WARN] Couldn't open Main Camera\n");
        }
        printf("[INFO] Main Camera set up\n");
    }

    // Audio In
    if (pipeline_audio_in) {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (audio_in_desc.type.compare("pulsesrc") == 0) {
        sprintf(buf, "device=%s ", audio_in_desc.name.c_str());
        std::string audio_conf_str = buf;
        // 0
//        pipeline_audio_in = gst_parse_launch
//            (("pulsesrc " + audio_conf_str + " ! alawenc ! rtppcmapay !application/x-rtp, payload=8, clock-rate=8000 ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
        // 1
        pipeline_audio_in = gst_parse_launch
            (("pulsesrc " + audio_conf_str + " ! rtpL16pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
        // 2
//        pipeline_audio_in = gst_parse_launch
//            (("pulsesrc " + audio_conf_str + "! alawenc ! rtpgstpay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
        // 3
//        pipeline_audio_in = gst_parse_launch
//            (("pulsesrc " + audio_conf_str + " ! avenc_aac ! rtpmp4apay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
        printf("[INFO] Audio-In set up (%s)\n", audio_conf_str.c_str());
    } else {
        fprintf(stderr, "[WARN] Couldn't open Audio-In\n");
        return -1;
    }

    if (!pipeline_audio_in) {
        if (error_audio) {
            gst_printerr ("[ERROR] pipeline_audio_in could not be constructed: %s.\n",
                    GST_STR_NULL (error_audio->message));
            g_clear_error (&error_audio);
        } else {
            gst_printerr ("[ERROR] pipeline_audio_in could not be constructed.\n");
        }
        return -1;
    } else if (error_audio) {
        gst_printerr ("[WARNING] erroneous pipeline_audio_in: %s\n",
                GST_STR_NULL (error_audio->message));
        g_clear_error (&error_audio);
        gst_object_unref (pipeline_audio_in);
        pipeline_audio_in = nullptr;
        return -1;
    }

    // Audio Out
    if (pipeline_audio_out) {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (audio_out_desc.type.compare("pulsesink") == 0) {

        std::string launch_str = "udpsrc port=10000 !application/x-rtp, media=audio, clock-rate=44100, encoding-name=L16, channels=1 ! rtpL16depay ! audioconvert ! queue ! pulsesink volume=10.0 device=" + audio_out_desc.name;
        pipeline_audio_out = gst_parse_launch (launch_str.c_str(), &error_audio);
        printf("[INFO] Audio-Out set up (%s)\n", launch_str.c_str());
    } else {
        fprintf(stderr, "[WARN] Couldn't open Audio-Out\n");
        return -1;
    }
    if (!pipeline_audio_out) {
        if (error_audio) {
            gst_printerr ("[ERROR] pipeline_audio_out could not be constructed: %s.\n",
                    GST_STR_NULL (error_audio->message));
            g_clear_error (&error_audio);
        } else {
            gst_printerr ("[ERROR] pipeline_audio_out could not be constructed.\n");
        }
        return -1;
    } else if (error_audio) {
        gst_printerr ("[WARNING] erroneous pipeline_audio_out: %s\n",
                GST_STR_NULL (error_audio->message));
        g_clear_error (&error_audio);
        gst_object_unref (pipeline_audio_out);
        pipeline_audio_out = nullptr;
        return -1;
    }

    return 0;
}

int VideoStreamer::deinitGStreamer(void)
{
    printf("[INFO] %s\n", __FUNCTION__);
    // gst_object_unref (bus);
    if (pipeline_stereo_left) {
        gst_element_set_state (pipeline_stereo_left, GST_STATE_NULL);
        gst_object_unref (pipeline_stereo_left);
        pipeline_stereo_left = nullptr;
    }

    // gst_object_unref (bus1);
    if (pipeline_stereo_right) {
        gst_element_set_state (pipeline_stereo_right, GST_STATE_NULL);
        gst_object_unref (pipeline_stereo_right);
        pipeline_stereo_right = nullptr;
    }

    if (pipeline_main) {
        gst_element_set_state (pipeline_main, GST_STATE_NULL);
        gst_object_unref (pipeline_main);
        pipeline_main = nullptr;
    }

    if (pipeline_audio_in) {
        gst_element_set_state (pipeline_audio_in, GST_STATE_NULL);
        gst_object_unref (pipeline_audio_in);
        pipeline_audio_in = nullptr;
    }

    if (pipeline_audio_out) {
        gst_element_set_state (pipeline_audio_out, GST_STATE_NULL);
        gst_object_unref (pipeline_audio_out);
        pipeline_audio_out = nullptr;
    }
    return 0;
}

int VideoStreamer::playStream(void)
{
    if (pipeline_stereo_left) {
        GstStateChangeReturn ret = gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_stereo_left GST_STATE_PLAYING\n");
            gst_object_unref (pipeline_stereo_left);
            pipeline_stereo_left = nullptr;
            return -1;
        }
    }
    if (pipeline_stereo_right) {
        GstStateChangeReturn ret = gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_stereo_right GST_STATE_PLAYING\n");
            gst_object_unref (pipeline_stereo_right);
            pipeline_stereo_right = nullptr;
            return -1;
        }
    }
    if (pipeline_main) {
        GstStateChangeReturn ret = gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_main GST_STATE_PLAYING\n");
            gst_object_unref (pipeline_main);
            pipeline_main = nullptr;
            return -1;
        }
    }

    if (pipeline_audio_in) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_audio_in, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_audio_in GST_STATE_PLAYING\n");
            gst_object_unref (pipeline_audio_in);
            pipeline_audio_in = nullptr;
            return -1;
        }
    }

    if (pipeline_audio_out) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_audio_out, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_audio_out GST_STATE_PLAYING\n");
            gst_object_unref (pipeline_audio_out);
            pipeline_audio_out = nullptr;
            return -1;
        }
        printf("[INFO] Succeed to set pipeline_audio_out GST_STATE_PLAYING\n");
    }

    return 0;
}

int VideoStreamer::stopStream(void)
{
    if (pipeline_stereo_left) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_stereo_left, GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_stereo_left GST_STATE_PAUSED\n");
            gst_object_unref (pipeline_stereo_left);
            pipeline_stereo_left = nullptr;
            return -1;
        }
    }
    if (pipeline_stereo_right) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_stereo_right, GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_stereo_right GST_STATE_PAUSED\n");
            gst_object_unref (pipeline_stereo_right);
            pipeline_stereo_right = nullptr;
            return -1;
        }
    }
    if (pipeline_main) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_main, GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_main GST_STATE_PAUSED\n");
            gst_object_unref (pipeline_main);
            pipeline_main = nullptr;
            return -1;
        }
    }

    if (pipeline_audio_in) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_audio_in, GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_audio_in GST_STATE_PAUSED\n");
            gst_object_unref (pipeline_audio_in);
            pipeline_audio_in = nullptr;
            return -1;
        }
    }

    if (pipeline_audio_out) {
        GstStateChangeReturn ret = gst_element_set_state (pipeline_audio_out, GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("[ERROR] Unable to set pipeline_audio_out GST_STATE_PAUSED\n");
            gst_object_unref (pipeline_audio_out);
            pipeline_audio_out = nullptr;
            return -1;
        }
        printf("[INFO] Succeed to set pipeline_audio_out GST_STATE_PAUSED\n");
    }

    return 0;
}

int VideoStreamer::run(CtrlClient &conn)
{
    int stream_state = 0;

    while (system_on) {
        if (conn.init()) {
            return -1;
        }
        if (conn.conn(headset_ip)) {
            fprintf(stderr, "[WARN] connectto() failed retry after 1 sec\n");
            sleep(1);
            continue;
        }

        if (initGStreamer() < 0)
        {
            perror ("gstreamer initialization failed.");
            return -1;
        }

        if (conn.write_id() < 0) {
            fprintf(stderr, "[ERROR] write_id() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        HeadsetCtrlCmdMsg msg = { conn.build_header((unsigned int)HeadsetCtrlCmd::PUT_STEREO_CAMERA_PROPERTY, sizeof(StereoViewProperty)), 0, 0, 0,};
        if (conn.write_cmd(msg) < 0) {
            fprintf(stderr, "[ERROR] write_cmd() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        if (conn.write_data((const void*)&stereo_view_property, sizeof(StereoViewProperty)) < 0) {
            fprintf(stderr, "[ERROR] write_data(HeadsetCtrlCmd::STEREO_CAMERA_PROPERTY) failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        if (conn.write_streamstate(stream_state) < 0) {
            fprintf(stderr, "[ERROR] write_streamstate() failed, %s(%d)\n", strerror(errno), errno);
            return -1;
        }

        while (system_on) {
            struct RemoteCtrlCmdMsg msg;
            int ret = conn.readcmd(msg);
            if (ret < 0) {
                fprintf(stderr, "[ERROR] read failed, %s(%d)\n", strerror(errno), errno);
            } else if ( ret == 0) {
                fprintf(stderr, "connection closed\n");
                {
                    printf(">>> STOP\n");
                    stream_state = 0;
                    stopStream();
                }
                conn.deinit();
                break;
            } else {
                switch (msg.cmd) {
                    case  RemoteCtrlCmd::PLAY:
                        {
                            printf(">>> PLAY\n");
                            stream_state = 1;
                            playStream();
                            if (conn.write_streamstate(stream_state) < 0) {
                                fprintf(stderr, "[ERROR] writeid failed, %s(%d)\n", strerror(errno), errno);
                                return -1;
                            }
                            break;
                        }
                    case RemoteCtrlCmd::STOP:
                        {
                            printf(">>> STOP\n");
                            stream_state = 0;
                            stopStream();
                            if (conn.write_streamstate(stream_state) < 0) {
                                fprintf(stderr, "[ERROR] writeid failed, %s(%d)\n", strerror(errno), errno);
                                return -1;
                            }
                            break;
                        }
                    case RemoteCtrlCmd::NONE:
                    default:
                        break;
                }
            }
            usleep(100);
        }
        if (deinitGStreamer() < 0)
        {
            perror ("gstreamer deinitialization failed.");
            return -1;
        }
    }
    return 0;
}
