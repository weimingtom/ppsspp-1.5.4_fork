// Copyright (c) 2013- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include <algorithm>

#include "gfx_es2/gpu_features.h"
#include "i18n/i18n.h"
#include "ui/ui_context.h"
#include "ui/view.h"
#include "ui/viewgroup.h"
#include "ui/ui.h"
#include "profiler/profiler.h"

#include "Common/LogManager.h"
#include "Common/CPUDetect.h"

#include "Core/MemMap.h"
#include "Core/Config.h"
#include "Core/System.h"
#include "Core/CoreParameter.h"
#include "Core/MIPS/MIPSTables.h"
#include "Core/MIPS/JitCommon/JitBlockCache.h"
#include "Core/MIPS/JitCommon/JitCommon.h"
#include "GPU/GPUInterface.h"
#include "GPU/GPUState.h"
#include "UI/MiscScreens.h"
#include "UI/DevScreens.h"
#include "UI/GameSettingsScreen.h"

#ifdef _WIN32
// Want to avoid including the full header here as it includes d3dx.h
int GetD3DXVersion();
#endif

static const char *logLevelList[] = {
	"Notice",
	"Error",
	"Warn",
	"Info",
	"Debug",
	"Verb."
};

void DevMenu::CreatePopupContents(UI::ViewGroup *parent) {
	using namespace UI;
	I18NCategory *dev = GetI18NCategory("Developer");
	I18NCategory *sy = GetI18NCategory("System");

	ScrollView *scroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, WRAP_CONTENT));
	LinearLayout *items = new LinearLayout(ORIENT_VERTICAL);

#if !defined(MOBILE_DEVICE)
	items->Add(new Choice(dev->T("Log View")))->OnClick.Handle(this, &DevMenu::OnLogView);
#endif
	items->Add(new Choice(dev->T("Logging Channels")))->OnClick.Handle(this, &DevMenu::OnLogConfig);
	items->Add(new Choice(sy->T("Developer Tools")))->OnClick.Handle(this, &DevMenu::OnDeveloperTools);
	items->Add(new Choice(dev->T("Jit Compare")))->OnClick.Handle(this, &DevMenu::OnJitCompare);
	items->Add(new Choice(dev->T("Shader Viewer")))->OnClick.Handle(this, &DevMenu::OnShaderView);
	items->Add(new Choice(dev->T("Toggle Freeze")))->OnClick.Handle(this, &DevMenu::OnFreezeFrame);
	items->Add(new Choice(dev->T("Dump Frame GPU Commands")))->OnClick.Handle(this, &DevMenu::OnDumpFrame);
	items->Add(new Choice(dev->T("Toggle Audio Debug")))->OnClick.Handle(this, &DevMenu::OnToggleAudioDebug);
#ifdef USE_PROFILER
	items->Add(new CheckBox(&g_Config.bShowFrameProfiler, dev->T("Frame Profiler"), ""));
#endif

	scroll->Add(items);
	parent->Add(scroll);

	RingbufferLogListener *ring = LogManager::GetInstance()->GetRingbufferListener();
	if (ring) {
		ring->SetEnabled(true);
	}
}

UI::EventReturn DevMenu::OnToggleAudioDebug(UI::EventParams &e) {
	g_Config.bShowAudioDebug = !g_Config.bShowAudioDebug;
	return UI::EVENT_DONE;
}


UI::EventReturn DevMenu::OnLogView(UI::EventParams &e) {
	UpdateUIState(UISTATE_PAUSEMENU);
	screenManager()->push(new LogScreen());
	return UI::EVENT_DONE;
}

UI::EventReturn DevMenu::OnLogConfig(UI::EventParams &e) {
	UpdateUIState(UISTATE_PAUSEMENU);
	screenManager()->push(new LogConfigScreen());
	return UI::EVENT_DONE;
}

UI::EventReturn DevMenu::OnDeveloperTools(UI::EventParams &e) {
	UpdateUIState(UISTATE_PAUSEMENU);
	screenManager()->push(new DeveloperToolsScreen());
	return UI::EVENT_DONE;
}

UI::EventReturn DevMenu::OnJitCompare(UI::EventParams &e) {
	UpdateUIState(UISTATE_PAUSEMENU);
	screenManager()->push(new JitCompareScreen());
	return UI::EVENT_DONE;
}

UI::EventReturn DevMenu::OnShaderView(UI::EventParams &e) {
	UpdateUIState(UISTATE_PAUSEMENU);
	screenManager()->push(new ShaderListScreen());
	return UI::EVENT_DONE;
}



UI::EventReturn DevMenu::OnFreezeFrame(UI::EventParams &e) {
	if (PSP_CoreParameter().frozen) {
		PSP_CoreParameter().frozen = false;
	} else {
		PSP_CoreParameter().freezeNext = true;
	}
	return UI::EVENT_DONE;
}

UI::EventReturn DevMenu::OnDumpFrame(UI::EventParams &e) {
	gpu->DumpNextFrame();
	return UI::EVENT_DONE;
}

void DevMenu::dialogFinished(const Screen_ *dialog, DialogResult result) {
	UpdateUIState(UISTATE_INGAME);
	// Close when a subscreen got closed.
	// TODO: a bug in screenmanager causes this not to work here.
	// TriggerFinish(DR_OK);
}

