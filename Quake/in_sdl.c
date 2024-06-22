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

extern cvar_t ui_mouse;
extern cvar_t language;

static qboolean windowhasfocus = true;	//just in case sdl fails to tell us...
static textmode_t textmode = TEXTMODE_OFF;

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
cvar_t	joy_device = { "joy_device", "0", CVAR_ARCHIVE };

cvar_t gyro_enable = {"gyro_enable", "1", CVAR_ARCHIVE};
cvar_t gyro_mode = {"gyro_mode", "0", CVAR_ARCHIVE}; // see gyromode_t
cvar_t gyro_turning_axis = {"gyro_turning_axis", "0", CVAR_ARCHIVE};

cvar_t gyro_yawsensitivity = {"gyro_yawsensitivity", "2.5", CVAR_ARCHIVE};
cvar_t gyro_pitchsensitivity= {"gyro_pitchsensitivity", "2.5", CVAR_ARCHIVE};

cvar_t gyro_calibration_x = {"gyro_calibration_x", "0", CVAR_ARCHIVE};
cvar_t gyro_calibration_y = {"gyro_calibration_y", "0", CVAR_ARCHIVE};
cvar_t gyro_calibration_z = {"gyro_calibration_z", "0", CVAR_ARCHIVE};

cvar_t gyro_noise_thresh = {"gyro_noise_thresh", "1.5", CVAR_ARCHIVE};

cvar_t joy_led_enable = {"joy_led_enable", "1", CVAR_ARCHIVE}; // Enable changing LED
cvar_t joy_led_r = {"joy_led_r", "0.3", CVAR_ARCHIVE};
cvar_t joy_led_g = {"joy_led_g", "0.07", CVAR_ARCHIVE};
cvar_t joy_led_b = {"joy_led_b", "0.0", CVAR_ARCHIVE};

cvar_t joy_ds_debug = {"joy_ds_debug", "0", CVAR_NONE};

