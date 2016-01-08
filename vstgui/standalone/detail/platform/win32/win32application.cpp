#include "win32preference.h"
#include "win32window.h"
#include "../../application.h"
#include "../../window.h"
#include "../../../iappdelegate.h"
#include "../../../iapplication.h"
#include "../../../../lib/platform/win32/win32support.h"

#include <Windows.h>
#include <ShellScalingAPI.h>

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

//------------------------------------------------------------------------
namespace VSTGUI {
namespace Standalone {
namespace Platform {
namespace Win32 {

using namespace VSTGUI::Standalone;
using VSTGUI::Standalone::Detail::IApplicationPlatformAccess;
using VSTGUI::Standalone::Detail::CommandWithKey;
using VSTGUI::Standalone::Detail::IPlatformWindowAccess;
using CommandWithKeyList =
    VSTGUI::Standalone::Detail::IApplicationPlatformAccess::CommandWithKeyList;
using VSTGUI::Standalone::Detail::PlatformCallbacks;

//------------------------------------------------------------------------
static IApplicationPlatformAccess* getApplicationPlatformAccess ()
{
	return IApplication::instance ().dynamicCast<IApplicationPlatformAccess> ();
}

//------------------------------------------------------------------------
class Application
{
public:
	void init ();
	void run ();

	void quit ();
	void onCommandUpdate ();
	AlertResult showAlert (const AlertBoxConfig& config) { return AlertResult::error; }
	void showAlertForWindow (const AlertBoxForWindowConfig& config) {}
private:
	bool running {true};
	Win32Preference prefs;
	HACCEL keyboardAccelerators {nullptr};
};

//------------------------------------------------------------------------
void Application::init ()
{
	useD2DHardwareRenderer (true);
	SetProcessDpiAwareness (PROCESS_PER_MONITOR_DPI_AWARE);

	auto app = getApplicationPlatformAccess ();
	vstgui_assert (app);

	app->init (prefs);

	PlatformCallbacks callbacks;
	callbacks.quit = [this] () { quit (); };
	callbacks.onCommandUpdate = [this] () { onCommandUpdate (); };
	callbacks.showAlert = [this] (const AlertBoxConfig& config) { return showAlert (config); };
	callbacks.showAlertForWindow = [this] (const AlertBoxForWindowConfig& config) {
		showAlertForWindow (config);
	};

	app->setPlatformCallbacks (std::move (callbacks));
	IApplication::instance ().getDelegate ().finishLaunching ();
}

//------------------------------------------------------------------------
void Application::onCommandUpdate ()
{
	if (keyboardAccelerators)
	{
		DestroyAcceleratorTable (keyboardAccelerators);
		keyboardAccelerators = nullptr;
	}
	auto& windows = IApplication::instance ().getWindows ();
	for (auto& w : windows)
	{
		auto platformWindow = w->dynamicCast<Detail::IPlatformWindowAccess> ();
		vstgui_assert (platformWindow);
		auto winWindow = platformWindow->getPlatformWindow ()->dynamicCast<IWin32Window> ();
		vstgui_assert (winWindow);
		winWindow->updateCommands ();
	}
	auto app = getApplicationPlatformAccess ();
	std::vector<ACCEL> accels;
	WORD cmd = 0;
	for (auto& grp : app->getCommandList ())
	{
		for (auto& e : grp.second)
		{
			if (e.defaultKey)
			{
				BYTE virt = FVIRTKEY | FCONTROL;
				auto upperKey = toupper (e.defaultKey);
				if (upperKey == e.defaultKey)
					virt |= FSHIFT;
				accels.push_back ({virt, static_cast<WORD> (upperKey), cmd});
			}
			++cmd;
		}
	}
	if (!accels.empty ())
		keyboardAccelerators =
		    CreateAcceleratorTable (accels.data (), static_cast<int> (accels.size ()));
}

//------------------------------------------------------------------------
void Application::quit ()
{
	auto windows = IApplication::instance ().getWindows (); // Yes, copy the window list
	for (auto& w : windows)
	{
		auto platformWindow = w->dynamicCast<Detail::IPlatformWindowAccess> ();
		vstgui_assert (platformWindow);
		auto winWindow = platformWindow->getPlatformWindow ()->dynamicCast<IWin32Window> ();
		vstgui_assert (winWindow);
		winWindow->onQuit ();
	}
	IApplication::instance ().getDelegate ().onQuit ();
	PostQuitMessage (0);
}

//------------------------------------------------------------------------
void Application::run ()
{
	MSG msg;
	while (GetMessage (&msg, NULL, 0, 0))
	{
		if (TranslateAccelerator (msg.hwnd, keyboardAccelerators, &msg))
			continue;

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
}

//------------------------------------------------------------------------
} // Win32
} // Platform
} // Standalone
} // VSTGUI

void* hInstance = nullptr; // for VSTGUI

//------------------------------------------------------------------------
int APIENTRY wWinMain (_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance,
                       _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	HRESULT hr = CoInitialize (NULL);
	if (FAILED (hr))
		return FALSE;

	hInstance = instance;

	VSTGUI::Standalone::Platform::Win32::Application app;
	app.init ();
	app.run ();

	CoUninitialize ();
	return 0;
}
