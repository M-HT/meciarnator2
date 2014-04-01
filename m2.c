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
#include <SDL/SDL_mixer.h>
#if defined(PANDORA)
    #include <unistd.h>
#endif

struct image_data {
    uint16_t x, y, width, height;
    uint8_t palette[768];
    uint8_t *data;
};

struct action {
    uint16_t time;
    uint8_t image_number;
    uint8_t action_type;
};

uint32_t screen_palette[256];
uint32_t screen_start;
uint8_t screen_data[4*65536];
SDL_Surface *screen;

uint8_t *main_image_data;
uint8_t main_image_palette[768];

uint8_t *sample_data;
uint32_t sample_data_length;
int have_audio;

uint32_t video_memory_address, current_image_palette, split_scanline;

uint64_t next_action_time, finish_time;
uint32_t next_action;

struct image_data images[32];

int quit;

uint32_t images_sizes[32][2] = {
    {0x1307, 0x6DC3},
    {0x2F67, 0xFD08},
    {0x326F, 0xFD08},
    {0x3E80, 0x8983},
    {0x2487, 0x2FB8},
    {0x2049, 0x4DD8},
    {0x2624, 0x3F38},
    {0x3412, 0x3E18},
    {0x3A89, 0x7E02},
    {0x0246, 0x05A8},
    {0x0493, 0x095C},
    {0x0250, 0x05B6},
    {0x0161, 0x05A8},
    {0x00ED, 0x0482},
    {0x0158, 0x05A8},
    {0x005D, 0x0380},
    {0x014B, 0x05A8},
    {0x24A4, 0x3C68},
    {0x061F, 0x0964},
    {0x0146, 0x0768},
    {0x0188, 0x07D1},
    {0x0199, 0x07D1},
    {0x4FD4, 0x77C0},
    {0x42F4, 0x9778},
    {0x364D, 0x3F08},
    {0x04C8, 0x122C},
    {0x075F, 0x1D08},
    {0x052B, 0x1450},
    {0x0BD5, 0x1FEB},
    {0x1441, 0x20EA},
    {0x1C07, 0x2E00},
    {0x0D6B, 0x2E00}
};

