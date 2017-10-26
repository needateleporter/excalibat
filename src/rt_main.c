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
#include "lumpy.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>

#include <signal.h>

/* TODO: Need to redefine main to SDL_main on some platforms... */
#include "SDL2/SDL.h"

#include "rt_actor.h"
#include "rt_stat.h"
#include "rt_vid.h"
#include "rt_menu.h"
#include "rt_sound.h"
#include "watcom.h"
#include "scriplib.h"
#include "rt_main.h"
#include "_rt_main.h"
#include "rt_com.h"
#include "rt_util.h"
#include "z_zone.h"
#include "w_wad.h"
#include "rt_game.h"
#include "rt_floor.h"
#include "rt_playr.h"
#include "rt_draw.h"
#include "rt_str.h"
#include "rt_view.h"
#include "rt_door.h"
#include "rt_ted.h"
#include "rt_in.h"
#include "rt_map.h"
#include "rt_rand.h"
#include "rt_debug.h"
#include "isr.h"
#include "rt_cfg.h"
#include "develop.h"
#include "version.h"
#include "rt_menu.h"
#include "rt_dr_a.h"
#include "rt_msg.h"
#include "rt_build.h"
#include "modexlib.h"
#include "rt_net.h"
#include "cin_main.h"
#include "rottnet.h"
#include "rt_scale.h"
#include "rt_crc.h"

#include "au_music.h"
#include "au_fxman.h"

volatile int    oldtime;
volatile int    gametime;

boolean         warp;
int             warpx=0;
int             warpy=0;
int             warpa=0;
int             NoSound;
int             polltime;
int             oldpolltime;
boolean         fizzlein = false;
int             pheight;

boolean SCREENSHOTS             = false;
boolean MAPSTATS                = false;
boolean TILESTATS               = false;
boolean HUD                     = false;

boolean dopefish;

boolean newlevel = false;
boolean infopause;
boolean quiet = false;

boolean pure = false;
boolean shareware = false;
boolean shareware_deluxe = false;
boolean flappy = false;
boolean devkeys = false;
boolean dfishpatch = false;

int enabledrtls = 0; // 0 - Disabled. Otherwise, it's bitwise (STANDARD, SHAREWARE, EXTREME)
int currentrtl = 0;
int demortl = -1;

#if (DEVELOPMENT == 1)
boolean DebugOk = true;
#else
boolean DebugOk = false;
#endif

static char savename[13] = "ROTT0000.LBM";
static int totalbytes;
static byte *bptr;

static boolean turbo;

static int NoWait;
static int startlevel=0;
static int demonumber=-1;

char CWD[40];                          // curent working directory
static boolean quitactive = false;

int timelimit;
int maxtimelimit;
boolean timelimitenabled;
boolean demoexit;
boolean noecho;

void CheckCommandLineParameters( void );
void CheckCommandLineParametersPostCfg (void);
void CheckGameVersion (void);
void PlayTurboGame( void );
void Init_Tables (void);
void CheckRemoteRidicule ( int scancode );
void SetRottScreenRes (int Width, int Height);

extern void crash_print (int);
extern int setup_homedir (void);

extern int iDropDemo;
extern boolean screen_fullscreen;

extern void ComSetTime ( void );
extern void VH_UpdateScreen (void);
extern void RottConsole ( void );
extern void ReadDelay(long delay);
extern void RecordDemoQuery ( void );


int main (int argc, char *argv[])
{
    _argc = argc;
    _argv = argv;

#if defined(PLATFORM_MACOSX)
    {
        /* OS X will give us a path in the form '/Applications/Rise of the Triad.app/Contents/MacOS/Rise of the Triad'.
           Our data is in Contents/Resources. */
        char *path;
        const char suffix[] = "/Resources/";
        int end;
        path = (char *)malloc(strlen(argv[0]) + strlen(suffix) + 1);
        if (path == NULL) return 1;
        strcpy(path, argv[0]);
        /* Back up two '/'s. */
        for (end = strlen(path)-1; end >= 0 && path[end] != '/'; end--);
        if (end >= 0) for (--end; end >= 0 && path[end] != '/'; end--);
        strcpy(&path[end], suffix);
        printf("Changing to working directory: %s\n", path);
        chdir(path);
        free(path);
    }
#endif

    signal (11, crash_print);

    if (setup_homedir() == -1)
        return 1;

    // Set which release version we're on
    gamestate.Version = ROTTVERSION;

    CheckGameVersion();

    // Silly thing Joe did a while back to replace
    // collector triads with 3D rendered dopefish.
    // http://www.dopefish.com/stuff/dfrottcd.zip
    if (shareware && CheckParm("DFISHPATCH"))
    {
        dfishpatch = true;
        ApplyDfishPatch();
    }

    DrawRottTitle();

    gamestate.randomseed = -1;
    gamestate.autorun = 0;

    StartupSoftError();

    CheckCommandLineParameters();

    // Start up Memory manager with a certain amount of reserved memory
    // TODO: How much? Scaled graphics?
    Z_Init(50000,1000000);

    IN_Startup();

    InitializeGameCommands();

    if (!standalone)
    {
        if (pure)
        {
            ReadConfig();
            ReadSETUPFiles();
        }
        else
            ReadNewConfig();

        doublestep = 0;
        CheckCommandLineParametersPostCfg();
        SetupWads();

        BuildTables();
        GetMenuInfo();
    }

    if (!standalone)
    {
        int status1 = 0;
        int status2 = 0;
        int status3 = 0;

        if (!NoSound)
        {
            if (!quiet)
                printf("MU_Startup: ");

            status1 = MU_Startup(false);

            if (!quiet)
                printf("%s\n", MUSIC_ErrorString(status1));

            if (!quiet)
                printf("SD_SetupFXCard: ");

            status2 = SD_SetupFXCard();

            if (!quiet)
                printf("%s\n", FX_ErrorString(status2));

            if (!status2)
            {
                if (!quiet)
                    printf("SD_Startup: ");

                status3 = SD_Startup(false);

                if (!quiet)
                    printf("%s\n", FX_ErrorString(status3));
            }
        }
        else if (!quiet)
            printf("Sound FX disabled.\n");

        if (status1 || status2 || status3)
        {
            printf("\n\nROTT was unable to initialize your ");

            if (status1)
            {
                printf("music ");
                MusicMode = 0;
            }

            if (status2 || status3)
            {
                if (status1)
                    printf( "or " );

                printf( "sound fx " );
                FXMode = 0;
            }

            printf("hardware.\n");
        }

        Init_Tables();
        InitializeRNG();
        InitializeMessages();
        LoadColorMap();
    }

    if (infopause)
    {
        printf("\n< Press any key to continue >\n");
        getch();
    }

    I_StartupTimer();
    I_StartupKeyboard();

    locplayerstate = &PLAYERSTATE[consoleplayer];

    if (standalone)
    {
        ServerLoop();
    }

    InitGraphicsMode();
    VL_SetVGAPlaneMode();
    VL_SetPalette(origpal);

    SetViewSize(8);

    playstate = ex_titles;

    gamestate.battlemode = battle_StandAloneGame;
    BATTLE_SetOptions(&BATTLE_Options[battle_StandAloneGame]);

    if (turbo)
    {
        if (modemgame)
        {
            turbo = false;
            NoWait = true;
        }
        else
        {
            PlayTurboGame();
        }
    }
    else
    {
        if (dopefish && !shareware)
        {
            DopefishTitle();
        }
        else if (!NoWait)
        {
            if (shareware && W_CheckNumForName("svendor") != -1)
            {
                lbm_t *LBM;

                LBM = (lbm_t *)W_CacheLumpName("svendor", PU_CACHE, Cvt_lbm_t, 1);
                VL_DecompressLBM(LBM, true, 255, 255, 255);

                I_Delay(40);

                MenuFadeOut();
            }

            ApogeeTitle();
        }
    }

    GameLoop();

    QuitGame();
    return 0;
}

void DrawRottTitle (void)
{
    char title[80];
    char buf[5];

    if (CheckParm("QUIET") == 0)
    {
#ifdef ANSIESC
        printf("\n\n\n");
#endif
        strcpy(title, "Rise of the Triad Startup  Version ");
        strcat(title, itoa(ROTTMAJORVERSION, &buf[0], 10));
        strcat(title, ".");

        if (dfishpatch)
            strcat(title, "DFISH");
        else
            strcat(title, itoa(ROTTMINORVERSION, &buf[0], 10));

#ifndef ANSIESC
        strcat(title, "\n");
#endif

        px = (80 - strlen(title)) >> 1;
        py = 0;

        UL_printf(title);

        memset (title,0,sizeof(title));

        if (gamestate.Product == ROTT_SHAREWARE)
            strcpy(title, shareware_deluxe ? "Lasersoft Deluxe Version" : "Shareware Version");
        else if (gamestate.Product == ROTT_SUPERCD)
            strcpy(title, "CD Version");
        else if (gamestate.Product == ROTT_SITELICENSE)
            strcpy(title, "Site License CD Version");
        else
            strcpy(title, "Commercial Version");

        px = (80 - strlen(title)) >> 1;
        py = 1;

        UL_printf(title);
#ifndef ANSIESC
        printf ("\n");
#endif

        UL_ColorBox (0, 0, 80, 2, 0x1e);
    }
}

