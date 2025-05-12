#include "SDL/SDLJoystick.h"
#include "Core/Config.h"
#include "Common/FileUtil.h"
#include "file/vfs.h"

#include "base/logging.h" //for ILOG

#include <iostream>
#include <string>
#include <thread> //for std::thread

using namespace std;

static int SDLJoystickEventHandlerWrapper(void* userdata, SDL_Event* event)
{
	static_cast<SDLJoystick *>(userdata)->ProcessInput(*event);
	return 0;
}

SDLJoystick::SDLJoystick(bool init_SDL ) : registeredAsEventHandler(false) {
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if (init_SDL) {
		SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
	}

	const char *dbPath = "gamecontrollerdb.txt";
	cout << "loading control pad mappings from " << dbPath << ": ";

	size_t size;
	u8 *mappingData = VFSReadFile(dbPath, &size);
	if (mappingData) {
		SDL_RWops *rw = SDL_RWFromConstMem(mappingData, size);
		// 1 to free the rw after use
		if (SDL_GameControllerAddMappingsFromRW(rw, 1) == -1) {
			cout << "Failed to read mapping data - corrupt?" << endl;
		}
		delete[] mappingData;
	} else {
		cout << "gamecontrollerdb.txt missing" << endl;
	}
	cout << "SUCCESS!" << endl;
	setUpControllers();
}

void SDLJoystick::setUpControllers() {
	int numjoys = SDL_NumJoysticks();
	for (int i = 0; i < numjoys; i++) {
		setUpController(i);
	}
	if (controllers.size() > 0) {
		cout << "pad 1 has been assigned to control pad: " << SDL_GameControllerName(controllers.front()) << endl;
	}
}

void SDLJoystick::setUpController(int deviceIndex) {
	if (!SDL_IsGameController(deviceIndex)) {
		cout << "Control pad device " << deviceIndex << " not supported by SDL game controller database, attempting to create default mapping..." << endl;
		int cbGUID = 33;
		char pszGUID[cbGUID];
		SDL_Joystick* joystick = SDL_JoystickOpen(deviceIndex);
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), pszGUID, cbGUID);
		// create default mapping - this is the PS3 dual shock mapping
		std::string mapping = string(pszGUID) + "," + string(SDL_JoystickName(joystick)) + ",x:b3,a:b0,b:b1,y:b2,back:b8,guide:b10,start:b9,dpleft:b15,dpdown:b14,dpright:b16,dpup:b13,leftshoulder:b4,lefttrigger:a2,rightshoulder:b6,rightshoulder:b5,righttrigger:a5,leftstick:b7,leftstick:b11,rightstick:b12,leftx:a0,lefty:a1,rightx:a3,righty:a4";
		if (SDL_GameControllerAddMapping(mapping.c_str()) == 1){
			cout << "Added default mapping ok" << endl;
		} else {
			cout << "Failed to add default mapping" << endl;
		}
		SDL_JoystickClose(joystick);
	}
	SDL_GameController *controller = SDL_GameControllerOpen(deviceIndex);
	if (controller) {
		if (SDL_GameControllerGetAttached(controller)) {
			controllers.push_back(controller);
			controllerDeviceMap[SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))] = deviceIndex;
			cout << "found control pad: " << SDL_GameControllerName(controller) << ", loading mapping: ";
			auto mapping = SDL_GameControllerMapping(controller);
			if (mapping == NULL) {
				//cout << "FAILED" << endl;
				cout << "Could not find mapping in SDL2 controller database" << endl;
			} else {
				cout << "SUCCESS, mapping is:" << endl << mapping << endl;
			}
		} else {
			SDL_GameControllerClose(controller);
		}
	}
}

SDLJoystick::~SDLJoystick() {
	if (registeredAsEventHandler) {
		SDL_DelEventWatch(SDLJoystickEventHandlerWrapper, this);
	}
	for (auto & controller : controllers) {
		SDL_GameControllerClose(controller);
	}
}

