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
#include "rt_view.h"
#include "z_zone.h"
#include "w_wad.h"
#include "lumpy.h"
#include "rt_util.h"
#include "rt_vid.h"
#include "rt_game.h"
#include "rt_draw.h"
#include "rt_ted.h"
#include "isr.h"
#include "rt_rand.h"
#include "rt_sound.h"
#include "modexlib.h"
#include "rt_menu.h"

#include <stdlib.h>

#include "rt_main.h"
#include "rt_battl.h"
#include "rt_floor.h"
#include "rt_str.h"
#include "watcom.h"
#include "develop.h"

#define LIGHTNINGLEVEL 4
#define MINLIGHTNINGLEVEL   2
#define MAXLIGHTNINGLEVEL   10

/*
=============================================================================

                               GLOBALS

=============================================================================
*/

int    StatusBar = 0;
int    lightninglevel=0;
boolean  lightning=false;
int    normalshade;
int    darknesslevel;
int    maxshade;
int    minshade;
int    baseminshade;
int    basemaxshade;
int    viewheight;
int    viewheightunscaled;
int    viewwidth;
int    viewwidthunscaled;
longword   heightnumerator;
fixed  scale;
int    screenofs;
int    centerx;
int    centerxunscaled;
int    centery;
int    centeryunscaled;
int    centeryfrac;
int    fulllight;
int    weaponscale;
int    viewsize;
byte *colormap;
byte *redmap;
byte *greenmap;
byte *playermaps[MAXPLAYERCOLORS];
short  pixelangle[MAXSCREENWIDTH];
byte   gammatable[GAMMAENTRIES];
int    gammaindex;
int    focalwidth=160;
int    yzangleconverter;
byte   uniformcolors[MAXPLAYERCOLORS]=
{
    25,
    222,
    29,
    206,
    52,
    6,
    155,
    16,
    90,
    129,
    109
};



/*
=============================================================================

                               LOCAL

=============================================================================
*/

static char *YourComputerSucksString = "Buy a 486! :)";

static int viewsizes[MAXVIEWSIZES*2]= { 80,48,
                                        128,72,
                                        160,88,
                                        192,104,
                                        224,120,
                                        256,136,
                                        288,152,
                                        320,168,
                                        320,184,
                                        320,200,
                                        320,200
                                      };

static int initialViewSizes[MAXVIEWSIZES*2]= { 80,48,
        128,72,
        160,88,
        192,104,
        224,120,
        256,136,
        288,152,
        320,168,
        320,184,
        320,200,
        320,200
                                             };

static int ColorMapLoaded=0;

static int      lightningtime=0;
static int      lightningdelta=0;
static int      lightningdistance=0;
static int      lightningsoundtime=0;
static boolean  periodic=false;
static int      periodictime=0;

void SetViewDelta ( void );
void UpdatePeriodicLighting (void);

/*
====================
=
= ResetFocalWidth
=
====================
*/
void ResetFocalWidth ( void )
{
    focalwidth=FOCALWIDTH;
    SetViewDelta();
}

/*
====================
=
= ChangeFocalWidth
=
====================
*/
void ChangeFocalWidth ( int amount )
{
    focalwidth=FOCALWIDTH+amount;
    SetViewDelta();
}


/*
====================
=
= SetViewDelta
=
====================
*/

void SetViewDelta ( void )
{
//screen_dest_height
//screen_src_width
//
// calculate scale value for vertical height calculations
// and sprite x calculations
//
    //scale = ((int)(centerxunscaled*screen_src_hscale)*focalwidth)/(160);
    scale = ((int)(centerxunscaled*screen_src_hscale)*focalwidth)/(160);
//
// divide heightnumerator by a posts distance to get the posts height for
// the heightbuffer.  The pixel height is height>>HEIGHTFRACTION
//
    heightnumerator = (((focalwidth/10)*(int)(centerxunscaled*screen_src_hscale)*4096)<<HEIGHTFRACTION);
}

/*
====================
=
= CalcProjection
=
====================
*/

