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
#include "rt_def.h"
#include <string.h>

#include "sprites.h"
#include "rt_map.h"
#include "rt_dr_a.h"
#include "_rt_map.h"
#include "isr.h"
#include "rt_util.h"
#include "modexlib.h"
#include "rt_draw.h"
#include "rt_stat.h"
#include "z_zone.h"
#include "w_wad.h"
#include "rt_main.h"
#include "rt_playr.h"
#include "lumpy.h"
#include "rt_door.h"
#include "rt_scale.h"
#include "rt_vid.h"
#include "rt_in.h"
#include "rt_ted.h"
#include "rt_game.h"
#include "rt_rand.h"
#include "rt_view.h"
#include "rt_floor.h"
#include "engine.h"
#include "develop.h"
#include "rt_menu.h"
#include "rt_net.h"
#include "rt_str.h"
#include "watcom.h"

//===========================================================================


static int tilesize;
static fixed xscale;
static fixed yscale;
static int mapscale = 2;
static int oldw, oldh;
static byte *skytile;
static int mapcolor = 8;

typedef struct PType
{
    int x;
    int y;
} Ptype;

static Ptype arrows[8][7] =
{
    { {4,2}, {2,4}, {2,3}, {0,3}, {0,1}, {2,1}, {2,0} },
    { {4,0}, {4,3}, {3,2}, {1,4}, {0,3}, {2,1}, {1,0} },
    { {2,0}, {4,2}, {3,2}, {3,4}, {1,4}, {1,2}, {0,2} },
    { {0,0}, {3,0}, {2,1}, {4,3}, {3,4}, {1,2}, {0,3} },
    { {0,2}, {2,0}, {2,1}, {4,1}, {4,3}, {2,3}, {2,4} },
    { {0,4}, {0,1}, {1,2}, {3,0}, {4,1}, {2,3}, {3,4} },
    { {2,4}, {0,2}, {1,2}, {1,0}, {3,0}, {3,2}, {4,2} },
    { {4,4}, {1,4}, {2,3}, {0,1}, {1,0}, {3,2}, {4,1} },
};

void DrawMap_MaskedShape (int x, int y, int lump, int type);

void CheatMap (void)
{
    int i;
    int j;
    statobj_t *temp;
    objtype *a;

    for(temp = FIRSTSTAT; temp; temp = temp->statnext)
        temp->flags |= FL_SEEN;

    for(a = FIRSTACTOR; a; a = a->next)
        a->flags |= FL_SEEN;

    for (j = 0; j < MAPSIZE; j++)
        for (i = 0; i < MAPSIZE; i++)
            mapseen[i][j] = 1;
}


void FixMapSeen (void)
{
    int i;
    int j;

    for (j = 0; j < MAPSIZE; j++)
        for (i = 0; i < MAPSIZE; i++)
            if (!mapseen[i][j])
            {
                if (i == 0 &&
                        ((mapseen[i][j + 1] && j < MAPSIZE - 1) || (mapseen[i][j - 1] && j > 0)) &&
                        mapseen[i + 1][j])
                    mapseen[i][j]=1;
                else if (i == MAPSIZE - 1 && mapseen[i - 1][j] &&
                         ((mapseen[i][j + 1] && j < MAPSIZE - 1) || (mapseen[i][j - 1] && j > 0)))
                    mapseen[i][j]=1;
                else if (j == 0 &&
                         ((mapseen[i + 1][j] && i<MAPSIZE - 1) || (mapseen[i-1][j] && i>0)) &&
                         mapseen[i][j+1])
                    mapseen[i][j]=1;
                else if (j == MAPSIZE - 1 &&
                         mapseen[i][j - 1] &&
                         ((mapseen[i+1][j] && i<MAPSIZE-1) || (mapseen[i-1][j] &&
                                 i > 0)))
                    mapseen[i][j]=1;
                else if ((((mapseen[i - 1][j]) && (mapseen[i][j + 1]) && (!(tilemap[i - 1][j + 1]))) ||
                          ((mapseen[i - 1][j]) && (mapseen[i][j - 1]) && (!(tilemap[i - 1][j - 1]))) ||
                          ((mapseen[i + 1][j]) && (mapseen[i][j + 1]) && (!(tilemap[i + 1][j + 1]))) ||
                          ((mapseen[i + 1][j]) && (mapseen[i][j - 1]) && (!(tilemap[i + 1][j - 1])))) &&
                         tilemap[i][j])
                    mapseen[i][j]=1;
            }
}


