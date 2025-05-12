/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

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

gyro related code is based on
https://github.com/yquake2/yquake2/blob/master/src/client/input/sdl.c

*/

#include "quakedef.h"
#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif

// GyroSpace and Play header
#include "GyroSpace/GyroSpace.h"

extern cvar_t ui_mouse;
extern cvar_t language;

static qboolean windowhasfocus = true;	//just in case sdl fails to tell us...
static textmode_t textmode = TEXTMODE_OFF;
static keydevice_t lastactivetype = KD_NONE;

static cvar_t in_debugkeys = {"in_debugkeys", "0", CVAR_NONE};

#ifdef __APPLE__
/* Mouse acceleration needs to be disabled on OS X */
#define MACOS_X_ACCELERATION_HACK
#endif

#ifdef MACOS_X_ACCELERATION_HACK
#include <IOKit/IOTypes.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>
#endif

// SDL2 Game Controller cvars
cvar_t	joy_deadzone_look = { "joy_deadzone_look", "0.175", CVAR_ARCHIVE };
cvar_t	joy_deadzone_move = { "joy_deadzone_move", "0.175", CVAR_ARCHIVE };
cvar_t	joy_outer_threshold_look = { "joy_outer_threshold_look", "0.02", CVAR_ARCHIVE };
cvar_t	joy_outer_threshold_move = { "joy_outer_threshold_move", "0.02", CVAR_ARCHIVE };
cvar_t	joy_deadzone_trigger = { "joy_deadzone_trigger", "0.2", CVAR_ARCHIVE };
cvar_t	joy_sensitivity_yaw = { "joy_sensitivity_yaw", "240", CVAR_ARCHIVE };
cvar_t	joy_sensitivity_pitch = { "joy_sensitivity_pitch", "130", CVAR_ARCHIVE };
cvar_t	joy_invert = { "joy_invert", "0", CVAR_ARCHIVE };
cvar_t	joy_exponent = { "joy_exponent", "2", CVAR_ARCHIVE };
cvar_t	joy_exponent_move = { "joy_exponent_move", "2", CVAR_ARCHIVE };
cvar_t	joy_swapmovelook = { "joy_swapmovelook", "0", CVAR_ARCHIVE };
cvar_t	joy_flick = { "joy_flick", "0", CVAR_ARCHIVE };
cvar_t	joy_flick_time = { "joy_flick_time", "0.125", CVAR_ARCHIVE };
cvar_t	joy_flick_recenter = { "joy_flick_recenter", "0.0", CVAR_ARCHIVE };
cvar_t	joy_flick_deadzone = { "joy_flick_deadzone", "0.9", CVAR_ARCHIVE };
cvar_t	joy_flick_noise_thresh = { "joy_flick_noise_thresh", "2.0", CVAR_ARCHIVE };
cvar_t	joy_flick_adjust_speed = { "joy_flick_adjust_speed", "30.0", CVAR_ARCHIVE };
cvar_t	joy_rumble = { "joy_rumble", "0.3", CVAR_ARCHIVE };
cvar_t	joy_device = { "joy_device", "0", CVAR_ARCHIVE };
cvar_t	joy_always_active = { "joy_always_active", "0", CVAR_ARCHIVE };

cvar_t gyro_enable = {"gyro_enable", "1", CVAR_ARCHIVE};
cvar_t gyro_mode = {"gyro_mode", "2", CVAR_ARCHIVE}; // 2 = GYRO_BUTTON_DISABLES (see gyromode_t)
cvar_t gyro_turning_axis = {"gyro_turning_axis", "0", CVAR_ARCHIVE};

cvar_t gyro_yawsensitivity = {"gyro_yawsensitivity", "2.5", CVAR_ARCHIVE};
cvar_t gyro_pitchsensitivity= {"gyro_pitchsensitivity", "2.5", CVAR_ARCHIVE};

cvar_t gyro_calibration_x = {"gyro_calibration_x", "0", CVAR_ARCHIVE};
cvar_t gyro_calibration_y = {"gyro_calibration_y", "0", CVAR_ARCHIVE};
cvar_t gyro_calibration_z = {"gyro_calibration_z", "0", CVAR_ARCHIVE};

cvar_t gyro_noise_thresh = {"gyro_noise_thresh", "1.5", CVAR_ARCHIVE};

static SDL_JoystickID		joy_active_instanceid = -1;
static int					joy_active_device = -1;
static SDL_GameController	*joy_active_controller = NULL;
static gamepadtype_t		joy_active_type = GAMEPAD_NONE;
static char					joy_active_name[256];
static qboolean				joy_has_rumble = false;

static qboolean	no_mouse = false;

static const int buttonremap[] =
{
	K_MOUSE1,	/* left button		*/
	K_MOUSE3,	/* middle button	*/
	K_MOUSE2,	/* right button		*/
	K_MOUSE4,	/* back button		*/
	K_MOUSE5	/* forward button	*/
};

/* total accumulated mouse movement since last frame */
static int		total_dx = 0, total_dy = 0;
static float	gyro_yaw = 0.f, gyro_roll = 0.f, gyro_pitch = 0.f, gyro_raw_mag = 0.f;
static float	gyro_center_frac = 0.f, gyro_center_amount = 0.f;

// used for gyro calibration
#define GYRO_CALIBRATION_SAMPLES	300
static float gyro_accum[3] = {0.f, 0.f, 0.f};
static unsigned int updates_countdown = 0;

static qboolean gyro_present = false;
static qboolean gyro_button_pressed = false;

static struct
{
	float	yaw;
	float	pitch;
	float	yaw_delta;
	float	prev_lerp_frac;
	float	prev_angle;
	float	prev_scale;
} flick;

static void IN_ResetFlickState (void)
{
	memset (&flick, 0, sizeof (flick));
}

static int SDLCALL IN_FilterMouseEvents (const SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_MOUSEMOTION:
	// case SDL_MOUSEBUTTONDOWN:
	// case SDL_MOUSEBUTTONUP:
		if (key_dest == key_menu)
			M_Mousemove (event->motion.x, event->motion.y);
		else if (key_dest == key_console)
			Con_Mousemove (event->motion.x, event->motion.y);
		return 0;
	}

	return 1;
}

static int SDLCALL IN_SDL2_FilterMouseEvents (void *userdata, SDL_Event *event)
{
	return IN_FilterMouseEvents (event);
}

void IN_ShowCursor (void)
{
	VID_SetMouseCursor (MOUSECURSOR_DEFAULT);

	if (!SDL_GetRelativeMouseMode ())
		return;
	if (SDL_SetRelativeMouseMode (SDL_FALSE) != 0)
		Con_Printf ("WARNING: could not disable relative mouse mode (%s).\n", SDL_GetError ());

	if (!ui_mouse.value)
	{
		SDL_Window *window = (SDL_Window *) VID_GetWindow ();
		int width, height;
		SDL_GetWindowSize (window, &width, &height);
		SDL_WarpMouseInWindow (window, width/2, height/2);
	}
}

void IN_HideCursor (void)
{
#ifdef __APPLE__
	{
		// Work around https://github.com/sezero/quakespasm/issues/48
		int width, height;
		SDL_GetWindowSize((SDL_Window*) VID_GetWindow(), &width, &height);
		SDL_WarpMouseInWindow((SDL_Window*) VID_GetWindow(), width / 2, height / 2);
	}
#endif
	if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0)
		Con_Printf("WARNING: could not enable relative mouse mode (%s).\n", SDL_GetError());
}

static void IN_BeginIgnoringMouseEvents(void)
{
	SDL_EventFilter currentFilter = NULL;
	void *currentUserdata = NULL;
	SDL_GetEventFilter(&currentFilter, &currentUserdata);
	if (currentFilter != IN_SDL2_FilterMouseEvents)
		SDL_SetEventFilter(IN_SDL2_FilterMouseEvents, NULL);
}

static void IN_EndIgnoringMouseEvents(void)
{
	SDL_EventFilter currentFilter;
	void *currentUserdata;
	if (SDL_GetEventFilter(&currentFilter, &currentUserdata) == SDL_TRUE)
		SDL_SetEventFilter(NULL, NULL);
}

