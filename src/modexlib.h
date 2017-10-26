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
//***************************************************************************
//
//    MODEXLIB.C - various utils palette funcs and modex stuff
//
//***************************************************************************

#ifndef _modexlib_public
#define _modexlib_public

#include "rt_def.h"

#include "SDL2/SDL.h"

#define MAXSCREENWIDTH     1920
#define MAXSCREENHEIGHT    1080

extern int *screen_dest_resolution;
extern int screen_dest_resolution_size;
extern int screen_dest_resolution_index;
extern char **screen_dest_resolution_strings;

extern int screen_src_width;
extern int screen_src_height;
extern int screen_dest_width;
extern int screen_dest_height;

extern double screen_src_wscale;
extern double screen_src_hscale;
extern double screen_src_minscale;
extern double screen_src_maxscale;
extern double screen_src_scalecropratio;

extern int screen_src_health_x;
extern int screen_src_health_y;
extern int screen_src_ammo_x;
extern int screen_src_ammo_x_bar;
extern int screen_src_ammo_y;
extern int screen_src_statusbar_x;

extern int screen_shake;

extern boolean screen_fullscreen;
extern boolean screen_classic;
extern int screen_aspect;

enum
{
    ASPECT_RATIO_CORRECT = 1,
    ASPECT_RATIO_IGNORE = 0,
    ASPECT_RATIO_IGNOREANDSTRETCH = -1
};

extern boolean screen_zoom;
extern double screen_zoom_scale;
extern double screen_zoom_rot;

extern int xscalelookup[MAXSCREENWIDTH];
extern int xscalelookupmax[MAXSCREENWIDTH];
extern int xscalelookup_plane[MAXSCREENWIDTH];
extern int xscalelookupmax_plane[MAXSCREENWIDTH];
extern int xscalecoordlookup[MAXSCREENWIDTH];
extern int xscaleprecoordlookup[MAXSCREENWIDTH];
extern int xscalecoordfloorlookup[MAXSCREENWIDTH];
extern int yscalelookup[MAXSCREENHEIGHT];
extern int yscalelookupmax[MAXSCREENHEIGHT];
extern int yscalecoordlookup[MAXSCREENHEIGHT];
extern int yscaleprecoordlookup[MAXSCREENHEIGHT];
extern int yscalecoordfloorlookup[MAXSCREENHEIGHT];
extern int ylookup[MAXSCREENHEIGHT];

extern byte *page1start;
extern byte *page2start;
extern byte *page3start;
extern int screensize;
extern byte *bufferofs;
extern byte *displayofs;

extern SDL_Surface *screen_src_surface;
extern byte *classicpage;
extern boolean classicpage_draw;
#define classicscreensize   64000 // 320*200

void ChangeVideoMode (void);
void SetWindowFullscreen (void);
void InitGraphicsMode (void);
void ChangeScreenResolution (int width, int height);
void VL_SetVGAPlaneMode (void);
void VL_ClearBuffer (byte *buf, byte color);
void VL_ClearVideo (byte color);
void VL_CopyDisplayToHidden (void);
void VL_CopyBufferToAll (byte *buffer);
void VL_CopyPlanarPage (byte *src, byte *dest);
void VL_CopyPlanarPageToMemory (byte *src, byte *dest);
void XFlipPage (void);
void WaitVBL (void);

#endif
