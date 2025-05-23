// SDL/EGL implementation of the framework.
// This is quite messy due to platform-specific implementations and #ifdef's.
// Note: SDL1.2 implementation is deprecated and will soon be replaced by SDL2.0.
// If your platform is not supported, it is suggested to use Qt instead.

#ifdef _WIN32
#pragma warning(disable:4091)  // workaround bug in VS2015 headers

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <ShellAPI.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include "SDL.h"
#ifndef _WIN32
#include "SDL/SDLJoystick.h"
SDLJoystick *joystick = NULL;
#endif

#if PPSSPP_PLATFORM(RPI)
#include <bcm_host.h>
#endif

#include <algorithm>
#include <cassert>

#include "base/display.h"
#include "base/logging.h"
#include "base/timeutil.h"
#include "gfx/gl_common.h"
#include "gfx_es2/gpu_features.h"
#include "input/input_state.h"
#include "input/keycodes.h"
#include "net/resolve.h"
#include "base/NKCodeFromSDL.h"
#include "util/const_map.h"
#include "util/text/utf8.h"
#include "math/math_util.h"

#include "Core/System.h"
#include "Core/Core.h"
#include "Core/Config.h"
#include "Common/GraphicsContext.h"
#include "Common/LogManager.h"

class GLDummyGraphicsContext : public DummyGraphicsContext {
public:
	GLDummyGraphicsContext() {
		CheckGLExtensions();
		draw_ = Draw::T3DCreateGLContext();
		bool success = draw_->CreatePresets();
		assert(success);
	}
	~GLDummyGraphicsContext() { if (draw_) { delete draw_; draw_ = 0;} }

	Draw::DrawContext *GetDrawContext() override {
		return draw_;
	}
private:
	Draw::DrawContext *draw_ = 0;
};

GlobalUIState lastUIState = UISTATE_MENU;
GlobalUIState GetUIState();

static SDL_Window* g_Screen = NULL;
static bool g_ToggleFullScreenNextFrame = false;
static int g_ToggleFullScreenType;
static int g_QuitRequested = 0;

static int g_DesktopWidth = 0;
static int g_DesktopHeight = 0;

#if defined(USING_EGL)
#include "EGL/egl.h"

#if !defined(USING_FBDEV)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include "SDL_syswm.h"
#include "math.h"

static EGLDisplay               g_eglDisplay    = NULL;
static EGLContext               g_eglContext    = NULL;
static EGLSurface               g_eglSurface    = NULL;
#ifdef USING_FBDEV
static EGLNativeDisplayType     g_Display       = NULL;
#else
static Display*                 g_Display       = NULL;
#endif
static NativeWindowType         g_Window        = (NativeWindowType)NULL;

int8_t CheckEGLErrors(const std::string& file, uint16_t line) {
	EGLenum error;
	std::string errortext;

	error = eglGetError();
	switch (error)
	{
		case EGL_SUCCESS: case 0:           return 0;
		case EGL_NOT_INITIALIZED:           errortext = "EGL_NOT_INITIALIZED"; break;
		case EGL_BAD_ACCESS:                errortext = "EGL_BAD_ACCESS"; break;
		case EGL_BAD_ALLOC:                 errortext = "EGL_BAD_ALLOC"; break;
		case EGL_BAD_ATTRIBUTE:             errortext = "EGL_BAD_ATTRIBUTE"; break;
		case EGL_BAD_CONTEXT:               errortext = "EGL_BAD_CONTEXT"; break;
		case EGL_BAD_CONFIG:                errortext = "EGL_BAD_CONFIG"; break;
		case EGL_BAD_CURRENT_SURFACE:       errortext = "EGL_BAD_CURRENT_SURFACE"; break;
		case EGL_BAD_DISPLAY:               errortext = "EGL_BAD_DISPLAY"; break;
		case EGL_BAD_SURFACE:               errortext = "EGL_BAD_SURFACE"; break;
		case EGL_BAD_MATCH:                 errortext = "EGL_BAD_MATCH"; break;
		case EGL_BAD_PARAMETER:             errortext = "EGL_BAD_PARAMETER"; break;
		case EGL_BAD_NATIVE_PIXMAP:         errortext = "EGL_BAD_NATIVE_PIXMAP"; break;
		case EGL_BAD_NATIVE_WINDOW:         errortext = "EGL_BAD_NATIVE_WINDOW"; break;
		default:                            errortext = "unknown"; break;
	}
	printf( "ERROR: EGL Error detected in file %s at line %d: %s (0x%X)\n", file.c_str(), line, errortext.c_str(), error );
	return 1;
}
#define EGL_ERROR(str, check) { \
		if (check) CheckEGLErrors( __FILE__, __LINE__ ); \
		printf("EGL ERROR: " str "\n"); \
		return 1; \
	}

