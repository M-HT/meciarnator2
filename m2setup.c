/**
 *
 *  Copyright (C) 2014 Roman Pauer
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>


struct _select_data {
    uint32_t width, x, numy;
    uint32_t y[3];
};

struct _select_data select_data[3] = {
    {182, 108, 3, { 58,  77, 96}},
    {140,  13, 2, {126, 144    }},
    {135, 174, 2, {126, 144    }}
};

uint8_t cfg_data[3];
uint8_t image_palette[768];
uint8_t image_data[64000];
SDL_Surface *screen;


static void invert_rect_focused(uint32_t group) {
    uint32_t value;
    uint32_t x, y;

    value = cfg_data[group];
    for (y = 0; y < 18; y++) {
        uint32_t offset;

        offset = (select_data[group].y[value] + y) * 320 + select_data[group].x;
        for (x = 0; x < select_data[group].width; x++) {
            image_data[offset + x] ^= 0x1f;
        }
    }
}

static void invert_rect_unfocused(uint32_t group) {
    uint32_t value, offset1, offset2, offset3;
    uint32_t x, y;

    value = cfg_data[group];
    offset1 = (select_data[group].y[value]) * 320 + select_data[group].x;
    offset2 = offset1 + 18*320;

    for (x = 0; x < select_data[group].width; x++) {
        image_data[offset1 + x] ^= 0x1f;
        image_data[offset2 + x + 1] ^= 0x1f;
    }

    offset3 = offset1 + select_data[group].width;

    for (y = 0; y < 17; y++) {
        image_data[y * 320 + offset1 + 320] ^= 0x1f;
        image_data[y * 320 + offset3] ^= 0x1f;
    }
}

static void display_screen(uint32_t *palette) {
    uint32_t *pixels;
    uint32_t index;

    SDL_LockSurface(screen);
    pixels = (uint32_t *) (screen->pixels);

    for (index = 0; index < 64000; index ++) {
        pixels[index] = palette[image_data[index]];
    }

    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
    SDL_Delay(16);
}

static void palette_fadein(void) {
    uint32_t palette[256];
    uint32_t i, index;

    for (i = 0; i < 64; i++) {
        for (index = 0; index < 256; index++) {
            palette[index] = 256 * 256 * ((((uint32_t)(image_palette[3*index    ])) * i) / 63)
                           + 256 *       ((((uint32_t)(image_palette[3*index + 1])) * i) / 63)
                           +             ((((uint32_t)(image_palette[3*index + 2])) * i) / 63);

        }
        display_screen(palette);
    }
}

static void palette_fadeout(void) {
    uint32_t palette[256];
    int32_t i;
    uint32_t index;

    for (i = 62; i >= 0; i--) {
        for (index = 0; index < 256; index++) {
            palette[index] = 256 * 256 * ((((uint32_t)(image_palette[3*index    ])) * i) / 63)
                           + 256 *       ((((uint32_t)(image_palette[3*index + 1])) * i) / 63)
                           +             ((((uint32_t)(image_palette[3*index + 2])) * i) / 63);

        }
        display_screen(palette);
    }
}

int main(int argc, char *argv[]) {
    const char *cfg_name;
    uint8_t image_packed[18787];
    uint32_t palette[256];
    uint32_t active_rect;
    int quit;
    SDL_Event event;

    cfg_name = NULL;

    // load cfg
    {
        FILE *f;

        memset(cfg_data, 0, 3);

        f = fopen("m2.cfg", "rb");
        if (f != NULL) {
            cfg_name = "m2.cfg";
        } else {
            f = fopen("M2.CFG", "rb");
            if (f != NULL) {
                cfg_name = "M2.CFG";
            }
        }

        if (f != NULL) {
            fread(cfg_data, 1, 3, f);
            fclose(f);

            cfg_data[1] >>= 4;
        } else {
            fprintf(stderr, "Config file doesn't exist");
        }
    }

    // load image
    {
        FILE *f;

        f = fopen("m2.001", "rb");
        if (f != NULL) {
            if (cfg_name == NULL) cfg_name = "m2.cfg";
        } else {
            f = fopen("M2.001", "rb");
            if (f != NULL) {
                if (cfg_name == NULL) cfg_name = "M2.CFG";
            } else {
                fprintf(stderr, "Data file doesn't exist");
                exit(1);
            }
        }

        fseek(f, -19555, SEEK_END);
        fread(image_packed, 1, 18787, f);
        fread(image_palette, 1, 768, f);
        fclose(f);
    }

    // palette
    {
        uint32_t index;

        for (index = 0; index < 768; index++) {
            image_palette[index] = (uint8_t)((image_palette[index] << 2) | (image_palette[index] >> 4));
        }

        for (index = 0; index < 256; index++) {
            palette[index] = 256 * 256 * (uint32_t)(image_palette[3*index]) + 256 * (uint32_t)(image_palette[3*index + 1]) + (uint32_t)(image_palette[3*index + 2]);
        }
    }

    // decode image
    {
        uint8_t temp_data[4096];
        uint32_t temp_offset, buffer_value, src_offset, dst_offset;

        memset(temp_data, 32, 4096);
        temp_offset = 4078;
        buffer_value = 0;
        src_offset = 0;
        dst_offset = 0;

        while (1) {
            buffer_value >>= 1;
            if ((buffer_value & 0x100) == 0) {
                if (src_offset >= 18787) break;
                buffer_value = (uint32_t)(image_packed[src_offset++]) | 0xff00;
            }

            if (buffer_value & 1) {
                uint8_t value1;

                if (src_offset >= 18787) break;
                value1 = image_packed[src_offset++];

                image_data[dst_offset++] = value1;
                temp_data[temp_offset] = value1;
                temp_offset = (temp_offset + 1) & 0xfff;
            } else {
                uint8_t b1, b2;
                uint32_t offset, count, index;

                if (src_offset >= (18787 - 1)) break;
                b1 = image_packed[src_offset++];
                b2 = image_packed[src_offset++];

                offset = b1 | (((uint32_t)b2 & 0xf0) << 4);
                count = (b2 & 0x0f) + 2;

                for (index = 0; index <= count; index++) {
                    uint8_t value2;

                    value2 = temp_data[(offset + index) & 0xfff];

                    image_data[dst_offset++] = value2;
                    temp_data[temp_offset] = value2;
                    temp_offset = (temp_offset + 1) & 0xfff;
                }
            }
        };
    }


    // SDL
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
            exit(2);
        }

        atexit(&SDL_Quit);

        screen = SDL_SetVideoMode(320, 200, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
        if (screen == NULL) {
            fprintf(stderr, "Unable to create SDL screen: %s\n", SDL_GetError());
            exit(3);
        }
        SDL_ShowCursor(SDL_DISABLE);
    }

    active_rect = 0;
    invert_rect_focused(0);
    invert_rect_unfocused(1);
    invert_rect_unfocused(2);


    // fade in
    palette_fadein();

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    quit = 0;
    while (!quit && SDL_WaitEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_TAB:
                        invert_rect_focused(active_rect);
                        invert_rect_unfocused(active_rect);
                        active_rect = (active_rect == 2)?0:(active_rect +1 );
                        invert_rect_unfocused(active_rect);
                        invert_rect_focused(active_rect);
                        display_screen(palette);
                        break;
                    case SDLK_DOWN:
                        invert_rect_focused(active_rect);
                        cfg_data[active_rect]++;
                        if (cfg_data[active_rect] == select_data[active_rect].numy) cfg_data[active_rect] = 0;
                        invert_rect_focused(active_rect);
                        display_screen(palette);
                        break;
                    case SDLK_UP:
                        invert_rect_focused(active_rect);
                        if (cfg_data[active_rect] == 0) cfg_data[active_rect] = (uint8_t)(select_data[active_rect].numy);
                        cfg_data[active_rect]--;
                        invert_rect_focused(active_rect);
                        display_screen(palette);
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        // save cfg
                        {
                            FILE *f;

                            f = fopen(cfg_name, "wb");
                            if (f != NULL) {
                                cfg_data[1] <<= 4;

                                fwrite(cfg_data, 1, 3, f);
                                fclose(f);
                            } else {
                                fprintf(stderr, "Unable to open file %s for writing\n", cfg_name);
                                exit(4);
                            }
                        }
                        palette_fadeout();
                        quit = 1;
                        break;
                    case SDLK_ESCAPE:
                        palette_fadeout();
                        quit = 1;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            default:
                break;
        };
    };


    exit(0);

    return 0;
}

