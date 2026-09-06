#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#define MAX_UNDO 20

typedef GtkWidget Ihandle;
typedef int (*Icallback)(Ihandle*);

#define IUP_DEFAULT 0
#define IUP_CLOSE -1
#define IUP_CENTER 0

static char g_file_buffer[1024] = {0};
static int g_dialog_status = 0;
static int g_is_save_dlg = 0;

typedef struct {
    unsigned char r, g, b;
} Pixel;

typedef struct {
    int width;
    int height;
    Pixel *data;
} Image;

Image *current_img = NULL;
Image *undo_stack[MAX_UNDO];
int undo_count = 0;

Ihandle *img_label = NULL;
Ihandle *status_label = NULL;

void free_image(Image *img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

Image *create_image(int w, int h) {
    Image *img = (Image *)malloc(sizeof(Image));
    img->width = w;
    img->height = h;
    img->data = (Pixel *)calloc(w * h, sizeof(Pixel));
    return img;
}

Image *copy_image(Image *src) {
    if (!src) return NULL;
    Image *dst = create_image(src->width, src->height);
    memcpy(dst->data, src->data, src->width * src->height * sizeof(Pixel));
    return dst;
}

void clear_undo_stack(void) {
    while (undo_count > 0) {
        free_image(undo_stack[--undo_count]);
    }
}

void save_undo(void) {
    if (!current_img) return;
    if (undo_count == MAX_UNDO) {
        free_image(undo_stack[0]);
        for (int i = 0; i < MAX_UNDO - 1; i++) {
            undo_stack[i] = undo_stack[i + 1];
        }
        undo_count--;
    }
    undo_stack[undo_count++] = copy_image(current_img);
}

int IupOpen(int *argc, char ***argv) {
    return gtk_init_check(argc, argv) ? 0 : -1;
}

void IupClose(void) {
    clear_undo_stack();
    if (current_img) free_image(current_img);
}

static void on_button_clicked(GtkWidget *widget, gpointer data) {
    Icallback cb = (Icallback)data;
    if (cb && cb(widget) == IUP_CLOSE) {
        gtk_main_quit();
    }
}

Ihandle* IupButton(const char *title, const char *action) {
    return gtk_button_new_with_label(title ? title : "");
}

Ihandle* IupLabel(const char *title) {
    return title ? gtk_label_new(title) : gtk_image_new();
}

Ihandle* IupDialog(Ihandle *child) {
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (child) gtk_container_add(GTK_CONTAINER(win), child);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return win;
}

Ihandle* IupVbox(Ihandle *child, ...) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    if (child) gtk_box_pack_start(GTK_BOX(box), child, FALSE, FALSE, 2);
    va_list args;
    va_start(args, child);
    Ihandle *item;
    while ((item = va_arg(args, Ihandle*)) != NULL) {
        gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 2);
    }
    va_end(args);
    return box;
}

Ihandle* IupHbox(Ihandle *child, ...) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    if (child) gtk_box_pack_start(GTK_BOX(box), child, FALSE, FALSE, 2);
    va_list args;
    va_start(args, child);
    Ihandle *item;
    while ((item = va_arg(args, Ihandle*)) != NULL) {
        gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 2);
    }
    va_end(args);
    return box;
}

void IupSetAttribute(Ihandle *ih, const char *name, const char *value) {
    if (!ih || !name || !value) return;
    if (strcmp(name, "TITLE") == 0) {
        if (GTK_IS_WINDOW(ih)) gtk_window_set_title(GTK_WINDOW(ih), value);
        else if (GTK_IS_LABEL(ih)) gtk_label_set_text(GTK_LABEL(ih), value);
        else if (GTK_IS_BUTTON(ih)) gtk_button_set_label(GTK_BUTTON(ih), value);
    } else if (strcmp(name, "DIALOGTYPE") == 0) {
        g_is_save_dlg = (strcmp(value, "SAVE") == 0);
    }
}

void IupSetCallback(Ihandle *ih, const char *name, Icallback func) {
    if (ih && func && strcmp(name, "ACTION") == 0) {
        g_signal_connect(ih, "clicked", G_CALLBACK(on_button_clicked), (gpointer)func);
    }
}

void IupShowXY(Ihandle *ih, int x, int y) {
    if (ih) gtk_widget_show_all(ih);
}

