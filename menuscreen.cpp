#include <lvgl/lvgl.h>
#include <iostream>
#include "menuscreen.h"
#include "music.h"
#include "options.h"

using namespace boost::filesystem;

std::vector<struct dent*> game_list();

// LVGL screen objects
static lv_obj_t *scr_list = NULL;
lv_obj_t *list = NULL;
extern lv_group_t *list_grp;

// Keyboard driver for associating with the group
extern lv_indev_t *kbd;

// Currently displayed list (cached so we can delete it's)
//  entries when it goes out of scope
static std::vector<struct dent *> dlist;

// Currently displayed node
static struct dent *d = NULL;

lv_obj_t *menuscreen_create()
{
    scr_list = lv_obj_create(NULL, NULL);

    populate_list(&list, NULL, root_menu());

    return scr_list;
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

void music_cb(struct dent *dent, struct dent *parent)
{
#ifdef DEBUG
    std::cout << "music_cb " << dent->p << std::endl;
#endif

    // generate list of all mp3 files and note the index
    //  of the selected one
    std::vector<std::string> subvec;
    int start_at = 0;
    for(int idx = 0; idx < dlist.size(); idx++)
    {
        if(is_regular_file(dlist[idx]->p))
        {
            if(equivalent(dlist[idx]->p, dent->p))
                start_at = subvec.size();
            subvec.push_back(dlist[idx]->p.native());
        }
    }

    std::string image(find_largest_jpg(dent->p.parent_path()));

    play_music_list(subvec, image, start_at);
}

void root_cb(
    struct dent *dent, struct dent *parent)
{
    (void)parent;
    (void)dent;
#ifdef DEBUG
    std::cout << "root_cb" << std::endl;
#endif
    populate_list(&list, NULL, root_menu());
}

void file_cb(struct dent *dent, struct dent *parent)
{
    (void)parent;
#ifdef DEBUG
    std::cout << "file_cb: ";
    std::cout << dent->p << std::endl;
#endif
    populate_list(&list, dent, enum_dir(dent->p));
}

void game_cb(struct dent *dent, struct dent *parent)
{
    populate_list(&list, dent, game_list());
}

static void notimpl_cb(struct dent *dent, struct dent *parent)
{
    std::vector<struct dent*> v;

    // Add back button
    auto prev_dent = new struct dent();
    prev_dent->name = SYMBOL_DIRECTORY"  ..";
    prev_dent->cb = root_cb;
    v.push_back(prev_dent);

    // Add not implemented text
    auto ni_dent = new struct dent();
    ni_dent->name = "Not Implemented";
    ni_dent->cb = NULL;
    v.push_back(ni_dent);

    populate_list(&list, NULL, v);
}

std::vector<struct dent *> root_menu()
{
    // Build a menu that lists the various main options
    std::vector<struct dent *> r;

    auto music_node = new struct dent();
    music_node->name = SYMBOL_AUDIO"  Music";
    music_node->is_parent = false;
    music_node->cb = file_cb;
    music_node->p = path(get_root());

    r.push_back(music_node);

    auto games_node = new struct dent();
    games_node->name = SYMBOL_IMAGE"  Games";
    games_node->is_parent = false;
    games_node->cb = game_cb;
    
    r.push_back(games_node);

    auto opts_node = new struct dent();
    opts_node->name = SYMBOL_SETTINGS"  Options";
    opts_node->cb = options_cb;

    r.push_back(opts_node);

    return r;
}

static lv_res_t list_cb(lv_obj_t *btn)
{
    auto dent = ((struct dent *)btn->free_ptr);
    if(dent->cb)
        dent->cb(dent, d);

    return LV_RES_OK;
}

void populate_list(lv_obj_t **l, struct dent *cdent,
    std::vector<struct dent *>entries)
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
	dlist = entries;
    d = cdent;

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
