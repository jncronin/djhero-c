#include <lvgl/lvgl.h>
#include <gst/gst.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <id3tag.h>

#include "music.h"
#include "image.h"


// LVGL screen objects
static lv_obj_t *scr_music = NULL;
static lv_obj_t *music_label = NULL;
static lv_obj_t *music_image = NULL;
static lv_img_t* cur_image = NULL;

static lv_obj_t *old_scr = NULL;

// mouse input - we sample the x value for our speed
static struct libevdev *mdev = NULL;
static int last_x = 11; // centre
static bool new_speed = false;

// pipeline data for the main player
static GstElement *pipeline = NULL;

// playlist
std::vector<std::string> cur_playlist;
int cur_playlist_idx = 0;

extern bool music_playing;

static void set_speed(GstElement *pline, int intspeed);

void music_init(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    // Build the screen objects
    scr_music = lv_obj_create(NULL, NULL);
    music_image = lv_img_create(scr_music, NULL);
    music_label = lv_label_create(scr_music, NULL);
    lv_obj_set_size(music_image, 240, 240);
    lv_obj_set_x(music_image, 40);
    lv_obj_set_style(music_label, &lv_style_pretty);
    lv_obj_refresh_style(music_label);
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

void play_music_list(std::vector<std::string> fnames,
    std::string image)
{
    if(fnames.size() == 0)
        return;
    
    // Load folder image
    auto new_image = load_image(image);
    lv_img_set_src(music_image, new_image);
    if(cur_image)
    {
        delete ((lv_img_t*)cur_image)->pixel_map;
        delete cur_image;
    }
    cur_image = new_image;

    cur_playlist = fnames;
    cur_playlist_idx = 0;

    old_scr = lv_scr_act();
    lv_scr_load(scr_music);
    play_music(fnames[0]);
}

static char *get_string(const char *tag_name, struct id3_tag *t)
{
	for(int i = 0; i < t->nframes; i++)
	{
		struct id3_frame *cf = t->frames[i];

		if(!strncmp(tag_name, cf->id, 5))
		{
			// we have found the tag, find a stringlist
			for(int j = 0; j < cf->nfields; j++)
			{
				union id3_field *f = &cf->fields[j];
				if(f->type == ID3_FIELD_TYPE_STRINGLIST)
				{
					unsigned int slen = id3_field_getnstrings(f);

					for(unsigned int k = 0; k < slen; k++)
					{
						const id3_ucs4_t *cs = id3_field_getstrings(f, k);

						char *ret = (char *)id3_ucs4_utf8duplicate(cs);

						return ret;
					}
				}
			}
		}
	}
	return NULL;
}

static void populate_id3(std::string fname)
{
    struct id3_file *id3 = id3_file_open(fname.c_str(), ID3_FILE_MODE_READONLY);
    struct id3_tag *t = id3_file_tag(id3);

    char *tit = get_string("TIT2", t);
    char *alb = get_string("TALB", t);
    char *art = get_string("TPE1", t);

    id3_file_close(id3);
    
    std::string stit = tit ? std::string(tit) : "Unknown Title";
    std::string salb = alb ? std::string(alb) : "Unknown Album";
    std::string sart = art ? std::string(art) : "Unknown Artist";

    std::string ret = stit + "\n" + salb + "\n" + sart;
    if(tit) free(tit);
    if(alb) free(alb);
    if(art) free(art);
   
    lv_label_set_text(music_label, ret.c_str());
}

void play_music(std::string fname)
{
    populate_id3(fname);

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