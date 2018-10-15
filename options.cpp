#include <lvgl/lvgl.h>

#include "menuscreen.h"
#include "dirlist.h"
#include "options.h"

extern lv_obj_t *list;

void pulse_audio_ports();

static void pa_cb(struct dent *dent, struct dent *parent)
{
	pulse_audio_ports();
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




	populate_list(&list, NULL, v);
}

