#include <lvgl/lvgl.h>
#include <gst/gst.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <id3tag.h>
#include <time.h>

#include "music.h"
#include "image.h"


// LVGL screen objects
static lv_obj_t *scr_music = NULL;
static lv_obj_t *music_label = NULL;
static lv_obj_t *music_image = NULL;
static lv_img_t* cur_image = NULL;
static lv_obj_t *tobj = NULL;
static lv_obj_t *prog = NULL;
static lv_obj_t *speed_slider = NULL;
static clock_t last_speed_change = 0;
static lv_obj_t *btn_strip = NULL;
static lv_group_t *btn_grp = NULL;

static lv_obj_t *scr_load = NULL;
static lv_obj_t *load_label = NULL;

static lv_obj_t *old_scr = NULL;

// mouse input - we sample the x value for our speed
static struct libevdev *mdev = NULL;
static int last_x = 11; // centre
static bool new_speed = false;

// pipeline data for the main player
static GstElement *pipeline = NULL;
static GstBus *bus = NULL;

// playlist
std::vector<std::string> cur_playlist;
int cur_playlist_idx = 0;

extern bool music_playing;
extern lv_indev_t *kbd;

static void set_speed(GstElement *pline, int intspeed);

static const char *btnm_map[] = { SYMBOL_PREV, SYMBOL_PAUSE, SYMBOL_NEXT, SYMBOL_EJECT, "" };

static void set_hidden(bool hidden)
{
    lv_obj_set_hidden(speed_slider, hidden);
    lv_obj_set_hidden(btn_strip, hidden);
    lv_obj_set_hidden(tobj, hidden);
}

void music_unhide()
{
    last_speed_change = clock();
    set_hidden(false);
}

// null function so that an orange border is not added to btnmap
static void null_style(lv_style_t *style)
{
    (void)style;
}

void music_init(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    // Build the screen objects
    scr_music = lv_obj_create(NULL, NULL);

    music_image = lv_img_create(scr_music, NULL);

    tobj = lv_obj_create(scr_music, NULL);
    music_label = lv_label_create(tobj, NULL);
    lv_obj_set_size(music_image, 240, 240);
    lv_obj_set_x(music_image, 40);

    // Make a transparent style
    static lv_style_t ts;
    lv_style_copy(&ts, &lv_style_plain);
    ts.body.opa = LV_OPA_50;

    lv_obj_set_style(tobj, &ts);
    lv_obj_refresh_style(tobj);

    // And a black style for the screen itself
    static lv_style_t bs;
    lv_style_copy(&bs, &lv_style_plain);
    bs.body.main_color = LV_COLOR_BLACK;
    bs.body.grad_color = LV_COLOR_BLACK;
    lv_obj_set_style(scr_music, &bs);
    lv_obj_refresh_style(scr_music);

    // Progress bar
    prog = lv_bar_create(scr_music, NULL);
    lv_obj_align(prog, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

    // Speed slider
    speed_slider = lv_bar_create(scr_music, NULL);
    lv_obj_align(speed_slider, prog, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_bar_set_range(speed_slider, 1, 21);
    lv_bar_set_value(speed_slider, 11);
    lv_obj_set_hidden(speed_slider, true);

    // Style for the above
    static lv_style_t bars;
    lv_style_copy(&bars, &lv_style_pretty);
    bars.body.empty = true;
    bars.body.border.color = LV_COLOR_WHITE;
    bars.body.border.part = LV_BORDER_FULL;
    bars.body.border.width = 2;
    lv_bar_set_style(prog, LV_BAR_STYLE_BG, &bars);
    lv_bar_set_style(prog, LV_BAR_STYLE_INDIC, &lv_style_pretty);
    lv_obj_refresh_style(prog);
    lv_bar_set_style(speed_slider, LV_BAR_STYLE_BG, &bars);
    lv_bar_set_style(speed_slider, LV_BAR_STYLE_INDIC, &lv_style_pretty);
    lv_obj_refresh_style(speed_slider);

    // Load screen
    scr_load = lv_obj_create(NULL, NULL);
    load_label = lv_label_create(scr_load, NULL);
    lv_label_set_text(load_label, "Loading...");

    // Button strip
    btn_strip = lv_btnm_create(scr_music, NULL);
    lv_btnm_set_map(btn_strip, btnm_map);
    lv_obj_align(btn_strip, speed_slider, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_btnm_set_style(btn_strip, LV_BTNM_STYLE_BG, &bars);
    btn_grp = lv_group_create();
    lv_group_add_obj(btn_grp, btn_strip);
    lv_group_set_style_mod_cb(btn_grp, null_style);
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

                lv_bar_set_value(speed_slider, last_x);
                music_unhide();

                std::cout << "new speed: " << last_x << std::endl;
            }
        }
    }

    if((clock() - last_speed_change) > CLOCKS_PER_SEC / 3)
    {
        set_hidden(true);
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

    if(pipeline)
    {
        gint64 position;

        while(!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position));
        lv_bar_set_value(prog, (int)(position/1000000000));

        // if we are within the first 5 seconds of a track, ensure duration is also updated
        if(position < 5000000000)
        {
            gint64 duration;
            while(!gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration));
            lv_bar_set_range(prog, 0, (int)(duration/1000000000));
        }
    }

    new_speed = false;

    while(auto msg = gst_bus_pop_filtered(bus, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)))
    {
        switch(GST_MESSAGE_TYPE(msg))
        {
            case GST_MESSAGE_ERROR:
                std::cout << "gst error" << std::endl;
                break;

            case GST_MESSAGE_EOS:
                // Enqueue next track if available
                if(cur_playlist_idx++ < cur_playlist.size())
                {
                    play_music(cur_playlist[cur_playlist_idx]);
                }
                else
                {
                    music_playing = false;
                    lv_scr_load(old_scr);
                }
                break;                
        }
    }
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

    // Display loading screen
    old_scr = lv_scr_act();
    lv_scr_load(scr_load);
    lv_task_handler();
    lv_tick_inc(5);
    
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

    lv_scr_load(scr_music);
    lv_indev_set_group(kbd, btn_grp);
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
    lv_obj_set_width(tobj, lv_obj_get_width(music_label));
}

void play_music(std::string fname)
{
    populate_id3(fname);

    if(pipeline == NULL)
    {
        // instantiate a new pipeline here once only
        //  - it can be reused by setting the uri property
        pipeline = gst_element_factory_make("playbin", "playbin");
        bus = gst_element_get_bus(pipeline);
    }

    g_object_set(pipeline, "uri", ("file:///" + fname).c_str(), NULL);

    auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        std::cout << "Error playing file" << std::endl;

        // TODO
        return;
    }

    // get duration into bar
    gint64 dur;

    while(!gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur));
    lv_bar_set_value(prog, 0);
    lv_bar_set_range(prog, 0, (int)(dur/1000000000));

    // set initial speed
    set_speed(pipeline, last_x);

    music_playing = true;
    music_unhide();
}