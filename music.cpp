#include <lvgl/lvgl.h>
#include <gst/gst.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "music.h"

// mouse input - we sample the x value for our speed
static struct libevdev *mdev = NULL;
static int last_x = 11; // centre
static bool new_speed = false;

// pipeline data for the main player
static GstElement *pipeline = NULL;

extern bool music_playing;

static void set_speed(GstElement *pline, int intspeed);

void music_init(int argc, char *argv[])
{
    gst_init(&argc, &argv);
}

void speed_ctrl_loop()
{
    int rc;
    if(mdev == NULL)
    {
        // open evdev device
        int fd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
        if(fd == -1)
        {
            printf("unable to open mouse device\n");
            exit(-1);
        }
        rc = libevdev_new_from_fd(fd, &mdev);
    }

    struct input_event ev;
    rc = libevdev_next_event(mdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if(rc == LIBEVDEV_READ_STATUS_SUCCESS)
    {
        if(ev.type == EV_REL && ev.code == REL_X)
        {
            if(ev.value != last_x)
            {
                last_x = ev.value;
                new_speed = true;

                std::cout << "new speed: " << last_x << std::endl;
            }
        }
    }

}

void music_loop()
{
    if(new_speed && pipeline)
    {
        GstState st;
        gst_element_get_state(pipeline, &st, NULL, GST_CLOCK_TIME_NONE);
        if(st == GST_STATE_PLAYING)
        {
            set_speed(pipeline, last_x);
        }
    }

    new_speed = false;

    // TODO handle music events
}

double dspeeds[] = {
    0.2, 0.33, 0.36, 0.39, 0.42, 0.46, 0.5, 0.6, 0.7, 0.8, 0.9,
    1.0,
    1.1, 1.3, 1.5, 1.7, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.3,
    3.6, 3.9
};

static void set_speed(GstElement *pline, int intspeed)
{
    double dspeed = dspeeds[intspeed];

    gint64 position;

    while(!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position));
    auto seek_event = gst_event_new_seek(dspeed, GST_FORMAT_TIME,
        (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
        GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, 0);
    gst_element_send_event(pipeline, seek_event);
}

void play_music(std::string fname)
{
    if(pipeline == NULL)
    {
        // instantiate a new pipeline here once only
        //  - it can be reused by setting the uri property
        pipeline = gst_element_factory_make("playbin", "playbin");
    }

    g_object_set(pipeline, "uri", ("file:///" + fname).c_str(), NULL);

    auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        std::cout << "Error playing file" << std::endl;

        // TODO
        return;
    }

    // set initial speed
    set_speed(pipeline, last_x);

    music_playing = true;
}