int8_t EGL_Open() {
#ifdef USING_FBDEV
	g_Display = ((EGLNativeDisplayType)0);
#else
	if ((g_Display = XOpenDisplay(NULL)) == NULL)
		EGL_ERROR("Unable to get display!", false);
#endif
	if ((g_eglDisplay = eglGetDisplay((NativeDisplayType)g_Display)) == EGL_NO_DISPLAY)
		EGL_ERROR("Unable to create EGL display.", true);
	if (eglInitialize(g_eglDisplay, NULL, NULL) != EGL_TRUE)
		EGL_ERROR("Unable to initialize EGL display.", true);
	return 0;
}

int8_t EGL_Init() {
	EGLConfig g_eglConfig;
	EGLint g_numConfigs = 0;
	EGLint attrib_list[]= {
	// TODO: Should cycle through fallbacks, like on Android
#ifdef USING_FBDEV
		EGL_RED_SIZE,        5,
		EGL_GREEN_SIZE,      6,
		EGL_BLUE_SIZE,       5,
#endif
		EGL_DEPTH_SIZE,      16,
		EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
#ifdef USING_GLES2
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
		EGL_SAMPLE_BUFFERS,  0,
		EGL_SAMPLES,         0,
		EGL_NONE};

	const EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	EGLBoolean result = eglChooseConfig(g_eglDisplay, attrib_list, &g_eglConfig, 1, &g_numConfigs);
	if (result != EGL_TRUE || g_numConfigs == 0) EGL_ERROR("Unable to query for available configs.", true);

	g_eglContext = eglCreateContext(g_eglDisplay, g_eglConfig, NULL, attributes );
	if (g_eglContext == EGL_NO_CONTEXT) EGL_ERROR("Unable to create GLES context!", true);

#if !defined(USING_FBDEV)
	//Get the SDL window handle
	SDL_SysWMinfo sysInfo; //Will hold our Window information
	SDL_VERSION(&sysInfo.version); //Set SDL version
#endif

#ifdef USING_FBDEV
	g_Window = (NativeWindowType)NULL;
#else
	g_Window = (NativeWindowType)sysInfo.info.x11.window;
#endif
	g_eglSurface = eglCreateWindowSurface(g_eglDisplay, g_eglConfig, g_Window, 0);
	if (g_eglSurface == EGL_NO_SURFACE)
		EGL_ERROR("Unable to create EGL surface!", true);

	if (eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext) != EGL_TRUE)
		EGL_ERROR("Unable to make GLES context current.", true);

	return 0;
}

