#include <lvgl/lvgl.h>
#include <gst/gst.h>

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

void overlay_init()
{
    ov_pipeline = gst_element_factory_make("playbin", "playbin");
    bus = gst_element_get_bus(ov_pipeline);
}

void overlay_loop()
{
    // TODO
}

void overlay_play(int id)
{
    static int cur_id = -1;

    gst_element_set_state(ov_pipeline, GST_STATE_NULL);
    g_object_set(ov_pipeline, "uri", ofiles[id], NULL);
    gst_element_set_state(ov_pipeline, GST_STATE_PLAYING);

    if(id != cur_id)
    {
        // TODO: show image
        cur_id = id;
    }
}