struct action actions[100] = {
    {0x84D0,   1,   3},
    {0xB798,   1,   1},
    {0x80E8,   2,   4},
    {0x03E8,   3,   8},
    {0xEA60,   4,   3},
    {0x6590,   0,   0},
    {0x9C40,   0,   0},
    {0x6590,   5,   3},
    {0x6590,   6,   3},
    {0x4E20,   7,   3},
    {0x4E20,   2,   3},
    {0x9C40,   8,   3},
    {0x4E20,   0,   1},
    {0x03E8,   0,   0},
    {0x03E8,   0,   6},
    {0x80E8,   9,   2},
    {0x02EE,   0,   7},
    {0x2422,   0,   9},
    {0x1388,   9,   2},
    {0x1388,   0,   0},
    {0x03E8,   9,   2},
    {0x2AF8,  10,   2},
    {0x07D0,   0,   7},
    {0x03E8,   0,   0},
    {0x1388,  10,   2},
    {0x1B58,  11,   2},
    {0x59D8,   0,   9},
    {0x03E8,   0,   1},
    {0x03E8,   0,  10},
    {0xB3B0,  12,   8},
    {0x01F4,   0,   1},
    {0x01F4,   0,   0},
    {0x03E8,   0,   5},
    {0x03E8,  12,   2},
    {0x07D0,  13,   2},
    {0x03E8,   0,   7},
    {0x01F4,   0,   0},
    {0x01F4,  13,   2},
    {0x03E8,  14,   2},
    {0x03E8,   0,   7},
    {0x01F4,   0,   0},
    {0x01F4,  14,   2},
    {0x03E8,  15,   2},
    {0x03E8,   0,   7},
    {0x01F4,   0,   0},
    {0x01F4,  15,   2},
    {0x03E8,  16,   2},
    {0x07D0,   0,   7},
    {0x2328,  17,   8},
    {0x4650,   0,   1},
    {0x1F40,  18,   2},
    {0x2710,  30,   8},
    {0x01F4,   0,   1},
    {0x01F4,   0,   9},
    {0x2328,  30,   8},
    {0x1770,  31,   2},
    {0x1388,  31,   7},
    {0x3A98,  17,   8},
    {0x9C40,   0,   1},
    {0xADD4,  18,   2},
    {0x03E8,   0,  10},
    {0x6D60,  19,   8},
    {0x03E8,   0,   1},
    {0x1F40,  19,   8},
    {0x1F40,  20,   2},
    {0x01F4,   0,   7},
    {0x01F4,   0,   0},
    {0x0BB8,  20,   2},
    {0x2EE0,  21,   2},
    {0x1F40,   0,   7},
    {0x2710,  22,   8},
    {0x3A98,  23,   3},
    {0xFFFA,   0, 255},
    {0xFFFA,  24,   3},
    {0x6590,   0, 255},
    {0x88B8,  25,   3},
    {0x2328,   0,  10},
    {0x4E20,   0,  10},
    {0xD6D8,  17,   3},
    {0x2AF8,   0,   1},
    {0x2710,  18,   2},
    {0xB798,  26,   8},
    {0x88B8,   0,   1},
    {0x1388,  27,   2},
    {0xAFC8,  28,   8},
    {0xC350,   3,   3},
    {0xDEA8,   0, 255},
    {0xFFFA,   1,   3},
    {0x3A98,   0, 255},
    {0x4E20,   0,   1},
    {0x2328,   2,   4},
    {0x2328,   2,   4},
    {0x2328,   2,   4},
    {0x03E8,   0,   8},
    {0x5DC0,   2,   3},
    {0x07D0,   0,   1},
    {0x01F4,   0,   0},
    {0x01F4,   0,   6},
    {0xBB80,  29,   2},
    {0x03E8,   0,   1}
};

uint32_t type4_heights[6] = {3, 0x0A, 0x19, 0x32, 0x55, 0x64};

uint8_t ending_text_packed[302] = {
0x50, 0x07, 0x22, 0x07, 0x0c, 0x40, 0x3E, 0x07, 0x18, 0x40, 0x34, 0x07,
0x20, 0x40, 0x2D, 0x07, 0x0E, 0x40, 0x0A, 0x07, 0x0E, 0x40, 0x28, 0x07,
0x0B, 0x40, 0x14, 0x07, 0x0B, 0x40, 0x25, 0x07, 0x08, 0x40, 0x17, 0x07,
0x0D, 0x40, 0x23, 0x07, 0x07, 0x40, 0x01, 0x70, 0x09, 0x07, 0x03, 0x70,
0x03, 0x07, 0x07, 0x70, 0x08, 0x40, 0x01, 0x07, 0x07, 0x40, 0x21, 0x07,
0x07, 0x40, 0x03, 0x70, 0x07, 0x07, 0x05, 0x70, 0x02, 0x07, 0x05, 0x70,
0x09, 0x40, 0x02, 0x70, 0x01, 0x07, 0x07, 0x40, 0x1F, 0x07, 0x07, 0x40,
0x05, 0x70, 0x05, 0x07, 0x06, 0x70, 0x05, 0x07, 0x09, 0x40, 0x05, 0x70,
0x01, 0x07, 0x07, 0x40, 0x1E, 0x07, 0x06, 0x40, 0x07, 0x70, 0x03, 0x07,
0x07, 0x70, 0x03, 0x07, 0x09, 0x40, 0x03, 0x07, 0x04, 0x70, 0x02, 0x07,
0x06, 0x40, 0x1D, 0x07, 0x06, 0x40, 0x01, 0x07, 0x08, 0x70, 0x01, 0x07,
0x08, 0x70, 0x01, 0x07, 0x09, 0x40, 0x09, 0x70, 0x03, 0x07, 0x06, 0x40,
0x1C, 0x07, 0x06, 0x40, 0x01, 0x07, 0x04, 0x70, 0x01, 0x07, 0x07, 0x70,
0x01, 0x07, 0x03, 0x70, 0x09, 0x40, 0x0B, 0x70, 0x03, 0x07, 0x06, 0x40,
0x1C, 0x07, 0x06, 0x40, 0x01, 0x07, 0x04, 0x70, 0x02, 0x07, 0x05, 0x70,
0x02, 0x07, 0x01, 0x70, 0x09, 0x40, 0x02, 0x70, 0x0E, 0x07, 0x06, 0x40,
0x1D, 0x07, 0x06, 0x40, 0x04, 0x70, 0x03, 0x07, 0x03, 0x70, 0x02, 0x07,
0x09, 0x40, 0x02, 0x70, 0x0F, 0x07, 0x06, 0x40, 0x1E, 0x07, 0x07, 0x40,
0x03, 0x70, 0x06, 0x07, 0x09, 0x40, 0x04, 0x70, 0x0E, 0x07, 0x07, 0x40,
0x1F, 0x07, 0x07, 0x40, 0x02, 0x70, 0x04, 0x07, 0x09, 0x40, 0x02, 0x07,
0x11, 0x70, 0x07, 0x40, 0x21, 0x07, 0x07, 0x40, 0x01, 0x70, 0x02, 0x07,
0x09, 0x40, 0x02, 0x70, 0x02, 0x07, 0x10, 0x70, 0x07, 0x40, 0x23, 0x07,
0x10, 0x40, 0x14, 0x07, 0x08, 0x40, 0x25, 0x07, 0x0D, 0x40, 0x12, 0x07,
0x0B, 0x40, 0x28, 0x07, 0x0E, 0x40, 0x0A, 0x07, 0x0E, 0x40, 0x2D, 0x07,
0x20, 0x40, 0x34, 0x07, 0x18, 0x40, 0x3E, 0x07, 0x0c, 0x40, 0x20, 0x00,
0x07, 0x00
};