void LogScreen::UpdateLog() {
	using namespace UI;
	RingbufferLogListener *ring = LogManager::GetInstance()->GetRingbufferListener();
	if (!ring)
		return;
	vert_->Clear();
	for (int i = ring->GetCount() - 1; i >= 0; i--) {
		TextView *v = vert_->Add(new TextView(ring->TextAt(i), FLAG_DYNAMIC_ASCII, false));
		uint32_t color = 0xFFFFFF;
		switch (ring->LevelAt(i)) {
		case LogTypes::LDEBUG: color = 0xE0E0E0; break;
		case LogTypes::LWARNING: color = 0x50FFFF; break;
		case LogTypes::LERROR: color = 0x5050FF; break;
		case LogTypes::LNOTICE: color = 0x30FF30; break;
		case LogTypes::LINFO: color = 0xFFFFFF; break;
		case LogTypes::LVERBOSE: color = 0xC0C0C0; break;
		}
		v->SetTextColor(0xFF000000 | color);
	}
	toBottom_ = true;
}

void LogScreen::update() {
	UIDialogScreenWithBackground::update();
	if (toBottom_) {
		toBottom_ = false;
		scroll_->ScrollToBottom();
	}
}

void LogScreen::CreateViews() {
	using namespace UI;
	I18NCategory *di = GetI18NCategory("Dialog");

	LinearLayout *outer = new LinearLayout(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, WRAP_CONTENT));
	root_ = outer;

	scroll_ = outer->Add(new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(1.0)));
	LinearLayout *bottom = outer->Add(new LinearLayout(ORIENT_HORIZONTAL, new LayoutParams(FILL_PARENT, WRAP_CONTENT)));
	bottom->Add(new Button(di->T("Back")))->OnClick.Handle<UIScreen>(this, &UIScreen::OnBack);
	cmdLine_ = bottom->Add(new TextEdit("", "Command Line", new LinearLayoutParams(1.0)));
	cmdLine_->OnEnter.Handle(this, &LogScreen::OnSubmit);
	bottom->Add(new Button(di->T("Submit")))->OnClick.Handle(this, &LogScreen::OnSubmit);

	vert_ = scroll_->Add(new LinearLayout(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, WRAP_CONTENT)));
	vert_->SetSpacing(0);

	UpdateLog();
}

UI::EventReturn LogScreen::OnSubmit(UI::EventParams &e) {
	std::string cmd = cmdLine_->GetText();

	// TODO: Can add all sorts of fun stuff here that we can't be bothered writing proper UI for, like various memdumps etc.

	NOTICE_LOG(SYSTEM, "Submitted: %s", cmd.c_str());

	UpdateLog();
	cmdLine_->SetText("");
	cmdLine_->SetFocus();
	return UI::EVENT_DONE;
}

void LogConfigScreen::CreateViews() {
	using namespace UI;

	I18NCategory *di = GetI18NCategory("Dialog");
	I18NCategory *dev = GetI18NCategory("Developer");

	root_ = new ScrollView(ORIENT_VERTICAL);

	LinearLayout *vert = root_->Add(new LinearLayout(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, WRAP_CONTENT)));
	vert->SetSpacing(0);

	LinearLayout *topbar = new LinearLayout(ORIENT_HORIZONTAL);
	topbar->Add(new Choice(di->T("Back")))->OnClick.Handle<UIScreen>(this, &UIScreen::OnBack);
	topbar->Add(new Choice(di->T("Toggle All")))->OnClick.Handle(this, &LogConfigScreen::OnToggleAll);
	topbar->Add(new Choice(di->T("Enable All")))->OnClick.Handle(this, &LogConfigScreen::OnEnableAll);
	topbar->Add(new Choice(di->T("Disable All")))->OnClick.Handle(this, &LogConfigScreen::OnDisableAll);
	topbar->Add(new Choice(dev->T("Log Level")))->OnClick.Handle(this, &LogConfigScreen::OnLogLevel);

	vert->Add(topbar);

	vert->Add(new ItemHeader(dev->T("Logging Channels")));

	LogManager *logMan = LogManager::GetInstance();

	int cellSize = 400;

	UI::GridLayoutSettings gridsettings(cellSize, 64, 5);
	gridsettings.fillCells = true;
	GridLayout *grid = vert->Add(new GridLayout(gridsettings, new LayoutParams(FILL_PARENT, WRAP_CONTENT)));

	for (int i = 0; i < LogManager::GetNumChannels(); i++) {
		LogTypes::LOG_TYPE type = (LogTypes::LOG_TYPE)i;
		LogChannel *chan = logMan->GetLogChannel(type);
		LinearLayout *row = new LinearLayout(ORIENT_HORIZONTAL, new LinearLayoutParams(cellSize - 50, WRAP_CONTENT));
		row->SetSpacing(0);
		row->Add(new CheckBox(&chan->enabled, "", "", new LinearLayoutParams(50, WRAP_CONTENT)));
		row->Add(new PopupMultiChoice((int *)&chan->level, chan->m_shortName, logLevelList, 1, 6, 0, screenManager(), new LinearLayoutParams(1.0)));
		grid->Add(row);
	}
}

UI::EventReturn LogConfigScreen::OnToggleAll(UI::EventParams &e) {
	LogManager *logMan = LogManager::GetInstance();
	for (int i = 0; i < LogManager::GetNumChannels(); i++) {
		LogChannel *chan = logMan->GetLogChannel((LogTypes::LOG_TYPE)i);
		chan->enabled = !chan->enabled;
	}
	return UI::EVENT_DONE;
}

