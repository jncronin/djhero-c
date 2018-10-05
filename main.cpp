#include <boost/filesystem.hpp>
#include <iostream>
#include <lvgl/lvgl.h>
#include <lv_drivers/display/monitor.h>
#include <lv_drivers/indev/mouse.h>
#include <lv_drivers/indev/keyboard.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <gd.h>

#if LV_MEM_CUSTOM == 0
#error Please set lv_conf.h LV_MEM_CUSTOM to 1
#endif

using namespace boost::filesystem;
using std::cout;
using std::endl;

struct dent
{
	std::string name;
	path p;
};

const char root[] = "/mnt/c/Windows/CSC/v2.0.6/namespace/winchester/john/Music/";

static int tick_thread(void *data);
static void populate_list(lv_obj_t **l, path p);
static const void *load_image(const char *fname);

// A screen containing only a list for scrolling up/down
lv_obj_t *scr_list;

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

static std::vector<struct dent*> enum_dir(path dir)
{
	std::vector<struct dent*> v;

	if(!equivalent(dir, path(root)))
	{
		auto prev_dent = new dent();
		prev_dent->name = SYMBOL_DIRECTORY"  ..";
		prev_dent->p = dir.parent_path();
		v.push_back(prev_dent);
	}

	for(auto x : directory_iterator(dir))
	{
		auto p = x.path();
		if(is_directory(p) || is_regular_file(p) && p.extension() == ".mp3")
		{
			auto cdent = new dent();

			if(is_directory(p))
				cdent->name = std::string(SYMBOL_DIRECTORY"  ") + p.filename().native();
			else
				cdent->name = std::string(SYMBOL_AUDIO"  ") + p.filename().native();
			cdent->p = p;
			v.push_back(cdent);
		}
	}

	return v;
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
		populate_list(&list, dent->p);
	}

	return LV_RES_OK;
}

static void populate_list(lv_obj_t **l, path p)
{
	if(list_grp)
	{
		lv_group_del(list_grp);
		lv_obj_del(*l);
	}
	list_grp = lv_group_create();
	lv_indev_set_group(kbd, list_grp);

	*l = lv_list_create(scr_list, NULL);

	lv_obj_set_pos(*l, 0, 0);
	lv_obj_set_size(*l, 320, 240);

	for(auto x : dlist)
		delete x;
	dlist = enum_dir(p);

	for(auto x : dlist)
	{
		const char *cs = x->name.c_str();
		auto btn = lv_list_add(*l, NULL, cs, list_cb);
		btn->free_ptr = x;
		lv_obj_set_height(btn, 240/8);
	//	lv_group_add_obj(list_grp, btn);
	}

	lv_group_add_obj(list_grp, *l);
}

int main()
{

	lv_init();

	monitor_init();
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.disp_flush = monitor_flush;
	disp_drv.disp_fill = monitor_fill;
	disp_drv.disp_map = monitor_map;
	lv_disp_drv_register(&disp_drv);
	
	mouse_init();
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read = mouse_read;
	lv_indev_drv_register(&indev_drv);
	
	keyboard_init();
	lv_indev_drv_t kb_drv;
	lv_indev_drv_init(&kb_drv);
	kb_drv.type = LV_INDEV_TYPE_KEYPAD;
	kb_drv.read = keyboard_read;
	kbd = lv_indev_drv_register(&kb_drv);

	SDL_CreateThread(tick_thread, "tick", NULL);

	
	path p(root);

	// Initialise screen
	scr_list = lv_obj_create(NULL, NULL);
	populate_list(&list, p);

	// Now playing screen
	scr_play = lv_obj_create(NULL, NULL);
	play_img = lv_img_create(scr_play, NULL);
	play_lab = lv_label_create(scr_play, NULL);

	lv_img_set_auto_size(play_img, false);
	lv_obj_set_size(play_img, 240, 240);
	lv_obj_set_x(play_img, 40);
	lv_obj_set_style(play_lab, &lv_style_pretty);
	lv_obj_refresh_style(play_lab);

	lv_scr_load(scr_list);

	while(true)
	{
		lv_task_handler();
		usleep(10000);
	}

	return 0;
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

	auto imgbuf = new int[240 * 240 * 4];
	int ptr = 0;
	for(int y = 0; y < 240; y++)
	{
		for(int x = 0; x < 240; x++)
		{
			imgbuf[ptr++] = scaled->tpixels[y][x];
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

static int tick_thread(void *data)
{
	(void)data;

	while(true)
	{
		SDL_Delay(5);
		lv_tick_inc(5);
	}

	return 0;
}