static void WaitForKeypress(void) {
    SDL_Event event;
    int wait;
#if defined(PANDORA)
    uint32_t endticks;
#endif

    // get rid of old events
    while (!quit && SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
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
    }

    wait = 1;

    // wait for keypress
#if defined(PANDORA)
    endticks = SDL_GetTicks() + 5000;
    while (!quit && wait && (SDL_GetTicks() < endticks)) {
        if (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            quit = 1;
                            break;
                        default:
                            wait = 0;
                            break;
                    }
                    break;
                case SDL_QUIT:
                    quit = 1;
                    break;
                default:
                    break;
            };
        } else {
            SDL_Delay(10);
        }
    };
#else
    while (!quit && wait && SDL_WaitEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;
                    default:
                        wait = 0;
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
#endif
}

static void UnpackImage(uint8_t *image_packed, uint32_t image_packed_length, uint8_t **p_image_unpacked) {
    uint8_t temp_data[4096];
    uint32_t temp_offset, buffer_value, src_offset, dst_offset;
    uint8_t *image_unpacked;

    memset(temp_data, 32, 4096);
    temp_offset = 4078;
    buffer_value = 0;
    src_offset = 0;
    dst_offset = 0;

    image_unpacked = (uint8_t *) malloc(64000);
    while (1) {
        buffer_value >>= 1;
        if ((buffer_value & 0x100) == 0) {
            if (src_offset >= image_packed_length) break;
            buffer_value = (uint32_t)(image_packed[src_offset++]) | 0xff00;
        }

        if (buffer_value & 1) {
            uint8_t value1;

            if (src_offset >= image_packed_length) break;
            value1 = image_packed[src_offset++];

            image_unpacked[dst_offset++] = value1;
            temp_data[temp_offset] = value1;
            temp_offset = (temp_offset + 1) & 0xfff;
        } else {
            uint8_t b1, b2;
            uint32_t offset, count, index;

            if (src_offset >= (image_packed_length - 1)) break;
            b1 = image_packed[src_offset++];
            b2 = image_packed[src_offset++];

            offset = b1 | (((uint32_t)b2 & 0xf0) << 4);
            count = (b2 & 0x0f) + 2;

            for (index = 0; index <= count; index++) {
                uint8_t value2;

                value2 = temp_data[(offset + index) & 0xfff];

                image_unpacked[dst_offset++] = value2;
                temp_data[temp_offset] = value2;
                temp_offset = (temp_offset + 1) & 0xfff;
            }
        }
    };

    *p_image_unpacked = (uint8_t *) realloc(image_unpacked, dst_offset);
}