/*
=======================
=
= DrawMap_Wall
=
=======================
*/

void DrawMap_Wall (int x, int y, int tile)
{
    byte *buf;
    byte *b;
    byte *source;
    byte *s;
    int i;

    x *= tilesize;
    y *= tilesize;

    buf = (byte *)bufferofs + ylookup[y] + x;

    source = W_CacheLumpNum(tile, PU_CACHE, CvtNull, 1);

    s = source;
    b = buf;

    for (i = 0; i < tilesize; i++, b++)
    {
        DrawMapPost(tilesize, s, b);
        s += hp_srcstep >> 10;
    }
}

/*
=======================
=
= DrawMap_AnimatedWall
=
=======================
*/

void DrawMap_AnimatedWall (int x, int y, int tile)
{
    DrawMap_Wall(x, y, animwalls[tile].texture);
}

/*
=======================
=
= DrawMap_SkyTile
=
=======================
*/

void DrawMap_SkyTile (int x, int y)
{
    byte *buf;
    byte *b;
    byte *s;
    int i;

    x *= tilesize;
    y *= tilesize;

    buf = (byte *)bufferofs + ylookup[y] + x;

    s = skytile;
    b = buf;

    for (i = 0; i < tilesize; i++, b++)
    {
        DrawMapPost(tilesize, s, b);
        s += hp_srcstep >> 10;
    }
}

/*
=======================
=
= DrawMap_MaskedWall
=
=======================
*/

void DrawMap_MaskedWall (int x, int y, int tile)
{
    if (IsPlatform(maskobjlist[tile]->tilex,maskobjlist[tile]->tiley))
    {
        if (!(maskobjlist[tile]->flags & MW_ABOVEPASSABLE))
            DrawMap_MaskedShape(x, y, maskobjlist[tile]->toptexture, 0);
        else if (!(maskobjlist[tile]->flags & MW_BOTTOMPASSABLE))
            DrawMap_MaskedShape(x, y, maskobjlist[tile]->bottomtexture, 1);
        else
            DrawMap_MaskedShape(x, y, maskobjlist[tile]->midtexture, 0);
    }
    else
        DrawMap_MaskedShape(x, y, maskobjlist[tile]->bottomtexture, 1);
}

/*
=======================
=
= DrawMap_Door
=
=======================
*/

void DrawMap_Door (int x, int y, int tile)
{
    if ((doorobjlist[tile]->lock > 0) && (doorobjlist[tile]->lock <= 4))
        DrawMap_Wall(x, y, W_GetNumForName("lock1") + doorobjlist[tile]->lock - 1);
    else if (doorobjlist[tile]->texture==doorobjlist[tile]->basetexture)
        DrawMap_Wall(x, y, doorobjlist[tile]->texture);
    else
        DrawMap_MaskedShape(x, y, doorobjlist[tile]->texture, 0);
}

/*
=======================
=
= DrawMap_PushWall
=
=======================
*/

void DrawMap_PushWall (int x, int y, pwallobj_t *pw)
{
    if (pw->texture & 0x1000)
        DrawMap_AnimatedWall(x, y, pw->texture & 0x3ff);
    else
        DrawMap_Wall(x, y, pw->texture & 0x3ff);
}

/*
=======================
=
= DrawMap_Actor
=
=======================
*/

void DrawMap_Actor (int x, int y, objtype *a)
{
    int translucent;

    if (!(a->flags & FL_SEEN))
        return;

    translucent = 0;

    if (a->flags & FL_TRANSLUCENT)
        translucent = 1;

    DrawMap_MaskedShape(x, y, a->shapenum + shapestart, translucent);
}

/*
=======================
=
= DrawMap_Sprite
=
=======================
*/

void DrawMap_Sprite (int x, int y, statobj_t *s)
{
    int translucent;

    if (!(s->flags & FL_SEEN))
        return;

    translucent = 0;

    if (s->flags & FL_TRANSLUCENT)
        translucent = 1;

    DrawMap_MaskedShape(x, y, s->shapenum + shapestart, translucent);
}

/*
=======================
=
= DrawMap_MaskedShape
=
=======================
*/