void CheckCommandLineParameters (void)
{
    // Everything after "QUIET" is custom for Excalibat
    char *PStrings[] = {"TEDLEVEL","NOWAIT","NOSOUND","NOW",
                        "TRANSPORT","DOPEFISH","SCREENSHOTS",
                        "MONO","MAPSTATS","TILESTATS","VER","net",
                        "PAUSE","SOUNDSETUP","WARP","IS8250","ENABLEVR",
                        "TIMELIMIT","MAXTIMELIMIT","NOECHO","DEMOEXIT","QUIET",
                        "PURE", "SCREENCLASSIC", "NOPIXCORR", "NOLETTBOX"
                        ,NULL
                       };

    int i, n;

    infopause = false;
    NoWait = false;
    NoSound = false;
    turbo = false;
    warp = false;
    dopefish = false;
    modemgame = false;
    SCREENSHOTS = false;
    MAPSTATS = false;
    TILESTATS = false;
    demoexit = false;

    modemgame = false;
    networkgame = false;
    consoleplayer = 0;
    numplayers = 1;
    timelimit = -1;
    timelimitenabled = false;
    noecho = false;
    quiet = false;

    if (CheckParm("?\0") || CheckParm("HELP") ||
            ((_argc > 1) && (_argv[1][0] == '?')))
    {
        printf ("Rise of the Triad  (c) 1995 Apogee Software\n\n");
        printf ("--- EXCALIBAT Port ---\n\n");
        printf ("COMMAND LINE PARAMETERS\n");
        printf ("   FULLSCREEN - Start in fullscreen mode\n");
        printf ("   WINDOW     - Start in windowed mode\n");
        printf ("   RESOLUTION - Specify the screen resolution to use\n");
        printf ("              - next param is <WIDTHxHEIGHT>\n");
        printf ("   SHAREWARE  - Force game to use Shareware files.\n");
        printf ("   NODELUXE   - If Shareware, don't look for DELUXE.WAD/RTL/RTC.\n");
        if (!shareware)
        {
            printf ("   FILERTL    - used to load Userlevels (RTL files)\n");
            printf ("              - next parameter is RTL filename\n");
            printf ("   FILERTC    - used to load Battlelevels (RTC files)\n");
            printf ("              - next parameter is RTC filename\n");
            printf ("   FILE       - used to load Extern WAD files\n");
            printf ("              - next parameter is WAD filename\n");
        }
        printf ("   NOJOYS     - Disable check for joystick.\n");
        printf ("   NOMOUSE    - Disable check for mouse.\n");
        printf ("   VER        - Version number.\n");
        printf ("   MAPSTATS   - Dump Map statistics to ERROR.\n");
        printf ("   TILESTATS  - Dump Tile statistics to ERROR.\n");
        printf ("   SCREENSHOTS- Clean screen capture for shots.\n");
        printf ("   PAUSE      - Pauses startup screen information.\n");
        printf ("   NOECHO     - Turn off sound reverb\n");
        printf ("   DEMOEXIT   - Exit program when demo is terminated\n");
        printf ("   WARP       - Warp to specific ROTT level\n");
        printf ("                next parameter is level to start on\n");
        printf ("   TIMELIMIT  - Play ROTT in time limit mode\n");
        printf ("                next parameter is time in seconds\n");
        printf ("   MAXTIMELIMIT - Maximimum time to count down from\n");
        printf ("                next parameter is time in seconds\n");
        printf ("   DOPEFISH   - ?\n");
        printf (" \n");
        printf ("CONTROLS\n");
        printf ("         Arrows           - Move\n");
        printf ("         Ctrl             - Fire\n");
        printf ("         Comma/Alt+left   - Sidestep Left\n");
        printf ("         Period/Alt+right - Sidestep Right\n");
        printf ("         Shift            - Run/Turn faster\n");
        printf ("         Space            - Use/Open\n");
        printf ("         1-4              - Choose Weapon\n");
        printf ("         5-6              - Scale Weapon Up/Down\n");
        printf ("         Enter            - Swap Weapon\n");
        printf ("         Backspace        - Turn 180\n");
        printf ("         Delete           - Drop Weapon\n");
        printf ("         +/-              - Change Viewsize\n");
        printf ("         PgUp/PgDn        - Look Up/Down\n");
        printf ("         Home/End         - Aim Up/Down\n");
        printf ("         [ ]              - Sound Volumen\n");
        printf ("         ( )              - Music Volumen\n");
        printf ("         Tab              - Enter Automapper\n");
        printf (" \n");
        printf ("AUTO-MAPPER\n");
        printf ("         Arrows           - Scroll around\n");
        printf ("         PgUp             - Zoom Out\n");
        printf ("         PgDn             - Zoom In\n");
        printf ("         Tab              - Exit Auto-Mapper\n");
        printf (" \n");
        printf ("HOTKEYS\n");
        printf ("         F1               - Help\n");
        printf ("         F2               - Save Game\n");
        printf ("         F3               - Restore Game\n");
        printf ("         F4               - Controls/Sound/Music\n");
        printf ("         F5               - Change Detail Level\n");
        printf ("         F6               - Quick Save\n");
        printf ("         F7               - Messages On/Off\n");
        printf ("         F8               - End Game\n");
        printf ("         F9               - Quick Load\n");
        printf ("         F10              - Quit\n");
        printf ("         F11              - Gamma Correction\n");
        printf (" \n");
        printf ("COMM-BAT\n");
        printf ("         F1 - F10         - RemoteRidicule(tm) sounds\n");
        printf ("         F12              - Live RemoteRidicule\n");
        printf ("         T                - Type message to all\n");
        printf ("         Z                - Type directed message\n");
        printf ("         Tab              - Toggle KillCount display\n");
        printf (" \n");
        printf ("SCREENSHOOT\n");
        printf ("         Alt+C            - Screenshoot in PCX format\n");

        exit(0);
    }

    // Check For command line parameters
    for (i = 1; i < _argc; i++)
    {
        n = US_CheckParm(_argv[i], PStrings);

        switch(n)
        {
            case 0:
                break;

            case 1:
                NoWait = true;
                break;

            case 2:
                NoSound = true;
                break;

            case 3:
                turbo = true;
                break;

            case 4:
                warp = true;
                warpx = ParseNum(_argv[i + 1]);
                warpy = ParseNum(_argv[i + 2]);
                warpa = ParseNum(_argv[i + 3]);
                break;

            case 5:
                dopefish = true;
                break;

            case 6:
                SCREENSHOTS = true;
                break;

            case 7:
                break;

            case 8:
                MAPSTATS = true;
                break;

            case 9:
                TILESTATS = true;
                break;

            case 10:
                printf("Rise of the Triad  (c) 1995 Apogee Software\n");

                if (gamestate.Product == ROTT_SHAREWARE)
                    printf(shareware_deluxe ? "Lasersoft Deluxe " : "Shareware ");
                else if (gamestate.Product == ROTT_SUPERCD)
                    printf("CD ");
                else if (gamestate.Product == ROTT_SITELICENSE)
                    printf("Site License ");
                else
                    printf("Commercial ");

                printf ("Version %d.%d\n", ROTTMAJORVERSION,ROTTMINORVERSION);
                exit (0);
                break;

            case 11:
                if (pure)
                {
                    Error("Netplay not supported in pure mode.\n");
                }

                InitROTTNET();
                numplayers = rottcom->numplayers;

                if ((shareware && (numplayers > MAXPLAYERS_SW)) ||
                        (numplayers > MAXPLAYERS))
                {
                    Error("Too many players.\n");
                }

                if (!quiet)
                    printf("Playing %ld player ROTT\n", (long int)numplayers);

                modemgame = true;

                if (rottcom->gametype == NETWORK_GAME)
                {
                    if (!quiet)
                        printf("NETWORK GAME\n");
                    networkgame = true;
                }
                else if (!quiet)
                    printf("MODEM GAME\n");

                break;

            case 12:
                infopause = true;
                break;

            case 13:
                break;

            case 14:
                startlevel = (ParseNum(_argv[i + 1])-1);
                break;

            case 15:
                break;

            case 16:
                break;

            case 17:
                timelimitenabled = true;
                timelimit = ParseNum(_argv[i + 1]);

                if (!quiet)
                    printf("Time Limit = %ld Seconds\n",(long int)timelimit);

                timelimit *= VBLCOUNTER;
                break;

            case 18:
                maxtimelimit = ParseNum(_argv[i + 1]);
                maxtimelimit *= VBLCOUNTER;
                break;

            case 19:
                noecho = true;
                break;

            case 20:
                demoexit = true;
                break;

            case 21:
                quiet = true;
                break;

            case 22:
                pure = true;
                screen_classic = true;
                break;

            case 23:
                screen_classic = true;
                break;

            case 24:
                screen_aspect = ASPECT_RATIO_IGNORE;
                break;

            case 25:
                screen_aspect = ASPECT_RATIO_IGNOREANDSTRETCH;
                break;
        }
    }
}

void CheckCommandLineParametersPostCfg (void)
{
    // Check command line parameters that would get overwritten
    // by config files in the above function.
    int arg, i;
    char *PStrings[] = {"FULLSCREEN", "WINDOW", "RESOLUTION", NULL};

    // These must be checked here so that they can override the cfg file
    for (i = 1; i < _argc; i++)
    {
        arg = US_CheckParm(_argv[i], PStrings);

        switch(arg)
        {
            case 0:
                screen_fullscreen = 1;
                break;

            case 1:
                screen_fullscreen = 0;
                break;

            case 2:
                i++;

                if (i < _argc)
                {
                    int width, height;

                    if ((sscanf(_argv[i], "%dx%d", &width, &height) == 2))
                    {
                        screen_dest_width = width;
                        screen_dest_height = height;
                    }
                    else
                        printf("Invalid resolution parameter: %s\n", _argv[i]);
                }
                else
                    printf("Missing resolution parameter\n");

                break;
        }
    }
}

boolean CheckIfFileExists (char *_filename)
{
    char filename[MAX_PATH];

    strncpy(filename, _filename, sizeof(filename));
    filename[sizeof(filename) - 1] = '\0';
    FixFilePath(filename);

    return (access(filename, 0) == 0);
}

int GetCRCFromFile (char *_filename)
{
    char filename[MAX_PATH];
    FILE *sourcefile;
    int crc16 = 0;

    strncpy(filename, _filename, sizeof(filename));
    filename[sizeof(filename) - 1] = '\0';
    FixFilePath(filename);

    if ((sourcefile = fopen(filename, "rb")) != NULL)
        crc16 = CalculateCRCFromFile(sourcefile);

    fclose(sourcefile);

    return crc16;
}