void IupMainLoop(void) { gtk_main(); }

Ihandle* IupFileDlg(void) {
    g_is_save_dlg = 0;
    return (Ihandle*)1;
}

int IupPopup(Ihandle *ih, int x, int y) {
    GtkFileChooserAction action = g_is_save_dlg ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN;
    const char *btn = g_is_save_dlg ? "_Save" : "_Open";
    GtkWidget *dialog = gtk_file_chooser_dialog_new(g_is_save_dlg ? "Save BMP Image" : "Open BMP Image",
                                                     NULL, action, "_Cancel", GTK_RESPONSE_CANCEL,
                                                     btn, GTK_RESPONSE_ACCEPT, NULL);
    if (g_is_save_dlg) gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            strncpy(g_file_buffer, filename, sizeof(g_file_buffer) - 1);
            g_free(filename);
            g_dialog_status = 0;
        } else g_dialog_status = -1;
    } else g_dialog_status = -1;
    gtk_widget_destroy(dialog);
    return 0;
}

int IupGetInt(Ihandle *ih, const char *name) {
    return (strcmp(name, "STATUS") == 0) ? g_dialog_status : 0;
}

char* IupGetAttribute(Ihandle *ih, const char *name) {
    return (strcmp(name, "VALUE") == 0) ? g_file_buffer : "";
}

void IupMessage(const char *title, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message ? message : "");
    gtk_window_set_title(GTK_WINDOW(dialog), title ? title : "Message");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

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

void refresh_display(void) {
    if (!current_img) return;
    int w = current_img->width;
    int h = current_img->height;

    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);
    if (!pixbuf) return;

    guchar *dest = gdk_pixbuf_get_pixels(pixbuf);
    int stride = gdk_pixbuf_get_rowstride(pixbuf);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            guchar *pixel_ptr = dest + y * stride + x * 3;
            pixel_ptr[0] = current_img->data[y * w + x].r;
            pixel_ptr[1] = current_img->data[y * w + x].g;
            pixel_ptr[2] = current_img->data[y * w + x].b;
        }
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(img_label), pixbuf);
    g_object_unref(pixbuf);

    char status[100];
    snprintf(status, sizeof(status), "Loaded: %dx%d px | Undo steps available: %d", w, h, undo_count);
    IupSetAttribute(status_label, "TITLE", status);
}

unsigned char clamp(int val) {
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (unsigned char)val;
}

void apply_grayscale(void) {
    if (!current_img) return;
    save_undo();
    for (int i = 0; i < current_img->width * current_img->height; i++) {
        Pixel *p = &current_img->data[i];
        unsigned char g = (unsigned char)(0.299 * p->r + 0.587 * p->g + 0.114 * p->b);
        p->r = p->g = p->b = g;
    }
    refresh_display();
}

void apply_invert(void) {
    if (!current_img) return;
    save_undo();
    for (int i = 0; i < current_img->width * current_img->height; i++) {
        Pixel *p = &current_img->data[i];
        p->r = 255 - p->r;
        p->g = 255 - p->g;
        p->b = 255 - p->b;
    }
    refresh_display();
}

void apply_brightness(int val) {
    if (!current_img) return;
    save_undo();
    for (int i = 0; i < current_img->width * current_img->height; i++) {
        Pixel *p = &current_img->data[i];
        p->r = clamp(p->r + val);
        p->g = clamp(p->g + val);
        p->b = clamp(p->b + val);
    }
    refresh_display();
}

void apply_flip_h(void) {
    if (!current_img) return;
    save_undo();
    int w = current_img->width;
    int h = current_img->height;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w / 2; x++) {
            Pixel tmp = current_img->data[y * w + x];
            current_img->data[y * w + x] = current_img->data[y * w + (w - 1 - x)];
            current_img->data[y * w + (w - 1 - x)] = tmp;
        }
    }
    refresh_display();
}

void apply_flip_v(void) {
    if (!current_img) return;
    save_undo();
    int w = current_img->width;
    int h = current_img->height;
    for (int y = 0; y < h / 2; y++) {
        for (int x = 0; x < w; x++) {
            Pixel tmp = current_img->data[y * w + x];
            current_img->data[y * w + x] = current_img->data[(h - 1 - y) * w + x];
            current_img->data[(h - 1 - y) * w + x] = tmp;
        }
    }
    refresh_display();
}

