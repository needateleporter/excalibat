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
/**********************************************************************
   module: MUSIC.C

   author: James R. Dose
   date:   March 25, 1994

   Device independant music playback routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_mixer.h"

#include "rt_def.h"
#include "rt_cfg.h"
#include "rt_util.h"

#include "au_music.h"

static int music_usetempmid = 0;
#define TEMPMID "tmpsong.mid"

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )

int MUSIC_ErrorCode = MUSIC_Ok;

#define MUSIC_SetErrorCode( status ) \
    MUSIC_ErrorCode = ( status );

static int music_oggenabled = false;
static int music_initialized = 0;
static int music_loopflag = MUSIC_PlayOnce;
static Mix_Music *music_musicchunk = NULL;
static int music_pausevolume = -1;

extern char ApogeePath[256];

/*---------------------------------------------------------------------
   Function: MUSIC_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *MUSIC_ErrorString (int ErrorNumber)
{
    char *ErrorString;

    switch(ErrorNumber)
    {
        case MUSIC_Warning :
        case MUSIC_Error :
            ErrorString = MUSIC_ErrorString(MUSIC_ErrorCode);
            break;

        case MUSIC_Ok :
            ErrorString = "Music ok.";
            break;

        default :
            ErrorString = "Unknown Music error code.";
            break;
    }

    return ErrorString;
}


/*---------------------------------------------------------------------
   Function: MUSIC_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int MUSIC_Init (void)
{
    if (music_initialized)
        return MUSIC_Error;

    if (!Mix_QuerySpec(NULL, NULL, NULL))
    {
        // Init if FX is off?
    }

    if(Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG)
        music_oggenabled = true;

    music_initialized = 1;

    return MUSIC_Ok;
}


/*---------------------------------------------------------------------
   Function: MUSIC_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int MUSIC_Shutdown (void)
{
    if (!music_initialized)
        return MUSIC_Error;

    MUSIC_StopSong();

    music_initialized = 0;
    music_loopflag = MUSIC_PlayOnce;

    return MUSIC_Ok;
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetVolume

   Sets the volume of music playback.
---------------------------------------------------------------------*/

void MUSIC_SetVolume (int volume)
{
    if (music_pausevolume == -1)
        Mix_VolumeMusic(volume >> 1);  // convert 0-255 to 0-128.
    else
        music_pausevolume = volume >> 1;
}


/*---------------------------------------------------------------------
   Function: MUSIC_GetVolume

   Returns the volume of music playback.
---------------------------------------------------------------------*/