void CheckGameVersion (void)
{
    // Check command line parameter for forcing shareware.
    int arg, i;
    char *PStrings[] = {"SHAREWARE", NULL};
    boolean forceShareware = false;

    for (i = 1; i < _argc; i++)
    {
        arg = US_CheckParm(_argv[i], PStrings);

        switch(arg)
        {
            case 0:
                forceShareware = true;
                break;
        }
    }

    if (CheckIfFileExists(STANDARDWAD) && !forceShareware)
    {
        if (CheckIfFileExists(STANDARDBATTLELEVELS_TIER3))
            gamestate.Product = ROTT_SITELICENSE;
        else if (CheckIfFileExists(STANDARDBATTLELEVELS_TIER2))
            gamestate.Product = ROTT_SUPERCD;
        else
            gamestate.Product = ROTT_REGISTERED;

        if (CheckIfFileExists(DATADIR STANDARDGAMELEVELS))
            enabledrtls |= 1 << RTL_STANDARD;

        if (CheckIfFileExists(DATADIR SHAREWAREGAMELEVELS))
            enabledrtls |= 1 << RTL_SHAREWARE;

        if (CheckIfFileExists(DATADIR EXTREMEGAMELEVELS))
            enabledrtls |= 1 << RTL_EXTREME;

        if (enabledrtls & (1 << RTL_STANDARD))
            demortl = RTL_STANDARD;
        else if (enabledrtls & (1 << RTL_SHAREWARE))
            demortl = RTL_SHAREWARE;
        else if (enabledrtls & (1 << RTL_EXTREME))
            demortl = RTL_EXTREME;


        return;
    }

    if (CheckIfFileExists(SHAREWAREWAD))
    {
        gamestate.Product = ROTT_SHAREWARE;
        shareware = true;
        enabledrtls |= 1 << RTL_SHAREWARE;
        currentrtl = RTL_SHAREWARE;
        demortl = RTL_SHAREWARE;

        // We're actually playing the DELUXE version. Does some very
        // minor changes.
        if (GetCRCFromFile(SHAREWAREWAD) == WADFILECHECKSUM_DELUXE)
            shareware_deluxe = true;

        return;
    }

    Error("No valid WAD file found.");
}

void SetupWads (void)
{
    char *newargs[99];
    int arg, argnum = 0;
    char tempstr[129];

    if (!shareware)
    {
        // Check for rtl files
        arg = CheckParm("filertl");

        if (arg != 0)
        {
            FILE *f;
            char buf[32];

            if (_argv[arg+1] != 0)   //are there a filename included
            {
                strcpy(tempstr,_argv[arg+1]);//copy it to tempstr

                if (strlen(tempstr) < MAX_PATH)
                {
                    if (access (tempstr, 0) != 0)   //try open
                    {
                        strcat (tempstr,".rtl");//non exists, try add .rtl
                        if (access (tempstr, 0) != 0)   //try open again
                        {
                            //stil no useful filename
                            strcat (tempstr," not found, skipping RTL file ");
                            printf("%s\n", tempstr);
                            goto NoRTL;
                        }
                    }

                    if((f = fopen( tempstr, "r" )) == NULL )  //try opnong file
                    {
                        strcat (tempstr," not could not be opened, skipping RTL file ");
                        printf("%s\n", tempstr);
                        goto NoRTL;
                    }
                    else
                    {
                        fread(buf,3,3,f);//is the 3 first letters RTL (RTC)
                        if (((strstr(buf,"RTL") != 0)||strstr(buf,"RTC") != 0))
                        {
                            strcpy (GameLevels.file,tempstr);
                            GameLevels.avail++;
                            currentrtl = RTL_CUSTOM;
                            enabledrtls = 0; // Ignore RTL selection.
                            strcpy (buf,"Adding ");
                            strcat (buf,tempstr);
                            printf("%s\n", buf);
                        }
                        fclose(f);
                    }
                }
            }
            else
            {
                printf("Missing RTL filename\n");
            }
        }
NoRTL:
        ;

        // Check for rtc files
        arg = CheckParm ("filertc");
        if (arg != 0)
        {
            FILE *f;
            char buf[32];
            if (_argv[arg+1] != 0)   //are there a filename included
            {
                strcpy (tempstr,_argv[arg+1]);//copy it to tempstr
                if (strlen (tempstr) < MAX_PATH)
                {
                    if (access (tempstr, 0) != 0)   //try open
                    {
                        strcat (tempstr,".rtc");//non exists, try add .rtc
                        if (access (tempstr, 0) != 0)   //try open again
                        {
                            //stil no useful filename
                            strcat (tempstr," not found, skipping RTC file ");
                            printf("%s\n", tempstr);
                            goto NoRTC;
                        }
                    }

                    if((f = fopen( tempstr, "r" )) == NULL )  //try opening file
                    {
                        strcat (tempstr," not could not be opened, skipping RTC file ");
                        printf("%s", tempstr);
                        goto NoRTC;
                    }
                    else
                    {
                        fread(buf,3,3,f);//is the 3 first letters RTL (RTC)
                        if (((strstr(buf,"RTL") != 0)||strstr(buf,"RTC") != 0))
                        {
                            strcpy (BattleLevels.file,tempstr);
                            BattleLevels.avail++;
                            strcpy (buf,"Adding ");
                            strcat (buf,tempstr);
                            printf("%s\n", buf);
                        }
                        fclose(f);
                    }
                }
            }
            else
            {
                printf("Missing RTC filename\n");
            }
        }
NoRTC:
        ;

        // Check for User wads
        arg = CheckParm("file");
        if (arg != 0)
            newargs[argnum++] = _argv[arg+1];

        arg = CheckParm("file1");
        if (arg != 0)
            newargs[argnum++] = _argv[arg+1];

        arg = CheckParm("file2");
        if (arg != 0)
            newargs[argnum++] = _argv[arg+1];

    }
    else if ((CheckParm ("file") > 0) || (CheckParm ("file1") > 0) || (CheckParm ("file2") > 0))
        printf("External wads ignored.\n");

    // Normal ROTT wads
    if (shareware)
        newargs[argnum++] = SHAREWAREWAD;
    else
        newargs[argnum++] = STANDARDWAD;

    // Check for Remote Ridicule WAD
    if (RemoteSounds.avail)
    {
        char *src;

        strcpy(tempstr,RemoteSounds.path);
        src = RemoteSounds.path + strlen(RemoteSounds.path) - 1;

        if (*src != '\\')
            strcat (tempstr,"\\\0");

        strcat(tempstr,RemoteSounds.file);
        newargs[argnum++] = tempstr;
    }
    else
        newargs[argnum++] = DATADIR "REMOTE1.RTS";

    newargs[argnum++] = NULL;

    W_InitMultipleFiles(newargs);
}

void PlayTurboGame (void)
{
    NewGame = true;
    locplayerstate->player = DefaultPlayerCharacter;
    playstate = ex_resetgame;
    GameLoop();
}


//***************************************************************************
//
// Init_Tables () - Init tables needed for double buffering
//
//***************************************************************************

void Init_Tables (void)
{
    int i;
    byte *shape;

    memset (&CWD[0], 0, 40);
    getcwd (CWD, 40);                      // get the current directory

    origpal=SafeMalloc(768);
    memcpy (origpal, W_CacheLumpName("pal",PU_CACHE, CvtNull, 1), 768);

    FindEGAColors();

    for (i = 0; i < 0x300; i++)
        *(origpal+(unsigned int)i) = (*(origpal+(unsigned int)i))>>2;

    // Cache in fonts
    shape = W_CacheLumpNum (W_GetNumForName ("smallfont"), PU_STATIC, Cvt_font_t, 1);
    smallfont = (font_t *)shape;
    CurrentFont = smallfont;

    // Cache in tiny font
    shape = W_CacheLumpNum (W_GetNumForName ("tinyfont"), PU_STATIC, Cvt_font_t, 1);
    tinyfont = (font_t *)shape;

    intensitytable=W_CacheLumpNum(W_GetNumForName("menucmap"),PU_STATIC, CvtNull, 1);
    fontcolor = egacolor[4];

    if (!quiet)
        printf("RT_MAIN: Fonts Initialized\n");
}




int NumberOfTeams (void)
{
    int index;
    int team[MAXPLAYERCOLORS];
    int count;
    int color;

    memset(team, 0, sizeof(team));

    count = 0;
    for(index = 0; index < numplayers; index++)
    {
        color = PLAYERSTATE[index].uniformcolor;

        if (!team[color])
        {
            team[color] = true;
            count++;
        }
    }

    return count;
}

