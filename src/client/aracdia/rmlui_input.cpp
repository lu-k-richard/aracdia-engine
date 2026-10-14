// Luanti / Aracdia
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "config.h"

#if ENABLE_ARACDIA_RMLUI

#include "client/aracdia/rmlui_input.h"
#include <IEventReceiver.h>
#include <Keycodes.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>

namespace AracdiaRmlInput {

int keyModifiersFromEvent(const SEvent &event)
{
	int mods = 0;

	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Shift)
			mods |= Rml::Input::KM_SHIFT;
		if (event.KeyInput.Control)
			mods |= Rml::Input::KM_CTRL;
	} else if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		if (event.MouseInput.Shift)
			mods |= Rml::Input::KM_SHIFT;
		if (event.MouseInput.Control)
			mods |= Rml::Input::KM_CTRL;
	}

	return mods;
}

Rml::Input::KeyIdentifier convertKey(EKEY_CODE key)
{
	switch (key) {
	case KEY_SPACE: return Rml::Input::KI_SPACE;
	case KEY_KEY_0: return Rml::Input::KI_0;
	case KEY_KEY_1: return Rml::Input::KI_1;
	case KEY_KEY_2: return Rml::Input::KI_2;
	case KEY_KEY_3: return Rml::Input::KI_3;
	case KEY_KEY_4: return Rml::Input::KI_4;
	case KEY_KEY_5: return Rml::Input::KI_5;
	case KEY_KEY_6: return Rml::Input::KI_6;
	case KEY_KEY_7: return Rml::Input::KI_7;
	case KEY_KEY_8: return Rml::Input::KI_8;
	case KEY_KEY_9: return Rml::Input::KI_9;
	case KEY_KEY_A: return Rml::Input::KI_A;
	case KEY_KEY_B: return Rml::Input::KI_B;
	case KEY_KEY_C: return Rml::Input::KI_C;
	case KEY_KEY_D: return Rml::Input::KI_D;
	case KEY_KEY_E: return Rml::Input::KI_E;
	case KEY_KEY_F: return Rml::Input::KI_F;
	case KEY_KEY_G: return Rml::Input::KI_G;
	case KEY_KEY_H: return Rml::Input::KI_H;
	case KEY_KEY_I: return Rml::Input::KI_I;
	case KEY_KEY_J: return Rml::Input::KI_J;
	case KEY_KEY_K: return Rml::Input::KI_K;
	case KEY_KEY_L: return Rml::Input::KI_L;
	case KEY_KEY_M: return Rml::Input::KI_M;
	case KEY_KEY_N: return Rml::Input::KI_N;
	case KEY_KEY_O: return Rml::Input::KI_O;
	case KEY_KEY_P: return Rml::Input::KI_P;
	case KEY_KEY_Q: return Rml::Input::KI_Q;
	case KEY_KEY_R: return Rml::Input::KI_R;
	case KEY_KEY_S: return Rml::Input::KI_S;
	case KEY_KEY_T: return Rml::Input::KI_T;
	case KEY_KEY_U: return Rml::Input::KI_U;
	case KEY_KEY_V: return Rml::Input::KI_V;
	case KEY_KEY_W: return Rml::Input::KI_W;
	case KEY_KEY_X: return Rml::Input::KI_X;
	case KEY_KEY_Y: return Rml::Input::KI_Y;
	case KEY_KEY_Z: return Rml::Input::KI_Z;
	case KEY_BACK: return Rml::Input::KI_BACK;
	case KEY_TAB: return Rml::Input::KI_TAB;
	case KEY_RETURN: return Rml::Input::KI_RETURN;
	case KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
	case KEY_PRIOR: return Rml::Input::KI_PRIOR;
	case KEY_NEXT: return Rml::Input::KI_NEXT;
	case KEY_END: return Rml::Input::KI_END;
	case KEY_HOME: return Rml::Input::KI_HOME;
	case KEY_LEFT: return Rml::Input::KI_LEFT;
	case KEY_UP: return Rml::Input::KI_UP;
	case KEY_RIGHT: return Rml::Input::KI_RIGHT;
	case KEY_DOWN: return Rml::Input::KI_DOWN;
	case KEY_INSERT: return Rml::Input::KI_INSERT;
	case KEY_DELETE: return Rml::Input::KI_DELETE;
	case KEY_F1: return Rml::Input::KI_F1;
	case KEY_F2: return Rml::Input::KI_F2;
	case KEY_F3: return Rml::Input::KI_F3;
	case KEY_F4: return Rml::Input::KI_F4;
	case KEY_F5: return Rml::Input::KI_F5;
	case KEY_F6: return Rml::Input::KI_F6;
	case KEY_F7: return Rml::Input::KI_F7;
	case KEY_F8: return Rml::Input::KI_F8;
	case KEY_F9: return Rml::Input::KI_F9;
	case KEY_F10: return Rml::Input::KI_F10;
	case KEY_F11: return Rml::Input::KI_F11;
	case KEY_F12: return Rml::Input::KI_F12;
	default: return Rml::Input::KI_UNKNOWN;
	}
}