#ifdef MACOS_X_ACCELERATION_HACK
static cvar_t in_disablemacosxmouseaccel = {"in_disablemacosxmouseaccel", "1", CVAR_ARCHIVE};
static double originalMouseSpeed = -1.0;

static io_connect_t IN_GetIOHandle(void)
{
	io_connect_t iohandle = MACH_PORT_NULL;
	io_service_t iohidsystem = MACH_PORT_NULL;
	mach_port_t masterport;
	kern_return_t status;

	status = IOMasterPort(MACH_PORT_NULL, &masterport);
	if (status != KERN_SUCCESS)
		return 0;

	iohidsystem = IORegistryEntryFromPath(masterport, kIOServicePlane ":/IOResources/IOHIDSystem");
	if (!iohidsystem)
		return 0;

	status = IOServiceOpen(iohidsystem, mach_task_self(), kIOHIDParamConnectType, &iohandle);
	IOObjectRelease(iohidsystem);

	return iohandle;
}

static void IN_DisableOSXMouseAccel (void)
{
	io_connect_t mouseDev = IN_GetIOHandle();
	if (mouseDev != 0)
	{
		if (IOHIDGetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), &originalMouseSpeed) == kIOReturnSuccess)
		{
			if (IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), -1.0) != kIOReturnSuccess)
			{
				Cvar_Set("in_disablemacosxmouseaccel", "0");
				Con_Printf("WARNING: Could not disable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
			}
		}
		else
		{
			Cvar_Set("in_disablemacosxmouseaccel", "0");
			Con_Printf("WARNING: Could not disable mouse acceleration (failed at IOHIDGetAccelerationWithKey).\n");
		}
		IOServiceClose(mouseDev);
	}
	else
	{
		Cvar_Set("in_disablemacosxmouseaccel", "0");
		Con_Printf("WARNING: Could not disable mouse acceleration (failed at IO_GetIOHandle).\n");
	}
}

static void IN_ReenableOSXMouseAccel (void)
{
	io_connect_t mouseDev = IN_GetIOHandle();
	if (mouseDev != 0)
	{
		if (IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), originalMouseSpeed) != kIOReturnSuccess)
			Con_Printf("WARNING: Could not re-enable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
		IOServiceClose(mouseDev);
	}
	else
	{
		Con_Printf("WARNING: Could not re-enable mouse acceleration (failed at IO_GetIOHandle).\n");
	}
	originalMouseSpeed = -1;
}
#endif /* MACOS_X_ACCELERATION_HACK */


void IN_Activate (void)
{
	if (no_mouse)
		return;

#ifdef MACOS_X_ACCELERATION_HACK
	/* Save the status of mouse acceleration */
	if (originalMouseSpeed == -1 && in_disablemacosxmouseaccel.value)
		IN_DisableOSXMouseAccel();
#endif

	IN_HideCursor();
	IN_EndIgnoringMouseEvents();

	total_dx = 0;
	total_dy = 0;
}

void IN_Deactivate (qboolean free_cursor)
{
	if (no_mouse)
		return;

#ifdef MACOS_X_ACCELERATION_HACK
	if (originalMouseSpeed != -1)
		IN_ReenableOSXMouseAccel();
#endif

	if (free_cursor)
		IN_ShowCursor();
	else
		IN_HideCursor();

	/* discard all mouse events when input is deactivated */
	IN_BeginIgnoringMouseEvents();
}

void IN_DeactivateForConsole (void)
{
	IN_Deactivate(true);
}

void IN_DeactivateForMenu (void)
{
	IN_Deactivate(modestate == MS_WINDOWED || ui_mouse.value);
}

static qboolean IN_UseController (int device_index)
{
	SDL_GameController *gamecontroller;
	const char *controllername;

	if (device_index == joy_active_device)
		return true;

	if (joy_active_device != -1)
	{
#if SDL_VERSION_ATLEAST (2, 0, 9)
		if (joy_has_rumble)
			SDL_GameControllerRumble (joy_active_controller, 0, 0, 100);
#if SDL_VERSION_ATLEAST(2, 0, 14)
		if (SDL_GameControllerHasLED (joy_active_controller))
			SDL_GameControllerSetLED (joy_active_controller, 0, 0, 0);
#endif // SDL_VERSION_ATLEAST (2, 0, 14)
#endif // SDL_VERSION_ATLEAST (2, 0, 9)
		SDL_GameControllerClose (joy_active_controller);

		// Only show "gamepad removed" message when disabling the gamepad altogether,
		// not when switching to a different device
		if (device_index == -1)
			Con_Printf ("Gamepad removed: %s\n", joy_active_name);

		joy_active_name[0] = '\0';
		joy_active_controller = NULL;
		joy_active_instanceid = -1;
		joy_active_device = -1;
		joy_active_type = GAMEPAD_NONE;
		Cvar_SetValueQuick (&joy_device, -1);
		gyro_present = false;
		gyro_yaw = gyro_pitch = gyro_raw_mag = 0.f;
		joy_has_rumble = false;
		IN_ResetFlickState ();
	}

	if (device_index == -1)
		return true;

	if (device_index < 0 || device_index >= SDL_NumJoysticks ())
		return false;

	gamecontroller = SDL_GameControllerOpen (device_index);
	if (!gamecontroller)
	{
		Con_Warning ("couldn't open gamepad device %d\n", device_index);
		return false;
	}

	controllername = SDL_GameControllerName (gamecontroller);
	if (!controllername)
		controllername = "[Unknown gamepad]";
	Con_Printf ("Using gamepad: %s\n", controllername);

	joy_active_controller = gamecontroller;
	joy_active_instanceid = SDL_JoystickInstanceID (SDL_GameControllerGetJoystick (gamecontroller));
	joy_active_device = device_index;
	Cvar_SetValueQuick (&joy_device, device_index);
	// Save controller name so we can print it when unplugged (SDL_GameControllerName would return NULL)
	q_strlcpy (joy_active_name, controllername, sizeof (joy_active_name));

	// Save controller family so we can show more appropriate button names
#if SDL_VERSION_ATLEAST(2, 0, 12)
	switch (SDL_GameControllerGetType (joy_active_controller))
	{
	default:
	case SDL_CONTROLLER_TYPE_XBOX360:
	case SDL_CONTROLLER_TYPE_XBOXONE:
		joy_active_type = GAMEPAD_XBOX;
		break;

	case SDL_CONTROLLER_TYPE_PS3:
	case SDL_CONTROLLER_TYPE_PS4:
	case SDL_CONTROLLER_TYPE_PS5:
		joy_active_type = GAMEPAD_PLAYSTATION;
		break;

	case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
#if SDL_VERSION_ATLEAST(2, 24, 0)
	case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
	case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
	case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
#endif // SDL_VERSION_ATLEAST(2, 24, 0)
		joy_active_type = GAMEPAD_NINTENDO;
		break;
	}
#else
	// No controller type info (old SDL headers), assume Xbox-compatible
	joy_active_type = GAMEPAD_XBOX;
#endif // SDL_VERSION_ATLEAST(2, 0, 12)

#if SDL_VERSION_ATLEAST(2, 0, 14)
	if (SDL_GameControllerHasLED (joy_active_controller))
	{
		// orange LED, seemed fitting for Quake
		SDL_GameControllerSetLED (joy_active_controller, 80, 20, 0);
	}

	// Array of sensors to handle
	SDL_SensorType sensors[] = { SDL_SENSOR_GYRO, SDL_SENSOR_ACCEL };
	const char* sensor_names[] = { "Gyro", "Accelerometer" };
	qboolean sensor_enabled[] = { false, false };

	for (int i = 0; i < 2; i++)
	{
		if (SDL_GameControllerHasSensor (joy_active_controller, sensors[i]) &&
			!SDL_GameControllerSetSensorEnabled (joy_active_controller, sensors[i], SDL_TRUE))
		{
			sensor_enabled[i] = true;
#if SDL_VERSION_ATLEAST(2, 0, 16)
			if (sensors[i] == SDL_SENSOR_GYRO)
			{
				Con_Printf ("%s sensor enabled at %g Hz\n", sensor_names[i],
					SDL_GameControllerGetSensorDataRate (joy_active_controller, sensors[i]));
			}
			else
			{
				Con_Printf ("%s sensor enabled.\n", sensor_names[i]);
			}
#else
			Con_Printf ("%s sensor enabled.\n", sensor_names[i]);
#endif
		}
		else
		{
			Con_Printf ("%s sensor not found\n", sensor_names[i]);
		}
	}

	// Update gyro_present flag if the gyroscope is enabled
	gyro_present = sensor_enabled[0];
#endif // SDL_VERSION_ATLEAST(2, 0, 14)

#if SDL_VERSION_ATLEAST (2, 0, 9)
	joy_has_rumble = SDL_GameControllerHasRumble (joy_active_controller);
#endif // SDL_VERSION_ATLEAST (2, 0, 9)
	return true;
}

