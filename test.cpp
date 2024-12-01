#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

void list_windows(Display *disp, Window root);
void list_windows_by_Pid(Display *disp, Window root, int pid);

void list_windows(Display *disp, Window root) {
    Window parent, *children;
    unsigned int nchildren;

    if (XQueryTree(disp, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            char *window_name = NULL;
            
            XFetchName(disp, children[i], &window_name);
            if (window_name) {
                printf("Window ID: 0x%lx, Name: %s\n", children[i], window_name);
                XFree(window_name);  // free the allocated memory for the window name
            } else {
                printf("Window ID: 0x%lx, Name: (No name)\n", children[i]);
            }
        }
        XFree(children);  // freethe memory for the list of children windows
    } else {
        fprintf(stderr, "Failed to query the window tree\n");
    }
}

void list_windows_by_Pid(Display *disp, Window root, int pid) {
    Window parent, *children;
    unsigned int nchildren;

    if (XQueryTree(disp, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            char *window_name = NULL;
            XFetchName(disp, children[i], &window_name);

            if (window_name) {
                printf("Checking window ID: 0x%lx, Name: %s\n", children[i], window_name);

                Atom actual_type;
                int actual_format;
                unsigned long nitems, bytes_after;
                unsigned char *prop;

                if (XGetWindowProperty(disp, children[i], XInternAtom(disp, "_NET_WM_PID", False), 0, 1, False, XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success && prop) {
                    int window_pid = *((int *)prop);
                    XFree(prop);
                    
                    printf("Window ID: 0x%lx has PID: %d\n", children[i], window_pid);
                    
                    if (window_pid == pid) {
                        printf("Found matching window for PID %d: Window ID: 0x%lx, Name: %s\n", pid, children[i], window_name);
                    }
                } else {
                    printf("Window ID: 0x%lx does not have a _N
                    
                    ET_WM_PID property.\n", children[i]);
                }
                XFree(window_name);
            }
        }
        XFree(children);
    } else {
        fprintf(stderr, "Failed to query the window tree\n");
    }
}

int main() {
    Display *disp = XOpenDisplay(NULL);
    if (!disp) {
        fprintf(stderr, "Unable to open X display\n");
        return 1;
    }

    Window root = DefaultRootWindow(disp);
    
    // list_windows(disp, root);

    list_windows_by_Pid(disp, root, 5865);

    XCloseDisplay(disp);

    return 0;
}
