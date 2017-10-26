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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "rt_def.h"
#include "rt_main.h"
#include "rt_game.h"
#include "_rt_game.h"
#include "rt_menu.h"
#include "z_zone.h"
#include "w_wad.h"
#include "lumpy.h"
#include "rt_playr.h"
#include "rt_util.h"
#include "rt_ted.h"
#include "rt_draw.h"
#include "rt_view.h"
#include "rt_vid.h"
#include "rt_door.h"
#include "rt_in.h"
#include "rt_str.h"
#include "isr.h"
#include "rt_build.h"
#include "rt_rand.h"
#include "rt_cfg.h"
#include "rt_sound.h"
#include "version.h"
#include "rt_crc.h"
#include "modexlib.h"
#include "engine.h"
#include "gmove.h"
#include "rt_com.h"
#include "rt_net.h"
#include "watcom.h"
#include "rt_floor.h"
#include "rt_msg.h"
#include "rt_scale.h"
#include "develop.h"

//******************************************************************************
//
// GLOBALS
//
//******************************************************************************

int PlayerSnds[5] = {SD_PLAYERTCSND, SD_PLAYERTBSND, SD_PLAYERDWSND,
                     SD_PLAYERLNSND, SD_PLAYERIPFSND
                    };


unsigned short SHAKETICS = 0xFFFF;

int damagecount = 0;
HighScore Scores[MaxScores] =
{
    {"Tom", 70000, 7, 1},
    {"Chuck", 60000, 6, 1},
    {"Mark", 50000, 5, 1},
    {"The Joes", 40000, 4, 1},
    {"William", 30000, 3, 1},
    {"Jim", 20000, 2, 1},
    {"Steve", 10000, 1, 1},
};

HighScore DefaultScores[MaxScores] =
{
    {"Tom", 70000, 7, 1},
    {"Chuck", 60000, 6, 1},
    {"Mark", 50000, 5, 1},
    {"The Joes", 40000, 4, 1},
    {"William", 30000, 3, 1},
    {"Jim", 20000, 2, 1},
    {"Steve", 10000, 1, 1},
};

char SaveFileName[ 13 ] = "ROTTGAM?.ROT\0";
char NewSaveFileName[64] = "rottgam?_other.rot\0";

//******************************************************************************
//
// LOCALS
//
//******************************************************************************

static int KeyX[4] = {KEY1_X, KEY2_X, KEY3_X, KEY4_X};

static const char *Names[5] = {"Taradino", "Thi", "Doug", "Lorelei", "Ian Paul"};

static const char *LastNames[5] = {"Cassatt", "Barrett", "Wendt", "Ni", "Freeley"};

static STR ScoreStr;
static STR LivesStr;
static STR TriadStr;
static STR KillStr;

static pic_t *lifeptnums[10];
static pic_t *lifenums[10];
static pic_t *timenums[10];
static pic_t *scorenums[10];
static pic_t *keys[4];
static pic_t *men[5];

static pic_t *health[6];
static pic_t *ammo[26];
static pic_t *erase;
static pic_t *eraseb;
static pic_t *fragpic[5];
static pic_t *frag100pic[5];
static pic_t *negfragpic[5];
static pic_t *menneg[5];
static pic_t *blankfragpic;

static int powerpics;
static int poweradjust;

static int poweruptime;
static int powerupheight;
static int protectiontime;
static int protectionheight;


static boolean EndLevelStuff = false;
static boolean borderset     = false;
static int oldsec            = -1;

static pic_t *BkPic;
int SaveTime = 0;
int oldhealth;

static int oldplayerhealth;
static int oldpercenthealth;

static int playeruniformcolor;

#define NUMBONUSES   11
#define BONUSBONUS   100000


void DrawPPic (int xpos, int ypos, int width, int height, byte *src, int num, boolean up, boolean bufferofsonly);


//******************************************************************************
//
// CacheLumpGroup ()
//
//******************************************************************************
void CacheLumpGroup (char *startlump, pic_t **lumparray, int numberoflumps)
{
    int lumpnum;
    int i;

    lumpnum = W_GetNumForName(startlump);

    for( i = 0; i < numberoflumps; i++ )
    {
        lumparray[i] = (pic_t *)W_CacheLumpNum(lumpnum + i, PU_LEVEL, Cvt_pic_t, 1);
    }
}

//******************************************************************************
//
// SetupPlayScreen ()
//
//******************************************************************************
void SetupPlayScreen (void)
{
    int i;
    int j;
    int num;

    erase = (pic_t *)W_CacheLumpName("erase", PU_LEVEL, Cvt_pic_t, 1);
    eraseb = (pic_t *)W_CacheLumpName("eraseb", PU_LEVEL, Cvt_pic_t, 1);

    CacheLumpGroup("tmnum0", timenums, 10);
    CacheLumpGroup("lfnum0", lifeptnums, 10);
    CacheLumpGroup("lvnum0", lifenums, 10);
    CacheLumpGroup("health1b", health, 6);
    CacheLumpGroup("key1", keys, 4);

    if (!BATTLEMODE)
    {
        CacheLumpGroup("scnum0", scorenums, 10);

        num = locplayerstate->player;
        men[num] = (pic_t *)W_CacheLumpNum(W_GetNumForName("MAN1") +
                                           num, PU_LEVEL, Cvt_pic_t, 1);
    }
    else
    {
        int man;
        int num100;
        int negnum;
        int negman;

        CacheLumpGroup("kilnum0", scorenums, 10);

        negnum = W_GetNumForName("botnpic1");
        num = W_GetNumForName("botpic0");
        num100 = W_GetNumForName("botopic1");
        negman = W_GetNumForName("negman1");
        man = W_GetNumForName("man1");

        blankfragpic = (pic_t *)W_CacheLumpNum(num, PU_LEVEL, Cvt_pic_t, 1);

        num++;

        for (i = 0; i < numplayers; i++)
        {
            j = PLAYERSTATE[i].player;

            if (!gamestate.teamplay)
            {
                fragpic[j] = (pic_t *)W_CacheLumpNum(num + j, PU_LEVEL, Cvt_pic_t, 1);
                frag100pic[j] = (pic_t *)W_CacheLumpNum(num100 + j, PU_LEVEL, Cvt_pic_t, 1);
                negfragpic[j] = (pic_t *)W_CacheLumpNum(negnum + j, PU_LEVEL, Cvt_pic_t, 1);
            }
            else
            {
                negfragpic[j] = (pic_t *)W_CacheLumpName("teamnpic", PU_LEVEL, Cvt_pic_t, 1);
                fragpic[j] = (pic_t *)W_CacheLumpName("teampic", PU_LEVEL, Cvt_pic_t, 1);
                frag100pic[j] = fragpic[j];
            }

            menneg[j] = (pic_t *)W_CacheLumpNum(negman + j, PU_LEVEL, Cvt_pic_t, 1);
            men[j] = (pic_t *)W_CacheLumpNum(man + j, PU_LEVEL, Cvt_pic_t, 1);
        }
    }

    powerpics = W_GetNumForName("GDMODEP");
    poweradjust = POWERUPTICS / 16;

    num = W_GetNumForName("INF_B");

    // bullet weapons
    ammo[0] = (pic_t *)W_CacheLumpNum(num, PU_LEVEL, Cvt_pic_t, 1);
    ammo[1] = (pic_t *)W_CacheLumpNum(num, PU_LEVEL, Cvt_pic_t, 1);
    ammo[2] = (pic_t *)W_CacheLumpNum(num++, PU_LEVEL, Cvt_pic_t, 1);

    for (i = 3; i < 13; i++)
    {
        ammo[i] = (pic_t *)W_CacheLumpNum(num++, PU_LEVEL, Cvt_pic_t, 1);
    }

    ammo[13] = (pic_t *)W_CacheLumpNum(num, PU_LEVEL, Cvt_pic_t, 1);
    ammo[14] = (pic_t *)W_CacheLumpNum(num, PU_LEVEL, Cvt_pic_t, 1);
    ammo[15] = (pic_t *)W_CacheLumpNum(num++, PU_LEVEL, Cvt_pic_t, 1);


    for (i = 16; i < 26; i++)
    {
        ammo[i] = (pic_t *)W_CacheLumpNum(num++, PU_LEVEL, Cvt_pic_t, 1);
    }

    oldplayerhealth = -1;
    oldpercenthealth = -1;
}



//******************************************************************************
//
// GameMemToScreen()
//
//******************************************************************************

void GameMemToScreen (pic_t *source, int x, int y, int bufferofsonly)
{
    /*if (bufferofsonly)
        VL_MemToScreen((byte *)&source->data, source->width,
            source->height, x, y);
    else*/
    GM_MemToScreen((byte *)&source->data, source->width,
                   source->height, x, y);
}


//******************************************************************************
//
// DrawPlayScreen ()
//
//******************************************************************************
void DrawPlayScreen (boolean bufferofsonly)
{
    pic_t *shape;
    int shapenum;

    if (SHOW_TOP_STATUS_BAR())
    {
        shape = ( pic_t * )W_CacheLumpName( "stat_bar", PU_CACHE, Cvt_pic_t, 1 );
        GameMemToScreen( shape, screen_src_statusbar_x, 0, bufferofsonly );
    }

    if (BATTLEMODE)
        DrawKills(bufferofsonly);

    if (SHOW_BOTTOM_STATUS_BAR())
    {
        shape = (pic_t *)W_CacheLumpName("bottbar", PU_CACHE, Cvt_pic_t, 1);

        if (SHOW_KILLS())
            GameMemToScreen( shape, screen_src_statusbar_x,
                             screen_src_height - (40 * screen_src_minscale), bufferofsonly );
        else
            GameMemToScreen( shape, screen_src_statusbar_x,
                             screen_src_height - (16 * screen_src_minscale), bufferofsonly );

        DrawBarAmmo(bufferofsonly);
        DrawBarHealth(bufferofsonly);

        if (demoplayback)
        {
            shape = (pic_t *)W_CacheLumpName("demo", PU_CACHE, Cvt_pic_t, 1);
            DrawPPic(((screen_src_width / 2.0) - (12 * screen_src_minscale)),
                     screen_src_height - (15 * screen_src_minscale), shape->width, shape->height,
                     (byte *)&shape->data, 1, true, bufferofsonly); // 148, 185
        }
    }

    if (!SHOW_TOP_STATUS_BAR())
        return;

//draws small player picture and name in topbar
    oldsec = -1;

    DrawTime(bufferofsonly);

    if (!BATTLEMODE)
    {
        int character;
        int width;
        int height;

        character = locplayerstate->player;
        GameMemToScreen(men[character], screen_src_statusbar_x + (MEN_X * screen_src_minscale),
                        MEN_Y * screen_src_minscale, bufferofsonly);

        CurrentFont = tinyfont;

        // Draw player's name
        DrawGameString (screen_src_statusbar_x + ((MEN_X + 3) * screen_src_minscale),
                        (MEN_Y + 2) * screen_src_minscale, Names[character], bufferofsonly);

        VW_MeasurePropString(LastNames[character], &width, &height);

        DrawGameString (screen_src_statusbar_x + ((MEN_X + 44 - width) * screen_src_minscale),
                        (MEN_Y + 8) * screen_src_minscale, LastNames[character], bufferofsonly);

        UpdateLives(locplayerstate->lives);
        UpdateScore(gamestate.score);
        DrawTriads(bufferofsonly);
        DrawLives(bufferofsonly);
        DrawScore(bufferofsonly);
    }

    DrawKeys(bufferofsonly);

    if (locplayerstate->poweruptime)
    {
        if (player->flags & FL_GODMODE)
            shapenum = powerpics;
        else if (player->flags & FL_DOGMODE)
            shapenum = powerpics + 1;
        else if (player->flags & FL_FLEET)
            shapenum = powerpics + 2;
        else if (player->flags & FL_ELASTO)
            shapenum = powerpics + 3;
        else if (player->flags & FL_SHROOMS)
            shapenum = powerpics + 4;

        shape = (pic_t *)W_CacheLumpNum(shapenum, PU_CACHE, Cvt_pic_t, 1);

        GameMemToScreen(eraseb, screen_src_statusbar_x + (POWERUP1X * screen_src_minscale),
                        POWERUPY * screen_src_minscale, bufferofsonly);

        DrawMPPic(screen_src_statusbar_x + (POWERUP1X * screen_src_minscale),
                  (POWERUPY + powerupheight) * screen_src_minscale, shape->width,
                  shape->height - powerupheight, powerupheight, (byte *)&shape->data,
                  bufferofsonly);
    }


    if (locplayerstate->protectiontime)
    {
        if (player->flags & FL_BPV)
            shapenum = powerpics + 6;
        else if (player->flags & FL_GASMASK)
            shapenum = powerpics + 5;
        else if (player->flags & FL_AV)
            shapenum = powerpics + 7;

        shape = (pic_t *)W_CacheLumpNum(shapenum, PU_CACHE, Cvt_pic_t, 1);

        GameMemToScreen(eraseb, screen_src_statusbar_x + (POWERUP2X * screen_src_minscale),
                        POWERUPY * screen_src_minscale, bufferofsonly);

        DrawMPPic(screen_src_statusbar_x + (POWERUP2X * screen_src_minscale),
                  (POWERUPY + protectionheight) * screen_src_minscale, shape->width,
                  shape->height - protectionheight, protectionheight, (byte *)&shape->data,
                  bufferofsonly);
    }
}


//******************************************************************************
//
// ShortenCodeName()
//
//******************************************************************************
void GetShortCodeName (char *dest, char *source, int maxwidth)
{
    int width;
    int height;
    int length;

    strcpy(dest, source);

    // Shorten name to fit
    length = strlen(dest);
    VW_MeasurePropString(dest, &width, &height);

    while(width > maxwidth)
    {
        dest[length] = 0;
        length--;

        VW_MeasurePropString(dest, &width, &height);
    }
}