static void IN_SetupJoystick (void)
{
	int	count = SDL_NumJoysticks ();
	int	device_index = CLAMP (-1, (int)joy_device.value, count - 1);
	IN_UseController (device_index);
}

static qboolean IN_RemapJoystick (void)
{
	int i, count;

	if (joy_active_instanceid == -1)
		return false;

	for (i = 0, count = SDL_NumJoysticks (); i < count; i++)
	{
		if (SDL_JoystickGetDeviceInstanceID (i) == joy_active_instanceid)
		{
			joy_active_device = i;
			Cvar_SetValueQuick (&joy_device, i);
			return true;
		}
	}

	return false;
}

void IN_StartupJoystick (void)
{
    int i;
    int nummappings;
    char controllerdb[MAX_OSPATH];

    if (COM_CheckParm("-nojoy"))
        return;

#if SDL_VERSION_ATLEAST(2, 0, 12)
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 14)
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 22)
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");

	// Enable rumble and motion sensors for PS4 and PS5 controllers while under Bluetooth connectivitiy
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 23, 2)
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 25, 1)
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_PS3, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 26, 0)
	SDL_SetHint (SDL_HINT_JOYSTICK_HIDAPI_WII, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_SetHint (SDL_HINT_JOYSTICK_RAWINPUT, "1");
	SDL_SetHint (SDL_HINT_JOYSTICK_RAWINPUT_CORRELATE_XINPUT, "1");
#endif

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1)
    {
        Con_Warning("could not initialize SDL Game Controller\n");
        return;
    }

    // Load additional SDL2 controller definitions from gamecontrollerdb.txt
    for (i = 0; i < com_numbasedirs; i++)
    {
        q_snprintf(controllerdb, sizeof(controllerdb), "%s/gamecontrollerdb.txt", com_basedirs[i]);
        nummappings = SDL_GameControllerAddMappingsFromFile(controllerdb);
        if (nummappings > 0)
            Con_Printf("%d mappings loaded from gamecontrollerdb.txt\n", nummappings);
    }

    IN_SetupJoystick();
}