void CalcProjection ( void )
{
    int   i, j, scleft, scright, unscleft, unscright;
    int   frac;
    int   intang;
    byte *table;
    byte *ptr;
    int   length;
    int   *pangle;



// Already being called in ResetFocalWidth
//    SetViewDelta();

//
// load in tables file
//


//Hey, isn't this stuff already loaded in?
//Why don't we make this a lump?
    table=W_CacheLumpName("tables",PU_STATIC, CvtNull, 1);
    ptr=table;

//
// get size of table
//

    memcpy(&length,ptr,sizeof(int));
    SwapIntelLong(&length);
    ptr+=sizeof(int);
    pangle=SafeMalloc(length*sizeof(int));
    memcpy(pangle,ptr,length*sizeof(int));

    frac=((length*65536/centerxunscaled))>>1;

    // FIXME: NOT ACCURATE.
    scleft = centerx - 1;
    scright = centerx;
    intang = 0;

    for (i = 0; i < centerxunscaled; i++)
    {
        // start 1/2 pixel over, so viewangle bisects two middle pixels
        intang = pangle[frac>>16];
        SwapIntelLong(&intang);

        for (j = 0; j < xscalelookup[centerxunscaled-1-i]; j++)
        {
            pixelangle[scleft--] = (short)intang;
        }

        for (j = 0; j < xscalelookup[centerxunscaled+i]; j++)
        {
            pixelangle[scright++] = (short)-intang;
        }

        frac += (length*65536/centerxunscaled);
    }

    if (centerx - (centerxunscaled * screen_src_wscale) > 0)
    {
        i = 0;
        unscleft = 0;
        while (i < scleft)
        {
            i += xscalelookup[unscleft];
            unscleft++;
        }

        i = 0;
        unscright = 0;
        while (i < scright)
        {
            i += xscalelookup[unscright];
            unscright++;
        }

        // TODO: Fix center misalignment.
        scright = 0;
        for (i = 0; i < unscright; i++)
            scright += xscalelookup[i];

        intang++;

        while (scleft > 0 || scright < viewwidth)
        {
            for (j = 0; j < xscalelookup[unscleft-1]; j++)
            {
                if (scleft > 0)
                    pixelangle[scleft--] = intang;
            }

            for (j = 0; j < xscalelookup[unscright]; j++)
            {
                if (scright < viewwidth)
                    pixelangle[scright++] = -intang;
            }

            intang++;
            unscleft--;

            // TODO: Fix this. In fact, fix the misalignment of the sky with xscalelookup.
            //if (xscalelookup[unscright+1] > 0)
            unscright++;
        }

        pixelangle[0] = intang;
    }

    table=W_CacheLumpName("tables",PU_CACHE, CvtNull, 1);
    SafeFree(pangle);
}


/*
==========================
=
= SetViewSize
=
==========================
*/

void SetViewSize(int size)
{
    int height;
    int maxheight;
    int screenx;
    int screeny;
    int topy;
    int i;

    // Generate the proper viewsizes for the current resolution.
    for (i = 0; i < MAXVIEWSIZES; i++)
    {
        viewsizes[i<<1] =
            initialViewSizes[i<<1] * screen_src_wscale;
        viewsizes[(i<<1) + 1] =
            initialViewSizes[(i<<1) + 1] * screen_src_hscale;
    }

    if ((size<0) || (size>=MAXVIEWSIZES))  //bna added
    {
        printf("Illegal screen size = %d\n",size);
        size = 8;//set default value
        viewsize = 8;
    }

    viewwidth = viewsizes[size<<1];
    viewwidthunscaled = initialViewSizes[size<<1]; // must be divisable by 16
    viewheight = viewsizes[(size<<1) + 1];
    viewheightunscaled = initialViewSizes[(size<<1) + 1]; // must be even

    maxheight = screen_src_height;
    topy = 0;

    // Only keep the kills flag
    StatusBar &= ~( BOTTOM_STATUS_BAR | TOP_STATUS_BAR
                    | STATUS_PLAYER_STATS );

    // Bottom Kill Boxes
    if (SHOW_KILLS())
        maxheight -= 24 * screen_src_minscale;

    // Top Status Bar
    if (size < 9)
    {
        StatusBar |= TOP_STATUS_BAR;
        maxheight -= 16 * screen_src_minscale;
        topy      += 16 * screen_src_minscale;
    }

    // Bottom Status Bar
    if (size < 8)
    {
        StatusBar |= BOTTOM_STATUS_BAR;
        maxheight -= 16 * screen_src_minscale;
    }
    else if (size < 10)
        StatusBar |= STATUS_PLAYER_STATS;

    height = viewheight;

    // Prevent over-scaling weapons.
    if (size > 7)
        height = viewsizes[15]; // [(7<<1)+1]

    weaponscale = (height << 16) / initialViewSizes[15]; // [(7<<1)+1]

    i = 0;
    centerx = 0;
    while (centerx < viewwidth >> 1)
    {
        centerx += xscalelookup[i];
        i++;
    }
    centerxunscaled = viewwidthunscaled >> 1;

    i = 0;
    centery = 0;
    while (centery < viewheight >> 1)
    {
        centery += yscalelookup[i];
        i++;
    }

    centeryunscaled = viewheightunscaled >> 1;
    centeryfrac = (centeryunscaled << 16);

    yzangleconverter = ( 0xaf85 * viewheightunscaled ) / 200.0;

    // Center the view horizontally
    screenx = (screen_src_width - viewwidth) >> 1;

    // Center the view vertically
    if (viewheight >= maxheight)
    {
        screeny = topy;
        viewheight = maxheight;
    }
    else
    {
        screeny = ( ( maxheight - viewheight ) >> 1 ) + topy;
    }

    // TODO: Do we need screenofs?
    // Calculate offset of view window
    screenofs = screenx + ylookup[ screeny ];

//
// calculate trace angles and projection constants
//

    ResetFocalWidth();


// Already being called in ResetFocalWidth
//   SetViewDelta();


    CalcProjection();

}