int convertMouseButton(u32 button)
{
	// Irrlicht forwards SDL button ids (left=1, middle=2, right=3).
	// RmlUi expects left=0, right=1, middle=2 (see RmlUi_Platform_SDL.cpp).
	switch (button) {
	case SDL_BUTTON_LEFT: return 0;
	case SDL_BUTTON_RIGHT: return 1;
	case SDL_BUTTON_MIDDLE: return 2;
	default: return 3;
	}
}

static int mouseButtonIndexFromEvent(const SEvent &event)
{
	switch (event.MouseInput.Event) {
	case EMIE_LMOUSE_PRESSED_DOWN:
	case EMIE_LMOUSE_LEFT_UP:
	case EMIE_LMOUSE_DOUBLE_CLICK:
	case EMIE_LMOUSE_TRIPLE_CLICK:
		return 0;
	case EMIE_RMOUSE_PRESSED_DOWN:
	case EMIE_RMOUSE_LEFT_UP:
		return 1;
	case EMIE_MMOUSE_PRESSED_DOWN:
	case EMIE_MMOUSE_LEFT_UP:
		return 2;
	default:
		return convertMouseButton(event.MouseInput.Button);
	}
}

bool feedEvent(Rml::Context *context, const SEvent &event)
{
	if (!context)
		return false;

	const int mods = keyModifiersFromEvent(event);

	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		const auto &mouse = event.MouseInput;
		context->ProcessMouseMove(mouse.X, mouse.Y, mods);

		switch (mouse.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
		case EMIE_MMOUSE_PRESSED_DOWN:
		case EMIE_RMOUSE_PRESSED_DOWN:
		case EMIE_XMOUSE_PRESSED_DOWN:
			return context->ProcessMouseButtonDown(
					mouseButtonIndexFromEvent(event), mods);
		case EMIE_LMOUSE_LEFT_UP:
		case EMIE_MMOUSE_LEFT_UP:
		case EMIE_RMOUSE_LEFT_UP:
		case EMIE_XMOUSE_LEFT_UP:
			return context->ProcessMouseButtonUp(
					mouseButtonIndexFromEvent(event), mods);
		case EMIE_MOUSE_WHEEL:
			return context->ProcessMouseWheel(mouse.Wheel, mods);
		case EMIE_MOUSE_MOVED:
			return context->ProcessMouseMove(mouse.X, mouse.Y, mods);
		default:
			break;
		}
		return false;
	}

	if (event.EventType == EET_KEY_INPUT_EVENT) {
		const auto &key = event.KeyInput;
		const auto identifier = convertKey(key.Key);

		if (key.PressedDown) {
			bool consumed = context->ProcessKeyDown(identifier, mods);
			if (key.Char != 0 && key.Char < 128)
				consumed |= context->ProcessTextInput(static_cast<Rml::Character>(key.Char));
			return consumed;
		}

		return context->ProcessKeyUp(identifier, mods);
	}

	return false;
}

} // namespace AracdiaRmlInput

#endif // ENABLE_ARACDIA_RMLUI
