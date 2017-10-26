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

#include "SDL2/SDL.h"

#include "rt_main.h"
#include "rt_def.h"
#include "rt_in.h"
#include "_rt_in.h"
#include "isr.h"
#include "rt_util.h"
#include "rt_vh_a.h"
#include "rt_cfg.h"
#include "rt_msg.h"
#include "rt_playr.h"
#include "rt_net.h"
#include "rt_com.h"
#include "rt_cfg.h"
#include "rt_menu.h"

#include "keyb.h"

#define MAXMESSAGELENGTH      (COM_MAXTEXTSTRINGLENGTH-1)

//****************************************************************************
//
// GLOBALS
//
//****************************************************************************]

//
// Used by menu routines that need to wait for a button release.
// Sometimes the mouse driver misses an interrupt, so you can't wait for
// a button to be released.  Instead, you must ignore any buttons that
// are pressed.
//
int IgnoreMouse = 0;

// configuration variables
//
boolean  MousePresent;
boolean  JoysPresent[MaxJoys];
boolean  JoyPadPresent     = 0;

//    Global variables
//
boolean  Paused;
char LastASCII;
volatile int LastScan;

byte Joy_xb,
     Joy_yb,
     Joy_xs,
     Joy_ys;
word Joy_x,
     Joy_y;


int LastLetter = 0;
char LetterQueue[MAXLETTERS];
ModemMessage MSG;

static SDL_Joystick *sdl_joysticks[MaxJoys];
static int sdl_mouse_delta_x = 0;
static int sdl_mouse_delta_y = 0;
static word sdl_mouse_button_mask = 0;
static int sdl_total_sticks = 0;
static word *sdl_stick_button_state = NULL;
static word sdl_sticks_joybits = 0;
int sdl_mouse_grabbed = SDL_FALSE;
boolean sdl_force_mouse_ungrab = false;
boolean sdl_window_focused = false;
static unsigned int scancodes[SDL_NUM_SCANCODES];
extern boolean screen_fullscreen;

extern int MenuNum;

const char ScanChars[128] =    // Scan code names with single chars
{
    0 , 0 ,'1','2','3','4','5','6','7','8','9','0','-','=', 0 , 0 ,
    'q','w','e','r','t','y','u','i','o','p','[',']', 0 , 0 ,'a','s',
    'd','f','g','h','j','k','l',';','\'','`', 0 ,'\\','z','x','c','v',
    'b','n','m',',','.','/', 0 , 0 , 0 ,' ', 0 , 0 , 0 , 0 , 0 , 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,'-', 0 ,'5', 0 ,'+', 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0
};

const char ShiftedScanChars[128] =    // Shifted Scan code names with single chars
{
    0 , 0 ,'!','@','#','$','%','^','&','*','(',')','_','+', 0 , 0 ,
    'Q','W','E','R','T','Y','U','I','O','P','{','}', 0 , 0 ,'A','S',
    'D','F','G','H','J','K','L',':','"','~', 0 ,'|','Z','X','C','V',
    'B','N','M','<','>','?', 0 , 0 , 0 ,' ', 0 , 0 , 0 , 0 , 0 , 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,'-', 0 ,'5', 0 ,'+', 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
    0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0
};



//****************************************************************************
//
// LOCALS
//
//****************************************************************************]

static KeyboardDef KbdDefs = {0x1d,0x38,0x47,0x48,0x49,0x4b,0x4d,0x4f,0x50,0x51};
static JoystickDef JoyDefs[MaxJoys];
static ControlType Controls[MAXPLAYERS];


static boolean  IN_Started;

static   Direction   DirTable[] =      // Quick lookup for total direction
{
    dir_NorthWest, dir_North,  dir_NorthEast,
    dir_West,      dir_None,   dir_East,
    dir_SouthWest, dir_South,  dir_SouthEast
};

int (far *function_ptr)();

static char *ParmStrings[] = {"nojoys","nomouse",NULL};


#define sdldebug printf

static int sdl_mouse_button_filter(SDL_Event const *event)
{
    /*
     * What DOS games expect:
     *  0   left button pressed if 1
     *  1   right button pressed if 1
     *  2   middle button pressed if 1
     *
     *   (That is, this is what Int 33h (AX=0x05) returns...)
     */

    Uint8 bmask = SDL_GetMouseState(NULL, NULL);
    sdl_mouse_button_mask = 0;  /* this is a static var. */
    if (bmask & SDL_BUTTON_LMASK) sdl_mouse_button_mask |= 1;
    if (bmask & SDL_BUTTON_RMASK) sdl_mouse_button_mask |= 2;
    if (bmask & SDL_BUTTON_MMASK) sdl_mouse_button_mask |= 4;
    return(0);
} /* sdl_mouse_up_filter */