void SDLJoystick::registerEventHandler() {
	SDL_AddEventWatch(SDLJoystickEventHandlerWrapper, this);
	registeredAsEventHandler = true;
}

keycode_t SDLJoystick::getKeycodeForButton(SDL_GameControllerButton button) {
ILOG("<<<<<<<<<<<<getKeycodeForButton: %d, \n", button);
	switch (button) {
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return NKCODE_DPAD_UP;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return NKCODE_DPAD_DOWN;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return NKCODE_DPAD_LEFT;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return NKCODE_DPAD_RIGHT;
	case SDL_CONTROLLER_BUTTON_A:
		return NKCODE_BUTTON_2;
	case SDL_CONTROLLER_BUTTON_B:
		return NKCODE_BUTTON_3;
	case SDL_CONTROLLER_BUTTON_X:
		return NKCODE_BUTTON_4;
	case SDL_CONTROLLER_BUTTON_Y:
		return NKCODE_BUTTON_1;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		return NKCODE_BUTTON_5;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		return NKCODE_BUTTON_6;
	case SDL_CONTROLLER_BUTTON_START:
		return NKCODE_BUTTON_10;
	case SDL_CONTROLLER_BUTTON_BACK:
		return NKCODE_BUTTON_9; // select button
	case SDL_CONTROLLER_BUTTON_GUIDE:
		return NKCODE_BACK; // pause menu
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		return NKCODE_BUTTON_THUMBL;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		return NKCODE_BUTTON_THUMBR;
	case SDL_CONTROLLER_BUTTON_INVALID:
	default:
		return NKCODE_UNKNOWN;
	}
}

static int start_state = 0;
static int select_state = 0;