//******************************************************************************
//
// DrawKills ()
//
//******************************************************************************
void DrawKills (boolean bufferofsonly)
{
    int rank;
    int xpos;
    char codename[ MAXCODENAMELENGTH ];
    int width;
    int playernum;
    int playerimage;
    int temp;
    pic_t *pic;

    int kills_y = screen_src_height - (24 * screen_src_minscale);
    int kills_name_y = kills_y + (16 * screen_src_minscale);

    CurrentFont = tinyfont;

    if (SHOW_TOP_STATUS_BAR())
    {
        playernum = BATTLE_Team[consoleplayer];
        playerimage = BATTLE_TeamLeader[playernum];

        // Set uniformcolor
        playeruniformcolor = PLAYERSTATE[playerimage].uniformcolor;

        // Draw player's point box
        pic = men[PLAYERSTATE[playerimage].player];
        if (gamestate.ShowScores && (BATTLE_Points[playernum] < 0))
            pic = menneg[PLAYERSTATE[playerimage].player];

        DrawPPic(screen_src_statusbar_x + (MEN_X * screen_src_minscale),
                 MEN_Y * screen_src_minscale, pic->width, pic->height,
                 (byte *)&pic->data, 1, true, bufferofsonly);

        // Draw player's name
        if (gamestate.teamplay)
            GetShortCodeName(codename, colorname[ playeruniformcolor ], 42);
        else
            GetShortCodeName(codename, PLAYERSTATE[ playerimage ].codename, 42);

        DrawGameString(screen_src_statusbar_x + ((MEN_X + 2) * screen_src_minscale),
                       (MEN_Y + 2) * screen_src_minscale, codename, bufferofsonly);

        // Draw "It" if player is 'it'
        if (((gamestate.battlemode == battle_Tag) || (gamestate.battlemode == battle_Hunter)) &&
                (BATTLE_It == BATTLE_Team[consoleplayer]))
            DrawGameString(screen_src_statusbar_x + ((MEN_X + 22) * screen_src_minscale),
                           (MEN_Y + 8) * screen_src_minscale, "It", bufferofsonly);

        // Draw triad if player is 'it' or has caught a triad
        if (PLAYER[consoleplayer]->flags & FL_DESIGNATED)
        {
            pic = W_CacheLumpName( "smalltri", PU_CACHE, Cvt_pic_t, 1 );

            DrawPPic(screen_src_statusbar_x + ((TRIAD_X - 1) * screen_src_minscale),
                     (TRIAD_Y - 2) * screen_src_minscale, pic->width, pic->height,
                     (byte *)&pic->data, 1, true, bufferofsonly);
        }
        else if (gamestate.ShowScores && (DisplayPoints != bo_kills_infinite))
        {
            // Draw Kill goal
            if ((gamestate.battlemode == battle_Collector) ||
                    (gamestate.battlemode == battle_StandAloneGame))
                temp = BATTLE_NumCollectorItems;
            else
                temp = DisplayPoints;

            ltoa(temp % 1000, KillStr.str, 10);
            KillStr.length = strlen(KillStr.str);
            DrawNumber((TRIAD_X - 6) * screen_src_minscale,
                       TRIAD_Y * screen_src_minscale, 3, 5, bufferofsonly);
        }

        // Set uniformcolor
        playeruniformcolor = PLAYERSTATE[consoleplayer].uniformcolor;

        if (gamestate.ShowScores)
        {
            // Draw local player's points
            temp = BATTLE_Points[playernum] % 1000;

            if (temp < 0)
                temp = -temp;

            ltoa(temp, KillStr.str, 10);
            KillStr.length = strlen(KillStr.str);

            DrawNumber((LIVES_X - 12) * screen_src_minscale,
                       LIVES_Y * screen_src_minscale, 3, 4, bufferofsonly);
        }
        else
        {
            pic = W_CacheLumpName("minus", PU_CACHE, Cvt_pic_t, 1);
            StatusDrawColoredPic(screen_src_statusbar_x + ((LIVES_X - 12) * screen_src_minscale),
                                 LIVES_Y * screen_src_minscale, pic, bufferofsonly, playeruniformcolor);
            StatusDrawColoredPic(screen_src_statusbar_x + ((LIVES_X - 4) * screen_src_minscale),
                                 LIVES_Y * screen_src_minscale, pic, bufferofsonly, playeruniformcolor);
        }

        // Draw whoever is 'It'
        playernum = BATTLE_It;
        playerimage = BATTLE_TeamLeader[playernum];

        // Set uniformcolor
        playeruniformcolor = PLAYERSTATE[playerimage].uniformcolor;

        // Draw player's point box
        pic = men[PLAYERSTATE[playerimage].player];
        if (gamestate.ShowScores && (BATTLE_Points[ playernum ] < 0))
            pic = menneg[ PLAYERSTATE[ playerimage ].player ];

        DrawPPic(screen_src_statusbar_x + (LEADER_X * screen_src_minscale),
                 LEADER_Y * screen_src_minscale, pic->width, pic->height, (byte *)&pic->data,
                 1, true, bufferofsonly );

        if ((gamestate.battlemode == battle_Tag) || (gamestate.battlemode == battle_Hunter))
            DrawGameString (screen_src_statusbar_x + ((LEADER_X + 22) * screen_src_minscale),
                            (LEADER_Y + 8) * screen_src_minscale, "It", bufferofsonly);

        if (gamestate.ShowScores)
        {
            // Draw number of points
            temp = BATTLE_Points[playernum] % 1000;

            if ( temp < 0 )
                temp = -temp;

            ltoa(temp, KillStr.str, 10);
            KillStr.length = strlen(KillStr.str);
            DrawNumber(LEADER_NUM_X * screen_src_minscale,
                       LEADER_NUM_Y * screen_src_minscale, 3, 4, bufferofsonly);
        }
        else
        {
            pic = W_CacheLumpName("minus", PU_CACHE, Cvt_pic_t, 1);
            StatusDrawColoredPic(screen_src_statusbar_x + (LEADER_NUM_X * screen_src_minscale),
                                 LEADER_NUM_Y * screen_src_minscale, pic, bufferofsonly, playeruniformcolor );
            StatusDrawColoredPic(screen_src_statusbar_x + ((LEADER_NUM_X + 8) * screen_src_minscale),
                                 LEADER_NUM_Y * screen_src_minscale, pic, bufferofsonly, playeruniformcolor );
        }

        // Draw name
        if (gamestate.teamplay)
            DrawGameString(screen_src_statusbar_x + (LEADER_NAME_X * screen_src_minscale),
                           (LEADER_NAME_Y - 1) * screen_src_minscale, colorname[playeruniformcolor],
                           bufferofsonly);
        else
        {
            GetShortCodeName(codename, PLAYERSTATE[playerimage].codename, 42);
            DrawGameString (screen_src_statusbar_x + ((LEADER_NAME_X - 1) * screen_src_minscale),
                            LEADER_NAME_Y * screen_src_minscale, codename, bufferofsonly);
        }
    }

    // Only draw the rest of the rifraff when the kill count is selected
    if (!SHOW_KILLS())
        return;

    // Draw all the other losers
    xpos = KILLS_X;

    for (rank = 0; rank < BATTLE_NumberOfTeams; rank++)
    {
        playernum = BATTLE_PlayerOrder[rank];
        playerimage = BATTLE_TeamLeader[playernum];

        if ((playernum == BATTLE_It) && SHOW_TOP_STATUS_BAR())
            continue;

        // Set uniformcolor
        playeruniformcolor = PLAYERSTATE[playerimage].uniformcolor;

        // Draw player's point box
        pic = fragpic[PLAYERSTATE[playerimage].player];

        if (gamestate.ShowScores)
        {
            if (BATTLE_Points[playernum] < 0)
                pic = negfragpic[PLAYERSTATE[playerimage].player];
            else if (BATTLE_Points[playernum] >= 100)
                pic = frag100pic[PLAYERSTATE[playerimage].player];
        }

        DrawPPic(screen_src_statusbar_x + xpos, kills_y, pic->width, pic->height,
                 (byte *)&pic->data, 1, true, bufferofsonly );

        // Draw number of points
        if (gamestate.ShowScores)
        {
            temp = BATTLE_Points[playernum] % 1000;

            if ( temp < 0 )
                temp = -temp;

            ltoa(temp, KillStr.str, 10);
            KillStr.length = strlen(KillStr.str);
            width = 2;

            if (temp > 99)
                width = 3;

            DrawNumber(xpos +
                       ((KILLS_OFFSET + 16 - (8 * width)) * screen_src_minscale), kills_y,
                       width, 4, bufferofsonly);
        }
        else
        {
            pic = (pic_t *)W_CacheLumpName("minus", PU_CACHE, Cvt_pic_t, 1);
            StatusDrawColoredPic(screen_src_statusbar_x + xpos + (KILLS_OFFSET * screen_src_minscale),
                                 kills_y, pic, bufferofsonly, playeruniformcolor);
            StatusDrawColoredPic(screen_src_statusbar_x + xpos + ((KILLS_OFFSET + 8) * screen_src_minscale),
                                 kills_y, pic, bufferofsonly, playeruniformcolor);
        }

        // Get codename
        if (gamestate.teamplay)
            GetShortCodeName(codename, colorname[playeruniformcolor],
                             KILLS_WIDTH - 2);
        else
            GetShortCodeName(codename, PLAYERSTATE[playerimage].codename,
                             KILLS_WIDTH - 2);

        // Draw name
        DrawGameString(screen_src_statusbar_x + xpos + screen_src_minscale, kills_name_y, codename,
                       bufferofsonly);

        // Advance to next position
        xpos += KILLS_WIDTH * screen_src_minscale;

        if (xpos >= screen_src_width)
            break;
    }

    while (1)
    {
        if (xpos >= screen_src_width)
            break;

        pic = blankfragpic;
        DrawPPic(screen_src_statusbar_x + xpos, kills_y, pic->width, pic->height,
                 (byte *)&pic->data, 1, true, bufferofsonly);

        // Advance to next position
        xpos += KILLS_WIDTH * screen_src_minscale;
    }
}


//******************************************************************************
//
// DrawPlayers ()
//
//******************************************************************************
void DrawPlayers (void)
{
    int num;
    int xpos;
    char codename[MAXCODENAMELENGTH];
    int length;
    int width;
    int height;
    int team;
    int player;
    int character;
    pic_t *pic;
    pic_t *enemy;
    pic_t *friend;

    int players_y = screen_src_height - ((24 - (gamestate.teamplay ? 0 : 4)) * screen_src_minscale) -
                    (69 * screen_src_hscale);
    int players_name_y = players_y + (16 * screen_src_minscale);
    int players_team_y = players_y + (24 * screen_src_minscale);

    num = W_GetNumForName("botpic1");

    scorenums[0] = (pic_t *)W_CacheLumpName("kilnum0", PU_CACHE, Cvt_pic_t, 1);
    friend = (pic_t *)W_CacheLumpName("t_friend", PU_CACHE, Cvt_pic_t, 1);
    enemy = (pic_t *)W_CacheLumpName("t_enemy", PU_CACHE, Cvt_pic_t, 1);

    // Draw all the losers
    CurrentFont = tinyfont;

    xpos = (screen_src_width - (min(numplayers, MAXKILLBOXES) * KILLS_WIDTH * screen_src_minscale)) / 2.0;

    for (team = 0; team < BATTLE_NumberOfTeams; team++)
    {
        for (player = 0; player < numplayers; player++)
        {
            if (BATTLE_Team[player] == team)
            {
                character = PLAYERSTATE[player].player;

                fragpic[character] = (pic_t *)W_CacheLumpNum(num + character,
                                     PU_CACHE, Cvt_pic_t, 1);

                if ((numplayers <= MAXKILLBOXES) || (player != consoleplayer))
                {
                    // Set uniformcolor
                    playeruniformcolor = PLAYERSTATE[player].uniformcolor;

                    // Draw player's point box
                    pic = fragpic[PLAYERSTATE[player].player];

                    VL_DrawPic(xpos, players_y, pic);

                    if (gamestate.teamplay)
                    {
                        if (BATTLE_Team[player] == BATTLE_Team[consoleplayer])
                            VL_DrawPic(xpos, players_team_y, friend);
                        else
                            VL_DrawPic(xpos, players_team_y, enemy);
                    }

                    //strcpy(KillStr.str, "00");
                    //KillStr.length = strlen(KillStr.str);
                    //DrawNumber(xpos + (KILLS_OFFSET * screen_src_minscale), players_y, 2, 4, true);
                    StatusDrawColoredPicFreeX(xpos + (KILLS_OFFSET * screen_src_minscale),
                                              players_y, scorenums[0], true, playeruniformcolor);
                    StatusDrawColoredPicFreeX(xpos + ((KILLS_OFFSET + 8)* screen_src_minscale),
                                              players_y, scorenums[0], true, playeruniformcolor);

                    // Get codename
                    strcpy(codename, PLAYERSTATE[player].codename);

                    // Shorten name to fit into point count
                    length = strlen(codename);
                    US_MeasureStr(&width, &height, "%s", codename);
                    while(width > KILLS_WIDTH)
                    {
                        codename[length] = 0;
                        length--;
                        US_MeasureStr(&width, &height, "%s", codename);
                    }

                    // Draw name
                    PrintX = xpos;
                    PrintY = players_name_y;
                    US_Print(codename);

                    // Advance to next position
                    xpos += KILLS_WIDTH * screen_src_minscale;
                }
            }

            if (xpos >= screen_src_width)
                break;
        }

        if (xpos >= screen_src_width)
            break;
    }
}

//******************************************************************************
//
// StatusDrawPic ()
//
//******************************************************************************

void StatusDrawPic (unsigned x, unsigned y, pic_t *nums, boolean bufferofsonly)
{
    DrawMPPic(screen_src_statusbar_x + x, y, nums->width, nums->height, 0,
              (byte *)&nums->data, bufferofsonly);
}

//******************************************************************************
//
// StatusDrawColoredPic ()
//
//******************************************************************************

void StatusDrawColoredPic (unsigned x, unsigned y, pic_t *nums, boolean bufferofsonly, int color)
{
    DrawColoredMPPic(screen_src_statusbar_x + x, y, nums->width, nums->height, 0,
                     (byte *)&nums->data, bufferofsonly, color);
}

void StatusDrawColoredPicFreeX (unsigned x, unsigned y, pic_t *nums, boolean bufferofsonly, int color)
{
    DrawColoredMPPic(x, y, nums->width, nums->height, 0,
                     (byte *)&nums->data, bufferofsonly, color);
}

//******************************************************************************
//
// DrawGameString ()
//
// draw string to game screen at x,y
//
//******************************************************************************

void DrawGameString (int x, int y, const char *str, boolean bufferofsonly)
{
    byte *tempbuf;

    px = x;
    py = y;

    if (bufferofsonly)
        VW_DrawPropString(str);
    else
    {
        tempbuf = bufferofs;
        bufferofs = page1start;
        VW_DrawPropString(str);
        bufferofs = tempbuf;
    }
}


//******************************************************************************
//
// DrawNumber ()
//
// right justifies and pads with zeros
//
//******************************************************************************

void DrawNumber (int x, int y, int width, int which, boolean bufferofsonly)
{
    unsigned length, c;
    char *str;
    byte z;

    switch (which)
    {
        case 1:
            str = ScoreStr.str;
            length = ScoreStr.length;
            break;

        case 2:
            str = LivesStr.str;
            length = LivesStr.length;
            break;

        case 3:
            str = TriadStr.str;
            length = TriadStr.length;
            break;

        case 4:
        case 5:
        case 6:
            str = KillStr.str;
            length = KillStr.length;
            break;
    }

    z = width - length;

    while (z)
    {
        switch (which)
        {
            case 1:
                StatusDrawPic(x, y, scorenums[0], bufferofsonly);
                x += 8 * screen_src_minscale;
                break;

            case 2:
                StatusDrawPic(x, y, lifenums[0], bufferofsonly);
                x += 8 * screen_src_minscale;
                break;

            case 3:
                StatusDrawPic(x, y, lifeptnums[0], bufferofsonly);
                x += 6 * screen_src_minscale;
                break;

            case 4:
                StatusDrawColoredPic(x, y, scorenums[0], bufferofsonly, playeruniformcolor);
                x += 8 * screen_src_minscale;
                break;

            case 5:
                StatusDrawPic(x, y, lifeptnums[0], bufferofsonly);
                x += 6 * screen_src_minscale;
                break;

            case 6:
                StatusDrawPic(x, y, lifenums[0], bufferofsonly);
                x += 8 * screen_src_minscale;
                break;
        }

        z--;
    }

    c = length <= ((unsigned)width ? 0 : length-width);

    while (c < length)
    {
        switch (which)
        {
            case 1:
                StatusDrawPic(x, y, scorenums[str[c] - '0'], bufferofsonly);
                x += 8 * screen_src_minscale;
                break;
            case 2:
                StatusDrawPic(x, y, lifenums[str[c] - '0'], bufferofsonly);
                x += 8 * screen_src_minscale;
                break;
            case 3:
                StatusDrawPic(x, y, lifeptnums[str[c] - '0'], bufferofsonly);
                x += 6 * screen_src_minscale;
                break;
            case 4:
                StatusDrawColoredPic(x, y, scorenums[str[c] - '0'], bufferofsonly, playeruniformcolor);
                x += 8 * screen_src_minscale;
                break;
            case 5:
                StatusDrawPic(x, y, lifeptnums[str[c] - '0'], bufferofsonly);
                x += 6 * screen_src_minscale;
                break;
            case 6:
                StatusDrawPic(x, y, lifenums[str[c] - '0'], bufferofsonly);
                x += 8 * screen_src_minscale;
                break;
        }

        c++;
    }
}



//******************************************************************************
//
// HealPlayer ()
//
//******************************************************************************

void HealPlayer (int points, objtype *ob)
{
    playertype *pstate;
    int maxhitpoints;

    M_LINKSTATE(ob, pstate);

    pstate->health += points;
    maxhitpoints = MaxHitpointsForCharacter(pstate);

    if (pstate->health > maxhitpoints)
        pstate->health = maxhitpoints;

    if (SHOW_BOTTOM_STATUS_BAR() && (ob == player))
        DrawBarHealth(false);
}

//******************************************************************************
//
// DrawLives ()
//
//******************************************************************************

void DrawLives (boolean bufferofsonly)
{
    if (!SHOW_TOP_STATUS_BAR())
        return;

    if (!EndLevelStuff)
        DrawNumber(LIVES_X * screen_src_minscale, LIVES_Y * screen_src_minscale, 2, 2, bufferofsonly);
}