static int sdl_mouse_motion_filter(SDL_Event const *event)
{
    static int mouse_x = 0;
    static int mouse_y = 0;
    int mouse_relative_x = 0;
    int mouse_relative_y = 0;

    if (event->type == SDL_JOYBALLMOTION)
    {
        mouse_relative_x = event->jball.xrel/100;
        mouse_relative_y = event->jball.yrel/100;
        mouse_x += mouse_relative_x;
        mouse_y += mouse_relative_y;
    } /* if */
    else
    {
        if (sdl_mouse_grabbed || screen_fullscreen)
        {
            mouse_relative_x = event->motion.xrel;
            mouse_relative_y = event->motion.yrel;
            mouse_x += mouse_relative_x;
            mouse_y += mouse_relative_y;
        } /* if */
        else
        {
            mouse_relative_x = event->motion.x - mouse_x;
            mouse_relative_y = event->motion.y - mouse_y;
            mouse_x = event->motion.x;
            mouse_y = event->motion.y;
        } /* else */
    } /* else */

#if 0
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x > surface->w) mouse_x = surface->w;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y > surface->h) mouse_y = surface->h;
#endif

    /* set static vars... */
    sdl_mouse_delta_x += mouse_relative_x;
    sdl_mouse_delta_y += mouse_relative_y;

    return(0);
} /* sdl_mouse_motion_filter */

/*
 * The windib driver can't alert us to the keypad enter key, which
 *  Ken's code depends on heavily. It sends it as the same key as the
 *  regular return key. These users will have to hit SHIFT-ENTER,
 *  which we check for explicitly, and give the engine a keypad enter
 *  enter event.
 */
static int handle_keypad_enter_hack(const SDL_Event *event)
{
    static int kp_enter_hack = 0;
    int retval = 0;

    if (event->key.keysym.scancode == SDL_SCANCODE_RETURN)
    {
        if (event->key.state == SDL_PRESSED)
        {
            if (event->key.keysym.mod & KMOD_SHIFT)
            {
                kp_enter_hack = 1;
                retval = scancodes[SDL_SCANCODE_KP_ENTER];
            } /* if */
        } /* if */

        else  /* key released */
        {
            if (kp_enter_hack)
            {
                kp_enter_hack = 0;
                retval = scancodes[SDL_SCANCODE_KP_ENTER];
            } /* if */
        } /* if */
    } /* if */

    return(retval);
} /* handle_keypad_enter_hack */


static int sdl_key_filter(const SDL_Event *event)
{
    int k;
    int keyon;
    int strippedkey;
    int extended;

    // CTRL+G = Force ungrab mouse.
    if ( (event->key.keysym.sym == SDLK_g) &&
            (event->key.state == SDL_PRESSED) &&
            (event->key.keysym.mod & KMOD_CTRL) )
    {
        sdl_force_mouse_ungrab ^= 1;
        sdl_mouse_grabbed = sdl_force_mouse_ungrab ? SDL_FALSE : SDL_TRUE;
        SDL_SetRelativeMouseMode(sdl_mouse_grabbed);
        return 0;
    }

    // ALT+ENTER = Toggle fullscreen.
    else if ( ( (event->key.keysym.sym == SDLK_RETURN) ||
                (event->key.keysym.sym == SDLK_KP_ENTER) ) &&
              (event->key.state == SDL_PRESSED) &&
              (event->key.keysym.mod & KMOD_ALT) )
    {
        screen_fullscreen ^= 1;
        SetWindowFullscreen();
        return 0;
    }

    /* HDG: put this above the scancode lookup otherwise it is never reached */
    // TODO: Fix this. Holding Pause triggers slowmo.
    if ((event->key.keysym.scancode == SDL_SCANCODE_PAUSE) &&
            (event->key.state == SDL_PRESSED))
    {
        PausePressed = true;
        return 0;
    }

    k = handle_keypad_enter_hack(event);

    if (!k)
    {
        k = scancodes[event->key.keysym.scancode];

        if (!k)   /* No DOS equivalent defined. */
            return 0;
    }

    /* Fix elweirdo SDL capslock/numlock handling, always treat as press */
    if ((event->key.keysym.scancode != SDL_SCANCODE_CAPSLOCK) &&
            (event->key.keysym.scancode != SDL_SCANCODE_NUMLOCKCLEAR) &&
            (event->key.state == SDL_RELEASED))
        k += 128;  /* +128 signifies that the key is released in DOS. */

    if (event->key.keysym.scancode == SDL_SCANCODE_SCROLLLOCK)
        PanicPressed = true;
    else
    {
        extended = ((k & 0xFF00) >> 8);

        keyon = k & 0x80;
        strippedkey = k & 0x7f;

        if (extended != 0)
        {
            KeyboardQueue[ Keytail ] = extended;
            Keytail = ( Keytail + 1 )&( KEYQMAX - 1 );
            k = scancodes[event->key.keysym.scancode] & 0xFF;
            if (event->key.state == SDL_RELEASED)
                k += 128;  /* +128 signifies that the key is released in DOS. */
        }

        if (keyon)        // Up event
            Keystate[strippedkey]=0;
        else                 // Down event
        {
            Keystate[strippedkey]=1;
            LastScan = k;
        }

        KeyboardQueue[ Keytail ] = k;
        Keytail = ( Keytail + 1 )&( KEYQMAX - 1 );
    }

    return 0;
}


void CheckSDLWindowGrabState(void)
{
    int grab_mouse = SDL_TRUE;

    // Never grab on unfocused windows
    // (unless the mouse is disabled or CTRL+G is triggered).
    if (!sdl_window_focused || !mouseenabled || sdl_force_mouse_ungrab)
        grab_mouse = SDL_FALSE;
    // Always grab on fullscreen.
    else if (screen_fullscreen)
        grab_mouse = SDL_TRUE;
    // Otherwise, never grab if the game is not playing.
    else if (playstate != ex_stillplaying || demoplayback || GamePaused || inmenu)
        grab_mouse = SDL_FALSE;

    if (sdl_mouse_grabbed != grab_mouse)
    {
        SDL_SetRelativeMouseMode(grab_mouse);
        sdl_mouse_grabbed = grab_mouse;
    }
}