UI::EventReturn LogConfigScreen::OnEnableAll(UI::EventParams &e) {
	LogManager *logMan = LogManager::GetInstance();
	for (int i = 0; i < LogManager::GetNumChannels(); i++) {
		LogChannel *chan = logMan->GetLogChannel((LogTypes::LOG_TYPE)i);
		chan->enabled = true;
	}
	return UI::EVENT_DONE;
}

UI::EventReturn LogConfigScreen::OnDisableAll(UI::EventParams &e) {
	LogManager *logMan = LogManager::GetInstance();
	for (int i = 0; i < LogManager::GetNumChannels(); i++) {
		LogChannel *chan = logMan->GetLogChannel((LogTypes::LOG_TYPE)i);
		chan->enabled = false;
	}
	return UI::EVENT_DONE;
}

UI::EventReturn LogConfigScreen::OnLogLevelChange(UI::EventParams &e) {
	RecreateViews();
	return UI::EVENT_DONE;
}

UI::EventReturn LogConfigScreen::OnLogLevel(UI::EventParams &e) {
	I18NCategory *dev = GetI18NCategory("Developer");

	auto logLevelScreen = new LogLevelScreen(dev->T("Log Level"));
	logLevelScreen->OnChoice.Handle(this, &LogConfigScreen::OnLogLevelChange);
	if (e.v)
		logLevelScreen->SetPopupOrigin(e.v);
	screenManager()->push(logLevelScreen);
	return UI::EVENT_DONE;
}

LogLevelScreen::LogLevelScreen(const std::string &title) : ListPopupScreen(title) {
	int NUMLOGLEVEL = 6;    
	std::vector<std::string> list;
	for (int i = 0; i < NUMLOGLEVEL; ++i) {
		list.push_back(logLevelList[i]);
	}
	adaptor_ = UI::StringVectorListAdaptor(list, -1);
}

void LogLevelScreen::OnCompleted(DialogResult result) {
	if (result != DR_OK)
		return;
	int selected = listView_->GetSelected();
	LogManager *logMan = LogManager::GetInstance();
	
	for (int i = 0; i < LogManager::GetNumChannels(); ++i) {
		LogTypes::LOG_TYPE type = (LogTypes::LOG_TYPE)i;
		LogChannel *chan = logMan->GetLogChannel(type);
		if (chan->enabled)
			chan->level = (LogTypes::LOG_LEVELS)(selected + 1);
	}
}

const char *GetCompilerABI() {
#if PPSSPP_ARCH(ARMV7)
	return "armeabi-v7a";
#elif PPSSPP_ARCH(ARM)
	return "armeabi";
#elif PPSSPP_ARCH(ARM64)
	return "arm64";
#elif PPSSPP_ARCH(X86)
	return "x86";
#elif PPSSPP_ARCH(AMD64)
	return "x86-64";
#else
	return "other";
#endif
}

void SystemInfoScreen::CreateViews() {
	using namespace Draw;
	using namespace UI;

	// NOTE: Do not translate this section. It will change a lot and will be impossible to keep up.
	I18NCategory *di = GetI18NCategory("Dialog");
	root_ = new AnchorLayout(new LayoutParams(FILL_PARENT, FILL_PARENT));

	ViewGroup *leftColumn = new AnchorLayout(new LinearLayoutParams(1.0f));
	root_->Add(leftColumn);

	AddStandardBack(root_);

	TabHolder *tabHolder = new TabHolder(ORIENT_VERTICAL, 225, new AnchorLayoutParams(10, 0, 10, 0, false));
	tabHolder->SetTag("DevSystemInfo");

	root_->Add(tabHolder);
	ViewGroup *deviceSpecsScroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, FILL_PARENT));
	deviceSpecsScroll->SetTag("DevSystemInfoDeviceSpecs");
	LinearLayout *deviceSpecs = new LinearLayout(ORIENT_VERTICAL);
	deviceSpecs->SetSpacing(0);
	deviceSpecsScroll->Add(deviceSpecs);
	tabHolder->AddTab("Device Info", deviceSpecsScroll);

	deviceSpecs->Add(new ItemHeader("System Information"));
	deviceSpecs->Add(new InfoItem("Name", System_GetProperty(SYSPROP_NAME)));
	deviceSpecs->Add(new InfoItem("Lang/Region", System_GetProperty(SYSPROP_LANGREGION)));
	std::string board = System_GetProperty(SYSPROP_BOARDNAME);
	if (!board.empty())
		deviceSpecs->Add(new InfoItem("Board", board));
	deviceSpecs->Add(new InfoItem("ABI", GetCompilerABI()));
#ifdef _WIN32
	if (IsDebuggerPresent()) {
		deviceSpecs->Add(new InfoItem("Debugger Present", "Yes"));
	}
#endif

	deviceSpecs->Add(new ItemHeader("CPU Information"));
	deviceSpecs->Add(new InfoItem("Name", cpu_info.brand_string));
#if defined(ARM) || defined(ARM64) || defined(MIPS)
	deviceSpecs->Add(new InfoItem("Cores", StringFromInt(cpu_info.num_cores)));
#else
	int totalThreads = cpu_info.num_cores * cpu_info.logical_cpu_count;
	std::string cores = StringFromFormat("%d (%d per core, %d cores)", totalThreads, cpu_info.logical_cpu_count, cpu_info.num_cores);
	deviceSpecs->Add(new InfoItem("Threads", cores));