//******************************************************************************
//
// GiveExtraMan ()
//
//******************************************************************************

void GiveExtraMan (void)
{
    if (locplayerstate->lives < 99)
        locplayerstate->lives++;

    UpdateLives(locplayerstate->lives);
    DrawLives(false);
}



//******************************************************************************
//
// DrawScore ()
//
//******************************************************************************

void DrawScore (boolean bufferofsonly)
{
    if (!SHOW_TOP_STATUS_BAR())
        return;

    if (!BATTLEMODE)
        DrawNumber(SCORE_X * screen_src_minscale, SCORE_Y * screen_src_minscale, 10, 1, bufferofsonly);
}


//******************************************************************************
//
// GivePoints ()
//
//******************************************************************************

void GivePoints (long points)
{
    gamestate.score += points;

    UpdateScore(gamestate.score);

    if (!EndLevelStuff)
        DrawScore(false);
}


//******************************************************************************
//
// GiveKey ()
//
//******************************************************************************

void GiveKey (int key)
{
    locplayerstate->keys |= (1 << key);
    DrawKeys(false);
}


//******************************************************************************
//
// GiveLives ()
//
//******************************************************************************

void GiveLives (int newlives)
{
    if ((locplayerstate->lives + newlives) <= 99)
        locplayerstate->lives += newlives;
    else
        locplayerstate->lives = 99;

    UpdateLives(locplayerstate->lives);
    DrawLives(false);
}

//******************************************************************************
//
// EnableOldWeapon ()
//
//******************************************************************************

void EnableOldWeapon(playertype *pstate, objtype *ob)
{
    LASTSTAT->flags |= FL_ABP;
    LASTSTAT->flags &= ~FL_RESPAWN;

    MakeStatActive(LASTSTAT);

    pstate->weaponx = ob->tilex;
    pstate->weapony = ob->tiley;
}


//******************************************************************************
//
// GiveWeapon ()
//
//******************************************************************************

void GiveWeapon (objtype *ob, int weapon)
{
    playertype *pstate;

    M_LINKSTATE(ob, pstate);

    if (pstate->weapon == weapon)
        return;

    pstate->HASBULLETWEAPON[weapon] = 1;

    if ((pstate->weapon == pstate->bulletweapon) && (pstate->weapon < weapon))
    {
        pstate->new_weapon = weapon;
        pstate->weapondowntics = WEAPONS[pstate->weapon].screenheight / GMOVE;

        if ((ob == player) && (SHOW_BOTTOM_STATUS_BAR()))
            DrawBarAmmo(false);
    }

    if (gamestate.BattleOptions.WeaponPersistence)
    {
        SpawnStatic(ob->tilex, ob->tiley, GetItemForWeapon(weapon), 9);
        EnableOldWeapon(pstate, ob);
    }

    if (weapon > pstate->bulletweapon)
        pstate->bulletweapon = weapon;
}


//******************************************************************************
//
// GiveMissileWeapon ()
//
//******************************************************************************

void GiveMissileWeapon(objtype *ob, int which)
{
    playertype *pstate;

    M_LINKSTATE(ob, pstate);

    if (!gamestate.BattleOptions.WeaponPersistence)
    {
        if (pstate->ammo && (pstate->missileweapon != -1) &&
                !(WEAPON_IS_MAGICAL(which)) && !(WEAPON_IS_MAGICAL(pstate->missileweapon)))
        {
            int nx,ny;

            nx = ob->tilex;
            ny = ob->tiley;

            if (IsPlatform(nx,ny))
                SpawnStatic(nx, ny, GetItemForWeapon(pstate->missileweapon), 9);
            else
            {
                int newz = sprites[ob->tilex][ob->tiley]->z;
                SpawnStatic(nx, ny, GetItemForWeapon(pstate->missileweapon), -1);
                LASTSTAT->z = newz;
            }

            LASTSTAT->ammo = pstate->ammo;
            EnableOldWeapon(pstate, ob);
        }
    }
    else if (!WEAPON_IS_MAGICAL(which))
    {
        int newz = sprites[ob->tilex][ob->tiley]->z;
        SpawnStatic(ob->tilex, ob->tiley, GetItemForWeapon(which), 9);
        LASTSTAT->z = newz;
        EnableOldWeapon(pstate, ob);
    }

    pstate->new_weapon = pstate->missileweapon = which;
    pstate->weapondowntics = WEAPONS[pstate->weapon].screenheight / GMOVE;
}


//******************************************************************************
//
// DrawKeys ()
//
//******************************************************************************

void DrawKeys (boolean bufferofsonly)
{
    if (!SHOW_TOP_STATUS_BAR())
        return;

    if (locplayerstate->keys & 1)
        GameMemToScreen(keys[0], screen_src_statusbar_x + (KeyX[0] * screen_src_minscale),
                        KEY_Y * screen_src_minscale, bufferofsonly);

    if (locplayerstate->keys & 2)
        GameMemToScreen(keys[1], screen_src_statusbar_x + (KeyX[1] * screen_src_minscale),
                        KEY_Y * screen_src_minscale, bufferofsonly);

    if (locplayerstate->keys & 4)
        GameMemToScreen(keys[2], screen_src_statusbar_x + (KeyX[2] * screen_src_minscale),
                        KEY_Y * screen_src_minscale, bufferofsonly);

    if (locplayerstate->keys & 8)
        GameMemToScreen(keys[3], screen_src_statusbar_x + (KeyX[3] * screen_src_minscale),
                        KEY_Y * screen_src_minscale, bufferofsonly);
}


//******************************************************************************
//
// StatusDrawTime ()
//
//******************************************************************************

void StatusDrawTime (unsigned x, unsigned y, unsigned num, boolean bufferofsonly)
{
    DrawMPPic(screen_src_statusbar_x + x, y, timenums[num]->width, timenums[num]->height, 0,
              (byte *)&timenums[num]->data, bufferofsonly);
}


//******************************************************************************
//
// DrawTimeNumber ()
//
// right justifies and pads with blanks
//
//******************************************************************************

void DrawTimeNumber (int x, int y, int number, boolean seconds, boolean bufferofsonly)
{
    char str[20];

    ltoa(number, str, 10);

    if (seconds)
    {
        if (number < 10)
        {
            StatusDrawTime(x * screen_src_minscale, y * screen_src_minscale, 0, bufferofsonly);
            StatusDrawTime((x + 8) * screen_src_minscale, y * screen_src_minscale, str[0] - '0', bufferofsonly);
        }
        else
        {
            StatusDrawTime(x * screen_src_minscale, y * screen_src_minscale, str[0] - '0', bufferofsonly);
            StatusDrawTime((x + 8) * screen_src_minscale, y * screen_src_minscale, str[1] - '0', bufferofsonly);
        }
    }
    else
    {
        if (number < 10)
            StatusDrawTime((x + 8) * screen_src_minscale, y * screen_src_minscale, str[0] - '0', bufferofsonly);
        else
        {
            StatusDrawTime(x * screen_src_minscale, y * screen_src_minscale, str[0] - '0', bufferofsonly);
            StatusDrawTime((x + 8) * screen_src_minscale, y * screen_src_minscale, str[1] - '0', bufferofsonly);
        }
    }
}


//******************************************************************************
//
// DrawTimeXY ()
//
//******************************************************************************

void DrawTimeXY (int x, int y, int sec, boolean bufferofsonly)
{
    int min;
    int hour;

    while (sec > ((9 * 3600) + 3599))
    {
        sec -= (9 * 3600) + 3599;
    }

    hour = sec / 3600;
    min = (sec / 60) - (hour * 60);
    sec %= 60;

    DrawTimeNumber(x + HOUR_X, y, hour, false, bufferofsonly);
    DrawTimeNumber(x + MIN_X, y, min, true, bufferofsonly);
    DrawTimeNumber(x + SEC_X, y, sec, true, bufferofsonly);
}


//******************************************************************************
//
// DrawTime ()
//
//******************************************************************************

void DrawTime (boolean bufferofsonly)
{
    int sec;

    if (!SHOW_TOP_STATUS_BAR())
        return;

    if (timelimitenabled)
        sec = (timelimit - gamestate.TimeCount) / VBLCOUNTER;
    else
        sec = gamestate.TimeCount / VBLCOUNTER;

    if (oldsec != sec)
    {
        oldsec = sec;
        DrawTimeXY(GAMETIME_X, GAMETIME_Y, sec, bufferofsonly);
    }
}


//******************************************************************************
//
// DrawMPPic ()
//
// Purpose
//    Draws a masked, planer pic at xpos, ypos.
//
// Parms
//    xpos   - x position.
//    ypos   - y position.
//    width  - width of pic : should be << 2.
//    height - height of pic.
//    src    - data to draw.
//
// Returns
//    Nothing.
//
//******************************************************************************