void GameLoop (void)
{
    int NextLevel;

    while (1)
    {
        switch(playstate)
        {
            case ex_battledone:
                while(damagecount > 0)
                    DoBorderShifts();

                damagecount = 0;
                SetBorderColor(0);

                StopWind();

                ShutdownClientControls();

                SD_Play(SD_LEVELDONESND);

                if ((player->flags & FL_DOGMODE) ||
                        (gamestate.battlemode == battle_Eluder))
                {
                    MU_StartSong(song_dogend);
                }
                else
                {
                    MU_StartSong(song_endlevel);
                }

                VL_FillPalette(255, 255, 255);
                VL_FadeIn(0, 255, origpal, 10);

                BattleLevelCompleted(consoleplayer);

                BATTLE_Shutdown();

                Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                ingame = false;

                if (networkgame)
                    AddGameEndCommand();

                AdjustMenuStruct();

                CalcTics();
                CalcTics();

                playstate = ex_titles;
                break;

            case ex_titles:

                BATTLE_Shutdown();
                MU_StartSong(song_title);

                if ((!NoWait) && (!modemgame))
                {
                    byte dimpal[768];
                    int i;
                    byte *tempbuf;

                    for (i = 0; i < 0x300; i++)
                        dimpal[i] = origpal[i]>>2;

                    CalcTics();
                    CalcTics();

                    IN_ClearKeysDown();

                    while (IN_GetMouseButtons()) {}

                    while ((!LastScan) && (!IN_GetMouseButtons()))
                    {
                        MenuFadeOut();
                        ClearGraphicsScreen();
                        SetPalette(&dimpal[0]);

                        // TODO LOWPRIORITY - For modified/patched WADs, the title scripts
                        // shartitl and shartit2 can change. Check for a checksum change and use
                        // the cinematic engine if shartitl or shartit2 are different. Tempoarily
                        // changing it to screen_classic is the best way to do this.
                        if (screen_classic)
                        {
                            PlayMovie("shartitl", true);
                            if (LastScan || IN_GetMouseButtons())
                                break;

                            PlayMovie("shartit2", true);
                            if (LastScan || IN_GetMouseButtons())
                                break;
                        }
                        else
                        {
                            // Simplify title sequence so that it doesn't require reading a script
                            // or managing a dedicated cinematic engine. Also makes it better for
                            // higher resolutions.
                            PlayTitleSequence();
                            if (LastScan || IN_GetMouseButtons())
                                break;
                        }

                        SD_Play(SD_LIGHTNINGSND);
                        MenuFadeIn();
                        I_Delay(30);
                        SD_Play(SD_ACTORSQUISHSND);

                        tempbuf = bufferofs;
                        bufferofs = page1start; // fixed, was displayofs

                        DrawNormalSpriteFree(screen_src_width - (94 * screen_src_minscale),
                                             screen_src_height - (41 * screen_src_minscale), W_GetNumForName("rsac"));
                        VW_UpdateScreen(); // fixed, was missing

                        bufferofs = tempbuf;
                        I_Delay(30);

                        if (LastScan || IN_GetMouseButtons())
                            break;

                        DoCreditScreen();

                        if ((!LastScan) && (!IN_GetMouseButtons()) && (demortl != -1))
                            CheckHighScore(0, 0, false, demortl);

                        if ((!LastScan) && (!IN_GetMouseButtons()) && !shareware)
                            DoMicroStoryScreen();

                        if ((!LastScan) && (!IN_GetMouseButtons()) && (!GameLevels.avail) && (demortl != -1))
                        {
                            if (currentrtl != demortl)
                            {
                                currentrtl = demortl;
                                quicksaveslot = -1;
                            }

                            if (demonumber == -1)
                                demonumber = RandomNumber("GameLoop", 0);

                            for (i = 0; i < 4; i++)
                            {
                                demonumber = (demonumber + 1) % 4;

                                if (DemoExists(demonumber + 1))
                                    break;
                            }

                            if (DemoExists(demonumber + 1))
                            {
                                ingame = true;
                                LoadDemo(demonumber + 1);
                                break;
                            }
                        }
                    }
                }

                if (playstate != ex_demoplayback)
                {
                    if (demoexit)
                        QuitGame();

                    NoWait = false;
                    SwitchPalette(origpal, 35);
                    CP_MainMenu();
                }
                break;

            case ex_resetgame:
                InitCharacter();

                InitializeMessages();

                fizzlein = true;
                BATTLE_GetSpecials();
                BATTLE_SetOptions(&BATTLE_Options[gamestate.battlemode]);

                if (modemgame)
                {
                    fizzlein = false;

                    if (consoleplayer == 0)
                        SetupGameMaster();
                    else
                        SetupGamePlayer();

                    if (gamestate.Version < ROTTVERSION)
                        Error("This version of Rise of the Triad (%d.%d) is incompatible with\n"
                              "version %d.%d.", ROTTMAJORVERSION, ROTTMINORVERSION,
                              gamestate.Version / 10, gamestate.Version % 10);

                    if (gamestate.teamplay)
                    {
                        int teams = NumberOfTeams();

                        if ((gamestate.battlemode == battle_CaptureTheTriad) && (teams != 2))
                        {
                            CP_CaptureTheTriadError();
                            playstate = ex_titles;
                            continue;
                        }
                        else if (teams < 2)
                        {
                            CP_TeamPlayErrorMessage();
                            playstate = ex_titles;
                            continue;
                        }
                    }
                }

                InitCharacter();

                BATTLE_Init(gamestate.battlemode, numplayers);

                NewGame = true;

                if (BATTLEMODE && BATTLE_ShowKillCount)
                    StatusBar |= STATUS_KILLS;
                else
                    StatusBar &= ~STATUS_KILLS;

                if (!loadedgame)
                {
                    if (!BATTLEMODE && !shareware && (pure || currentrtl == RTL_STANDARD))
                        PlayCinematic();

                    SetupGameLevel();
                }

                IN_ClearKeyboardQueue();

                SetupScreen(true);

                MenuFixup();
                playstate = ex_stillplaying;
                break;

            case ex_stillplaying:
                InitializeMessages();

                SHAKETICS = 0xFFFF;

                if (modemgame)
                {
                    ComSetTime();
                    turbo = false;
                }
                else if (turbo)
                    turbo = false;
                else
                    newlevel = true;

                PlayLoop();
                break;

            case ex_died:
                Died();
                StopWind();

                while (damagecount > 0)
                    DoBorderShifts();

                damagecount = 0;
                SetBorderColor(0);

                if (demorecord)
                    FreeDemo();

                if (demoplayback)
                {
                    FreeDemo();
                    playstate = ex_demodone;
                }
                else
                {
                    ShutdownClientControls();

                    Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                    if (!CheckForQuickLoad())
                    {
                        if (locplayerstate->lives < 0)
                        {
                            if (!timelimitenabled)
                            {
                                CheckHighScore(gamestate.score, gamestate.mapon + 1, false, currentrtl);
                                playstate = ex_titles;
                                AdjustMenuStruct();
                                ingame = false;
                                locplayerstate->health = MaxHitpointsForCharacter(locplayerstate);
                                gamestate.score = 0;
                                locplayerstate->lives = 3;
                                locplayerstate->weapon = wp_pistol;
                                locplayerstate->triads = 0;
                                UpdateLives(locplayerstate->lives);
                                UpdateScore(gamestate.score);
                            }
                            else
                                QuitGame();
                        }
                        else
                        {
                            fizzlein = true;
                            SetupGameLevel();
                            UpdateTriads(player, 0);
                            playstate = ex_stillplaying;
                        }
                    }
                }
                break;

            case ex_warped:
                StopWind();
                TurnShakeOff();
                SHAKETICS = 0xffff;
                gamestate.TimeCount = 0;
                gamestate.frame = 0;

                Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                fizzlein = true;
                SetupGameLevel();

                playstate = ex_stillplaying;
                break;

            case ex_skiplevel:
            case ex_secretdone:
            case ex_secretlevel:
            case ex_completed:
            case ex_bossdied:
                ShutdownClientControls();

                TurnShakeOff();
                SHAKETICS = 0xffff;

                if (!timelimitenabled)
                {
                    gamestate.TimeCount = 0;
                    gamestate.frame=0;
                }

                StopWind();

                if ((playstate == ex_bossdied) && (gamestate.mapon != 30) && !shareware)
                {
                    int shape;
                    lbm_t *LBM;
                    byte *s;
                    patch_t *p;
                    char str[50];
                    int width, height;

                    MenuFadeOut();
                    LBM = (lbm_t *)W_CacheLumpName("deadboss", PU_CACHE, Cvt_lbm_t, 1);
                    VL_DecompressLBM(LBM, false, 0, 0, 0);
                    //MenuFadeOut();

                    switch (gamestate.mapon)
                    {
                        case 6:
                            shape = W_GetNumForName("deadstev");
                            break;
                        case 14:
                            shape = W_GetNumForName("deadjoe");
                            break;
                        case 22:
                            shape = W_GetNumForName("deadrobo");
                            break;
                        case 33:
                            shape = W_GetNumForName("deadtom");
                            break;
                    }

                    s = W_CacheLumpNum(shape, PU_CACHE, Cvt_patch_t, 1);
                    p = (patch_t *)s;
                    DrawNormalSpriteFree((screen_src_width - (int)(p->origsize * screen_src_minscale)) >> 1,
                                         (screen_src_height + (int)(30 * screen_src_minscale) -
                                          (int)((p->height - p->topoffset) * screen_src_minscale)) >> 1, shape);

                    switch (gamestate.mapon)
                    {
                        case 6:
                            strcpy(&str[0],"\"General\" John Darian");
                            break;
                        case 14:
                            strcpy(&str[0],"Sebastian \"Doyle\" Krist");
                            break;
                        case 22:
                            strcpy(&str[0],"the NME");
                            break;
                        case 33:
                            strcpy(&str[0],"El Oscuro");
                            break;
                    }

                    CurrentFont = smallfont;
                    US_MeasureStr(&width, &height, "%s", str);
                    US_ClippedPrint((screen_src_width - (int)(width * screen_src_minscale)) >> 1,
                                    screen_src_height - (int)(20 * screen_src_minscale), str);
                    VW_UpdateScreen();
                    MenuFadeIn();

                    WaitKeyUp();
                    LastScan = 0;
                    while (!LastScan) IN_UpdateKeyboard();
                    LastScan=0;
                }

                LevelCompleted(playstate);

                NextLevel = GetNextMap(player->tilex, player->tiley);

                demoplayback = false;

                Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                if (NextLevel != -1)
                {
                    gamestate.mapon = NextLevel;

                    if (!shareware && (pure || currentrtl == RTL_STANDARD))
                        PlayCinematic();

                    fizzlein = true;
                    SetupGameLevel();
                    playstate = ex_stillplaying;
                }
                else
                    playstate = ex_gameover;
                break;

            case ex_demodone:
                ingame = false;
                ShutdownClientControls();
                TurnShakeOff();
                SHAKETICS = 0xffff;
                gamestate.TimeCount = 0;
                gamestate.frame = 0;

                demoplayback = false;

                Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                if (predemo_violence != -1)
                {
                    gamestate.violence = predemo_violence;
                    predemo_violence = -1;
                }

                playstate = ex_titles;
                break;

            case ex_gameover:
                StopWind();
                if (shareware || (!pure && currentrtl == RTL_SHAREWARE))
                    DoEndCinematicShareware();
                else if (!pure && currentrtl == RTL_EXTREME)
                    DoEndCinematicExtreme();
                else if (pure || currentrtl == RTL_STANDARD)
                    DoEndCinematic();

                if (playstate == ex_gameover)
                {
                    CheckHighScore(gamestate.score, gamestate.mapon + 1, false, currentrtl);

                    ingame = false;
                    AdjustMenuStruct();
                    playstate = ex_titles;
                }
                break;

            case ex_demorecord:
                ShutdownClientControls();
                Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                RecordDemo();
                SetupGameLevel();

                fizzlein = true;
                playstate = ex_stillplaying;
                break;

            case ex_demoplayback:
                ShutdownClientControls();
                Z_FreeTags(PU_LEVELSTRUCT, PU_LEVELEND);       // Free current level

                SetupDemo();
                SetupGameLevel();

                fizzlein = true;
                playstate = ex_stillplaying;
                break;
        }
    }
}

boolean CheckForQuickLoad (void)
{
    if (pickquick)
    {
        SetupMenuBuf();

        pickquick = CP_DisplayMsg("\nQuick load saved game?\n", 12);

        if (pickquick)
        {
            AllocateSavedScreenPtr();
            CP_LoadGame(1, 1);
            FreeSavedScreenPtr();
        }
        else
        {
            // Erase the quick load message
            VL_FadeOut(0, 255, 0, 0, 0, 20);
        }

        ShutdownMenuBuf();
    }

    return pickquick;
}

//===========================================================================

void ShutDown (void)
{
    if (!standalone == false)
        WriteConfig ();

//   if (
//       (networkgame==false) &&
//       (modemgame==true)
//      )
//      {
//      ShutdownModemGame ();
//      }

    ShutdownClientControls();
    I_ShutdownKeyboard();
    ShutdownGameCommands();
    MU_Shutdown();
    I_ShutdownTimer();
    SD_Shutdown();
    IN_Shutdown ();
    ShutdownSoftError ();
    Z_ShutDown();
}