void EGL_Close() {
	if (g_eglDisplay != NULL) {
		eglMakeCurrent(g_eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
		if (g_eglContext != NULL) {
			eglDestroyContext(g_eglDisplay, g_eglContext);
		}
		if (g_eglSurface != NULL) {
			eglDestroySurface(g_eglDisplay, g_eglSurface);
		}
		eglTerminate(g_eglDisplay);
		g_eglDisplay = NULL;
	}
	if (g_Display != NULL) {
#if !defined(USING_FBDEV)
		XCloseDisplay(g_Display);
#endif
		g_Display = NULL;
	}
	g_eglSurface = NULL;
	g_eglContext = NULL;
}
#endif

int getDisplayNumber(void) {
	int displayNumber = 0;
	char * displayNumberStr;

	//get environment
	displayNumberStr=getenv("SDL_VIDEO_FULLSCREEN_HEAD");

	if (displayNumberStr)
	{
		displayNumber = atoi(displayNumberStr);
	}

	return displayNumber;
}

// Simple implementations of System functions


void SystemToast(const char *text) {
#ifdef _WIN32
	MessageBox(0, text, "Toast!", MB_ICONINFORMATION);
#else
	puts(text);
#endif
}

void ShowKeyboard() {
	// Irrelevant on PC
}

void Vibrate(int length_ms) {
	// Ignore on PC
}

void System_SendMessage(const char *command, const char *parameter) {
	if (!strcmp(command, "toggle_fullscreen")) {
		g_ToggleFullScreenNextFrame = true;
		if (strcmp(parameter, "1") == 0) {
			g_ToggleFullScreenType = 1;
		} else if (strcmp(parameter, "0") == 0) {
			g_ToggleFullScreenType = 0;
		} else {
			// Just toggle.
			g_ToggleFullScreenType = -1;
		}
	} else if (!strcmp(command, "finish")) {
		// Do a clean exit
		g_QuitRequested = true;
	}
}

void System_AskForPermission(SystemPermission permission) {}
PermissionStatus System_GetPermissionStatus(SystemPermission permission) { return PERMISSION_STATUS_GRANTED; }

void LaunchBrowser(const char *url) {
#if defined(MOBILE_DEVICE)
	ILOG("Would have gone to %s but LaunchBrowser is not implemented on this platform", url);
#elif defined(_WIN32)
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
	std::string command = std::string("open ") + url;
	system(command.c_str());
#else
	std::string command = std::string("xdg-open ") + url;
	int err = system(command.c_str());
	if (err) {
		ILOG("Would have gone to %s but xdg-utils seems not to be installed", url)
	}
#endif
}

void LaunchMarket(const char *url) {
#if defined(MOBILE_DEVICE)
	ILOG("Would have gone to %s but LaunchMarket is not implemented on this platform", url);
#elif defined(_WIN32)
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
	std::string command = std::string("open ") + url;
	system(command.c_str());
#else
	std::string command = std::string("xdg-open ") + url;
	int err = system(command.c_str());
	if (err) {
		ILOG("Would have gone to %s but xdg-utils seems not to be installed", url)
	}
#endif
}

void LaunchEmail(const char *email_address) {
#if defined(MOBILE_DEVICE)
	ILOG("Would have opened your email client for %s but LaunchEmail is not implemented on this platform", email_address);
#elif defined(_WIN32)
	ShellExecute(NULL, "open", (std::string("mailto:") + email_address).c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
	std::string command = std::string("open mailto:") + email_address;
	system(command.c_str());
#else
	std::string command = std::string("xdg-email ") + email_address;
	int err = system(command.c_str());
	if (err) {
		ILOG("Would have gone to %s but xdg-utils seems not to be installed", email_address)
	}
#endif
}

std::string System_GetProperty(SystemProperty prop) {
	switch (prop) {
	case SYSPROP_NAME:
#ifdef _WIN32
		return "SDL:Windows";
#elif __linux__
		return "SDL:Linux";
#elif __APPLE__
		return "SDL:OSX";
#else
		return "SDL:";
#endif
	case SYSPROP_LANGREGION:
		return "en_US";
	default:
		return "";
	}
}

int System_GetPropertyInt(SystemProperty prop) {
	switch (prop) {
	case SYSPROP_AUDIO_SAMPLE_RATE:
		return 44100;
	case SYSPROP_DISPLAY_REFRESH_RATE:
		return 60000;
	case SYSPROP_DEVICE_TYPE:
#if defined(MOBILE_DEVICE)
		return DEVICE_TYPE_MOBILE;
#else
		return DEVICE_TYPE_DESKTOP;
#endif
	default:
		return -1;
	}
}

bool System_GetPropertyBool(SystemProperty prop) {
	switch (prop) {
	case SYSPROP_HAS_BACK_BUTTON:
		return true;
	case SYSPROP_APP_GOLD:
#ifdef GOLD
		return true;
#else
		return false;
#endif
	default:
		return false;
	}
}

extern void mixaudio(void *userdata, Uint8 *stream, int len) {
	NativeMix((short *)stream, len / 4);
}

// returns -1 on failure
static int parseInt(const char *str) {
	int val;
	int retval = sscanf(str, "%d", &val);
	printf("%i = scanf %s\n", retval, str);
	if (retval != 1) {
		return -1;
	} else {
		return val;
	}
}

static float parseFloat(const char *str) {
	float val;
	int retval = sscanf(str, "%f", &val);
	printf("%i = sscanf %s\n", retval, str);
	if (retval != 1) {
		return -1.0f;
	} else {
		return val;
	}
}

void ToggleFullScreenIfFlagSet() {
	if (g_ToggleFullScreenNextFrame) {
		g_ToggleFullScreenNextFrame = false;

		Uint32 window_flags = SDL_GetWindowFlags(g_Screen);
		if (g_ToggleFullScreenType == -1) {
			window_flags ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else if (g_ToggleFullScreenType == 1) {
			window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else {
			window_flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		SDL_SetWindowFullscreen(g_Screen, window_flags);
	}
}

#ifdef _WIN32
#undef main
#endif
int main(int argc, char *argv[]) {
#if PPSSPP_PLATFORM(RPI)
	bcm_host_init();
#endif
	putenv((char*)"SDL_VIDEO_CENTERED=1");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

#ifdef USING_EGL
SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
#endif
	std::string app_name;
	std::string app_name_nice;
	std::string version;
	bool landscape;
	NativeGetAppInfo(&app_name, &app_name_nice, &landscape, &version);

	bool joystick_enabled = true;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
		joystick_enabled = false;
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
			fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
			return 1;
		}
	}

#ifdef __APPLE__
	// Make sure to request a somewhat modern GL context at least - the
	// latest supported by MacOSX (really, really sad...)
	// Requires SDL 2.0
	// We really should upgrade to SDL 2.0 soon.
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

#ifdef USING_EGL
#if NO_SDLGL
	if (EGL_Open())
		return 1;
#endif
#endif

	// Get the video info before doing anything else, so we don't get skewed resolution results.
	// TODO: support multiple displays correctly
	SDL_DisplayMode displayMode;
	int should_be_zero = SDL_GetCurrentDisplayMode(0, &displayMode);
	if (should_be_zero != 0) {
		fprintf(stderr, "Could not get display mode: %s\n", SDL_GetError());
		return 1;
	}
	g_DesktopWidth = displayMode.w;
	g_DesktopHeight = displayMode.h;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1);

	Uint32 mode;
#ifdef USING_GLES2
#if NO_SDLGL

#if !PC_NO_FULLSCREEN //added
	mode = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
#else
	mode = SDL_WINDOW_OPENGL;
//not run here, pc, skip, added
mode &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
mode &= ~SDL_WINDOW_FULLSCREEN;
//mode |= SDL_WINDOW_RESIZABLE;
#endif

#else

#if !PC_NO_FULLSCREEN //added
	mode = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN; //mode = SDL_WINDOW_OPENGL;
#else
	mode = SDL_WINDOW_OPENGL;
mode &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
mode &= ~SDL_WINDOW_FULLSCREEN;
//mode |= SDL_WINDOW_RESIZABLE;
#endif

#endif
#if NO_SDLGL
    //skip
    // Requires at least OpenGL ES 2.0
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else	
    // Requires at least OpenGL ES 2.0
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);	
#endif
#else
	mode = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif
	int set_xres = -1;
	int set_yres = -1;
	bool portrait = false;
	bool set_ipad = false;
	float set_dpi = 1.0f;
	float set_scale = 1.0f;

	// Produce a new set of arguments with the ones we skip.
	int remain_argc = 1;
	const char *remain_argv[256] = { argv[0] };

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i],"--fullscreen"))
			mode |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		else if (set_xres == -2)
			set_xres = parseInt(argv[i]);
		else if (set_yres == -2)
			set_yres = parseInt(argv[i]);
		else if (set_dpi == -2)
			set_dpi = parseFloat(argv[i]);
		else if (set_scale == -2)
			set_scale = parseFloat(argv[i]);
		else if (!strcmp(argv[i],"--xres"))
			set_xres = -2;
		else if (!strcmp(argv[i],"--yres"))
			set_yres = -2;
		else if (!strcmp(argv[i],"--dpi"))
			set_dpi = -2;
		else if (!strcmp(argv[i],"--scale"))
			set_scale = -2;
		else if (!strcmp(argv[i],"--ipad"))
			set_ipad = true;
		else if (!strcmp(argv[i],"--portrait"))
			portrait = true;
		else {
			remain_argv[remain_argc++] = argv[i];
		}
	}

	// Is resolution is too low to run windowed
	if (g_DesktopWidth < 480 * 2 && g_DesktopHeight < 272 * 2) {
		mode |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	if (mode & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		pixel_xres = g_DesktopWidth;
		pixel_yres = g_DesktopHeight;
		g_Config.bFullScreen = true;
	} else {
		// set a sensible default resolution (2x)
		pixel_xres = 480 * 2 * set_scale;
		pixel_yres = 272 * 2 * set_scale;
		if (portrait) {
			std::swap(pixel_xres, pixel_yres);
		}
		g_Config.bFullScreen = false;
	}

	set_dpi = 1.0f / set_dpi;

	if (set_ipad) {
		pixel_xres = 1024;
		pixel_yres = 768;
	}
	if (!landscape) {
		std::swap(pixel_xres, pixel_yres);
	}

	if (set_xres > 0) {
		pixel_xres = set_xres;
	}
	if (set_yres > 0) {
		pixel_yres = set_yres;
	}

	float dpi_scale = 1.0f;
	if (set_dpi > 0) {
		dpi_scale = set_dpi;
	}

	dp_xres = (float)pixel_xres * dpi_scale;
	dp_yres = (float)pixel_yres * dpi_scale;

#ifdef _MSC_VER
	// VFSRegister("temp/", new DirectoryAssetReader("E:\\Temp\\"));
	TCHAR path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path);
	PathAppend(path, (app_name + "\\").c_str());
