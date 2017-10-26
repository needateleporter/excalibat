/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include <sys/stat.h>
#include "SDL2/SDL.h"
#include "modexlib.h"
#include "rt_util.h"
#include "rt_in.h"

// GLOBAL VARIABLES

int *screen_dest_resolution = NULL;
int screen_dest_resolution_size = 0;
int screen_dest_resolution_index = -1;
#define SCREEN_DEST_RESOLUTION_STRING_SIZE      12
#define SCREEN_DEST_RESOLUTION_STRING_LAYOUT    "%dX%d"
char **screen_dest_resolution_strings = NULL;

int screen_src_width = 640;
int screen_src_height = 400;
int screen_dest_width = -1;
int screen_dest_height = 480;

SDL_Rect screen_src_rect = {0, 0, 640, 400};
SDL_Rect screen_dest_rect = {0, 0, 640, 480};

double screen_src_wscale = 2.0; // From 320
double screen_src_hscale = 2.0; // From 200
double screen_src_minscale = 2.0; // min(wscale, hscale)
double screen_src_maxscale = 2.0; // max(wscale, hscale)
double screen_src_scalecropratio = 1.0; // mimscale / maxscale

int screen_src_health_x = 40;
int screen_src_health_y = 370;
int screen_src_ammo_x = 600;
int screen_src_ammo_x_bar = 600;
int screen_src_ammo_y = 368;
int screen_src_statusbar_x = 0; // For centered status bars on widescreen.

int screen_shake = -1; // -1 = none, 0 = right, 1 = left, 2 = down, 3 = up
SDL_Rect screen_shake_rect_in[4] = {{0, 0, 632, 400}, {8, 0, 632, 400}, {0, 0, 640, 394}, {6, 0, 640, 394}},
screen_shake_rect_out[4] = {{8, 0, 632, 400}, {0, 0, 632, 400}, {6, 0, 640, 394}, {0, 0, 640, 394}};

boolean screen_fullscreen = true;
boolean screen_classic = false;
int screen_aspect = ASPECT_RATIO_CORRECT;

boolean screen_zoom = false;
double screen_zoom_scale = 0;
double screen_zoom_rot = 0;
SDL_Point screen_zoom_center = {0,0};
SDL_Rect screen_zoom_rect = {0, 0, 640, 480};

// ---
// Pixel scale lookup tables:

// Scale size per pre-scaled pixel.
int xscalelookup[MAXSCREENWIDTH];
int xscalelookup_plane[MAXSCREENWIDTH];
int yscalelookup[MAXSCREENHEIGHT];

// Scale size (max) per pre-scaled pixel. TODO: FIXME
int xscalelookupmax[MAXSCREENWIDTH];
int xscalelookupmax_plane[MAXSCREENWIDTH];
int yscalelookupmax[MAXSCREENHEIGHT];

// First coordinate of post-scaled pixel region per pre-scaled pixel.
int xscalecoordlookup[MAXSCREENWIDTH];
int yscalecoordlookup[MAXSCREENHEIGHT];

// Pre-scaled pixel region per post-scaled pixel.
int xscaleprecoordlookup[MAXSCREENWIDTH];
int yscaleprecoordlookup[MAXSCREENHEIGHT];

// First coordinate of post-scaled pixel region per pixel.
int xscalecoordfloorlookup[MAXSCREENWIDTH];
int yscalecoordfloorlookup[MAXSCREENHEIGHT];

// ---

int ylookup[MAXSCREENHEIGHT];

byte *page1start;
byte *page2start;
byte *page3start;
int screensize;
byte *bufferofs;
byte *displayofs;

byte *classicpage;
boolean classicpage_draw = false;

SDL_Surface *screen_src_surface = NULL; // 8-bit base surface to draw on.
SDL_Surface *screen_blit_surface = NULL; // 32-bit conversion surface for blitting and screen shaking.
static SDL_Window *screen_window = NULL;
static SDL_Renderer *screen_renderer = NULL;
static SDL_Texture *screen_texture = NULL;

