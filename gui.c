#include <stdio.h>
#include <stdlib.h> /* Required for malloc/free in display_image */
#include <iup.h>

#include "gui.h"
#include "image.h"
#include "operations.h"

/*
    Current image being edited.
*/
Image *current_image = NULL;

/*
    Previous image for one-level Undo.
*/
Image *undo_image = NULL;

/*
    IUP image display.
*/
Ihandle *image_display;

/*
    Main window.
*/
Ihandle *main_dialog;

/*
    Current IUP image.
*/
Ihandle *current_iup_image;

/*
    Function declarations.
*/
void display_image(Image *image);
void save_undo(void);




/*
    ====================================================
    DISPLAY IMAGE
    ====================================================
*/
void display_image(Image *image)
{
    Ihandle *new_iup_image;
    unsigned char *rgb_buffer;
    int total_pixels;
    int i;

    if (image == NULL || image->data == NULL)
        return;

    /* 1. Safely pack the pixels */
    total_pixels = image->width * image->height;
    rgb_buffer = (unsigned char *)malloc((size_t)total_pixels * 3);
    
    if (rgb_buffer == NULL)
    {
        IupMessage("Error", "Memory allocation failed.");
        return;
    }

    for (i = 0; i < total_pixels; i++)
    {
        rgb_buffer[i * 3 + 0] = image->data[i].r;
        rgb_buffer[i * 3 + 1] = image->data[i].g;
        rgb_buffer[i * 3 + 2] = image->data[i].b;
    }

    /* 2. Create the IUP image */
    new_iup_image = IupImageRGB(image->width, image->height, rgb_buffer);
    free(rgb_buffer);

    if (new_iup_image == NULL)
    {
        printf("ERROR: Could not create IUP image.\n");
        return;
    }

    IupSetAttribute(image_display, "IMAGE", NULL);

    if (current_iup_image != NULL)
    {
        IupDestroy(current_iup_image);
    }

    /* 4. Assign the new image */
    current_iup_image = new_iup_image;

    /* Bind directly using the handle, bypassing the string name issue entirely */
    IupSetAttributeHandle(image_display, "IMAGE", current_iup_image);

    /* 5. Clear strict constraints so the label can adapt */
    IupSetAttribute(image_display, "RASTERSIZE", NULL);
    IupSetAttribute(image_display, "MAXSIZE", NULL);
    
    /* 6. Force a full layout refresh */
    IupRefresh(main_dialog);
    IupUpdate(image_display);
}


/*
    ====================================================
    OPEN IMAGE
    ====================================================
*/



