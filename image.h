#ifndef IMAGE_H
#define IMAGE_H

#include <lvgl/lvgl.h>
#include <string>

lv_img_t *load_image(std::string fname, int w=240, int h=240);

#endif