void DrawMPPic (int xpos, int ypos, int width, int height, int heightmod,
                byte *src, boolean bufferofsonly)
{
    int olddest, dest;
    int plane, x, y, i, j;
    byte pixel;
    int finalw = width * screen_src_minscale * 4;
    byte *tempsrc;

    olddest = ylookup[ypos] + xpos;
    tempsrc = src;

    for (plane = 0; plane < 4; plane++)
    {
        dest = olddest;
        src = tempsrc + (plane * width * height);

        if (heightmod)
            src += (plane*heightmod*width);

        for (y = 0; y < height; y++)
        {
            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *src++;

                while (xscalelookup_plane[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (j = 0; j < yscalelookup[y]; j++)
                            if (bufferofsonly)
                                *(dest+bufferofs+(screen_src_width * j)) = pixel;
                            else
                            {
                                *(dest+page1start+(screen_src_width * j)) = pixel;
                                *(dest+page2start+(screen_src_width * j)) = pixel;
                                *(dest+page3start+(screen_src_width * j)) = pixel;
                            }
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookup[y]) - i;
        }
    }
}


//******************************************************************************
//
// DrawColoredMPPic ()
//
// Purpose
//    Draws a masked, planer pic at xpos, ypos.
//
// Parms
//    xpos   - x position.
//    ypos   - y position.
//    width  - width of pic : should be << 2.
//    height - height of pic.
//    src    - data to draw.
//
// Returns
//    Nothing.
//
//******************************************************************************

void DrawColoredMPPic (int xpos, int ypos, int width, int height, int heightmod,
                       byte *src, boolean bufferofsonly, int color)
{
    int olddest, dest;
    int plane, x, y, i, j;
    byte pixel;
    int finalw = width * screen_src_minscale * 4;
    byte *tempsrc;
    byte *cmap;

    cmap = playermaps[color] + (1 << 12);//pixel = *(cmap + (*src++));

    olddest = ylookup[ypos] + xpos;
    tempsrc = src;

    for (plane = 0; plane < 4; plane++)
    {
        dest = olddest;
        src = tempsrc + (plane * width * height);

        if (heightmod)
            src += (plane*heightmod*width);

        for (y = 0; y < height; y++)
        {
            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *(cmap + (*src++));

                while (xscalelookup_plane[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (j = 0; j < yscalelookup[y]; j++)
                            if (bufferofsonly)
                                *(dest+bufferofs+(screen_src_width * j)) = pixel;
                            else
                            {
                                *(dest+page1start+(screen_src_width * j)) = pixel;
                                *(dest+page2start+(screen_src_width * j)) = pixel;
                                *(dest+page3start+(screen_src_width * j)) = pixel;
                            }
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookup[y]) - i;
        }
    }
}


//******************************************************************************
//
// UpdateScore ()
//
//******************************************************************************

void UpdateScore (unsigned int num)
{
    if (num > 999999999)
    {
        num = 999999999;
        gamestate.score = 999999999;
    }

    ltoa(num, ScoreStr.str, 10);
    ScoreStr.length = strlen(ScoreStr.str);
}


//******************************************************************************
//
// UpdateLives ()
//
//******************************************************************************

void UpdateLives (int num)
{
    ltoa(num, LivesStr.str, 10);
    LivesStr.length = strlen(LivesStr.str);
}

//****************************************************************************
//
// ClearTriads ()
//
//****************************************************************************
void ClearTriads (playertype *pstate)
{
    pstate->triads = 0;
    ltoa(pstate->triads, TriadStr.str, 10);
    TriadStr.length = strlen(TriadStr.str);
}

//****************************************************************************
//
// UpdateTriads ()
//
//****************************************************************************

void UpdateTriads (objtype *ob, int num)
{
    playertype *pstate;

    M_LINKSTATE(ob,pstate);

    pstate->triads += num;

    if (pstate->triads >= 100)
    {
        GiveLives(1);

        if (ob == player)
        {
            AddMessage("100 Life Item Points!  Extra Life!", MSG_BONUS);
            SD_PlaySoundRTP(SD_GET1UPSND, player->x, player->y);
        }

        pstate->triads -= 100;
    }

    if (ob == player)
    {
        ltoa(pstate->triads, TriadStr.str, 10);
        TriadStr.length = strlen(TriadStr.str);
    }
}

//****************************************************************************
//
// DrawTriads ()
//
//****************************************************************************

void DrawTriads (boolean bufferofsonly)
{
    if (!SHOW_TOP_STATUS_BAR())
        return;

    if (!EndLevelStuff)
        DrawNumber(TRIAD_X * screen_src_minscale, TRIAD_Y * screen_src_minscale, 2, 3, bufferofsonly);
}


//******************************************************************************
//
// DrawPPic ()
//
//******************************************************************************

void DrawPPic (int xpos, int ypos, int width, int height, byte *src, int num,
               boolean up, boolean bufferofsonly)
{
    int olddest, dest;
    int plane, x, y, i, j;
    byte pixel;
    int k, amt;
    int finalw = width * screen_src_minscale * 4;
    byte *tempsrc;

    if (up)
        amt = 8 * screen_src_minscale;
    else
        amt = -8 * screen_src_minscale;

    olddest = ylookup[ypos] + xpos;
    tempsrc = src;

    for (plane = 0; plane < 4; plane++)
    {
        dest = olddest;
        src = tempsrc + (plane * width * height);

        for (y = 0; y < height; y++)
        {
            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *src++;

                while (xscalelookup_plane[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (k = 0; k < num; k++)
                            for (j = 0; j < yscalelookup[y]; j++)
                                if (bufferofsonly)
                                    *(dest+bufferofs+(screen_src_width * j)+(amt*k)) = pixel;
                                else
                                {
                                    *(dest+page1start+(screen_src_width * j)+(amt*k)) = pixel;
                                    *(dest+page2start+(screen_src_width * j)+(amt*k)) = pixel;
                                    *(dest+page3start+(screen_src_width * j)+(amt*k)) = pixel;
                                }
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookup[y]) - i;
        }
    }
}


//****************************************************************************
//
// DrawBarHealth ()
//
//****************************************************************************

void DrawBarHealth (boolean bufferofsonly)
{
    int percenthealth;
    int health_y;

    if (!SHOW_BOTTOM_STATUS_BAR())
        return;

    health_y = screen_src_health_y;

    if (SHOW_KILLS())
        health_y -= KILLS_HEIGHT * screen_src_minscale;

    percenthealth = (locplayerstate->health * 10) / MaxHitpointsForCharacter(locplayerstate);

    oldpercenthealth = percenthealth + 1;

    if (playstate == ex_died)
    {
        DrawPPic(screen_src_statusbar_x + screen_src_health_x, health_y, 8 >> 2, 16, (byte *)&erase->data,
                 10, true, bufferofsonly);

        return;
    }

    if (locplayerstate->health <= 0)
        oldpercenthealth = 0;

    if (oldpercenthealth >= 11)
        oldpercenthealth = 10;

    if (oldpercenthealth < 4)
        DrawPPic(screen_src_statusbar_x + screen_src_health_x, health_y, 8 >> 2, 16,
                 (byte *)&health[0]->data, oldpercenthealth,
                 true, bufferofsonly );
    else if (oldpercenthealth < 5)
        DrawPPic(screen_src_statusbar_x + screen_src_health_x, health_y, 8 >> 2, 16,
                 (byte *)&health[1]->data, oldpercenthealth,
                 true, bufferofsonly);
    else
        DrawPPic(screen_src_statusbar_x + screen_src_health_x, health_y, 8 >> 2, 16,
                 (byte *)&health[2]->data, oldpercenthealth,
                 true, bufferofsonly);

    if (oldpercenthealth < 10)
        DrawPPic(screen_src_statusbar_x + screen_src_health_x + (8 * oldpercenthealth * screen_src_minscale),
                 health_y, 8 >> 2, 16, (byte *)&erase->data, 10 - oldpercenthealth,
                 true, bufferofsonly);
}


//****************************************************************************
//
// DrawBarAmmo ()
//
//****************************************************************************

void DrawBarAmmo (boolean bufferofsonly)
{
    int ammo_y;

    if (!SHOW_BOTTOM_STATUS_BAR() || (playstate == ex_died))
        return;

    ammo_y = screen_src_ammo_y;

    if (SHOW_KILLS())
        ammo_y -= KILLS_HEIGHT * screen_src_minscale;

    DrawPPic (screen_src_statusbar_x + screen_src_ammo_x_bar, ammo_y + (1* screen_src_minscale),
              8 >> 2, 16, (byte *)&erase->data, 10, false, bufferofsonly);

    if (!ARMED(player->dirchoosetime))
        return;

    if ((locplayerstate->new_weapon < wp_bazooka) ||
            (locplayerstate->new_weapon == wp_godhand) ||
            (gamestate.BattleOptions.Ammo == bo_infinite_shots))
    {
        DrawPPic(screen_src_statusbar_x + screen_src_ammo_x_bar - (16 * screen_src_minscale),
                 ammo_y, 24 >> 2, 16, (byte *)&ammo[0]->data, 1, true, bufferofsonly);

        DrawPPic(screen_src_statusbar_x + screen_src_ammo_x_bar - (32 * screen_src_minscale),
                 ammo_y + (1 * screen_src_minscale), 8 >> 2, 16, (byte *)&erase->data,
                 2, true, bufferofsonly);
    }
    else if (locplayerstate->new_weapon == wp_dog)
    {
        DrawPPic(screen_src_statusbar_x + screen_src_ammo_x_bar - (16 * screen_src_minscale),
                 ammo_y, 24 >> 2, 16, (byte *)&ammo[12]->data, 1, true, bufferofsonly);

        DrawPPic(screen_src_statusbar_x + screen_src_ammo_x_bar - (32 * screen_src_minscale),
                 ammo_y + (1 * screen_src_minscale), 8 >> 2, 16, (byte *)&erase->data,
                 2, true, bufferofsonly);
    }
    else
    {
        DrawPPic(screen_src_statusbar_x + screen_src_ammo_x_bar, ammo_y + (1 * screen_src_minscale),
                 8 >> 2, 16, (byte *)&ammo[ locplayerstate->new_weapon]->data,
                 locplayerstate->ammo, false, bufferofsonly);
    }
}


//******************************************************************************
//
// SingleDrawPPic ()
//
//******************************************************************************

void SingleDrawPPic (int xpos, int ypos, int width, int height, byte *src, int num, boolean up)
{
    byte *olddest, *dest;
    int plane, x, y, i, j;
    byte pixel;
    int k, amt;
    int finalw = width * screen_src_minscale * 4;
    byte *tempsrc;

    if (up)
        amt = 8 * screen_src_minscale;
    else
        amt = -8 * screen_src_minscale;

    olddest = (byte *)(bufferofs - screenofs + ylookup[ypos] + xpos);
    tempsrc = src;

    for (plane = 0; plane < 4; plane++)
    {
        dest = olddest;
        src = tempsrc + (plane * width * height);

        for (y = 0; y < height; y++)
        {
            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *src++;

                while (xscalelookup_plane[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == plane && i < finalw)
                {
                    if (pixel != 255)
                        for (k = 0; k < num; k++)
                            for (j = 0; j < yscalelookup[y]; j++)
                                *(dest+(screen_src_width * j)+(amt*k)) = pixel;
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookup[y]) - i;
        }
    }
}



//****************************************************************************
//
// DrawStats ()
//
//****************************************************************************

void DrawStats (void)
{
    int percenthealth;
    int health_y;
    int ammo_y;

    if (!SHOW_PLAYER_STATS() || (playstate == ex_died) || (locplayerstate->health <= 0))
        return;

    health_y = screen_src_health_y;
    ammo_y  = screen_src_ammo_y;

    if (SHOW_KILLS())
    {
        health_y -= KILLS_HEIGHT * screen_src_minscale;
        ammo_y   -= KILLS_HEIGHT * screen_src_minscale;
    }

    if (oldplayerhealth != locplayerstate->health)
    {
        oldplayerhealth = locplayerstate->health;

        percenthealth = (locplayerstate->health * 10) / MaxHitpointsForCharacter(locplayerstate);

        oldpercenthealth = percenthealth + 1;
    }

    if (oldpercenthealth > 10)
        oldpercenthealth = 10;

    if (oldpercenthealth < 4)
        SingleDrawPPic(screen_src_health_x - (16 * screen_src_minscale), health_y, 8 >> 2, 16,
                       (byte *)&health[3]->data, oldpercenthealth, true);
    else if (oldpercenthealth < 5)
        SingleDrawPPic(screen_src_health_x - (16 * screen_src_minscale), health_y, 8 >> 2, 16,
                       (byte *)&health[4]->data, oldpercenthealth, true);
    else
        SingleDrawPPic(screen_src_health_x - (16 * screen_src_minscale), health_y, 8 >> 2, 16,
                       (byte *)&health[5]->data, oldpercenthealth, true);

    if (ARMED(consoleplayer))
    {
        if ((locplayerstate->new_weapon < wp_bazooka) ||
                (locplayerstate->new_weapon == wp_godhand) ||
                (gamestate.BattleOptions.Ammo == bo_infinite_shots))
            SingleDrawPPic(screen_src_ammo_x - (16 * screen_src_minscale), ammo_y, 24 >> 2, 16,
                           (byte *)&ammo[13]->data, 1, true);
        else if (locplayerstate->new_weapon == wp_dog)
            SingleDrawPPic(screen_src_ammo_x - (16 * screen_src_minscale), ammo_y + (1 * screen_src_minscale), 24 >> 2, 16,
                           (byte *)&ammo[25]->data, 1, true);
        else
            SingleDrawPPic(screen_src_ammo_x, ammo_y + (1 * screen_src_minscale), 8 >> 2, 16,
                           (byte *)&ammo[13 + locplayerstate->new_weapon]->data,
                           locplayerstate->ammo, false);
    }
}


//****************************************************************************
//
// DrawPauseXY ()
//
//****************************************************************************

void DrawPauseXY (int x, int y)
{
    pic_t *p;

    if (GamePaused)
    {
        p = (pic_t *)W_CacheLumpNum(W_GetNumForName ("paused"), PU_CACHE, Cvt_pic_t, 1);
        VL_DrawPic(x, y, p);
        DrawEpisodeLevel(x, y);
    }
    else
    {
        p = (pic_t *)W_CacheLumpNum(W_GetNumForName ("wait"), PU_CACHE, Cvt_pic_t, 1);
        VL_DrawPic(x, y, p);
    }
}

//****************************************************************************
//
// DrawPause ()
//
//****************************************************************************

void DrawPause ()
{
    pic_t *p;
    byte *bufftemp = bufferofs;

    bufferofs -= screenofs;

    if (GamePaused)
    {
        p = (pic_t *) W_CacheLumpNum(W_GetNumForName("paused"), PU_CACHE, Cvt_pic_t, 1);

        DrawPauseXY((int)(screen_src_width - ((p->width << 2) * screen_src_minscale)) >> 1,
                    (int)(screen_src_height - (p->height * screen_src_minscale)) >> 1);
    }
    else
    {
        p = (pic_t *) W_CacheLumpNum(W_GetNumForName("wait"), PU_CACHE, Cvt_pic_t, 1);

        DrawPauseXY((int)(screen_src_width - ((p->width << 2) * screen_src_minscale)) >> 1,
                    (int)(screen_src_height - (p->height * screen_src_minscale)) >> 1);
    }

    bufferofs = bufftemp;
}

//****************************************************************************
//
// GM_DrawBonus ()
//
//****************************************************************************

void GM_DrawBonus (int which)
{
    int x;

    if (which < stat_gasmask)
    {
        x = POWERUP1X;
        poweruptime = GetBonusTimeForItem(which);
        poweradjust = poweruptime >> 4;
        powerupheight = 0;
        GM_UpdateBonus(poweruptime - poweradjust - 1, true);
    }
    else
    {
        x = POWERUP2X;
        protectiontime = GetBonusTimeForItem(which);
        poweradjust = protectiontime >> 4;
        protectionheight = 0;
        GM_UpdateBonus(protectiontime - poweradjust - 1, false);
    }
}


//******************************************************************************
//
// GM_UpdateBonus ()
//
//******************************************************************************

void GM_UpdateBonus (int time, int powerup)
{
    pic_t *shape;
    int shapenum;

    if (powerup)
    {
        if (time < (poweruptime - poweradjust))
        {
            powerupheight++;

            if (!SHOW_TOP_STATUS_BAR())
                poweruptime = time;
        }
    }
    else
    {
        if (time < (protectiontime - poweradjust))
        {
            protectionheight++;

            if (!SHOW_TOP_STATUS_BAR())
                protectiontime = time;
        }
    }


    if (!SHOW_TOP_STATUS_BAR())
        return;

    if (!time)
    {
        if (powerup == 1)
            shapenum = POWERUP1X;
        else
            shapenum = POWERUP2X;

        GM_MemToScreen(( byte * )&eraseb->data, eraseb->width,
                       eraseb->height, screen_src_statusbar_x + (shapenum * screen_src_minscale),
                       POWERUPY * screen_src_minscale);

        return;
    }

    if (powerup)
    {
        if (time < (poweruptime - poweradjust))
        {
            if (player->flags & FL_GODMODE)
                shapenum = powerpics;
            else if (player->flags & FL_DOGMODE)
                shapenum = powerpics + 1;
            else if (player->flags & FL_FLEET)
                shapenum = powerpics + 2;
            else if (player->flags & FL_ELASTO)
                shapenum = powerpics + 3;
            else if (player->flags & FL_SHROOMS)
                shapenum = powerpics + 4;
            else
            {
                GM_MemToScreen((byte *)&eraseb->data, eraseb->width, eraseb->height,
                               screen_src_statusbar_x + (POWERUP1X * screen_src_minscale), POWERUPY * screen_src_minscale);

                return;
            }

            poweruptime = time;

            shape = (pic_t *)W_CacheLumpNum(shapenum, PU_CACHE, Cvt_pic_t, 1);

            GM_MemToScreen((byte *)&eraseb->data, eraseb->width,
                           eraseb->height, screen_src_statusbar_x + (POWERUP1X * screen_src_minscale),
                           POWERUPY * screen_src_minscale);

            DrawMPPic(screen_src_statusbar_x + (POWERUP1X * screen_src_minscale),
                      (POWERUPY + powerupheight) * screen_src_minscale, shape->width,
                      shape->height - powerupheight, powerupheight,
                      (byte *)&shape->data, false);
        }
    }
    else
    {
        if (time < (protectiontime - poweradjust))
        {
            if (player->flags & FL_BPV)
                shapenum = powerpics + 6;
            else if (player->flags & FL_GASMASK)
                shapenum = powerpics + 5;
            else if (player->flags & FL_AV)
                shapenum = powerpics + 7;

            protectiontime = time;

            shape = (pic_t *)W_CacheLumpNum( shapenum, PU_CACHE, Cvt_pic_t, 1);

            GM_MemToScreen((byte *)&eraseb->data, eraseb->width,
                           eraseb->height, screen_src_statusbar_x + (POWERUP2X * screen_src_minscale),
                           POWERUPY * screen_src_minscale);

            DrawMPPic(screen_src_statusbar_x + (POWERUP2X * screen_src_minscale),
                      (POWERUPY + protectionheight) * screen_src_minscale,
                      shape->width, shape->height - protectionheight,
                      protectionheight, (byte *)&shape->data, false);
        }
    }
}


//******************************************************************************
//
// DrawEpisodeLevel ()
//
// right justifies and pads with blanks
//
//******************************************************************************

void DrawEpisodeLevel (int x, int y)
{
    int level;
    char str[20];
    pic_t *p;

    if (!BATTLEMODE)
    {
        ltoa(gamestate.episode, str, 10);

        VL_MemToScreen((byte *)&timenums[str[0]-'0']->data, 8>>2, 16,
                       x + (29 * screen_src_minscale), y + (16 * screen_src_minscale));

        if ((gamestate.mapon == 6) || (gamestate.mapon == 14) ||
                (gamestate.mapon == 22) || (gamestate.mapon == 32) ||
                (gamestate.mapon == 33))
        {
            p = (pic_t *)W_CacheLumpName("tnumb", PU_CACHE, Cvt_pic_t, 1);
            VL_MemToScreen((byte *)&p->data, 8>>2, 16,
                           x + (40 * screen_src_minscale), y + (16 * screen_src_minscale));

            if (gamestate.mapon == 6)
                level = 1;
            else if (gamestate.mapon == 14)
                level = 2;
            else if (gamestate.mapon == 22)
                level = 3;
            else if (gamestate.mapon == 32)
                level = 4;
            else
                level = 5;
        }
        else
            level = GetLevel(gamestate.episode, gamestate.mapon);

        level = abs(level);
        ltoa(level, str, 10);
    }
    else
    {
        p = (pic_t *) W_CacheLumpName ("battp", PU_CACHE, Cvt_pic_t, 1);
        VL_MemToScreen ((byte *)&p->data, 32>>2, 16,
                        x + (16 * screen_src_minscale), y + (15 * screen_src_minscale));

        level = abs(gamestate.mapon + 1);
        ltoa(level, str, 10);
    }

    VL_MemToScreen((byte *)&timenums[str[0]-'0']->data, 8>>2, 16,
                   x + (49 * screen_src_minscale), y + (16 * screen_src_minscale));

    if (level >= 10)
        VL_MemToScreen((byte *)&timenums[str[1]-'0']->data, 8>>2, 16,
                       x + (57 * screen_src_minscale), y + (16 * screen_src_minscale));
}


//******************************************************************************
//
// GM_MemToScreen ()
//
//******************************************************************************

void GM_MemToScreen (byte *src, int width, int height, int xpos, int ypos)
{
    int olddest, dest;
    int plane, x, y, i, j;
    byte pixel;
    int finalw = width * screen_src_minscale * 4;
    byte *tempsrc;

    olddest = ylookup[ypos] + xpos;
    tempsrc = src;

    for (plane = 0; plane < 4; plane++)
    {
        dest = olddest;
        src = tempsrc + (plane * width * height);

        for (y = 0; y < height; y++)
        {
            i = 0;

            for (x = 0; x < width; x++)
            {
                pixel = *src++;

                while (xscalelookup_plane[i] != plane && i < finalw)
                {
                    i++;
                    dest++;
                }

                while (xscalelookup_plane[i] == plane && i < finalw)
                {
                    for (j = 0; j < yscalelookup[y]; j++)
                        *(dest+page1start+(screen_src_width * j)) = pixel;
                    //*(dest+page2start+(screen_src_width * j)) = pixel;
                    //*(dest+page3start+(screen_src_width * j)) = pixel;
                    i++;
                    dest++;
                }
            }

            dest += (screen_src_width * yscalelookup[y]) - i;
        }
    }
}


//==========================================================================

/*
==================
=
= ScreenShake
=
==================
*/

void ScreenShake (void)
{
    if (SHAKETICS != 0xFFFF)
    {
        SHAKETICS -= tics;

        //bna safety val check
        if (SHAKETICS >= 0xFF00)
            SHAKETICS = 0xFFFF;

        screen_shake = RandomNumber("ScreenShake", 0) & 3;
    }
}

//******************************************************************************
//
// DoBorderShifts ()
//
//******************************************************************************

void DoBorderShifts (void)
{
    if (damagecount)
    {
        if (damagecount > 100)
            damagecount = 100;

        damagecount -= 6;

        if (damagecount < 0)
            damagecount = 0;

        SetBorderColor(*(colormap + (((100 - damagecount) >> 2) << 8) + 48));
        borderset = true;
    }
    else if (borderset)
    {
        SetBorderColor(0);
        borderset = false;
    }
}


//******************************************************************************
//
// DrawHighScores ()
//
//******************************************************************************

void DrawHighScores (void)
{
    char buffer[16];
    char buffer1[5];

    int i, w, h;
    HighScore *s;

    for (i = 0, s = Scores; i < MaxScores; i++, s++)
    {
        PrintY = 25 + (16 * i);

        //
        // name
        //
        PrintX = 3*8;
        DrawMenuBufPropString(PrintX, PrintY, s->name);

        //
        // level
        //
        ultoa(s->completed, buffer, 10);

        PrintX = (17 * 8)-10;

        if (shareware)
            DrawMenuBufPropString(PrintX, PrintY, "S");
        else
        {
            itoa(s->episode, buffer1, 10);

            DrawMenuBufPropString(PrintX, PrintY, buffer1);
        }

        DrawMenuBufPropString(PrintX, PrintY, "-");

        if (s->completed == 7 && !shareware)
            DrawMenuBufPropString(PrintX, PrintY, "B");
        else if (s->completed == 8 && !shareware)
            DrawMenuBufPropString(PrintX, PrintY, "S");
        else if (s->completed == 9 && !shareware)
            DrawMenuBufPropString(PrintX, PrintY, "C");
        else if (s->completed == 10 && !shareware)
            DrawMenuBufPropString(PrintX, PrintY, "D");
        else
            DrawMenuBufPropString(PrintX, PrintY, buffer);

        //
        // score
        //
        ultoa(s->score, buffer, 10);

        VW_MeasurePropString (buffer, &w, &h);
        PrintX = (33 * 8) - w;
        DrawMenuBufPropString (PrintX, PrintY, buffer);
    }
}



//******************************************************************************
//
// CheckHighScore ()
//
//******************************************************************************

void CheckHighScore (long score, word other, boolean INMENU, int rtl)
{
    word        i,j;
    int         n;
    HighScore   myscore;
    int         level;

    if (!pure)
    {
        ReadNewScores(rtl);
    }

    MenuFadeIn();
    if (!INMENU)
        SetupMenuBuf ();

    strcpy (myscore.name,"");
    myscore.score     = score;

    level = GetLevel (gamestate.episode, other-1);

    myscore.episode   = gamestate.episode;
    myscore.completed = level;

    CurrentFont = smallfont;

    for (i = 0, n = -1; i < MaxScores; i++)
    {
        if ((myscore.score > Scores[i].score)  ||
                ((myscore.score == Scores[i].score) &&
                 (myscore.completed > Scores[i].completed)))
        {
            for (j = MaxScores; --j > i;)
                Scores[j] = Scores[j - 1];
            Scores[i] = myscore;
            n = i;
            break;
        }
    }

    if (INMENU)
    {
        SetAlternateMenuBuf();
        SetMenuTitle ("High Scores");
        ClearMenuBuf();
        DrawHighScores ();
        if (n != -1)
            DisplayInfo (6);
        else
            DisplayInfo (5);
        FlipMenuBuf ();
    }
    else
    {
        ClearMenuBuf ();
        SetMenuTitle ("High Scores");
        DrawHighScores ();
        if (n != -1)
            DisplayInfo (6);
        else
            DisplayInfo (5);
        RefreshMenuBuf (0);
    }

    if (n != -1)
    {
        PrintY = 25 + (16 * n);
        PrintX = 3*8;
        US_LineInput (PrintX, PrintY, Scores[n].name, NULL,
                      true, 10, 98, 0);

        if (!pure)
            WriteNewScores();
    }
    else
    {
        IN_ClearKeysDown ();
        if ( INMENU )
        {
            while( !IN_CheckAck () )
            {
                RefreshMenuBuf (0);
            }
        }
        else
        {
            for( i = 0; i <= 150; i += tics )
            {
                RefreshMenuBuf (0);
                if (IN_CheckAck ())
                {
                    break;
                }
            }
        }
    }

    if (INMENU)
    {
        SD_Play (SD_ESCPRESSEDSND);
    }
    else
    {
        ShutdownMenuBuf ();
    }
}


//===========================================================================

//#define HEADERX      140
//#define BONERNAMEX   170
#define HEADERX      152
#define BONERNAMEX   166


/*
==================
=
= DrawEOLHeader ()
=
==================
*/
void DrawEOLHeader (int playstate)
{
    int health;
    char tempstr[15];
    char *string;
    int level;
    int w;
    int h;
    char str1[10];
    char str2[10];

    // Originally x=30, width=250.
    int xStart = (screen_src_width - (250 * screen_src_minscale)) / 2.0;

    VL_TBar(xStart, (int)(5 * screen_src_minscale),
            (int)(250 * screen_src_minscale), (int)(75 * screen_src_minscale));

    switch(playstate)
    {
        case ex_skiplevel:
            if ((gamestate.violence >= vl_high) &&
                    (gamestate.difficulty >= gd_hard))
                string = "LEVEL WUSSED OUT ON!";
            else
                string = "LEVEL SKIPPED.";
            break;

        case ex_secretdone:
            string = "SECRET LEVEL COMPLETED!";
            break;

        case ex_secretlevel:
            string = "SECRET EXIT TAKEN!";
            break;

        case ex_gameover:
            string = "GAME COMPLETED!";
            break;

        case ex_bossdied:
            string = "BOSS DEFEATED!";
            break;

        default:
            string = "LEVEL COMPLETED!";
            break;
    }

    VW_MeasurePropString(string, &w, &h);

    px = (screen_src_width - (w * screen_src_minscale)) / 2;
    py = 10 * screen_src_minscale;
    VW_DrawPropString(string);

    // draw episode number
    string = "EPISODE";
    VW_MeasurePropString(string, &w, &h);
    px = xStart + ((HEADERX - w - 30) * screen_src_minscale);
    py = 25 * screen_src_minscale;
    VW_DrawPropString(string);

    itoa(gamestate.episode, tempstr, 10);
    px = xStart + ((BONERNAMEX - 30) * screen_src_minscale);
    VW_DrawPropString(tempstr);

    // draw area number
    level = GetLevel(gamestate.episode, gamestate.mapon);
    itoa(level, tempstr, 10);

    py = 35 * screen_src_minscale;

    if (playstate == ex_secretdone)
        string = "SECRET AREA";
    else if (playstate == ex_bossdied)
        string = "BOSS AREA";
    else if (gamestate.mapon == 32)
        string = "CHASE AREA";
    else
        string = "AREA";

    VW_MeasurePropString(string, &w, &h);
    px = xStart + ((HEADERX - w - 30) * screen_src_minscale);
    VW_DrawPropString(string);

    if ( gamestate.mapon != 33 )
    {
        px = xStart + ((BONERNAMEX - 30) * screen_src_minscale);
        VW_DrawPropString(tempstr);
    }

    string = "SCORE";
    VW_MeasurePropString(string, &w, &h);
    px = xStart + ((HEADERX - w - 30) * screen_src_minscale);
    py = 45 * screen_src_minscale;
    VW_DrawPropString(string);

    px = xStart + ((BONERNAMEX - 30) * screen_src_minscale);
    itoa(gamestate.score, tempstr, 10);
    VW_DrawPropString(tempstr);

    string = "HEALTH";
    VW_MeasurePropString(string, &w, &h);
    px = xStart + ((HEADERX - w - 30) * screen_src_minscale);
    py = 55 * screen_src_minscale;
    VW_DrawPropString(string);

    px = xStart + ((BONERNAMEX - 30) * screen_src_minscale);
    health = ((locplayerstate->health * 100) /
              MaxHitpointsForCharacter(locplayerstate));

    itoa(health, tempstr, 10);
    VW_DrawPropString(tempstr);
    VW_DrawPropString("%");

    //
    // Secret count
    //
    itoa(gamestate.secretcount,&(str1[0]),10);
    strcat(str1," / ");
    itoa(gamestate.secrettotal,&(str2[0]),10);
    strcat(str1,str2);

    string = "SECRET WALLS";
    VW_MeasurePropString(string, &w, &h);
    px = xStart + ((HEADERX - w - 30) * screen_src_minscale);
    py = 65 * screen_src_minscale;
    VW_DrawPropString(string);

    px = xStart + ((BONERNAMEX - 30) * screen_src_minscale);
    VW_DrawPropString(str1);

    VW_UpdateScreen ();
}

boolean EndBonusFirst;
boolean EndBonusSkip;
int EndBonusNumBonuses;
int EndBonusVoice;
int EndBonusStartY;

void DrawEndBonus (char *string, char *bonusstring, int type)
{
    int w;
    int h;
    int health;
    char tempstr[15];

    // Originally x=5, width=310.
    int xStart = (screen_src_width - (310 * screen_src_minscale)) / 2.0;

    // Originally x=30, width=250.
    int topXStart = (screen_src_width - (250 * screen_src_minscale)) / 2.0;

    if (EndBonusFirst)
    {
        VL_TBar(xStart, (int)((EndBonusStartY - 2) * screen_src_minscale),
                (int)(310 * screen_src_minscale), (int)(4 * screen_src_minscale));
        EndBonusFirst = false;
    }

    VL_TBar(xStart, (int)((EndBonusStartY + 2) * screen_src_minscale),
            (int)(310 * screen_src_minscale), (int)(10 * screen_src_minscale));
    VW_MeasurePropString(string, &w, &h);

    py = EndBonusStartY * screen_src_minscale;
    if (bonusstring == NULL)
    {
        px = (screen_src_width - (w * screen_src_minscale)) / 2;
        VW_DrawPropString(string);
    }
    else
    {
        px = xStart + ((BONERNAMEX - w - 5) * screen_src_minscale);
        VW_DrawPropString(string);

        EndBonusNumBonuses++;
        VW_MeasurePropString(bonusstring, &w, &h);
        px = xStart + ((310 - w - 5) * screen_src_minscale);
        py = EndBonusStartY * screen_src_minscale;
        VW_DrawPropString(bonusstring);
    }

    // Update Score
    py = 45 * screen_src_minscale;
    px = topXStart + ((BONERNAMEX - 30) * screen_src_minscale);
    VL_DrawPicRedraw(px, py, (int)(107 * screen_src_minscale), (int)(21 * screen_src_minscale), BkPic);
    VL_TBar(px, py, (int)(107 * screen_src_minscale), (int)(21 * screen_src_minscale));
    itoa(gamestate.score, tempstr, 10);
    VW_DrawPropString(tempstr);

    // Update Health
    py = 55 * screen_src_minscale;
    px = topXStart + ((BONERNAMEX - 30) * screen_src_minscale);
    health = ((locplayerstate->health * 100) / MaxHitpointsForCharacter(locplayerstate));
    itoa(health, tempstr, 10);
    VW_DrawPropString(tempstr);
    VW_DrawPropString("%");

    switch(type)
    {
        case 0:
            EndBonusVoice = SD_Play(SD_ENDBONUS1SND);
            break;

        case 1:
            EndBonusVoice = SD_Play(SD_NOBONUSSND);
            break;

        case 2:
            VL_FillPalette(255, 255, 255);
            VW_UpdateScreen();
            VL_FadeIn(0, 255, origpal, 10);
            EndBonusVoice = SD_Play(SD_LIGHTNINGSND);
            break;
    }

    EndBonusStartY += 10;

    VW_UpdateScreen();
    while(SD_SoundActive(EndBonusVoice) && !EndBonusSkip)
        if (IN_CheckAck())
            EndBonusSkip = true;
}



/*
==================
=
= LevelCompleted
=
= Exit with the screen faded out
=
==================
*/

extern int OLDLMWEAPON;
extern int OLDLWEAPON;

void LevelCompleted (exit_t playstate)
{
    objtype *obj;
    boolean dobonus;
    int i;
    int kr;
    int sr;
    int tr;
    int missileratio;
    int superratio;
    int healthratio;
    int democraticratio;
    int plantratio;
    int cnt;

    EndBonusNumBonuses = 0;
    EndBonusFirst = true;
    EndBonusSkip = false;
    EndBonusStartY = 90;

    IN_StartAck();
    EndBonusVoice = 0;

    if (playstate != ex_bossdied)
    {
        EndBonusVoice = SD_Play(SD_LEVELDONESND);
        VL_FillPalette(255, 255, 255);
        VL_FadeIn(0, 255, origpal, 10);

        if (player->flags & FL_DOGMODE)
            MU_StartSong(song_dogend);
        else
            MU_StartSong(song_endlevel);
    }

    BkPic = (pic_t *)W_CacheLumpName("mmbk", PU_CACHE, Cvt_pic_t, 1);
    VL_DrawPicFullscreen(BkPic);

    CheckHolidays();
    CurrentFont = smallfont;

    // Kill powerups
    if (player->flags & FL_ELASTO)
        player->flags &= ~FL_NOFRICTION;

    player->flags &= ~(FL_FLEET | FL_SHROOMS | FL_ELASTO | FL_GODMODE |
                       FL_DOGMODE | FL_BPV | FL_AV | FL_GASMASK);

    // Turn off quickload for next level
    pickquick = false;

    //
    // FIGURE RATIOS OUT BEFOREHAND
    //
    kr = 0;
    tr = 0;
    tr = 0;
    superratio = 0;
    missileratio = 0;
    healthratio = 0;
    democraticratio = 0;
    plantratio = 0;

    if (gamestate.killtotal)
        kr = (gamestate.killcount * 100) / gamestate.killtotal;

    if (gamestate.secrettotal)
        sr = (gamestate.secretcount * 100) / gamestate.secrettotal;

    if (gamestate.treasuretotal)
        tr = (gamestate.treasurecount * 100) / gamestate.treasuretotal;

    if (gamestate.supertotal)
        superratio = (gamestate.supercount * 100) / gamestate.supertotal;

    if (gamestate.missiletotal)
        missileratio = (gamestate.missilecount * 100) / gamestate.missiletotal;

    if (gamestate.healthtotal)
        healthratio = (gamestate.healthcount * 100) / gamestate.healthtotal;

    if (gamestate.democratictotal)
        democraticratio = (gamestate.democraticcount * 100) / gamestate.democratictotal;

    if (gamestate.planttotal)
        plantratio = (gamestate.plantcount * 100) / gamestate.planttotal;

    DrawEOLHeader(playstate);

    while(SD_SoundActive(EndBonusVoice) && !EndBonusSkip)
    {
        if (IN_CheckAck())
            EndBonusSkip = true;
    }

    if (GetNextMap(player->tilex,player->tiley) == -1)
    {
        if (gamestate.dipballs == 3)
        {
            gamestate.score += 100000;
            DrawEndBonus("DIP BONUS", "100000 POINTS", 0);
            EndBonusStartY += 10;
        }

        if (locplayerstate->lives > 0)
        {
            char str[20];
            char str2[60];

            DrawEndBonus("EXTRA LIVES BONUS", "\0", 0);

            itoa(locplayerstate->lives, str, 10);
            strcpy(str2, str);
            strcat(str2, " EXTRA LIVES =");
            DrawEndBonus("\0", str2, 0);

            itoa(locplayerstate->lives, str, 10);
            strcpy(str2, str);
            strcat(str2, " X 10000 = ");

            itoa(locplayerstate->lives * 10000, str, 10);
            strcat(str2, str);
            strcat(str2, " POINTS");

            gamestate.score += 10000 * locplayerstate->lives;
            DrawEndBonus("\0", str2, 0);
        }
    }
    else
    {
        //
        // Check for SKIN OF YO TEETH
        //
        if (locplayerstate->health <= 10)
        {
            locplayerstate->health = MaxHitpointsForCharacter(locplayerstate);
            DrawEndBonus("SKIN OF YOUR TEETH", "100% HEALTH", 0);
        }

        // BULL IN CHINA SHOP BONUS
        if (tr == 100)
        {
            gamestate.score += 10000;
            DrawEndBonus("BULL IN CHINA SHOP", "10000 POINTS", 0);
        }

        // SUPERCHARE BONUS
        if (superratio == 100)
        {
            gamestate.score += 10000;
            DrawEndBonus("SUPERCHARGE BONUS", "10000 POINTS", 0);
        }

        // BLEEDER BONUS
        if (healthratio == 100)
        {
            gamestate.score += 10000;
            DrawEndBonus("BLEEDER BONUS", "10000 POINTS", 0);
        }

        // ADRENALINE BONUS
        if (kr == 100)
        {
            gamestate.score += 10000;
            DrawEndBonus("ADRENALINE BONUS", "10000 POINTS", 0);
        }

        // CURIOSITY BONUS
        dobonus = true;

        //
        // Check switches
        cnt = lastswitch - &switches[0];
        if (cnt != 0)
            for (i = 0; i < cnt; i++)
                if ((switches[i].flags & FL_S_FLIPPED) == 0)
                {
                    dobonus = false;
                    break;
                }

        //
        // Check pillars
        for (obj = FIRSTACTOR; obj != NULL; obj = obj->next)
            if ((obj->obclass == pillarobj) && ((obj->flags & FL_FLIPPED) == 0))
                dobonus = false;

        if ((gamestate.secrettotal) && (sr != 100))
            dobonus = false;

        if (dobonus)
        {
            gamestate.score += 10000;
            DrawEndBonus("CURIOSITY BONUS", "10000 POINTS", 0);
        }

        // GROUND ZERO BONUS
        if (gamestate.DOGROUNDZEROBONUS)
        {
            gamestate.score += 10000;
            DrawEndBonus("GROUND ZERO BONUS", "10000 POINTS", 0);
        }

        // REPUBLICAN BONUS 1
        if (missileratio == 100)
        {
            gamestate.score += 5000;
            DrawEndBonus("REPUBLICAN BONUS 1", " 5000 POINTS", 0);
        }

        // REPUBLICAN BONUS 2
        if (plantratio == 100)
        {
            gamestate.score += 5000;
            DrawEndBonus("REPUBLICAN BONUS 2", " 5000 POINTS", 0);
        }

        // DEMOCRATIC BONUS 1
        if (gamestate.DODEMOCRATICBONUS1)
        {
            gamestate.score += 5000;
            DrawEndBonus("DEMOCRATIC BONUS 1", " 5000 POINTS", 0);
        }

        // DEMOCRATIC BONUS 2
        if (democraticratio == 100)
        {
            gamestate.score += 5000;
            DrawEndBonus("DEMOCRATIC BONUS 2", " 5000 POINTS", 0);
        }
    }

    if (EndBonusNumBonuses == 0)
        DrawEndBonus("NO BONUS!", NULL, 1);

    if ((EndBonusNumBonuses != 0) || (playstate == ex_gameover))
    {
        SD_Play(PlayerSnds[locplayerstate->player]);

        // DO BONUS BONUS
        if (EndBonusNumBonuses == NUMBONUSES)
        {
            IN_StartAck();
            while(!IN_CheckAck())
                ;

            VL_DrawPicFullscreen(BkPic);

            gamestate.score += BONUSBONUS;
            DrawEOLHeader(playstate);
            EndBonusFirst = true;
            EndBonusStartY = 110;
            EndBonusSkip = true;
            DrawEndBonus("BONUS BONUS!  1,000,000 POINTS!", NULL, 2);
        }
        else if ((kr == 100) && dobonus)
        {
            IN_StartAck();
            while(!IN_CheckAck())
                ;

            VL_DrawPicFullscreen(BkPic);

            DrawEOLHeader(playstate);
            EndBonusFirst = true;
            EndBonusStartY = 110;
            DrawEndBonus("You have done well.", NULL, 3);

            if (shareware)
                EndBonusVoice = SD_Play(SD_RICOCHET3SND);
            else
                EndBonusVoice = SD_Play(SD_PERCENT100SND);

            EndBonusSkip = false;
            DrawEndBonus("This level is toast.", NULL, 3);

        }
    }

    VW_UpdateScreen();

    IN_StartAck();
    while(!IN_CheckAck())
        ;

    EndLevelStuff = false;
    CurrentFont = smallfont;
}


void DrawTallyHeader (int which)
{
    pic_t *Name;
    pic_t *KillCount;
    pic_t *TimesYouKilledPerson;
    pic_t *TimesPersonKilledYou;
    pic_t *Suicides;
    pic_t *Score;
    pic_t *Blank;
    pic_t *TopBar;

    Name                 = (pic_t *)W_CacheLumpName("t_name",   PU_CACHE, Cvt_pic_t, 1);
    Blank                = (pic_t *)W_CacheLumpName("t_blnk",   PU_CACHE, Cvt_pic_t, 1);
    KillCount            = (pic_t *)W_CacheLumpName("t_kcount", PU_CACHE, Cvt_pic_t, 1);
    TimesYouKilledPerson = (pic_t *)W_CacheLumpName("t_kilper", PU_CACHE, Cvt_pic_t, 1);
    TimesPersonKilledYou = (pic_t *)W_CacheLumpName("t_perkil", PU_CACHE, Cvt_pic_t, 1);
    Suicides             = (pic_t *)W_CacheLumpName("t_suicid", PU_CACHE, Cvt_pic_t, 1);
    Score                = (pic_t *)W_CacheLumpName("t_score",  PU_CACHE, Cvt_pic_t, 1);
    TopBar               = (pic_t *)W_CacheLumpName("t_bar",    PU_CACHE, Cvt_pic_t, 1);

    IFont = (cfont_t *)W_CacheLumpName("sifont", PU_CACHE, Cvt_cfont_t, 1);

    switch(which)
    {
        case 0:
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       8 * screen_src_minscale, TopBar);
            DrawIntensityString(screen_src_statusbar_x + (12 * screen_src_minscale),
                                11 * screen_src_minscale, "FINAL SCORE", 20);
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       24 * screen_src_minscale, Name);
            VL_DrawPic(screen_src_statusbar_x + (136 * screen_src_minscale),
                       24 * screen_src_minscale, KillCount);
            VL_DrawPic(screen_src_statusbar_x + (184 * screen_src_minscale),
                       24 * screen_src_minscale, Suicides);
            VL_DrawPic(screen_src_statusbar_x + (272 * screen_src_minscale),
                       24 * screen_src_minscale, Score);
            break;

        case 1:
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       8 * screen_src_minscale, TopBar);
            DrawIntensityString(screen_src_statusbar_x + (12 * screen_src_minscale),
                                11 * screen_src_minscale, "FINAL SCORE", 20);
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       24 * screen_src_minscale, Name);
            VL_DrawPic(screen_src_statusbar_x + (136 * screen_src_minscale),
                       24 * screen_src_minscale, Blank);
            VL_DrawPic(screen_src_statusbar_x + (272 * screen_src_minscale),
                       24 * screen_src_minscale, Score);
            break;

        case 2:
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       8 * screen_src_minscale, TopBar);
            DrawIntensityString(screen_src_statusbar_x + (12 * screen_src_minscale),
                                11 * screen_src_minscale, "YOUR KILLS", 20);
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       24 * screen_src_minscale, Name);
            VL_DrawPic(screen_src_statusbar_x + (136 * screen_src_minscale),
                       24 * screen_src_minscale, KillCount);
            VL_DrawPic(screen_src_statusbar_x + (184 * screen_src_minscale),
                       24 * screen_src_minscale, TimesYouKilledPerson);
            break;

        case 3:
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       8 * screen_src_minscale, TopBar);
            DrawIntensityString(screen_src_statusbar_x + (12 * screen_src_minscale),
                                11 * screen_src_minscale, "YOUR DEATHS", 20);
            VL_DrawPic(screen_src_statusbar_x + (8 * screen_src_minscale),
                       24 * screen_src_minscale, Name);
            VL_DrawPic(screen_src_statusbar_x + (136 * screen_src_minscale),
                       24 * screen_src_minscale, TimesPersonKilledYou);
            break;
    }

    DrawTimeXY(TALLYTIME_X, TALLYTIME_Y, gamestate.TimeCount / VBLCOUNTER, true);
}