void SDLJoystick::ProcessInput(SDL_Event &event){
	switch (event.type) {
	case SDL_CONTROLLERBUTTONDOWN:
ILOG("<<<<<<<<<<<<<<<SDL_CONTROLLERBUTTONDOWN\n");
#if USE_EMULATE_MENU_BUTTON
/*
For GPM280Z2:
A==0
B==1
X==2
Y==3
SELECT==4
START==6
L1==9
R1==10
*/
if (event.cbutton.button == 4) {
	start_state = 1;
ILOG("<<<<<<<<<<START DOWN\n");
} else if (event.cbutton.button == 6) {
	select_state = 1;
ILOG("<<<<<<<<<<SELECT DOWN\n");
}
#endif
		if (event.cbutton.state == SDL_PRESSED) {
			auto code = getKeycodeForButton((SDL_GameControllerButton)event.cbutton.button);
			if (code != NKCODE_UNKNOWN) {
				KeyInput key;
				key.flags = KEY_DOWN;
				key.keyCode = code;
				key.deviceId = DEVICE_ID_PAD_0 + getDeviceIndex(event.cbutton.which);
				NativeKey(key);
			}
		}
		break;
	case SDL_CONTROLLERBUTTONUP:
ILOG("<<<<<<<<<<<<<<<SDL_CONTROLLERBUTTONUP\n");
#if USE_EMULATE_MENU_BUTTON
/*
For GPM280Z2:
A==0
B==1
X==2
Y==3
SELECT==4
START==6
L1==9
R1==10
*/
if (event.cbutton.button == 4) {
	start_state = 0;
ILOG("<<<<<<<<<<START UP, %d, %d\n", start_state, select_state);
} else if (event.cbutton.button == 6) {
	select_state = 0;
ILOG("<<<<<<<<<<SELECT UP, %d, %d\n", start_state, select_state);
}

if ((start_state == 1 && select_state == 0) || 
    (start_state == 0 && select_state == 1)) {
	//use START+SELECT to emulate ESC key, symcode == 27, keycode == 111
ILOG("<<<<<<<<<<START+SELECT\n");
fflush(stdout);
	std::thread t([this]() {
		KeyInput key;
		key.flags = KEY_DOWN;
//(InputKeyCode)SDLK_ESCAPE; //
		key.keyCode = (keycode_t)this->getKeycodeForButton((SDL_GameControllerButton)5); //or joystick menu button 5
		key.deviceId = DEVICE_ID_KEYBOARD;
		NativeKey(key);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));		
		key.flags = KEY_UP;
//(InputKeyCode)SDLK_ESCAPE; //
		key.keyCode = (keycode_t)this->getKeycodeForButton((SDL_GameControllerButton)5); //or joystick menu button 5
		key.deviceId = DEVICE_ID_KEYBOARD;
		NativeKey(key);
	});
	t.detach();
}
#endif
		if (event.cbutton.state == SDL_RELEASED) {
			auto code = getKeycodeForButton((SDL_GameControllerButton)event.cbutton.button);
			if (code != NKCODE_UNKNOWN) {
				KeyInput key;
				key.flags = KEY_UP;
				key.keyCode = code;
				key.deviceId = DEVICE_ID_PAD_0 + getDeviceIndex(event.cbutton.which);
				NativeKey(key);
			}
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		AxisInput axis;
		axis.axisId = event.caxis.axis;
		// 1.2 to try to approximate the PSP's clamped rectangular range.
		axis.value = 1.2 * event.caxis.value * g_Config.fXInputAnalogSensitivity / 32767.0f;
		if (axis.value > 1.0f) axis.value = 1.0f;
		if (axis.value < -1.0f) axis.value = -1.0f;
ILOG("<<<<<<<<<<<<<<<SDL_CONTROLLERAXISMOTION %d, %d, %d, %f\n", event.caxis.which, event.caxis.axis, event.caxis.value, axis.value);
#if USE_MOTION_AS_JOYBUTTON
	int emulate_key = -1;
	if (event.caxis.axis == 0 && value == -1) {
		//left
		emulate_key = 21;
	} else if (event.caxis.axis == 0 && value == 1) {
		//right
		emulate_key = 22;
	} else if (event.caxis.axis == 1 && value == -1) {
		//up
		emulate_key = 19;
	} else if (event.caxis.axis == 1 && value == 1) {
		//down
		emulate_key = 20;
	}
	if (emulate_key > 0)
	{
		std::thread t([emulate_key]() {
			KeyInput key;
			key.flags = KEY_DOWN;
			key.keyCode = (InputKeyCode)emulate_key;
			key.deviceId = DEVICE_ID_KEYBOARD;
			NativeKey(key);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));		
			key.flags = KEY_UP;
			key.keyCode = (InputKeyCode)emulate_key;
			key.deviceId = DEVICE_ID_KEYBOARD;
			NativeKey(key);
		});
		t.detach();
	}
#else
		axis.deviceId = DEVICE_ID_PAD_0 + getDeviceIndex(event.caxis.which);
		axis.flags = 0;
		NativeAxis(axis);
#endif
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
ILOG("<<<<<<<<<<<<<<<SDL_CONTROLLERDEVICEREMOVED\n");
		// for removal events, "which" is the instance ID for SDL_CONTROLLERDEVICEREMOVED		
		for (auto it = controllers.begin(); it != controllers.end(); ++it) {
			if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(*it)) == event.cdevice.which) {
				SDL_GameControllerClose(*it);
				controllers.erase(it);
				break;
			}
		}
		break;
	case SDL_CONTROLLERDEVICEADDED:
ILOG("<<<<<<<<<<<<<<<SDL_CONTROLLERDEVICEADDED\n");
		// for add events, "which" is the device index!
		int prevNumControllers = controllers.size();
		setUpController(event.cdevice.which);
		if (prevNumControllers == 0 && controllers.size() > 0) {
			cout << "pad 1 has been assigned to control pad: " << SDL_GameControllerName(controllers.front()) << endl;
		}
		break;
	}
}

int SDLJoystick::getDeviceIndex(int instanceId) {
	auto it = controllerDeviceMap.find(instanceId);
	if (it == controllerDeviceMap.end()) {
			// could not find device
			return -1;
	}
	return it->second;
}
