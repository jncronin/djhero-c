#include <lvgl/lvgl.h>
#include <iostream>
#include "dirlist.h"
#include "menuscreen.h"

void send_serial(char c);

extern bool game_playing;

static lv_obj_t *load_scr = NULL;
static lv_obj_t *load_text = NULL;

struct gamedent : dent
{
    std::string cmdline;
};

static lv_obj_t *get_load_scr(std::string pretty)
{
    if(!load_scr)
    {
        load_scr = lv_obj_create(NULL, NULL);
        load_text = lv_label_create(load_scr, NULL);
    }
    
    lv_label_set_text(load_text, ("Loading " + pretty + "...").c_str());
    return load_scr;
}

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

    send_serial('0');
    auto gd = (struct gamedent *)dent;
#ifdef DEBUG
    std::cout << "game_cb: " << gd->cmdline << std::endl;
#endif
    auto old_scr = lv_scr_act();
    auto new_scr = get_load_scr(gd->name);
    lv_scr_load(new_scr);
    lv_task_handler();
    lv_tick_inc(5);
    game_playing = true;
    system(gd->cmdline.c_str());
    game_playing = false;
#ifdef DEBUG
    std::cout << "game done" << std::endl;
#endif
    lv_scr_load(old_scr);
    send_serial('9');
}

static struct gamedent *mame(std::string pretty, std::string mamename)
{
    auto p = new struct gamedent();
    p->name = pretty;
    p->cmdline = "FRAMEBUFFER=/dev/fb1 /usr/local/bin/advmame --quiet " + mamename;
    p->cb = game_cb;
    return p;
}

static struct gamedent *mame(std::string mamename)
{
    return mame(mamename, mamename);
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

    v.push_back(mame("1941"));
    v.push_back(mame("1942"));
    v.push_back(mame("AfterBurner", "aburner"));
    v.push_back(mame("Aliens", "aliens"));
    v.push_back(mame("Contra", "contra"));
    v.push_back(mame("Donkey Kong", "dkong"));
    v.push_back(mame("Ice Climber", "iceclimb"));
    v.push_back(mame("Jackal", "jackal"));
    v.push_back(mame("Legend of Kage", "lkage"));
    v.push_back(mame("Mario Brothers", "mario"));
    v.push_back(mame("Sonic the Hedgehog", "mp_sonic"));
    v.push_back(mame("Sonic the Hedgehog 2", "mp_soni2"));
    v.push_back(mame("Paperboy", "paperboy"));
    v.push_back(mame("Super Mario Brothers", "suprmrio"));
    v.push_back(mame("Pac-Man", "pacman"));
    v.push_back(mame("Ms. Pac-Man", "mspacman"));
    v.push_back(mame("Frogger", "frogger"));
    v.push_back(mame("Joust", "joust"));
    v.push_back(mame("Arkanoid", "arkanoid"));
    v.push_back(mame("Galaga", "galaga"));
    v.push_back(mame("Galaga '88", "galaga88"));
    v.push_back(mame("Balloon Fight", "balonfgt"));

    std::sort(v.begin(), v.end(), gamesort);
    return v;
}
