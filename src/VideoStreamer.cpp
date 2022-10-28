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
                , const struct AudioInDesc &audioin_desc
                , const struct AudioOutDesc &audioout_desc
                )
    : system_on(system_on)
    , headset_ip(headset_ip)
    , stereo_flag(stereo_flag)
    , camera_desc_left(camera_desc_left)
    , camera_desc_right(camera_desc_right)
    , stereo_view_property(stereo_view_property)
    , camera_desc_main(camera_desc_main)
    , audioin_desc(audioin_desc)
    , audioout_desc(audioout_desc)
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
    std::string main_camera_dev_file;
    std::string stereo_camera_left_dev_file;
    std::string stereo_camera_right_dev_file;

    GError *error = NULL;
    GError *error1 = NULL;
    GError *error2 = NULL;
    GError *error_audio = NULL;

    gst_init (NULL, NULL);

    size_t size = 128;
    char buf[size];

    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=%d/1 ", camera_desc_left.width, camera_desc_left.height, camera_desc_left.framerate);
    std::string stereo_video_conf_str = buf;

    // one eye of the stereo camera
    if (stereo_camera_ptr->getGStreamVideoSourceLeftStr(stereo_camera_left_dev_file) == 0) {
        printf(">> Stereo Camera (Left) (%s)\n", stereo_camera_left_dev_file.c_str());
        pipeline_stereo_left = gst_parse_launch
          (("v4l2src device=" + stereo_camera_left_dev_file + " ! image/jpeg, " + stereo_video_conf_str+ "  ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10000").data(), &error);
        //gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
    } else {
        fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // mono mode
    if (stereo_flag)
    {
        // one eye of the stereo camera
        if (stereo_camera_ptr->getGStreamVideoSourceRightStr(stereo_camera_right_dev_file) == 0) {
            printf(">> Stereo Camera (Right) (%s)\n", stereo_camera_right_dev_file.c_str());
            pipeline_stereo_right = gst_parse_launch
              (("v4l2src device=" + stereo_camera_right_dev_file + " ! image/jpeg, " + stereo_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10001").data(), &error1);
            //gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
        } else {
            fprintf(stderr, "[ERROR] %s:%d\n", __FUNCTION__, __LINE__);
            return -1;
        }
    }

    //main camera
#if 0
    sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=60/1 ", main_video_width, main_video_height);
    std::string main_video_conf_str = buf;

    if (camera_ptr->getGStreamVideoSourceStr(main_camera_dev_file) == 0) {
        printf(">> Main Camera (%s)\n", main_camera_dev_file.c_str());
        pipeline_main = gst_parse_launch
          (("v4l2src device=" + main_camera_dev_file + " ! image/jpeg, " + main_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error2);
        //gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Main Camera\n");
    }

    //audio in
//    pipeline_audio = gst_parse_launch
//        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! alawenc ! rtpgstpay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);

//    pipeline_audio = gst_parse_launch
//        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! alawenc ! rtppcmapay !application/x-rtp, payload=8, clock-rate=8000 ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);

    pipeline_audio = gst_parse_launch
        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! rtpL16pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);

//    pipeline_audio = gst_parse_launch
//        (("pulsesrc device=alsa_input.usb-046d_Logitech_StreamCam_6A86D645-02.analog-stereo ! rtpL8pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
#endif
    if (camera_ptr->isValid()) {
        sprintf(buf, "width=%d, height=%d, pixel-aspect-ratio=1/1, framerate=%d/1 ", camera_desc_main.width, camera_desc_main.height, camera_desc_main.framerate);
        std::string main_video_conf_str = buf;

        if (camera_ptr->getGStreamVideoSourceStr(main_camera_dev_file) == 0) {
            printf(">> Main Camera (%s)\n", main_camera_dev_file.c_str());
            pipeline_main = gst_parse_launch
              (("v4l2src device=" + main_camera_dev_file + " ! image/jpeg, " + main_video_conf_str + " ! rtpjpegpay ! udpsink host=" + headset_ip + " port=10003").data(), &error2);
            //gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
        } else {
            fprintf(stderr, "[WARN] Couldn't open Main Camera\n");
        }
        printf("[INFO] Main Camera set up\n");
    }

    sprintf(buf, "device=%s ", audioin_desc.name.c_str());
    std::string audio_conf_str = buf;

    if (audioin_desc.type.compare("pulsesrc") == 0) {
//        pipeline_audio = gst_parse_launch
//            (("pulsesrc " + audio_conf_str + " ! alawenc ! rtppcmapay !application/x-rtp, payload=8, clock-rate=8000 ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
        pipeline_audio = gst_parse_launch
            (("pulsesrc " + audio_conf_str + " ! rtpL16pay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
//        pipeline_audio = gst_parse_launch
//            (("pulsesrc " + audio_conf_str + "! alawenc ! rtpgstpay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
//        pipeline_audio = gst_parse_launch
//            (("pulsesrc " + audio_conf_str + " ! avenc_aac ! rtpmp4apay ! udpsink host=" + headset_ip + " port=10004").data(), &error_audio);
        printf("[INFO] Audio-In set up (%s)\n", audio_conf_str.c_str());
    } else {
        fprintf(stderr, "[WARN] Couldn't open Audio-In\n");
    }

    //gst_element_set_state(pipeline_audio, GST_STATE_PLAYING);

    if (error != NULL) {
        g_clear_error (&error);
        g_error("Couldn't launch the pipeline_stereo_left");
        return -1;
    }

    if (error1 != NULL) {
        g_clear_error (&error1);
        g_error("Couldn't launch the pipeline_stereo_right");
        return -1;
    }

    if (error2 != NULL) {
        g_clear_error (&error2);
        g_error("Couldn't launch the pipeline_main");
        return -1;
    }

    if (error_audio != NULL) {
        g_clear_error (&error_audio);
        g_error("Couldn't launch the pipeline_audio");
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
    }

    // gst_object_unref (bus1);
    if (pipeline_stereo_right) {
        gst_element_set_state (pipeline_stereo_right, GST_STATE_NULL);
        gst_object_unref (pipeline_stereo_right);
    }

    if (pipeline_main) {
        gst_element_set_state (pipeline_main, GST_STATE_NULL);
        gst_object_unref (pipeline_main);
    }

    if (pipeline_audio) {
        gst_element_set_state (pipeline_audio, GST_STATE_NULL);
        gst_object_unref (pipeline_audio);
    }

    return 0;
}

int VideoStreamer::playStream(void)
{
    if (pipeline_stereo_left) gst_element_set_state(pipeline_stereo_left, GST_STATE_PLAYING);
    if (pipeline_stereo_right) gst_element_set_state(pipeline_stereo_right, GST_STATE_PLAYING);
    if (pipeline_main) gst_element_set_state(pipeline_main, GST_STATE_PLAYING);
    if (pipeline_audio) gst_element_set_state(pipeline_audio, GST_STATE_PLAYING);
    return 0;
}

int VideoStreamer::stopStream(void)
{
    if (pipeline_stereo_left) gst_element_set_state(pipeline_stereo_left, GST_STATE_PAUSED);
    if (pipeline_stereo_right) gst_element_set_state(pipeline_stereo_right, GST_STATE_PAUSED);
    if (pipeline_main) gst_element_set_state(pipeline_main, GST_STATE_PAUSED);
    if (pipeline_audio) gst_element_set_state(pipeline_audio, GST_STATE_PAUSED);
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

        HeadsetCtrlCmdMsg msg = { conn.build_header((unsigned int)HeadsetCtrlCmd::STEREO_CAMERA_PROPERTY, sizeof(StereoViewProperty)), 0, 0, 0,};
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