static int root_sdl_event_filter(const SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            return(sdl_window_focused ? sdl_key_filter(event) : 1);
        case SDL_JOYBALLMOTION:
        case SDL_MOUSEMOTION:
            return(sdl_window_focused ? sdl_mouse_motion_filter(event) : 1);
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            return(sdl_window_focused ? sdl_mouse_button_filter(event) : 1);
        case SDL_QUIT:
            SDL_Quit();
            exit(42);
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                sdl_window_focused = true;
            else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                sdl_window_focused = false;
            break;
    }

    return 1;
}


static void sdl_handle_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        root_sdl_event_filter(&event);
}


//******************************************************************************
//
// IN_PumpEvents () - Let platform process an event queue.
//
//******************************************************************************
void IN_PumpEvents(void)
{
    sdl_handle_events();
}



//******************************************************************************
//
// INL_GetMouseDelta () - Gets the amount that the mouse has moved from the
//                        mouse driver
//
//******************************************************************************

void INL_GetMouseDelta(int *x,int *y)
{
    IN_PumpEvents();

    *x = sdl_mouse_delta_x;
    *y = sdl_mouse_delta_y;

    sdl_mouse_delta_x = sdl_mouse_delta_y = 0;
}



//******************************************************************************
//
// IN_GetMouseButtons () - Gets the status of the mouse buttons from the
//                         mouse driver
//
//******************************************************************************

word IN_GetMouseButtons
(
    void
)

{
    word buttons = 0;

    IN_PumpEvents();

    buttons = sdl_mouse_button_mask;

// Used by menu routines that need to wait for a button release.
// Sometimes the mouse driver misses an interrupt, so you can't wait for
// a button to be released.  Instead, you must ignore any buttons that
// are pressed.

    IgnoreMouse &= buttons;
    buttons &= ~IgnoreMouse;

    return (buttons);
}


//******************************************************************************
//
// IN_IgnoreMouseButtons () -
//    Tells the mouse to ignore the currently pressed buttons.
//
//******************************************************************************

void IN_IgnoreMouseButtons
(
    void
)

{
    IgnoreMouse |= IN_GetMouseButtons();
}


//******************************************************************************
//
// IN_GetJoyAbs () - Reads the absolute position of the specified joystick
//
//******************************************************************************

void IN_GetJoyAbs (word joy, word *xp, word *yp)
{
    Joy_x  = Joy_y = 0;
    Joy_xs = joy? 2 : 0;       // Find shift value for x axis
    Joy_xb = 1 << Joy_xs;      // Use shift value to get x bit mask
    Joy_ys = joy? 3 : 1;       // Do the same for y axis
    Joy_yb = 1 << Joy_ys;

    if (joy < sdl_total_sticks)
    {
        Joy_x = SDL_JoystickGetAxis (sdl_joysticks[joy], 0);
        Joy_y = SDL_JoystickGetAxis (sdl_joysticks[joy], 1);
    }
    else
    {
        Joy_x = 0;
        Joy_y = 0;
    }

    *xp = Joy_x;
    *yp = Joy_y;
}

void JoyStick_Vals (void)
{

}


//******************************************************************************
//
// INL_GetJoyDelta () - Returns the relative movement of the specified
//                     joystick (from +/-127)
//
//******************************************************************************

void INL_GetJoyDelta (word joy, int *dx, int *dy)
{
    word        x, y;
    JoystickDef *def;
    static longword lasttime;

    IN_GetJoyAbs (joy, &x, &y);
    def = JoyDefs + joy;

    if (x < def->threshMinX)
    {
        if (x < def->joyMinX)
            x = def->joyMinX;

        x = -(x - def->threshMinX);
        x *= def->joyMultXL;
        x >>= JoyScaleShift;
        *dx = (x > 127)? -127 : -x;
    }
    else if (x > def->threshMaxX)
    {
        if (x > def->joyMaxX)
            x = def->joyMaxX;

        x = x - def->threshMaxX;
        x *= def->joyMultXH;
        x >>= JoyScaleShift;
        *dx = (x > 127)? 127 : x;
    }
    else
        *dx = 0;

    if (y < def->threshMinY)
    {
        if (y < def->joyMinY)
            y = def->joyMinY;

        y = -(y - def->threshMinY);
        y *= def->joyMultYL;
        y >>= JoyScaleShift;
        *dy = (y > 127)? -127 : -y;
    }
    else if (y > def->threshMaxY)
    {
        if (y > def->joyMaxY)
            y = def->joyMaxY;

        y = y - def->threshMaxY;
        y *= def->joyMultYH;
        y >>= JoyScaleShift;
        *dy = (y > 127)? 127 : y;
    }
    else
        *dy = 0;

    lasttime = GetTicCount();
}



//******************************************************************************
//
// INL_GetJoyButtons () - Returns the button status of the specified
//                        joystick
//
//******************************************************************************

word INL_GetJoyButtons (word joy)
{
    word  result = 0;

    if (joy < sdl_total_sticks)
        result = sdl_stick_button_state[joy];

    return result;
}

//******************************************************************************
//
// INL_SetJoyScale () - Sets up scaling values for the specified joystick
//
//******************************************************************************

