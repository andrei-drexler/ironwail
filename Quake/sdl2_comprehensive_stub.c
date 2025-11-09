/*
 * Comprehensive SDL2 stub library for headless testing
 * Auto-generated stubs for all SDL functions used by Ironwail
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#define SDL_STUB_LOG(name) fprintf(stderr, "[SDL Stub] %s()\n", name)

/* SDL Types */
typedef struct SDL_version { uint8_t major, minor, patch; } SDL_version;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Cursor SDL_Cursor;
typedef struct SDL_Surface {
    uint32_t flags;
    void *format;
    int w, h, pitch;
    void *pixels;
    void *userdata;
    int locked;
    void *lock_data;
    void *clip_rect;
    void *map;
    int refcount;
} SDL_Surface;
typedef struct SDL_RWops SDL_RWops;
typedef union SDL_Event SDL_Event;
typedef struct SDL_mutex SDL_mutex;
typedef struct SDL_cond SDL_cond;
typedef struct SDL_Thread SDL_Thread;
typedef void *SDL_GLContext;
typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;
typedef int32_t SDL_atomic_t;
typedef uint32_t SDL_WindowFlags;
typedef uint32_t SDL_GLattr;
typedef int SDL_Keycode;
typedef int SDL_Scancode;
typedef void (*SDL_EventFilter)(void *userdata, SDL_Event *event);

/* Constants */
#define SDL_INIT_TIMER          0x00000001u
#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_JOYSTICK       0x00000200u
#define SDL_INIT_HAPTIC         0x00001000u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS         0x00004000u

static char sdl_error[256] = "";
static SDL_version sdl_ver = {2, 30, 0};

/* Core functions */
int SDL_Init(uint32_t flags) { SDL_STUB_LOG("SDL_Init"); return 0; }
void SDL_Quit(void) { SDL_STUB_LOG("SDL_Quit"); }
int SDL_InitSubSystem(uint32_t flags) { return 0; }
void SDL_QuitSubSystem(uint32_t flags) {}
uint32_t SDL_WasInit(uint32_t flags) { return flags; }
void SDL_GetVersion(SDL_version *v) { if (v) *v = sdl_ver; }
const char *SDL_GetError(void) { return sdl_error; }
void SDL_SetError(const char *fmt, ...) { snprintf(sdl_error, sizeof(sdl_error), "%s", fmt ? fmt : ""); }
void SDL_ClearError(void) { sdl_error[0] = '\0'; }
int SDL_SetHint(const char *name, const char *value) { return 1; }

/* Timing */
void SDL_Delay(uint32_t ms) {
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000};
    nanosleep(&ts, NULL);
}
uint64_t SDL_GetPerformanceCounter(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
uint64_t SDL_GetPerformanceFrequency(void) { return 1000000000ULL; }

/* Window (stubs) */
SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, uint32_t flags) {
    SDL_STUB_LOG("SDL_CreateWindow");
    return NULL;
}
void SDL_DestroyWindow(SDL_Window *window) {}
void SDL_RaiseWindow(SDL_Window *window) {}
int SDL_GetWindowDisplayIndex(SDL_Window *window) { return 0; }
uint32_t SDL_GetWindowFlags(SDL_Window *window) { return 0; }
uint32_t SDL_GetWindowPixelFormat(SDL_Window *window) { return 0; }
void SDL_GetWindowSize(SDL_Window *window, int *w, int *h) {
    if (w) *w = 640;
    if (h) *h = 480;
}

/* OpenGL (stubs) */
SDL_GLContext SDL_GL_CreateContext(SDL_Window *window) { return (SDL_GLContext)1; }
void SDL_GL_DeleteContext(SDL_GLContext context) {}
int SDL_GL_SetAttribute(SDL_GLattr attr, int value) { return 0; }
int SDL_GL_GetAttribute(SDL_GLattr attr, int *value) { if (value) *value = 0; return 0; }
void SDL_GL_ResetAttributes(void) {}
int SDL_GL_SetSwapInterval(int interval) { return 0; }
void SDL_GL_SwapWindow(SDL_Window *window) {}
void *SDL_GL_GetProcAddress(const char *proc) { return NULL; }

