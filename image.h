#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

typedef struct {
    int width;
    int height;
    Pixel *data;
} Image;
#pragma pack(pop)

Image *load_bmp(const char *filename);
int save_bmp(const char *filename, const Image *image);
Image *copy_image(const Image *source);
void free_image(Image *image);

#endif
