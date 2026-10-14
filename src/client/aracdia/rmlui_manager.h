// Luanti / Aracdia
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "config.h"

#if ENABLE_ARACDIA_RMLUI

#include <RmlUi/Core/ElementDocument.h>
#include <functional>
#include <string>
#include <vector>

struct SEvent;

namespace video {
class IVideoDriver;
}

/// Client-side RmlUi overlay manager (Phase C.0–C.2).
class AracdiaRmlUiManager {
public:
	using ActionCallback = std::function<void(const std::string &screen,
			const std::string &action, const std::string &payload_json)>;

	static AracdiaRmlUiManager &get();

	void init(video::IVideoDriver *driver);
	void shutdown();

	/// Lazy init on first UI open — avoids touching GL during normal gameplay.
	void ensureReady();

	/// True when at least one screen is on the stack.
	bool isActive() const { return !m_stack.empty(); }

	/// While active, game input and camera movement are suppressed.
	bool capturesInput() const { return isActive(); }

	bool hasScreen(const char *id) const;
	bool pushScreen(const char *id);
	void closeTop();
	void closeAll();
	void closeScreen(const std::string &id);
	void toggleDevSpike();

	void setActionCallback(ActionCallback callback);

	void openFromServer(const std::string &screen, const std::string &json_state);
	void updateFromServer(const std::string &screen, const std::string &json_state);
	void closeFromServer(const std::string &screen);

	void dispatchUiAction(const std::string &action, const std::string &payload_json = "{}");

	bool processEvent(const SEvent &event);

	void update(float dtime);
	void render();

	void syncViewport(video::IVideoDriver *driver);
	void onResize(int width, int height);

private:
	struct Screen {
		std::string id;
		Rml::ElementDocument *document = nullptr;
	};

	AracdiaRmlUiManager() = default;

	const char *documentSourceForId(const char *id) const;
	Rml::ElementDocument *findDocument(const std::string &id);
	void applyJsonState(Rml::ElementDocument *document, const std::string &screen_id,
			const std::string &json_state);
	void attachActionListener(Rml::ElementDocument *document);
	void syncMousePosition();

	bool m_initialized = false;
	bool m_init_failed = false;
	int m_width = 0;
	int m_height = 0;
	ActionCallback m_action_callback;
	std::vector<Screen> m_stack;
};

inline bool isAracdiaUiCapturingInput()
{
	return AracdiaRmlUiManager::get().capturesInput();
}

#endif // ENABLE_ARACDIA_RMLUI