void INL_SetJoyScale (word joy)
{
    JoystickDef *def;

    def = &JoyDefs[joy];
    def->joyMultXL = JoyScaleMax / (def->threshMinX - def->joyMinX);
    def->joyMultXH = JoyScaleMax / (def->joyMaxX - def->threshMaxX);
    def->joyMultYL = JoyScaleMax / (def->threshMinY - def->joyMinY);
    def->joyMultYH = JoyScaleMax / (def->joyMaxY - def->threshMaxY);
}



//******************************************************************************
//
// IN_SetupJoy () - Sets up thresholding values and calls INL_SetJoyScale()
//                  to set up scaling values
//
//******************************************************************************

void IN_SetupJoy (word joy, word minx, word maxx, word miny, word maxy)
{
    word     d,r;
    JoystickDef *def;

    def = &JoyDefs[joy];

    def->joyMinX = minx;
    def->joyMaxX = maxx;
    r = maxx - minx;
    d = r / 3;
    def->threshMinX = ((r / 2) - d) + minx;
    def->threshMaxX = ((r / 2) + d) + minx;

    def->joyMinY = miny;
    def->joyMaxY = maxy;
    r = maxy - miny;
    d = r / 3;
    def->threshMinY = ((r / 2) - d) + miny;
    def->threshMaxY = ((r / 2) + d) + miny;

    INL_SetJoyScale (joy);
}


//******************************************************************************
//
// INL_StartJoy () - Detects & auto-configures the specified joystick
//                   The auto-config assumes the joystick is centered
//
//******************************************************************************


boolean INL_StartJoy (word joy)
{
    word x,y;

    if (!SDL_WasInit(SDL_INIT_JOYSTICK))
    {
        SDL_Init(SDL_INIT_JOYSTICK);
        sdl_total_sticks = SDL_NumJoysticks();
        if (sdl_total_sticks > MaxJoys) sdl_total_sticks = MaxJoys;

        if ((sdl_stick_button_state == NULL) && (sdl_total_sticks > 0))
        {
            sdl_stick_button_state = (word *) malloc(sizeof (word) * sdl_total_sticks);
            if (sdl_stick_button_state == NULL)
                SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
            else
                memset(sdl_stick_button_state, '\0', sizeof (word) * sdl_total_sticks);
        }
        SDL_JoystickEventState(SDL_ENABLE);
    }

    if (joy >= sdl_total_sticks) return (false);
    sdl_joysticks[joy] = SDL_JoystickOpen (joy);

    IN_GetJoyAbs (joy, &x, &y);

    if
    (
        ((x == 0) || (x > MaxJoyValue - 10))
        || ((y == 0) || (y > MaxJoyValue - 10))
    )
        return(false);
    else
    {
        IN_SetupJoy (joy, 0, x * 2, 0, y * 2);
        return (true);
    }
}



//******************************************************************************
//
// INL_ShutJoy() - Cleans up the joystick stuff
//
//******************************************************************************

void INL_ShutJoy (word joy)
{
    JoysPresent[joy] = false;

    if (joy < sdl_total_sticks) SDL_JoystickClose (sdl_joysticks[joy]);
}



//******************************************************************************
//
// IN_Startup() - Starts up the Input Mgr
//
//******************************************************************************