/*
    ====================================================
    GRAYSCALE
    ====================================================
*/
int grayscale_callback(Ihandle *self)
{
    if (current_image == NULL)
    {
        IupMessage("Grayscale", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    grayscale_image(current_image);
    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    INVERT
    ====================================================
*/
int invert_callback(Ihandle *self)
{
    if (current_image == NULL)
    {
        IupMessage("Invert", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    invert_image(current_image);
    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    HORIZONTAL FLIP
    ====================================================
*/
int flip_horizontal_callback(Ihandle *self)
{
    if (current_image == NULL)
    {
        IupMessage("Flip Horizontal", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    flip_horizontal(current_image);
    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    VERTICAL FLIP
    ====================================================
*/
int flip_vertical_callback(Ihandle *self)
{
    if (current_image == NULL)
    {
        IupMessage("Flip Vertical", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    flip_vertical(current_image);
    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    BRIGHTNESS
    ====================================================
*/
int brightness_callback(Ihandle *self)
{
    int brightness = 0;

    if (current_image == NULL)
    {
        IupMessage("Brightness", "No image is loaded.");
        return IUP_DEFAULT;
    }

    if (IupGetParam("Brightness Adjustment", NULL, NULL,
                    "Value (-255 to 255): %i[-255,255]\n",
                    &brightness, NULL))
    {
        save_undo();
        adjust_brightness(current_image, brightness);
        display_image(current_image);
    }

    return IUP_DEFAULT;
}


/*
    ====================================================
    ROTATE 90 DEGREES
    ====================================================
*/
int rotate_callback(Ihandle *self)
{
    Image *rotated;

    if (current_image == NULL)
    {
        IupMessage("Rotate", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();

    rotated = rotate_image_90(current_image);
    if (rotated != NULL)
    {
        free_image(current_image);
        current_image = rotated;
        display_image(current_image);
    }
    else
    {
        IupMessage("Error", "Could not rotate image.");
    }

    return IUP_DEFAULT;
}


/*
    ====================================================
    CROP
    ====================================================
*/
int crop_callback(Ihandle *self)
{
    int x = 0;
    int y = 0;
    int w;
    int h;
    Image *cropped;

    if (current_image == NULL)
    {
        IupMessage("Crop", "No image is loaded.");
        return IUP_DEFAULT;
    }

    w = current_image->width;
    h = current_image->height;

    if (IupGetParam("Crop Image", NULL, NULL,
                    "X Coordinate: %i\n"
                    "Y Coordinate: %i\n"
                    "Width: %i\n"
                    "Height: %i\n",
                    &x, &y, &w, &h, NULL))
    {
        cropped = crop_image(current_image, x, y, w, h);
        if (cropped != NULL)
        {
            save_undo();
            free_image(current_image);
            current_image = cropped;
            display_image(current_image);
        }
        else
        {
            IupMessage("Error", "Invalid crop region or parameters out of bounds.");
        }
    }

    return IUP_DEFAULT;
}


/*
    ====================================================
    BLUR
    ====================================================
*/
int blur_callback(Ihandle *self)
{
    if (current_image == NULL)
    {
        IupMessage("Blur", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    blur_image(current_image);
    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    SHARPEN
    ====================================================
*/
int sharpen_callback(Ihandle *self)
{
    if (current_image == NULL)
    {
        IupMessage("Sharpen", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    sharpen_image(current_image);
    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    UNDO
    ====================================================
*/
int undo_callback(Ihandle *self)
{
    Image *temp;

    if (undo_image == NULL)
    {
        IupMessage("Undo", "Nothing to undo.");
        return IUP_DEFAULT;
    }

    temp = current_image;
    current_image = undo_image;
    undo_image = temp;

    free_image(undo_image);
    undo_image = NULL;

    display_image(current_image);

    return IUP_DEFAULT;
}


/*
    ====================================================
    SAVE
    ====================================================
*/
int save_image_callback(Ihandle *self)
{
    Ihandle *file_dialog;
    char *filename;

    if (current_image == NULL)
    {
        IupMessage("Save", "No image is loaded.");
        return IUP_DEFAULT;
    }

    file_dialog = IupFileDlg();

    IupSetAttribute(file_dialog, "DIALOGTYPE", "SAVE");
    IupSetAttribute(file_dialog, "FILTER", "*.bmp");
    IupSetAttribute(file_dialog, "FILTERINFO", "24-bit BMP files");
    IupSetAttribute(file_dialog, "TITLE", "Save Image As");

    IupPopup(file_dialog, IUP_CENTER, IUP_CENTER);

    if (IupGetInt(file_dialog, "STATUS") != -1)
    {
        filename = IupGetAttribute(file_dialog, "VALUE");

        if (save_bmp(filename, current_image))
        {
            IupMessage("Save", "Image saved successfully.");
        }
        else
        {
            IupMessage("Error", "Could not save BMP image to the specified path.");
        }
    }

    IupDestroy(file_dialog);
    return IUP_DEFAULT;
}


/*
    ====================================================
    START GUI
    ====================================================
*/
void start_gui(int argc, char **argv)
{
    image_display = IupLabel(NULL);
    IupSetAttributeHandle(image_display, "IMAGE", current_iup_image);
    IupSetAttribute(image_display, "ALIGNMENT", "ACENTER:ACENTER");
    
    IupSetAttribute(image_display, "EXPAND", "NO");

    /* Horizontal layout with image on left and button sidebar on right */
    layout = IupHbox(image_display, button_panel, NULL);
    IupSetAttribute(layout, "GAP", "15");
    IupSetAttribute(layout, "MARGIN", "15x15");
    IupSetAttribute(layout, "ALIGNMENT", "ACENTER");

    main_dialog = IupDialog(layout);
    IupSetAttribute(main_dialog, "TITLE", "BMP Image Editor");
    IupSetAttribute(main_dialog, "RASTERSIZE", "900x600");

    IupShowXY(main_dialog, IUP_CENTER, IUP_CENTER);

    IupMainLoop();

    free_image(current_image);
    free_image(undo_image);

    if (current_iup_image != NULL)
    {
        IupDestroy(current_iup_image);
    }

    IupDestroy(main_dialog);
    IupClose();
}





int open_image_callback(Ihandle *self)
{
    Ihandle *file_dialog = IupFileDlg();
    IupSetAttribute(file_dialog, "DIALOGTYPE", "OPEN");
    IupSetAttribute(file_dialog, "TITLE", "Select BMP Image");
    IupSetAttribute(file_dialog, "EXTFILTER", "BMP Files (*.bmp)|*.bmp|All Files (*.*)|*.*|");

    IupPopup(file_dialog, IUP_CENTER, IUP_CENTER);

    if (IupGetInt(file_dialog, "STATUS") != -1)
    {
        char *filename = IupGetAttribute(file_dialog, "VALUE");
        if (filename && filename[0] != '\0')
        {
            printf("[GUI] Selected file: %s\n", filename);
            fflush(stdout);

            Image *new_image = load_bmp(filename);
            if (new_image != NULL)
            {
                if (current_image != NULL)
                {
                    free_image(current_image);
                }
                current_image = new_image;
                printf("[SUCCESS] Loaded %s (%dx%d)\n", filename, new_image->width, new_image->height);
                fflush(stdout);

                Ihandle *dlg = IupGetDialog(self);
                if (dlg) IupUpdate(dlg);
                Ihandle *canvas = IupGetHandle("canvas");
                if (canvas) IupUpdate(canvas);
            }
            else
            {
                printf("[ERROR] Failed to load BMP file: %s\n", filename);
                fflush(stdout);
            }
        }
    }
    else
    {
        printf("[GUI] File selection cancelled.\n");
        fflush(stdout);
    }

    IupDestroy(file_dialog);
    return IUP_DEFAULT;
}
