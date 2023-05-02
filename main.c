#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <X11/Xlib.h>

#define abs(v) (v?v*-1:v)

#define HEIGHT 1000
#define WIDTH 1000

typedef struct Blob {
    float x;
    float y;
    float r;
} Blob;

Blob blobs[2] = {
    {0, 0, 100},
    {WIDTH/2 + WIDTH/4, HEIGHT/2, 110}
};

int8_t in_bounds(int32_t, int32_t, int64_t, int64_t);
void gc_put_pixel(void *, int32_t, int32_t, uint32_t);
void update(Display *, GC *, Window *, XImage *);

uint32_t decodeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) + (g << 8) + b;
}

#define BLOB_COUNT sizeof(blobs)/sizeof(blobs[0])
float sum;
int sqrdist;
int dx;
int dy;

void draw(void *memory)
{
    
    for(int j = 0; j < HEIGHT; ++j) {
        for(int i = 0; i < WIDTH; ++i) {
            sum = 0;
            for(uint32_t b = 0; b < BLOB_COUNT; ++b) {
                dx = blobs[b].x - i;
                dy = blobs[b].y - j;
                sqrdist = dx*dx + dy*dy;
                sum += 400 * blobs[b].r * blobs[b].r / sqrdist;
            }
            if(sum > 255) sum = 255;
            gc_put_pixel(memory, i, j, decodeRGB((int)sum%64, (int)sum%128, (int)sum%256));
        }
    }
}

int8_t exitloop = 0;

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    int screen = DefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0
    );

    char *memory = (char *) malloc(sizeof(uint32_t)*HEIGHT*WIDTH);

    XWindowAttributes winattr = {0};
    XGetWindowAttributes(display, window, &winattr);

    XImage *image = XCreateImage(
        display, winattr.visual, winattr.depth,
        ZPixmap, 0, memory,
        WIDTH, HEIGHT,
        32, WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);

    Mask mouse = PointerMotionMask;
    XSelectInput(display, window, ExposureMask | mouse);

    XMapWindow(display, window);
    XSync(display, False);

    XEvent event;

    update(display, &graphics, &window, image);

    while(!exitloop) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            switch(event.type) {
                case Expose: {
                    update(display, &graphics, &window, image);
                    break;
                }
                case ClientMessage: {
                    if((Atom) event.xclient.data.l[0] == delete_window) {
                        exitloop = 1;   
                    }
                    break;
                }
                case MotionNotify: {
                    blobs[0].x = event.xmotion.x;
                    blobs[0].y = event.xmotion.y;
                    draw(memory);
                    update(display, &graphics, &window, image);
                    break;
                }
            }
        }
    }


    XCloseDisplay(display);

    free(memory);

    return 0;
}

void update(Display *display, GC *graphics, Window *window, XImage *image)
{
    XPutImage(
        display,
        *window,
        *graphics,
        image,
        0, 0,
        0, 0,
        WIDTH, HEIGHT
    );

    XSync(display, False);
}

int8_t in_bounds(int32_t x, int32_t y, int64_t w, int64_t h)
{
    return (x >= 0 && x < w && y >= 0 && y < h);
}

void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color)
{
    if(in_bounds(x, y, WIDTH, HEIGHT))
        *((uint32_t *) memory + y * WIDTH + x) = color;
}