void IN_Startup (void)
{
    boolean checkjoys;

    word i;

    if (IN_Started)
        return;

    /*
      all keys are now mapped to the wolf3d-style names,
      except where no such name is available.
     */

    /*
        TODO: Utilize SDL_Scancode and SDL_Keycode for SDL2.

        Right now, it's a basic SDL_Scancode -> Old Scancode (keyb.h)
        conversion.
    */
    memset(scancodes, '\0', sizeof (scancodes));
    scancodes[SDL_SCANCODE_ESCAPE]          = sc_Escape;
    scancodes[SDL_SCANCODE_1]               = sc_1;
    scancodes[SDL_SCANCODE_2]               = sc_2;
    scancodes[SDL_SCANCODE_3]               = sc_3;
    scancodes[SDL_SCANCODE_4]               = sc_4;
    scancodes[SDL_SCANCODE_5]               = sc_5;
    scancodes[SDL_SCANCODE_6]               = sc_6;
    scancodes[SDL_SCANCODE_7]               = sc_7;
    scancodes[SDL_SCANCODE_8]               = sc_8;
    scancodes[SDL_SCANCODE_9]               = sc_9;
    scancodes[SDL_SCANCODE_0]               = sc_0;

    //scancodes[SDLK_EQUALS]          = 0x4E;
    scancodes[SDL_SCANCODE_EQUALS]          = sc_Equals;

    scancodes[SDL_SCANCODE_BACKSPACE]       = sc_BackSpace;
    scancodes[SDL_SCANCODE_TAB]             = sc_Tab;
    scancodes[SDL_SCANCODE_Q]               = sc_Q;
    scancodes[SDL_SCANCODE_W]               = sc_W;
    scancodes[SDL_SCANCODE_E]               = sc_E;
    scancodes[SDL_SCANCODE_R]               = sc_R;
    scancodes[SDL_SCANCODE_T]               = sc_T;
    scancodes[SDL_SCANCODE_Y]               = sc_Y;
    scancodes[SDL_SCANCODE_U]               = sc_U;
    scancodes[SDL_SCANCODE_I]               = sc_I;
    scancodes[SDL_SCANCODE_O]               = sc_O;
    scancodes[SDL_SCANCODE_P]               = sc_P;
    scancodes[SDL_SCANCODE_LEFTBRACKET]     = sc_OpenBracket;
    scancodes[SDL_SCANCODE_RIGHTBRACKET]    = sc_CloseBracket;
    scancodes[SDL_SCANCODE_RETURN]          = sc_Return;
    scancodes[SDL_SCANCODE_LCTRL]           = sc_Control;
    scancodes[SDL_SCANCODE_A]               = sc_A;
    scancodes[SDL_SCANCODE_S]               = sc_S;
    scancodes[SDL_SCANCODE_D]               = sc_D;
    scancodes[SDL_SCANCODE_F]               = sc_F;
    scancodes[SDL_SCANCODE_G]               = sc_G;
    scancodes[SDL_SCANCODE_H]               = sc_H;
    scancodes[SDL_SCANCODE_J]               = sc_J;
    scancodes[SDL_SCANCODE_K]               = sc_K;
    scancodes[SDL_SCANCODE_L]               = sc_L;
    scancodes[SDL_SCANCODE_SEMICOLON]       = 0x27;
    scancodes[SDL_SCANCODE_APOSTROPHE]      = 0x28;
    scancodes[SDL_SCANCODE_GRAVE]           = 0x29;

    /* left shift, but ROTT maps it to right shift in isr.c */
    scancodes[SDL_SCANCODE_LSHIFT]          = sc_RShift; /* sc_LShift */

    scancodes[SDL_SCANCODE_BACKSLASH]       = 0x2B;
    /* Accept the German eszett as a backslash key */
    //scancodes[SDLK_WORLD_63]       = 0x2B;
    scancodes[SDL_SCANCODE_Z]               = sc_Z;
    scancodes[SDL_SCANCODE_X]               = sc_X;
    scancodes[SDL_SCANCODE_C]               = sc_C;
    scancodes[SDL_SCANCODE_V]               = sc_V;
    scancodes[SDL_SCANCODE_B]               = sc_B;
    scancodes[SDL_SCANCODE_N]               = sc_N;
    scancodes[SDL_SCANCODE_M]               = sc_M;
    scancodes[SDL_SCANCODE_COMMA]           = sc_Comma;
    scancodes[SDL_SCANCODE_PERIOD]          = sc_Period;
    scancodes[SDL_SCANCODE_SLASH]           = 0x35;
    scancodes[SDL_SCANCODE_RSHIFT]          = sc_RShift;
    scancodes[SDL_SCANCODE_KP_DIVIDE]       = 0x35;

    /* 0x37 is printscreen */
    //scancodes[SDLK_KP_MULTIPLY]     = 0x37;

    scancodes[SDL_SCANCODE_LALT]            = sc_Alt;
    scancodes[SDL_SCANCODE_RALT]            = sc_Alt;
    scancodes[SDL_SCANCODE_MODE]            = sc_Alt;
    scancodes[SDL_SCANCODE_RCTRL]           = sc_Control;
    scancodes[SDL_SCANCODE_SPACE]           = sc_Space;
    scancodes[SDL_SCANCODE_CAPSLOCK]        = sc_CapsLock;
    scancodes[SDL_SCANCODE_F1]              = sc_F1;
    scancodes[SDL_SCANCODE_F2]              = sc_F2;
    scancodes[SDL_SCANCODE_F3]              = sc_F3;
    scancodes[SDL_SCANCODE_F4]              = sc_F4;
    scancodes[SDL_SCANCODE_F5]              = sc_F5;
    scancodes[SDL_SCANCODE_F6]              = sc_F6;
    scancodes[SDL_SCANCODE_F7]              = sc_F7;
    scancodes[SDL_SCANCODE_F8]              = sc_F8;
    scancodes[SDL_SCANCODE_F9]              = sc_F9;
    scancodes[SDL_SCANCODE_F10]             = sc_F10;
    scancodes[SDL_SCANCODE_F11]             = sc_F11;
    scancodes[SDL_SCANCODE_F12]             = sc_F12;
    scancodes[SDL_SCANCODE_NUMLOCKCLEAR]    = 0x45;
    scancodes[SDL_SCANCODE_SCROLLLOCK]      = 0x46;

    //scancodes[SDLK_MINUS]           = 0x4A;
    scancodes[SDL_SCANCODE_MINUS]           = sc_Minus;

    scancodes[SDL_SCANCODE_KP_7]            = sc_Home;
    scancodes[SDL_SCANCODE_KP_8]            = sc_UpArrow;
    scancodes[SDL_SCANCODE_KP_9]            = sc_PgUp;
    scancodes[SDL_SCANCODE_HOME]            = sc_Home;
    scancodes[SDL_SCANCODE_UP]              = sc_UpArrow;
    scancodes[SDL_SCANCODE_PAGEUP]          = sc_PgUp;
    // Make this a normal minus, for viewport changing
    //scancodes[SDLK_KP_MINUS]        = 0xE04A;
    scancodes[SDL_SCANCODE_KP_MINUS]        = sc_Minus;
    scancodes[SDL_SCANCODE_KP_4]            = sc_LeftArrow;
    scancodes[SDL_SCANCODE_KP_5]            = 0x4C;
    scancodes[SDL_SCANCODE_KP_6]            = sc_RightArrow;
    scancodes[SDL_SCANCODE_LEFT]            = sc_LeftArrow;
    scancodes[SDL_SCANCODE_RIGHT]           = sc_RightArrow;

    //scancodes[SDLK_KP_PLUS]         = 0x4E;
    scancodes[SDL_SCANCODE_KP_PLUS]         = sc_Plus;

    scancodes[SDL_SCANCODE_KP_1]            = sc_End;
    scancodes[SDL_SCANCODE_KP_2]            = sc_DownArrow;
    scancodes[SDL_SCANCODE_KP_3]            = sc_PgDn;
    scancodes[SDL_SCANCODE_END]             = sc_End;
    scancodes[SDL_SCANCODE_DOWN]            = sc_DownArrow;
    scancodes[SDL_SCANCODE_PAGEDOWN]        = sc_PgDn;
    scancodes[SDL_SCANCODE_DELETE]          = sc_Delete;
    scancodes[SDL_SCANCODE_KP_0]            = sc_Insert;
    scancodes[SDL_SCANCODE_INSERT]          = sc_Insert;
    scancodes[SDL_SCANCODE_KP_ENTER]        = sc_Return;

    checkjoys = true;
    MousePresent = true;

    for (i = 1; i < _argc; i++)
    {
        switch (US_CheckParm (_argv[i], ParmStrings))
        {
            case 0:
                checkjoys = false;
                break;

            case 1:
                MousePresent = false;
                break;
        }
    }

    if (!MousePresent)
        mouseenabled = false;
    else if (!quiet)
        printf("IN_Startup: Mouse Present\n");

    for (i = 0; i < MaxJoys; i++)
    {
        JoysPresent[i] = checkjoys ? INL_StartJoy(i) : false;
        if (INL_StartJoy(i) && !quiet)
            printf("IN_Startup: Joystick Present\n");
    }

    IN_Started = true;
}


