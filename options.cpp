#include <lvgl/lvgl.h>
#include <stdlib.h>

#include "menuscreen.h"
#include "dirlist.h"
#include "options.h"

extern lv_obj_t *list;

void pulse_audio_ports();

static void pa_cb(struct dent *dent, struct dent *parent)
{
	pulse_audio_ports();
}

static void v0_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 0%");
}
static void v25_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 25%");
}
static void v50_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 50%");
}
static void v75_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 75%");
}
static void v100_cb(struct dent *dent, struct dent *parent)
{
	system("/usr/bin/amixer sset Master playback 100%");
}

void options_cb(struct dent *dent, struct dent *parent)
{
	std::vector<struct dent *> v;

	// Add back button
	auto prev_dent = new struct dent();
	prev_dent->name = SYMBOL_DIRECTORY"  ..";
	prev_dent->cb = root_cb;
	v.push_back(prev_dent);

	// Pulse audio
	auto pa_dent = new struct dent();
	pa_dent->name = "Reset Amplifier";
	pa_dent->cb = pa_cb;
	v.push_back(pa_dent);

	// Volume controls
	auto v0 = new struct dent();
	v0->name = "Volume 0%";
	v0->cb = v0_cb;
	v.push_back(v0);

	auto v25 = new struct dent();
	v25->name = "Volume 25%";
	v25->cb = v25_cb;
	v.push_back(v25);

	auto v50 = new struct dent();
	v50->name = "Volume 50%";
	v50->cb = v50_cb;
	v.push_back(v50);

	auto v75 = new struct dent();
	v75->name = "Volume 75%";
	v75->cb = v75_cb;
	v.push_back(v75);

	auto v100 = new struct dent();
	v100->name = "Volume 100%";
	v100->cb = v100_cb;
	v.push_back(v100);

	populate_list(&list, NULL, v);
}