void apply_rotate90(void) {
    if (!current_img) return;
    save_undo();
    int w = current_img->width;
    int h = current_img->height;
    Image *rot = create_image(h, w);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            rot->data[x * h + (h - 1 - y)] = current_img->data[y * w + x];
        }
    }
    free_image(current_img);
    current_img = rot;
    refresh_display();
}

void apply_blur(void) {
    if (!current_img) return;
    save_undo();
    int w = current_img->width;
    int h = current_img->height;
    Image *out = create_image(w, h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        Pixel p = current_img->data[ny * w + nx];
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
    free_image(current_img);
    current_img = out;
    refresh_display();
}

void apply_sharpen(void) {
    if (!current_img) return;
    save_undo();
    int w = current_img->width;
    int h = current_img->height;
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
                        Pixel p = current_img->data[ny * w + nx];
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
    free_image(current_img);
    current_img = out;
    refresh_display();
}

void apply_crop(void) {
    if (!current_img) return;
    save_undo();
    int w = current_img->width;
    int h = current_img->height;
    if (w <= 20 || h <= 20) return;

    int new_w = w * 3 / 4;
    int new_h = h * 3 / 4;
    Image *cropped = create_image(new_w, new_h);

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            cropped->data[y * new_w + x] = current_img->data[y * w + x];
        }
    }
    free_image(current_img);
    current_img = cropped;
    refresh_display();
}

void apply_undo(void) {
    if (undo_count == 0) return;
    if (current_img) free_image(current_img);
    current_img = undo_stack[--undo_count];
    refresh_display();
}

int cb_open(Ihandle *self) {
    Ihandle *filedlg = IupFileDlg();
    IupSetAttribute(filedlg, "DIALOGTYPE", "OPEN");
    IupPopup(filedlg, IUP_CENTER, IUP_CENTER);

    if (IupGetInt(filedlg, "STATUS") != -1) {
        char *filename = IupGetAttribute(filedlg, "VALUE");
        Image *img = load_bmp(filename);
        if (img) {
            clear_undo_stack();
            if (current_img) free_image(current_img);
            current_img = img;
            refresh_display();
        } else {
            IupMessage("Error", "Failed to open BMP file!");
        }
    }
    return IUP_DEFAULT;
}

int cb_save(Ihandle *self) {
    if (!current_img) {
        IupMessage("Error", "No image loaded to save!");
        return IUP_DEFAULT;
    }
    Ihandle *filedlg = IupFileDlg();
    IupSetAttribute(filedlg, "DIALOGTYPE", "SAVE");
    IupPopup(filedlg, IUP_CENTER, IUP_CENTER);

    if (IupGetInt(filedlg, "STATUS") != -1) {
        char *filename = IupGetAttribute(filedlg, "VALUE");
        if (!save_bmp(filename, current_img)) {
            IupMessage("Error", "Failed to save BMP file!");
        } else {
            IupMessage("Success", "Image saved successfully!");
        }
    }
    return IUP_DEFAULT;
}

int cb_exit(Ihandle *self) {
    return IUP_CLOSE;
}