static void ConvertPalette6to8(uint8_t *palette) {
    uint32_t index;

    for (index = 0; index < 768; index++) {
        palette[index] = (uint8_t)((palette[index] << 2) | (palette[index] >> 4));
    }
}

static void DisplayScreen(void) {
    uint32_t *pixels;
    uint32_t src_start, index;

    SDL_LockSurface(screen);

    pixels = (uint32_t *)(screen->pixels);

    src_start = 4 * screen_start;

    if ((split_scanline > 0) && (split_scanline < 400)) {
        uint32_t length1;

        length1 = split_scanline * 160;

        for (index = 0; index < length1; index++) {
            pixels[index] = screen_palette[screen_data[src_start + index]];
        }
        for (index = 0; index < 64000 - length1; index++) {
            pixels[length1 + index] = screen_palette[screen_data[index]];
        }
    } else {
        for (index = 0; index < 64000; index++) {
            pixels[index] = screen_palette[screen_data[src_start + index]];
        }
    }

    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
}

static void PaletteFadeIn(uint8_t *palette) {
    uint32_t i, index;

    for (i = 0; i < 64; i++) {
        for (index = 0; index < 256; index++) {
            screen_palette[index] = 256 * 256 * ((((uint32_t)(palette[3*index    ])) * i) / 63)
                                  + 256 *       ((((uint32_t)(palette[3*index + 1])) * i) / 63)
                                  +             ((((uint32_t)(palette[3*index + 2])) * i) / 63);

        }
        DisplayScreen();
        SDL_Delay(16);
    }
}

static void PaletteFadeOut(void) {
    uint8_t palette[768];
    uint32_t index;
    int32_t i;

    for (index = 0; index < 256; index++) {
        palette[3*index    ] = (uint8_t)((screen_palette[index] >> 16) & 0xff);
        palette[3*index + 1] = (uint8_t)((screen_palette[index] >>  8) & 0xff);
        palette[3*index + 2] = (uint8_t)((screen_palette[index]      ) & 0xff);
    }

    for (i = 62; i >= 0; i--) {
        for (index = 0; index < 256; index++) {
            screen_palette[index] = 256 * 256 * ((((uint32_t)(palette[3*index    ])) * i) / 63)
                                  + 256 *       ((((uint32_t)(palette[3*index + 1])) * i) / 63)
                                  +             ((((uint32_t)(palette[3*index + 2])) * i) / 63);

        }
        DisplayScreen();
        SDL_Delay(16);
    }
}

static void SetPaletteAndDisplayScreen(uint8_t *palette) {
    uint32_t index;

    for (index = 0; index < 256; index++) {
        screen_palette[index] = 256 * 256 * ((uint32_t)(palette[3*index    ]))
                              + 256 *       ((uint32_t)(palette[3*index + 1]))
                              +             ((uint32_t)(palette[3*index + 2]));

    }
    DisplayScreen();
}

static void CopyImageToVideoMemory(uint32_t image_number) {
    uint32_t x, y, width, height, i;

    x = images[image_number].x;
    y = images[image_number].y;
    width = images[image_number].width;
    height = images[image_number].height;

    for (i = 0; i < height; i++) {
        uint32_t dst_offset, src_offset;

        dst_offset = (video_memory_address*65536 + (i + y)*320 + x);
        src_offset = i * width;

        memcpy(&(screen_data[dst_offset]), &(images[image_number].data[src_offset]), width);
    }
}