//===========================================================================

#if (DEVELOPMENT == 1)
extern int totallevelsize;
#endif

void QuitGame (void)
{
#if (DEBUG == 1)
    char buf[5];
#endif
#if (DEVELOPMENT == 1)
    int temp;
    int k;
#else
    byte *txtscn;
#endif

    MU_FadeOut(200);

    while (MU_FadeActive())
    {
        int time = GetTicCount();
        while (GetTicCount() == time) {}
    }

    PrintMapStats();
    PrintTileStats();

#if (DEVELOPMENT == 1)
    printf("Clean Exit\n");

    if (gamestate.TimeCount)
    {
        temp = (gamestate.frame * VBLCOUNTER*100) / gamestate.TimeCount;
        printf("fps  = %2ld.%2ld\n", temp/100, temp%100);
    }

    printf("argc=%ld\n", _argc);

    for (k = 0; k < _argc; k++)
        printf("%s\n", _argv[k]);

    /*    switch(_heapchk())
        {
            case _HEAPOK:
                printf("OK - heap is good\n");
                break;

            case _HEAPEMPTY:
                printf("OK - heap is empty\n");
                break;

            case _HEAPBADBEGIN:
                printf("ERROR - heap is damaged\n");
                break;

            case _HEAPBADNODE:
                printf("ERROR - bad node in heap\n");
                break;

        }*/
    printf("\nLight Characteristics\n");
    printf("---------------------\n");
    if (fog)
        printf("FOG is ON\n");
    else
        printf("FOG is OFF\n");
    printf("LIGHTLEVEL=%ld\n", GetLightLevelTile());
    printf("LIGHTRATE =%ld\n", GetLightRateTile());
    printf("\nCENTERY=%ld\n", centery);
#else

    if (shareware)
        txtscn = (byte *)W_CacheLumpNum(W_GetNumForName("shareend"), PU_CACHE, CvtNull, 1);
    else
        txtscn = (byte *)W_CacheLumpNum(W_GetNumForName("regend"), PU_CACHE, CvtNull, 1);

#if defined (ANSIESC)
    DisplayTextSplash(txtscn, 25);
#endif

#if (DEBUG == 1)
    px = ERRORVERSIONCOL;
    py = ERRORVERSIONROW;
#if (BETA == 1)
    UL_printf("á");
#else
    UL_printf(itoa(ROTTMAJORVERSION, &buf[0], 10));
#endif
    // Skip the dot
    px++;

    UL_printf(itoa(ROTTMINORVERSION, &buf[0], 10));
#endif
#endif

    SDL_Quit();
    exit(0);
}

void InitCharacter (void)
{
    locplayerstate->health = MaxHitpointsForCharacter(locplayerstate);

    if (timelimitenabled)
        locplayerstate->lives = 1;
    else
        locplayerstate->lives = 3;

    ClearTriads(locplayerstate);
    locplayerstate->playerheight = characters[locplayerstate->player].height;
//   locplayerstate->stepwhich = 0;
//   locplayerstate->steptime = 0;

    gamestate.score = 0;

    if (gamestate.battlemode == battle_StandAloneGame)
    {
        gamestate.mapon = startlevel;
        gamestate.difficulty = DefaultDifficulty;
    }
    else
        gamestate.difficulty = gd_hard;

    gamestate.dipballs = 0;
    gamestate.TimeCount = 0;

    godmode = 0;
    damagecount = 0;

    UpdateScore(gamestate.score);
}




void UpdateGameObjects (void)
{
    int j;
    objtype *ob, *temp;
    battle_status BattleStatus;

    if (controlupdatestarted == 0)
    {
        return;
    }

    UpdateClientControls();

    if (!demoplayback)
        PollControls();

    CalcTics();

    UpdateClientControls();

    while (oldpolltime < oldtime)
    {
        UpdateClientControls ();
        MoveDoors();
        ProcessElevators();
        MovePWalls();
        UpdateLightning ();
        TriggerStuff();
        CheckCriticalStatics();

        for(j = 0; j < numclocks; j++)
            if (Clocks[j].time1 &&
                    ((gamestate.TimeCount == Clocks[j].time1) ||
                     (gamestate.TimeCount == Clocks[j].time2)))
                TRIGGER[Clocks[j].linkindex] = 1;

        for (ob = firstactive; ob;)
        {
            temp = ob->nextactive;
            DoActor (ob);
#if (DEVELOPMENT == 1)
            if ((ob->x<=0) || (ob->y<=0))
                Error("object xy below zero obj->x=%ld obj->y=%ld obj->obclass=%ld\n",ob->x,ob->y,ob->obclass);
            if ((ob->angle<0) || (ob->angle>=FINEANGLES))
                Error("object angle below zero obj->angle=%ld obj->obclass=%ld\n",ob->angle,ob->obclass);
#endif
            ob = temp;
        }

        BattleStatus = BATTLE_CheckGameStatus(battle_refresh, 0);
        if (BattleStatus != battle_no_event)
        {
            switch (BattleStatus)
            {
                case battle_end_game:
                case battle_out_of_time:
                    playstate = ex_battledone;
                    break;

                case battle_end_round:
                    SetWhoHaveWeapons();
                    break;
            }

            if (playstate == ex_battledone)
                break;
        }

#if (SYNCCHECK == 1)
        CheckForSyncCheck();
#endif
        if (timelimitenabled)
        {
            if ((timelimit - gamestate.TimeCount) > maxtimelimit)
                timelimit = maxtimelimit + gamestate.TimeCount;

            if (gamestate.TimeCount == timelimit)
            {
                locplayerstate->lives = -1;
                playstate = ex_died;
            }
        }

        gamestate.TimeCount++;

        ResetCurrentCommand();

        oldpolltime++;
        if (GamePaused)
            break;
    }

    UpdateClientControls ();

    if (!noecho)
    {
        if (player->flags & FL_SHROOMS)
            FX_SetReverb(230);
        else if (sky == 0)
            FX_SetReverb(min(numareatiles[player->areanumber] >> 1, 90));
    }
}


void PauseLoop(void)
{
    StopWind();

    UpdateClientControls();

    while (oldpolltime < oldtime)
    {
        CheckUnPause();
#if (SYNCCHECK == 1)
        CheckForSyncCheck();
#endif
        oldpolltime++;

        if (!GamePaused)
            break;
    }

    CalcTics();

    if (!demoplayback)
        PollControls ();

    if (RefreshPause && GamePaused &&
            ((GetTicCount() - pausedstartedticcount) >= blanktime))
    {
        RefreshPause = false;
        StartupScreenSaver();
    }
}



void PlayLoop (void)
{
    boolean canquit = true;
    int quittime = 0;

    if (!loadedgame && !timelimitenabled)
    {
        gamestate.TimeCount = 0;
        gamestate.frame = 0;
    }

fromloadedgame:

    GamePaused = false;

    if (!loadedgame)
    {
        DrawPlayScreen(true);
        missobj = NULL;
    }
    else
    {
        loadedgame = false;
        DoLoadGameSequence();
    }

    tics      = 0;

    if (!fizzlein)
        StartupClientControls();
    else
        ShutdownClientControls();

    // set detail level
    doublestep = 2 - DetailLevel;

    ResetMessageTime();
    DeletePriorityMessage(MSG_SYSTEM);

    if ((gamestate.battlemode == battle_Normal) && (numplayers == 1))
    {
        AddMessage("Comm-bat is for Modem and Network games.", MSG_GAME);
        AddMessage("You will not be facing any", MSG_GAME);
        AddMessage("opponents.  Have fun and explore.", MSG_GAME);
    }

    while (playstate == ex_stillplaying)
    {
        UpdateClientControls();

        if (GamePaused)
        {
            PauseLoop();

            if (RefreshPause)
                ThreeDRefresh();
            else
                UpdateScreenSaver();
        }
        else
        {
            if (controlupdatestarted == 1)
                UpdateGameObjects();

            ThreeDRefresh();
        }

        SyncToServer();

        // Don't allow player to quit if entering message
        canquit = !MSG.messageon;

        PollKeyboard();

        MISCVARS->madenoise = false;

        AnimateWalls();

        UpdateClientControls();

#if (DEVELOPMENT == 1)
        Z_CheckHeap();
#endif

        if (AutoDetailOn)
            AdaptDetail();

        UpdateClientControls();

        DoSprites();
        DoAnimatedMaskedWalls();

        UpdatePlayers();

        DrawTime(false);

        UpdateClientControls();

        if (!BATTLEMODE && CP_CheckQuick(LastScan))
        {
            boolean escaped = false;

            if (LastScan == sc_Escape)
            {
                MU_StoreSongPosition();
                MU_StartSong(song_menu);
                escaped = true;
            }

            TurnShakeOff();
            StopWind();
            SetBorderColor(0);
            ShutdownClientControls();

            if (demoplayback)
            {
                FreeDemo();
                playstate = ex_demodone;

                if (demoexit)
                    QuitGame();

                return;
            }

            ControlPanel(LastScan);

            // set detail level
            doublestep = 2 - DetailLevel;

            inmenu = false;

            if (playstate == ex_titles)
                return;

            if (playstate == ex_stillplaying)
                SetupScreen(false);

            if (loadedgame)
                goto fromloadedgame;

            if ((playstate == ex_stillplaying) && (!fizzlein || GamePaused))
                StartupClientControls();

            if ((playstate == ex_stillplaying) && !GamePaused && escaped)
            {
                MU_StartSong(song_level);
                MU_RestoreSongPosition();
            }
        }

        if (BATTLEMODE)
        {
            if (!MSG.messageon)
                CheckRemoteRidicule(LastScan);

            if (!quitactive)
            {
                if ((LastScan == sc_Escape) && canquit)
                {
                    quitactive = true;
                    quittime   = GetTicCount() + QUITTIMEINTERVAL;

                    if (consoleplayer == 0 || !networkgame)
                        AddMessage("Do you want to end this game? "
                                   "(\\FY\\O/\\FN\\O)", MSG_QUIT);
                    else
                        AddMessage("Do you want to exit to DOS? "
                                   "(\\FY\\O/\\EN\\O)", MSG_QUIT);
                }
            }
            else
            {
                if (GetTicCount() > quittime)
                    quitactive = false;
                else if (LastScan == sc_N)
                {
                    DeletePriorityMessage(MSG_QUIT);
                    quitactive = false;
                }
                else if (LastScan == sc_Y)
                {
                    DeletePriorityMessage(MSG_QUIT);

                    if ((consoleplayer == 0) || !networkgame)
                        AddEndGameCommand();
                    else
                        AddQuitCommand();
                }
            }
        }
    }
}