void DrawMap_MaskedShape (int x, int y, int lump, int type)
{
    // Calculate center coordinates for sprites
    x *= tilesize;
    y *= tilesize;
    x += tilesize >> 1;
    y += tilesize >> 1;
    DrawPositionedScaledSprite(x, y, lump, tilesize, type);
}

/*
=======================
=
= DrawMap_PlayerArrow
=
=======================
*/

/* Indices: mapscale, reduced coordinate */
static const int arrowscale[4][5] =
{
    { 1,17,32,47,63},  /* Mapscale 0: 64 pixels/sprite */
    { 1, 9,16,23,31},  /* Mapscale 1: 32 pixels/sprite */
    { 1, 5, 8,11,15},  /* Mapscale 2: 16 pixels/sprite */
    { 1, 3, 4, 5, 7}
}; /* Mapscale 3:  8 pixels/sprite */

void DrawMap_PlayerArrow (int x, int y, int dir)
{
    int i;

    x *= tilesize;
    y *= tilesize;

    /* You can't draw a 4x4 arrow */
    if(mapscale == 4)
    {
        VL_Bar(x + screen_src_minscale, y + screen_src_minscale,
               2 * screen_src_minscale, 2 * screen_src_minscale, 244);
        return;
    }

    for (i = 0; i < 6; i++)
    {
        VL_DrawLine(arrowscale[mapscale][arrows[dir][i].x] + x,
                    arrowscale[mapscale][arrows[dir][i].y] + y,
                    arrowscale[mapscale][arrows[dir][i+1].x] + x,
                    arrowscale[mapscale][arrows[dir][i+1].y] + y,
                    244);
    }

    VL_DrawLine(arrowscale[mapscale][arrows[dir][6].x] + x,
                arrowscale[mapscale][arrows[dir][6].y] + y,
                arrowscale[mapscale][arrows[dir][0].x] + x,
                arrowscale[mapscale][arrows[dir][0].y] + y,
                244);
}

/*
=======================
=
= DrawMap_Player
=
=======================
*/

void DrawMap_Player (int x, int y)
{
    if (player->flags & FL_SHROOMS)
        DrawMap_PlayerArrow(x, y, (RandomNumber("DrawMap_PLAYER", 0) >> 5));
    else
        DrawMap_PlayerArrow(x, y, (((player->angle + (FINEANGLES / 16)) & (FINEANGLES - 1)) >> 8));

    DrawMap_MaskedShape(x, y, player->shapenum + shapestart, 0);
}

/*
=======================
=
= DrawMap
=
=======================
*/

void DrawMap (int cx, int cy)
{
    fixed x, y;
    statobj_t *s;
    objtype *a;
    int i, j;
    int mapx, mapy;
    int wall;

    // Clear buffer
    VL_ClearBuffer(bufferofs, egacolor[mapcolor]);

    x = cx >> 16;
    y = cy >> 16;

    // Draw Walls,Doors,maskedwalls,animatingwalls
    for (j = 0; j < yscale; j++)
    {
        // Don't go off the bottom of the map
        mapy = j + y;

        if (mapy < 0)
            continue;

        if (mapy > 127)
            break;

        for (i = 0; i < xscale; i++)
        {
            // Don't go off the right side of the map
            mapx = i + x;

            if (mapx < 0)
                continue;

            if (mapx > 127)
                break;

            if ((mapx == player->tilex) && (mapy == player->tiley))
            {
                DrawMap_Player(i, j);
                continue;
            }

            wall = tilemap[mapx][mapy];

            // Check for absence of wall
            if (wall)
            {

                if (!mapseen[mapx][mapy])
                    continue;

                // Check to see if it is a door or masked wall
                if (wall & 0x8000)
                {
                    if (wall & 0x4000)
                        DrawMap_MaskedWall(i, j, wall & 0x3ff);
                    else
                        DrawMap_Door(i, j, wall & 0x3ff);

                    // Check to see if it is an animating wall
                }
                else if (wall & 0x1000)
                    DrawMap_AnimatedWall(i, j, wall & 0x3ff);
                else if (IsWindow(mapx, mapy))
                {
                    if (sky != 0)
                        DrawMap_SkyTile(i, j);
                    else
                        Error("Trying to draw a sky on a level without sky\n");

                    // Must be a normal wall or a wall with something above
                }
                else
                    DrawMap_Wall(i, j, wall & 0x3ff);
            }
            else
            {
                a = actorat[mapx][mapy];

                // Check for absence of actor

                if (a)
                {
                    switch(a->which)
                    {
                        case PWALL:
                            if (!mapseen[mapx][mapy])
                                continue;

                            DrawMap_PushWall(i, j, (pwallobj_t *)a);
                            break;
                        case ACTOR:
                            DrawMap_Actor(i, j, a);
                            break;
                        case SPRITE:
                            DrawMap_Actor(i, j, a);
                            break;
                        default:
                            SoftError("Unable to resolve actorat at x=%d y=%d which=%d\n",
                                      mapx, mapy, a->which);
                            break;
                    }
                }
                else
                {
                    s = sprites[mapx][mapy];

                    // Check for absence of sprite

                    if (s)
                        DrawMap_Sprite(i, j, s);
                }
            }
        }
    }
}