/* Input - Mouse/Keyboard */
uint32_t SDL_GetMouseState(int *x, int *y) { if (x) *x = 0; if (y) *y = 0; return 0; }
SDL_bool SDL_GetRelativeMouseMode(void) { return SDL_FALSE; }
int SDL_SetRelativeMouseMode(SDL_bool enabled) { return 0; }
const char *SDL_GetKeyName(SDL_Keycode key) { return "Unknown"; }
const char *SDL_GetScancodeName(SDL_Scancode scancode) { return "Unknown"; }

/* Game Controller */
SDL_bool SDL_IsGameController(int joystick_index) { return SDL_FALSE; }
void *SDL_GameControllerOpen(int joystick_index) { return NULL; }
void SDL_GameControllerClose(void *gamecontroller) {}
const char *SDL_GameControllerName(void *gamecontroller) { return "Stub Controller"; }
const char *SDL_GameControllerNameForIndex(int joystick_index) { return "Stub Controller"; }
int SDL_GameControllerGetAxis(void *gamecontroller, int axis) { return 0; }
uint8_t SDL_GameControllerGetButton(void *gamecontroller, int button) { return 0; }
void *SDL_GameControllerGetJoystick(void *gamecontroller) { return NULL; }
int SDL_GameControllerGetType(void *gamecontroller) { return 0; }
SDL_bool SDL_GameControllerHasLED(void *gamecontroller) { return SDL_FALSE; }
SDL_bool SDL_GameControllerHasRumble(void *gamecontroller) { return SDL_FALSE; }
SDL_bool SDL_GameControllerHasSensor(void *gamecontroller) { return SDL_FALSE; }
int SDL_GameControllerRumble(void *gamecontroller, uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms) { return -1; }
int SDL_GameControllerSetLED(void *gamecontroller, uint8_t red, uint8_t green, uint8_t blue) { return -1; }
int SDL_GameControllerSetSensorEnabled(void *gamecontroller, int type, SDL_bool enabled) { return -1; }
float SDL_GameControllerGetSensorDataRate(void *gamecontroller, int type) { return 0.0f; }
int SDL_GameControllerAddMappingsFromRW(SDL_RWops *rw, int freerw) { return 0; }

/* Joystick */
int SDL_NumJoysticks(void) { return 0; }
int SDL_JoystickGetDeviceInstanceID(int device_index) { return -1; }
int SDL_JoystickInstanceID(void *joystick) { return -1; }

/* Events */
int SDL_PollEvent(SDL_Event *event) { return 0; }
void SDL_SetEventFilter(SDL_EventFilter filter, void *userdata) {}
void SDL_GetEventFilter(SDL_EventFilter *filter, void **userdata) {
    if (filter) *filter = NULL;
    if (userdata) *userdata = NULL;
}

/* Text input */
SDL_bool SDL_IsTextInputActive(void) { return SDL_FALSE; }

/* Clipboard */
char *SDL_GetClipboardText(void) { return strdup(""); }
int SDL_SetClipboardText(const char *text) { return 0; }

/* Audio */
int SDL_OpenAudio(void *desired, void *obtained) { return -1; }
void SDL_CloseAudio(void) {}
void SDL_PauseAudio(int pause_on) {}
void SDL_LockAudio(void) {}
void SDL_UnlockAudio(void) {}
const char *SDL_GetAudioDeviceName(int index, int iscapture) { return NULL; }
const char *SDL_GetCurrentAudioDriver(void) { return "dummy"; }

/* Display */
int SDL_GetNumDisplayModes(int displayIndex) { return 1; }
int SDL_GetDisplayMode(int displayIndex, int modeIndex, void *mode) { return 0; }
int SDL_GetCurrentDisplayMode(int displayIndex, void *mode) { return 0; }
int SDL_GetDesktopDisplayMode(int displayIndex, void *mode) { return 0; }

/* Cursor */
SDL_Cursor *SDL_CreateSystemCursor(int id) { return (SDL_Cursor *)1; }
void SDL_FreeCursor(SDL_Cursor *cursor) {}
void SDL_SetCursor(SDL_Cursor *cursor) {}

