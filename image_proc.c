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
    if (!img) return;
    int total_bytes = img->w * img->h * img->channels;
    for (int i = 0; i < total_bytes; i++) {
        int new_val = img->data[i] + val;
        if (new_val < 0) new_val = 0;
        if (new_val > 255) new_val = 255;
        img->data[i] = (unsigned char)new_val;
    }
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

Image *apply_crop(Image *img, int start_x, int start_y, int crop_w, int crop_h) {
    if (!img || start_x < 0 || start_y < 0 || start_x + crop_w > img->w || start_y + crop_h > img->h) return NULL;
    Image *cropped = create_image(crop_w, crop_h);
    for (int y = 0; y < crop_h; y++) {
        for (int x = 0; x < crop_w; x++) {
            int src_idx = ((start_y + y) * img->w + (start_x + x)) * img->channels;
            int dst_idx = (y * crop_w + x) * img->channels;
            for (int c = 0; c < img->channels; c++) {
                cropped->data[dst_idx + c] = img->data[src_idx + c];
            }
        }
    }
    return cropped;
}
    }
    return cropped;
}