void ChangeVideoMode (void)
{
    // TODO: Determine if resizing the window or recreating the window is better in SDL2.
    // (Renderer/texture/surface is recreated regardless)
    //
    // For windowed mode, it's nice to have centered windows when changing res, especially
    // if the initial res puts the borders out of the screen.
    //
    //SDL_SetWindowSize(screen_window, screen_dest_width, screen_dest_height);

    if (screen_texture != NULL)
        SDL_DestroyTexture(screen_texture);

    if (screen_renderer != NULL)
        SDL_DestroyRenderer(screen_renderer);

    if (screen_window != NULL)
        SDL_DestroyWindow(screen_window);

    screen_window = SDL_CreateWindow("Rise of the Triad",
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     screen_dest_width, screen_dest_height, 0);

    if (screen_window == NULL)
        Error("Could not intialize SDL window.\n");

    // TODO: Icon
    // SDL_SetWindowIcon(screen_window, screen_icon_surface);

    // Initialize texture and renderer.
    screen_renderer = SDL_CreateRenderer(screen_window, -1, SDL_RENDERER_ACCELERATED);

    if (screen_renderer == NULL)
        Error("Could not intialize SDL renderer.\n");

    //SDL_RenderSetLogicalSize(screen_renderer, screen_dest_width, screen_dest_height);
    SDL_SetRenderDrawColor(screen_renderer, 0, 0, 0, 255);
    SDL_RenderClear(screen_renderer);
    SDL_RenderPresent(screen_renderer);

    screen_texture = SDL_CreateTexture(screen_renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING, screen_src_width, screen_src_height);

    if (screen_texture == NULL)
        Error("Could not intialize SDL texture.\n");

    SetWindowFullscreen();

    // Initialize 8-bit base surface.
    if (screen_src_surface != NULL)
    {
        SDL_FreeSurface(screen_src_surface);
        screen_src_surface = NULL;
    }

    screen_src_surface = SDL_CreateRGBSurface(0,
                         screen_src_width, screen_src_height, 8, 0, 0, 0, 0);

    if (screen_src_surface == NULL)
        Error("Could not intialize 8-bit base SDL surface.\n");

    SDL_FillRect(screen_src_surface, NULL, 0);

    // Initialize 32-bit conversion surface.
    if (screen_blit_surface != NULL)
    {
        SDL_FreeSurface(screen_blit_surface);
        screen_blit_surface = NULL;
    }

    screen_blit_surface = SDL_CreateRGBSurface(0,
                          screen_src_width, screen_src_height, 32, 0, 0, 0, 0);

    if (screen_blit_surface == NULL)
        Error("Could not intialize 32-bit conversion SDL surface.\n");

    SDL_FillRect(screen_blit_surface, NULL, 0);
}

void SetWindowFullscreen (void)
{
    if (screen_fullscreen)
    {
        if (SDL_SetWindowFullscreen(screen_window, SDL_WINDOW_FULLSCREEN) != 0)
        {
            printf("Could not set the window to fullscreen.");
            SDL_SetWindowFullscreen(screen_window, 0);
            screen_fullscreen = 0;
        }
    }
    else
    {
        SDL_SetWindowFullscreen(screen_window, 0);
    }
}

void InitDefaultResolution (void)
{
    SDL_DisplayMode tempMode;

    if (SDL_GetDesktopDisplayMode(0, &tempMode) == 0)
    {
        screen_dest_width = tempMode.w;
        screen_dest_height = tempMode.h;
    }
}


