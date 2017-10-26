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
#ifndef _rt_vid_public
#define _rt_vid_public

//***************************************************************************
//
// Public header for RT_VID.C
//
//***************************************************************************

#include "lumpy.h"


//***************************************************************************
//
// GLOBALS
//
//***************************************************************************

extern boolean  screenfaded;


//***************************************************************************
//
// PROTOTYPES
//
//***************************************************************************
void VL_MemToScreen (byte *source, int width, int height, int xpos, int ypos);
void DrawTiledRegion( int x, int y, int width, int height, int offx, int offy, pic_t *tile );
void VL_DrawPic (int x, int y, pic_t *pic);
void VL_DrawPicFullscreen (pic_t *pic);
void VL_DrawPicRedraw (int x, int y, int width, int height, pic_t *pic);
void VL_Bar (int x, int y, int width, int height, int color);
void VL_Hlin (unsigned x, unsigned y, unsigned width, unsigned color);
void VL_Vlin (int y1, int y2, int x, int color);
void VL_THlin (unsigned x, unsigned y, unsigned width, boolean up);
void VL_TVlin (unsigned x, unsigned y, unsigned height, boolean up);
int VW_MarkUpdateBlock (int x1, int y1, int x2, int y2);
void VW_UpdateScreen (void);

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn (int start, int end, byte *palette, int steps);
void VL_DecompressLBM (lbm_t *lbminfo, boolean show, int backr, int backg, int backb);
void VL_FadeToColor (int time, int red, int green, int blue);
void VL_TBar (int x, int y, int width, int height);

void SwitchPalette (byte *newpal, int steps);
void SetBorderColor (int color);

void VL_DrawPostPic (int lumpnum);
void VL_DrawLine (int x1, int y1, int x2, int y2, byte color);

#define MenuFadeOut()   VL_FadeOut (0, 255, 0, 0, 0, 10)
#define MenuFadeIn()       VL_FadeIn (0, 255, origpal, 10)

#endif