#endif
	deviceSpecs->Add(new ItemHeader("GPU Information"));

	DrawContext *draw = screenManager()->getDrawContext();

	deviceSpecs->Add(new InfoItem("3D API", std::string(draw->GetInfoString(InfoField::APINAME))));
	deviceSpecs->Add(new InfoItem("Vendor", std::string(draw->GetInfoString(InfoField::VENDORSTRING))));
	std::string vendor = draw->GetInfoString(InfoField::VENDOR);
	if (vendor.size())
		deviceSpecs->Add(new InfoItem("Vendor (detected)", vendor));
	deviceSpecs->Add(new InfoItem("Driver Version", draw->GetInfoString(InfoField::DRIVER)));
#ifdef _WIN32
	if (g_Config.iGPUBackend != GPU_BACKEND_VULKAN)
		deviceSpecs->Add(new InfoItem("Driver Version", System_GetProperty(SYSPROP_GPUDRIVER_VERSION)));
#if !PPSSPP_PLATFORM(UWP)
	if (GetGPUBackend() == GPUBackend::DIRECT3D9) {
		deviceSpecs->Add(new InfoItem("D3DX Version", StringFromFormat("%d", GetD3DXVersion())));
	}
#endif
#endif
	deviceSpecs->Add(new ItemHeader("OS Information"));
	deviceSpecs->Add(new InfoItem("Memory Page Size", StringFromFormat("%d bytes", GetMemoryProtectPageSize())));
	deviceSpecs->Add(new InfoItem("RW/RX exclusive: ", PlatformIsWXExclusive() ? "Yes" : "No"));
#ifdef ANDROID
	deviceSpecs->Add(new InfoItem("Sustained perf mode: ", System_GetPropertyBool(SYSPROP_SUPPORTS_SUSTAINED_PERF_MODE) ? "Yes" : "No"));
#endif

	const char *build = "Release";
#ifdef _DEBUG
	build = "Debug";
#endif
	deviceSpecs->Add(new InfoItem("PPSSPP build: ", build));

#ifdef __ANDROID__
	deviceSpecs->Add(new ItemHeader("Audio Information"));
	deviceSpecs->Add(new InfoItem("Sample rate", StringFromFormat("%d Hz", System_GetPropertyInt(SYSPROP_AUDIO_SAMPLE_RATE))));
	deviceSpecs->Add(new InfoItem("Frames per buffer", StringFromFormat("%d", System_GetPropertyInt(SYSPROP_AUDIO_FRAMES_PER_BUFFER))));
	deviceSpecs->Add(new InfoItem("Optimal sample rate", StringFromFormat("%d Hz", System_GetPropertyInt(SYSPROP_AUDIO_OPTIMAL_SAMPLE_RATE))));
	deviceSpecs->Add(new InfoItem("Optimal frames per buffer", StringFromFormat("%d", System_GetPropertyInt(SYSPROP_AUDIO_OPTIMAL_FRAMES_PER_BUFFER))));

	deviceSpecs->Add(new ItemHeader("Display Information"));
	deviceSpecs->Add(new InfoItem("Native Resolution", StringFromFormat("%dx%d",
		System_GetPropertyInt(SYSPROP_DISPLAY_XRES),
		System_GetPropertyInt(SYSPROP_DISPLAY_YRES))));
	deviceSpecs->Add(new InfoItem("Refresh rate", StringFromFormat("%0.3f Hz", (float)System_GetPropertyInt(SYSPROP_DISPLAY_REFRESH_RATE) / 1000.0f)));
#endif


	deviceSpecs->Add(new ItemHeader("Version Information"));
	std::string apiVersion;
	if (GetGPUBackend() == GPUBackend::OPENGL) {
		if (gl_extensions.IsGLES) {
			apiVersion = StringFromFormat("v%d.%d.%d ES", gl_extensions.ver[0], gl_extensions.ver[1], gl_extensions.ver[2]);
		} else {
			apiVersion = StringFromFormat("v%d.%d.%d", gl_extensions.ver[0], gl_extensions.ver[1], gl_extensions.ver[2]);
		}
	} else {
		apiVersion = draw->GetInfoString(InfoField::APIVERSION);
		if (apiVersion.size() > 30)
			apiVersion.resize(30);
	}
	deviceSpecs->Add(new InfoItem("API Version", apiVersion));
	deviceSpecs->Add(new InfoItem("Shading Language", draw->GetInfoString(InfoField::SHADELANGVERSION)));

#ifdef __ANDROID__
	std::string moga = System_GetProperty(SYSPROP_MOGA_VERSION);
	if (moga.empty()) {
		moga = "(none detected)";
	}
	deviceSpecs->Add(new InfoItem("Moga", moga));
#endif

#ifdef __ANDROID__
	char temp[256];
	sprintf(temp, "%dx%d", System_GetPropertyInt(SYSPROP_DISPLAY_XRES), System_GetPropertyInt(SYSPROP_DISPLAY_YRES));
	deviceSpecs->Add(new InfoItem("Display resolution", temp));
#endif

	ViewGroup *buildConfigScroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, FILL_PARENT));
	buildConfigScroll->SetTag("DevSystemInfoBuildConfig");
	LinearLayout *buildConfig = new LinearLayout(ORIENT_VERTICAL);
	buildConfig->SetSpacing(0);
	buildConfigScroll->Add(buildConfig);
	tabHolder->AddTab("Build Config", buildConfigScroll);

	buildConfig->Add(new ItemHeader("Build Configuration"));
