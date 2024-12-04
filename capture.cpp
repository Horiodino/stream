#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to hold pixel data for easier manipulation
typedef struct {
    unsigned char r, g, b, a;
} Pixel;


void capture_window_pixels(Display *disp, Window window);
void save_pixel_data_to_file(XImage *image, const char *filename, int width, int height);

void save_pixel_data_to_file(XImage *image, const char *filename, int width, int height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }

    // Simple PPM (Portable Pixmap) image format
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            
            // Extract color components
            unsigned char r = (pixel >> 16) & 0xFF;
            unsigned char g = (pixel >> 8) & 0xFF;
            unsigned char b = pixel & 0xFF;
            
            // Write RGB values
            fputc(r, file);
            fputc(g, file);
            fputc(b, file);
        }
    }

    fclose(file);
    printf("Pixel data saved to %s\n", filename);
}

void capture_window_pixels(Display *disp, Window window) {
    // Get window attributes to determine size and depth
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(disp, window, &attrs)) {
        fprintf(stderr, "Failed to get window attributes\n");
        return;
    }

    // Capture the entire window image
    XImage *image = XGetImage(disp, window, 
                               0, 0,  // Start from top-left corner
                               attrs.width, attrs.height,  // Full window size
                               AllPlanes,  // Capture all color planes
                               ZPixmap);   // Pixel format
    if (!image) {
        fprintf(stderr, "Failed to capture window image\n");
        return;
    }

    // Process and save pixel data
    printf("Window Size: %d x %d\n", attrs.width, attrs.height);
    printf("Image Depth: %d bits\n", image->depth);

    // Save full image to a file
    save_pixel_data_to_file(image, "window_capture.ppm", attrs.width, attrs.height);

    // Example: Detailed pixel analysis
    printf("Pixel Analysis:\n");
    for (int y = 0; y < 10 && y < attrs.height; y++) {
        for (int x = 0; x < 10 && x < attrs.width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            
            // Extract color components (assumes 24-bit color)
            int red = (pixel >> 16) & 0xFF;
            int green = (pixel >> 8) & 0xFF;
            int blue = pixel & 0xFF;
            
            printf("Pixel at (%d,%d): R=%d, G=%d, B=%d\n", 
                   x, y, red, green, blue);
        }
    }

    // Free the image when done
    XDestroyImage(image);
}

int main() {
    Display *disp = XOpenDisplay(NULL);
    if (!disp)
    {
      fprintf(stderr, "Unable to open X display\n");
      return 1;
    }

    Window root = DefaultRootWindow(disp);
    
    // Example: Capture pixels of a specific window by ID
    Window target_window = 0x3400009;  // Replace with actual window ID
    
    capture_window_pixels(disp, target_window);

    XCloseDisplay(disp);
    return 0;
}