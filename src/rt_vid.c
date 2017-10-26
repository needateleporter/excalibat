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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rt_def.h"
#include "rt_vid.h"
#include "_rt_vid.h"
#include "rt_menu.h"
#include "rt_util.h"
#include "modexlib.h"
#include "profile.h"
#include "watcom.h"
#include "rt_str.h"
#include "rt_draw.h"
#include "rt_in.h"
#include "rt_main.h"
#include "z_zone.h"
#include "lumpy.h"
#include "rt_vh_a.h"
#include "isr.h"
#include "rt_view.h"
#include "cin_efct.h"
#include "w_wad.h"


//******************************************************************************
//
// GLOBALS
//
//******************************************************************************

byte     palette1[256][3], palette2[256][3];
boolean  screenfaded;


//******************************************************************************
//
// VL_MemToScreen ()
//
//******************************************************************************

void VL_MemToScreen (byte *source, int width, int height, int xpos, int ypos)
{
    int origdest;
    int dest;
    byte pixel;
    int plane, x, y, i, j;
    int finalw = width * screen_src_minscale * 4;
    byte *tempsrc;

    origdest = ylookup[ypos] + xpos;

    tempsrc = source;

    for (plane = 0; plane < 4; plane++)
    {
        dest = origdest;
        source = tempsrc + (plane * width * height);

        for (y = 0; y < height; y++)
        {
            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *source++;

                while (xscalelookup_plane[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (j = 0; j < yscalelookup[y]; j++)
                            *(dest+bufferofs+(screen_src_width * j)) = pixel;
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookup[y]) - i;
        }
    }
}

void VL_MemToScreenFull (byte *source, int width, int height)
{
    int origdest;
    int dest;
    byte pixel;
    int plane, x, y, i, j, k;
    int finalw, finalh;
    int htstart, ht;
    int xscale[screen_src_width];
    double cur;
    byte *tempsrc;

    origdest = ylookup[0];

    tempsrc = source;

    // Generate x-scale table.
    finalw = width * screen_src_wscale;

    if (finalw > screen_src_width)
        finalw = screen_src_width;

    finalw *= 4;

    plane = 0;
    cur = screen_src_maxscale;
    for (x = 0; x < finalw; x++)
    {
        xscale[x] = plane;
        if (cur <= x+1)
        {
            if (plane < 3)
                plane++;
            else
                plane = 0;

            cur += screen_src_maxscale;
        }
    }

    finalh = height * screen_src_hscale;

    if (finalh > screen_src_height)
        finalh = screen_src_height;

    cur = screen_src_maxscale;
    i = 0;
    for (y = 0; y < finalh; y++)
    {
        if (cur <= y+1)
        {
            i++;

            if (i >= height)
                break;

            cur += screen_src_maxscale;
        }
    }
    ht = i;
    htstart = (height - ht) >> 1;

    for (plane = 0; plane < 4; plane++)
    {
        dest = origdest;
        source = tempsrc + (plane * width * height) + (width * htstart);

        k = 0;

        for (y = 0; y < height; y++)
        {
            if (k >= finalh)
                break;

            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *source++;

                while (xscale[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscale[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (j = 0; j < yscalelookupmax[y]; j++)
                            *(dest+bufferofs+(screen_src_width * j)) = pixel;
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookupmax[y]) - i;
            k += yscalelookupmax[y];
        }
    }
}

void VL_MemToScreenRedraw (byte *source, int width, int height, int xpos, int ypos, int destwidth, int destheight)
{
    int origdest;
    int dest;
    byte pixel;
    int plane, x, y, i, j, k;
    int finalw, finalh;
    int htstart, ht;
    int xscale[screen_src_width];
    double cur;
    byte *tempsrc;

    origdest = ylookup[0];

    tempsrc = source;

    // Generate x-scale table.
    finalw = width * screen_src_wscale;

    if (finalw > screen_src_width)
        finalw = screen_src_width;

    finalw *= 4;

    plane = 0;
    cur = screen_src_maxscale;
    for (x = 0; x < finalw; x++)
    {
        xscale[x] = plane;
        if (cur <= x+1)
        {
            if (plane < 3)
                plane++;
            else
                plane = 0;

            cur += screen_src_maxscale;
        }
    }

    // Generate y-scale table.
    finalh = height * screen_src_hscale;

    if (finalh > screen_src_height)
        finalh = screen_src_height;

    cur = screen_src_maxscale;
    i = 0;
    for (y = 0; y < finalh; y++)
    {
        if (cur <= y+1)
        {
            i++;

            if (i >= height)
                break;

            cur += screen_src_maxscale;
        }
    }
    ht = i;
    htstart = (height - ht) >> 1;

    for (plane = 0; plane < 4; plane++)
    {
        dest = origdest;
        source = tempsrc + (plane * width * height) + (width * htstart);

        k = 0;

        for (y = 0; y < height; y++)
        {
            if (k >= finalh || k >= ypos + destheight)
                break;

            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *source++;

                while (xscale[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscale[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (j = 0; j < yscalelookupmax[y]; j++)
                            if ((i >= xpos) && (i < (xpos + destwidth)) &&
                                    ((k+j) >= ypos) && ((k+j) < (ypos + destheight)))
                                *(dest+bufferofs+(screen_src_width * j)) = pixel;
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookupmax[y]) - i;
            k += yscalelookupmax[y];
        }
    }
}

//*************************************************************************
//
// DrawTiledRegion () - Fills the specified region with a tiled image
//
//*************************************************************************
void DrawTiledRegion (int x, int y, int width, int height, int offx, int offy, pic_t *tile)
{
    byte *source;
    byte *sourceoff;
    int sourcex, sourcey;
    int sourcewidth, sourceheight;
    int plane;
    int planesize;
    byte *start;
    byte *origdest;
    byte *dest;
    int startoffset;
    int HeightIndex;
    int WidthIndex;
    int i, j;
    double cur;

    start = (byte *)(bufferofs + x + ylookup[y]);

    source = &tile->data;
    sourcewidth  = tile->width;
    sourceheight = tile->height;

    if (offx >= sourcewidth)
        offx %= sourcewidth;

    if (offy >= sourceheight)
        offy %= sourceheight;

    startoffset = offy * sourcewidth;
    planesize = sourcewidth * sourceheight;

    plane = 4;
    while (plane > 0)
    {
        origdest = start;

        sourcey = offy;
        sourceoff = source + startoffset;
        HeightIndex = height;

        j = 0;
        cur = screen_src_minscale;

        while (HeightIndex--)
        {
            dest = origdest;
            sourcex = offx;
            WidthIndex = width;
            i = 0;

            while (WidthIndex--)
            {
                while (xscalelookup_plane[i] != (4 - plane) && i < width)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == (4 - plane) && i < width)
                {
                    *dest = sourceoff[sourcex];

                    i++;
                    dest++;
                }

                sourcex++;

                if (sourcex >= sourcewidth)
                    sourcex = 0;
            }

            origdest += screen_src_width;

            j++;

            if (cur <= j+1)
            {
                cur += screen_src_minscale;

                sourceoff += sourcewidth;
                sourcey++;

                if (sourcey >= sourceheight)
                {
                    sourcey = 0;
                    sourceoff = source;
                }
            }
        }

        source += planesize;

        plane--;
    }
}


//******************************************************************************
//
// VL_DrawPic () - Draws a linear pic
//
//******************************************************************************

void VL_DrawPic (int x, int y, pic_t *pic)
{
    VL_MemToScreen((byte *)&pic->data, pic->width, pic->height, x, y);
}

// Used for the HUNT Team background.
void VL_DrawPicFullscreen(pic_t *pic)
{
    VL_MemToScreenFull((byte *)&pic->data, pic->width, pic->height);
}

// Used for the HUNT Team background redraw in DoEndBonus.
void VL_DrawPicRedraw(int x, int y, int width, int height, pic_t *pic)
{
    VL_MemToScreenRedraw((byte *)&pic->data, pic->width, pic->height, x, y, width, height);
}


//******************************************************************************
//
// VL_Bar () - Draws a bar
//
//******************************************************************************

void VL_Bar (int x, int y, int width, int height, int color)
{
    byte *dest = (byte *)(bufferofs + ylookup[y] + x);

    while (height--)
    {
        memset(dest, color, width);

        dest += screen_src_width;
    }
}


//******************************************************************************
//
// VL_TBar () - Draws a bar
//
//******************************************************************************

void VL_TBar (int x, int y, int width, int height)
{
    int w = width;

    while (height--)
    {
        byte *dest = (byte *)(bufferofs+ylookup[y]+x);

        width = w;

        while (width--)
        {
            byte pixel = *dest;

            pixel = *(colormap+(27<<8)+pixel);

            *dest = pixel;

            dest++;
        }

        y++;
    }
}


//******************************************************************************
//
// VL_Hlin () - Draws a horizontal line
//
//******************************************************************************

void VL_Hlin (unsigned x, unsigned y, unsigned width, unsigned color)
{
    byte *dest = (byte *)(bufferofs+ylookup[y]+x);

    memset(dest, color, width);
}


//******************************************************************************
//
// VL_Vlin () - Draws a vertical line
//
//******************************************************************************

void VL_Vlin (int x, int y, int height, int color)
{
    byte *dest = (byte *)(bufferofs+ylookup[y]+x);

    while (height--)
    {
        *dest = color;

        dest += screen_src_width;
    }
}


//******************************************************************************
//
// VL_THlin ()
//
//******************************************************************************

void VL_THlin (unsigned x, unsigned y, unsigned width, boolean up)
{
    byte *dest = (byte *)(bufferofs+ylookup[y]+x);

    while (width--)
    {
        byte pixel = *dest;

        if (up)
        {
            pixel = *(colormap+(13<<8)+pixel);
        }
        else
        {
            pixel = *(colormap+(27<<8)+pixel);
        }

        *dest = pixel;

        dest++;
    }
}



//******************************************************************************
//
// VL_TVlin ()
//
//******************************************************************************

void VL_TVlin (unsigned x, unsigned y, unsigned height, boolean up)
{
    byte *dest = (byte *)(bufferofs+ylookup[y]+x);

    while (height--)
    {
        byte pixel = *dest;

        if (up)
        {
            pixel = *(colormap+(13<<8)+pixel);
        }
        else
        {
            pixel = *(colormap+(27<<8)+pixel);
        }

        *dest = pixel;

        dest += screen_src_width;
    }
}


/*
================================================================================

            Double buffer management routines

================================================================================
*/


//******************************************************************************
//
// VW_UpdateScreen ()
//
//******************************************************************************


void VW_UpdateScreen (void)
{
    VH_UpdateScreen ();
}

//===========================================================================

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps)
{
    int i, j, orig, delta;
    byte *origptr, *newptr;

    if (screenfaded)
        return;

    WaitVBL ();
    VL_GetPalette(&palette1[0][0]);
    memcpy(palette2, palette1, 768);

    for (i = 0; i < steps; i++)
    {
        origptr = &palette1[start][0];
        newptr = &palette2[start][0];

        for (j = start; j <= end; j++)
        {
            orig = *origptr++;
            delta = red - orig;
            *newptr++ = orig + delta * i / steps;
            orig = *origptr++;
            delta = green - orig;
            *newptr++ = orig + delta * i / steps;
            orig = *origptr++;
            delta = blue - orig;
            *newptr++ = orig + delta * i / steps;
        }

        WaitVBL();
        VL_SetPalette(&palette2[0][0]);
    }

    VL_FillPalette(red, green, blue);

    screenfaded = true;
}


/*
=================
=
= VL_FadeToColor
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

void VL_FadeToColor (int time, int red, int green, int blue)
{
    int      i,j,orig,delta;
    byte  *origptr, *newptr;
    int dmax,dmin;

    if (screenfaded)
        return;

    WaitVBL ();
    VL_GetPalette (&palette1[0][0]);
    memcpy (palette2, palette1, 768);

    dmax=(maxshade<<16)/time;
    dmin=(minshade<<16)/time;
//
// fade through intermediate frames
//
    for (i = 0; i < time; i+=tics)
    {
        origptr = &palette1[0][0];
        newptr = &palette2[0][0];

        for (j = 0; j <= 255; j++)
        {
            orig = *origptr++;
            delta = ((red>>2)-orig)<<16;
            *newptr++ = orig + FixedMul(delta/time,i);
            orig = *origptr++;
            delta = ((green>>2)-orig)<<16;
            *newptr++ = orig + FixedMul(delta/time,i);
            orig = *origptr++;
            delta = ((blue>>2)-orig)<<16;
            *newptr++ = orig + FixedMul(delta/time,i);
        }

        maxshade=(dmax*(time-i))>>16;
        minshade=(dmin*(time-i))>>16;
        WaitVBL ();
        VL_SetPalette (&palette2[0][0]);
        ThreeDRefresh();
        CalcTics();

    }

//
// final color
//
    VL_FillPalette (red>>2,green>>2,blue>>2);

    screenfaded = true;
}




/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn (int start, int end, byte *palette, int steps)
{
    int i,j,delta;

    WaitVBL ();
    VL_GetPalette(&palette1[0][0]);

    memcpy(&palette2[0][0], &palette1[0][0], sizeof(palette1));

    start *= 3;
    end = end * 3 + 2;

    for (i = 0; i < steps; i++)
    {
        for (j = start; j <= end; j++)
        {
            delta = palette[j] - palette1[0][j];
            palette2[0][j] = palette1[0][j] + delta * i / steps;
        }

        WaitVBL ();
        VL_SetPalette(&palette2[0][0]);
    }

    VL_SetPalette(palette);
    screenfaded = false;
}



//******************************************************************************
//
// SwitchPalette
//
//******************************************************************************

void SwitchPalette (byte *newpal, int steps)
{
    byte *temp;

    VL_FadeOut(0, 255, 0, 0, 0, steps >> 1);

    temp = bufferofs;
    bufferofs = displayofs;
    VL_Bar(0, 0, screen_src_width, screen_src_height, 0);
    bufferofs = temp;

    VL_FadeIn(0, 255, newpal, steps >> 1);
}


#if 0

/*
=================
=
= VL_TestPaletteSet
=
= Sets the palette with outsb, then reads it in and compares
= If it compares ok, fastpalette is set to true.
=
=================
*/

void VL_TestPaletteSet (void)
{
    int   i;

    for (i=0; i<768; i++)
        palette1[0][i] = i;

    fastpalette = true;
    VL_SetPalette (&palette1[0][0]);
    VL_GetPalette (&palette2[0][0]);
    if (_fmemcmp (&palette1[0][0],&palette2[0][0],768))
        fastpalette = false;
}


/*
==================
=
= VL_ColorBorder
=
==================
*/

void VL_ColorBorder (int color)
{
    _AH=0x10;
    _AL=1;
    _BH=color;
    geninterrupt (0x10);
    bordercolor = color;
}


#endif


//==========================================================================

//****************************************************************************
//
// VL_DecompressLBM ()
//
// LIMITATIONS - Only works with 320x200!!!
//
//****************************************************************************

void VL_DecompressLBM (lbm_t *lbminfo, boolean show, int backr, int backg, int backb)
{
    int countx, county;
    byte b, rept;
    byte *source = (byte *)&lbminfo->data;
    byte *buf;
    int ht = lbminfo->height;
    byte pal[768];
    int curx, i, j, k;

    memcpy(&pal[0], lbminfo->palette, 768);

    VL_NormalizePalette(&pal[0]);
    VL_ClearVideo(BestColor(backr, backg, backb, &pal[0]));

    buf = (byte *)bufferofs;
    buf += screen_src_statusbar_x;

    county = 0;

    while (ht--)
    {
        countx = 0;
        curx = 0;

        do
        {
            rept = *source++;

            if (rept > 0x80)   // compressed chunk of one color
            {
                rept = (rept ^ 0xff) + 2;
                b = *source++;
                i = rept;
                while (i--)
                {
                    for (k = 0; k < yscalelookup[county]; k++)
                    {
                        memset(buf + (screen_src_width * k), b, xscalelookup[countx]);
                    }

                    buf += xscalelookup[countx];
                    curx += xscalelookup[countx];
                    countx++;
                }
            }
            else if (rept < 0x80)     // uncompressed chunk
            {
                rept++;
                i = rept;
                while (i--)
                {
                    j = xscalelookup[countx];
                    while (j--)
                    {
                        for (k = 0; k < yscalelookup[county]; k++)
                        {
                            *(buf + (screen_src_width * k)) = *source;
                        }

                        buf++;
                        curx++;
                    }
                    countx++;
                    source++;
                }
            } // rept of 0x80 is NOP

        }
        while (countx < lbminfo->width);

        buf += screen_src_width * yscalelookup[county] - curx;
        county++;
    }

    if (show)
    {
        VW_UpdateScreen();
        VL_FadeIn(0, 255, &pal[0], 15);
    }
}

//****************************************************************************
//
// SetBorderColor
//
//****************************************************************************

void SetBorderColor (int color)
{
    // TODO: Was this even in ROTT? DOSBox shows no borders. Old hardware with DOS?

    // bna section start

    byte *cnt, *Ycnt, *b;

    b = (byte *)bufferofs;

    // color 56 could be used

    //paint top red line
    for (cnt = b; cnt < (b + viewwidth); cnt++)
    {
        for (Ycnt = cnt; Ycnt < (cnt + ((int)(5 * screen_src_minscale) * screen_src_width)); Ycnt += screen_src_width)
        {
            *Ycnt = color;
        }
    }

    //paint left red line
    for (cnt = b; cnt < (b + (int)(5 * screen_src_minscale)); cnt++)
    {
        for (Ycnt = cnt; Ycnt < cnt + (viewheight * screen_src_width); Ycnt+=screen_src_width)
        {
            *Ycnt = color;
        }
    }

    //paint right red line
    for (cnt = (b + (viewwidth - (int)(5 * screen_src_minscale))); cnt < b + viewwidth; cnt++)
    {
        for (Ycnt = cnt; Ycnt < cnt + (viewheight * screen_src_width); Ycnt += screen_src_width)
        {
            *Ycnt = color;
        }
    }

    //paint lower red line
    for (cnt = (b + ((viewheight - (int)(5 * screen_src_minscale)) * screen_src_width));
            cnt < (b + ((viewheight - (int)(5 * screen_src_minscale)) * screen_src_width)) + viewwidth; cnt++)
    {
        for (Ycnt = cnt; Ycnt < (b + (viewheight * screen_src_width)); Ycnt += screen_src_width)
        {
            *Ycnt = color;
        }
    }

    // bna section end
}

//****************************************************************************
//
// SetBorderColorInterrupt
//
//****************************************************************************

void SetBorderColorInterrupt (int color)
{
    STUB_FUNCTION;
}


//****************************************************************************
//
// VL_DrawPostPic
//
//****************************************************************************

void VL_DrawPostPic (int lumpnum)
{
    DrawPostPic(lumpnum);
}

//****************************************************************************
//
// VL_DrawLine
//
//****************************************************************************

void VL_DrawLine (int x1, int y1, int x2, int y2, byte color)
{
    int dx;
    int dy;
    int xinc;
    int yinc;
    int count;

    dx=(x2-x1);
    dy=(y2-y1);
    if (abs(dy)>=abs(dx))
    {
        count=abs(dy);
        yinc=(dy<<16)/count;
        if (dy==0)
        {
            return;
        }
        else
        {
            xinc=(dx<<16)/count;
        }
    }
    else
    {
        count=abs(dx);
        xinc=(dx<<16)/count;
        if (dx==0)
        {
            return;
        }
        else
        {
            yinc=(dy<<16)/count;
        }
    }
    x1<<=16;
    y1<<=16;
    while (count>0)
    {
        *((byte *)bufferofs+(x1>>16)+(ylookup[y1>>16]))=color;
        x1+=xinc;
        y1+=yinc;
        count--;
    }
}