#ifdef JENKINS
	buildConfig->Add(new InfoItem("Built by", "Jenkins"));
#endif
#ifdef _DEBUG
	buildConfig->Add(new InfoItem("_DEBUG", ""));
#else
	buildConfig->Add(new InfoItem("NDEBUG", ""));
#endif
#ifdef USING_GLES2
	buildConfig->Add(new InfoItem("USING_GLES2", ""));
#endif
#ifdef MOBILE_DEVICE
	buildConfig->Add(new InfoItem("MOBILE_DEVICE", ""));
#endif
	if (System_GetPropertyBool(SYSPROP_APP_GOLD)) {
		buildConfig->Add(new InfoItem("GOLD", ""));
	}

	ViewGroup *cpuExtensionsScroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, FILL_PARENT));
	cpuExtensionsScroll->SetTag("DevSystemInfoCPUExt");
	LinearLayout *cpuExtensions = new LinearLayout(ORIENT_VERTICAL);
	cpuExtensions->SetSpacing(0);
	cpuExtensionsScroll->Add(cpuExtensions);

	tabHolder->AddTab("CPU Extensions", cpuExtensionsScroll);

	cpuExtensions->Add(new ItemHeader("CPU Extensions"));
	std::vector<std::string> exts;
	SplitString(cpu_info.Summarize(), ',', exts);
	for (size_t i = 2; i < exts.size(); i++) {
		cpuExtensions->Add(new TextView(exts[i]))->SetFocusable(true);
	}

	ViewGroup *gpuExtensionsScroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, FILL_PARENT));
	gpuExtensionsScroll->SetTag("DevSystemInfoOGLExt");
	LinearLayout *gpuExtensions = new LinearLayout(ORIENT_VERTICAL);
	gpuExtensions->SetSpacing(0);
	gpuExtensionsScroll->Add(gpuExtensions);

	if (g_Config.iGPUBackend == GPU_BACKEND_OPENGL) {
		tabHolder->AddTab("OGL Extensions", gpuExtensionsScroll);

		if (!gl_extensions.IsGLES) {
			gpuExtensions->Add(new ItemHeader("OpenGL Extensions"));
		} else if (gl_extensions.GLES3) {
			gpuExtensions->Add(new ItemHeader("OpenGL ES 3.0 Extensions"));
		} else {
			gpuExtensions->Add(new ItemHeader("OpenGL ES 2.0 Extensions"));
		}
		exts.clear();
		SplitString(g_all_gl_extensions, ' ', exts);
		std::sort(exts.begin(), exts.end());
		for (size_t i = 0; i < exts.size(); i++) {
			gpuExtensions->Add(new TextView(exts[i]))->SetFocusable(true);
		}

		exts.clear();
		SplitString(g_all_egl_extensions, ' ', exts);
		std::sort(exts.begin(), exts.end());

		// If there aren't any EGL extensions, no need to show the tab.
		if (exts.size() > 0) {
			ViewGroup *eglExtensionsScroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, FILL_PARENT));
			eglExtensionsScroll->SetTag("DevSystemInfoEGLExt");
			LinearLayout *eglExtensions = new LinearLayout(ORIENT_VERTICAL);
			eglExtensions->SetSpacing(0);
			eglExtensionsScroll->Add(eglExtensions);

			tabHolder->AddTab("EGL Extensions", eglExtensionsScroll);

			eglExtensions->Add(new ItemHeader("EGL Extensions"));

			for (size_t i = 0; i < exts.size(); i++) {
				eglExtensions->Add(new TextView(exts[i]))->SetFocusable(true);
			}
		}
	} else if (g_Config.iGPUBackend == GPU_BACKEND_VULKAN) {
		tabHolder->AddTab("Vulkan Features", gpuExtensionsScroll);

		gpuExtensions->Add(new ItemHeader("Vulkan Features"));
		std::vector<std::string> features = draw->GetFeatureList();
		for (auto &feature : features) {
			gpuExtensions->Add(new TextView(feature))->SetFocusable(true);
		}
		gpuExtensions->Add(new ItemHeader("Vulkan Extensions"));
		std::vector<std::string> extensions = draw->GetExtensionList();
		for (auto &extension : extensions) {
			gpuExtensions->Add(new TextView(extension))->SetFocusable(true);
		}
	}
}

void AddressPromptScreen::CreatePopupContents(UI::ViewGroup *parent) {
	using namespace UI;

	I18NCategory *dev = GetI18NCategory("Developer");

	addrView_ = new TextView(dev->T("Enter address"), ALIGN_HCENTER, false);
	parent->Add(addrView_);

	ViewGroup *grid = new GridLayout(GridLayoutSettings(60, 40));
	parent->Add(grid);

	for (int i = 0; i < 16; ++i) {
		char temp[16];
		snprintf(temp, 16, " %X ", i);
		buttons_[i] = new Button(temp);
		grid->Add(buttons_[i])->OnClick.Handle(this, &AddressPromptScreen::OnDigitButton);
	}

	parent->Add(new Button(dev->T("Backspace")))->OnClick.Handle(this, &AddressPromptScreen::OnBackspace);
}

void AddressPromptScreen::OnCompleted(DialogResult result) {
	if (result == DR_OK) {
		UI::EventParams e{};
		e.v = root_;
		e.a = addr_;
		OnChoice.Trigger(e);
	}
}

