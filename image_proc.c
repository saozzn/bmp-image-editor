#include "image_proc.h"

void apply_grayscale(Image *img) {
    if (!img || !img->data) return;
    for (int i = 0; i < img->width * img->height; i++) {
        Pixel *p = &img->data[i];
        unsigned char g = (unsigned char)(0.299 * p->r + 0.587 * p->g + 0.114 * p->b);
        p->r = p->g = p->b = g;
    }
}

void apply_invert(Image *img) {
    if (!img || !img->data) return;
    for (int i = 0; i < img->width * img->height; i++) {
        Pixel *p = &img->data[i];
        p->r = 255 - p->r;
        p->g = 255 - p->g;
        p->b = 255 - p->b;
    }
}

void apply_brightness(Image *img, int val) {
    if (!img || !img->data) return;
    for (int i = 0; i < img->width * img->height; i++) {
        Pixel *p = &img->data[i];
        p->r = clamp(p->r + val);
        p->g = clamp(p->g + val);
        p->b = clamp(p->b + val);
    }
}

void apply_flip_h(Image *img) {
    if (!img || !img->data) return;
    int w = img->width;
    int h = img->height;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w / 2; x++) {
            Pixel tmp = img->data[y * w + x];
            img->data[y * w + x] = img->data[y * w + (w - 1 - x)];
            img->data[y * w + (w - 1 - x)] = tmp;
        }
    }
}

void apply_flip_v(Image *img) {
    if (!img || !img->data) return;
    int w = img->width;
    int h = img->height;
    for (int y = 0; y < h / 2; y++) {
        for (int x = 0; x < w; x++) {
            Pixel tmp = img->data[y * w + x];
            img->data[y * w + x] = img->data[(h - 1 - y) * w + x];
            img->data[(h - 1 - y) * w + x] = tmp;
        }
    }
}

Image *apply_rotate90(Image *img) {
    if (!img || !img->data) return NULL;
    int w = img->width;
    int h = img->height;
    Image *rot = create_image(h, w);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            rot->data[x * h + (h - 1 - y)] = img->data[y * w + x];
        }
    }
    return rot;
}

Image *apply_blur(Image *img) {
    if (!img || !img->data) return NULL;
    int w = img->width;
    int h = img->height;
    Image *out = create_image(w, h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        Pixel p = img->data[ny * w + nx];
                        r_sum += p.r; g_sum += p.g; b_sum += p.b;
                        count++;
                    }
                }
            }
            out->data[y * w + x].r = r_sum / count;
            out->data[y * w + x].g = g_sum / count;
            out->data[y * w + x].b = b_sum / count;
        }
    }
    return out;
}

Image *apply_sharpen(Image *img) {
    if (!img || !img->data) return NULL;
    int w = img->width;
    int h = img->height;
    Image *out = create_image(w, h);
    int kernel[3][3] = { {0, -1, 0}, {-1, 5, -1}, {0, -1, 0} };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        Pixel p = img->data[ny * w + nx];
                        int k = kernel[dy + 1][dx + 1];
                        r_sum += p.r * k; g_sum += p.g * k; b_sum += p.b * k;
                    }
                }
            }
            out->data[y * w + x].r = clamp(r_sum);
            out->data[y * w + x].g = clamp(g_sum);
            out->data[y * w + x].b = clamp(b_sum);
        }
    }
    return out;
}

Image *apply_crop(Image *img) {
    if (!img || !img->data) return NULL;
    int w = img->width;
    int h = img->height;
    if (w <= 20 || h <= 20) return NULL;

    int new_w = w * 4 / 5;
    int new_h = h * 4 / 5;
    int start_x = w * 1 / 10;
    int start_y = h * 1 / 10;

    Image *cropped = create_image(new_w, new_h);
    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            cropped->data[y * new_w + x] = img->data[(start_y + y) * w + (start_x + x)];
        }
    }
    return cropped;
}