#else
	// Mac / Linux
	char path[2048];
	const char *the_path = getenv("HOME");
	if (!the_path) {
		struct passwd* pwd = getpwuid(getuid());
		if (pwd)
			the_path = pwd->pw_dir;
	}
	strcpy(path, the_path);
	if (path[strlen(path)-1] != '/')
		strcat(path, "/");
#endif

#ifdef _WIN32
	NativeInit(remain_argc, (const char **)remain_argv, path, "D:\\", nullptr);
#else
	NativeInit(remain_argc, (const char **)remain_argv, path, "/tmp", nullptr);
#endif

#if !NO_SDLGL
	// Use the setting from the config when initing the window.
	if (g_Config.bFullScreen)
		mode |= SDL_WINDOW_FULLSCREEN_DESKTOP;

#if PC_NO_FULLSCREEN //added
	//don't use this://set_dpi = 1.0f;
	pixel_xres = 480;
	pixel_yres = 272;
mode &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
mode &= ~SDL_WINDOW_FULLSCREEN;
//mode |= SDL_WINDOW_RESIZABLE;
#if 1
#if USE_ROTATE_90 || USE_ROTATE_270
	std::swap(pixel_xres, pixel_yres);
#endif
#endif

#endif

	g_Screen = SDL_CreateWindow(app_name_nice.c_str(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(getDisplayNumber()),\
					SDL_WINDOWPOS_UNDEFINED, pixel_xres, pixel_yres, mode);

	if (g_Screen == NULL) {
		NativeShutdown();
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 2;
	}
#endif

#if !NO_SDLGL
	SDL_GLContext glContext = SDL_GL_CreateContext(g_Screen);
	if (glContext == NULL) {
		NativeShutdown();
		fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
		SDL_Quit();
		return 2;
	}
#else
	SDL_GLContext glContext = NULL;	
#endif

#ifdef USING_EGL
#if NO_SDLGL
	EGL_Init();
#endif
#endif

#if !NO_SDLGL
	SDL_SetWindowTitle(g_Screen, (app_name_nice + " " + PPSSPP_GIT_VERSION).c_str());

#if defined(MOBILE_DEVICE) || USE_HIDE_SDL_SHOWCURSOR
	SDL_ShowCursor(SDL_DISABLE);
#endif
#endif

#ifndef USING_GLES2
	// Some core profile drivers elide certain extensions from GL_EXTENSIONS/etc.
	// glewExperimental allows us to force GLEW to search for the pointers anyway.
	if (gl_extensions.IsCoreContext)
		glewExperimental = true;
	if (GLEW_OK != glewInit()) {
		printf("Failed to initialize glew!\n");
		return 1;
	}
	// Unfortunately, glew will generate an invalid enum error, ignore.
	if (gl_extensions.IsCoreContext)
		glGetError();

	if (GLEW_VERSION_2_0) {
		printf("OpenGL 2.0 or higher.\n");
	} else {
		printf("Sorry, this program requires OpenGL 2.0.\n");
		return 1;
	}
#endif


	pixel_in_dps_x = (float)pixel_xres / dp_xres;
	pixel_in_dps_y = (float)pixel_yres / dp_yres;
	g_dpi_scale_x = dp_xres / (float)pixel_xres;
	g_dpi_scale_y = dp_yres / (float)pixel_yres;
	g_dpi_scale_real_x = g_dpi_scale_x;
	g_dpi_scale_real_y = g_dpi_scale_y;

	printf("Pixels: %i x %i\n", pixel_xres, pixel_yres);
	printf("Virtual pixels: %i x %i\n", dp_xres, dp_yres);

	GraphicsContext *graphicsContext = new GLDummyGraphicsContext();
	NativeInitGraphics(graphicsContext);

	NativeResized();

	SDL_AudioSpec fmt, ret_fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.freq = 44100;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 2048;
	fmt.callback = &mixaudio;
	fmt.userdata = (void *)0;

	if (SDL_OpenAudio(&fmt, &ret_fmt) < 0) {
		ELOG("Failed to open audio: %s", SDL_GetError());
	} else {
		if (ret_fmt.samples != fmt.samples) // Notify, but still use it
			ELOG("Output audio samples: %d (requested: %d)", ret_fmt.samples, fmt.samples);
		if (ret_fmt.freq != fmt.freq || ret_fmt.format != fmt.format || ret_fmt.channels != fmt.channels) {
			ELOG("Sound buffer format does not match requested format.");
			ELOG("Output audio freq: %d (requested: %d)", ret_fmt.freq, fmt.freq);
			ELOG("Output audio format: %d (requested: %d)", ret_fmt.format, fmt.format);
			ELOG("Output audio channels: %d (requested: %d)", ret_fmt.channels, fmt.channels);
			ELOG("Provided output format does not match requirement, turning audio off");
			SDL_CloseAudio();
		}
	}

	// Audio must be unpaused _after_ NativeInit()
	SDL_PauseAudio(0);
#ifndef _WIN32
	if (joystick_enabled) {
		joystick = new SDLJoystick();
	} else {
		joystick = nullptr;
	}
#endif
	EnableFZ();

	int framecount = 0;
	float t = 0;
	float lastT = 0;
	bool mouseDown = false;

	while (true) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			float mx = event.motion.x * g_dpi_scale_x;
			float my = event.motion.y * g_dpi_scale_y;

			switch (event.type) {
			case SDL_QUIT:
				g_QuitRequested = 1;
				break;

#if !defined(MOBILE_DEVICE) && !USE_HIDE_SDL_SHOWCURSOR
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
				{
ILOG("<<<<<<<<<<<<SDL_WINDOWEVENT_RESIZED, %d, %d, %f\n", event.window.data1, event.window.data2, dpi_scale);
					Uint32 window_flags = SDL_GetWindowFlags(g_Screen);
					bool fullscreen = (window_flags & SDL_WINDOW_FULLSCREEN);

					pixel_xres = event.window.data1;
					pixel_yres = event.window.data2;
					dp_xres = (float)pixel_xres * dpi_scale;
					dp_yres = (float)pixel_yres * dpi_scale;
					NativeResized();

					// Set variable here in case fullscreen was toggled by hotkey
					g_Config.bFullScreen = fullscreen;

					// Hide/Show cursor correctly toggling fullscreen
					if (lastUIState == UISTATE_INGAME && fullscreen && !g_Config.bShowTouchControls) {
						SDL_ShowCursor(SDL_DISABLE);
					} else if (lastUIState != UISTATE_INGAME || !fullscreen) {
						SDL_ShowCursor(SDL_ENABLE);
					}
					break;
				}

				default:
					break;
				}
				break;
#endif
			case SDL_KEYDOWN:
				{
					if (event.key.repeat > 0) { break;}
					int k = event.key.keysym.sym;
					KeyInput key;
					key.flags = KEY_DOWN;
					auto mapped = KeyMapRawSDLtoNative.find(k);
					if (mapped == KeyMapRawSDLtoNative.end() || mapped->second == NKCODE_UNKNOWN) {
						break;
					}
					key.keyCode = mapped->second;
					key.deviceId = DEVICE_ID_KEYBOARD;
ILOG("<<<<<<<<<<<<SDL_KEYDOWN, %d, %d\n", event.key.keysym.sym, key.keyCode);
#if 0
#if USE_ROTATE_90 || USE_ROTATE_270
if (key.keyCode == 44) {
//Press 'P'
if (1) {
	g_display.rotation = DisplayRotation::ROTATE_0;
	g_display.rot_matrix.setIdentity();
UpdateScreenScale(272, 480);
} else {
	g_display.rotation = DisplayRotation::ROTATE_90;
	g_display.rot_matrix.setRotationZ90();
UpdateScreenScale(272, 272);
}

}
#endif
#endif					
					NativeKey(key);

#if 0
//only for test
//P:44->Down:20
if (key.keyCode == 44) { //Press Key P to emulate press key Down Arrow
	std::thread t([]() {
		KeyInput key;
		key.flags = KEY_DOWN;
		key.keyCode = (InputKeyCode)20;
		key.deviceId = DEVICE_ID_KEYBOARD;
		NativeKey(key);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));		
		key.flags = KEY_UP;
		key.keyCode = (InputKeyCode)20;
		key.deviceId = DEVICE_ID_KEYBOARD;
		NativeKey(key);
	});
	t.detach();
}
#endif					
					
					break;
				}
			case SDL_KEYUP:
				{
ILOG("<<<<<<<<<<<<SDL_KEYUP, %d\n", event.key.keysym.sym);				
					if (event.key.repeat > 0) { break;}
					int k = event.key.keysym.sym;
					KeyInput key;
					key.flags = KEY_UP;
					auto mapped = KeyMapRawSDLtoNative.find(k);
					if (mapped == KeyMapRawSDLtoNative.end() || mapped->second == NKCODE_UNKNOWN) {
						break;
					}
					key.keyCode = mapped->second;
					key.deviceId = DEVICE_ID_KEYBOARD;
					NativeKey(key);
					break;
				}
			case SDL_TEXTINPUT:
				{
					int pos = 0;
					int c = u8_nextchar(event.text.text, &pos);
					KeyInput key;
					key.flags = KEY_CHAR;
					key.keyCode = c;
					key.deviceId = DEVICE_ID_KEYBOARD;
					NativeKey(key);
					break;
				}
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					{
						mouseDown = true;
						TouchInput input;
						input.x = mx;
						input.y = my;
						input.flags = TOUCH_DOWN | TOUCH_MOUSE;
						input.id = 0;
						NativeTouch(input);
						KeyInput key(DEVICE_ID_MOUSE, NKCODE_EXT_MOUSEBUTTON_1, KEY_DOWN);
						NativeKey(key);
					}
					break;
				case SDL_BUTTON_RIGHT:
					{
						KeyInput key(DEVICE_ID_MOUSE, NKCODE_EXT_MOUSEBUTTON_2, KEY_DOWN);
						NativeKey(key);
					}
					break;
				}
				break;
			case SDL_MOUSEWHEEL:
				{
					KeyInput key;
					key.deviceId = DEVICE_ID_MOUSE;
					if (event.wheel.y > 0) {
						key.keyCode = NKCODE_EXT_MOUSEWHEEL_UP;
					} else {
						key.keyCode = NKCODE_EXT_MOUSEWHEEL_DOWN;
					}
					key.flags = KEY_DOWN;
					NativeKey(key);

					// SDL2 doesn't consider the mousewheel a button anymore
					// so let's send the KEY_UP right away.
					// Maybe KEY_UP alone will suffice?
					key.flags = KEY_UP;
					NativeKey(key);
				}
			case SDL_MOUSEMOTION:
				if (mouseDown) {
					TouchInput input;
					input.x = mx;
					input.y = my;
					input.flags = TOUCH_MOVE | TOUCH_MOUSE;
					input.id = 0;
					NativeTouch(input);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					{
						mouseDown = false;
						TouchInput input;
						input.x = mx;
						input.y = my;
						input.flags = TOUCH_UP | TOUCH_MOUSE;
						input.id = 0;
						NativeTouch(input);
						KeyInput key(DEVICE_ID_MOUSE, NKCODE_EXT_MOUSEBUTTON_1, KEY_UP);
						NativeKey(key);
					}
					break;
				case SDL_BUTTON_RIGHT:
					{
						KeyInput key(DEVICE_ID_MOUSE, NKCODE_EXT_MOUSEBUTTON_2, KEY_UP);
						NativeKey(key);
					}
					break;
				}
				break;
			default:
#ifndef _WIN32
				if (joystick) {
					joystick->ProcessInput(event);
				}
#endif
				break;
			}
		}
		if (g_QuitRequested)
			break;
		const uint8_t *keys = SDL_GetKeyboardState(NULL);
		UpdateRunLoop();
		if (g_QuitRequested)
			break;
#if !defined(MOBILE_DEVICE) && !USE_HIDE_SDL_SHOWCURSOR
		if (lastUIState != GetUIState()) {
			lastUIState = GetUIState();
			if (lastUIState == UISTATE_INGAME && g_Config.bFullScreen && !g_Config.bShowTouchControls)
				SDL_ShowCursor(SDL_DISABLE);
			if (lastUIState != UISTATE_INGAME && g_Config.bFullScreen)
				SDL_ShowCursor(SDL_ENABLE);
		}
#endif

		if (framecount % 60 == 0) {
			// glsl_refresh(); // auto-reloads modified GLSL shaders once per second.
		}

#ifdef USING_EGL
		eglSwapBuffers(g_eglDisplay, g_eglSurface);
#if !NO_SDLGL		
		if (!keys[SDLK_TAB] || t - lastT >= 1.0/60.0) {
			SDL_GL_SwapWindow(g_Screen);
			lastT = t;
		}
#endif		
#else
		if (!keys[SDLK_TAB] || t - lastT >= 1.0/60.0) {
			SDL_GL_SwapWindow(g_Screen);
			lastT = t;
		}
#endif

		ToggleFullScreenIfFlagSet();
		time_update();
		t = time_now();
		framecount++;
	}
#ifndef _WIN32
	delete joystick;
#endif
	NativeShutdownGraphics();
#if !NO_SDLGL	
	graphicsContext->Shutdown();
	NativeShutdown();
	delete graphicsContext;
#endif	
	// Faster exit, thanks to the OS. Remove this if you want to debug shutdown
	// The speed difference is only really noticable on Linux. On Windows you do notice it though
#ifndef MOBILE_DEVICE
	exit(0);
#endif
	SDL_PauseAudio(1);
	SDL_CloseAudio();
#ifdef USING_EGL
	EGL_Close();
#endif
	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
#if PPSSPP_PLATFORM(RPI)
	bcm_host_deinit();
#endif

	exit(0);
	return 0;
}