UI::EventReturn AddressPromptScreen::OnDigitButton(UI::EventParams &e) {
	for (int i = 0; i < 16; ++i) {
		if (buttons_[i] == e.v) {
			AddDigit(i);
		}
	}
	return UI::EVENT_DONE;
}

UI::EventReturn AddressPromptScreen::OnBackspace(UI::EventParams &e) {
	BackspaceDigit();
	return UI::EVENT_DONE;
}

void AddressPromptScreen::AddDigit(int n) {
	if ((addr_ & 0xF0000000) == 0) {
		addr_ = addr_ * 16 + n;
	}
	UpdatePreviewDigits();
}

void AddressPromptScreen::BackspaceDigit() {
	addr_ /= 16;
	UpdatePreviewDigits();
}

void AddressPromptScreen::UpdatePreviewDigits() {
	I18NCategory *dev = GetI18NCategory("Developer");

	if (addr_ != 0) {
		char temp[32];
		snprintf(temp, 32, "%8X", addr_);
		addrView_->SetText(temp);
	} else {
		addrView_->SetText(dev->T("Enter address"));
	}
}

bool AddressPromptScreen::key(const KeyInput &key) {
	if (key.flags & KEY_DOWN) {
		if (key.keyCode >= NKCODE_0 && key.keyCode <= NKCODE_9) {
			AddDigit(key.keyCode - NKCODE_0);
		} else if (key.keyCode >= NKCODE_A && key.keyCode <= NKCODE_F) {
			AddDigit(10 + key.keyCode - NKCODE_A);
		// NKCODE_DEL is backspace.
		} else if (key.keyCode == NKCODE_DEL) {
			BackspaceDigit();
		} else if (key.keyCode == NKCODE_ENTER) {
			TriggerFinish(DR_OK);
		} else {
			return UIDialogScreen::key(key);
		}
	} else {
		return UIDialogScreen::key(key);
	}
	return true;
}

// Three panes: Block chooser, MIPS view, ARM/x86 view
void JitCompareScreen::CreateViews() {
	I18NCategory *di = GetI18NCategory("Dialog");
	I18NCategory *dev = GetI18NCategory("Developer");

	using namespace UI;
	
	root_ = new LinearLayout(ORIENT_HORIZONTAL);

	ScrollView *leftColumnScroll = root_->Add(new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(1.0f)));
	LinearLayout *leftColumn = leftColumnScroll->Add(new LinearLayout(ORIENT_VERTICAL));

	ScrollView *midColumnScroll = root_->Add(new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(2.0f)));
	LinearLayout *midColumn = midColumnScroll->Add(new LinearLayout(ORIENT_VERTICAL));
	midColumn->SetTag("JitCompareLeftDisasm");
	leftDisasm_ = midColumn->Add(new LinearLayout(ORIENT_VERTICAL));
	leftDisasm_->SetSpacing(0.0f);

	ScrollView *rightColumnScroll = root_->Add(new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(2.0f)));
	rightColumnScroll->SetTag("JitCompareRightDisasm");
	LinearLayout *rightColumn = rightColumnScroll->Add(new LinearLayout(ORIENT_VERTICAL));
	rightDisasm_ = rightColumn->Add(new LinearLayout(ORIENT_VERTICAL));
	rightDisasm_->SetSpacing(0.0f);

	leftColumn->Add(new Choice(dev->T("Current")))->OnClick.Handle(this, &JitCompareScreen::OnCurrentBlock);
	leftColumn->Add(new Choice(dev->T("By Address")))->OnClick.Handle(this, &JitCompareScreen::OnSelectBlock);
	leftColumn->Add(new Choice(dev->T("Prev")))->OnClick.Handle(this, &JitCompareScreen::OnPrevBlock);
	leftColumn->Add(new Choice(dev->T("Next")))->OnClick.Handle(this, &JitCompareScreen::OnNextBlock);
	leftColumn->Add(new Choice(dev->T("Random")))->OnClick.Handle(this, &JitCompareScreen::OnRandomBlock);
	leftColumn->Add(new Choice(dev->T("FPU")))->OnClick.Handle(this, &JitCompareScreen::OnRandomFPUBlock);
	leftColumn->Add(new Choice(dev->T("VFPU")))->OnClick.Handle(this, &JitCompareScreen::OnRandomVFPUBlock);
	leftColumn->Add(new Choice(dev->T("Stats")))->OnClick.Handle(this, &JitCompareScreen::OnShowStats);
	leftColumn->Add(new Choice(di->T("Back")))->OnClick.Handle<UIScreen>(this, &UIScreen::OnBack);
	blockName_ = leftColumn->Add(new TextView(dev->T("No block")));
	blockAddr_ = leftColumn->Add(new TextEdit("", "", new LayoutParams(FILL_PARENT, WRAP_CONTENT)));
	blockAddr_->OnTextChange.Handle(this, &JitCompareScreen::OnAddressChange);
	blockStats_ = leftColumn->Add(new TextView(""));

	EventParams ignore{};
	OnCurrentBlock(ignore);
}