//******************************************************************************
//
// CheckRemoteRidicule ()
//
//******************************************************************************

void CheckRemoteRidicule (int scancode)
{
    int num = -1;

    switch (scancode)
    {
        case sc_F1:
            num = 0;
            break;

        case sc_F2:
            num = 1;
            break;

        case sc_F3:
            num = 2;
            break;

        case sc_F4:
            num = 3;
            break;

        case sc_F5:
            if (!Keyboard[sc_RShift])
                num = 4;
            break;

        case sc_F6:
            num = 5;
            break;

        case sc_F7:
            if (!Keyboard[sc_RShift])
                num = 6;
            break;

        case sc_F8:
            num = 7;
            break;

        case sc_F9:
            num = 8;
            break;

        case sc_F10:
            num = 9;
            break;
    }

    if (num >= 0)
    {
        AddRemoteRidiculeCommand(consoleplayer, MSG_DIRECTED_TO_ALL, num);
        LastScan = 0;
    }
}

//******************************************************************************
//
// DoBossKey ()
//
//******************************************************************************

void DoBossKey (void)
{
    STUB_FUNCTION;
}


//******************************************************************************
//
// PollKeyboard ()
//
//******************************************************************************
static boolean mapbuttonpressed = false;
static boolean autopressed = false;
void PollKeyboard (void)
{
    if (demoplayback)
        IN_UpdateKeyboard();

    if (!BATTLEMODE)
    {
        CheckDebug ();

        if (Keyboard[sc_CapsLock] && DebugOk)
            DebugKeys ();
    }

    if (locplayerstate->buttonstate[bt_autorun])
    {
        if (!autopressed)
        {
            autopressed = true;
            gamestate.autorun ^= 1;

            if (gamestate.autorun == 0)
                AddMessage("AutoRun is \\cOFF", MSG_SYSTEM);
            else
                AddMessage("AutoRun is \\cON", MSG_SYSTEM);
        }
    }
    else
        autopressed = false;

    if (!modemgame && devkeys)
        CheckDevelopmentKeys();

    if (!MSG.messageon && !quitactive)
    {
        if ((Keyboard[buttonscan[bt_message]]) && BATTLEMODE)
        {
            // Send message to all
            MSG.messageon = true;
            MSG.directed = false;
            MSG.inmenu = false;
            MSG.remoteridicule = -1;
            MSG.towho = MSG_DIRECTED_TO_ALL;
            MSG.textnum = AddMessage("_", MSG_MODEM);
            MSG.length = 1;
            DeletePriorityMessage(MSG_MACRO);
        }
        else if ((Keyboard[buttonscan[bt_directmsg]]) && BATTLEMODE)
        {
            // Send directed message
            MSG.messageon = true;
            MSG.directed = true;
            MSG.inmenu = false;
            MSG.remoteridicule = -1;
            MSG.towho = 0;
            MSG.textnum = AddMessage("_", MSG_MODEM);
            MSG.length = 1;
            DeletePriorityMessage( MSG_MACRO );
        }

        if (buttonpoll[bt_map])
        {
            if (!mapbuttonpressed)
            {
                mapbuttonpressed = true;

                if (!BATTLEMODE)
                {
                    // Automap
                    StopWind();
                    DoMap(player->tilex, player->tiley);
                }
                else
                {
                    // Show kill counts
                    if (SHOW_KILLS())
                    {
                        BATTLE_ShowKillCount = false;
                        StatusBar &= ~STATUS_KILLS;
                    }
                    else
                    {
                        StatusBar |= STATUS_KILLS;
                        BATTLE_ShowKillCount = true;
                    }

                    SetupScreen(true);
                }
            }
        }
        else
            mapbuttonpressed = false;

        // Shrink screen
        if (Keyboard[sc_Minus])
        {
            Keyboard[sc_Minus] = false;

            if (viewsize > 0)
            {
                viewsize--;
                SetupScreen(true);
            }
        }

        // Expand screen
        if (Keyboard[sc_Plus])
        {
            Keyboard[sc_Plus] = false;

            if (viewsize < MAXVIEWSIZES - 1)
            {
                viewsize++;
                SetupScreen(true);
            }
        }

        // Set detail
        if (Keyboard[sc_F5] && (!BATTLEMODE || Keyboard[sc_RShift]))
        {
            Keyboard[sc_F5] = false;
            LastScan = 0;

            if (pure)
            {
                DetailLevel++;

                if (DetailLevel > 2)
                    DetailLevel = 0;

                switch (DetailLevel)
                {
                    case 0:
                        AddMessage("Low detail", MSG_SYSTEM);
                        break;

                    case 1:
                        AddMessage("Medium detail", MSG_SYSTEM);
                        break;

                    case 2:
                        AddMessage("High detail", MSG_SYSTEM);
                        break;
                }

                doublestep = 2 - DetailLevel;
            }
            else
            {
                screen_classic ^= 1;

                switch (screen_classic)
                {
                    case 0:
                        AddMessage("Classic detail disabled.", MSG_SYSTEM);
                        break;

                    case 1:
                        AddMessage("Classic detail enabled.", MSG_SYSTEM);
                        break;
                }

                ChangeScreenResolution(screen_dest_width, screen_dest_height);
                ChangeVideoMode();
                VL_SetVGAPlaneMode();
                VL_SetPalette(origpal);
                SetupScreen(true);
            }
        }

        // Turn messages on/off
        if (Keyboard[sc_F7] && (!BATTLEMODE || Keyboard[sc_RShift]))
        {
            Keyboard[sc_F7] = false;
            LastScan = 0;
            MessagesEnabled = !MessagesEnabled;

            if (!MessagesEnabled)
                AddMessage("Messages disabled.", MSG_MSGSYSTEM);
            else
                AddMessage("Messages enabled.", MSG_MSGSYSTEM);
        }

        if (Keyboard[sc_F6] && !BATTLEMODE)
        {
            Keyboard[sc_F6] = false;

            if (Keyboard[sc_RShift])
            {
                ShutdownClientControls();
                UndoQuickSaveGame();
                StartupClientControls();
            }
            else if (quicksaveslot == -1)
            {
                ShutdownClientControls();
                LastScan = sc_F2;
                inmenu = true;
                ControlPanel(LastScan);
                StartupClientControls();
            }
            else
            {
                LastScan = 0;
                ShutdownClientControls();
                ThreeDRefresh();
                QuickSaveGame();
                StartupClientControls();
            }
        }

        if (Keyboard[sc_F12] && !BATTLEMODE)
        {
            Keyboard[sc_F12] = false;
            LastScan = 0;
            DoBossKey();
        }

        // Gamma correction
        if (Keyboard[sc_F11])
        {
            char str[50] = "Gamma Correction Level ";
            char str2[10];

            gammaindex++;
            if (gammaindex == NUMGAMMALEVELS)
                gammaindex = 0;

            VL_SetPalette(origpal);
            itoa(gammaindex, str2, 10);
            strcat(str, str2);
            AddMessage(str, MSG_SYSTEM);

            while(Keyboard[sc_F11])
            {
                IN_UpdateKeyboard();
            }
        }

#if 0
        if ( Keyboard[ sc_M ] )
        {
            char str[ 50 ] = "Mouse Y-Rotation Input Scale ";
            char str2[ 10 ];

            if ( Keyboard[ sc_RShift ] )
                mouse_ry_input_scale += 50;
            else
                mouse_ry_input_scale -= 50;

            itoa(mouse_ry_input_scale,str2,10);
            strcat( str, str2 );
            AddMessage( str, MSG_SYSTEM );

        }
#endif

        // Increase volume
        if (Keyboard[sc_CloseBracket])
        {
            if (Keyboard[sc_RShift])
            {
                char str[50] = "Music Volume Level ";
                char str2[10];

                if (MUvolume < 255)
                {
                    MUvolume++;
                }
                MU_SetVolume(MUvolume);

                itoa(MUvolume, str2, 10);
                strcat(str, str2);
                AddMessage(str, MSG_SYSTEM);
            }
            else
            {
                char str[50] = "Sound FX Volume Level ";
                char str2[10];

                if (FXvolume < 255)
                {
                    FXvolume++;
                }
                FX_SetVolume(FXvolume);

                itoa(FXvolume, str2, 10);
                strcat(str, str2);
                AddMessage(str, MSG_SYSTEM);
            }
        }

        // Decrease volume
        if (Keyboard[sc_OpenBracket])
        {
            if (Keyboard[sc_RShift])
            {
                char str[50] = "Music Volume Level ";
                char str2[10];

                if (MUvolume > 0)
                {
                    MUvolume--;
                }
                MU_SetVolume(MUvolume);

                itoa(MUvolume, str2, 10);
                strcat(str, str2);
                AddMessage(str, MSG_SYSTEM);
            }
            else
            {
                char str[50] = "Sound FX Volume Level ";
                char str2[10];

                if (FXvolume > 0)
                {
                    FXvolume--;
                }
                FX_SetVolume(FXvolume);

                itoa(FXvolume, str2, 10);
                strcat(str, str2);
                AddMessage(str, MSG_SYSTEM);
            }
        }

#if (DEVELOPMENT == 1)
        if ( Keyboard[ sc_CapsLock ] && Keyboard[ sc_V ] )
        {
            SaveScreen( false );
        }
#endif
        else if ( Keyboard[ sc_Alt] && Keyboard[ sc_C ] )
        {
            SaveScreen( false );
        }

    }

    /* SDL doesn't send proper release events for these */
    if (Keystate[sc_CapsLock])
    {
        Keystate[sc_CapsLock]++;
        if (Keystate[sc_CapsLock] == 3)
            Keystate[sc_CapsLock] = 0;
    }
    if (Keystate[0x45]) /* numlock */
    {
        Keystate[0x45]++;
        if (Keystate[0x45] == 3)
            Keystate[0x45] = 0;
    }
}


//******************************************************************************
//
// CheckDevelopmentKeys ()
//
//******************************************************************************
void CheckDevelopmentKeys
(
    void
)

