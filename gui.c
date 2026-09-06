#include "gui.h"

static Image *current_img = NULL;
static Image *undo_stack[MAX_UNDO];
static int undo_count = 0;

static Ihandle *img_label = NULL;
static Ihandle *status_label = NULL;

static void clear_undo_stack(void) {
    while (undo_count > 0) {
        free_image(undo_stack[--undo_count]);
    }
}

static void save_undo(void) {
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

static void refresh_display(void) {
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

    char status[128];
    snprintf(status, sizeof(status), "Loaded: %dx%d px | Undo steps available: %d", w, h, undo_count);
    gtk_label_set_text(GTK_LABEL(status_label), status);
}

static void show_error(const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), "Error");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void show_info(const char *title, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int check_image_loaded(void) {
    if (!current_img || !current_img->data) {
        show_error("No image loaded! Please open a 24-bit BMP image first.");
        return 0;
    }
    return 1;
}

static int cb_open(Ihandle *self) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open BMP Image", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            Image *img = load_bmp(filename);
            if (img) {
                clear_undo_stack();
                if (current_img) free_image(current_img);
                current_img = img;
                refresh_display();
            } else {
                show_error("Failed to open file! Please ensure it is a valid 24-bit uncompressed BMP image.");
            }
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
    return IUP_DEFAULT;
}

static int cb_save(Ihandle *self) {
    if (!check_image_loaded()) return IUP_DEFAULT;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save BMP Image", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            if (save_bmp(filename, current_img)) show_info("Success", "BMP Image saved successfully!");
            else show_error("Failed to save image file!");
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
    return IUP_DEFAULT;
}

static int cb_grayscale(Ihandle *s) { if (!check_image_loaded()) return IUP_DEFAULT; save_undo(); apply_grayscale(current_img); refresh_display(); return IUP_DEFAULT; }
static int cb_invert(Ihandle *s) { if (!check_image_loaded()) return IUP_DEFAULT; save_undo(); apply_invert(current_img); refresh_display(); return IUP_DEFAULT; }
static int cb_binc(Ihandle *s) { if (!check_image_loaded()) return IUP_DEFAULT; save_undo(); apply_brightness(current_img, 25); refresh_display(); return IUP_DEFAULT; }
static int cb_bdec(Ihandle *s) { if (!check_image_loaded()) return IUP_DEFAULT; save_undo(); apply_brightness(current_img, -25); refresh_display(); return IUP_DEFAULT; }
static int cb_fliph(Ihandle *s) { if (!check_image_loaded()) return IUP_DEFAULT; save_undo(); apply_flip_h(current_img); refresh_display(); return IUP_DEFAULT; }
static int cb_flipv(Ihandle *s) { if (!check_image_loaded()) return IUP_DEFAULT; save_undo(); apply_flip_v(current_img); refresh_display(); return IUP_DEFAULT; }

static int cb_rot90(Ihandle *s) {
    if (!check_image_loaded()) return IUP_DEFAULT;
    save_undo();
    Image *rot = apply_rotate90(current_img);
    if (rot) { free_image(current_img); current_img = rot; refresh_display(); }
    return IUP_DEFAULT;
}

static int cb_blur(Ihandle *s) {
    if (!check_image_loaded()) return IUP_DEFAULT;
    save_undo();
    Image *out = apply_blur(current_img);
    if (out) { free_image(current_img); current_img = out; refresh_display(); }
    return IUP_DEFAULT;
}

static int cb_sharp(Ihandle *s) {
    if (!check_image_loaded()) return IUP_DEFAULT;
    save_undo();
    Image *out = apply_sharpen(current_img);
    if (out) { free_image(current_img); current_img = out; refresh_display(); }
    return IUP_DEFAULT;
}

static int cb_crop(Ihandle *s) {
    if (!check_image_loaded()) return IUP_DEFAULT;
    int x = 0, y = 0, w = current_img->w / 2, h = current_img->h / 2;
    if (!IupGetParam("Crop Image", NULL, 0,
                     "Start X (px): %i
Start Y (px): %i
Width (px): %i
Height (px): %i
",
                     &x, &y, &w, &h, NULL)) return IUP_DEFAULT;
    save_undo();
    Image *cropped = apply_crop(current_img, x, y, w, h);
    if (cropped) {
        current_img = cropped;
        refresh_display();
    } else {
        show_error("Invalid crop dimensions!");
    }
    return IUP_DEFAULT;
}
    save_undo();
    free_image(current_img);
    current_img = cropped;
    refresh_display();
    return IUP_DEFAULT;
}