void JitCompareScreen::UpdateDisasm() {
	leftDisasm_->Clear();
	rightDisasm_->Clear();

	using namespace UI;

	I18NCategory *dev = GetI18NCategory("Developer");

	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();

	char temp[256];
	snprintf(temp, sizeof(temp), "%i/%i", currentBlock_, blockCache->GetNumBlocks());
	blockName_->SetText(temp);

	if (currentBlock_ < 0 || currentBlock_ >= blockCache->GetNumBlocks()) {
		leftDisasm_->Add(new TextView(dev->T("No block")));
		rightDisasm_->Add(new TextView(dev->T("No block")));
		blockStats_->SetText("");
		return;
	}

	JitBlock *block = blockCache->GetBlock(currentBlock_);

	snprintf(temp, sizeof(temp), "%08x", block->originalAddress);
	blockAddr_->SetText(temp);

	// Alright. First generate the MIPS disassembly.
	
	// TODO: Need a way to communicate branch continuing.
	for (u32 addr = block->originalAddress; addr <= block->originalAddress + block->originalSize * 4; addr += 4) {
		char temp[256];
		MIPSDisAsm(Memory::Read_Instruction(addr), addr, temp, true);
		std::string mipsDis = temp;
		leftDisasm_->Add(new TextView(mipsDis))->SetFocusable(true);
	}

#if defined(ARM)
	std::vector<std::string> targetDis = DisassembleArm2(block->normalEntry, block->codeSize);
#elif defined(ARM64)
	std::vector<std::string> targetDis = DisassembleArm64(block->normalEntry, block->codeSize);
#elif defined(_M_IX86) || defined(_M_X64)
	std::vector<std::string> targetDis = DisassembleX86(block->normalEntry, block->codeSize);
#endif
#if defined(ARM) || defined(ARM64) || defined(_M_IX86) || defined(_M_X64)
	for (size_t i = 0; i < targetDis.size(); i++) {
		rightDisasm_->Add(new TextView(targetDis[i]))->SetFocusable(true);
	}
#endif

	int numMips = leftDisasm_->GetNumSubviews();
	int numHost = rightDisasm_->GetNumSubviews();

	snprintf(temp, sizeof(temp), "%d to %d : %d%%", numMips, numHost, 100 * numHost / numMips);
	blockStats_->SetText(temp);
}