//******************************************************************************
//
// IN_Shutdown() - Shuts down the Input Mgr
//
//******************************************************************************

void IN_Shutdown (void)
{
    word  i;

    if (IN_Started==false)
        return;

//   INL_ShutMouse();

    for (i = 0; i < MaxJoys; i++)
        INL_ShutJoy(i);

    IN_Started = false;
}


//******************************************************************************
//
// IN_ClearKeysDown() - Clears the keyboard array
//
//******************************************************************************

void IN_ClearKeysDown (void)
{
    LastScan = sc_None;
    memset ((void *)Keyboard, 0, sizeof (Keyboard));
}


//******************************************************************************
//
// IN_ReadControl() - Reads the device associated with the specified
//    player and fills in the control info struct
//
//******************************************************************************

void IN_ReadControl (int player, ControlInfo *info)
{
    boolean     realdelta;
    word        buttons;
    int         dx,dy;
    Motion      mx,my;
    ControlType type;

    KeyboardDef *def;

    dx = dy = 0;
    mx = my = motion_None;
    buttons = 0;


    switch (type = Controls[player])
    {
        case ctrl_Keyboard:
            def = &KbdDefs;

#if 0
            if (Keyboard[def->upleft])
                mx = motion_Left,my = motion_Up;
            else if (Keyboard[def->upright])
                mx = motion_Right,my = motion_Up;
            else if (Keyboard[def->downleft])
                mx = motion_Left,my = motion_Down;
            else if (Keyboard[def->downright])
                mx = motion_Right,my = motion_Down;
#endif
            if (Keyboard[sc_UpArrow])
                my = motion_Up;
            else if (Keyboard[sc_DownArrow])
                my = motion_Down;

            if (Keyboard[sc_LeftArrow])
                mx = motion_Left;
            else if (Keyboard[sc_RightArrow])
                mx = motion_Right;

            if (Keyboard[def->button0])
                buttons += 1 << 0;
            if (Keyboard[def->button1])
                buttons += 1 << 1;
            realdelta = false;
            break;

#if 0
        case ctrl_Joystick1:
        case ctrl_Joystick2:
            INL_GetJoyDelta (type - ctrl_Joystick, &dx, &dy);
            buttons = INL_GetJoyButtons (type - ctrl_Joystick);
            realdelta = true;
            break;

        case ctrl_Mouse:
            INL_GetMouseDelta (&dx,&dy);
            buttons = IN_GetMouseButtons ();
            realdelta = true;
            break;

#endif
        default:
            ;
    }

    if (realdelta)
    {
        mx = (dx < 0)? motion_Left : ((dx > 0)? motion_Right : motion_None);
        my = (dy < 0)? motion_Up : ((dy > 0)? motion_Down : motion_None);
    }
    else
    {
        dx = mx * 127;
        dy = my * 127;
    }

    info->x = dx;
    info->xaxis = mx;
    info->y = dy;
    info->yaxis = my;
    info->button0 = buttons & (1 << 0);
    info->button1 = buttons & (1 << 1);
    info->button2 = buttons & (1 << 2);
    info->button3 = buttons & (1 << 3);
    info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}


//******************************************************************************
//
// IN_WaitForKey() - Waits for a scan code, then clears LastScan and
//    returns the scan code
//
//******************************************************************************

ScanCode IN_WaitForKey (void)
{
    ScanCode result;

    while (!(result = LastScan))
        IN_PumpEvents();
    LastScan = 0;
    return (result);
}


//******************************************************************************
//
// IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
//******************************************************************************

boolean  btnstate[8];