int MUSIC_GetVolume (void)
{
    return(Mix_VolumeMusic(-1) << 1);  // convert 0-128 to 0-255.
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetLoopFlag

   Set whether the music will loop or end when it reaches the end of
   the song.
---------------------------------------------------------------------*/

void MUSIC_SetLoopFlag (int loopflag)
{
    music_loopflag = loopflag;
}


/*---------------------------------------------------------------------
   Function: MUSIC_SongPlaying

   Returns whether there is a song playing.
---------------------------------------------------------------------*/

int MUSIC_SongPlaying (void)
{
    return((Mix_PlayingMusic()) ? TRUE : FALSE);
}


/*---------------------------------------------------------------------
   Function: MUSIC_Continue

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MUSIC_Continue (void)
{
    // Mix_ResumeMusic or Mix_SetMusicPosition doesn't work with the
    // MIDI format. Shame.

    if (Mix_GetMusicType(music_musicchunk) != MUS_MID)
    {
        Mix_ResumeMusic();
    }
    else if (music_pausevolume != -1)
    {
        Mix_VolumeMusic(music_pausevolume);
        music_pausevolume = -1;
    }
}


/*---------------------------------------------------------------------
   Function: MUSIC_Pause

   Pauses playback of a song.
---------------------------------------------------------------------*/

void MUSIC_Pause (void)
{

    // Mix_PauseMusic or Mix_SetMusicPosition doesn't work with the
    // MIDI format. Shame.
    // Let's just set the volume to 0 and let it play out.

    if (Mix_GetMusicType(music_musicchunk) != MUS_MID)
    {
        Mix_PauseMusic();
    }
    else if (music_pausevolume == -1)
    {
        music_pausevolume = Mix_VolumeMusic(-1);
        Mix_VolumeMusic(0);
    }
}


/*---------------------------------------------------------------------
   Function: MUSIC_StopSong

   Stops playback of current song.
---------------------------------------------------------------------*/

int MUSIC_StopSong (void)
{
    if (Mix_PlayingMusic() || Mix_PausedMusic())
        Mix_HaltMusic();

    if (music_musicchunk)
        Mix_FreeMusic(music_musicchunk);

    music_musicchunk = NULL;

    return(MUSIC_Ok);
}


/*---------------------------------------------------------------------
   Function: MUSIC_PlaySong

   Begins playback of MIDI song.
---------------------------------------------------------------------*/

int MUSIC_PlaySong (unsigned char *song, int size, int loopflag)
{
    MUSIC_StopSong();

    if (music_usetempmid)
    {
        char filename[MAX_PATH];
        int handle;

        // save the file somewhere, so SDL_mixer can load it
        GetPathFromEnvironment(filename, ApogeePath, TEMPMID);
        handle = SafeOpenWrite(filename);

        SafeWrite(handle, song, size);
        close(handle);

        // finally, we can load it with SDL_mixer
        music_musicchunk = Mix_LoadMUS(filename);
    }
    else
    {
        SDL_RWops *rw;
        rw = SDL_RWFromMem((void *) song, size);
        music_musicchunk = Mix_LoadMUS_RW(rw, 1);
    }

    if (music_musicchunk == NULL)
        return MUSIC_Error;

    Mix_PlayMusic(music_musicchunk, (loopflag == MUSIC_PlayOnce) ? 0 : -1);

    MUSIC_Continue();

    return MUSIC_Ok;
}

/*---------------------------------------------------------------------
   Function: MUSIC_PlaySongFromFile

   Begins playback of song from external file.
---------------------------------------------------------------------*/

int MUSIC_PlaySongFromExternalFile (char *songname, int loopflag)
{
    char filename[MAX_PATH];

    MUSIC_StopSong();

    //if (!music_oggenabled)
    //    return MUSIC_Ok;

    // save the file somewhere, so SDL_mixer can load it
    GetPathFromEnvironment(filename, ApogeePath, "test.ogg");
    music_musicchunk = Mix_LoadMUS(filename);

    if (music_musicchunk == NULL)
        return MUSIC_Error;

    Mix_PlayMusic(music_musicchunk, (loopflag == MUSIC_PlayOnce) ? 0 : -1);

    MUSIC_Continue();

    return MUSIC_Ok;
}


/*---------------------------------------------------------------------
   Function: MUSIC_SetSongTime

   Sets the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_SetSongTime (unsigned long milliseconds)
{
}


/*---------------------------------------------------------------------
   Function: MUSIC_GetSongPosition

   Returns the position of the song pointer.
---------------------------------------------------------------------*/

void MUSIC_GetSongPosition (songposition *pos)
{
}


/*---------------------------------------------------------------------
   Function: MUSIC_FadeOut

   Fades music volume from current level to another over a specified
   period of time.
---------------------------------------------------------------------*/

int MUSIC_FadeOut (int milliseconds)
{
    Mix_FadeOutMusic(milliseconds);

    return MUSIC_Ok;
}


/*---------------------------------------------------------------------
   Function: MUSIC_FadeActive

   Returns whether the fade routine is active.
---------------------------------------------------------------------*/

int MUSIC_FadeActive (void)
{
    return((Mix_FadingMusic() != MIX_NO_FADING) ? TRUE : FALSE);
}
