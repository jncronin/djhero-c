#include <lvgl/lvgl.h>
#include <iostream>
#include "dirlist.h"
#include "menuscreen.h"

extern bool game_playing;

struct gamedent : dent
{
    std::string cmdline;
};

static bool gamesort(struct dent *a, struct dent *b)
{
    if(a->is_parent)
        return true;
    if(b->is_parent)
        return false;
    return a->name.compare(b->name) < 0;
}

static void game_cb(struct dent *dent, struct dent *parent)
{
    (void)parent;

    auto gd = (struct gamedent *)dent;
    std::cout << "game_cb: " << gd->cmdline << std::endl;
    game_playing = true;
    system(gd->cmdline.c_str());
    game_playing = false;
    std::cout << "game done";
}


// Currently we hard-code in the games known to work
std::vector<struct dent*> game_list()
{
    std::vector<struct dent*> v;

    // prev
    auto prev_dent = new struct dent();
    prev_dent->name = SYMBOL_DIRECTORY"  ..";
    prev_dent->cb = root_cb;
    prev_dent->is_parent = true;
    v.push_back(prev_dent);

    struct gamedent *p;

    p = new struct gamedent();
    p->name = "1941";
    p->cmdline = "FRAMEBUFFER=/dev/fb1 /usr/local/bin/advmame 1941";
    p->cb = game_cb;
    v.push_back(p);

    p = new struct gamedent();
    p->name = "1942";
    p->cmdline = "FRAMEBUFFER=/dev/fb1 /usr/local/bin/advmame 1942";
    p->cb = game_cb;
    v.push_back(p);

    p = new struct gamedent();
    p->name = "Frogger";
    p->cmdline = "FRAMEBUFFER=/dev/fb1 /usr/local/bin/advmame frogger";
    p->cb = game_cb;
    v.push_back(p);

    p = new struct gamedent();
    p->name = "Pac-Man";
    p->cmdline = "FRAMEBUFFER=/dev/fb1 /usr/local/bin/advmame pacman";
    p->cb = game_cb;
    v.push_back(p);



    std::sort(v.begin(), v.end(), gamesort);
    return v;
}