//******************************************************************************
//
// DrawCPUJape ()
//
//******************************************************************************

void DrawCPUJape (void)
{
    int width;
    int height;

    CurrentFont = tinyfont;
    VW_MeasurePropString(YourComputerSucksString, &width, &height);

    // Buy a 486. ;)
    DrawGameString((screen_src_width / 2) - ((width * screen_src_minscale) / 2),
                   (screen_src_height / 2) + (int)(26 * screen_src_minscale), // 100 + (48 / 2) + 2
                   YourComputerSucksString, true );
}


/*
==========================
=
= SetupScreen
=
==========================
*/

void SetupScreen (boolean flip)
{
    pic_t *shape;

    SetViewSize(viewsize);

    shape = (pic_t *)W_CacheLumpName("backtile", PU_CACHE, Cvt_pic_t, 1);
    DrawTiledRegion(0, 0, screen_src_width, screen_src_height, 0, 0, shape);

    if (viewsize == 0)
        DrawCPUJape();

    DrawPlayScreen(true);

    if (flip == true)
    {
        ThreeDRefresh();
        VL_CopyDisplayToHidden();
    }
}



void LoadColorMap( void )
{
    int i,j;
    int lump, length;

    if (ColorMapLoaded==1)
        Error("Called LoadColorMap twice\n");
    ColorMapLoaded=1;
//
//   load in the light tables
//   256 byte align tables
//

    lump = W_GetNumForName("colormap");
    length = W_LumpLength (lump) + 255;
    colormap = SafeMalloc (length);
    colormap = (byte *)( ((long)colormap + 255)&~0xff);
    W_ReadLump (lump,colormap);

// Fix fire colors in colormap

    for (i=31; i>=16; i--)
        for (j=0xea; j<0xf9; j++)
            colormap[i*256+j]=colormap[(((i-16)/4+16))*256+j];

// Get special maps

    lump = W_GetNumForName("specmaps");
    length = W_LumpLength (lump+1) + 255;
    redmap = SafeMalloc (length);
    redmap = (byte *)( ((long)redmap + 255)&~0xff);
    W_ReadLump (lump+1,redmap);
    greenmap = redmap+(16*256);

// Get player colormaps

//   if (modemgame==true)
    {
        lump = W_GetNumForName("playmaps")+1;
        for (i=0; i<MAXPLAYERCOLORS; i++)
        {
            length = W_LumpLength (lump+i) + 255;
            playermaps[i] = SafeMalloc (length);
            playermaps[i] = (byte *)( ((long)playermaps[i] + 255)&~0xff);
            W_ReadLump (lump+i,playermaps[i]);
        }
    }

    if (!quiet)
        printf("RT_VIEW: Colormaps Initialized\n");

}

