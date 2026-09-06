#ifndef IMAGE_H
#define IMAGE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int   bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int   bfOffBits;
} BMPFileHeader;

typedef struct {
    unsigned int   biSize;
    int            biWidth;
    int            biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    int            biXPelsPerMeter;
    int            biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

typedef struct { unsigned char r, g, b; } Pixel;
typedef struct { int width; int height; Pixel *data; } Image;

void free_image(Image *img);
Image *create_image(int w, int h);
Image *copy_image(Image *src);
Image *load_bmp(const char *filename);
int save_bmp(const char *filename, Image *img);
unsigned char clamp(int val);
#endif