#define BT_RANK_X    23
#define BT_PLAYER_X  30
#define BT_KILLS_X   ( 139 + ( ( 40 + 20 ) / 2 ) )
#define BT_DEATHS_X  ( 193 + ( ( 56 + 20 ) / 2 ) )
#define BT_SCORE_X   ( 273 + ( ( 46 + 20 ) / 2 ) )


void ShowKills (int localplayer)
{
    int w;
    int h;
    int i;
    int j;
    int temp;
    int rank;
    int player;
    int killer;
    int victim;
    int color;
    char tempstr[15];
    int KillCount[MAXPLAYERS];
    int Order[MAXPLAYERS];
    int NumPlayers;

    // show at the most 11 players
    NumPlayers = min(numplayers, 11);

    // Count kills
    for (killer = 0; killer < NumPlayers; killer++)
    {
        Order[killer] = killer;
        KillCount[killer] = 0;

        for (victim = 0; victim < NumPlayers; victim++)
            if (BATTLE_Team[victim] != BATTLE_Team[killer])
                KillCount[killer] += WhoKilledWho[killer][victim];
    }

    for (i = 0; i < NumPlayers - 1; i++)
        for (j = i + 1; j < NumPlayers; j++)
            if (KillCount[Order[i]] < KillCount[Order[j]])
            {
                temp = Order[i];
                Order[i] = Order[j];
                Order[j] = temp;
            }

    DrawTallyHeader(2);

    IFont = (cfont_t * )W_CacheLumpNum(W_GetNumForName("sifont"), PU_CACHE, Cvt_cfont_t, 1);
    CurrentFont = smallfont;
    py = 43 * screen_src_minscale;

    for(rank = 0; rank < NumPlayers; rank++)
    {
        player = Order[rank];

        color = 21;

        // Highlight the your score
        if (player == localplayer)
            color = 241;

        // Draw rank if not tied with previous rank
        if ((rank == 0) || (KillCount[player] != KillCount[Order[rank - 1]]))
            itoa(rank + 1, tempstr, 10);
        else
            strcpy(tempstr, "Tie");

        VW_MeasureIntensityPropString (tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_RANK_X - w) * screen_src_minscale),
                            py, tempstr, color);

        // Draw name
        DrawIntensityString(screen_src_statusbar_x + (BT_PLAYER_X * screen_src_minscale), py,
                            PLAYERSTATE[player].codename, color);

        // Draw kills
        itoa(KillCount[player], tempstr, 10);
        VW_MeasureIntensityPropString(tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_KILLS_X - w) * screen_src_minscale),
                            py, tempstr, color);

        // Draw times you killed that person
        if (player != localplayer)
            itoa(WhoKilledWho[localplayer][player], tempstr, 10);
        else
            strcpy(tempstr, "-");

        VW_MeasureIntensityPropString(tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_DEATHS_X - w) * screen_src_minscale),
                            py, tempstr, color);

        if (gamestate.teamplay)
            DrawIntensityString(screen_src_statusbar_x + ((BT_DEATHS_X + 16) * screen_src_minscale),
                                py, colorname[PLAYERSTATE[player].uniformcolor], color);

        py += h * screen_src_minscale;
    }
}