/*
==========================
=
= SetupLightLevels
=
==========================
*/
#define LIGHTRATEBASE 252
#define LIGHTRATEEND  267
#define LIGHTLEVELBASE 216
#define LIGHTLEVELEND  223
void SetupLightLevels ( void )
{
    int glevel;

    periodic=false;
    fog=0;
    lightsource=0;

// Set up light level for level

    if (((word)MAPSPOT(2,0,1)>=104) && ((word)MAPSPOT(2,0,1)<=105))
        fog=(word)MAPSPOT(2,0,1)-104;
    else
        Error ("There is no Fog icon on map %d\n",gamestate.mapon);
    if ((word)MAPSPOT(3,0,1)==139)
    {
        if (fog==0)
        {
            lightsource=1;
            lights=Z_Malloc(MAPSIZE*MAPSIZE*(sizeof(unsigned long)),PU_LEVEL,NULL);
            memset (lights,0,MAPSIZE*MAPSIZE*(sizeof(unsigned long)));
        }
        else
            Error("You cannot use light sourcing on a level with fog on map %d\n",gamestate.mapon);
    }
    else if ((word)MAPSPOT(3,0,1))
        Error("You must use the lightsource icon or nothing at all at (3,0) in plane 1 on map %d\n",gamestate.mapon);
    if (((word)MAPSPOT(2,0,0)>=LIGHTLEVELBASE) && ((word)MAPSPOT(2,0,0)<=LIGHTLEVELEND))
        glevel=(MAPSPOT(2,0,0)-LIGHTLEVELBASE);
    else
        Error("You must specify a valid darkness level icon at (2,0) on map %d\n",gamestate.mapon);

    SetLightLevels ( glevel );

    if (((word)MAPSPOT(3,0,0)>=LIGHTRATEBASE) && ((word)MAPSPOT(3,0,0)<=LIGHTRATEEND))
        glevel=(MAPSPOT(3,0,0)-LIGHTRATEBASE);
    else
    {
//      Error("You must specify a valid darkness rate icon at (3,0) on map %ld\n",gamestate.mapon);
        glevel = 4;
    }

    SetLightRate ( glevel );
    lightningtime=0;
    lightningdistance=0;
    lightninglevel=0;
    lightningdelta=0;
    lightningsoundtime=0;
}

/*
==========================
=
= SetLightLevels
=
==========================
*/
void SetLightLevels ( int darkness )
{
    if (fog==0)
    {
        baseminshade=0x10+((7-darkness)>>1);
        basemaxshade=0x1f-(darkness>>1);
    }
    else
    {
        baseminshade=darkness;
        basemaxshade=0x10;
    }
    minshade=baseminshade;
    maxshade=basemaxshade;
    darknesslevel=darkness;
}

/*
==========================
=
= GetLightLevelTile
=
==========================
*/
int GetLightLevelTile ( void )
{
    if (fog==0)
    {
        return ((7-((baseminshade-0x10)<<1))+LIGHTLEVELBASE);
    }
    else
    {
        return (baseminshade+LIGHTLEVELBASE);
    }
}


/*
==========================
=
= SetLightRate
=
==========================
*/
void SetLightRate ( int rate )
{
    normalshade=(HEIGHTFRACTION+8)-rate;
    if (normalshade>14) normalshade=14;
    if (normalshade<3) normalshade=3;
}

/*
==========================
=
= GetLightRate
=
==========================
*/
int GetLightRate ( void )
{
    return ((HEIGHTFRACTION+8)-normalshade);
}

/*
==========================
=
= GetLightRateTile
=
==========================
*/
int GetLightRateTile ( void )
{
    return ((HEIGHTFRACTION+8)-normalshade+LIGHTRATEBASE);
}



/*
==========================
=
= UpdateLightLevel
=
==========================
*/
void UpdateLightLevel (int area)
{
    int numlights;
    int targetmin;
    int targetmax;
    int numtiles;

    if (fog==true)
        return;

    numtiles=(numareatiles[area]>>5)-2;
    numlights=(LightsInArea[area]-numtiles)>>1;

    if (numlights<0)
        numlights=0;
    if (numlights>GENERALNUMLIGHTS)
        numlights=GENERALNUMLIGHTS;
    targetmin=baseminshade+(GENERALNUMLIGHTS-numlights);
    targetmax=basemaxshade-numlights;
    if (targetmax<baseminshade)
        targetmax=baseminshade;
    if (targetmin>targetmax)
        targetmin=targetmax;

    if (minshade>targetmin)
        minshade-=1;
    else if (minshade<targetmin)
        minshade+=1;

    if (maxshade>targetmax)
        maxshade-=1;
    else if (maxshade<targetmax)
        maxshade+=1;
}