void GetDisplayModes (void)
{
    int i, j, numModes, isDupe;
    int *screen_dest_resolution_new = NULL;
    char **screen_dest_resolution_strings_new = NULL;
    SDL_DisplayMode tempMode;

    // TODO HIGHPRIORITY - Instead of using MAXSCREENWIDTH and MAXSCREENHEIGHT
    // defines, define an integer based on the largest values of all display
    // modes and dynamically allocate (instead of static array size).

    if (screen_dest_resolution != NULL)
    {
        free(screen_dest_resolution);
        for (i = 0; i < screen_dest_resolution_size; i++)
        {
            free(screen_dest_resolution_strings[i]);
        }
        free(screen_dest_resolution_strings);

        screen_dest_resolution_size = 0;
        screen_dest_resolution_index = -1;
    }

    // TODO - Multiple Monitors
    numModes = SDL_GetNumDisplayModes(0);

    // TODO LOWPRIORITY - Sort by aspect ratio (using GCD)?
    // Refresh rates?
    for (i = 0; i < numModes; i++)
    {
        SDL_GetDisplayMode(0, i, &tempMode);

        // TODO HIGHPRIORITY - Remove me once MAXSCREENWIDTH/MAXSCREENHEIGHT
        // is obsolete
        if (tempMode.w > MAXSCREENWIDTH || tempMode.h > MAXSCREENHEIGHT)
        {
            continue;
        }

        isDupe = false;

        for (j = 0; j < screen_dest_resolution_size; j++)
        {
            if (screen_dest_resolution[j*2] == tempMode.w &&
                    screen_dest_resolution[(j*2)+1] == tempMode.h)
            {
                isDupe = true;
                break;
            }
        }

        if (!isDupe)
        {
            screen_dest_resolution_new = realloc(screen_dest_resolution,
                                                 sizeof(int) * (screen_dest_resolution_size + 1) * 2);

            if (screen_dest_resolution_new == NULL)
            {
                // TODO - Error handling.
            }
            else
            {
                screen_dest_resolution_strings_new = realloc(screen_dest_resolution_strings,
                                                     sizeof(*screen_dest_resolution_strings) * (screen_dest_resolution_size + 1));

                if (screen_dest_resolution_strings_new == NULL)
                {
                    // TODO - Error handling. Uh-oh.
                }
                else
                {
                    screen_dest_resolution = screen_dest_resolution_new;
                    screen_dest_resolution_strings = screen_dest_resolution_strings_new;

                    screen_dest_resolution[screen_dest_resolution_size*2] = tempMode.w;
                    screen_dest_resolution[(screen_dest_resolution_size*2)+1] = tempMode.h;

                    screen_dest_resolution_strings[screen_dest_resolution_size] =
                        malloc(SCREEN_DEST_RESOLUTION_STRING_SIZE * sizeof(char *));

                    sprintf(screen_dest_resolution_strings[screen_dest_resolution_size],
                            SCREEN_DEST_RESOLUTION_STRING_LAYOUT, tempMode.w, tempMode.h);

                    screen_dest_resolution_size++;
                }
            }

            // Based on current config, determine the resolution index.
            if (tempMode.w == screen_dest_width &&
                    tempMode.h == screen_dest_height)
            {
                screen_dest_resolution_index = screen_dest_resolution_size - 1;
            }
        }
    }

    // If the default resolution index doesn't match any of the current ones,
    // add a new entry just in case.
    if (screen_dest_resolution_index == -1)
    {
        screen_dest_resolution_index = screen_dest_resolution_size;

        screen_dest_resolution_new = realloc(screen_dest_resolution,
                                             sizeof(int) * (screen_dest_resolution_size + 1) * 2);

        if (screen_dest_resolution_new == NULL)
        {
            // TODO - Error handling.
        }
        else
        {
            screen_dest_resolution_strings_new = realloc(screen_dest_resolution_strings,
                                                 sizeof(*screen_dest_resolution_strings) * (screen_dest_resolution_size + 1));

            if (screen_dest_resolution_strings_new == NULL)
            {
                // TODO - Error handling. Uh-oh.
            }
            else
            {
                screen_dest_resolution = screen_dest_resolution_new;
                screen_dest_resolution_strings = screen_dest_resolution_strings_new;

                screen_dest_resolution[screen_dest_resolution_size*2] = screen_dest_width;
                screen_dest_resolution[(screen_dest_resolution_size*2)+1] = screen_dest_height;

                screen_dest_resolution_strings[screen_dest_resolution_size] =
                    malloc(SCREEN_DEST_RESOLUTION_STRING_SIZE * sizeof(char *));

                sprintf(screen_dest_resolution_strings[screen_dest_resolution_size],
                        SCREEN_DEST_RESOLUTION_STRING_LAYOUT, screen_dest_width, screen_dest_height);

                screen_dest_resolution_size++;
            }
        }
    }
}