/* Surface */
void SDL_FreeSurface(SDL_Surface *surface) { if (surface) free(surface); }
SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *src, int freesrc) { return NULL; }
int SDL_SetColorKey(SDL_Surface *surface, int flag, uint32_t key) { return 0; }
uint32_t SDL_MapRGB(const void *format, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/* RWops */
SDL_RWops *SDL_RWFromFile(const char *file, const char *mode) { return NULL; }
SDL_RWops *SDL_RWFromConstMem(const void *mem, int size) { return NULL; }
int64_t SDL_RWsize(SDL_RWops *context) { return -1; }
int64_t SDL_RWseek(SDL_RWops *context, int64_t offset, int whence) { return -1; }
size_t SDL_RWread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum) { return 0; }
int SDL_RWclose(SDL_RWops *context) { return 0; }

/* Threading */
SDL_mutex *SDL_CreateMutex(void) { return (SDL_mutex *)1; }
void SDL_DestroyMutex(SDL_mutex *mutex) {}
int SDL_LockMutex(SDL_mutex *mutex) { return 0; }
int SDL_UnlockMutex(SDL_mutex *mutex) { return 0; }
SDL_cond *SDL_CreateCond(void) { return (SDL_cond *)1; }
void SDL_DestroyCond(SDL_cond *cond) {}
int SDL_CondSignal(SDL_cond *cond) { return 0; }
int SDL_CondBroadcast(SDL_cond *cond) { return 0; }
int SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex) { return 0; }
SDL_Thread *SDL_CreateThread(void *(*fn)(void *), const char *name, void *data) { return NULL; }

/* Atomics */
int SDL_AtomicGet(SDL_atomic_t *a) { return a ? *(int*)a : 0; }
int SDL_AtomicSet(SDL_atomic_t *a, int v) { if (a) *(int*)a = v; return v; }
int SDL_AtomicAdd(SDL_atomic_t *a, int v) {
    if (!a) return 0;
    int old = *(int*)a;
    *(int*)a += v;
    return old;
}
SDL_bool SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval) {
    if (!a) return SDL_FALSE;
    if (*(int*)a == oldval) { *(int*)a = newval; return SDL_TRUE; }
    return SDL_FALSE;
}
void *SDL_AtomicGetPtr(void **a) { return a ? *a : NULL; }
void *SDL_AtomicSetPtr(void **a, void *v) { if (a) *a = v; return v; }

/* Platform */
const char *SDL_GetPlatform(void) { return "Linux"; }

/* Locale */
void *SDL_GetPreferredLocales(void) { return NULL; }

/* URL */
int SDL_OpenURL(const char *url) { return -1; }

/* Misc */
SDL_bool SDL_HasSSE(void) { return SDL_TRUE; }
SDL_bool SDL_HasSSE2(void) { return SDL_TRUE; }
SDL_bool SDL_HasScreenKeyboardSupport(void) { return SDL_FALSE; }

/* Additional Window functions */
void SDL_SetWindowTitle(SDL_Window *window, const char *title) {}
void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon) {}
void SDL_SetWindowPosition(SDL_Window *window, int x, int y) {}
void SDL_SetWindowSize(SDL_Window *window, int w, int h) {}
void SDL_SetWindowMinimumSize(SDL_Window *window, int min_w, int min_h) {}
void SDL_SetWindowBordered(SDL_Window *window, SDL_bool bordered) {}
int SDL_SetWindowFullscreen(SDL_Window *window, uint32_t flags) { return 0; }
int SDL_SetWindowDisplayMode(SDL_Window *window, const void *mode) { return 0; }
void SDL_ShowWindow(SDL_Window *window) {}
void SDL_WarpMouseInWindow(SDL_Window *window, int x, int y) {}

/* Message Box */
int SDL_ShowSimpleMessageBox(uint32_t flags, const char *title, const char *message, SDL_Window *window) { return 0; }

/* Text input */
void SDL_StartTextInput(void) {}
void SDL_StopTextInput(void) {}

/* Threading */
void SDL_WaitThread(SDL_Thread *thread, int *status) {}

/* Memory */
void SDL_free(void *mem) { free(mem); }

/* Environment */
int SDL_setenv(const char *name, const char *value, int overwrite) { return setenv(name, value, overwrite); }