/*
=======================
=
= SetupFullMap
=
=======================
*/

void SetupFullMap (void)
{
    int y;
    int startx, starty, width, endy;
    pic_t *pic;

    // Fill in backgrounds
    pic = (pic_t *) W_CacheLumpNum (W_GetNumForName ("mmbk"), PU_CACHE, Cvt_pic_t, 1);
    VL_DrawPicFullscreen(pic);
    CheckHolidays();

    // Clear area for map
    startx = (screen_src_width - (128 * screen_src_minscale)) / 2.0; // 96, 128, 96
    starty = 37 * screen_src_minscale;
    width = 128 * screen_src_minscale;
    endy = starty + (127 * screen_src_minscale);

    for (y = starty; y < endy; y++)
        memset((byte *)bufferofs + ylookup[y] + startx, 0, width);
}

/*
=======================
=
= DrawFullMap
=
=======================
*/
void DrawFullMapPixel (int color, byte *buf, int width, int height);

void DrawFullMap (void)
{
    statobj_t *s;
    objtype *a;
    int mapx, mapy;
    int wall;
    byte *buf;
    int bufystart, bufxstart;
    int i;

    SetupFullMap();

    bufxstart = (screen_src_width - (128 * screen_src_minscale)) / 2.0; // 96, 128, 96
    bufystart = 37 * screen_src_minscale;

    i = bufxstart;
    // Draw Walls,Doors,maskedwalls,animatingwalls
    for (mapx = 0; mapx < mapwidth; i += xscalelookup[mapx], mapx++)
    {
        buf = (byte *)bufferofs + ylookup[bufystart] + i;

        for (mapy = 0; mapy < mapheight; buf += screen_src_width * yscalelookup[mapy], mapy++)
        {
            if ((mapx == player->tilex) && (mapy == player->tiley))
            {
                DrawFullMapPixel(egacolor[MAP_PLAYERCOLOR], buf, xscalelookup[mapx], yscalelookup[mapy]);
                continue;
            }

            wall = tilemap[mapx][mapy];

            // Check for absence of wall
            if (wall)
            {

                if (!mapseen[mapx][mapy])
                    continue;

                if (wall & 0x8000)
                {
                    if (wall & 0x4000)
                    {
                        DrawFullMapPixel(egacolor[MAP_MWALLCOLOR], buf,
                                         xscalelookup[mapx], yscalelookup[mapy]); // Masked wall
                    }
                    else
                    {
                        DrawFullMapPixel(egacolor[MAP_DOORCOLOR], buf,
                                         xscalelookup[mapx], yscalelookup[mapy]); // Door
                    }
                }
                else if (wall & 0x1000)
                    DrawFullMapPixel(egacolor[MAP_AWALLCOLOR], buf,
                                     xscalelookup[mapx], yscalelookup[mapy]); // Animated wall
                else if (IsWindow(mapx, mapy))
                {
                    if (sky != 0)
                        DrawFullMapPixel(egacolor[MAP_SKYCOLOR], buf,
                                         xscalelookup[mapx], yscalelookup[mapy]); // Sky
                    else
                        Error("Trying to draw a sky on a level without sky\n");
                }
                else
                    DrawFullMapPixel(egacolor[MAP_WALLCOLOR], buf,
                                     xscalelookup[mapx], yscalelookup[mapy]); // Normal wall
            }
            else
            {
                a = actorat[mapx][mapy];

                // Check for absence of actor
                if (a)
                {
                    switch(a->which)
                    {
                        case PWALL:
                            if (!mapseen[mapx][mapy])
                                continue;

                            DrawFullMapPixel(egacolor[MAP_PWALLCOLOR], buf,
                                             xscalelookup[mapx], yscalelookup[mapy]);
                            break;

                        case ACTOR:
                            if (a->flags & FL_SEEN)
                            {
                                if (a->obclass == inertobj)
                                    DrawFullMapPixel(egacolor[MAP_SPRITECOLOR], buf,
                                                     xscalelookup[mapx], yscalelookup[mapy]);
                                else
                                    DrawFullMapPixel(egacolor[MAP_ACTORCOLOR], buf,
                                                     xscalelookup[mapx], yscalelookup[mapy]);
                            }
                            break;

                        case SPRITE:
                            if (a->flags & FL_SEEN)
                                DrawFullMapPixel(egacolor[MAP_SPRITECOLOR], buf,
                                                 xscalelookup[mapx], yscalelookup[mapy]);
                            break;

                        default:
                            SoftError("Unable to resolve actorat at x=%d y=%d which=%d\n",
                                      mapx, mapy, a->which);
                            break;
                    }
                }
                else
                {
                    s = sprites[mapx][mapy];

                    // Check for absence of sprite

                    if (s && (s->flags & FL_SEEN))
                        DrawFullMapPixel(egacolor[MAP_SPRITECOLOR], buf,
                                         xscalelookup[mapx], yscalelookup[mapy]);
                }
            }
        }
    }

    FlipPage();
}