void IN_StartAck (void)
{
    unsigned i,
             buttons = 0;

//
// get initial state of everything
//
    LastScan = 0;

    IN_ClearKeysDown ();
    memset (btnstate, 0, sizeof(btnstate));

    IN_PumpEvents();

    buttons = IN_JoyButtons () << 4;

    buttons |= IN_GetMouseButtons();

    for (i=0; i<8; i++,buttons>>=1)
        if (buttons&1)
            btnstate[i] = true;
}



//******************************************************************************
//
// IN_CheckAck ()
//
//******************************************************************************

boolean IN_CheckAck (void)
{
    unsigned i,
             buttons = 0;

//
// see if something has been pressed
//
    if (LastScan)
        return true;

    IN_PumpEvents();

    buttons = IN_JoyButtons () << 4;

    buttons |= IN_GetMouseButtons();

    for (i=0; i<8; i++,buttons>>=1)
        if ( buttons&1 )
        {
            if (!btnstate[i])
                return true;
        }
        else
            btnstate[i]=false;

    return false;
}



//******************************************************************************
//
// IN_Ack ()
//
//******************************************************************************

void IN_Ack (void)
{
    IN_StartAck ();

    while (!IN_CheckAck ())
        ;
}



//******************************************************************************
//
// IN_UserInput() - Waits for the specified delay time (in ticks) or the
//    user pressing a key or a mouse button. If the clear flag is set, it
//    then either clears the key or waits for the user to let the mouse
//    button up.
//
//******************************************************************************

boolean IN_UserInput (long delay)
{
    long lasttime;

    lasttime = GetTicCount();

    IN_StartAck ();
    do
    {
        if (IN_CheckAck())
            return true;
    }
    while ((GetTicCount() - lasttime) < delay);

    return (false);
}

//===========================================================================


/*
===================
=
= IN_JoyButtons
=
===================
*/

byte IN_JoyButtons (void)
{
    unsigned joybits = sdl_sticks_joybits;

    return (byte) joybits;
}


//******************************************************************************
//
// IN_UpdateKeyboard ()
//
//******************************************************************************

/* HACK HACK HACK */
static int queuegotit=0;

void IN_UpdateKeyboard (void)
{
    int tail;
    int key;

    if (!queuegotit)
        IN_PumpEvents();

    queuegotit=0;

    if (Keytail != Keyhead)
    {
        tail = Keytail;

        while (Keyhead != tail)
        {
            if (KeyboardQueue[Keyhead] & 0x80)        // Up event
            {
                key = KeyboardQueue[Keyhead] & 0x7F;   // AND off high bit

//            if (keysdown[key])
//            {
//               KeyboardQueue[Keytail] = KeyboardQueue[Keyhead];
//               Keytail = (Keytail+1)&(KEYQMAX-1);
//            }
//            else
                Keyboard[key] = 0;
            }
            else                                      // Down event
            {
                Keyboard[KeyboardQueue[Keyhead]] = 1;
//            keysdown[KeyboardQueue[Keyhead]] = 1;
            }

            Keyhead = (Keyhead+1)&(KEYQMAX-1);

        }        // while
    }           // if

    // Carry over movement keys from the last refresh
//   keysdown[sc_RightArrow] = Keyboard[sc_RightArrow];
//   keysdown[sc_LeftArrow]  = Keyboard[sc_LeftArrow];
//   keysdown[sc_UpArrow]    = Keyboard[sc_UpArrow];
//   keysdown[sc_DownArrow]  = Keyboard[sc_DownArrow];
}



//******************************************************************************
//
// IN_InputUpdateKeyboard ()
//
//******************************************************************************

int IN_InputUpdateKeyboard (void)
{
    int key;
    int returnval = 0;
    boolean done = false;

//   _disable ();

    if (Keytail != Keyhead)
    {
        int tail = Keytail;

        while (!done && (Keyhead != tail))
        {
            if (KeyboardQueue[Keyhead] & 0x80)        // Up event
            {
                key = KeyboardQueue[Keyhead] & 0x7F;   // AND off high bit

                Keyboard[key] = 0;
            }
            else                                      // Down event
            {
                Keyboard[KeyboardQueue[Keyhead]] = 1;
                returnval = KeyboardQueue[Keyhead];
                done = true;
            }

            Keyhead = (Keyhead+1)&(KEYQMAX-1);
        }
    }           // if

//   _enable ();

    return (returnval);
}


//******************************************************************************
//
// IN_ClearKeyboardQueue ()
//
//******************************************************************************

void IN_ClearKeyboardQueue (void)
{
    return;

//   IN_ClearKeysDown ();

//   Keytail = Keyhead = 0;
//   memset (KeyboardQueue, 0, sizeof (KeyboardQueue));
//   I_SendKeyboardData(0xf6);
//   I_SendKeyboardData(0xf4);
}


//******************************************************************************
//
// QueueLetterInput ()
//
//******************************************************************************

