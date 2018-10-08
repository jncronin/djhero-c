#ifndef MENUSCREEN_H
#define MENUSCREEN_H

#include "dirlist.h"

lv_obj_t *menuscreen_create();
std::vector<struct dent*> root_menu();
void populate_list(lv_obj_t **l, struct dent* cdent,
    std::vector<struct dent *>entries);
void file_cb(struct dent *dent, struct dent *parent);
void root_cb(struct dent *dent, struct dent *parent);
void music_cb(struct dent *dent, struct dent *parent);

#endif
