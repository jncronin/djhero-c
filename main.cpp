#include <boost/filesystem.hpp>
#include <iostream>
#include <lvgl/lvgl.h>
#include <lv_drivers/display/fbdev.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <gd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dirlist.h"
#include "menuscreen.h"
#include "music.h"
#include "overlay.h"

#if LV_MEM_CUSTOM == 0
#error Please set lv_conf.h LV_MEM_CUSTOM to 1
#endif

using namespace boost::filesystem;
using std::cout;
using std::endl;

static void populate_list(lv_obj_t **l, path p);
static const void *load_image(const char *fname);
static bool kb_read(lv_indev_data_t *data);

// Is a game currently playing?  Used to ignore input
bool game_playing = false;

// Is music playing?
bool music_playing = false;

// A screen containing only a list for scrolling up/down
static lv_obj_t *scr_list;

// Now playing screen
lv_obj_t *scr_play;
lv_obj_t *play_img;
lv_obj_t *play_lab;

// The list itself
lv_obj_t *list = NULL;

// A group for keyboard navigation
lv_group_t *list_grp = NULL;

// Keyboard driver
lv_indev_t *kbd;

// Current directory listing
std::vector<struct dent*> dlist;

// Data for evdev keyboard driver
static uint32_t kb_last_key;
static lv_indev_state_t kb_state;
static struct libevdev *dev = NULL;

static uint32_t keycode_to_ascii(uint32_t ie_key)
{
	switch(ie_key)
	{
		case KEY_RIGHT:
			return LV_GROUP_KEY_RIGHT;
		case KEY_LEFT:
			return LV_GROUP_KEY_LEFT;
		case KEY_UP:
			return LV_GROUP_KEY_UP;
		case KEY_DOWN:
			return LV_GROUP_KEY_DOWN;
		case KEY_ESC:
			return LV_GROUP_KEY_ESC;
		case KEY_ENTER:
			return LV_GROUP_KEY_ENTER;
		case KEY_R:
			return 'r';
		case KEY_G:
			return 'g';
		case KEY_B:
			return 'b';
		case KEY_BACKSPACE:
			return '\b';
		default:
			return ie_key;
	}
}

static const char * find_largest_jpg(path cp)
{
	uintmax_t max_fs = 0;
	std::string *cmax = NULL;

	for(auto x : directory_iterator(cp))
	{
		auto p = x.path();
		if(is_regular_file(p) && (p.extension() == ".jpg" || p.extension() == ".jpeg"))
		{
			auto csize = file_size(p);
			if(csize > max_fs)
			{
				max_fs = csize;
				if(cmax)
					delete cmax;
				cmax = new std::string(p.c_str());
			}
		}
	}

	if(cmax)
		return cmax->c_str();
	else
		return NULL;
}

static lv_res_t list_cb(lv_obj_t *btn)
{
	auto dent = ((struct dent*)btn->free_ptr);
	if(is_regular_file(dent->p))
	{
		// Find the largest *.jpg file in the folder
		auto img_file = find_largest_jpg(dent->p.parent_path());
		if(img_file == NULL)
		{
			lv_img_set_src(play_img, SYMBOL_AUDIO);
		}
		else
		{
			lv_img_set_src(play_img, load_image(img_file));
		}

		//lv_img_set_src(play_img, load_image("Folder.jpg"));
		lv_label_set_text(play_lab, dent->p.filename().c_str());
		
		lv_scr_load(scr_play);
		
		cout << "Pressed " << ((struct dent*)btn->free_ptr)->p << endl;
	
		//exit(0);
	}
	else
	{
		//populate_list(&list, dent->p);
	}

	return LV_RES_OK;
}

int main(int argc, char *argv[])
{
	int fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
	if(fd == -1)
	{
		printf("unable to open device\n");
		return -1;
	}

	int rc = libevdev_new_from_fd(fd, &dev);

	lv_init();
	music_init(argc, argv);
	overlay_init();

	// Framebuffer
	fbdev_init();
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.disp_flush = fbdev_flush;
	lv_disp_drv_register(&disp_drv);

	// Keyboard via evdev interface
	lv_indev_drv_t ipt;
	lv_indev_drv_init(&ipt);
	ipt.type = LV_INDEV_TYPE_KEYPAD;
	ipt.read = kb_read;
	kbd = lv_indev_drv_register(&ipt);
	
	// Initialise screen
	scr_list = menuscreen_create();

	lv_scr_load(scr_list);

	while(true)
	{
		if(game_playing)
		{
			usleep(100000);
		}
		else
		{
			struct input_event ev;
			rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

			if(rc == LIBEVDEV_READ_STATUS_SUCCESS)
			{
				if(ev.type == EV_KEY)
				{
					if(ev.value == 1)
					{
						kb_last_key = ev.code;
						kb_state = LV_INDEV_STATE_PR;
					}
					else if(ev.value == 0)
					{
						kb_state = LV_INDEV_STATE_REL;
					}
				}
			}

			speed_ctrl_loop();
			if(music_playing)
			{
				music_loop();
			}
				
			lv_task_handler();
			usleep(5000);
			lv_tick_inc(5);
		}
	}

	return 0;
}

static bool kb_read(lv_indev_data_t *data)
{
	static uint32_t last_key;
	static lv_indev_state_t last_state;

	uint32_t new_key = keycode_to_ascii(kb_last_key);

	// detect any key presses to awaken the buttons on the music screen
	if(kb_state != last_state || last_key != new_key)
	{
		music_unhide();
		printf("unhiding due to state %u and key %u\n", kb_state, new_key);
		last_state = kb_state;
		last_key = new_key;

		// handle overlay buttons
		if(last_state == LV_INDEV_STATE_PR)
		{
			if(last_key == 'r')
				overlay_play(0);
			else if(last_key == 'g')
				overlay_play(1);
			else if(last_key == 'b')
				overlay_play(2);
		}
	}

	data->state = kb_state;
	data->key = new_key;

	return false;
}

static const void *load_image(const char *fname)
{
	// Test image load
	auto f = fopen(fname, "r");
	auto img = gdImageCreateFromJpegEx(f, 1);
	fclose(f);

	gdImageSetInterpolationMethod(img, GD_BILINEAR_FIXED);
	auto scaled = gdImageScale(img, 240, 240);

	gdImageDestroy(img);

	auto imgbuf = new int16_t[240 * 240];
	int ptr = 0;
	for(int y = 0; y < 240; y++)
	{
		for(int x = 0; x < 240; x++)
		{
			// convert to rgb565
			int rgb = scaled->tpixels[y][x];

			int b = rgb & 0xff;
			int g = (rgb >> 8) & 0xff;
			int r = (rgb >> 16) & 0xff;

			imgbuf[ptr++] = (int16_t)((b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11));
		}
	}

	gdImageDestroy(scaled);

	auto lvimg = new lv_img_t();
	memset(lvimg, 0, sizeof(lv_img_t));
	lvimg->header.format = LV_IMG_FORMAT_INTERNAL_RAW;
	lvimg->header.w = 240;
	lvimg->header.h = 240;
	lvimg->header.alpha_byte = 0;
	lvimg->pixel_map = (const uint8_t *)imgbuf;

	return (const void *)lvimg;
}