void ShowDeaths (int localplayer)
{
    int w;
    int h;
    int i;
    int j;
    int temp;
    int rank;
    int player;
    int killer;
    int victim;
    int color;
    char tempstr[15];
    int DeathCount[MAXPLAYERS];
    int Order[MAXPLAYERS];
    int NumPlayers;

    // show at the most 11 players
    NumPlayers = min(numplayers, 11);

    // Count Deaths
    for (victim = 0; victim < NumPlayers; victim++)
    {
        Order[victim] = victim;
        DeathCount[victim] = 0;

        for (killer = 0; killer < NumPlayers; killer++)
            DeathCount[victim] += WhoKilledWho[killer][victim];
    }

    for (i = 0; i < NumPlayers - 1; i++)
        for (j = i + 1; j < NumPlayers; j++)
            if (DeathCount[Order[i]] < DeathCount[Order[j]])
            {
                temp = Order[i];
                Order[i] = Order[j];
                Order[j] = temp;
            }


    DrawTallyHeader(3);

    IFont = (cfont_t * )W_CacheLumpNum(W_GetNumForName ("sifont"), PU_CACHE, Cvt_cfont_t, 1);
    CurrentFont = smallfont;
    py = 43 * screen_src_minscale;

    for (rank = 0; rank < NumPlayers; rank++)
    {
        player = Order[rank];
        color = 21;

        // Highlight the your score
        if (player == localplayer)
            color = 241;

        // Draw rank if not tied with previous rank
        if ((rank == 0) || (DeathCount[player] != DeathCount[Order[rank - 1]]))
            itoa(rank + 1, tempstr, 10);
        else
            strcpy(tempstr, "Tie");

        VW_MeasureIntensityPropString(tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_RANK_X - w) * screen_src_minscale),
                            py, tempstr, color);

        // Draw name
        DrawIntensityString(screen_src_statusbar_x + (BT_PLAYER_X * screen_src_minscale),
                            py, PLAYERSTATE[player].codename, color);

        // Draw deaths
        itoa(DeathCount[player], tempstr, 10);
        VW_MeasureIntensityPropString(tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_KILLS_X - w) * screen_src_minscale),
                            py, tempstr, color);

        // Draw times you were killed by that person
        itoa(WhoKilledWho[player][localplayer], tempstr, 10);
        VW_MeasureIntensityPropString(tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_DEATHS_X - w) * screen_src_minscale),
                            py, tempstr, color);

        if (gamestate.teamplay)
            DrawIntensityString(screen_src_statusbar_x + ((BT_DEATHS_X + 16) * screen_src_minscale),
                                py, colorname[ PLAYERSTATE[ player ].uniformcolor ], color);

        py += h * screen_src_minscale;
    }
}


void ShowEndScore (int localplayer)
{
    int w;
    int h;
    int rank;
    int leader;
    int team;
    int color;
    int killer;
    int victim;
    int killcount;
    int suicidecount;
    char tempstr[15];
    boolean dofullstats;
    int NumPlayers;

    // show at the most 11 players
    NumPlayers = min(numplayers, 11);

    dofullstats = false;
    switch (gamestate.battlemode)
    {
        case battle_Normal:
        case battle_ScoreMore:
        case battle_Hunter:
            dofullstats = true;
            DrawTallyHeader(0);
            break;

        case battle_Collector:
        case battle_Scavenger:
        case battle_Tag:
        case battle_Eluder:
        case battle_Deluder:
        case battle_CaptureTheTriad:
            DrawTallyHeader(1);
            break;
    }

    IFont = (cfont_t * )W_CacheLumpNum(W_GetNumForName("sifont"), PU_CACHE, Cvt_cfont_t, 1);
    CurrentFont = smallfont;
    py = 43 * screen_src_minscale;

    for (rank = 0; rank < BATTLE_NumberOfTeams; rank++)
    {
        team = BATTLE_PlayerOrder[rank];

        color = 21;
        if (team == BATTLE_Team[localplayer])
            color = 241;

        // Draw rank if not tied with previous rank
        if ((rank == 0) || (BATTLE_Points[team] != BATTLE_Points[BATTLE_PlayerOrder[rank - 1]]))
            itoa(rank + 1, tempstr, 10);
        else
            strcpy(tempstr, "Tie");

        VW_MeasureIntensityPropString(tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_RANK_X - w) * screen_src_minscale), py,
                            tempstr, color);

        // Draw name of team leader
        leader = BATTLE_TeamLeader[team];
        if (gamestate.teamplay)
            DrawIntensityString(screen_src_statusbar_x + (BT_PLAYER_X * screen_src_minscale),
                                py, colorname[PLAYERSTATE[leader].uniformcolor], color);
        else
            DrawIntensityString(screen_src_statusbar_x + (BT_PLAYER_X * screen_src_minscale),
                                py, PLAYERSTATE[leader].codename, color);

        if (dofullstats)
        {
            // Count how many kills each person on the team got
            killcount = 0;
            suicidecount = 0;
            for (killer = 0; killer < NumPlayers; killer++)
                if (BATTLE_Team[killer] == team)
                    for (victim = 0; victim < NumPlayers; victim++)
                        if (BATTLE_Team[victim] != team)
                            killcount += WhoKilledWho[killer][victim];
                        else
                            suicidecount += WhoKilledWho[killer][victim];

            // Draw kills
            itoa(killcount, tempstr, 10);
            VW_MeasureIntensityPropString ( tempstr, &w, &h);
            DrawIntensityString(screen_src_statusbar_x + ((BT_KILLS_X - w) * screen_src_minscale),
                                py, tempstr, color);

            // Draw suicides
            itoa(suicidecount, tempstr, 10);
            VW_MeasureIntensityPropString ( tempstr, &w, &h);
            DrawIntensityString(screen_src_statusbar_x + ((BT_DEATHS_X - w) * screen_src_minscale),
                                py, tempstr, color);
        }

        // Draw Score
        itoa(BATTLE_Points[team], tempstr, 10);
        VW_MeasureIntensityPropString (tempstr, &w, &h);
        DrawIntensityString(screen_src_statusbar_x + ((BT_SCORE_X - w) * screen_src_minscale),
                            py, tempstr, color);

        py += h * screen_src_minscale;
    }
}