void InitGraphicsMode (void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        Error("Could not initialize SDL video.\n");

    if (screen_dest_width < 0)
    {
        InitDefaultResolution();
    }

    ChangeScreenResolution(screen_dest_width, screen_dest_height);

    GetDisplayModes();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    ChangeVideoMode();

    CheckSDLWindowGrabState();
}

void ChangeScreenRes (int srcwidth, int srcheight, int destwidth, int destheight)
{
    double classic_dest_wscale, classic_dest_hscale;
    int screen_classic_dest_arc_width, screen_classic_dest_arc_height,
        screen_classic_dest_arc_x, screen_classic_dest_arc_y;
    int i;

    // Change resolution vars.
    screen_src_width = srcwidth;
    screen_src_height = srcheight;
    screen_dest_width = destwidth;
    screen_dest_height = destheight;

    // Change scaling vars.
    screen_src_wscale = screen_src_width / 320.0;
    screen_src_hscale = screen_src_height / 200.0;
    screen_src_minscale = (screen_src_wscale > screen_src_hscale)
                          ? screen_src_hscale : screen_src_wscale;
    screen_src_maxscale = (screen_src_wscale > screen_src_hscale)
                          ? screen_src_wscale : screen_src_hscale;
    screen_src_scalecropratio = screen_src_minscale / screen_src_maxscale;

    // In Classic Mode, src -scale-> dest_arc -paint-> dest
    if (screen_classic && screen_aspect != ASPECT_RATIO_IGNOREANDSTRETCH)
    {
        classic_dest_wscale = screen_dest_width / 320.0;

        if (screen_aspect == ASPECT_RATIO_CORRECT)
            classic_dest_hscale = screen_dest_height / 240.0;
        else
            classic_dest_hscale = screen_dest_height / 200.0;

        if (classic_dest_wscale != classic_dest_hscale)
        {
            screen_classic_dest_arc_width = srcwidth *
                                            (classic_dest_wscale > classic_dest_hscale ? classic_dest_hscale : classic_dest_wscale);

            if (screen_aspect == ASPECT_RATIO_CORRECT)
                screen_classic_dest_arc_height = (srcheight * 240.0 *
                                                  (classic_dest_wscale > classic_dest_hscale ? classic_dest_hscale : classic_dest_wscale))
                                                 / 200.0;
            else
                screen_classic_dest_arc_height = (srcheight *
                                                  (classic_dest_wscale > classic_dest_hscale ? classic_dest_hscale : classic_dest_wscale));

            screen_classic_dest_arc_x = (screen_dest_width - screen_classic_dest_arc_width) / 2.0;
            screen_classic_dest_arc_y = (screen_dest_height - screen_classic_dest_arc_height) / 2.0;
        }
        else
        {
            screen_classic_dest_arc_width = destwidth;
            screen_classic_dest_arc_height = destheight;
            screen_classic_dest_arc_x = 0;
            screen_classic_dest_arc_y = 0;
        }

        screen_dest_rect.x = screen_classic_dest_arc_x;
        screen_dest_rect.y = screen_classic_dest_arc_y;
        screen_dest_rect.w = screen_classic_dest_arc_width;
        screen_dest_rect.h = screen_classic_dest_arc_height;
    }
    else
    {
        screen_dest_rect.x = 0;
        screen_dest_rect.y = 0;
        screen_dest_rect.w = screen_dest_width;
        screen_dest_rect.h = screen_dest_height;
    }

    screen_zoom_center.x = (screen_dest_rect.x + screen_dest_rect.w) / 2.0;
    screen_zoom_center.y = (screen_dest_rect.y + screen_dest_rect.h) / 2.0;

    screen_src_rect.x = 0;
    screen_src_rect.y = 0;
    screen_src_rect.w = screen_src_width;
    screen_src_rect.h = screen_src_height;

    // Screen shaking now done through SDL positional draws.
    for (i = 0; i < 4; i++)
    {
        screen_shake_rect_in[i].x = screen_shake_rect_out[i].x = screen_src_rect.x;
        screen_shake_rect_in[i].y = screen_shake_rect_out[i].y = screen_src_rect.y;
        screen_shake_rect_in[i].w = screen_shake_rect_out[i].w = screen_src_rect.w;
        screen_shake_rect_in[i].h = screen_shake_rect_out[i].h = screen_src_rect.h;
    }

    screen_shake_rect_in[0].w = screen_shake_rect_out[0].w =
                                    screen_shake_rect_in[1].w = screen_shake_rect_out[1].w =
                                            screen_src_width - (int)(4 * screen_src_minscale);
    screen_shake_rect_out[0].x = (int)(4 * screen_src_minscale);
    screen_shake_rect_in[1].x = (int)(4 * screen_src_minscale);

    screen_shake_rect_in[2].h = screen_shake_rect_out[2].h =
                                    screen_shake_rect_in[3].h = screen_shake_rect_out[3].h =
                                            screen_src_height - (int)(3 * screen_src_minscale);
    screen_shake_rect_out[2].y = (int)(3 * screen_src_minscale);
    screen_shake_rect_in[3].y = (int)(3 * screen_src_minscale);

    // Change status bar vars.
    screen_src_health_x = 20 * screen_src_minscale;
    screen_src_health_y = screen_src_height - (15 * screen_src_minscale);
    screen_src_ammo_x = screen_src_width - (20 * screen_src_minscale);
    screen_src_ammo_x_bar = 300 * screen_src_minscale;
    screen_src_ammo_y = screen_src_height - (16 * screen_src_minscale);
    screen_src_statusbar_x = (screen_src_width - (320 * screen_src_minscale)) / 2.0;
}