UI::EventReturn JitCompareScreen::OnAddressChange(UI::EventParams &e) {
	if (!MIPSComp::jit) {
		return UI::EVENT_DONE;
	}
	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();
	if (!blockCache)
		return UI::EVENT_DONE;
	u32 addr;
	if (blockAddr_->GetText().size() > 8)
		return UI::EVENT_DONE;
	if (1 == sscanf(blockAddr_->GetText().c_str(), "%08x", &addr)) {
		if (Memory::IsValidAddress(addr)) {
			currentBlock_ = blockCache->GetBlockNumberFromStartAddress(addr);
			UpdateDisasm();
		}
	}
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnShowStats(UI::EventParams &e) {
	if (!MIPSComp::jit) {
		return UI::EVENT_DONE;
	}

	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();
	BlockCacheStats bcStats;
	blockCache->ComputeStats(bcStats);
	NOTICE_LOG(JIT, "Num blocks: %i", bcStats.numBlocks);
	NOTICE_LOG(JIT, "Average Bloat: %0.2f%%", 100 * bcStats.avgBloat);
	NOTICE_LOG(JIT, "Min Bloat: %0.2f%%  (%08x)", 100 * bcStats.minBloat, bcStats.minBloatBlock);
	NOTICE_LOG(JIT, "Max Bloat: %0.2f%%  (%08x)", 100 * bcStats.maxBloat, bcStats.maxBloatBlock);

	int ctr = 0, sz = (int)bcStats.bloatMap.size();
	for (auto iter : bcStats.bloatMap) {
		if (ctr < 10 || ctr > sz - 10) {
			NOTICE_LOG(JIT, "%08x: %f", iter.second, iter.first);
		} else if (ctr == 10) {
			NOTICE_LOG(JIT, "...");
		}
		ctr++;
	}

	return UI::EVENT_DONE;
}


UI::EventReturn JitCompareScreen::OnSelectBlock(UI::EventParams &e) {
	I18NCategory *dev = GetI18NCategory("Developer");

	auto addressPrompt = new AddressPromptScreen(dev->T("Block address"));
	addressPrompt->OnChoice.Handle(this, &JitCompareScreen::OnBlockAddress);
	screenManager()->push(addressPrompt);
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnPrevBlock(UI::EventParams &e) {
	currentBlock_--;
	UpdateDisasm();
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnNextBlock(UI::EventParams &e) {
	currentBlock_++;
	UpdateDisasm();
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnBlockAddress(UI::EventParams &e) {
	if (!MIPSComp::jit) {
		return UI::EVENT_DONE;
	}

	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();
	if (!blockCache)
		return UI::EVENT_DONE;

	if (Memory::IsValidAddress(e.a)) {
		currentBlock_ = blockCache->GetBlockNumberFromStartAddress(e.a);
	} else {
		currentBlock_ = -1;
	}
	UpdateDisasm();
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnRandomBlock(UI::EventParams &e) {
	if (!MIPSComp::jit) {
		return UI::EVENT_DONE;
	}

	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();
	if (!blockCache)
		return UI::EVENT_DONE;

	int numBlocks = blockCache->GetNumBlocks();
	if (numBlocks > 0) {
		currentBlock_ = rand() % numBlocks;
	}
	UpdateDisasm();
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnRandomVFPUBlock(UI::EventParams &e) {
	OnRandomBlock(IS_VFPU);
	return UI::EVENT_DONE;
}

UI::EventReturn JitCompareScreen::OnRandomFPUBlock(UI::EventParams &e) {
	OnRandomBlock(IS_FPU);
	return UI::EVENT_DONE;
}

void JitCompareScreen::OnRandomBlock(int flag) {
	if (!MIPSComp::jit) {
		return;
	}
	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();
	if (!blockCache)
		return;

	int numBlocks = blockCache->GetNumBlocks();
	if (numBlocks > 0) {
		bool anyWanted = false;
		int tries = 0;
		while (!anyWanted && tries < 10000) {
			currentBlock_ = rand() % numBlocks;
			const JitBlock *b = blockCache->GetBlock(currentBlock_);
			for (u32 addr = b->originalAddress; addr <= b->originalAddress + b->originalSize; addr += 4) {
				MIPSOpcode opcode = Memory::Read_Instruction(addr);
				if (MIPSGetInfo(opcode) & flag) {
					char temp[256];
					MIPSDisAsm(opcode, addr, temp);
					// INFO_LOG(HLE, "Stopping VFPU instruction: %s", temp);
					anyWanted = true;
					break;
				}
			}
			tries++;
		}
	}
	UpdateDisasm();
}

UI::EventReturn JitCompareScreen::OnCurrentBlock(UI::EventParams &e) {
	if (!MIPSComp::jit) {
		return UI::EVENT_DONE;
	}
	JitBlockCache *blockCache = MIPSComp::jit->GetBlockCache();
	if (!blockCache)
		return UI::EVENT_DONE;
	std::vector<int> blockNum;
	blockCache->GetBlockNumbersFromAddress(currentMIPS->pc, &blockNum);
	if (blockNum.size() > 0) {
		currentBlock_ = blockNum[0];
	} else {
		currentBlock_ = -1;
	}
	UpdateDisasm();
	return UI::EVENT_DONE;
}

int ShaderListScreen::ListShaders(DebugShaderType shaderType, UI::LinearLayout *view) {
	using namespace UI;
	std::vector<std::string> shaderIds_ = gpu->DebugGetShaderIDs(shaderType);
	int count = 0;
	for (auto id : shaderIds_) {
		Choice *choice = view->Add(new Choice(gpu->DebugGetShaderString(id, shaderType, SHADER_STRING_SHORT_DESC)));
		choice->SetTag(id);
		choice->OnClick.Handle(this, &ShaderListScreen::OnShaderClick);
		count++;
	}
	return count;
}

struct { DebugShaderType type; const char *name; } shaderTypes[] = {
	{ SHADER_TYPE_VERTEX, "Vertex" },
	{ SHADER_TYPE_FRAGMENT, "Fragment" },
	// { SHADER_TYPE_GEOMETRY, "Geometry" },
	{ SHADER_TYPE_VERTEXLOADER, "VertexLoader" },
	{ SHADER_TYPE_PIPELINE, "Pipeline" },
	{ SHADER_TYPE_DEPAL, "Depal" },
};

void ShaderListScreen::CreateViews() {
	using namespace UI;

	I18NCategory *di = GetI18NCategory("Dialog");

	LinearLayout *layout = new LinearLayout(ORIENT_VERTICAL);
	root_ = layout;

	tabs_ = new TabHolder(ORIENT_HORIZONTAL, 40, new LinearLayoutParams(1.0));
	tabs_->SetTag("DevShaderList");
	layout->Add(tabs_);
	layout->Add(new Button(di->T("Back")))->OnClick.Handle<UIScreen>(this, &UIScreen::OnBack);
	for (size_t i = 0; i < ARRAY_SIZE(shaderTypes); i++) {
		ScrollView *scroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(1.0));
		LinearLayout *shaderList = new LinearLayout(ORIENT_VERTICAL, new LayoutParams(FILL_PARENT, WRAP_CONTENT));
		int count = ListShaders(shaderTypes[i].type, shaderList);
		scroll->Add(shaderList);
		tabs_->AddTab(StringFromFormat("%s (%d)", shaderTypes[i].name, count), scroll);
	}
}

UI::EventReturn ShaderListScreen::OnShaderClick(UI::EventParams &e) {
	using namespace UI;
	std::string id = e.v->Tag();
	DebugShaderType type = shaderTypes[tabs_->GetCurrentTab()].type;
	screenManager()->push(new ShaderViewScreen(id, type));
	return EVENT_DONE;
}

void ShaderViewScreen::CreateViews() {
	using namespace UI;

	I18NCategory *di = GetI18NCategory("Dialog");

	LinearLayout *layout = new LinearLayout(ORIENT_VERTICAL);
	root_ = layout;

	layout->Add(new TextView(gpu->DebugGetShaderString(id_, type_, SHADER_STRING_SHORT_DESC)));

	ScrollView *scroll = new ScrollView(ORIENT_VERTICAL, new LinearLayoutParams(1.0));
	scroll->SetTag("DevShaderView");
	layout->Add(scroll);

	LinearLayout *lineLayout = new LinearLayout(ORIENT_VERTICAL, new LinearLayoutParams(FILL_PARENT, WRAP_CONTENT));
	lineLayout->SetSpacing(0.0);
	scroll->Add(lineLayout);

	std::vector<std::string> lines;
	SplitString(gpu->DebugGetShaderString(id_, type_, SHADER_STRING_SOURCE_CODE), '\n', lines);

	for (auto line : lines) {
		lineLayout->Add(new TextView(line, FLAG_DYNAMIC_ASCII, true));
	}

	layout->Add(new Button(di->T("Back")))->OnClick.Handle<UIScreen>(this, &UIScreen::OnBack);
}