/*
==========================
=
= SetIllumination
=
= Postive value lightens
= Negative value darkens
=
==========================
*/
void SetIllumination (int level)
{
    if (fog)
        return;
    maxshade-=level;
    if (maxshade>31)
        maxshade=31;
    if (maxshade<0x10)
        maxshade=0x10;
    minshade-=level;
    if (minshade<0x10)
        minshade=0x10;
    if (minshade>31)
        minshade=31;
}

/*
==========================
=
= GetIlluminationDelta
=
==========================
*/
int GetIlluminationDelta (void)
{
    if (fog)
        return 0;
    else
        return maxshade-basemaxshade;
}

/*
==========================
=
= UpdateLightning
=
==========================
*/
void UpdateLightning (void)
{
    if (periodic==true)
    {
        UpdatePeriodicLighting();
        return;
    }
    if ((fog==1) || (lightning==false))
        return;

    if (lightningtime<=0)
    {
        if (lightningsoundtime>0)
            SD_Play3D (SD_LIGHTNINGSND, 0, lightningdistance);
        lightningtime=GameRandomNumber("UpdateLightning",0)<<1;
        lightningdistance=GameRandomNumber("UpdateLightning",0);
        lightninglevel=(255-lightningdistance)>>LIGHTNINGLEVEL;
        if (lightninglevel<MINLIGHTNINGLEVEL)
            lightninglevel=MINLIGHTNINGLEVEL;
        if (lightninglevel>MAXLIGHTNINGLEVEL)
            lightninglevel=MAXLIGHTNINGLEVEL;
        lightningdelta=lightninglevel>>1;
        lightningsoundtime=lightningdistance>>1;
        if (lightningdistance<100)
        {
            SetIllumination(lightninglevel);
        }
    }
    else if (lightninglevel>0)
    {
        lightninglevel-=lightningdelta;
        if (lightninglevel<=0)
        {
            lightninglevel=0;
        }
    }
    else
        lightningtime--;
    if (lightningsoundtime)
    {
        lightningsoundtime--;
        if (lightningsoundtime<=0)
        {
            int volume;

            volume=255-lightningdistance;
            if (volume>170) volume=170;
            SD_PlayPitchedSound ( SD_LIGHTNINGSND, volume,-(lightningdistance<<2));
            lightningsoundtime=0;
        }
    }
}

/*
==========================
=
= UpdatePeriodicLighting
=
==========================
*/
#define PERIODICMAG (6)
#define PERIODICSTEP (20)
#define PERIODICBASE (0x0f)
void UpdatePeriodicLighting (void)
{
    int val;

    val=FixedMul(PERIODICMAG,sintable[periodictime]);
    periodictime=(periodictime+PERIODICSTEP)&(FINEANGLES-1);
    basemaxshade=PERIODICBASE+(PERIODICMAG)+val;
    baseminshade=basemaxshade-GENERALNUMLIGHTS-1;
}

/*
==========================
=
= SetModemLightLevel
=
==========================
*/
void SetModemLightLevel ( int type )
{
    periodic=false;
    fulllight=false;
    switch (type)
    {
        case bo_light_dark:
            MAPSPOT(2,0,0)=(word)216;
            MAPSPOT(3,0,0)=(word)255;
            MAPSPOT(3,0,1)=(word)139;
            MAPSPOT(2,0,1)=(word)104;
            SetupLightLevels ();
            break;
        case bo_light_normal:
            break;
        case bo_light_bright:
            MAPSPOT(2,0,0)=(word)223;
            MAPSPOT(3,0,0)=(word)267;
            MAPSPOT(3,0,1)=(word)0;
            MAPSPOT(2,0,1)=(word)104;
            SetupLightLevels ();
            break;
        case bo_light_fog:
            MAPSPOT(2,0,0)=(word)219;
            MAPSPOT(3,0,0)=(word)259;
            MAPSPOT(2,0,1)=(word)105;
            MAPSPOT(3,0,1)=(word)0;
            SetupLightLevels ();
            break;
        case bo_light_periodic:
            fog=0;
            MAPSPOT(2,0,1)=(word)104;
            MAPSPOT(3,0,1)=(word)139;
            SetupLightLevels ();
            periodic=true;
            break;
        case bo_light_lightning:
            if (sky!=0)
            {
                MAPSPOT(2,0,0)=(word)222;
                MAPSPOT(3,0,0)=(word)255;
                MAPSPOT(3,0,1)=(word)139;
                MAPSPOT(2,0,1)=(word)104;
                SetupLightLevels ();
                lightning=true;
            }
            break;
    }
}

