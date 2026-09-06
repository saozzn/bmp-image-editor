#include "image.h"

unsigned char clamp(int val) {
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (unsigned char)val;
}

void free_image(Image *img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

Image *create_image(int w, int h) {
    if (w <= 0 || h <= 0) return NULL;
    Image *img = (Image *)malloc(sizeof(Image));
    img->width = w;
    img->height = h;
    img->data = (Pixel *)calloc(w * h, sizeof(Pixel));
    return img;
}

Image *copy_image(Image *src) {
    if (!src || !src->data) return NULL;
    Image *dst = create_image(src->width, src->height);
    memcpy(dst->data, src->data, src->width * src->height * sizeof(Pixel));
    return dst;
}

Image *load_bmp(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    BMPFileHeader fh;
    BMPInfoHeader ih;
    if (fread(&fh, sizeof(fh), 1, f) != 1 || fread(&ih, sizeof(ih), 1, f) != 1) {
        fclose(f);
        return NULL;
    }

    if (fh.bfType != 0x4D42 || ih.biBitCount != 24 || ih.biCompression != 0) {
        fclose(f);
        return NULL;
    }

    int w = ih.biWidth;
    int h = ih.biHeight < 0 ? -ih.biHeight : ih.biHeight;
    int top_down = ih.biHeight < 0;

    Image *img = create_image(w, h);
    int padding = (4 - (w * 3) % 4) % 4;

    fseek(f, fh.bfOffBits, SEEK_SET);

    for (int y = 0; y < h; y++) {
        int row = top_down ? y : (h - 1 - y);
        for (int x = 0; x < w; x++) {
            unsigned char bgr[3];
            if (fread(bgr, 3, 1, f) != 1) {
                free_image(img);
                fclose(f);
                return NULL;
            }
            img->data[row * w + x].b = bgr[0];
            img->data[row * w + x].g = bgr[1];
            img->data[row * w + x].r = bgr[2];
        }
        fseek(f, padding, SEEK_CUR);
    }

    fclose(f);
    return img;
}

int save_bmp(const char *filename, Image *img) {
    if (!img || !img->data) return 0;
    FILE *f = fopen(filename, "wb");
    if (!f) return 0;

    int w = img->width;
    int h = img->height;
    int padding = (4 - (w * 3) % 4) % 4;
    unsigned int image_size = (w * 3 + padding) * h;

    BMPFileHeader fh = {0x4D42, sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + image_size, 0, 0, sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)};
    BMPInfoHeader ih = {sizeof(BMPInfoHeader), w, h, 1, 24, 0, image_size, 0, 0, 0, 0};

    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);

    unsigned char pad[3] = {0, 0, 0};

    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            Pixel p = img->data[y * w + x];
            unsigned char bgr[3] = {p.b, p.g, p.r};
            fwrite(bgr, 3, 1, f);
        }
        if (padding > 0) fwrite(pad, padding, 1, f);
    }

    fclose(f);
    return 1;
}