void DrawFullMapPixel (int color, byte *buf, int width, int height)
{
    int x, y;

    for (x = 0; x < width; x++)
        for (y = 0; y < height; y++)
            *(buf + (screen_src_width * y) + x) = color;
}

/*
=======================
=
= DrawMapInfo
=
=======================
*/

void DrawMapInfo (void)
{
    char temp[80];
    int width, height;

    CurrentFont = tinyfont;

    PrintX = 2 * screen_src_minscale;
    PrintY = 2 * screen_src_minscale;
    strcpy(&temp[0], &(LevelName[0]));
    US_MeasureStr(&width, &height, "%s", &temp[0]);

    VL_TBar (0, 0, screen_src_width, (height+4) * screen_src_minscale);

    US_BufPrint(&temp[0]);

    strcpy(&temp[0], "TAB=EXIT");
    US_MeasureStr(&width, &height, "%s", &temp[0]);

    PrintX = screen_src_width - ((4 + width) * screen_src_minscale);
    PrintY = 2 * screen_src_minscale;

    US_BufPrint (&temp[0]);

    strcpy(&temp[0], "< > CHANGE BACKGROUND COLOR");
    US_MeasureStr(&width, &height, "%s", &temp[0]);

    PrintX = (int)(screen_src_width - (width * screen_src_minscale)) >> 1;
    PrintY = 2 * screen_src_minscale;

    US_BufPrint(&temp[0]);
}

/*
=======================
=
= SetupMapScale
=
=======================
*/

void SetupMapScale (int s)
{
    mapscale = s;
    tilesize = 64 >> mapscale;
    xscale = screen_src_width / tilesize;
    yscale = screen_src_height / tilesize;
    hp_srcstep = 0x10000 << mapscale;
}


/*
=======================
=
= ChangeMapScale
=
=======================
*/

void ChangeMapScale (int *newx, int *newy, int newmapscale)
{
    if (newmapscale < 0 || newmapscale > FULLMAP_SCALE)
        return;

    if (newmapscale == FULLMAP_SCALE)
        DrawFullMap();

    *newx = *newx + (xscale << 15);
    *newy = *newy + (yscale << 15);

    SetupMapScale(newmapscale);

    *newx = *newx - (xscale << 15);
    *newy = *newy - (yscale << 15);
}


/*
=======================
=
= SetupMapper
=
=======================
*/
void SetupMapper (void)
{
    FixMapSeen();

    tics = 0;
    oldw = viewwidth;
    oldh = viewheight;
    viewwidth = screen_src_width;
    viewheight = screen_src_height;

    if (sky != 0)
    {
        skytile = SafeMalloc(64*64);
        MakeSkyTile(skytile);
    }
}

