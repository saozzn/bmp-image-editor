#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H
#include "image.h"

void apply_grayscale(Image *img);
void apply_invert(Image *img);
void apply_brightness(Image *img, int val);
void apply_flip_h(Image *img);
void apply_flip_v(Image *img);
Image *apply_rotate90(Image *img);
Image *apply_blur(Image *img);
Image *apply_sharpen(Image *img);
Image *apply_crop(Image *img, int start_x, int start_y, int crop_w, int crop_h);
#endif