void BattleLevelCompleted (int localplayer)
{
    ControlInfo ci;
    int w;
    int h;
    int key;
    int Screen;
    int LastScreen;
    int Player;
    char text[80];

    IN_ClearKeysDown();

    Player = localplayer;
    Screen = 1;
    LastScreen = 0;
    key = -1;

    while (1)
    {
        if (Screen != LastScreen)
        {
            VL_DrawPostPic (W_GetNumForName("trilogo"));

            switch(Screen)
            {
                case 1:
                    ShowEndScore(Player);
                    break;

                case 2:
                    ShowKills(Player);
                    break;

                case 3:
                    ShowDeaths(Player);
                    break;
            }

            CurrentFont = tinyfont;

            sprintf(text, "Page %d of 3.  Use arrows to switch stats.  "
                    "Press Esc to quit.", Screen);
            VW_MeasurePropString(text, &w, &h);

            py = screen_src_height - ((h + 2) * screen_src_minscale);
            px = (screen_src_width - (w * screen_src_minscale)) / 2;

            VW_DrawPropString(text);
            VW_UpdateScreen();

            do
            {
                ReadAnyControl (&ci);
            }
            while(ci.dir == (dirtype)key);
        }

        LastScreen = Screen;
        ReadAnyControl(&ci);
        key = ci.dir;

        if ((Screen > 1) && (key == dir_West))
        {
            Screen--;
            MN_PlayMenuSnd(SD_MOVECURSORSND);
        }
        else if ((Screen < 3) && (key == dir_East))
        {
            Screen++;
            MN_PlayMenuSnd(SD_MOVECURSORSND);
        }

        // Allow us to select which player to view
        if (Keyboard[sc_RShift] && (key == dir_South))
        {
            Player++;

            if (Player >= numplayers)
                Player = 0;

            LastScreen = 0;
            MN_PlayMenuSnd (SD_SELECTSND);
        }

        if (Keyboard[sc_RShift] && (key == dir_North))
        {
            Player--;

            if (Player < 0)
                Player = numplayers - 1;

            LastScreen = 0;
            MN_PlayMenuSnd (SD_SELECTSND);
        }

        if (Keyboard[sc_Escape])
            break;
    }

    while (Keyboard[sc_Escape])
    {
        IN_UpdateKeyboard ();
    }

    MN_PlayMenuSnd (SD_ESCPRESSEDSND);

    CurrentFont = smallfont;
}

//==========================================================================

#define STARTRADIUS (0xa000)
#define STOPRADIUS (0x14000)
#define DEATHRADIUS (STOPRADIUS-STARTRADIUS)
#define ROTRATE      (5)
#define TOTALDEATHROT (FINEANGLES<<1)
#define RADIUSINC   ((DEATHRADIUS)/(TOTALDEATHROT))

/*
==================
=
= ZoomDeathOkay
=
==================
*/
boolean ZoomDeathOkay (void)
{
    int x, y;
    int radius;

    if (!((player->state == &s_ashwait) || ((player->flags & FL_HBM) && (gamestate.violence >= vl_high))))
        return false;

    radius = STOPRADIUS;
    x = player->x;
    y = player->y;

    while (radius > 0)
    {
        if (tilemap[x >> 16][(y + radius) >> 16])
            return false;
        if (tilemap[x >> 16][(y - radius) >> 16])
            return false;
        if (tilemap[(x - radius) >> 16][y >> 16])
            return false;
        if (tilemap[(x + radius) >> 16][y >> 16])
            return false;
        if (tilemap[(x + radius) >> 16][(y + radius) >> 16])
            return false;
        if (tilemap[(x + radius) >> 16][(y - radius) >> 16])
            return false;
        if (tilemap[(x - radius) >> 16][(y + radius) >> 16])
            return false;
        if (tilemap[(x - radius) >> 16][(y - radius) >> 16])
            return false;

        radius-=0x10000;
    }

    return true;
}

/*
==================
=
= Died
=
==================
*/

#define DEATHROTATE 6

extern boolean dopefish;
void Died (void)
{
    long dx, dy;
    int iangle, curangle, clockwise, change;
    int da;
    int rate;
    lbm_t *LBM;
    int slowrate;
    playertype *pstate;
    objtype *killerobj = (objtype *)player->target;

    //player->yzangle=0; //WHY?

    if (killerobj == NULL)
        killerobj = player;

    if (CheckParm("slowdeath"))
        slowrate = 3;
    else
        slowrate = 0;

    M_LINKSTATE(player, pstate);

    if (ZoomDeathOkay() && (!pstate->falling))
    {
        int x, y, z, radius, heightoffset;
        int endangle, startangle, killangle;
        boolean deadflagset;
        objtype *dummy;

        x = player->x;
        y = player->y;
        z = player->z;
        dummy = player;
        SpawnPlayerobj(x >> 16, y >> 16, 0, 0);
        player = dummy;
        dummy = new;
        dummy->x = x;
        dummy->drawx = x;
        dummy->y = y;
        dummy->z = z;
        dummy->drawy = y;
        dummy->flags = player->flags;
        player->momentumx = 0;
        player->momentumy = 0;
        player->speed = 0;

        radius = STARTRADIUS;
        heightoffset = pstate->heightoffset;
        deadflagset = false;
        startangle = (player->angle + ANG180) & (FINEANGLES - 1);
        endangle = startangle + TOTALDEATHROT;
        killangle = startangle + (TOTALDEATHROT >> 1);

        if (dopefish)
            AddMessage("Dopefish Death Cam", MSG_SYSTEM);

        for (iangle=startangle;;)
        {
            if (iangle > killangle)
            {
                if (!deadflagset)
                {
                    dummy->hitpoints = 0;
                    pstate->health = 0;
                    dummy->flags &= ~FL_DYING;
                    dummy->flags |= FL_SHOOTABLE;

                    if (player->state == &s_ashwait)
                        dummy->flags |= FL_SKELETON;

                    Collision(dummy, (objtype *)NULL, 0, 0);
                    deadflagset = true;

                    if ((killerobj == player) && (gamestate.violence >= vl_high) &&
                            (gamestate.difficulty >= gd_hard))
                        SD_Play(SD_YOUSUCKSND);
                    else
                        SD_Play(SD_PLAYERTCDEATHSND + (pstate->player));
                }
            }
            else
                dummy->flags &= ~FL_DYING;

            if (dopefish)
            {
                dummy->momentumx += (RandomNumber("Died", 0) << 6) - (256 << 5);
                dummy->momentumy += (RandomNumber("Died", 0) << 6) - (256 << 5);
            }

            player->x = x + FixedMul(radius, costable[iangle & (FINEANGLES - 1)]);
            player->y = y - FixedMul(radius, sintable[iangle & (FINEANGLES - 1)]);
            player->z = dummy->z;
            player->angle = (iangle + ANG180) & (FINEANGLES - 1);

            if (dopefish)
            {
                int dx, dy;

                dx = dummy->x - player->x;
                dy = player->y - dummy->y;

                if (dx && dy)
                    player->angle = atan2_appx(dx, dy);
            }

            pstate->heightoffset = heightoffset;
            player->yzangle = 0;
            UpdateGameObjects();
            player->momentumx = 0;
            player->momentumy = 0;
            player->speed = 0;
            ThreeDRefresh();
            AnimateWalls();
            DoSprites();
            DoAnimatedMaskedWalls();
            UpdatePlayers();
            UpdateLightLevel(player->areanumber);

            if (iangle<endangle)
            {
                iangle += (tics << ROTRATE);
                radius += tics * (RADIUSINC << ROTRATE);
            }

            if ((dummy->state == dummy->state->next) && (iangle >= endangle))
                break;
        }
    }
    else if (!pstate->falling)
    {
        //
        // swing around to face attacker
        //

        rate = DEATHROTATE - slowrate;

        if (killerobj == player)
        {
            iangle = player->angle;

            if ((gamestate.violence >= vl_high) && (gamestate.difficulty >= gd_hard))
                SD_Play(SD_YOUSUCKSND);
        }
        else
        {
            SD_Play (SD_PLAYERTCDEATHSND + (pstate->player));

            if (killerobj->which == PWALL)
            {
                dx = ((pwallobj_t *)killerobj)->x - player->x;
                dy = player->y - ((pwallobj_t *)killerobj)->y;
            }
            else
            {
                dx = killerobj->x - player->x;
                dy = player->y - killerobj->y;
            }

            iangle = atan2_appx (dx,dy);       // returns -pi to pi
        }

        da = iangle - player->angle;

        if (da > 0)
            clockwise = 1;
        else
            clockwise = 0;

        da = abs(da);

        if (da > ANG180)
        {
            clockwise ^= 1;
            da = ANGLES - da;
        }

        curangle = player->angle;

        do
        {
            DoBorderShifts();
            change = tics << rate;

            if (clockwise == 1)
                curangle += change;
            else
                curangle -= change;

            da -= change;

            if (curangle >= ANGLES)
                curangle -= ANGLES;
            if (curangle < 0)
                curangle += ANGLES;

            player->angle = (curangle & (FINEANGLES - 1));

            ThreeDRefresh();
            CalcTics();
        }
        while (da > 0);
    }
    else
    {
        DrawSky(true);
        FlipPage();
    }

    while (damagecount)
        DoBorderShifts();

    DoBorderShifts ();

    locplayerstate->weapon = -1;        // take away weapon

    if (!timelimitenabled)
        locplayerstate->lives--;

    if (!pstate->falling)
        ThreeDRefresh ();

    FlipPage();
    FlipPage();

    if (locplayerstate->lives > -1)
    {
        int rng;

        rng = RandomNumber("Died", 0);

        if (pstate->falling)
        {
            RotateBuffer (0, 0, FINEANGLES, FINEANGLES >> 6, VBLCOUNTER * (1 + slowrate));
            SD_Play(SD_PLAYERTCDEATHSND + pstate->player);
            pstate->falling = false;
        }
        else if (rng < 64)
            RotateBuffer (0, 0, FINEANGLES, FINEANGLES * 64, VBLCOUNTER * (2 + slowrate));
        else if (rng < 128)
            RotateBuffer (0, 0, FINEANGLES, FINEANGLES >> 6, VBLCOUNTER * (1 + slowrate));
        else if (rng < 192)
            RotateBuffer(0, FINEANGLES * 4, FINEANGLES, FINEANGLES * 64, VBLCOUNTER * (3 + slowrate));
        else
            VL_FadeToColor (VBLCOUNTER * 2, 100, 0, 0);

        screenfaded = false;

        VL_FadeOut(0, 255, 0, 0, 0, VBLCOUNTER >> 1);
        screen_zoom = false;

        gamestate.episode = 1;
        player->flags &= ~FL_DONE;

        InitializeWeapons(locplayerstate);
        ResetPlayerstate(locplayerstate);

        UpdateLives(locplayerstate->lives);
        UpdateScore(gamestate.score);

        DrawTriads(true);
        DrawLives(true);
        DrawKeys(true);
        DrawScore(true);
    }
    else
    {
        int rng;

        SD_Play(SD_GAMEOVERSND);
        rng = RandomNumber("Died", 0);

        if (rng < 64)
            RotateBuffer(0, FINEANGLES >> 1, FINEANGLES, FINEANGLES * 64, VBLCOUNTER * (3 + slowrate));
        else if (rng < 128)
            VL_FadeToColor(VBLCOUNTER * 3, 255, 255, 255);
        else if (rng < 192)
            RotateBuffer(0, FINEANGLES * 2, FINEANGLES, FINEANGLES * 64, VBLCOUNTER * (3 + slowrate));
        else
            RotateBuffer(0, FINEANGLES * 2, FINEANGLES, FINEANGLES * 64, VBLCOUNTER * (3 + slowrate));

        screenfaded = false;

        VL_FadeOut(0, 255, 0, 0, 0, VBLCOUNTER >> 1);
        screen_zoom = false;

        MU_StartSong(song_gameover);

        if (gamestate.violence == vl_excessive || shareware)
            LBM = (lbm_t *)W_CacheLumpNum(W_GetNumForName("bootblod"), PU_CACHE, Cvt_lbm_t, 1);
        else
            LBM = (lbm_t *)W_CacheLumpNum(W_GetNumForName("bootnorm"), PU_CACHE, Cvt_lbm_t, 1);

        VL_DecompressLBM(LBM, true, 0, 0, 0);

        StopWind();

        IN_UserInput(VBLCOUNTER * 60);

        MainMenu[savegame].active = 0;
        MainMenu[viewscores].routine = (void *)CP_ViewScores;
        MainMenu[viewscores].texture[6] = '7';
        MainMenu[viewscores].texture[7] = '\0';
        MainMenu[viewscores].letter = 'V';
    }

    ClearGraphicsScreen();

    VL_FadeIn (0, 255, origpal, 15);
}


//******************************************************************************
//
// DoLoadGameAction ()
//
//******************************************************************************

static byte whichstr = 0;

void DoLoadGameAction (void)
{
    if ((SaveTime+1) < GetTicCount())
    {
        byte *temp = bufferofs;

        bufferofs = displayofs;
        SaveTime = GetTicCount();

        CurrentFont=tinyfont;

        px = 92;
        py = 152;
        if (whichstr)
        {
//         VW_DrawPropString ("ƒ");
            //VW_DrawPropString (".");
            whichstr = 0;
        }
        else
        {
//         VW_DrawPropString ("„");
            //VW_DrawPropString (".");
            whichstr = 1;
        }
        bufferofs = temp;
    }
}

//******************************************************************************
//
// DoCheckSum ()
//
//******************************************************************************

long DoCheckSum (byte *source, int size, long csum)
{
    int i;
    long checksum;

    checksum = csum;

    for (i = 0; i < size; i++)
        checksum=updatecrc(checksum,*(source+i));

    return checksum;
}

//******************************************************************************
//
// CaculateSaveGameCheckSum ()
//
//******************************************************************************

#define SAVECHECKSUMSIZE (10000)
long CalculateSaveGameCheckSum (char *filename)
{
    int handle;
    int lengthleft;
    int length;
    byte *altbuffer;
    long checksum;

    altbuffer=SafeMalloc(SAVECHECKSUMSIZE);

    checksum = 0;

    // Open the savegame file

    handle = SafeOpenRead (filename);
    lengthleft = filelength (handle);

    while (lengthleft>0)
    {
        length=SAVECHECKSUMSIZE;
        if (length>lengthleft)
            length=lengthleft;

        SafeRead(handle,altbuffer,length);
        checksum = DoCheckSum (altbuffer, length, checksum);

        lengthleft-=length;
    }

    SafeFree(altbuffer);

    close (handle);

    return checksum;
}

//******************************************************************************
//
// StoreBuffer
//
//******************************************************************************
void StoreBuffer (int handle, byte *src, int size)
{
    SafeWrite(handle,&size,sizeof(size));
    SafeWrite(handle,src,size);
}

//******************************************************************************
//
// SaveTag
//
//******************************************************************************
void SaveTag (int handle, char *tag, int size)
{
    SafeWrite(handle,tag,size);
}


//******************************************************************************
//
// SaveTheGame ()
//
// Expects game to be premalloced
//
//******************************************************************************

