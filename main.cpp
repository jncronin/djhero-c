#include <boost/filesystem.hpp>
#include <iostream>
#include <lvgl/lvgl.h>
#include <lv_drivers/display/monitor.h>
#include <lv_drivers/indev/mouse.h>
#include <SDL2/SDL.h>

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


// A screen containing only a list for scrolling up/down
lv_obj_t *scr_list;

// The list itself
lv_obj_t *list;

// Current directory listing
std::vector<struct dent*> dlist;

static std::vector<struct dent*> enum_dir(path dir)
{
	std::vector<struct dent*> v;

	if(!equivalent(dir, path(root)))
	{
		auto prev_dent = new dent();
		prev_dent->name = "..";
		prev_dent->p = dir.parent_path();
		v.push_back(prev_dent);
	}

	for(auto x : directory_iterator(dir))
	{
		auto p = x.path();
		if(is_directory(p) || is_regular_file(p) && p.extension() == ".mp3")
		{
			auto cdent = new dent();
			cdent->name = p.filename().native();
			cdent->p = p;
			v.push_back(cdent);
		}
	}

	return v;
}

void populate_list(lv_obj_t **l, path p)
{
	*l = lv_list_create(scr_list, NULL);

	dlist = enum_dir(p);

	for(auto x : dlist)
	{
		const char *cs = x->name.c_str();
		lv_list_add(*l, NULL, cs, NULL);
	}
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
	
	SDL_CreateThread(tick_thread, "tick", NULL);

	
	path p(root);

	// Initialise screen
	scr_list = lv_obj_create(NULL, NULL);

	populate_list(&list, p);

	lv_scr_load(scr_list);

	while(true)
	{
		lv_task_handler();
		usleep(10000);
	}

	while(true)
	{
		auto v = enum_dir(p);
		int i = 0;
		for(auto x : v)
		{
			cout << i++ << ": " << x->name << endl;
		}

		scanf("%i", &i);

		p = v[i]->p;

		if(is_regular_file(p))
		{
			cout << "chose : " << p << endl;
			return 0;
		}
	}

	return 0;
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

