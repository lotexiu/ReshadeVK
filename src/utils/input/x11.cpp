#include "x11.hpp"

#if RESHADEVK_HAS_X11

#include <cstdio>
#include <limits>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <imgui.h>

namespace reshadevk::input {

// ── X11 helpers ───────────────────────────────────────────────────────────────

static bool x11_key_down(Display* display, const char keymap[32], KeySym keysym) {
	const KeyCode kc = XKeysymToKeycode(display, keysym);
	if (kc == 0)
		return false;
	return (keymap[kc >> 3] & (1 << (kc & 7))) != 0;
}

static Window x11_get_active_window(Display* display) {
	const Atom activeAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
	if (activeAtom == None)
		return 0;

	Atom           actualType   = None;
	int            actualFormat = 0;
	unsigned long  itemCount    = 0;
	unsigned long  bytesAfter   = 0;
	unsigned char* data         = nullptr;

	const int rc = XGetWindowProperty(
			display, DefaultRootWindow(display), activeAtom,
			0, 1, False, AnyPropertyType,
			&actualType, &actualFormat, &itemCount, &bytesAfter, &data);

	if (rc != Success || !data || itemCount == 0) {
		if (data)
			XFree(data);
		return 0;
	}
	Window active = *reinterpret_cast<Window*>(data);
	XFree(data);
	return active;
}

static pid_t x11_get_window_pid(Display* display, Window window) {
	const Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
	if (pidAtom == None || window == 0)
		return -1;

	Atom           actualType   = None;
	int            actualFormat = 0;
	unsigned long  itemCount    = 0;
	unsigned long  bytesAfter   = 0;
	unsigned char* data         = nullptr;

	const int rc = XGetWindowProperty(
			display, window, pidAtom,
			0, 1, False, XA_CARDINAL,
			&actualType, &actualFormat, &itemCount, &bytesAfter, &data);

	if (rc != Success || !data || itemCount == 0) {
		if (data)
			XFree(data);
		return -1;
	}
	pid_t pid = static_cast<pid_t>(*reinterpret_cast<unsigned long*>(data));
	XFree(data);
	return pid;
}

// ── update_x11 ────────────────────────────────────────────────────────────────
void update_x11(SwapchainData& swData) {
	auto* display = static_cast<Display*>(swData.nativeDisplay);
	if (!display) {
		display = XOpenDisplay(nullptr);
		if (!display)
			return;
		swData.nativeDisplay         = display;
		swData.ownsDisplayConnection = true;
	}

	const Window window    = x11_get_active_window(display);
	swData.nativeWindow    = static_cast<unsigned long>(window);

	ImGuiIO& io            = ImGui::GetIO();
	const pid_t myPid      = getpid();
	const pid_t activePid  = x11_get_window_pid(display, window);
	const bool  hasFocus   = (window != 0 && activePid == myPid);
	swData.inputFocused    = hasFocus;
	io.AddFocusEvent(hasFocus);

	char keymap[32] = {};
	XQueryKeymap(display, keymap);

	const bool toggleDown = x11_key_down(display, keymap, XK_F8);
	if (toggleDown && !swData.prevToggleKeyDown && hasFocus) {
		swData.interactiveEnabled = !swData.interactiveEnabled;
		std::printf("[reshadeVK] input %s (F8)\n", swData.interactiveEnabled ? "ativado" : "desativado");
	}
	swData.prevToggleKeyDown = toggleDown;

	const bool canCapture = hasFocus && swData.interactiveEnabled;

	Window        root   = 0;
	Window        child  = 0;
	int           rootX  = 0, rootY = 0;
	int           winX   = 0, winY  = 0;
	unsigned int  mask   = 0;
	Bool pointerInside   = False;
	if (window != 0)
		pointerInside = XQueryPointer(display, window, &root, &child, &rootX, &rootY, &winX, &winY, &mask);

	if (canCapture && pointerInside)
		io.AddMousePosEvent(static_cast<float>(winX), static_cast<float>(winY));
	else
		io.AddMousePosEvent(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	const bool nextMouseDown[5] = {
		canCapture && ((mask & Button1Mask) != 0),
		canCapture && ((mask & Button3Mask) != 0),
		canCapture && ((mask & Button2Mask) != 0),
		false,
		false,
	};
	for (int i = 0; i < 5; i++) {
		if (nextMouseDown[i] != swData.prevMouseDown[i]) {
			io.AddMouseButtonEvent(i, nextMouseDown[i]);
			swData.prevMouseDown[i] = nextMouseDown[i];
		}
	}

	io.AddKeyEvent(ImGuiKey_Tab,        canCapture && x11_key_down(display, keymap, XK_Tab));
	io.AddKeyEvent(ImGuiKey_LeftArrow,  canCapture && x11_key_down(display, keymap, XK_Left));
	io.AddKeyEvent(ImGuiKey_RightArrow, canCapture && x11_key_down(display, keymap, XK_Right));
	io.AddKeyEvent(ImGuiKey_UpArrow,    canCapture && x11_key_down(display, keymap, XK_Up));
	io.AddKeyEvent(ImGuiKey_DownArrow,  canCapture && x11_key_down(display, keymap, XK_Down));
	io.AddKeyEvent(ImGuiKey_Enter,      canCapture && (x11_key_down(display, keymap, XK_Return)
	                                               || x11_key_down(display, keymap, XK_KP_Enter)));
	io.AddKeyEvent(ImGuiKey_Escape,     canCapture && x11_key_down(display, keymap, XK_Escape));
	io.AddKeyEvent(ImGuiKey_Backspace,  canCapture && x11_key_down(display, keymap, XK_BackSpace));
	io.AddKeyEvent(ImGuiKey_Space,      canCapture && x11_key_down(display, keymap, XK_space));
}

} // namespace reshadevk::input

#endif // RESHADEVK_HAS_X11