void IN_ShutdownJoystick (void)
{
	IN_UseController (-1);
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

qboolean IN_HasGamepad (void)
{
	return joy_active_controller != NULL;
}

const char *IN_GetGamepadName (void)
{
	return joy_active_controller ? joy_active_name : NULL;
}

gamepadtype_t IN_GetGamepadType (void)
{
	return joy_active_type;
}

void IN_UseNextGamepad (int dir, qboolean allow_disable)
{
	int i, j, numiter, numdev;

	numdev = SDL_NumJoysticks ();
	numiter = allow_disable ? numdev : numdev - 1;

	for (i = 0, j = joy_active_device + dir; i < numiter; i++, j += dir)
	{
		if (j < -1)
			j = numdev - 1;
		else if (j < 0)
			j = allow_disable ? -1 : numdev - 1;
		else if (j >= numdev)
			j = allow_disable ? -1 : 0;

		if ((j == -1 || SDL_IsGameController (j)) && IN_UseController (j))
			return;
	}
}

qboolean IN_HasRumble (void)
{
	return joy_has_rumble;
}

void IN_GyroActionDown (void)
{
	gyro_button_pressed = true;
}

void IN_GyroActionUp (void)
{
	gyro_button_pressed = false;
}

/*
================
Joy_Device_f

Called when joy_device changes
================
*/
static void Joy_Device_f (cvar_t *cvar)
{
	if ((int)cvar->value != joy_active_device)
		IN_SetupJoystick ();
}

/*
================
Joy_Device_Completion_f

Tab completion for the joy_device cvar
================
*/
static void Joy_Device_Completion_f (cvar_t *cvar, const char *partial)
{
	int i, count;

	for (i = 0, count = SDL_NumJoysticks (); i < count; i++)
		if (SDL_IsGameController (i))
			Con_AddToTabList (va ("%d", i), partial, SDL_GameControllerNameForIndex (i));
}

/*
================
Joy_Flick_f

Called when joy_flick changes
================
*/
static void Joy_Flick_f (cvar_t *cvar)
{
	IN_ResetFlickState ();
}

/*
================
IN_UpdateSDLTextInput

Calls SDL_StartTextInput/StopTextInput as needed
based on textmode and the device type
================
*/
static void IN_UpdateSDLTextInput (void)
{
	// For devices with an on-screen keyboard (e.g. Steam Deck) we only start text input
	// if requested explicitly (TEXTMODE_ON) in order to avoid having the OSK pop up
	// in searchable menus (Options, Maps, or Mods), which use emulated char events instead.
	// For all the other devices we start text input if not disabled explicitly (TEXTMODE_OFF).
	// In other words, TEXTMODE_NOPOPUP is treated as OFF on Steam Deck, ON on desktop.
	qboolean enabled = SDL_HasScreenKeyboardSupport () ? textmode == TEXTMODE_ON : textmode != TEXTMODE_OFF;
	if (enabled)
		SDL_StartTextInput ();
	else
		SDL_StopTextInput ();
}

/*
================
IN_Init
================
*/
void IN_Init (void)
{
	textmode = Key_TextEntry();
	IN_UpdateSDLTextInput ();

	if (safemode || COM_CheckParm("-nomouse"))
	{
		no_mouse = true;
		/* discard all mouse events when input is deactivated */
		IN_BeginIgnoringMouseEvents();
	}

#ifdef MACOS_X_ACCELERATION_HACK
	Cvar_RegisterVariable(&in_disablemacosxmouseaccel);
#endif
	Cvar_RegisterVariable(&in_debugkeys);
	Cvar_RegisterVariable(&joy_sensitivity_yaw);
	Cvar_RegisterVariable(&joy_sensitivity_pitch);
	Cvar_RegisterVariable(&joy_deadzone_look);
	Cvar_RegisterVariable(&joy_deadzone_move);
	Cvar_RegisterVariable(&joy_outer_threshold_look);
	Cvar_RegisterVariable(&joy_outer_threshold_move);
	Cvar_RegisterVariable(&joy_deadzone_trigger);
	Cvar_RegisterVariable(&joy_invert);
	Cvar_RegisterVariable(&joy_exponent);
	Cvar_RegisterVariable(&joy_exponent_move);
	Cvar_RegisterVariable(&joy_swapmovelook);
	Cvar_RegisterVariable(&joy_flick);
	Cvar_SetCallback (&joy_flick, Joy_Flick_f);
	Cvar_RegisterVariable(&joy_flick_time);
	Cvar_RegisterVariable(&joy_flick_recenter);
	Cvar_RegisterVariable(&joy_flick_deadzone);
	Cvar_RegisterVariable(&joy_flick_noise_thresh);
	Cvar_RegisterVariable(&joy_flick_adjust_speed);
	Cvar_RegisterVariable(&joy_rumble);
	Cvar_RegisterVariable(&joy_device);
	Cvar_SetCallback(&joy_device, Joy_Device_f);
	Cvar_SetCompletion(&joy_device, Joy_Device_Completion_f);
	Cvar_RegisterVariable(&joy_always_active);

	Cvar_RegisterVariable(&gyro_enable);
	Cvar_RegisterVariable(&gyro_mode);
	Cvar_RegisterVariable(&gyro_turning_axis);

	Cvar_RegisterVariable(&gyro_yawsensitivity);
	Cvar_RegisterVariable(&gyro_pitchsensitivity);

	Cvar_RegisterVariable(&gyro_calibration_x);
	Cvar_RegisterVariable(&gyro_calibration_y);
	Cvar_RegisterVariable(&gyro_calibration_z);
	Cvar_RegisterVariable(&gyro_noise_thresh);

	Cmd_AddCommand ("+gyroaction", IN_GyroActionDown);
	Cmd_AddCommand ("-gyroaction", IN_GyroActionUp);

	IN_Activate();
	IN_StartupJoystick();
	Sys_ActivateKeyFilter(true);
}

void IN_Shutdown (void)
{
	Sys_ActivateKeyFilter(false);
	IN_Deactivate(true);
	IN_ShutdownJoystick();
}

extern cvar_t v_centerspeed;
extern cvar_t cl_maxpitch; /* johnfitz -- variable pitch clamping */
extern cvar_t cl_minpitch; /* johnfitz -- variable pitch clamping */
extern cvar_t scr_fov;

static float IN_FovScale (void)
{
	return tan (DEG2RAD (r_refdef.basefov) * 0.5f) / tan (DEG2RAD (scr_fov.value) * 0.5f);
}

void IN_MouseMotion(int dx, int dy)
{
	if (!windowhasfocus)
		dx = dy = 0;	//don't change view angles etc while unfocused.
	if (cls.state != ca_connected || cls.signon != SIGNONS || key_dest != key_game || CL_InCutscene ())
	{
		total_dx = 0;
		total_dy = 0;
		return;
	}
	total_dx += dx;
	total_dy += dy;
}

typedef struct joyaxis_s
{
	float x;
	float y;
} joyaxis_t;

typedef struct joy_buttonstate_s
{
	qboolean buttondown[SDL_CONTROLLER_BUTTON_MAX];
} joybuttonstate_t;

typedef struct axisstate_s
{
	float axisvalue[SDL_CONTROLLER_AXIS_MAX]; // normalized to +-1
} joyaxisstate_t;

static joybuttonstate_t joy_buttonstate;
static joyaxisstate_t joy_axisstate;

static double joy_buttontimer[SDL_CONTROLLER_BUTTON_MAX];
static double joy_emulatedkeytimer[6];

#ifdef __WATCOMC__ /* OW1.9 doesn't have powf() / sqrtf() */
#define powf pow
#define sqrtf sqrt
#endif

/*
================
IN_AxisMagnitude

Returns the vector length of the given joystick axis
================
*/
static vec_t IN_AxisMagnitude(joyaxis_t axis)
{
	vec_t magnitude = sqrtf((axis.x * axis.x) + (axis.y * axis.y));
	return magnitude;
}

/*
================
IN_ApplyEasing

assumes axis values are in [-1, 1] and the vector magnitude has been clamped at 1.
Raises the axis values to the given exponent, keeping signs.
================
*/
static joyaxis_t IN_ApplyEasing(joyaxis_t axis, float exponent)
{
	joyaxis_t result = {0};
	vec_t eased_magnitude;
	vec_t magnitude = IN_AxisMagnitude(axis);
	
	if (magnitude == 0)
		return result;
	
	eased_magnitude = powf(magnitude, exponent);
	
	result.x = axis.x * (eased_magnitude / magnitude);
	result.y = axis.y * (eased_magnitude / magnitude);
	return result;
}

/*
================
IN_ApplyDeadzone

in: raw joystick axis values converted to floats in +-1
out: applies a circular inner deadzone and a circular outer threshold and clamps the magnitude at 1
     (my 360 controller is slightly non-circular and the stick travels further on the diagonals)

deadzone is expected to satisfy 0 < deadzone < 1 - outer_threshold
outer_threshold is expected to satisfy 0 < outer_threshold < 1 - deadzone

from https://github.com/jeremiah-sypult/Quakespasm-Rift
and adapted from http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
================
*/
static joyaxis_t IN_ApplyDeadzone(joyaxis_t axis, float deadzone, float outer_threshold)
{
	joyaxis_t result = {0};
	vec_t magnitude = IN_AxisMagnitude(axis);
	
	if ( magnitude > deadzone ) {
		// rescale the magnitude so deadzone becomes 0, and 1-outer_threshold becomes 1
		const vec_t new_magnitude = q_min(1.0, (magnitude - deadzone) / (1.0 - deadzone - outer_threshold));
		const vec_t scale = new_magnitude / magnitude;
		result.x = axis.x * scale;
		result.y = axis.y * scale;
	}
	
	return result;
}

/*
================
IN_KeyForControllerButton
================
*/
static int IN_KeyForControllerButton(SDL_GameControllerButton button)
{
	switch (button)
	{
		case SDL_CONTROLLER_BUTTON_A: return K_ABUTTON;
		case SDL_CONTROLLER_BUTTON_B: return K_BBUTTON;
		case SDL_CONTROLLER_BUTTON_X: return K_XBUTTON;
		case SDL_CONTROLLER_BUTTON_Y: return K_YBUTTON;
		// Note: back and start are always mapped to TAB/ESC, the player cannot rebind them
		case SDL_CONTROLLER_BUTTON_BACK: return K_TAB;
		case SDL_CONTROLLER_BUTTON_START: return K_ESCAPE;
		case SDL_CONTROLLER_BUTTON_LEFTSTICK: return K_LTHUMB;
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return K_RTHUMB;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return K_LSHOULDER;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return K_RSHOULDER;
		case SDL_CONTROLLER_BUTTON_DPAD_UP: return K_DPAD_UP;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return K_DPAD_DOWN;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return K_DPAD_LEFT;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return K_DPAD_RIGHT;
		case SDL_CONTROLLER_BUTTON_MISC1: return K_MISC1;
		case SDL_CONTROLLER_BUTTON_PADDLE1: return K_PADDLE1;
		case SDL_CONTROLLER_BUTTON_PADDLE2: return K_PADDLE2;
		case SDL_CONTROLLER_BUTTON_PADDLE3: return K_PADDLE3;
		case SDL_CONTROLLER_BUTTON_PADDLE4: return K_PADDLE4;
		case SDL_CONTROLLER_BUTTON_TOUCHPAD: return K_TOUCHPAD;
		default: return 0;
	}
}

/*
================
IN_JoyKeyEvent

Sends a Key_Event if a unpressed -> pressed or pressed -> unpressed transition occurred,
and generates key repeats if the button is held down.

Adapted from DarkPlaces by lordhavoc
================
*/
static void IN_JoyKeyEvent(qboolean wasdown, qboolean isdown, int key, double *timer)
{
	static const double repeatdelay = 0.5; // time (in seconds) between initial press and first repetition
	static const double repeatrate = 32.0; // ticks per second

	// we can't use `realtime` for key repeats because it is not monotomic
	const double currenttime = Sys_DoubleTime();
	
	if (wasdown)
	{
		if (isdown)
		{
			if (currenttime >= *timer)
			{
				*timer = currenttime + 1.0 / repeatrate;
				lastactivetype = KD_GAMEPAD;
				Key_Event(key, true);
			}
		}
		else
		{
			*timer = 0;
			lastactivetype = KD_GAMEPAD;
			Key_Event(key, false);
		}
	}
	else
	{
		if (isdown)
		{
			*timer = currenttime + repeatdelay;
			lastactivetype = KD_GAMEPAD;
			Key_Event(key, true);
		}
	}
}

static joyaxis_t IN_GetLookAxis (joyaxisstate_t *state)
{
	joyaxis_t axis;
	axis.x = state->axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX];
	axis.y = state->axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY];
	return axis;
}