{
    if ( Keyboard[ sc_CapsLock ] && Keyboard[ sc_T ] )
    {
        if ( warp == true )
        {
            player->x     = warpx;
            player->y     = warpy;
            player->angle = warpa;
            locplayerstate->anglefrac = warpa << ANGLEBITS;
            player->momentumx = 0;
            player->momentumy = 0;
            player->momentumz = 0;
        }
        return;
    }
    // Lower wall height
    if ( Keyboard[ sc_5 ] )
    {
        if ( levelheight > 1 )
        {
            levelheight--;
        }

        while( Keyboard[ sc_5 ] )
        {
            IN_UpdateKeyboard ();
        }

        maxheight = ( levelheight << 6 ) - 32;
        nominalheight = maxheight - 32;
    }

    // Raise wall height
    if ( Keyboard[ sc_6 ] )
    {
        levelheight++;

        while( Keyboard[ sc_6 ] )
        {
            IN_UpdateKeyboard();
        }

        maxheight = ( levelheight << 6 ) - 32;
        nominalheight = maxheight - 32;
    }

    if ( Keyboard[ sc_8 ] )
    {
        char str[ 50 ] = "You are now player ";
        char str2[ 10 ];

        locplayerstate->player++;
        if ( locplayerstate->player == 5 )
        {
            locplayerstate->player = 0;
        }

        while( Keyboard[ sc_8 ] )
        {
            IN_UpdateKeyboard ();
        }

        itoa( locplayerstate->player, str2, 10 );
        strcat( str, str2 );
        AddMessage( str, MSG_SYSTEM );
    }

    // Cycle forward through wall textures
    if (Keyboard[sc_W])
    {
        int i,j;

        for(i=0; i<128; i++)
            for(j=0; j<128; j++)
            {
                if (IsWall(i,j))
                {
                    if (tilemap[i][j] ==
                            (W_GetNumForName("WALLSTOP")-W_GetNumForName("WALLSTRT")-1))
                        tilemap[i][j] = 1;
                    else
                        tilemap[i][j] ++;
                }
            }
        while(Keyboard[sc_W])
            IN_UpdateKeyboard ();

    }



    if (Keyboard[sc_Q])
    {
        int i,j;

        for(i=0; i<128; i++)
            for(j=0; j<128; j++)
            {
                if (IsWall(i,j))
                {
                    if (tilemap[i][j] == 1)
                        tilemap[i][j] = 74;
                    else
                        tilemap[i][j] --;
                }
            }
        while(Keyboard[sc_Q])
            IN_UpdateKeyboard ();

    }

    // Step through cieling/skies
    if ( Keyboard[ sc_K ] )
    {
        if ( sky > 0 )
        {
            MAPSPOT( 1, 0, 0 )++;
            if ( MAPSPOT( 1, 0, 0 ) > 239 )
            {
                MAPSPOT( 1, 0, 0 ) = 234;
            }
        }
        else
        {
            MAPSPOT( 1, 0, 0 )++;
            if ( MAPSPOT( 1, 0, 0 ) > 198 + 15 )
            {
                MAPSPOT( 1, 0, 0 ) = 198;
            }
        }

        SetPlaneViewSize();

        while( Keyboard[ sc_K ] )
        {
            IN_UpdateKeyboard();
        }
    }

    // Step through floors
    if ( Keyboard[ sc_L ] )
    {
        MAPSPOT( 0, 0, 0 )++;
        if ( MAPSPOT( 0, 0, 0 ) > 180 + 15 )
        {
            MAPSPOT( 0, 0, 0 ) = 180;
            SetPlaneViewSize();

            while( Keyboard[ sc_L ] )
            {
                IN_UpdateKeyboard();
            }
        }
    }

    // Increase darkness level
    if ( Keyboard[ sc_M ] )
    {
        if ( darknesslevel < 7 )
        {
            darknesslevel++;
        }

        SetLightLevels( darknesslevel );

        while( Keyboard[ sc_M ] )
        {
            IN_UpdateKeyboard();
        }
    }

    // Decrease darkness level
    if ( Keyboard[ sc_N ] )
    {
        if ( darknesslevel > 0 )
        {
            darknesslevel--;
        }

        SetLightLevels( darknesslevel );

        while( Keyboard[ sc_N ] )
        {
            IN_UpdateKeyboard();
        }
    }

    // Increase light rate
    if ( Keyboard[ sc_B ] )
    {
        SetLightRate( GetLightRate() + 1 );
        myprintf( "normalshade = %ld\n", normalshade );

        while( Keyboard[ sc_B ] )
        {
            IN_UpdateKeyboard();
        }
    }

    // Decrease light rate
    if ( Keyboard[ sc_V ] )
    {
        SetLightRate( GetLightRate() - 1 );
        myprintf( "normalshade = %ld\n", normalshade );

        while( Keyboard[ sc_V ] )
        {
            IN_UpdateKeyboard();
        }
    }

    // Toggle light diminishing on/off
    if ( Keyboard[ sc_T ] )
    {
        fulllight ^= 1;

        while( Keyboard[ sc_T ] )
        {
            IN_UpdateKeyboard();
        }
    }
}



short   BigShort (short l)
{
    byte    b1,b2;

    b1 = l&255;
    b2 = (l>>8)&255;

    return (b1<<8) + b2;
}

long    BigLong (long l)
{
    byte    b1,b2,b3,b4;

    b1 = l&255;
    b2 = (l>>8)&255;
    b3 = (l>>16)&255;
    b4 = (l>>24)&255;

    return ((long)b1<<24) + ((long)b2<<16) + ((long)b3<<8) + b4;
}

/*
==============
=
= WriteLBMfile
=
==============
*/

void WriteLBMfile (char *filename, byte *data, int width, int height)
{
    byte    *lbm, *lbmptr;
    long    *formlength, *bmhdlength, *cmaplength, *bodylength;
    long    length;
    bmhd_t  basebmhd;
    int     handle;
    int     i;

    lbm = lbmptr = (byte *) SafeMalloc ((screen_src_width*screen_src_height)+4000);

//
// start FORM
//
    *lbmptr++ = 'F';
    *lbmptr++ = 'O';
    *lbmptr++ = 'R';
    *lbmptr++ = 'M';

    formlength = (long *)lbmptr;
    lbmptr+=4;                      // leave space for length

    *lbmptr++ = 'P';
    *lbmptr++ = 'B';
    *lbmptr++ = 'M';
    *lbmptr++ = ' ';

//
// write BMHD
//
    *lbmptr++ = 'B';
    *lbmptr++ = 'M';
    *lbmptr++ = 'H';
    *lbmptr++ = 'D';

    bmhdlength = (long *)lbmptr;
    lbmptr+=4;                      // leave space for length

    memset (&basebmhd,0,sizeof(basebmhd));
    basebmhd.w = BigShort(width);
    basebmhd.h = BigShort(height);
    basebmhd.nPlanes = BigShort(8);
    basebmhd.xAspect = BigShort(5);
    basebmhd.yAspect = BigShort(6);
    basebmhd.pageWidth = BigShort(width);
    basebmhd.pageHeight = BigShort(height);

    memcpy (lbmptr,&basebmhd,sizeof(basebmhd));
    lbmptr += sizeof(basebmhd);

    length = lbmptr-(byte *)bmhdlength-4;
    *bmhdlength = BigLong(length);
    if (length&1)
        *lbmptr++ = 0;          // pad chunk to even offset

//
// write CMAP
//
    *lbmptr++ = 'C';
    *lbmptr++ = 'M';
    *lbmptr++ = 'A';
    *lbmptr++ = 'P';

    cmaplength = (long *)lbmptr;
    lbmptr+=4;                      // leave space for length

    for (i = 0; i < 0x300; i++)
        *lbmptr++ = (*(origpal+i))<<2;

// memcpy (lbmptr,&origpal[0],768);
// lbmptr += 768;

    length = lbmptr-(byte *)cmaplength-4;
    *cmaplength = BigLong(length);
    if (length&1)
        *lbmptr++ = 0;          // pad chunk to even offset

//
// write BODY
//
    *lbmptr++ = 'B';
    *lbmptr++ = 'O';
    *lbmptr++ = 'D';
    *lbmptr++ = 'Y';

    bodylength = (long *)lbmptr;
    lbmptr+=4;                      // leave space for length

    memcpy (lbmptr,data,width*height);
    lbmptr += width*height;

    length = lbmptr-(byte *)bodylength-4;
    *bodylength = BigLong(length);
    if (length&1)
        *lbmptr++ = 0;          // pad chunk to even offset

//
// done
//
    length = lbmptr-(byte *)formlength-4;
    *formlength = BigLong(length);
    if (length&1)
        *lbmptr++ = 0;          // pad chunk to even offset

//
// write output file
//
    handle = SafeOpenWrite (filename);

    SafeWrite (handle, lbm, lbmptr-lbm);

    close (handle);
    SafeFree(lbm);
}


//****************************************************************************
//
// GetFileName ()
//
//****************************************************************************

void GetFileName (boolean saveLBM)
{
    int i;

    for (i = 0; i < 9999; i++)
    {
        char filename[128];

        if (saveLBM)
        {
            sprintf(savename, "rott%04d.lbm", i);
        }
        else
        {
            sprintf(savename, "rott%04d.pcx", i);
        }

        GetPathFromEnvironment( filename, ApogeePath, savename );

        if (access(filename, F_OK) != 0)
        {
            return;
        }
    }
}

//****************************************************************************
//
// SaveScreen ()
//
//****************************************************************************

boolean inhmenu;