void ChangeScreenResolution (int width, int height)
{
    if (screen_classic)
        ChangeScreenRes(320, 200, width, height);
    else if (screen_aspect == ASPECT_RATIO_CORRECT)
        ChangeScreenRes(width, (int)((height * 200) / 240.0), width, height);
    else
        ChangeScreenRes(width, height, width, height);
}

/*
====================
=
= WaitVBL
=
====================
*/
void WaitVBL (void)
{
    SDL_Delay(16667/1000);
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/
void VL_SetVGAPlaneMode (void)
{
    int i, j, k, m, plane;
    double cur;

    // Set up basic Y table.
    j = 0;
    for (i = 0; i < screen_src_height; i++)
    {
        ylookup[i] = j;
        j += screen_src_width;
    }

    // Set up X-Scale table.
    cur = screen_src_minscale;
    i = 0;
    j = 0;
    m = 0;
    xscalecoordlookup[0] = 0;
    for (k = 0; k < screen_src_width; k++)
    {
        j++;

        if (cur <= k+1)
        {
            xscalelookup[i] = j;
            i++;
            j = 0;
            m = k;

            if (i >= screen_src_width)
                break;

            xscalecoordlookup[i] = k;

            cur += screen_src_minscale;
        }

        xscaleprecoordlookup[k] = i;
        xscalecoordfloorlookup[k] = m;
    }
    xscalelookup[i] = j;

    cur = screen_src_maxscale;
    i = 0;
    j = 0;
    for (k = 0; k < screen_src_width; k++)
    {
        j++;

        if (cur <= k+1)
        {
            xscalelookupmax[i] = j;
            i++;
            j = 0;

            if (i >= screen_src_width)
                break;

            cur += screen_src_maxscale;
        }
    }
    xscalelookupmax[i] = j;

    plane = 0;
    cur = screen_src_minscale;
    for (k = 0; k < screen_src_width; k++)
    {
        xscalelookup_plane[k] = plane;
        if (cur <= k+1)
        {
            if (plane < 3)
                plane++;
            else
                plane = 0;

            cur += screen_src_minscale;
        }
    }

    plane = 0;
    cur = screen_src_maxscale;
    for (k = 0; k < screen_src_width; k++)
    {
        xscalelookupmax_plane[k] = plane;
        if (cur <= k+1)
        {
            if (plane < 3)
                plane++;
            else
                plane = 0;

            cur += screen_src_maxscale;
        }
    }

    // Set up Y-Scale table.
    cur = screen_src_minscale;
    i = 0;
    j = 0;
    m = 0;
    yscaleprecoordlookup[0] = 0;
    for (k = 0; k < screen_src_height; k++)
    {
        j++;

        if (cur <= k+1)
        {
            yscalelookup[i] = j;
            i++;
            j = 0;
            m = k;

            if (i >= screen_src_height)
                break;

            yscalecoordlookup[i] = k;

            cur += screen_src_minscale;
        }

        yscaleprecoordlookup[k] = i;
        yscalecoordfloorlookup[k] = m;
    }
    yscalelookup[i] = j;

    cur = screen_src_maxscale;
    i = 0;
    j = 0;
    for (k = 0; k < screen_src_height; k++)
    {
        j++;

        if (cur <= k+1)
        {
            yscalelookupmax[i] = j;
            i++;
            j = 0;

            if (i >= screen_src_height)
                break;

            cur += screen_src_maxscale;
        }
    }
    yscalelookupmax[i] = j;

    screensize = screen_src_height * screen_src_width;

    // TODO: Reimplement triple buffering?
    page1start = screen_src_surface->pixels;
    page2start = screen_src_surface->pixels;
    page3start = screen_src_surface->pixels;

    displayofs = page1start;
    bufferofs = page2start;

    if (classicpage == NULL)
        classicpage = SafeMalloc(64000);

    XFlipPage();
}

/*
=======================
=
= VL_CopyPlanarPage
=
=======================
*/
void VL_CopyPlanarPage (byte *src, byte *dest)
{
    memcpy(dest, src, screensize);
}

/*
=======================
=
= VL_CopyPlanarPageToMemory
=
=======================
*/
void VL_CopyPlanarPageToMemory (byte *src, byte *dest)
{
    memcpy(dest, src, screensize);
}

/*
=======================
=
= VL_CopyBufferToAll
=
=======================
*/
void VL_CopyBufferToAll (byte *buffer)
{
    /*
        if (page1start != buffer)
            memcpy(page1start, buffer, screensize);

        if (page2start != buffer)
            memcpy(page2start, buffer, screensize);

        if (page3start != buffer)
            memcpy(page3start, buffer, screensize);
    */
}

/*
=======================
=
= VL_CopyDisplayToHidden
=
=======================
*/
void VL_CopyDisplayToHidden (void)
{
    VL_CopyBufferToAll(displayofs);
}

/*
=================
=
= VL_ClearBuffer
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearBuffer (byte *buf, byte color)
{
    memset(buf, color, screensize);
}

/*
=================
=
= VL_ClearVideo
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearVideo (byte color)
{
    memset(screen_src_surface->pixels, color, screensize);
}

/* C version of rt_vh_a.asm */

void VH_UpdateScreen (void)
{
    XFlipPage();
}


/*
=================
=
= XFlipPage
=
=================
*/

void XFlipPage (void)
{
    CheckSDLWindowGrabState();

    SDL_RenderClear(screen_renderer);

    if (screen_shake >= 0 && screen_shake < 4)
    {
        SDL_FillRect(screen_blit_surface, NULL, 0);
        SDL_BlitSurface(screen_src_surface, &screen_shake_rect_in[screen_shake],
                        screen_blit_surface, &screen_shake_rect_out[screen_shake]);
    }
    else
        SDL_LowerBlit(screen_src_surface, &screen_src_surface->clip_rect,
                      screen_blit_surface, &screen_blit_surface->clip_rect);

    SDL_UpdateTexture(screen_texture, NULL, screen_blit_surface->pixels,
                      screen_src_width * sizeof(Uint32));

    if (screen_zoom)
    {
        screen_zoom_rect.w = (double)screen_dest_rect.w / screen_zoom_scale;
        screen_zoom_rect.h = (double)screen_dest_rect.h / screen_zoom_scale;
        screen_zoom_rect.x = screen_zoom_center.x - ((double)screen_zoom_rect.w / 2.0);
        screen_zoom_rect.y = screen_zoom_center.y - ((double)screen_zoom_rect.h / 2.0);

        SDL_RenderCopyEx(screen_renderer, screen_texture, &screen_src_rect, &screen_zoom_rect,
                         screen_zoom_rot, NULL, SDL_FLIP_NONE);
    }
    else
        SDL_RenderCopy(screen_renderer, screen_texture, &screen_src_rect, &screen_dest_rect);

    SDL_RenderPresent(screen_renderer);
}