static joyaxis_t IN_GetMoveAxis (joyaxisstate_t *state)
{
	joyaxis_t axis;
	axis.x = state->axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX];
	axis.y = state->axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY];
	return axis;
}

static qboolean IN_JoyActive (void)
{
	return joy_active_controller != NULL && (joy_always_active.value || lastactivetype == KD_GAMEPAD);
}

/*
================
IN_Commands

Emit key events for game controller buttons, including emulated buttons for analog sticks/triggers
================
*/
void IN_Commands (void)
{
	joyaxisstate_t newaxisstate;
	int i;
	const float stickthreshold = 0.9;
	const float triggerthreshold = joy_deadzone_trigger.value;
	
	if (!joy_active_controller)
		return;

	// emit key events for controller buttons
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		qboolean newstate = SDL_GameControllerGetButton(joy_active_controller, (SDL_GameControllerButton)i);
		qboolean oldstate = joy_buttonstate.buttondown[i];
		
		joy_buttonstate.buttondown[i] = newstate;
		
		// NOTE: This can cause a reentrant call of IN_Commands, via SCR_ModalMessage when confirming a new game.
		IN_JoyKeyEvent(oldstate, newstate, IN_KeyForControllerButton((SDL_GameControllerButton)i), &joy_buttontimer[i]);
	}
	
	for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
	{
		newaxisstate.axisvalue[i] = SDL_GameControllerGetAxis(joy_active_controller, (SDL_GameControllerAxis)i) / 32768.0f;
	}
	
	// emit emulated arrow keys so the analog sticks can be used in the menu
	if (key_dest != key_game)
	{
		joyaxis_t old_move = IN_GetMoveAxis (&joy_axisstate);
		joyaxis_t new_move = IN_GetMoveAxis (&newaxisstate);
		IN_JoyKeyEvent(old_move.x < -stickthreshold, new_move.x < -stickthreshold, K_LEFTARROW,		&joy_emulatedkeytimer[0]);
		IN_JoyKeyEvent(old_move.x >  stickthreshold, new_move.x >  stickthreshold, K_RIGHTARROW,	&joy_emulatedkeytimer[1]);
		IN_JoyKeyEvent(old_move.y < -stickthreshold, new_move.y < -stickthreshold, K_UPARROW,		&joy_emulatedkeytimer[2]);
		IN_JoyKeyEvent(old_move.y >  stickthreshold, new_move.y >  stickthreshold, K_DOWNARROW,		&joy_emulatedkeytimer[3]);
	}
	else if (lastactivetype != KD_GAMEPAD)
	{
		if (IN_AxisMagnitude (IN_GetLookAxis (&newaxisstate)) > joy_deadzone_look.value ||
			IN_AxisMagnitude (IN_GetMoveAxis (&newaxisstate)) > joy_deadzone_move.value)
			lastactivetype = KD_GAMEPAD;
	}

	// scroll console with look stick
	if (key_dest == key_console)
	{
		const float scrollthreshold = 0.1f;
		const float maxscrollspeed = 72.f; // lines per second
		const float scrollinterval = 1.f / maxscrollspeed; 
		static double timer = 0.0;
		joyaxis_t raw, deadzone, eased;
		float scale;

		raw = IN_GetLookAxis (&newaxisstate);
		deadzone = IN_ApplyDeadzone (raw, joy_deadzone_look.value, joy_outer_threshold_look.value);
		eased = IN_ApplyEasing (deadzone, joy_exponent.value);
		if (joy_invert.value)
			eased.y = -eased.y;

		scale = fabs (eased.y);
		if (scale > scrollthreshold)
		{
			scale = (scale - scrollthreshold) / (1.f - scrollthreshold);
			timer -= scale * host_rawframetime;
			if (timer < 0.0)
			{
				int ticks = (int) ceil (-timer / scrollinterval);
				timer += ticks * scrollinterval;
				Con_Scroll (eased.y < 0.0f ? ticks : -ticks);
			}
		}
		else
		{
			timer = 0.0;
		}
	}

	// emit emulated keys for the analog triggers
	IN_JoyKeyEvent(joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > triggerthreshold,  newaxisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > triggerthreshold, K_LTRIGGER, &joy_emulatedkeytimer[4]);
	IN_JoyKeyEvent(joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > triggerthreshold, newaxisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > triggerthreshold, K_RTRIGGER, &joy_emulatedkeytimer[5]);
	
	joy_axisstate = newaxisstate;

#if SDL_VERSION_ATLEAST (2, 0, 9)
	if (joy_has_rumble && !IN_IsCalibratingGyro () && joy_rumble.value > 0.f && IN_JoyActive ())
	{
		float strength = CLAMP (0.f, joy_rumble.value, 1.f) * 0xffff;
		float lofreq = GetClampedFraction (S_GetLoFreqLevel (), 0.067f, 0.45f);
		float hifreq = GetClampedFraction (S_GetHiFreqLevel (), 0.061f, 0.45f);
		hifreq *= hifreq;
		SDL_GameControllerRumble (joy_active_controller, lofreq * strength, hifreq * strength, 100);
	}
#endif // SDL_VERSION_ATLEAST (2, 0, 9)
}

/*
================
IN_FlickStickEasing
================
*/
static float IN_FlickStickEasing (float frac)
{
	frac = 1.f - frac;
	frac = 1.f - frac * frac;
	return frac;
}

/*
================
IN_JoyMove
================
*/
void IN_JoyMove (usercmd_t *cmd)
{
	float	speed;
	joyaxis_t moveRaw, moveDeadzone, moveEased;
	joyaxis_t lookRaw, lookDeadzone, lookEased;
	extern	cvar_t	sv_maxspeed;

	if (!joy_active_controller)
		return;

	if (cl.paused || key_dest != key_game)
		return;

	moveRaw = IN_GetMoveAxis (&joy_axisstate);
	lookRaw = IN_GetLookAxis (&joy_axisstate);

	moveDeadzone = IN_ApplyDeadzone(moveRaw, joy_deadzone_move.value, joy_outer_threshold_move.value);
	lookDeadzone = IN_ApplyDeadzone(lookRaw, joy_deadzone_look.value, joy_outer_threshold_look.value);

	moveEased = IN_ApplyEasing(moveDeadzone, joy_exponent_move.value);
	lookEased = IN_ApplyEasing(lookDeadzone, joy_exponent.value);

	if ((in_speed.state & 1) ^ (cl_alwaysrun.value != 0.0 || cl_forwardspeed.value >= sv_maxspeed.value))
		// running
		speed = sv_maxspeed.value;
	else if (cl_forwardspeed.value >= sv_maxspeed.value)
		// not running, with always run = vanilla
		speed = q_min(sv_maxspeed.value, cl_forwardspeed.value / cl_movespeedkey.value);
	else
		// not running, with always run = off or quakespasm
		speed = cl_forwardspeed.value;

	cmd->sidemove += speed * moveEased.x;
	cmd->forwardmove -= speed * moveEased.y;

	if (CL_InCutscene ())
		return;

	// handle flick stick if enabled
	if (joy_flick.value && gyro_present && gyro_enable.value)
	{
		float		angle, scale, lerp_frac, delta;
		qboolean	isactive, wasactive;

		// get current stick position in polar coordinates
		angle = NormalizeAngle (RAD2DEG (atan2 (lookRaw.y, lookRaw.x)) + 90.f);
		scale = IN_AxisMagnitude (lookRaw);

		// handle state transitions
		isactive = scale > joy_flick_deadzone.value;
		wasactive = flick.prev_scale > joy_flick_deadzone.value;
		if (isactive != wasactive)
		{
			if (!wasactive) // start new flick
			{
				flick.prev_lerp_frac = 0.f;
				flick.yaw = angle;
				flick.pitch = cl.viewangles[PITCH];
			}
		}
		else if (isactive) // continuous adjustments
		{
			delta = AngleDifference (angle, flick.prev_angle);
			if (joy_flick_noise_thresh.value > 0.f) // filter small movements
			{
				float filter_scale = fabs (delta) / joy_flick_noise_thresh.value;
				if (filter_scale < 1.f)
				{
					filter_scale = LERP (0.05f, 1.f, filter_scale * filter_scale);
					delta *= filter_scale;
					angle = NormalizeAngle (flick.prev_angle + delta);
				}
			}
			flick.yaw_delta += delta;
		}

		// apply yaw adjustment
		if (joy_flick_adjust_speed.value > 0.f)
			delta = flick.yaw_delta * q_min (1.0, host_rawframetime * joy_flick_adjust_speed.value);
		else
			delta = flick.yaw_delta;
		if (fabs (delta) > 0.01)
		{
			cl.viewangles[YAW] -= delta;
			flick.yaw_delta -= delta;
		}

		// advance angle animation
		if (joy_flick_time.value > 0.f)
		{
			lerp_frac = flick.prev_lerp_frac + host_rawframetime / joy_flick_time.value;
			lerp_frac = CLAMP (0.f, lerp_frac, 1.f);
		}
		else
			lerp_frac = 1.f;

		delta = IN_FlickStickEasing (lerp_frac) - IN_FlickStickEasing (flick.prev_lerp_frac);
		cl.viewangles[YAW] -= flick.yaw * delta;
		cl.viewangles[PITCH] -= flick.pitch * delta * CLAMP (0.f, joy_flick_recenter.value, 1.f);

		// update state
		flick.prev_scale = scale;
		flick.prev_angle = angle;
		flick.prev_lerp_frac = lerp_frac;
	}
	else // traditional joystick look
	{
		IN_ResetFlickState ();

		cl.viewangles[YAW] -= lookEased.x * joy_sensitivity_yaw.value * host_rawframetime;
		cl.viewangles[PITCH] += lookEased.y * joy_sensitivity_pitch.value * (joy_invert.value ? -1.0 : 1.0) * host_rawframetime;

		if (lookEased.x != 0 || lookEased.y != 0)
			V_StopPitchDrift();
	}

	/* johnfitz -- variable pitch clamping */
	if (cl.viewangles[PITCH] > cl_maxpitch.value)
		cl.viewangles[PITCH] = cl_maxpitch.value;
	if (cl.viewangles[PITCH] < cl_minpitch.value)
		cl.viewangles[PITCH] = cl_minpitch.value;
}