#if (BETA == 1)
#define SSX (160-(46*2))
#define SSY (17)
#endif
void SaveScreen (boolean saveLBM)
{
    byte *buffer;
    byte *screen;
    boolean oldHUD;
    char filename[ 128 ];

#if (BETA == 1)
    unsigned tmp;
    char buf[30];
    int i;
#endif


    oldHUD=HUD;
    HUD=false;
    doublestep=0;
    if (inhmenu==false)
        screen = (byte *) bufferofs;
    else
        screen = (byte *) displayofs;

    if (inhmenu==false)
        ThreeDRefresh ();
    doublestep = 2 - DetailLevel;

    //buffer = (byte *) SafeMalloc (65000);
    buffer = (byte *) SafeMalloc ((screen_src_height*screen_src_width)+4000);

#if (BETA == 1)
    if (SCREENSHOTS == false)
    {
        if (screen!=(byte *)bufferofs)
        {
            tmp=bufferofs;
            bufferofs=displayofs;
        }
        CurrentFont=tinyfont;

        for (i=-1; i<6; i++)
            memset((byte *)bufferofs+(ylookup[i+SSY])+(SSX>>2),0,46);
        px=SSX;
        py=SSY;
        VW_DrawPropString(" Rise of the Triad (c) 1995 Apogee  Version ");
        VW_DrawPropString(itoa(ROTTMAJORVERSION,&buf[0],10));
        VW_DrawPropString(".");
        VW_DrawPropString(itoa(ROTTMINORVERSION,&buf[0],10));
        px=SSX+13;
        py=SSY+8;
        VW_DrawPropString(" Episode ");
        VW_DrawPropString(itoa(gamestate.episode,&buf[0],10));
        VW_DrawPropString(" Area ");
        VW_DrawPropString(itoa(GetLevel(gamestate.episode, gamestate.mapon),&buf[0],10));

        if (screen!=(byte *)bufferofs)
            bufferofs=tmp;
    }
#endif



    GetFileName (saveLBM);
    GetPathFromEnvironment( filename, ApogeePath, savename );
    //
    memcpy(buffer,screen , screen_src_width*screen_src_height);//bna
    //bna--VL_CopyPlanarPageToMemory(screen,buffer);

    if (saveLBM)
    {
        WriteLBMfile (filename, buffer, screen_src_width, screen_src_height);
#if (DEVELOPMENT == 1)
        while (Keyboard[sc_CapsLock] && Keyboard[sc_C])
#else
        while (Keyboard[sc_Alt] && Keyboard[sc_V])
#endif
            IN_UpdateKeyboard ();
    }
    else
    {
        WritePCX (filename, buffer);
#if (DEVELOPMENT == 1)
        while (Keyboard[sc_CapsLock] && Keyboard[sc_X])
#else
        while (Keyboard[sc_Alt] && Keyboard[sc_C])
#endif
            IN_UpdateKeyboard ();
    }

    SafeFree(buffer);
    HUD=oldHUD;
}

//****************************************************************************
//
// WritePCX ()
//
//****************************************************************************

void WritePCX (char *file, byte *source)
{
    PCX_HEADER pcxHDR;
    byte *tempbuffer;
    byte pal[0x300];
    int pcxhandle;
    int i, j, y;
    unsigned char c;
    unsigned char buffer1[GAP_SIZE];

    pcxhandle = SafeOpenWrite (file);

    /* --- init the header that we'll write.
     *    Note: since DPaint never reads & writes at the same time,
     *    it is okay to share the same read & write structure,
     *    unlike in CONVERT.EXE.
     */

    memset (&pcxHDR, 0, sizeof(PCX_HEADER));

    pcxHDR.manufacturer  = 10;
    pcxHDR.version       = 5;
    pcxHDR.encoding      = 1;

    pcxHDR.bitsperpixel  = 8;           //bpp;
    pcxHDR.xmin          = pcxHDR.ymin = 0;
    pcxHDR.xmax          = screen_src_width - 1;
    pcxHDR.ymax          = screen_src_height - 1;
    pcxHDR.hres          = screen_src_width;         //N_COLUMNS;
    pcxHDR.vres          = screen_src_height;         //N_LINES;

    // bytesperline doesn't take into account multiple planes.
    // Output in same format as bitmap (planar vs packed).
    //
    pcxHDR.bytesperline  = screen_src_width;         //bitmap->width;

    pcxHDR.nplanes       = 1;           //bitmap->planes;
    pcxHDR.reserved      = 0;

    // First 16 colors of our palette info.
    for (i = 0, j = 0; i < 16; ++i, j += 3)
    {
        pcxHDR.colormap[i][0] = (unsigned char)(origpal[j]);
        pcxHDR.colormap[i][1] = (unsigned char)(origpal[j]+2);
        pcxHDR.colormap[i][2] = (unsigned char)(origpal[j]+3);
    }

    //
    // Write the 128-byte header
    //
    SafeWrite(pcxhandle,&pcxHDR, sizeof (PCX_HEADER));

    memset (buffer1, 0, GAP_SIZE);

    SafeWrite (pcxhandle, &buffer1, GAP_SIZE);

    tempbuffer = (byte *) SafeMalloc ((screen_src_height*screen_src_width)+4000);
    bptr = tempbuffer;
    totalbytes = 0;

    //
    // Write to a bit-packed file.
    //
    for (y = 0;  y < screen_src_height;  ++y)       // for each line in band
        if (PutBytes (((unsigned char *) (source+(y*screen_src_width))),
                      pcxHDR.bytesperline))
            Error ("Error writing PCX bit-packed line!\n");

    SafeWrite (pcxhandle, tempbuffer, totalbytes);

    //
    // Write out PCX palette
    //
    c = 0x0C;

    for (i = 0; i < 0x300; i++)
        pal[i] = (*(origpal+i))<<2;

    SafeWrite (pcxhandle, &c, 1);
    SafeWrite (pcxhandle, &pal[0], 768);

    close (pcxhandle);
    SafeFree (tempbuffer);
}


//****************************************************************************
//
// PutBytes ()
//
// Write bytes to a file, handling packing as it goes.
// Returns :  0 == SUCCESS
//            1 == FAIL.
//
//****************************************************************************

int PutBytes (unsigned char *ptr, unsigned int bytes)
{
    unsigned int startbyte, count;
    char b;

    while (bytes > 0)
    {
        // check for a repeating byte value
        startbyte = *ptr;
        *ptr++ = 0;
        --bytes;
        count = 1;
        while (*ptr == startbyte && bytes > 0 && count < 63)
        {
            *ptr = 0;
            ++ptr;
            --bytes;
            ++count;
        }
        // If we can pack the sequence, or if we have to add a
        //    byte before it because the top 2 bits of the value
        //    are 1's, write a packed sequence of 2 bytes.
        //    Otherwise, just write the byte value.
        //
        if (count > 1  ||  (startbyte & 0xc0) == 0xc0)
        {
            b = 0xc0 | count;

            *bptr++ = b;
            totalbytes++;
        }
        b = startbyte;

        *bptr++ = b;
        totalbytes++;
    }
    return (0);
}


//****************************************************************************
//
// PlayCinematic () - Play intro cinematics
//
//****************************************************************************

void PlayCinematic (void)
{
    byte pal[768];
    int cinY;

    if (turbo)
        return;

    switch (gamestate.mapon)
    {
            // Start of EPISODE 1
        case 0:
            MU_StartSong(song_cinematic1);
            VL_FadeOut(0, 255, 0, 0, 0, 20);
            VL_ClearBuffer(bufferofs, 0);

            // Ultra-widescreen cinematics = not good.
            if (screen_src_width > (screen_dest_height * 2.0))
            {
                DrawNormalSprite(0, 30, W_GetNumForName("nicolas"));
            }
            else
            {
                cinY = (screen_src_height - (120 * screen_src_maxscale)) * 0.375;
                DrawNormalSpriteCinematic(cinY, W_GetNumForName("nicolas"));
            }

            DrawNormalSprite(0, 200 - 32, W_GetNumForName("oneyear"));
            FlipPage();
            memcpy(&pal[0], W_CacheLumpName("nicpal", PU_CACHE, CvtNull, 1), 768);
            VL_NormalizePalette(&pal[0]);
            VL_FadeIn(0, 255, pal, 20);
            I_Delay(60);
            VL_FadeOut(0, 255, 0, 0, 0, 20);

            IN_UpdateKeyboard();
            if (LastScan != 0)
            {
                LastScan = 0;
                return;
            }


            if (screen_src_width > (screen_src_height * 1.2) * 1.78)
                cinY = 20 * screen_src_minscale;
            else
                cinY = (screen_src_height - (120 * screen_src_maxscale)) * 0.25;

            SD_PlayPitchedSound(SD_LIGHTNINGSND, 255, -1500);
            DoInBetweenCinematic(cinY, W_GetNumForName("binoculr"), 80, false,
                                 "The HUNT cases an\n"
                                 "ancient monastery.");

            IN_UpdateKeyboard();
            if (LastScan != 0)
            {
                LastScan = 0;
                return;
            }

            SD_Play(SD_NMESEESND);
            DoInBetweenCinematic(cinY, W_GetNumForName("binosee"), 80, false,
                                 "\"There they are,\" says\n"
                                 "Cassatt. \"Let's get back\n"
                                 "to the boat and inform HQ.\"");

            IN_UpdateKeyboard();
            if (LastScan != 0)
            {
                LastScan = 0;
                return;
            }

            SD_Play(SD_HIGHGUARD1SEESND);
            DoInBetweenCinematic(cinY, W_GetNumForName("boatgard"), 80, false,
                                 "\"The intruders, on that hill!\"");

            IN_UpdateKeyboard();
            if (LastScan != 0)
            {
                LastScan = 0;
                return;
            }

            SD_Play(SD_EXPLODESND);
            DoInBetweenCinematic(cinY, W_GetNumForName("boatblow"), 80, false,
                                 "\"There goes our ride home,\"\n"
                                 "says Barrett.  \"Looks like\n"
                                 "the only way out is in....\"");

            IN_UpdateKeyboard();
            LastScan = 0;
            break;

            // Start of EPISODE 2
        case 8:
            MU_StartSong(song_cinematic2);
            DoInBetweenCinematic(0, W_GetNumForName("epi12"), 1200, true,
                                 "The HUNT makes their way\n"
                                 "into the main keep.");

            IN_UpdateKeyboard();
            LastScan = 0;
            break;

            // Start of EPISODE 3
        case 16:
            MU_StartSong(song_cinematic1);

            if (screen_src_width > (screen_dest_height * 2.0))
                cinY = 20 * screen_src_minscale;
            else
                cinY = (screen_src_height - (120 * screen_src_maxscale)) * 0.25;

            DoInBetweenCinematic(cinY, W_GetNumForName("epi23"), 1200, false,
                                 "The HUNT stands before a pair\n"
                                 "of ominous wooden doors.\n"
                                 "The sounds of machinery and\n"
                                 "servomotors fill the air.\n");

            IN_UpdateKeyboard();
            LastScan = 0;
            break;
            // Start of EPISODE 4
        case 24:
            MU_StartSong(song_cinematic2);
            DoInBetweenCinematic(0, W_GetNumForName("epi34"), 1200, true,
                                 "Stairs lead down beneath the\n"
                                 "keep.  From behind the doors\n"
                                 "come the moans of the undead.");

            IN_UpdateKeyboard();
            LastScan = 0;
            break;
    }
}