/*
=======================
=
= ShutdownMapper
=
=======================
*/
void ShutdownMapper (void)
{
    VL_ClearVideo(0);
    viewwidth = oldw;
    viewheight = oldh;
    SetupScreen(true);

    if (sky != 0)
        SafeFree(skytile);

    if (mouseenabled && MousePresent)
        PollMouseMove();
}

/*
=======================
=
= DoMap
=
=======================
*/

void DoMap (int cx, int cy)
{
    int x, y;
    int dx, dy;
    boolean done;
    int quitkey;
    ControlInfo control;

    ShutdownClientControls();

    done = false;

    while (Keyboard[sc_Tab])
        IN_UpdateKeyboard();

    x = (cx - (xscale >> 1)) << 16;
    y = (cy - (yscale >> 1)) << 16;

    SetupMapper();

    transparentlevel = 25;

    ChangeMapScale(&x, &y, mapscale);

    while (!done)
    {
        IN_UpdateKeyboard();

        if (Keyboard[sc_Tab] || Keyboard[sc_Escape])
        {
            if (Keyboard[sc_Tab])
                quitkey = sc_Tab;
            else
                quitkey = sc_Escape;

            done = true;
        }

        if (Keyboard[sc_Home])
        {
            x = (cx - (xscale >> 1)) << 16;
            y = (cy - (yscale >> 1)) << 16;
        }

        dx = 0;
        dy = 0;

        if (mapscale == FULLMAP_SCALE)
            CalcTics();
        else
        {
            DrawMap(x,y);
            DrawMapInfo();
            FlipPage();
            CalcTics();
            DoSprites();
            AnimateWalls();
        }

        ReadAnyControl(&control);

        if (Keyboard[sc_PgUp] || Keyboard[sc_Plus] || control.button1)
        {
            ChangeMapScale(&x, &y, mapscale + 1);

            while(Keyboard[sc_PgUp])
                IN_UpdateKeyboard ();
            while(Keyboard[sc_Plus])
                IN_UpdateKeyboard ();
            while(control.button1)
                ReadAnyControl(&control);
        }

        if (Keyboard[sc_PgDn] || Keyboard[sc_Minus] || control.button0)
        {
            ChangeMapScale(&x, &y, mapscale - 1);

            while(Keyboard[sc_PgDn])
                IN_UpdateKeyboard ();
            while(Keyboard[sc_Minus])
                IN_UpdateKeyboard ();
            while(control.button0)
                ReadAnyControl(&control);
        }

        if (Keyboard[sc_CapsLock] && Keyboard[sc_C])
        {
            inhmenu = true;
            SaveScreen(true);
            inhmenu = false;
        }

        if (Keyboard[sc_CapsLock] && Keyboard[sc_X])
        {
            inhmenu = true;
            SaveScreen(false);
            inhmenu = false;
        }

        if (Keyboard[sc_Comma])
        {
            if (mapcolor > 0)
                mapcolor--;

            while(Keyboard[sc_Comma])
                IN_UpdateKeyboard();
        }

        if (Keyboard[sc_Period])
        {
            if (mapcolor < 15)
                mapcolor++;

            while(Keyboard[sc_Period])
                IN_UpdateKeyboard ();
        }

        if (mapscale != FULLMAP_SCALE)
        {
            if (control.dir == dir_East)
                dx = (tics << 17) / (5 - mapscale);
            if (control.dir == dir_West)
                dx = -(tics << 17) / (5 - mapscale);
            if (control.dir == dir_South)
                dy = (tics << 17) / (5 - mapscale);
            if (control.dir == dir_North)
                dy = -(tics << 17) / (5 - mapscale);
        }

#if (DEVELOPMENT == 1)
        if (Keyboard[sc_M])
        {
            CheatMap();
            ChangeMapScale(&x, &y, mapscale);
        }
#endif

        x += dx;
        y += dy;

        if (x > 0x7effff)
            x = 0x7effff;
        else if (x < -(xscale << 15))
            x = -(xscale << 15);

        if (y > 0x7effff)
            y = 0x7effff;
        else if (y < -(yscale << 15))
            y = -(yscale << 15);
    }

    while (Keyboard[quitkey])
        IN_UpdateKeyboard();

    LastScan = 0;
    Keyboard[sc_Escape] = 0;
    Keyboard[sc_Tab] = 0;

    ShutdownMapper();

    StartupClientControls();
}