static float IN_RecenterEasing (float frac)
{
	return frac * frac;
}

void IN_GyroMove(usercmd_t *cmd)
{
	float scale, duration, lerp_frac;
	if (!gyro_enable.value)
		return;

	if (!IN_JoyActive ())
		return;

	if (cl.paused || key_dest != key_game)
		return;

	if (CL_InCutscene ())
		return;

	scale = (180.f / M_PI) * host_rawframetime * IN_FovScale ();
	switch ((int)gyro_mode.value)
	{
	case GYRO_BUTTON_DISABLES:
		if (gyro_button_pressed)
			return;
		break;
	case GYRO_BUTTON_ENABLES:
		if (!gyro_button_pressed)
			return;
		break;
	case GYRO_BUTTON_INVERTS_DIR:
		if (gyro_button_pressed)
			scale = -scale;
		break;
	default:
		break;
	}

	// apply gyro
	cl.viewangles[YAW] += scale * gyro_yaw * gyro_yawsensitivity.value;
	cl.viewangles[PITCH] -= scale * gyro_pitch * gyro_pitchsensitivity.value;

	// Default pitch drift code tries to move towards the ideal pitch
	// every frame, which means it is always fighting the player's input.
	// When gyro is active we disable the default behavior, and instead
	// we perform "additive" camera centering by storing the delta
	// between the ideal pitch and the current pitch when centerview is used,
	// and injecting that delta over some amount of time.

	// stop standard pitch drifting
	V_StopPitchDrift ();

	// store angle delta and reset centering anim if centerview was used this frame
	if (cl.lastcenterstart == cl.time)
	{
		gyro_center_frac = 0.f;
		gyro_center_amount = cl.idealpitch - cl.viewangles[PITCH];
	}

	// try to roughly mimic the look of the default pitch drift code
	if (gyro_center_amount != 0.f && v_centerspeed.value > 0.f)
	{
		duration = fabs (gyro_center_amount / v_centerspeed.value);
		lerp_frac = gyro_center_frac + host_rawframetime / duration;
		lerp_frac = CLAMP (0.f, lerp_frac, 1.f);
	}
	else
		lerp_frac = 1.f;
	scale = IN_RecenterEasing (lerp_frac) - IN_RecenterEasing (gyro_center_frac);
	gyro_center_frac = lerp_frac;
	cl.viewangles[PITCH] += gyro_center_amount * scale;

	/* johnfitz -- variable pitch clamping */
	if (cl.viewangles[PITCH] > cl_maxpitch.value)
		cl.viewangles[PITCH] = cl_maxpitch.value;
	if (cl.viewangles[PITCH] < cl_minpitch.value)
		cl.viewangles[PITCH] = cl_minpitch.value;
}

void IN_MouseMove(usercmd_t *cmd)
{
	float		dmx, dmy;
	float		sens;
	qboolean	mlook = (in_mlook.state & 1) || freelook.value;

	sens = sensitivity.value * IN_FovScale ();

	dmx = total_dx * sens;
	dmy = total_dy * sens;

	total_dx = 0;
	total_dy = 0;

	if ((in_strafe.state & 1) || (lookstrafe.value && mlook))
		cmd->sidemove += m_side.value * dmx;
	else
		cl.viewangles[YAW] -= m_yaw.value * dmx;

	if (mlook)
	{
		if (dmx || dmy)
			V_StopPitchDrift ();
	}

	if (mlook && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * dmy;
		/* johnfitz -- variable pitch clamping */
		if (cl.viewangles[PITCH] > cl_maxpitch.value)
			cl.viewangles[PITCH] = cl_maxpitch.value;
		if (cl.viewangles[PITCH] < cl_minpitch.value)
			cl.viewangles[PITCH] = cl_minpitch.value;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * dmy;
		else
			cmd->forwardmove -= m_forward.value * dmy;
	}
}

void IN_Move(usercmd_t *cmd)
{
	IN_JoyMove(cmd);
	IN_GyroMove(cmd);
	IN_MouseMove(cmd);
}

void IN_ClearStates (void)
{
}

void IN_UpdateInputMode (void)
{
	textmode_t want_textmode = Key_TextEntry();
	if (textmode != want_textmode)
	{
		textmode = want_textmode;
		IN_UpdateSDLTextInput ();
	}
}

qboolean IN_EmulatedCharEvents (void)
{
	return textmode == TEXTMODE_NOPOPUP && !SDL_IsTextInputActive ();
}

keydevice_t IN_GetLastActiveDeviceType (void)
{
	return lastactivetype;
}