cvar_t joy_ds_rt_mode              = {"joy_ds_rt_mode", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_startpos          = {"joy_ds_rt_startpos", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_endpos            = {"joy_ds_rt_endpos", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_strength          = {"joy_ds_rt_strength", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_snapforce         = {"joy_ds_rt_snapforce", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_frequency         = {"joy_ds_rt_frequency", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_period            = {"joy_ds_rt_period", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_gallop_firstfoot  = {"joy_ds_rt_gallop_firstfoot", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_gallop_secondfoot = {"joy_ds_rt_gallop_secondfoot", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_amplitude_a       = {"joy_ds_rt_amplitude_a", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_amplitude_b       = {"joy_ds_rt_amplitude_b", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_slope_start       = {"joy_ds_rt_slope_start", "0", CVAR_ARCHIVE};
cvar_t joy_ds_rt_slope_end         = {"joy_ds_rt_slope_end", "0", CVAR_ARCHIVE};

cvar_t joy_ds_lt_mode              = {"joy_ds_lt_mode", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_startpos          = {"joy_ds_lt_startpos", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_endpos            = {"joy_ds_lt_endpos", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_strength          = {"joy_ds_lt_strength", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_snapforce         = {"joy_ds_lt_snapforce", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_frequency         = {"joy_ds_lt_frequency", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_period            = {"joy_ds_lt_period", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_gallop_firstfoot  = {"joy_ds_lt_gallop_firstfoot", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_gallop_secondfoot = {"joy_ds_lt_gallop_secondfoot", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_amplitude_a       = {"joy_ds_lt_amplitude_a", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_amplitude_b       = {"joy_ds_lt_amplitude_b", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_slope_start       = {"joy_ds_lt_slope_start", "0", CVAR_ARCHIVE};
cvar_t joy_ds_lt_slope_end         = {"joy_ds_lt_slope_end", "0", CVAR_ARCHIVE};

static SDL_JoystickID joy_active_instanceid = -1;
static int joy_active_device = -1;
static SDL_GameController *joy_active_controller = NULL;
static char joy_active_name[256];

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
static int	total_dx, total_dy = 0;
static float gyro_yaw, gyro_pitch = 0;

// used for gyro calibration
static float gyro_accum[3];
static unsigned int num_samples;
static unsigned int updates_countdown = 0;

static qboolean gyro_present = false;
static qboolean gyro_button_pressed = false;

static qboolean led_present = false;
static vec3_t joy_led = {0.f, 0.f, 0.f};
static vec3_t prev_joy_led = {-1.f, -1.f, -1.f};

// Sony DualSense related code. An info was taken from these resources (huge thanks!):
// https://controllers.fandom.com/wiki/Sony_DualSense
// https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db
// https://github.com/nowrep/dualsensectl
static qboolean ds_triggers_present = false;
// Enums for output effect
enum ds_trigger_state {
	// Official supported modes
	tm_off = 0x05,
	tm_feedback = 0x21,
	tm_bow = 0x22,
	tm_galloping = 0x23,
	tm_weapon = 0x25,
	tm_vibration = 0x26,
	tm_machine = 0x27,
	tm_simple_feedback = 0x01,
	tm_simple_weapon = 0x02,
	tm_simple_vibration = 0x06,
	tm_limited_feedback = 0x11,
	tm_limited_weapon = 0x12,
};
// These are fields we use
#define DS_ENABLE_BITS1 0
#define DS_RT_BYTES 10
#define DS_LT_BYTES 21
static uint8_t ds_effects_state[47] = {0};
// If we are unable to read triggers status from HID, use these as fallback
static float ds_rt_threshold = 0;
static float ds_lt_threshold = 0;
// DualSense HID report
#define DS_REPORT_ID_USB 0x1
#define DS_REPORT_ID_BT 0x31
#define DS_REPORT_SIZE_USB 64
#define DS_REPORT_SIZE_BT 78
#define DS_REPORT_SIZE_MAX (DS_REPORT_SIZE_BT * 2)
static uint8_t ds_input_report[DS_REPORT_SIZE_MAX];
static int ds_report_size = 0; // >0 assumes we have received report
static int ds_report_offset = 0; // >0 assumes we have correct report id and size
// Nybbles report applied trigger input effect.
// -1 means trigger doesn't have effect applied.
static int ds_rt_nybble = -1;
static int ds_lt_nybble = -1;

// Using separate thread for reading input report
#if SDL_VERSION_ATLEAST(2, 0, 18)
static qboolean hidapi_init = false;
static SDL_hid_device *joy_active_hid = NULL;
static SDL_atomic_t hid_read_cancel;
static SDL_Thread *hid_read_thread;
#endif

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
	if (SDL_SetRelativeMouseMode(SDL_FALSE) != 0)
		Con_Printf("WARNING: could not disable relative mouse mode (%s).\n", SDL_GetError());
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

#if SDL_VERSION_ATLEAST(2, 0, 14)
void IN_UpdateLED (void)
{
	if (!joy_led_enable.value)
		return;

	if (IN_HasLED ())
	{
		joy_led[0] = CLAMP (0, joy_led_r.value, 1);
		joy_led[1] = CLAMP (0, joy_led_g.value, 1);
		joy_led[2] = CLAMP (0, joy_led_b.value, 1);

#define BLEND_COLOR(color, base_color, add_color, add_scale) \
		color = base_color + ( add_color - base_color ) * add_scale;

		// Blend color shifts on top of current color. Doing almost the same as V_CalcBlend from view.c
		// V_PolyBlend resets v_blend alpha component, on some machines it prevents producing effect.
		vec3_t v_blend_led = { 0.f, 0.f, 0.f };
		float v_blend_scale = 0.f;
		float r, g, b, a, a2;

		r = 0;
		g = 0;
		b = 0;
		a = 0;

		for (int j = 0; j < NUM_CSHIFTS; j++)
		{
			//johnfitz -- only apply leaf contents color shifts during intermission
			if (cl.intermission && j != CSHIFT_CONTENTS)
				continue;
			//johnfitz

			a2 = cl.cshifts[j].percent / 255.0;
			// QuakeSpasm
			if (!a2)
				continue;
			a = a + a2 * (1 - a);
			a2 = a2 / a;
			r = r * (1 - a2) + cl.cshifts[j].destcolor[0] * a2;
			g = g * (1 - a2) + cl.cshifts[j].destcolor[1] * a2;
			b = b * (1 - a2) + cl.cshifts[j].destcolor[2] * a2;
		}

		v_blend_led[0] = r / 255;
		v_blend_led[1] = g / 255;
		v_blend_led[2] = b / 255;
		v_blend_scale = CLAMP (0, a, 1);

		v_blend_scale = pow (sin (v_blend_scale * M_PI), 0.5);
		VectorScale (v_blend_led, v_blend_scale, v_blend_led);
		BLEND_COLOR (joy_led[0], joy_led[0], v_blend_led[0], v_blend_scale);
		BLEND_COLOR (joy_led[1], joy_led[1], v_blend_led[1], v_blend_scale);
		BLEND_COLOR (joy_led[2], joy_led[2], v_blend_led[2], v_blend_scale);

		// Blend flash from view punches (gunfire)
		vec3_t punch_led;
		_VectorCopy (joy_led, punch_led);
		VectorNormalize (punch_led); // Get maximum LED color brightness

		float punchblend = CLAMP (0, (cl.time - cl.punchtime), 1);// / 0.1f;

		punchblend = (1 - punchblend);// * 0.2;

		VectorScale (punch_led, punchblend, punch_led);
		BLEND_COLOR (joy_led[0], joy_led[0], punch_led[0], punchblend);
		BLEND_COLOR (joy_led[1], joy_led[1], punch_led[1], punchblend);
		BLEND_COLOR (joy_led[2], joy_led[2], punch_led[2], punchblend);

		joy_led[0] = CLAMP (0, joy_led[0], 1);
		joy_led[1] = CLAMP (0, joy_led[1], 1);
		joy_led[2] = CLAMP (0, joy_led[2], 1);

		qboolean led_changed = false;

		for (int i = 0; i < 3; i++)
		{
			if (joy_led[i] != prev_joy_led[i])
			{
				led_changed = true;
				prev_joy_led[i] = joy_led[i];
			}
		}

		if (led_changed)
			SDL_GameControllerSetLED (joy_active_controller, joy_led[0] * 255, joy_led[1] * 255, joy_led[2] * 255);
	}
}
#endif // SDL_VERSION_ATLEAST(2, 0, 14)

const char* IN_GetDSTriggerModeName (int mode)
{
	switch (mode)
	{
	case DS_TRIGGER_OFF:
	default:
		return "Off";
	case DS_TRIGGER_WEAPON:
		return "Weapon";
	case DS_TRIGGER_FEEDBACK:
		return "Feedback";
	case DS_TRIGGER_SLOPE:
		return "Slope";
	case DS_TRIGGER_VIBRATION:
		return "Vibration";
	case DS_TRIGGER_BOW:
		return "Bow";
	case DS_TRIGGER_GALLOPING:
		return "Galloping";
	case DS_TRIGGER_MACHINE:
		return "Machine";
	case DS_TRIGGER_SIMPLE_FEEDBACK:
		return "Simple Feedback (bugged)";
	case DS_TRIGGER_SIMPLE_WEAPON:
		return "Simple Weapon (bugged)";
	case DS_TRIGGER_SIMPLE_VIBRATION:
		return "Simple Vibration (bugged)";
	case DS_TRIGGER_LIMITED_FEEDBACK:
		return "Limited Feedback (bugged)";
	case DS_TRIGGER_LIMITED_WEAPON:
		return "Limited Weapon (bugged)";
	}
}

#if SDL_VERSION_ATLEAST(2, 0, 16)
void IN_SetupDSTrigger (qboolean right_trigger)
{
	int mode = 0;
	uint8_t startpos = 0;
	uint8_t endpos = 0;
	uint8_t strength = 0;
	uint8_t snapforce = 0;
	uint8_t frequency = 0;
	uint8_t period = 0;
	uint8_t gallop_firstfoot = 0;
	uint8_t gallop_secondfoot = 0;
	uint8_t amplitude_a = 0;
	uint8_t amplitude_b = 0;
	uint8_t slope_start = 0;
	uint8_t slope_end = 0;

	uint16_t startandstop = 0;
	uint32_t force_zones  = 0;
	uint16_t active_zones = 0;

	int trigger_byte_fields;
	float *ds_trigger_threshold;

	if (right_trigger)
	{
		mode = q_max (0, joy_ds_rt_mode.value);

		startpos          = joy_ds_rt_startpos.value;
		endpos            = joy_ds_rt_endpos.value;
		strength          = joy_ds_rt_strength.value;
		snapforce         = joy_ds_rt_snapforce.value;
		frequency         = joy_ds_rt_frequency.value;
		period            = joy_ds_rt_period.value;
		gallop_firstfoot  = joy_ds_rt_gallop_firstfoot.value;
		gallop_secondfoot = joy_ds_rt_gallop_secondfoot.value;
		amplitude_a       = joy_ds_rt_amplitude_a.value;
		amplitude_b       = joy_ds_rt_amplitude_b.value;
		slope_start       = joy_ds_rt_slope_start.value;
		slope_end         = joy_ds_rt_slope_end.value;

		trigger_byte_fields = DS_RT_BYTES;
		ds_trigger_threshold = &ds_rt_threshold;
	}
	else
	{
		mode = q_max (0, joy_ds_lt_mode.value);

		startpos          = joy_ds_lt_startpos.value;
		endpos            = joy_ds_lt_endpos.value;
		strength          = joy_ds_lt_strength.value;
		snapforce         = joy_ds_lt_snapforce.value;
		frequency         = joy_ds_lt_frequency.value;
		period            = joy_ds_lt_period.value;
		gallop_firstfoot  = joy_ds_lt_gallop_firstfoot.value;
		gallop_secondfoot = joy_ds_lt_gallop_secondfoot.value;
		amplitude_a       = joy_ds_lt_amplitude_a.value;
		amplitude_b       = joy_ds_lt_amplitude_b.value;
		slope_start       = joy_ds_lt_slope_start.value;
		slope_end         = joy_ds_lt_slope_end.value;

		trigger_byte_fields = DS_LT_BYTES;
		ds_trigger_threshold = &ds_lt_threshold;
	}

	switch (mode)
	{
		case DS_TRIGGER_WEAPON:
			startpos = CLAMP (2, startpos, 6); // up to 7 is valid
			endpos = CLAMP (startpos + 1, endpos, 7); // up to 8 is valid
			strength = CLAMP (1, strength, 8);

			startandstop = (uint16_t)((1 << startpos) | (1 << endpos));

			ds_effects_state[trigger_byte_fields +  0] = tm_weapon;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((startandstop >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((startandstop >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = strength-1;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = (endpos+2) / 10.f; //at 8 it fails to trigger
			break;
		case DS_TRIGGER_FEEDBACK:
			startpos = CLAMP (1, startpos, 9); //from 0 is valid, but always triggers as pressed
			strength = CLAMP (1, strength, 8);

			strength = (strength - 1) & 0x07;
			for (int i = startpos; i < 10; i++)
			{
				force_zones  |= (uint32_t)(strength << (3 * i));
				active_zones |= (uint16_t)(1 << i);
			}

			ds_effects_state[trigger_byte_fields +  0] = tm_feedback;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((active_zones >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((active_zones >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = (uint8_t)((force_zones >>  0) & 0xff);
			ds_effects_state[trigger_byte_fields +  4] = (uint8_t)((force_zones >>  8) & 0xff);
			ds_effects_state[trigger_byte_fields +  5] = (uint8_t)((force_zones >> 16) & 0xff);
			ds_effects_state[trigger_byte_fields +  6] = (uint8_t)((force_zones >> 24) & 0xff);
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 10.f;
			break;
		case DS_TRIGGER_SLOPE:
			startpos = CLAMP (1, startpos, 8); //from 0 is valid, but always triggers as pressed
			endpos = CLAMP (startpos+1, endpos, 9);
			slope_start = CLAMP (1, slope_start, 8);
			slope_end = CLAMP (1, slope_end, 8);

			uint8_t strengths[10] = {0};
			float slope = 1.0f * (slope_end - slope_start) / (endpos - startpos);
			for (int i = (int)startpos; i < 10; i++) {
                if (i <= endpos)
                    strengths[i] = (uint8_t)roundf(slope_start + slope * (i - startpos));
                else
                    strengths[i] = slope_end;
				strength = (uint8_t)((strengths[i] - 1) & 0x07);
				force_zones  |= (uint32_t)(strength << (3 * i));
				active_zones |= (uint16_t)(1 << i);
			}

			ds_effects_state[trigger_byte_fields +  0] = tm_feedback;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((active_zones >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((active_zones >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = (uint8_t)((force_zones >>  0) & 0xff);
			ds_effects_state[trigger_byte_fields +  4] = (uint8_t)((force_zones >>  8) & 0xff);
			ds_effects_state[trigger_byte_fields +  5] = (uint8_t)((force_zones >> 16) & 0xff);
			ds_effects_state[trigger_byte_fields +  6] = (uint8_t)((force_zones >> 24) & 0xff);
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 10.f;
			break;
		case DS_TRIGGER_VIBRATION:
			startpos = CLAMP (1, startpos, 9); //from 0 is valid, but always triggers as pressed
			strength = CLAMP (1, strength, 8); //Nielk1 specs states it's 0 at minimum, probably typo? 

			strength = (strength - 1) & 0x07;
			for (int i = startpos; i < 10; i++)
			{
				force_zones  |= (uint32_t)(strength << (3 * i));
				active_zones |= (uint16_t)(1 << i);
			}

			ds_effects_state[trigger_byte_fields +  0] = tm_vibration;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((active_zones >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((active_zones >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = (uint8_t)((force_zones >>  0) & 0xff);
			ds_effects_state[trigger_byte_fields +  4] = (uint8_t)((force_zones >>  8) & 0xff);
			ds_effects_state[trigger_byte_fields +  5] = (uint8_t)((force_zones >> 16) & 0xff);
			ds_effects_state[trigger_byte_fields +  6] = (uint8_t)((force_zones >> 24) & 0xff);
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = frequency;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 10.f;
			break;
		case DS_TRIGGER_BOW:
			startpos = CLAMP (1, startpos, 7); //from 0 is valid, but always triggers as pressed
			endpos = CLAMP (startpos+1, endpos, 8);
			// Specs again say minimal for these two allowed are 0, i'm not sure...
			strength = CLAMP (1, strength, 8);
			snapforce = CLAMP (1, snapforce, 8);

			startandstop = (uint16_t)((1 << startpos) | (1 << endpos));
			uint32_t force_pair = (uint32_t)((((strength  - 1) & 0x07) << (3 * 0)) | (((snapforce - 1) & 0x07) << (3 * 1)));

			ds_effects_state[trigger_byte_fields +  0] = tm_bow;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((startandstop >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((startandstop >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = (uint8_t)((force_pair >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  4] = (uint8_t)((force_pair >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = endpos / 10.f;
			break;
		case DS_TRIGGER_GALLOPING:
			startpos = CLAMP (0, startpos, 8);
			endpos = CLAMP (startpos+1, endpos, 9);
			gallop_firstfoot = CLAMP (0, gallop_firstfoot, 6);
			gallop_secondfoot = CLAMP (gallop_firstfoot+1, gallop_secondfoot, 7);

			startandstop = (uint16_t)((1 << startpos) | (1 << endpos));
			uint32_t timeandratio = (uint32_t)(((gallop_secondfoot & 0x07) << (3 * 0)) | ((gallop_firstfoot & 0x07) << (3 * 1)));

			ds_effects_state[trigger_byte_fields +  0] = tm_galloping;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((startandstop >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((startandstop >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = (uint8_t)((timeandratio >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  4] = frequency;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 10.f;
			break;
		case DS_TRIGGER_MACHINE:
			startpos = CLAMP (1, startpos, 8);
			endpos = CLAMP (startpos+1, endpos, 9);
			amplitude_a = CLAMP (0, amplitude_a, 7);
			amplitude_b = CLAMP (0, amplitude_b, 7);

			startandstop = (uint16_t)((1 << startpos) | (1 << endpos));
			uint32_t strength_pair = (uint32_t)(((amplitude_a & 0x07) << (3 * 0)) | ((amplitude_b & 0x07) << (3 * 1)));

			ds_effects_state[trigger_byte_fields +  0] = tm_machine;
			ds_effects_state[trigger_byte_fields +  1] = (uint8_t)((startandstop >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  2] = (uint8_t)((startandstop >> 8) & 0xff);
			ds_effects_state[trigger_byte_fields +  3] = (uint8_t)((strength_pair >> 0) & 0xff);
			ds_effects_state[trigger_byte_fields +  4] = frequency;
			ds_effects_state[trigger_byte_fields +  5] = period;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 10.f;
			break;
		case DS_TRIGGER_SIMPLE_FEEDBACK:
			startpos = CLAMP (0, startpos, 9);
			strength = CLAMP (0, strength, 10);

			startpos = (uint8_t)((float)startpos / 10 * 255);
			strength = (uint8_t)((float)strength / 10 * 255);

			ds_effects_state[trigger_byte_fields +  0] = tm_simple_feedback;
			ds_effects_state[trigger_byte_fields +  1] = startpos;
			ds_effects_state[trigger_byte_fields +  2] = strength;
			ds_effects_state[trigger_byte_fields +  3] = 0;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 255.f;
			break;
		case DS_TRIGGER_SIMPLE_WEAPON:
			startpos = CLAMP (0, startpos, 8);
			endpos = CLAMP (startpos+1, endpos, 9);
			strength = CLAMP (0, strength, 10);

			startpos = (uint8_t)((float)startpos / 10 * 255);
			endpos = (uint8_t)((float)endpos / 10 * 255);
			strength = (uint8_t)((float)strength / 10 * 255);

			ds_effects_state[trigger_byte_fields +  0] = tm_simple_weapon;
			ds_effects_state[trigger_byte_fields +  1] = startpos;
			ds_effects_state[trigger_byte_fields +  2] = endpos;
			ds_effects_state[trigger_byte_fields +  3] = strength;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = endpos / 255.f;
			break;
		case DS_TRIGGER_SIMPLE_VIBRATION:
			startpos = CLAMP (0, startpos, 8);
			strength = CLAMP (0, strength, 10);

			startpos = (uint8_t)((float)startpos / 10 * 255);
			strength = (uint8_t)((float)strength / 10 * 255);

			ds_effects_state[trigger_byte_fields +  0] = tm_simple_vibration;
			ds_effects_state[trigger_byte_fields +  1] = frequency;
			ds_effects_state[trigger_byte_fields +  2] = strength;
			ds_effects_state[trigger_byte_fields +  3] = startpos;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 255.f;
			break;
		case DS_TRIGGER_LIMITED_FEEDBACK:
			// Broken, stops producing effect after first press
			startpos = CLAMP (0, startpos, 9);
			strength = CLAMP (1, strength, 10);

			startpos = (uint8_t)((float)startpos / 10 * 255);
			strength = (uint8_t)((float)strength / 10 * 255);

			ds_effects_state[trigger_byte_fields +  0] = tm_limited_feedback;
			ds_effects_state[trigger_byte_fields +  1] = startpos;
			ds_effects_state[trigger_byte_fields +  2] = strength;
			ds_effects_state[trigger_byte_fields +  3] = 0;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = startpos / 255.f;
			break;
		case DS_TRIGGER_LIMITED_WEAPON:
			startpos = CLAMP (0, startpos, 9);
			endpos = CLAMP (0, endpos, 9);
			strength = CLAMP (0, strength, 10);

			startpos = (uint8_t)((float)startpos / 10 * 255);
			endpos = (uint8_t)((float)endpos / 10 * 255);

			startpos = CLAMP (16, startpos, 255 - 100 - 16);
			endpos = CLAMP (startpos, endpos, startpos + 100);

			ds_effects_state[trigger_byte_fields +  0] = tm_limited_weapon;
			ds_effects_state[trigger_byte_fields +  1] = startpos;
			ds_effects_state[trigger_byte_fields +  2] = endpos;
			ds_effects_state[trigger_byte_fields +  3] = strength;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = endpos / 255.f;
			break;
		case DS_TRIGGER_OFF:
		default:
			ds_effects_state[trigger_byte_fields +  0] = tm_off;
			ds_effects_state[trigger_byte_fields +  1] = 0;
			ds_effects_state[trigger_byte_fields +  2] = 0;
			ds_effects_state[trigger_byte_fields +  3] = 0;
			ds_effects_state[trigger_byte_fields +  4] = 0;
			ds_effects_state[trigger_byte_fields +  5] = 0;
			ds_effects_state[trigger_byte_fields +  6] = 0;
			ds_effects_state[trigger_byte_fields +  7] = 0;
			ds_effects_state[trigger_byte_fields +  8] = 0;
			ds_effects_state[trigger_byte_fields +  9] = 0;
			ds_effects_state[trigger_byte_fields + 10] = 0;

			*ds_trigger_threshold = 0.f;
			break;
	}
}

void IN_SetupDSTriggers (void)
{
	if (!IN_HasAdaptiveTriggers ())
		return;
	ds_effects_state[DS_ENABLE_BITS1] = (1<<2) | (1<<3); // Enable triggers effects setting
	IN_SetupDSTrigger (0);
	IN_SetupDSTrigger (1);
	SDL_GameControllerSendEffect (joy_active_controller, ds_effects_state, sizeof(ds_effects_state) / sizeof(ds_effects_state[0]));
}
#endif // SDL_VERSION_ATLEAST(2, 0, 16)

void IN_ResetCurrentController (void)
{
	if (!joy_active_controller)
		return;

	// to enable LED update when switch gamepad back and forth
	prev_joy_led[0] = -1.f;
	prev_joy_led[1] = -1.f;
	prev_joy_led[2] = -1.f;
#if SDL_VERSION_ATLEAST(2, 0, 16)
	// Reset to player index 1 and blue LED, default ones
	SDL_GameControllerSetPlayerIndex (joy_active_controller, 0);
	SDL_GameControllerSetLED (joy_active_controller, 0, 0, 64);
	if (SDL_GameControllerGetType (joy_active_controller) == SDL_CONTROLLER_TYPE_PS5 )
	{
		ds_effects_state[DS_ENABLE_BITS1] = (1<<2) | (1<<3); // Enable triggers effects setting
		ds_effects_state[DS_RT_BYTES +  0] = tm_off;
		ds_effects_state[DS_LT_BYTES +  0] = tm_off;
		SDL_GameControllerSendEffect (joy_active_controller, ds_effects_state, sizeof(ds_effects_state) / sizeof(ds_effects_state[0]));
	}
#endif // SDL_VERSION_ATLEAST(2, 0, 16)
}

static void DS_Triggers_cvar_callback (cvar_t *cvar)
{
#if SDL_VERSION_ATLEAST(2, 0, 16)
	IN_SetupDSTriggers ();
#endif // SDL_VERSION_ATLEAST(2, 0, 16)
}

static void Joy_DS_Mode_Completion_f (cvar_t *cvar, const char *partial)
{
	for (int i = 0; i < DS_TRIGGER_COUNT; i++)
		Con_AddToTabList (va ("%d", i), partial, IN_GetDSTriggerModeName (i));
}

#if SDL_VERSION_ATLEAST(2, 0, 18) //Required for HIDAPI
static int Read_HID (void* unused)
{
	while (!(SDL_AtomicGet (&hid_read_cancel)))
	{
		// Read DualSense HID input report
		uint8_t ds_report_id = 0x0;

		if (IN_HasAdaptiveTriggers () && hidapi_init && joy_active_hid)
		{
			ds_report_size = SDL_hid_read (joy_active_hid, ds_input_report, DS_REPORT_SIZE_MAX);
			if ((ds_report_size == -1) && (joy_ds_debug.value)) Con_Warning ("HID Read fail\n");
			if ((ds_report_size == 0) && (joy_ds_debug.value)) Con_Warning ("HID Read no data\n");
		}

		if (ds_report_size > 0)
		{
			ds_report_id = ds_input_report[0];
			if (joy_ds_debug.value >= 2) {
				Con_Printf ("Bytes read %d\n", ds_report_size);
				Con_Printf ("HID ReportID = 0x%x\n", ds_report_id);
			}
			if (((ds_report_size == DS_REPORT_SIZE_USB) && (ds_report_id == DS_REPORT_ID_USB))) ds_report_offset = 1;
			else if (((ds_report_size == DS_REPORT_SIZE_BT) && (ds_report_id == DS_REPORT_ID_BT))) ds_report_offset = 2;
			else { if (joy_ds_debug.value) Con_Warning ("Incorrect HID report id and size\n"); }
		}
	}
	return 0;
}
#endif //SDL_VERSION_ATLEAST(2, 0, 18)

static qboolean IN_UseController (int device_index)
{
	SDL_GameController *gamecontroller;
	const char *controllername;

	if (device_index == joy_active_device)
		return true;

	if (joy_active_device != -1)
	{
		IN_ResetCurrentController ();
		SDL_GameControllerClose (joy_active_controller);

		// Only show "gamepad removed" message when disabling the gamepad altogether,
		// not when switching to a different device
		if (device_index == -1)
			Con_Printf ("Gamepad removed: %s\n", joy_active_name);

		joy_active_name[0] = '\0';
		joy_active_controller = NULL;
		joy_active_instanceid = -1;
		joy_active_device = -1;
		Cvar_SetValueQuick (&joy_device, -1);
		gyro_present = false;
		gyro_yaw = gyro_pitch = 0.f;
		led_present = false;
		ds_triggers_present = false;
#if SDL_VERSION_ATLEAST(2, 0, 18)
		SDL_AtomicSet (&hid_read_cancel, 1);
		SDL_WaitThread (hid_read_thread, NULL);
		hid_read_thread = NULL;
#endif
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

#if SDL_VERSION_ATLEAST(2, 0, 14)
	SDL_GameControllerSetPlayerIndex (joy_active_controller, 0);
	if (SDL_GameControllerHasLED (joy_active_controller))
	{
		led_present = true;
	}
	if (SDL_GameControllerHasSensor (joy_active_controller, SDL_SENSOR_GYRO)
		&& !SDL_GameControllerSetSensorEnabled (joy_active_controller, SDL_SENSOR_GYRO, SDL_TRUE))
	{
		gyro_present = true;
#if SDL_VERSION_ATLEAST(2, 0, 16)
		Con_Printf ("Gyro sensor enabled at %g Hz\n", SDL_GameControllerGetSensorDataRate (joy_active_controller, SDL_SENSOR_GYRO));
#else
		Con_printf ("Gyro sensor enabled.\n")
#endif // SDL_VERSION_ATLEAST(2, 0, 16)
	}
	else
	{
		Con_Printf ("Gyro sensor not found\n");
	}
#if SDL_VERSION_ATLEAST(2, 0, 16)
	if (SDL_GameControllerGetType (joy_active_controller) == SDL_CONTROLLER_TYPE_PS5 )
	{
		ds_triggers_present = true;
		IN_SetupDSTriggers ();
	}
#endif // SDL_VERSION_ATLEAST(2, 0, 16)
#endif // SDL_VERSION_ATLEAST(2, 0, 14)

#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_hid_device* hid_device;
	uint16_t vendor_id;
	uint16_t product_id;

	if (hidapi_init)
	{
		SDL_AtomicSet (&hid_read_cancel, 1);
		SDL_WaitThread (hid_read_thread, NULL);
		hid_read_thread = NULL;
		SDL_hid_close (joy_active_hid);

		vendor_id = SDL_GameControllerGetVendor (joy_active_controller);
		product_id = SDL_GameControllerGetProduct (joy_active_controller);

		hid_device = SDL_hid_open (vendor_id, product_id, NULL);

		if (hid_device)
		{
			joy_active_hid = hid_device;
			Con_Printf ("Opened HID for %s\n", joy_active_name);

			SDL_AtomicSet (&hid_read_cancel, 0);
			hid_read_thread = SDL_CreateThread (Read_HID, "HID Read Thread", NULL);
		}
	}
#endif // SDL_VERSION_ATLEAST(2, 0, 18)

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
	
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1 )
	{
		Con_Warning("could not initialize SDL Game Controller\n");
		return;
	}
	
	// Load additional SDL2 controller definitions from gamecontrollerdb.txt
	for (i = 0; i < com_numbasedirs; i++)
	{
		q_snprintf (controllerdb, sizeof(controllerdb), "%s/gamecontrollerdb.txt", com_basedirs[i]);
		nummappings = SDL_GameControllerAddMappingsFromFile(controllerdb);
		if (nummappings > 0)
			Con_Printf("%d mappings loaded from gamecontrollerdb.txt\n", nummappings);
	}

	IN_SetupJoystick ();
}

void IN_StartupHIDAPI (void)
{
#if SDL_VERSION_ATLEAST(2, 0, 18)
	if (SDL_hid_init () == -1)
	{
		Con_Warning ("could not initialize SDL HIDAPI\n");
		return;
	}
	hidapi_init = true;
#endif
}

void IN_ShutdownJoystick (void)
{
	IN_ResetCurrentController ();
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void IN_ShutdownHIDAPI (void)
{
#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_AtomicSet (&hid_read_cancel, 1);
	SDL_WaitThread (hid_read_thread, NULL);
	hid_read_thread = NULL;
	SDL_hid_exit ();
#endif
}

qboolean IN_HasGamepad (void)
{
	return joy_active_controller != NULL;
}

const char *IN_GetGamepadName (void)
{
	return joy_active_controller ? joy_active_name : NULL;
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

void IN_Init (void)
{
	textmode = Key_TextEntry();

	if (textmode == TEXTMODE_ON)
		SDL_StartTextInput();
	else
		SDL_StopTextInput();

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
	Cvar_RegisterVariable(&joy_device);
	Cvar_SetCallback(&joy_device, Joy_Device_f);
	Cvar_SetCompletion(&joy_device, Joy_Device_Completion_f);

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

	Cvar_RegisterVariable(&joy_led_enable);
	Cvar_RegisterVariable(&joy_led_r);
	Cvar_RegisterVariable(&joy_led_g);
	Cvar_RegisterVariable(&joy_led_b);

	Cvar_RegisterVariable(&joy_ds_debug);

	Cvar_RegisterVariable(&joy_ds_rt_mode);
	Cvar_RegisterVariable(&joy_ds_rt_startpos);
	Cvar_RegisterVariable(&joy_ds_rt_endpos);
	Cvar_RegisterVariable(&joy_ds_rt_strength);
	Cvar_RegisterVariable(&joy_ds_rt_snapforce);
	Cvar_RegisterVariable(&joy_ds_rt_frequency);
	Cvar_RegisterVariable(&joy_ds_rt_period);
	Cvar_RegisterVariable(&joy_ds_rt_gallop_firstfoot);
	Cvar_RegisterVariable(&joy_ds_rt_gallop_secondfoot);
	Cvar_RegisterVariable(&joy_ds_rt_amplitude_a);
	Cvar_RegisterVariable(&joy_ds_rt_amplitude_b);
	Cvar_RegisterVariable(&joy_ds_rt_slope_start);
	Cvar_RegisterVariable(&joy_ds_rt_slope_end);
	
	Cvar_RegisterVariable(&joy_ds_lt_mode);
	Cvar_RegisterVariable(&joy_ds_lt_startpos);
	Cvar_RegisterVariable(&joy_ds_lt_endpos);
	Cvar_RegisterVariable(&joy_ds_lt_strength);
	Cvar_RegisterVariable(&joy_ds_lt_snapforce);
	Cvar_RegisterVariable(&joy_ds_lt_frequency);
	Cvar_RegisterVariable(&joy_ds_lt_period);
	Cvar_RegisterVariable(&joy_ds_lt_gallop_firstfoot);
	Cvar_RegisterVariable(&joy_ds_lt_gallop_secondfoot);
	Cvar_RegisterVariable(&joy_ds_lt_amplitude_a);
	Cvar_RegisterVariable(&joy_ds_lt_amplitude_b);
	Cvar_RegisterVariable(&joy_ds_lt_slope_start);
	Cvar_RegisterVariable(&joy_ds_lt_slope_end);

	Cvar_SetCallback(&joy_ds_rt_mode, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_startpos, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_endpos, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_strength, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_snapforce, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_frequency, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_period, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_gallop_firstfoot, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_gallop_secondfoot, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_amplitude_a, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_amplitude_b, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_slope_start, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_rt_slope_end, DS_Triggers_cvar_callback);
	
	Cvar_SetCallback(&joy_ds_lt_mode, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_startpos, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_endpos, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_strength, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_snapforce, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_frequency, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_period, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_gallop_firstfoot, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_gallop_secondfoot, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_amplitude_a, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_amplitude_b, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_slope_start, DS_Triggers_cvar_callback);
	Cvar_SetCallback(&joy_ds_lt_slope_end, DS_Triggers_cvar_callback);

	Cvar_SetCompletion(&joy_ds_rt_mode, Joy_DS_Mode_Completion_f);
	Cvar_SetCompletion(&joy_ds_lt_mode, Joy_DS_Mode_Completion_f);

	IN_Activate();
	IN_StartupHIDAPI();
	IN_StartupJoystick();
	Sys_ActivateKeyFilter(true);
}

void IN_Shutdown (void)
{
	Sys_ActivateKeyFilter(false);
	IN_Deactivate(true);
	IN_ShutdownHIDAPI();
	IN_ShutdownJoystick();
}

extern cvar_t cl_maxpitch; /* johnfitz -- variable pitch clamping */
extern cvar_t cl_minpitch; /* johnfitz -- variable pitch clamping */
extern cvar_t scr_fov;


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
				Key_Event(key, true);
			}
		}
		else
		{
			*timer = 0;
			Key_Event(key, false);
		}
	}
	else
	{
		if (isdown)
		{
			*timer = currenttime + repeatdelay;
			Key_Event(key, true);
		}
	}
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
	float left_triggerthreshold  = joy_deadzone_trigger.value;
	float right_triggerthreshold = joy_deadzone_trigger.value;
	
	if (!joy_active_controller)
		return;

	if (IN_HasAdaptiveTriggers())
	{
		if (ds_lt_threshold)
			left_triggerthreshold = ds_lt_threshold;
		if (ds_rt_threshold)
			right_triggerthreshold = ds_rt_threshold;
	}

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
		int xaxis = joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX;
		int yaxis = joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY;
		IN_JoyKeyEvent(joy_axisstate.axisvalue[xaxis] < -stickthreshold, newaxisstate.axisvalue[xaxis] < -stickthreshold, K_LEFTARROW, &joy_emulatedkeytimer[0]);
		IN_JoyKeyEvent(joy_axisstate.axisvalue[xaxis] > stickthreshold,  newaxisstate.axisvalue[xaxis] > stickthreshold, K_RIGHTARROW, &joy_emulatedkeytimer[1]);
		IN_JoyKeyEvent(joy_axisstate.axisvalue[yaxis] < -stickthreshold, newaxisstate.axisvalue[yaxis] < -stickthreshold, K_UPARROW, &joy_emulatedkeytimer[2]);
		IN_JoyKeyEvent(joy_axisstate.axisvalue[yaxis] > stickthreshold,  newaxisstate.axisvalue[yaxis] > stickthreshold, K_DOWNARROW, &joy_emulatedkeytimer[3]);
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

		raw.x = newaxisstate.axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX];
		raw.y = newaxisstate.axisvalue[joy_swapmovelook.value ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY];
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

	// Read DualSense trigger status nybble from HID input report
	// If it fails, it falls back to triggers threshold values
	uint8_t ds_rt_stoplocation = 0;
	uint8_t ds_rt_status = 0;
	uint8_t ds_rt_effect = 0;
	uint8_t ds_lt_stoplocation = 0;
	uint8_t ds_lt_status = 0;
	uint8_t ds_lt_effect = 0;

	if (ds_report_offset > 0)
	{
		ds_rt_stoplocation = (ds_input_report[ds_report_offset + 41] >> 0) & 0x0f;
		ds_rt_status       = (ds_input_report[ds_report_offset + 41] >> 4) & 0x0f;
		ds_lt_stoplocation = (ds_input_report[ds_report_offset + 42] >> 0) & 0x0f;
		ds_lt_status       = (ds_input_report[ds_report_offset + 42] >> 4) & 0x0f;
		ds_rt_effect       = (ds_input_report[ds_report_offset + 47] >> 0) & 0x0f;
		ds_lt_effect       = (ds_input_report[ds_report_offset + 47] >> 4) & 0x0f;
		if (joy_ds_debug.value >= 3) {
			Con_Printf ("rt loc %d status %d lt loc %d status %d\n", ds_rt_stoplocation, ds_rt_status, ds_lt_stoplocation, ds_lt_status);
			Con_Printf ("rt effect 0x%x lt effect 0x%x\n", ds_rt_effect, ds_lt_effect);
		}
	}

	ds_rt_nybble = -1;
	ds_lt_nybble = -1;
	switch (ds_rt_effect)
	{
	case 2: // Weapon mode. Enables after pressure, stops when release trigger
		ds_rt_nybble = 1 * (ds_rt_status == 2);
		break;
	case 0: // No effect
		break;
	default: // Effects is applied
		ds_rt_nybble = 1 * (ds_rt_status > 0);
		break;
	}
	switch (ds_lt_effect)
	{
	case 2:
		ds_lt_nybble = 1 * (ds_lt_status == 2);
		break;
	case 0:
		break;
	default:
		ds_lt_nybble = 1 * (ds_lt_status > 0);
		break;
	}

	// Pretend we fully pressed trigger
	if (ds_rt_nybble != -1)
		newaxisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = 1.f * ds_rt_nybble;
	if (ds_lt_nybble != -1)
		newaxisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = 1.f * ds_lt_nybble;

	// emit emulated keys for the analog triggers
	IN_JoyKeyEvent(joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > left_triggerthreshold,   newaxisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > left_triggerthreshold,   K_LTRIGGER, &joy_emulatedkeytimer[4]);
	IN_JoyKeyEvent(joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > right_triggerthreshold, newaxisstate.axisvalue[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > right_triggerthreshold, K_RTRIGGER, &joy_emulatedkeytimer[5]);
	
	joy_axisstate = newaxisstate;
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
	
	moveRaw.x = joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_LEFTX];
	moveRaw.y = joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_LEFTY];
	lookRaw.x = joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_RIGHTX];
	lookRaw.y = joy_axisstate.axisvalue[SDL_CONTROLLER_AXIS_RIGHTY];
	
	if (joy_swapmovelook.value)
	{
		joyaxis_t temp = moveRaw;
		moveRaw = lookRaw;
		lookRaw = temp;
	}
	
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

	cl.viewangles[YAW] -= lookEased.x * joy_sensitivity_yaw.value * host_frametime;
	cl.viewangles[PITCH] += lookEased.y * joy_sensitivity_pitch.value * (joy_invert.value ? -1.0 : 1.0) * host_frametime;

	if (lookEased.x != 0 || lookEased.y != 0)
		V_StopPitchDrift();

	/* johnfitz -- variable pitch clamping */
	if (cl.viewangles[PITCH] > cl_maxpitch.value)
		cl.viewangles[PITCH] = cl_maxpitch.value;
	if (cl.viewangles[PITCH] < cl_minpitch.value)
		cl.viewangles[PITCH] = cl_minpitch.value;
}

void IN_GyroMove(usercmd_t *cmd)
{
	float scale;
	if (!gyro_enable.value)
		return;

	if (!joy_active_controller)
		return;

	if (cl.paused || key_dest != key_game)
		return;

	if (CL_InCutscene ())
		return;

	scale = (180.f / M_PI) * host_frametime;
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

	if (gyro_yaw || gyro_pitch)
		V_StopPitchDrift ();

	cl.viewangles[YAW] += scale * gyro_yaw * gyro_yawsensitivity.value;
	cl.viewangles[PITCH] -= scale * gyro_pitch * gyro_pitchsensitivity.value;

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

	sens = tan(DEG2RAD (r_refdef.basefov) * 0.5f) / tan (DEG2RAD (scr_fov.value) * 0.5f);
	sens *= sensitivity.value;

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
		if (textmode == TEXTMODE_ON)
		{
			SDL_StartTextInput();
			if (in_debugkeys.value)
				Con_Printf("SDL_StartTextInput time: %g\n", Sys_DoubleTime());
		}
		else
		{
			SDL_StopTextInput();
			if (in_debugkeys.value)
				Con_Printf("SDL_StopTextInput time: %g\n", Sys_DoubleTime());
		}
	}
}

textmode_t IN_GetTextMode (void)
{
	return textmode;
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
	Con_Printf ("Calibrating, please wait...\n");

	gyro_accum[0] = 0.0;
	gyro_accum[1] = 0.0;
	gyro_accum[2] = 0.0;

	num_samples = 0;
	updates_countdown = 300;
}

static void IN_UpdateGyroCalibration (void)
{
	if (!updates_countdown)
		return;

	updates_countdown--;
	if (!updates_countdown)
	{
		const float inverseSamples = 1.f / num_samples;
		Cvar_SetValue("gyro_calibration_x", gyro_accum[0] * inverseSamples);
		Cvar_SetValue("gyro_calibration_y", gyro_accum[1] * inverseSamples);
		Cvar_SetValue("gyro_calibration_z", gyro_accum[2] * inverseSamples);

		Con_Printf("Calibration results:\n X=%f Y=%f Z=%f\n",
			gyro_calibration_x.value,
			gyro_calibration_y.value,
			gyro_calibration_z.value);

		Con_Printf("Calibration finished\n");
	}
}

qboolean IN_HasGyro (void)
{
	return gyro_present;
}

qboolean IN_HasLED (void)
{
	return led_present;
}

qboolean IN_HasAdaptiveTriggers (void)
{
	return ds_triggers_present;
}

qboolean IN_IsCalibratingGyro (void)
{
	return updates_countdown != 0;
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
			if (key_dest == key_menu)
				M_Mousemove (event.button.x, event.button.y);
			else if (key_dest == key_console)
				Con_Mousemove (event.button.x, event.button.y);
			Key_Event(buttonremap[event.button.button - 1], event.button.state == SDL_PRESSED);
			break;

		case SDL_MOUSEWHEEL:
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

				if (updates_countdown)
				{
					gyro_accum[0] += event.csensor.data[0];
					gyro_accum[1] += event.csensor.data[1];
					gyro_accum[2] += event.csensor.data[2];
					num_samples++;
					break;
				}

				if (!gyro_turning_axis.value)
					gyro_yaw = event.csensor.data[1] - gyro_calibration_y.value; // yaw
				else
					gyro_yaw = -(event.csensor.data[2] - gyro_calibration_z.value); // roll
				gyro_pitch = event.csensor.data[0] - gyro_calibration_x.value;

				gyro_yaw = IN_FilterGyroSample (prev_yaw, gyro_yaw);
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

	IN_UpdateGyroCalibration ();
#if SDL_VERSION_ATLEAST(2, 0, 14)
	IN_UpdateLED ();
#endif // SDL_VERSION_ATLEAST(2, 0, 14)
}