void QueueLetterInput (void)
{
    int head = Keyhead;
    int tail = Keytail;
    char c;
    int scancode;
    boolean send = false;

    /* HACK HACK HACK */
    /*
      OK, we want the new keys NOW, and not when the update gets them.
      Since this called before IN_UpdateKeyboard in PollKeyboardButtons,
      we shall update here.  The hack is there to prevent IN_UpdateKeyboard
      from stealing any keys... - SBF
     */
    IN_PumpEvents();
    head = Keyhead;
    tail = Keytail;
    queuegotit=1;
    /* HACK HACK HACK */

    while (head != tail)
    {
        if (!(KeyboardQueue[head] & 0x80))        // Down event
        {
            scancode = KeyboardQueue[head];

            if (Keyboard[sc_RShift] || Keyboard[sc_LShift])
            {
                c = ShiftedScanChars[scancode];
            }
            else
            {
                c = ScanChars[scancode];
            }

            // If "is printable char", queue the character
            if (c)
            {
                LetterQueue[LastLetter] = c;
                LastLetter = (LastLetter+1)&(MAXLETTERS-1);

                // If typing a message, update the text with 'c'

                if ( MSG.messageon )
                {
                    Keystate[scancode]=0;
                    KeyboardQueue[head] = 0;
                    if ( MSG.inmenu )
                    {
                        if ( ( c == 'A' ) || ( c == 'a' ) )
                        {
                            MSG.towho = MSG_DIRECTED_TO_ALL;
                            send      = true;
                        }

                        if ( ( gamestate.teamplay ) &&
                                ( ( c == 'T' ) || ( c == 't' ) ) )
                        {
                            MSG.towho = MSG_DIRECTED_TO_TEAM;
                            send      = true;
                        }

                        if ( ( c >= '0' ) && ( c <= '9' ) )
                        {
                            int who;

                            if ( c == '0' )
                            {
                                who = 10;
                            }
                            else
                            {
                                who = c - '1';
                            }

                            // Skip over local player
                            if ( who >= consoleplayer )
                            {
                                who++;
                            }

                            if ( who < numplayers )
                            {
                                MSG.towho = who;
                                send      = true;
                            }
                        }

                        if ( send )
                        {
                            MSG.messageon = false;
                            KeyboardQueue[ head ] = 0;
                            Keyboard[ scancode ]  = 0;
                            LastScan              = 0;
                            FinishModemMessage( MSG.textnum, true );
                        }
                    }
                    else if ( ( scancode >= sc_1 ) && ( scancode <= sc_0 ) &&
                              ( Keyboard[ sc_Alt ] ) )
                    {
                        int msg;

                        msg = scancode - sc_1;

                        if ( CommbatMacros[ msg ].avail )
                        {
                            MSG.length = strlen( CommbatMacros[ msg ].macro ) + 1;
                            strcpy( Messages[ MSG.textnum ].text,
                                    CommbatMacros[ msg ].macro );

                            MSG.messageon = false;
                            FinishModemMessage( MSG.textnum, true );
                            KeyboardQueue[ head ] = 0;
                            Keyboard[ sc_Enter ]  = 0;
                            Keyboard[ sc_Escape ] = 0;
                            LastScan              = 0;
                        }
                        else
                        {
                            MSG.messageon = false;
                            MSG.directed  = false;

                            FinishModemMessage( MSG.textnum, false );
                            AddMessage( "No macro.", MSG_MACRO );
                            KeyboardQueue[ head ] = 0;
                            Keyboard[ sc_Enter ]  = 0;
                            Keyboard[ sc_Escape ] = 0;
                            LastScan              = 0;
                        }
                    }
                    else if ( MSG.length < MAXMESSAGELENGTH )
                    {
                        UpdateModemMessage (MSG.textnum, c);
                    }
                }
            }
            else
            {
                // If typing a message, check for special characters

                if ( MSG.messageon && MSG.inmenu )
                {
                    if ( scancode == sc_Escape )
                    {
                        MSG.messageon = false;
                        MSG.directed  = false;
                        FinishModemMessage( MSG.textnum, false );
                        KeyboardQueue[head] = 0;
                        Keyboard[sc_Enter]  = 0;
                        Keyboard[sc_Escape] = 0;
                        LastScan            = 0;
                    }
                }
                else if ( MSG.messageon && !MSG.inmenu )
                {
                    if ( ( scancode >= sc_F1 ) &&
                            ( scancode <= sc_F10 ) )
                    {
                        MSG.remoteridicule = scancode - sc_F1;
                        MSG.messageon = false;
                        FinishModemMessage(MSG.textnum, true);
                        KeyboardQueue[head] = 0;
                        Keyboard[sc_Enter]  = 0;
                        Keyboard[sc_Escape] = 0;
                        LastScan            = 0;
                    }

                    switch (scancode)
                    {
                        case sc_BackSpace:
                            KeyboardQueue[head] = 0;
                            if (MSG.length > 1)
                            {
                                ModemMessageDeleteChar (MSG.textnum);
                            }
                            Keystate[scancode]=0;
                            break;

                        case sc_Enter:
                            MSG.messageon = false;
                            FinishModemMessage(MSG.textnum, true);
                            KeyboardQueue[head] = 0;
                            Keyboard[sc_Enter]  = 0;
                            Keyboard[sc_Escape] = 0;
                            LastScan            = 0;
                            Keystate[scancode]=0;
                            break;

                        case sc_Escape:
                            MSG.messageon = false;
                            MSG.directed  = false;
                            FinishModemMessage(MSG.textnum, false);
                            KeyboardQueue[head] = 0;
                            Keyboard[sc_Enter]  = 0;
                            Keyboard[sc_Escape] = 0;
                            LastScan            = 0;
                            break;
                    }
                }
            }
        }

        head = (head+1)&(KEYQMAX-1);
    }        // while
}