static inline int IN_SDL2_ScancodeToQuakeKey(SDL_Scancode scancode)
{
	switch (scancode)
	{
	case SDL_SCANCODE_TAB: return K_TAB;
	case SDL_SCANCODE_RETURN: return K_ENTER;
	case SDL_SCANCODE_RETURN2: return K_ENTER;
	case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
	case SDL_SCANCODE_SPACE: return K_SPACE;

	case SDL_SCANCODE_A: return 'a';
	case SDL_SCANCODE_B: return 'b';
	case SDL_SCANCODE_C: return 'c';
	case SDL_SCANCODE_D: return 'd';
	case SDL_SCANCODE_E: return 'e';
	case SDL_SCANCODE_F: return 'f';
	case SDL_SCANCODE_G: return 'g';
	case SDL_SCANCODE_H: return 'h';
	case SDL_SCANCODE_I: return 'i';
	case SDL_SCANCODE_J: return 'j';
	case SDL_SCANCODE_K: return 'k';
	case SDL_SCANCODE_L: return 'l';
	case SDL_SCANCODE_M: return 'm';
	case SDL_SCANCODE_N: return 'n';
	case SDL_SCANCODE_O: return 'o';
	case SDL_SCANCODE_P: return 'p';
	case SDL_SCANCODE_Q: return 'q';
	case SDL_SCANCODE_R: return 'r';
	case SDL_SCANCODE_S: return 's';
	case SDL_SCANCODE_T: return 't';
	case SDL_SCANCODE_U: return 'u';
	case SDL_SCANCODE_V: return 'v';
	case SDL_SCANCODE_W: return 'w';
	case SDL_SCANCODE_X: return 'x';
	case SDL_SCANCODE_Y: return 'y';
	case SDL_SCANCODE_Z: return 'z';

	case SDL_SCANCODE_1: return '1';
	case SDL_SCANCODE_2: return '2';
	case SDL_SCANCODE_3: return '3';
	case SDL_SCANCODE_4: return '4';
	case SDL_SCANCODE_5: return '5';
	case SDL_SCANCODE_6: return '6';
	case SDL_SCANCODE_7: return '7';
	case SDL_SCANCODE_8: return '8';
	case SDL_SCANCODE_9: return '9';
	case SDL_SCANCODE_0: return '0';

	case SDL_SCANCODE_MINUS: return '-';
	case SDL_SCANCODE_EQUALS: return '=';
	case SDL_SCANCODE_LEFTBRACKET: return '[';
	case SDL_SCANCODE_RIGHTBRACKET: return ']';
	case SDL_SCANCODE_BACKSLASH: return '\\';
	case SDL_SCANCODE_NONUSHASH: return '#';
	case SDL_SCANCODE_SEMICOLON: return ';';
	case SDL_SCANCODE_APOSTROPHE: return '\'';
	case SDL_SCANCODE_GRAVE: return '`';
	case SDL_SCANCODE_COMMA: return ',';
	case SDL_SCANCODE_PERIOD: return '.';
	case SDL_SCANCODE_SLASH: return '/';
	case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

	case SDL_SCANCODE_BACKSPACE: return K_BACKSPACE;
	case SDL_SCANCODE_UP: return K_UPARROW;
	case SDL_SCANCODE_DOWN: return K_DOWNARROW;
	case SDL_SCANCODE_LEFT: return K_LEFTARROW;
	case SDL_SCANCODE_RIGHT: return K_RIGHTARROW;

	case SDL_SCANCODE_LALT: return K_ALT;
	case SDL_SCANCODE_RALT: return K_ALT;
	case SDL_SCANCODE_LCTRL: return K_CTRL;
	case SDL_SCANCODE_RCTRL: return K_CTRL;
	case SDL_SCANCODE_LSHIFT: return K_SHIFT;
	case SDL_SCANCODE_RSHIFT: return K_SHIFT;

	case SDL_SCANCODE_F1: return K_F1;
	case SDL_SCANCODE_F2: return K_F2;
	case SDL_SCANCODE_F3: return K_F3;
	case SDL_SCANCODE_F4: return K_F4;
	case SDL_SCANCODE_F5: return K_F5;
	case SDL_SCANCODE_F6: return K_F6;
	case SDL_SCANCODE_F7: return K_F7;
	case SDL_SCANCODE_F8: return K_F8;
	case SDL_SCANCODE_F9: return K_F9;
	case SDL_SCANCODE_F10: return K_F10;
	case SDL_SCANCODE_F11: return K_F11;
	case SDL_SCANCODE_F12: return K_F12;
	case SDL_SCANCODE_INSERT: return K_INS;
	case SDL_SCANCODE_DELETE: return K_DEL;
	case SDL_SCANCODE_PAGEDOWN: return K_PGDN;
	case SDL_SCANCODE_PAGEUP: return K_PGUP;
	case SDL_SCANCODE_HOME: return K_HOME;
	case SDL_SCANCODE_END: return K_END;

	case SDL_SCANCODE_NUMLOCKCLEAR: return K_KP_NUMLOCK;
	case SDL_SCANCODE_KP_DIVIDE: return K_KP_SLASH;
	case SDL_SCANCODE_KP_MULTIPLY: return K_KP_STAR;
	case SDL_SCANCODE_KP_MINUS: return K_KP_MINUS;
	case SDL_SCANCODE_KP_7: return K_KP_HOME;
	case SDL_SCANCODE_KP_8: return K_KP_UPARROW;
	case SDL_SCANCODE_KP_9: return K_KP_PGUP;
	case SDL_SCANCODE_KP_PLUS: return K_KP_PLUS;
	case SDL_SCANCODE_KP_4: return K_KP_LEFTARROW;
	case SDL_SCANCODE_KP_5: return K_KP_5;
	case SDL_SCANCODE_KP_6: return K_KP_RIGHTARROW;
	case SDL_SCANCODE_KP_1: return K_KP_END;
	case SDL_SCANCODE_KP_2: return K_KP_DOWNARROW;
	case SDL_SCANCODE_KP_3: return K_KP_PGDN;
	case SDL_SCANCODE_KP_ENTER: return K_KP_ENTER;
	case SDL_SCANCODE_KP_0: return K_KP_INS;
	case SDL_SCANCODE_KP_PERIOD: return K_KP_DEL;

	case SDL_SCANCODE_LGUI: return K_COMMAND;
	case SDL_SCANCODE_RGUI: return K_COMMAND;

	case SDL_SCANCODE_CAPSLOCK: return K_CAPSLOCK;
	case SDL_SCANCODE_SCROLLLOCK: return K_SCROLLLOCK;

	case SDL_SCANCODE_PRINTSCREEN: return K_PRINTSCREEN;

	case SDL_SCANCODE_PAUSE: return K_PAUSE;

	default: return 0;
	}
}

static void IN_DebugTextEvent(SDL_Event *event)
{
	Con_Printf ("SDL_TEXTINPUT '%s' time: %g\n", event->text.text, Sys_DoubleTime());
}

static void IN_DebugKeyEvent(SDL_Event *event)
{
	const char *eventtype = (event->key.state == SDL_PRESSED) ? "SDL_KEYDOWN" : "SDL_KEYUP";
	Con_Printf ("%s scancode: '%s' keycode: '%s' time: %g\n",
		eventtype,
		SDL_GetScancodeName(event->key.keysym.scancode),
		SDL_GetKeyName(event->key.keysym.sym),
		Sys_DoubleTime());
}

void IN_StartGyroCalibration (void)
{
#if SDL_VERSION_ATLEAST (2, 0, 9)
	// Disable rumble temporarily
	if (joy_has_rumble)
		SDL_GameControllerRumble (joy_active_controller, 0, 0, 100);
#endif // SDL_VERSION_ATLEAST (2, 0, 9)

	gyro_accum[0] = 0.0;
	gyro_accum[1] = 0.0;
	gyro_accum[2] = 0.0;

	updates_countdown = GYRO_CALIBRATION_SAMPLES;

	// Note: we modify updates_countdown first, before printing the message to the console,
	// because Con_Printf triggers a redraw, which in turn calls M_Calibration_Draw,
	// which would see that updates_countown is 0 and consider that calibration must be done.
	// Alternatively, we could use Con_SafePrintf (which doesn't refresh the screen).

	Con_Printf ("Calibrating, please wait...\n");
}

static qboolean IN_UpdateGyroCalibration (const float newsample[3])
{
	if (!updates_countdown)
		return false;

	gyro_accum[0] += newsample[0];
	gyro_accum[1] += newsample[1];
	gyro_accum[2] += newsample[2];

	updates_countdown--;
	if (!updates_countdown)
	{
		const float inverseSamples = 1.f / GYRO_CALIBRATION_SAMPLES;
		Cvar_SetValue("gyro_calibration_x", gyro_accum[0] * inverseSamples);
		Cvar_SetValue("gyro_calibration_y", gyro_accum[1] * inverseSamples);
		Cvar_SetValue("gyro_calibration_z", gyro_accum[2] * inverseSamples);

		Con_Printf("Calibration results:\n X=%f Y=%f Z=%f\n",
			gyro_calibration_x.value,
			gyro_calibration_y.value,
			gyro_calibration_z.value);

		Con_Printf("Calibration finished\n");

		return false;
	}

	return true;
}