static int cb_undo(Ihandle *s) {
    if (undo_count == 0) { show_info("Undo", "No previous state to restore."); return IUP_DEFAULT; }
    if (current_img) free_image(current_img);
    current_img = undo_stack[--undo_count];
    refresh_display();
    return IUP_DEFAULT;
}

static int cb_exit(Ihandle *s) { gtk_main_quit(); return IUP_CLOSE; }

void setup_and_run_gui(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "BMP Image Editor");

    GtkWidget *btn_open = gtk_button_new_with_label("Open BMP");
    GtkWidget *btn_save = gtk_button_new_with_label("Save BMP");
    GtkWidget *btn_undo = gtk_button_new_with_label("Undo");
    GtkWidget *btn_quit = gtk_button_new_with_label("Quit App");

    GtkWidget *btn_gray = gtk_button_new_with_label("Grayscale");
    GtkWidget *btn_inv  = gtk_button_new_with_label("Invert");
    GtkWidget *btn_binc = gtk_button_new_with_label("Bright +25");
    GtkWidget *btn_bdec = gtk_button_new_with_label("Bright -25");
    GtkWidget *btn_fliph = gtk_button_new_with_label("Flip H");
    GtkWidget *btn_flipv = gtk_button_new_with_label("Flip V");
    GtkWidget *btn_rot90 = gtk_button_new_with_label("Rotate 90");
    GtkWidget *btn_blur = gtk_button_new_with_label("Blur (3x3)");
    GtkWidget *btn_sharp = gtk_button_new_with_label("Sharpen");
    GtkWidget *btn_crop = gtk_button_new_with_label("Crop (80%)");

    g_signal_connect(btn_open, "clicked", G_CALLBACK(cb_open), NULL);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(cb_save), NULL);
    g_signal_connect(btn_undo, "clicked", G_CALLBACK(cb_undo), NULL);
    g_signal_connect(btn_quit, "clicked", G_CALLBACK(cb_exit), NULL);

    g_signal_connect(btn_gray, "clicked", G_CALLBACK(cb_grayscale), NULL);
    g_signal_connect(btn_inv,  "clicked", G_CALLBACK(cb_invert), NULL);
    g_signal_connect(btn_binc, "clicked", G_CALLBACK(cb_binc), NULL);
    g_signal_connect(btn_bdec, "clicked", G_CALLBACK(cb_bdec), NULL);
    g_signal_connect(btn_fliph, "clicked", G_CALLBACK(cb_fliph), NULL);
    g_signal_connect(btn_flipv, "clicked", G_CALLBACK(cb_flipv), NULL);
    g_signal_connect(btn_rot90, "clicked", G_CALLBACK(cb_rot90), NULL);
    g_signal_connect(btn_blur, "clicked", G_CALLBACK(cb_blur), NULL);
    g_signal_connect(btn_sharp, "clicked", G_CALLBACK(cb_sharp), NULL);
    g_signal_connect(btn_crop, "clicked", G_CALLBACK(cb_crop), NULL);

    img_label = gtk_image_new();
    status_label = gtk_label_new("No image loaded. Click 'Open BMP' to start.");

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_open, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_undo, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_quit, FALSE, FALSE, 2);

    GtkWidget *ops1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(ops1), btn_gray, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops1), btn_inv, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops1), btn_binc, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops1), btn_bdec, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops1), btn_fliph, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops1), btn_flipv, FALSE, FALSE, 2);

    GtkWidget *ops2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(ops2), btn_rot90, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops2), btn_blur, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops2), btn_sharp, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ops2), btn_crop, FALSE, FALSE, 2);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), ops1, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), ops2, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), img_label, TRUE, TRUE, 2);

    gtk_container_add(GTK_CONTAINER(win), vbox);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(win);
    gtk_main();

    clear_undo_stack();
    if (current_img) free_image(current_img);
}
