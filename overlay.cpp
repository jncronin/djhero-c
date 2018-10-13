#include <lvgl/lvgl.h>
#include <gst/gst.h>
#include <iostream>

#include "music.h"
#include "image.h"

// GStreamer objects
GstElement *ov_pipeline = NULL;
static GstBus *bus = NULL;

// overlay music files
static const char *ofiles[] =
{
    "file:///home/jncronin/djhero/a.aif",
    "file:///home/jncronin/djhero/b.aif",
    "file:///home/jncronin/djhero/c.aif"
};

// speed control
bool new_ov_speed = false;
extern int last_x;

// images
int cur_id = -1;
int new_id = -1;
static const char *imgfiles[] = 
{
    "/home/jncronin/src/djhero-c/a.jpg",
    "/home/jncronin/src/djhero-c/b.jpg",
    "/home/jncronin/src/djhero-c/c.jpg",
};
lv_obj_t **scrs;
lv_obj_t *old_scr = NULL;

static void show_overlay_image(int id)
{
    std::cout << "Show image " << id << std::endl;
    if(id >= 0)
    {
        // Only save the old screen if it
        //  is not an overlay image
        auto cur_scr = lv_scr_act();
        bool is_img = false;
        for(int i = 0; i < 3; i++)
        {
            if(scrs[i] == cur_scr)
            {
                is_img = true;
                break;
            }
        }
               
        if(!is_img)
        {
            old_scr = cur_scr;
        }
        lv_scr_load(scrs[id]);
    }
    else
    {
        lv_scr_load(old_scr);
    }
    
    cur_id = id;
}

void overlay_init()
{
    ov_pipeline = gst_element_factory_make("playbin", "playbin");
    
    // Set overlay louder than music player
    g_object_set(ov_pipeline, "volume", 3.0, NULL);
    
    bus = gst_element_get_bus(ov_pipeline);

    scrs = new lv_obj_t*[3];
    for(int i = 0; i < 3; i++)
    {
        scrs[i] = lv_obj_create(NULL, NULL);
        auto img = lv_img_create(scrs[i], NULL);
        lv_obj_set_size(img, 320, 240);
        lv_img_set_src(img, load_image(std::string(imgfiles[i]), 320, 240));
    }
}

void overlay_loop()
{
    if(new_ov_speed && ov_pipeline)
    {
        std::cout << "new_ov_speed" << std::endl;
        set_speed(ov_pipeline, last_x);
        std::cout << "new_ov_speed done" << std::endl;
    }
    // TODO handle events

    while(auto msg = gst_bus_pop_filtered(bus, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED)))
    {
        switch(GST_MESSAGE_TYPE(msg))
        {
            case GST_MESSAGE_ERROR:
                std::cout << "gst ov error" << std::endl;
                show_overlay_image(-1);
                gst_element_set_state(ov_pipeline, GST_STATE_NULL);
                break;

            case GST_MESSAGE_EOS:
                show_overlay_image(-1);
                gst_element_set_state(ov_pipeline, GST_STATE_NULL);
                break;

            case GST_MESSAGE_STATE_CHANGED:
                GstState st;
                gst_element_get_state(ov_pipeline, &st, NULL, 0);
                if(st == GST_STATE_PLAYING)
                {
                    // we may need to update the image
                    if(cur_id != new_id)
                    {
                        show_overlay_image(new_id);
                    }
                }
                break;
        }

        gst_message_unref(msg);
    }

    new_ov_speed = false;
}

void overlay_play(int id)
{
    static int cur_id = -1;

    std::cout << "overlay_play" << std::endl;

    gst_element_set_state(ov_pipeline, GST_STATE_NULL);
    std::cout << "setting uri" << std::endl;
    g_object_set(ov_pipeline, "uri", ofiles[id], NULL);
    auto ret = gst_element_set_state(ov_pipeline, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        std::cout << "Error playing overlay" << std::endl;
        return;
    }
    set_speed(ov_pipeline, last_x);

    new_id = id;

    std::cout << "overlay_play done" << std::endl;
}
