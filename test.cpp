#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

void list_windows(Display *disp, Window root);
void list_windows_by_Pid(Display *disp, Window root, int pid);
void capture_window_pixels(Display *disp, Window window);
void save_pixel_data_to_file(XImage *image, const char *filename, int width, int height);
void ppm_to_png(const char *ppm_filename, const char *png_filename);


void list_windows(Display *disp, Window root)
{
  Window parent, *children;
  unsigned int nchildren;

  if (XQueryTree(disp, root, &root, &parent, &children, &nchildren))
  {
    for (unsigned int i = 0; i < nchildren; i++)
    {
      char *window_name = NULL;

      XFetchName(disp, children[i], &window_name);
      if (window_name)
      {
        printf("Window ID: 0x%lx, Name: %s\n", children[i], window_name);
        XFree(window_name); // free the allocated memory for the window name
      }
      else
      {
        printf("Window ID: 0x%lx, Name: (No name)\n", children[i]);
      }
    }
    XFree(children); // freethe memory for the list of children windows
  }
  else
  {
    fprintf(stderr, "Failed to query the window tree\n");
  }
}

void list_windows_by_Pid(Display *disp, Window root, int pid)
{
  Window parent, *children;
  unsigned int nchildren;

  if (XQueryTree(disp, root, &root, &parent, &children, &nchildren))
  {
    for (unsigned int i = 0; i < nchildren; i++)
    {
      char *window_name = NULL;
      XFetchName(disp, children[i], &window_name);

      if (window_name)
      {
        printf("Checking window ID: 0x%lx, Name: %s\n", children[i],
               window_name);

        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char *prop;

        if (XGetWindowProperty(disp, children[i],
                               XInternAtom(disp, "_NET_WM_PID", False), 0, 1,
                               False, XA_CARDINAL, &actual_type, &actual_format,
                               &nitems, &bytes_after, &prop) == Success &&
            prop)
        {
          int window_pid = *((int *)prop);
          XFree(prop);

          printf("Window ID: 0x%lx has PID: %d\n", children[i], window_pid);

          if (window_pid == pid)
          {
            printf("Found matching window for PID %d: Window ID: 0x%lx, Name: "
                   "%s\n",
                   pid, children[i], window_name);
          }
        }
        else
        {
                    printf("Window ID: 0x%lx does not have a _NET_WM_PID property.\n", children[i]);
        }
        XFree(window_name);
      }
    }
    XFree(children);
  }
  else
  {
    fprintf(stderr, "Failed to query the window tree\n");
  }
}


void capture_window_pixels(Display *disp, Window window) {
    Window target_window = 0;
    XWindowAttributes attributes;

    Window root_return, parent, *children;
    unsigned int nchildren;
    
    if (XQueryTree(disp, window, &root_return, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            if (XGetWindowAttributes(disp, children[i], &attributes)) {
                if (attributes.map_state == IsViewable) {
                    target_window = children[i];
                    break;
                }
            }
        }
        
        if (children) XFree(children);
    }

    if (!target_window) {
        fprintf(stderr, "No suitable window found for capture\n");
        return;
    }

    if (!XGetWindowAttributes(disp, target_window, &attributes)) {
        fprintf(stderr, "Failed to get window attributes\n");
        return;
    }

    XImage *image = XGetImage(disp, target_window, 
                               0, 0, 
                               attributes.width, attributes.height, 
                               AllPlanes, ZPixmap);
    
    if (!image) {
        fprintf(stderr, "Failed to capture window image\n");
        return;
    }

    save_pixel_data_to_file(image, "window_capture.ppm", attributes.width, attributes.height);
}

void save_pixel_data_to_file(XImage *image, const char *filename, int width, int height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }

    fprintf(file, "P6\n%d %d\n255\n", width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned long pixel = image->f.get_pixel(image, x, y);

            unsigned char red = (pixel & image->red_mask) >> 16;
            unsigned char green = (pixel & image->green_mask) >> 8;
            unsigned char blue = (pixel & image->blue_mask);

            fputc(red, file);
            fputc(green, file);
            fputc(blue, file);
        }
    }

    fclose(file);
    printf("Saved pixel data to %s\n", filename);
}

void ppm_to_png(const char *ppm_filename, const char *png_filename) {
    char command[256];
    snprintf(command, sizeof(command), "convert %s %s", ppm_filename, png_filename);
    system(command);
    printf("Converted %s to %s\n", ppm_filename, png_filename);

    remove(ppm_filename);
    printf("Removed %s\n", ppm_filename);
}

  int main()
  {
    Display *disp = XOpenDisplay(NULL);
    if (!disp)
    {
      fprintf(stderr, "Unable to open X display\n");
      return 1;
    }

    Window root = DefaultRootWindow(disp);
    list_windows_by_Pid(disp, root, 5865);

    capture_window_pixels(disp, root);
    ppm_to_png("window_capture.ppm", "window_capture.png");

    XCloseDisplay(disp);

    return 0;
  }