boolean SaveTheGame (int num, gamestorage_t *game)
{
    char    numHex[2] = "0";
    char   filename[MAX_PATH];
    byte    *altbuffer;
    int    size;
    int    avail;
    int    savehandle;
    int    crc;
    int    i;
    char   letter;
    int myticcount;

    if (num > 15 || num < 0)
        Error("Illegal Saved game value=%d\n",num);

    //
    // Save Alternate Game Level information for reloading game
    //
    memset (&game->info, 0, sizeof (game->info));
    if (GameLevels.avail)
    {
        memcpy (&game->info.path[0], &GameLevels.path[0], sizeof (GameLevels.path));
        memcpy (&game->info.file[0], &GameLevels.file[0], sizeof (GameLevels.file));
        game->info.avail = true;
    }

    game->mapcrc=GetMapCRC (gamestate.mapon);

    sprintf(numHex, "%1x", num);

    if (pure)
    {
        // Create the proper file name
        SaveFileName[7] = toupper(numHex[0]);

        GetPathFromEnvironment( filename, ApogeePath, SaveFileName );
    }
    else
    {
        // Create the proper file name
        NewSaveFileName[7] = numHex[0];

        GetPathFromEnvironment( filename, ApogeePath, NewSaveFileName );
    }

    // Open the savegame file

    savehandle = SafeOpenWrite (filename);

    // Save out file tag

    size=4;
    SaveTag(savehandle,"ROTT",size);

    // Save out header

    size=sizeof(*game);

    SafeWrite(savehandle,game,size);

/////////////////////////////////////////////////////////////////////////////
// Save out rest of save game file beyond this point
/////////////////////////////////////////////////////////////////////////////

    // Door Tag

    size=4;
    SaveTag(savehandle,"DOOR",size);

    // Doors

    SaveDoors(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // Elevator Tag

    size = 9;
    SaveTag(savehandle,"ELEVATORS",size);

    // Elevators

    SaveElevators(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // Pushwall Tag

    size=5;
    SaveTag(savehandle,"PWALL",size);

    // PushWalls

    SavePushWalls(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // MaskedWalls Tag

    size=5;
    SaveTag(savehandle,"MWALL",size);

    // Masked Walls

    SaveMaskedWalls(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // Switches Tag

    size=6;
    SaveTag(savehandle,"SWITCH",size);

    // Switches

    SaveSwitches(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // Statics Tag

    size=6;
    SaveTag(savehandle,"STATIC",size);

    // Statics

    SaveStatics(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // Actors Tag

    size=5;
    SaveTag(savehandle,"ACTOR",size);

    // Actors

    SaveActors(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // TouchPlates Tag

    size=5;
    SaveTag(savehandle,"TOUCH",size);

    // TouchPlates

    SaveTouchPlates(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // GameState Tag

    size=9;
    SaveTag(savehandle,"GAMESTATE",size);

    // GameState

    size=sizeof(gamestate);
    SafeWrite(savehandle,&gamestate,size);

    // PlayerState Tag

    size=12;
    SaveTag(savehandle,"PLAYERSTATES",size);

    // PlayerStates
    size=sizeof(playertype);
    for(i=0; i<numplayers; i++)
    {
        SafeWrite(savehandle,&PLAYERSTATE[i],size);
    }

    // Mapseen Tag

    size=7;
    SaveTag(savehandle,"MAPSEEN",size);

    // MapSeen

    size=sizeof(mapseen);
    SafeWrite(savehandle,&mapseen,size);

    // Song Tag

    size=4;
    SaveTag(savehandle,"SONG",size);

    // Song info

    MU_SaveMusic(&altbuffer,&size);
    StoreBuffer(savehandle,altbuffer,size);
    SafeFree(altbuffer);

    // Misc Tag

    size=4;
    SaveTag(savehandle,"MISC",size);

    // Misc

    // ticcount
    myticcount = GetTicCount();
    size=sizeof(myticcount);
    SafeWrite(savehandle,&myticcount,size);

    // shaketics
    size=sizeof(SHAKETICS);
    SafeWrite(savehandle,&SHAKETICS,size);

    // damagecount
    size=sizeof(damagecount);
    SafeWrite(savehandle,&damagecount,size);

    // viewsize
    size=sizeof(viewsize);
    SafeWrite(savehandle,&viewsize,size);

    // poweruptimes
    size = sizeof (poweruptime);
    SafeWrite(savehandle,&poweruptime,size);

    size = sizeof (protectiontime);
    SafeWrite(savehandle,&protectiontime,size);

    size = sizeof (powerupheight);
    SafeWrite(savehandle,&powerupheight,size);

    size = sizeof (protectionheight);
    SafeWrite(savehandle,&protectionheight,size);

    size = sizeof (poweradjust);
    SafeWrite(savehandle,&poweradjust,size);

    close (savehandle);

    // Calculate CRC

    crc = CalculateSaveGameCheckSum (filename);

    // Append the crc

    savehandle = SafeOpenAppend (filename);

    size=sizeof(crc);
    SafeWrite(savehandle,&crc,size);

    close (savehandle);

    pickquick = true;
    return (true);
}


//******************************************************************************
//
// LoadTag
//
//******************************************************************************

void LoadTag (byte **src, char *tag, int size)
{
    if (StringsNotEqual((char *)*src,(char *)tag,size)==true)
        Error("Could not locate %s header in saved game file\n",tag);
    *src+=size;
}

//******************************************************************************
//
// LoadBuffer
//
//******************************************************************************
int LoadBuffer (byte **dest, byte **src)
{
    int size;

    memcpy(&size,*src,sizeof(size));
    *src+=sizeof(size);
    *dest=SafeMalloc(size);
    memcpy(*dest,*src,size);
    *src+=size;
    return size;
}


//******************************************************************************
//
// LoadTheGame ()
//
// Expects game to be premalloced
//
//******************************************************************************

boolean LoadTheGame (int num, gamestorage_t *game, int rtl)
{
    char    numHex[2] = "0";
    char   filename[128];
    byte    *loadbuffer;
    byte    *bufptr;
    byte    *altbuffer;
    int    size;
    int    totalsize;
    int    checksum;
    int    savedchecksum;
    int    i;
    word   mapcrc;
    int myticcount;
    int temprtl;

    if (num>15 || num<0)
        Error("Illegal Load game value=%d\n",num);

    sprintf(numHex, "%1x", num);

    if (pure)
    {
        // Create the proper file name
        SaveFileName[7] = toupper(numHex[0]);

        GetPathFromEnvironment( filename, ApogeePath, SaveFileName );
    }
    else
    {
        // Create the proper file name
        NewSaveFileName[7] = numHex[0];

        GetPathFromEnvironment( filename, ApogeePath, NewSaveFileName );
    }

    // Load the file

    totalsize=LoadFile(filename,(void **)&loadbuffer);
    bufptr=loadbuffer;

    // Calculate checksum

    checksum = DoCheckSum (loadbuffer, totalsize-sizeof(checksum), 0);

    // Retrieve saved checksum

    memcpy (&savedchecksum,loadbuffer+(totalsize-sizeof(savedchecksum)),sizeof(savedchecksum));

    // Compare the two checksums;

    if (checksum!=savedchecksum)
    {
        if (CP_DisplayMsg ("Your Saved Game file is\n"
                           "shall we say, \"corrupted\".\n"
                           "Would you like to\n"
                           "continue anyway (Y/N)?\n", 12)==false)
        {
            return false;
        }
    }

    temprtl = currentrtl;
    if (!pure && enabledrtls != 0)
        currentrtl = rtl;


    // Load in file tag

    size=4;
    LoadTag(&bufptr,"ROTT",size);

    // Load in header

    size=sizeof(*game);
    memcpy(game,bufptr,size);
    bufptr+=size;

    if (game->version!=ROTTVERSION)
    {
        currentrtl = temprtl;
        return false;
    }

    memcpy (&GameLevels, &game->info, sizeof (GameLevels));

    gamestate.episode=game->episode;
    gamestate.mapon=game->area;

    mapcrc=GetMapCRC (gamestate.mapon);

    if (mapcrc!=game->mapcrc)
    {
        currentrtl = temprtl;
        return false;
    }

/////////////////////////////////////////////////////////////////////////////
// Load in rest of saved game file beyond this point
/////////////////////////////////////////////////////////////////////////////

    // Free up the current level
    Z_FreeTags (PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

    gamestate.battlemode = battle_StandAloneGame;
    BATTLE_SetOptions( &BATTLE_Options[ battle_StandAloneGame ] );
    BATTLE_Init( gamestate.battlemode, 1 );

    DoLoadGameAction ();
    SetupGameLevel();

    // This prevents a nasty glitch when loading some saved games
    PreCacheGroup(W_GetNumForName("BULLETHO"),W_GetNumForName("ALTBHO"),cache_transpatch_t);

    // Door Tag

    size=4;
    LoadTag(&bufptr,"DOOR",size);

    // Doors

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadDoors(altbuffer,size);
    SafeFree(altbuffer);

    // Elevator Tag

    size = 9;
    LoadTag(&bufptr,"ELEVATORS",size);


    // Elevators

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadElevators(altbuffer,size);
    SafeFree(altbuffer);

    // Pushwall Tag

    size=5;
    LoadTag(&bufptr,"PWALL",size);

    // PushWalls

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadPushWalls(altbuffer,size);
    SafeFree(altbuffer);
#if 0
    // Animated Walls Tag

    size=5;
    LoadTag(&bufptr,"AWALL",size);

    // Animated Walls
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadAnimWalls(altbuffer,size);
    SafeFree(altbuffer);
#endif

    // MaskedWalls Tag

    size=5;
    LoadTag(&bufptr,"MWALL",size);

    // Masked Walls

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadMaskedWalls(altbuffer,size);
    SafeFree(altbuffer);

    // Switches Tag

    size=6;
    LoadTag(&bufptr,"SWITCH",size);

    // Switches

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadSwitches(altbuffer,size);
    SafeFree(altbuffer);


    // Statics Tag

    size=6;
    LoadTag(&bufptr,"STATIC",size);

    // Statics

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadStatics(altbuffer,size);
    SafeFree(altbuffer);

    // Actors Tag

    size=5;
    LoadTag(&bufptr,"ACTOR",size);

    // Actors

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadActors(altbuffer,size);
    SafeFree(altbuffer);

    // TouchPlates Tag

    size=5;
    LoadTag(&bufptr,"TOUCH",size);

    // TouchPlates

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    LoadTouchPlates(altbuffer,size);
    SafeFree(altbuffer);

    // SetupWindows

    SetupWindows();

    // GameState Tag

    size=9;
    LoadTag(&bufptr,"GAMESTATE",size);

    // GameState

    DoLoadGameAction ();
    size=sizeof(gamestate);
    memcpy(&gamestate,bufptr,size);
    bufptr+=size;

    // PlayerState Tag

    size=12;
    LoadTag(&bufptr,"PLAYERSTATES",size);

    // PlayerState

    DoLoadGameAction ();
    size=sizeof(playertype);
    for(i=0; i<numplayers; i++)
    {
        memcpy(&PLAYERSTATE[i],bufptr,size);
        bufptr+=size;
    }

    // Zero out player targets

    locplayerstate->guntarget=0;
    locplayerstate->targettime=0;

    // Mapseen Tag

    size=7;
    LoadTag(&bufptr,"MAPSEEN",size);

    // MapSeen

    DoLoadGameAction ();
    size=sizeof(mapseen);
    memcpy(&mapseen,bufptr,size);
    bufptr+=size;

    // Song Tag

    size=4;
    LoadTag(&bufptr,"SONG",size);

    // Song info

    DoLoadGameAction ();
    size=LoadBuffer(&altbuffer,&bufptr);
    MU_LoadMusic(altbuffer,size);
    SafeFree(altbuffer);

    // Misc Tag

    size=4;
    LoadTag(&bufptr,"MISC",size);

    // Misc

    // ticcount
    DoLoadGameAction ();
    size=sizeof(myticcount);
    memcpy((void *)&myticcount,bufptr,size);
    bufptr+=size;
    SaveTime = myticcount;
    ISR_SetTime(myticcount);

    // shaketics
    DoLoadGameAction ();
    size=sizeof(SHAKETICS);
    memcpy(&SHAKETICS,bufptr,size);
    bufptr+=size;

    // damagecount
    DoLoadGameAction ();
    size=sizeof(damagecount);
    memcpy(&damagecount,bufptr,size);
    bufptr+=size;

    // viewsize
    DoLoadGameAction ();
    size=sizeof(viewsize);
    memcpy(&viewsize,bufptr,size);
    bufptr+=size;

    // powerup times
    DoLoadGameAction ();
    size = sizeof (poweruptime);
    memcpy (&poweruptime, bufptr, size);
    bufptr += size;
    size = sizeof (protectiontime);
    memcpy (&protectiontime, bufptr, size);
    bufptr += size;
    size = sizeof (powerupheight);
    memcpy (&powerupheight, bufptr, size);
    bufptr += size;
    size = sizeof (protectionheight);
    memcpy (&protectionheight, bufptr, size);
    bufptr += size;
    size = sizeof (poweradjust);
    memcpy (&poweradjust, bufptr, size);
    bufptr += size;

    // Set the viewsize

    SetViewSize(viewsize);
    DoLoadGameAction ();

    // Connect areas

    ConnectAreas ();
    DoLoadGameAction ();

    // Free up the loadbuffer

    SafeFree (loadbuffer);
    DoLoadGameAction ();

    Illuminate();

    IN_UpdateKeyboard ();
    LoadPlayer ();
    DoLoadGameAction ();
    SetupPlayScreen();
    UpdateScore (gamestate.score);
    UpdateLives (locplayerstate->lives);
    UpdateTriads (player, 0);
    PreCache ();
    InitializeMessages();

    for (i=0; i<100; i++)
        UpdateLightLevel(player->areanumber);

    CalcTics();
    CalcTics();

    pickquick = true;

    if (temprtl != currentrtl)
        quicksaveslot = -1;

    return (true);
}


//******************************************************************************
//
// GetSavedMessage ()
//
// Expects message to be premalloced
//
//******************************************************************************

void GetSavedMessage (int num, char *message)
{
    char    numHex[2] = "0";
    gamestorage_t game;
    char   filename[128];
    byte    *loadbuffer;
    byte    *bufptr;
    int    size;

    if (num>15 || num<0)
        Error("Illegal Load game value=%d\n",num);

    sprintf(numHex, "%1x", num);

    if (pure)
    {
        // Create the proper file name
        SaveFileName[7] = toupper(numHex[0]);

        GetPathFromEnvironment( filename, ApogeePath, SaveFileName );
    }
    else
    {
        // Create the proper file name
        NewSaveFileName[7] = numHex[0];

        GetPathFromEnvironment( filename, ApogeePath, NewSaveFileName );
    }

    // Load the file

    size=LoadFile(filename,(void **)&loadbuffer);
    bufptr=loadbuffer;

    size=4;
    LoadTag(&bufptr,"ROTT",size);

    // Load in header

    size=sizeof(game);
    memcpy(&game,bufptr,size);
    strcpy(message,game.message);
    SafeFree(loadbuffer);
}

//******************************************************************************
//
// GetSavedHeader ()
//
// Expects game to be premalloced
//
//******************************************************************************

void GetSavedHeader (int num, gamestorage_t *game)
{
    char    numHex[2] = "0";
    char   filename[128];
    byte    *loadbuffer;
    byte    *bufptr;
    int    size;

    if (num>15 || num<0)
        Error("Illegal Load game value=%d\n",num);

    sprintf(numHex, "%1x", num);

    if (pure)
    {
        // Create the proper file name
        SaveFileName[7] = toupper(numHex[0]);

        GetPathFromEnvironment( filename, ApogeePath, SaveFileName );
    }
    else
    {
        // Create the proper file name
        NewSaveFileName[7] = numHex[0];

        GetPathFromEnvironment( filename, ApogeePath, NewSaveFileName );
    }

    // Load the file

    size=LoadFile(filename, (void **)&loadbuffer);
    bufptr=loadbuffer;

    size=4;
    LoadTag(&bufptr,"ROTT",size);

    // Load in header

    size=sizeof(*game);
    memcpy(game,bufptr,size);
    SafeFree(loadbuffer);
}



//******************************************************************************
//
// GetLevel ()
//
//******************************************************************************

int GetLevel (int episode, int mapon)
{
    int level;

    level = (mapon+1) - ((episode-1) << 3);

    return (level);
}