qboolean IN_HasGyro (void)
{
	return gyro_present;
}

float IN_GetRawGyroMagnitude (void)
{
	if (!gyro_present)
		return 0.f;
	return gyro_raw_mag;
}

float IN_GetRawLookMagnitude (void)
{
	joyaxis_t axis;
	if (!joy_active_controller)
		return 0.f;

	axis.x = joy_axisstate.axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX];
	axis.y = joy_axisstate.axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY];

	return IN_AxisMagnitude (axis);
}

float IN_GetRawMoveMagnitude (void)
{
	joyaxis_t axis;
	if (!joy_active_controller)
		return 0.f;

	axis.x = joy_axisstate.axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX];
	axis.y = joy_axisstate.axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY];

	return IN_AxisMagnitude (axis);
}

float IN_GetRawTriggerMagnitude (void)
{
	float left, right;
	if (!joy_active_controller)
		return 0.f;

	left = fabs (joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERLEFT]);
	right = fabs (joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERRIGHT]);

	return q_max (left, right);

}

qboolean IN_IsCalibratingGyro (void)
{
	return updates_countdown != 0;
}

float IN_GetGyroCalibrationProgress (void)
{
	return (GYRO_CALIBRATION_SAMPLES - updates_countdown) / (float) GYRO_CALIBRATION_SAMPLES;
}

static float IN_FilterGyroSample (float prev, float cur)
{
	float thresh = DEG2RAD (gyro_noise_thresh.value);
	float d = fabs (cur - prev);
	if (d < thresh)
	{
		d /= thresh;
		cur = LERP (prev, cur, 0.01f + 0.99f * d * d);
	}
	return cur;
}

void IN_SendKeyEvents (void)
{
	SDL_Event event;
	int key;
	qboolean down;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				Sys_ActivateKeyFilter(true);
				windowhasfocus = true;
				S_UnblockSound();
			}
			else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				windowhasfocus = false;
				S_BlockSound();
				Sys_ActivateKeyFilter(false);
			}
			else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				vid.width = event.window.data1;
				vid.height = event.window.data2;
				vid.resized = true;
			}
			break;
		case SDL_TEXTINPUT:
			lastactivetype = KD_KEYBOARD;

			if (in_debugkeys.value)
				IN_DebugTextEvent(&event);

		// SDL2: We use SDL_TEXTINPUT for typing in the console / chat.
		// SDL2 uses the local keyboard layout and handles modifiers
		// (shift for uppercase, etc.) for us.
			{
				unsigned char *ch;
				for (ch = (unsigned char *)event.text.text; *ch; ch++)
					if ((*ch & ~0x7F) == 0)
						Char_Event (*ch);
			}
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			down = (event.key.state == SDL_PRESSED);

			lastactivetype = KD_KEYBOARD;

			if (in_debugkeys.value)
				IN_DebugKeyEvent(&event);

		// SDL2: we interpret the keyboard as the US layout, so keybindings
		// are based on key position, not the label on the key cap.
			key = IN_SDL2_ScancodeToQuakeKey(event.key.keysym.scancode);

		// also pass along the underlying keycode using the proper current layout for Y/N prompts.
			Key_EventWithKeycode (key, down, event.key.keysym.sym);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (event.button.button < 1 ||
			    event.button.button > Q_COUNTOF(buttonremap))
			{
				Con_Printf ("Ignored event for mouse button %d\n",
							event.button.button);
				break;
			}
			lastactivetype = KD_MOUSE;
			if (key_dest == key_menu)
				M_Mousemove (event.button.x, event.button.y);
			else if (key_dest == key_console)
				Con_Mousemove (event.button.x, event.button.y);
			Key_Event(buttonremap[event.button.button - 1], event.button.state == SDL_PRESSED);
			break;

		case SDL_MOUSEWHEEL:
			lastactivetype = KD_MOUSE;
			if (event.wheel.y > 0)
			{
				Key_Event(K_MWHEELUP, true);
				Key_Event(K_MWHEELUP, false);
			}
			else if (event.wheel.y < 0)
			{
				Key_Event(K_MWHEELDOWN, true);
				Key_Event(K_MWHEELDOWN, false);
			}
			break;

		case SDL_MOUSEMOTION:
			IN_MouseMotion(event.motion.xrel, event.motion.yrel);
			break;

#if SDL_VERSION_ATLEAST(2, 0, 14)
		case SDL_CONTROLLERSENSORUPDATE:
			if (event.csensor.sensor == SDL_SENSOR_GYRO && event.csensor.which == joy_active_instanceid)
			{
				float prev_yaw = gyro_yaw;
				float prev_pitch = gyro_pitch;
				float prev_roll = gyro_roll;

				if (IN_UpdateGyroCalibration (event.csensor.data))
					break;

				// Gyro Axis Selection
				switch ((int)gyro_turning_axis.value) {
				case 0: // Yaw
					gyro_yaw = event.csensor.data[1] - gyro_calibration_y.value;
					break;
				case 1: // Roll
					gyro_yaw = -(event.csensor.data[2] - gyro_calibration_z.value);
					break;
				case 2: // Local Space
				{
					Vector3 localGyro = TransformToLocalSpace (
						event.csensor.data[1] - gyro_calibration_y.value,
						event.csensor.data[0] - gyro_calibration_x.value,
						event.csensor.data[2] - gyro_calibration_z.value,
						0.0f
					);

					gyro_yaw = localGyro.x;
					gyro_pitch = localGyro.y;
					gyro_roll = -localGyro.z;
				}
				break;

				case 3: // Player Space
				{
					Vector3 playerGyro = TransformToPlayerSpace (
						event.csensor.data[1] - gyro_calibration_y.value,
						event.csensor.data[0] - gyro_calibration_x.value,
						event.csensor.data[2] - gyro_calibration_z.value,
						GetGravityVector ()
					);

					gyro_yaw = playerGyro.x;
					gyro_pitch = playerGyro.y;
					gyro_roll = -playerGyro.z;
				}
				break;

				case 4: // World Space
				{
					Vector3 worldGyro = TransformToWorldSpace (
						event.csensor.data[1] - gyro_calibration_y.value,
						event.csensor.data[0] - gyro_calibration_x.value,
						event.csensor.data[2] - gyro_calibration_z.value,
						GetGravityVector ()
					);

					gyro_yaw = worldGyro.x;
					gyro_pitch = worldGyro.y;
					gyro_roll = -worldGyro.z;
				}
				break;

				default:
					gyro_yaw = 0.f;
					gyro_pitch = 0.f;
					gyro_roll = 0.f;
					break;
				}

				// Preserve direct pitch calibration
				gyro_pitch = event.csensor.data[0] - gyro_calibration_x.value;

				// Save unfiltered magnitude to display in the UI
				gyro_raw_mag = RAD2DEG (sqrt (gyro_yaw * gyro_yaw + gyro_pitch * gyro_pitch));

				// Apply filtering to smooth gyro movements
				gyro_yaw = IN_FilterGyroSample (prev_yaw, gyro_yaw);
				gyro_roll = IN_FilterGyroSample (prev_roll, gyro_roll);
				gyro_pitch = IN_FilterGyroSample (prev_pitch, gyro_pitch);
			}
			break;

#endif // SDL_VERSION_ATLEAST(2, 0, 14)

		case SDL_CONTROLLERDEVICEADDED:
			if (!IN_RemapJoystick ())
				IN_UseController (event.jdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
		case SDL_CONTROLLERDEVICEREMAPPED:
			if (!IN_RemapJoystick ())
				IN_SetupJoystick ();
			break;
#if SDL_VERSION_ATLEAST (2, 0, 14)
		case SDL_LOCALECHANGED:
			if (!q_strcasecmp (language.string, "auto"))
				language.callback (&language);
			break;
#endif

		case SDL_QUIT:
			CL_Disconnect ();
			Sys_Quit ();
			break;

		default:
			break;
		}
	}
}