int cb_grayscale(Ihandle *s) { apply_grayscale(); return IUP_DEFAULT; }
int cb_invert(Ihandle *s) { apply_invert(); return IUP_DEFAULT; }
int static int cb_binc(Ihandle *s) {
    if (!current_img) return IUP_DEFAULT;
    int val = 0;
    if (!IupGetParam("Adjust Brightness", NULL, 0, "Value (-255 to 255): %i[-255,255]\n", &val, NULL)) return IUP_DEFAULT;
    save_undo();
    apply_brightness(current_img, val);
    refresh_display();
    return IUP_DEFAULT;
}
int cb_bright_inc(Ihandle *s) {
        int val = 0;
        if (!IupGetParam("Adjust Brightness", NULL, 0, "Value (-255 to 255): %i[-255,255]
", &val, NULL)) return IUP_DEFAULT;
        if (current_img) apply_brightness(current_img, val);
        refresh_display();
        return IUP_DEFAULT;
    }
int cb_fliph(Ihandle *s) { apply_flip_h(); return IUP_DEFAULT; }
int cb_flipv(Ihandle *s) { apply_flip_v(); return IUP_DEFAULT; }
int cb_rot90(Ihandle *s) { apply_rotate90(); return IUP_DEFAULT; }
int cb_blur(Ihandle *s) { apply_blur(); return IUP_DEFAULT; }
int cb_sharp(Ihandle *s) { apply_sharpen(); return IUP_DEFAULT; }
int static int cb_crop(Ihandle *s) {
    if (!current_img) return IUP_DEFAULT;
    int x = 0, y = 0, w = current_img->w / 2, h = current_img->h / 2;
    if (!IupGetParam("Crop Image", NULL, 0, "Start X (px): %i\nStart Y (px): %i\nWidth (px): %i\nHeight (px): %i\n", &x, &y, &w, &h, NULL)) return IUP_DEFAULT;
    save_undo();
    Image *cropped = apply_crop(current_img, x, y, w, h);
    if (cropped) { current_img = cropped; refresh_display(); }
    return IUP_DEFAULT;
}
int cb_undo(Ihandle *s) { apply_undo(); return IUP_DEFAULT; }

int main(int argc, char **argv) {
    IupOpen(&argc, &argv);

    Ihandle *btn_open = IupButton("Open BMP", NULL);
    Ihandle *btn_save = IupButton("Save BMP", NULL);
    Ihandle *btn_undo = IupButton("Undo", NULL);
    Ihandle *btn_quit = IupButton("Quit App", NULL);
    
    Ihandle *btn_gray = IupButton("Grayscale", NULL);
    Ihandle *btn_inv  = IupButton("Invert", NULL);
    Ihandle *btn_binc = IupButton("Bright +25", NULL);
    Ihandle *btn_bdec = IupButton("Bright -25", NULL);
    Ihandle *btn_fliph = IupButton("Flip H", NULL);
    Ihandle *btn_flipv = IupButton("Flip V", NULL);
    Ihandle *btn_rot90 = IupButton("Rotate 90", NULL);
    Ihandle *btn_blur = IupButton("Blur (3x3)", NULL);
    Ihandle *btn_sharp = IupButton("Sharpen", NULL);
    Ihandle *btn_crop = IupButton("Crop", NULL);

    IupSetCallback(btn_open, "ACTION", (Icallback)cb_open);
    IupSetCallback(btn_save, "ACTION", (Icallback)cb_save);
    IupSetCallback(btn_undo, "ACTION", (Icallback)cb_undo);
    IupSetCallback(btn_quit, "ACTION", (Icallback)cb_exit);
    
    IupSetCallback(btn_gray, "ACTION", (Icallback)cb_grayscale);
    IupSetCallback(btn_inv,  "ACTION", (Icallback)cb_invert);
    IupSetCallback(btn_binc, "ACTION", (Icallback)cb_bright_inc);
    IupSetCallback(btn_bdec, "ACTION", (Icallback)cb_bright_dec);
    IupSetCallback(btn_fliph, "ACTION", (Icallback)cb_fliph);
    IupSetCallback(btn_flipv, "ACTION", (Icallback)cb_flipv);
    IupSetCallback(btn_rot90, "ACTION", (Icallback)cb_rot90);
    IupSetCallback(btn_blur, "ACTION", (Icallback)cb_blur);
    IupSetCallback(btn_sharp, "ACTION", (Icallback)cb_sharp);
    IupSetCallback(btn_crop, "ACTION", (Icallback)cb_crop);

    img_label = IupLabel(NULL);
    status_label = IupLabel("No image loaded. Click 'Open BMP'.");

    Ihandle *toolbar = IupHbox(btn_open, btn_save, btn_undo, btn_quit, NULL);
    Ihandle *ops1 = IupHbox(btn_gray, btn_inv, btn_binc, btn_bdec, btn_fliph, btn_flipv, NULL);
    Ihandle *ops2 = IupHbox(btn_rot90, btn_blur, btn_sharp, btn_crop, NULL);

    Ihandle *vbox = IupVbox(toolbar, ops1, ops2, status_label, img_label, NULL);

    Ihandle *dlg = IupDialog(vbox);
    IupSetAttribute(dlg, "TITLE", "BMP Image Editor");

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();

    return 0;
}