static void ClearScreenInVideoMemory(void) {
    uint32_t dst_offset;

    dst_offset = (video_memory_address*65536);

    memset(&(screen_data[dst_offset]), 0, 65536);
}

static void CopyScreenInVideoMemory(void) {
    uint32_t src_offset, dst_offset;

    src_offset = ((video_memory_address ^ 3)*65536);
    dst_offset = (video_memory_address*65536);

    memcpy(&(screen_data[dst_offset]), &(screen_data[src_offset]), 65536);
}

static inline uint32_t ConvertPaletteEntry6to8(uint32_t entry) {
    return (entry << 2) | ((entry >> 4) & 0x030303);
}

static void ExecuteAction(void) {
    switch (actions[next_action].action_type) {
        case 0:
            video_memory_address ^= 3;
            break;
        case 1:
            screen_start ^= 0xc000;
            SetPaletteAndDisplayScreen(images[current_image_palette].palette);
            break;
        case 2:
            CopyImageToVideoMemory(actions[next_action].image_number);
            current_image_palette = actions[next_action].image_number;

            DisplayScreen();
            break;
        case 3:
            screen_start ^= 0xc000;
            SetPaletteAndDisplayScreen(images[current_image_palette].palette);
            video_memory_address ^= 3;
            ClearScreenInVideoMemory();
            CopyImageToVideoMemory(actions[next_action].image_number);
            current_image_palette = actions[next_action].image_number;

            DisplayScreen();
            break;
        case 4:
            {
                uint32_t i;

                video_memory_address ^= 3;
                CopyScreenInVideoMemory();
                screen_start ^= 0xc000;
                for (i = 0; i < 6; i++) {
                    uint32_t copy_size, src_offset, dst_offset;

                    copy_size = type4_heights[i] * 320;
                    src_offset = (3*65536 + 32000 - copy_size);
                    dst_offset = (video_memory_address*65536);

                    memmove(&(screen_data[dst_offset]), &(screen_data[src_offset]), copy_size);

                    split_scanline = 400- 2*type4_heights[i];
                    DisplayScreen();

                    screen_start ^= 0xc000;
                    video_memory_address ^= 3;
                    SDL_Delay(66);
                }
                video_memory_address ^= 3;
                screen_start ^= 0xc000;

                memmove(&(screen_data[video_memory_address*65536+32000]), &(screen_data[0*65536+0]), 32000);

                split_scanline = 400;
                DisplayScreen();
            }
            break;
        case 5:
            ClearScreenInVideoMemory();

            DisplayScreen();
            break;
        case 6:
            CopyScreenInVideoMemory();

            DisplayScreen();
            break;
        case 7:
            screen_start ^= 0xc000;
            DisplayScreen();
            break;
        case 8:
            video_memory_address ^= 3;
            ClearScreenInVideoMemory();
            CopyImageToVideoMemory(actions[next_action].image_number);
            current_image_palette = actions[next_action].image_number;

            DisplayScreen();
            break;
        case 9:
            screen_palette[0] = ConvertPaletteEntry6to8(0x280000);
            DisplayScreen();
            SDL_Delay(33);
            screen_palette[0] = ConvertPaletteEntry6to8(0x110000);
            DisplayScreen();
            SDL_Delay(33);
            screen_palette[0] = 0;
            DisplayScreen();
            break;
        case 10:
            screen_palette[0] = ConvertPaletteEntry6to8(0x282828);
            DisplayScreen();
            SDL_Delay(33);
            screen_palette[0] = ConvertPaletteEntry6to8(0x111111);
            DisplayScreen();
            SDL_Delay(33);
            screen_palette[0] = 0;
            DisplayScreen();
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    SDL_Event event;
    int wait;

#if defined(PANDORA)
    if (argc >= 2) {
        chdir(argv[1]);
    }
#endif

    // load main image
    {
        FILE *f;
        uint8_t *main_image_packed;

        f = fopen("m2.001", "rb");
        if (f == NULL) {
            f = fopen("M2.001", "rb");
            if (f == NULL) {
                fprintf(stderr, "Data file doesn't exist\n");
                exit(1);
            }
        }

        main_image_packed = (uint8_t *) malloc(15364);

        fseek(f, -35687, SEEK_END);
        fread(main_image_packed, 1, 15364, f);
        fread(main_image_palette, 1, 768, f);
        fclose(f);

        UnpackImage(main_image_packed, 15364, &main_image_data);
        ConvertPalette6to8(main_image_palette);

        free(main_image_packed);
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


    screen_start = 0;
    split_scanline = 0;

    // fade in
    memcpy(screen_data, main_image_data, 64000);
    PaletteFadeIn(main_image_palette);


    // read images
    {
        FILE *f;
        uint8_t *image_packed;
        uint32_t i;

        f = fopen("m2.001", "rb");
        if (f == NULL) {
            f = fopen("M2.001", "rb");
            if (f == NULL) {
                fprintf(stderr, "Data file doesn't exist\n");
                exit(1);
            }
        }

        for (i = 0; i < 32; i++) {
            uint8_t info[8];

            image_packed = (uint8_t *) malloc(images_sizes[i][0]);

            fread(info, 1, 8, f);

            images[i].x = ((uint16_t)info[0]) | (((uint16_t)info[1]) << 8);
            images[i].y = (uint16_t)info[2];
            images[i].width = ((uint16_t)info[4]) | (((uint16_t)info[5]) << 8);
            images[i].height = ((uint16_t)info[6]) | (((uint16_t)info[7]) << 8);

            fread(images[i].palette, 1, 768, f);
            ConvertPalette6to8(images[i].palette);

            fread(image_packed, 1, images_sizes[i][0], f);
            UnpackImage(image_packed, images_sizes[i][0], &(images[i].data));

            free(image_packed);
        }

        fclose(f);
    }

    // read samples
    {
        FILE *f;

        f = fopen("m2.000", "rb");
        if (f == NULL) {
            f = fopen("M2.000", "rb");
            if (f == NULL) {
                fprintf(stderr, "Sample file doesn't exist\n");
                exit(1);
            }
        }

        fseek(f, 0, SEEK_END);
        sample_data_length = ftell(f);
        fseek(f, 0, SEEK_SET);

        sample_data = (uint8_t *) malloc(sample_data_length);

        fread(sample_data, 1, sample_data_length, f);
        fclose(f);
    }


    // SDL audio
    {
        have_audio = 0;
        if ( SDL_InitSubSystem(SDL_INIT_AUDIO) == 0 ) {
            if ( Mix_OpenAudio(16124, AUDIO_U8, 1, 512) == 0) {
                int frequency, channels;
                uint16_t format;

                if ( Mix_QuerySpec(&frequency, &format, &channels) )
                {
                    have_audio = 1;
                    if (frequency != 16124) {
                        have_audio = 0;
                        fprintf(stderr, "frequency = %i (desired = 16124)\n", frequency);
                    }
                    if (format != AUDIO_U8) {
                        have_audio = 0;
                        fprintf(stderr, "format = %i (desired = %i)\n", format, AUDIO_U8);
                    }
                    if (channels != 1) {
                        have_audio = 0;
                        fprintf(stderr, "channels = %i (desired = 1)\n", channels);
                    }

                }

                if (!have_audio) {
                    Mix_CloseAudio();
                }
            }

            if (!have_audio) {
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
            }
        }
    }

    quit = 0;

    WaitForKeypress();

    if (quit) {
        if (have_audio) {
            Mix_CloseAudio();
        }
        exit(0);
    }

    PaletteFadeOut();

    memset(screen_data, 0, 65536*4);
    // preload images
    {
        uint32_t i;

        screen_start = 0x8000;
        DisplayScreen();

        for (i = 0; i < 100; i++) {
            memcpy(&(screen_data[(0*65536+i*320)]), &(images[2].data[(32000+i*320)]), 320);
        }

        for (i = 0; i < 100; i++) {
            memcpy(&(screen_data[(3*65536+i*320)]), &(images[2].data[(0+i*320)]), 320);
        }

        video_memory_address = 1;
        CopyImageToVideoMemory(0);
        current_image_palette = 0;
        SetPaletteAndDisplayScreen(images[0].palette);
    }


    // play samples
    if (have_audio) {
        Mix_Chunk chunk;
        chunk.allocated = 0;
        chunk.abuf = sample_data;
        chunk.alen = sample_data_length;
        chunk.volume = 128;

        Mix_PlayChannel(-1, &chunk, 0);
    }

    next_action_time = ((uint64_t)SDL_GetTicks()) << 32;
    next_action = 0;

    finish_time = next_action_time + ( ((uint64_t)( (( sample_data_length | 0x7fff ) * 1000) / 16124 )) << 32 );

    wait = 1;
    while (!quit && wait) {
        uint64_t current_ticks;
        int delay;

        delay = 1;
        current_ticks = ((uint64_t)SDL_GetTicks()) << 32;

        if (current_ticks >= finish_time) {
            wait = 0;
        } else if (current_ticks >= next_action_time) {
            ExecuteAction();
            delay = 0;
            next_action_time += (((uint64_t)(((uint32_t)actions[next_action].time) * 1000)) << 32) / 16124;
            next_action++;
            if (next_action >= 100) next_action_time = UINT64_C(0xffffffffffffffff);
        }

        while (!quit && wait && SDL_PollEvent(&event)) {
            delay = 0;
            switch(event.type) {
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            quit = 1;
                            break;
                        default:
                            wait = 0;
                            break;
                    }
                    break;
                case SDL_QUIT:
                    quit = 1;
                    break;
                default:
                    break;
            };
        }

        if (delay && !quit && wait) {
            SDL_Delay(1);
        }
    }

    if (have_audio) {
        Mix_HaltChannel(-1);
        Mix_CloseAudio();
    }

    if (quit) {
        exit(0);
    }


    screen_start = 0;
    split_scanline = 0;

    // fade in
    memcpy(screen_data, main_image_data, 64000);
    PaletteFadeIn(main_image_palette);


    WaitForKeypress();

    if (quit) {
        exit(0);
    }

    PaletteFadeOut();

    // write final textmode picture
    {
        uint8_t ending_text[80*25];
        uint32_t src_offset, dst_offset, y, x, i;

        memset(ending_text, 0, 80*25);
        src_offset = 0;
        dst_offset = 0;

        while (ending_text_packed[src_offset]) {
            memset(&(ending_text[dst_offset]), ending_text_packed[src_offset+1], ending_text_packed[src_offset]);
            dst_offset += ending_text_packed[src_offset];
            src_offset += 2;
        };

        for (y = 0; y < 25; y++) {
            dst_offset = y * 8 * 320;
            for (x = 0; x < 80; x++) {
                memset(&(screen_data[dst_offset + 4*x]), (ending_text[80*y + x] >> 4) & 0xf, 4);
            }
            for (i= 1; i < 8; i++) {
                memcpy(&(screen_data[dst_offset + 320*i]), &(screen_data[dst_offset]), 320);
            }
        }

        screen_palette[ 0] = 0x000000;
        screen_palette[ 1] = 0x0000AA;
        screen_palette[ 2] = 0x00AA00;
        screen_palette[ 3] = 0x00AAAA;
        screen_palette[ 4] = 0xAA0000;
        screen_palette[ 5] = 0xAA00AA;
        screen_palette[ 6] = 0xAA5500;
        screen_palette[ 7] = 0xAAAAAA;

        screen_palette[ 8] = 0x555555;
        screen_palette[ 9] = 0x5555FF;
        screen_palette[10] = 0x55FF55;
        screen_palette[11] = 0x55FFFF;
        screen_palette[12] = 0xFF5555;
        screen_palette[13] = 0xFF55FF;
        screen_palette[14] = 0xFFFF55;
        screen_palette[15] = 0xFFFFFF;

        DisplayScreen();
    }

    WaitForKeypress();

    exit(0);

    return 0;
}

