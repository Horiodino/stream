#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/Xlib-xcb.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xcomposite.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <bits/algorithmfwd.h>
#include <chrono>

using namespace std;

struct DisplayInfo
{
    Display *display;
    Window root;
    int pid;
    std::vector<Window> child;
};

class Timer
{
private:
    std::chrono::high_resolution_clock::time_point start;

public:
    Timer()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    void reset()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    float elapsed()
    {
        std::chrono::duration<float> duration = std::chrono::high_resolution_clock::now() - start;
        return duration.count();
    }
};

void ListDisplayInfo(Display *disp, Window root);
std::vector<Window> ListWindowsByPid(Display *disp, Window root, int pid);
XImage *CaptureWindowPixels(Display *disp, Window window);
void SavePixelDataToFile(XImage *image, const char *filename, int width, int height);
void PpmToPng(const char *ppm_filename, const char *png_filename);
void PicturetoVideo(string path);
void SavePixelFormat(XImage *img, string path);

int i = 0;

struct WindowInfo
{
    Window window;
    int pid;
    std::string title;
};

class AllDisplays
{
public:
    AllDisplays()
    {
        display = XOpenDisplay(NULL);
        if (!display)
        {
            throw std::runtime_error("Unable to open X display");
        }

        Window root = DefaultRootWindow(display);
        Atom clientListAtom = XInternAtom(display, "_NET_CLIENT_LIST", False);
        Atom actualType;
        int actualFormat;
        unsigned long nItems, bytesAfter;
        unsigned char *prop = NULL;

        if (XGetWindowProperty(display, root, clientListAtom, 0, ~0L, False, XA_WINDOW,
                               &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success)
        {
            Window *windows = reinterpret_cast<Window *>(prop);
            for (unsigned long i = 0; i < nItems; ++i)
            {
                std::string title = GetWindowTitle(display, windows[i]);
                int pid = GetWindowPID(display, windows[i]);
                WindowInfo info{windows[i], pid, title};
                m_windows.push_back(info);
            }

            if (prop)
                XFree(prop);
        }
    }
    ~AllDisplays()
    {
        if (display)
        {
            XCloseDisplay(display);
        }
    }

    const std::vector<WindowInfo> &GetWindows() const
    {
        return m_windows;
    }

private:
    Display *display;
    std::string GetWindowTitle(Display *display, Window window)
    {
        std::string title;

        //  _NET_WM_VISIBLE_NAME first
        Atom netVisibleNameAtom = XInternAtom(display, "_NET_WM_VISIBLE_NAME", False);
        Atom utf8StringAtom = XInternAtom(display, "UTF8_STRING", False);

        Atom actualType;
        int actualFormat;
        unsigned long nItems, bytesAfter;
        unsigned char *prop = NULL;

        if (XGetWindowProperty(display, window, netVisibleNameAtom, 0, 1024, False, utf8StringAtom,
                               &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success)
        {
            if (prop)
            {
                title = reinterpret_cast<char *>(prop);
                XFree(prop);
            }
        }

        if (title.empty())
        {
            //  _NET_WM_NAME
            Atom netWmNameAtom = XInternAtom(display, "_NET_WM_NAME", False);
            if (XGetWindowProperty(display, window, netWmNameAtom, 0, 1024, False, utf8StringAtom,
                                   &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success)
            {
                if (prop)
                {
                    title = reinterpret_cast<char *>(prop);
                    XFree(prop);
                }
            }
        }

        //  fallback to WM_NAME
        if (title.empty())
        {
            XTextProperty textProp;
            char **list = NULL;
            int count = 0;

            if (XGetWMName(display, window, &textProp) && textProp.value)
            {
                if (XTextPropertyToStringList(&textProp, &list, &count) && count > 0)
                {
                    title = list[0];
                    XFreeStringList(list);
                }
                XFree(textProp.value);
            }
        }

        return title;
    }

    int GetWindowPID(Display *display, Window window)
    {
        Atom pidAtom = XInternAtom(display, "_NET_WM_PID", False);
        Atom actualType;
        int actualFormat;
        unsigned long nItems, bytesAfter;
        unsigned char *prop = NULL;
        int pid = -1;

        if (XGetWindowProperty(display, window, pidAtom, 0, 1, False, XA_CARDINAL,
                               &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success)
        {
            if (prop)
            {
                pid = *reinterpret_cast<int *>(prop);
                XFree(prop);
            }
        }

        return pid;
    }

    std::vector<WindowInfo> m_windows;
};

void ListDisplayInfo(Display *disp, Window root)
{
    Window parent, *children;
    unsigned int nchildren;
    if (XQueryTree(disp, root, &root, &parent, &children, &nchildren))
    {

        if (parent != None)
        {
            char *parent_name = NULL;
            XFetchName(disp, parent, &parent_name);
            if (parent_name)
            {
                printf("\033[1;31m");
                printf("Parent Window ID: 0x%lx, Name: %s\n", parent, parent_name);
                printf("\033[0m");
                XFree(parent_name);
            }
        }
        for (unsigned int i = 0; i < nchildren; i++)
        {
            char *window_name = NULL;
            XFetchName(disp, children[i], &window_name);
            if (window_name)
            {
                printf("Checking window ID: 0x%lx, Name: %s\n", children[i], window_name);

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

std::vector<Window> ListWindowsByPid(Display *disp, Window root, int pid)
{
    std::vector<Window> windows;
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
                        windows.push_back(children[i]);
                    }
                }
                else
                {
                    printf("Window ID: 0x%lx does not have a _NET_WM_PID property.\n",
                           children[i]);
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
    return windows;
}

XImage *CaptureWindowPixels(Display *disp, Window window)
{
    if (!disp)
    {
        fprintf(stderr, "Invalid display\n");
        return nullptr;
    }

    if (!window)
    {
        fprintf(stderr, "Invalid window\n");
        return nullptr;
    }

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(disp, window, &attrs))
    {
        fprintf(stderr, "Failed to get window attributes\n");
        return nullptr;
    }

    if (attrs.map_state != IsViewable)
    {
        fprintf(stderr, "Window is not viewable\n");
        return nullptr;
    }

    XImage *image = XGetImage(disp, window, 0, 0, attrs.width, attrs.height, AllPlanes, ZPixmap);
    if (!image)
    {
        fprintf(stderr, "Failed to capture window image\n");
        return nullptr;
    }

    return image;
}

XImage *CaptureRootAlways(Display *disp, Window window)
{
    Window root = DefaultRootWindow(disp);
    XWindowAttributes attrs;

    int i = 0;
    while (window)
    {

        if (!XGetWindowAttributes(disp, window, &attrs))
        {
            return nullptr;
        }

        int x, y;
        Window child;
        XTranslateCoordinates(disp, window, root, 0, 0, &x, &y, &child);
        XImage *img = XGetImage(disp, root, x, y, attrs.width, attrs.height, AllPlanes, ZPixmap);
        if (img)
        {
            SavePixelFormat(img, "window_capture/");
            XDestroyImage(img);
        }
        i++;
    }

    return nullptr;
}

void PrintWindowDetails(Display *disp, Window window)
{
    XWindowAttributes attrs;
    if (XGetWindowAttributes(disp, window, &attrs))
    {
        printf("Window 0x%lx Details:\n", window);
        printf("  Width: %d, Height: %d\n", attrs.width, attrs.height);
        printf("  X: %d, Y: %d\n", attrs.x, attrs.y);
        printf("  Depth: %d\n", attrs.depth);
        printf("  Map State: %s\n",
               attrs.map_state == IsUnmapped ? "Unmapped" : attrs.map_state == IsViewable ? "Viewable"
                                                        : attrs.map_state == IsUnviewable ? "Unviewable"
                                                                                          : "Unknown");

        if (attrs.visual)
        {
            printf("  Visual Class: %d\n", attrs.visual->c_class);
        }
    }
}

void SavePixelDataToFile(XImage *image, const char *filename, int width, int height)
{
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }

    fprintf(file, "P6\n%d %d\n255\n", width, height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
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

void PpmToPng(const char *ppm_filename, const char *png_filename)
{
    char command[256];
    snprintf(command, sizeof(command), "convert %s %s", ppm_filename,
             png_filename);
    system(command);
    printf("Converted %s to %s\n", ppm_filename, png_filename);

    remove(ppm_filename);
    printf("Removed %s\n", ppm_filename);
}

void PicturetoVideo(string path)
{
    char count_command[256];
    snprintf(count_command, sizeof(count_command),
             "ls -1 %s*.jpeg | wc -l", path.c_str());

    FILE *pipe = popen(count_command, "r");
    char buffer[128];
    fgets(buffer, sizeof(buffer), pipe);
    int image_count = atoi(buffer);
    pclose(pipe);

    int framerate = 30; // default
    if (image_count < 30)
    {
        framerate = 10;
    }
    else if (image_count > 100)
    {
        framerate = 60;
    }

    char command[256];
    snprintf(command, sizeof(command),
             "ffmpeg -framerate %d -i %s%%d.jpeg -c:v libx264 -pix_fmt yuv420p -y %soutput.mp4",
             framerate, path.c_str(), path.c_str());
    system(command);

    printf("Converted jpeg files from %s to %soutput.mp4 at %d fps\n",
           path.c_str(), path.c_str(), framerate);
}

void SavePixelFormat(XImage *img, string path)
{
    static int frame_count = 0;
    string filename = path + "pixel_data_" + std::to_string(frame_count++) + ".bin";
    FILE *file = fopen(filename.c_str(), "wb");

    if (!file)
    {
        fprintf(stderr, "Failed to open file for pixel data\n");
        return;
    }

    fwrite(&img->width, sizeof(int), 1, file);
    fwrite(&img->height, sizeof(int), 1, file);
    fwrite(&img->bytes_per_line, sizeof(int), 1, file);

    size_t data_size = static_cast<size_t>(img->height) * static_cast<size_t>(img->bytes_per_line);
    fwrite(img->data, 1, data_size, file);

    fclose(file);
}

void PixelFormatToVideo(string path)
{
    string raw_file = path + "frames.raw";
    FILE *raw = fopen(raw_file.c_str(), "wb");
    if (!raw)
        return;

    int frame_count = 0;
    int width = 0, height = 0;
    int bytes_per_line = 0;

    while (true)
    {
        string frame_file = path + "pixel_data_" + std::to_string(frame_count) + ".bin";
        FILE *frame = fopen(frame_file.c_str(), "rb");
        if (!frame)
            break;

        // header information
        fread(&width, sizeof(int), 1, frame);
        fread(&height, sizeof(int), 1, frame);
        fread(&bytes_per_line, sizeof(int), 1, frame);

        // Read and write frame data
        unsigned char *buffer = new unsigned char[height * bytes_per_line];
        size_t expected_size = static_cast<size_t>(height) * static_cast<size_t>(bytes_per_line);
        if (fread(buffer, 1, expected_size, frame) == expected_size)
        {
            fwrite(buffer, 1, height * bytes_per_line, raw);
        }

        delete[] buffer;
        fclose(frame);
        remove(frame_file.c_str()); // clean up individual frame files
        frame_count++;
    }

    fclose(raw);

    if (frame_count == 0)
        return;

    char command[512];
    snprintf(command, sizeof(command),
             "ffmpeg -f rawvideo -pixel_format bgra"
             " -video_size %dx%d -framerate 30"
             " -i \"%s\""
             " -c:v libx264 -pix_fmt yuv420p -y \"%soutput.mp4\"",
             width, height, raw_file.c_str(), path.c_str());

    system(command);
    remove(raw_file.c_str());
    printf("Created video from %d frames at %s\n", frame_count, path.c_str());
}

int main()
{

    try
    {
        Timer timer_create;

        Display *display = XOpenDisplay(NULL);
        if (!display)
        {
            fprintf(stderr, "Failed to open X display\n");
            return 1;
        }

        Window root = DefaultRootWindow(display);
        CaptureRootAlways(display, root);
        float elapsed = timer_create.elapsed();
        FILE *time_file = fopen("window_capture/capture_time.txt", "w");
        if (time_file)
        {
            fprintf(time_file, "Capture time: %.2f seconds\n", elapsed);
            fclose(time_file);
        }
        XCloseDisplay(display);
        PixelFormatToVideo("window_capture/");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
 
    return 0;
}