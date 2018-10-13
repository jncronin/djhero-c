#include <lvgl/lvgl.h>
#include <gd.h>

#include "image.h"

lv_img_t *load_image(std::string fname, int w, int h)
{
    auto f = fopen(fname.c_str(), "r");
    auto img = gdImageCreateFromJpegEx(f, 1);
    fclose(f);

    gdImageSetInterpolationMethod(img, GD_BILINEAR_FIXED);
	auto scaled = gdImageScale(img, w, h);

	gdImageDestroy(img);

	auto imgbuf = new int16_t[w * h];
	int ptr = 0;
	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
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
	lvimg->header.w = w;
	lvimg->header.h = h;
    lvimg->header.alpha_byte = 0;
	lvimg->pixel_map = (const uint8_t *)imgbuf;

	return lvimg;
}