// Luanti / Aracdia
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "config.h"

#if ENABLE_ARACDIA_RMLUI

#include "client/aracdia/rmlui_manager.h"
#include "client/aracdia/rmlui_system_interface.h"
#include "client/aracdia/rmlui_input.h"
#include "client/renderingengine.h"
#include "client/renderingengine.h"
#include "filesys.h"
#include "log.h"
#include "porting.h"
#include "util/numeric.h"
#include "convert_json.h"
#include <json/json.h>

#include <IEventReceiver.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi_Renderer_GL2.h>
#include <RmlUi_Renderer_GL3.h>

#include <IVideoDriver.h>
#include <IrrlichtDevice.h>
#include <memory>
#include <memory>

namespace {

enum class RmlBackend {
	None,
	GL2,
	GL3,
};

AracdiaRmlSystemInterface g_system_interface;
RmlBackend g_backend = RmlBackend::None;
std::unique_ptr<RenderInterface_GL2> g_renderer_gl2;
std::unique_ptr<RenderInterface_GL3> g_renderer_gl3;
Rml::Context *g_context = nullptr;

struct RendererAccess {
	static void setViewport(int width, int height)
	{
		if (g_renderer_gl3)
			g_renderer_gl3->SetViewport(width, height);
		else if (g_renderer_gl2)
			g_renderer_gl2->SetViewport(width, height);
	}

	static void beginFrame()
	{
		if (g_renderer_gl3)
			g_renderer_gl3->BeginFrame();
		else if (g_renderer_gl2)
			g_renderer_gl2->BeginFrame();
	}

	static void endFrame()
	{
		if (g_renderer_gl3)
			g_renderer_gl3->EndFrame();
		else if (g_renderer_gl2)
			g_renderer_gl2->EndFrame();
	}

	static bool ready() { return g_backend != RmlBackend::None; }
};

bool prefer_gl3_backend(video::IVideoDriver *driver)
{
	return driver->getDriverType() == video::EDT_OPENGL3;
}

void reset_renderer_state()
{
	g_renderer_gl3.reset();
	g_renderer_gl2.reset();
	g_backend = RmlBackend::None;
}

const char *const kSpikeScreenId = "spike";
const char *const kOverlayScreenId = "overlay";
const char *const kPanelScreenId = "panel";
const char *const kMenuScreenId = "menu";

Rml::String actionForElement(Rml::Element *element)
{
	if (!element)
		return {};

	const Rml::String &id = element->GetId();
	if (id == "btn_resume")
		return "resume";
	if (id == "btn_build")
		return "build_toggle";
	if (id == "btn_quit")
		return "quit_game";

	return element->GetAttribute<Rml::String>("data-action", "");
}

Rml::Element *findActionElement(Rml::Event &event)
{
	auto matches = [](Rml::Element *element) {
		return element && !actionForElement(element).empty();
	};

	Rml::Element *element = event.GetTargetElement();
	if (!matches(element))
		element = event.GetCurrentElement();

	while (element) {
		if (matches(element))
			return element;
		element = element->GetParentNode();
	}

	if (!g_context)
		return nullptr;

	const float mx = event.GetParameter("mouse_x", -1.f);
	const float my = event.GetParameter("mouse_y", -1.f);
	if (mx < 0.f || my < 0.f)
		return nullptr;

	element = g_context->GetElementAtPoint(Rml::Vector2f(mx, my));
	while (element) {
		if (matches(element))
			return element;
		element = element->GetParentNode();
	}
	return nullptr;
}

class UiActionListener : public Rml::EventListener {
public:
	void ProcessEvent(Rml::Event &event) override
	{
		// Mousedown is more reliable than Click when focus/hover state is fragile.
		if (event.GetId() != Rml::EventId::Mousedown)
			return;
		if (event.GetParameter("button", -1) != 0)
			return;

		Rml::Element *element = findActionElement(event);
		if (!element)
			return;

		const Rml::String action = actionForElement(element);
		if (action.empty())
			return;

		AracdiaRmlUiManager::get().dispatchUiAction(action, "{}");
	}
};

UiActionListener g_ui_action_listener;

float uiDensityRatio()
{
	// Match Irrlicht GUI skin scaling (see clientlauncher.cpp).
	return std::max(0.5f, rangelim(g_settings->getFloat("gui_scaling"), 0.5f, 20.f) *
			RenderingEngine::getDisplayDensity());
}

void syncUiDensity()
{
	if (!g_context)
		return;
	const float ratio = uiDensityRatio();
	g_context->SetDensityIndependentPixelRatio(ratio);
}

void setElementText(Rml::ElementDocument *document, const char *id, const std::string &text)
{
	if (!document || !id)
		return;
	if (Rml::Element *element = document->GetElementById(id))
		element->SetInnerRML(text);
}

void applyPanelState(Rml::ElementDocument *document, const Json::Value &state)
{
	if (!document || !state.isObject())
		return;

	if (state.isMember("title"))
		setElementText(document, "title", state["title"].asString());
	if (state.isMember("message"))
		setElementText(document, "message", state["message"].asString());
	if (state.isMember("subtitle"))
		setElementText(document, "message", state["subtitle"].asString());

	if (Rml::Element *button = document->GetElementById("btn_primary")) {
		if (state.isMember("button_label"))
			button->SetInnerRML(state["button_label"].asString());
		if (state.isMember("button_action"))
			button->SetAttribute("data-action", state["button_action"].asString());
	}
}

void applyMenuState(Rml::ElementDocument *document, const Json::Value &state)
{
	if (!document || !state.isObject())
		return;

	if (state.isMember("title"))
		setElementText(document, "title", state["title"].asString());
	if (state.isMember("subtitle"))
		setElementText(document, "subtitle", state["subtitle"].asString());

	if (state.isMember("build_toggle_label"))
		setElementText(document, "btn_build", state["build_toggle_label"].asString());

	const bool build_mode = state.get("build_mode", false).asBool();
	if (Rml::Element *button = document->GetElementById("btn_build"))
		button->SetClass("active", build_mode);
}

const char *const kMenuDocument = R"RML(
<rml>
<head>
<style>
body {
	font-family: Arimo;
	width: 100%;
	height: 100%;
	margin: 0;
	display: flex;
	align-items: center;
	justify-content: center;
}
#shell {
	position: relative;
	width: 360dp;
	padding: 28dp 24dp 22dp 24dp;
	background: rgba(10, 10, 15, 0.80);
	border: 1dp rgba(99, 102, 241, 0.40);
	border-radius: 14dp;
	color: #E0E7FF;
	text-align: center;
	display: block;
}
#title {
	display: block;
	font-size: 24dp;
	font-weight: bold;
	margin: 0 0 6dp 0;
	color: #E0E7FF;
}
#subtitle {
	display: block;
	font-size: 12dp;
	margin: 0 0 18dp 0;
	color: #A8A8B8;
}
.rule {
	display: block;
	height: 1dp;
	background: #2A2A38;
	margin: 14dp 0;
}
button {
	display: block;
	width: 100%;
	box-sizing: border-box;
	padding: 10dp 16dp;
	margin: 0 0 10dp 0;
	border-radius: 8dp;
	cursor: pointer;
	font-size: 14dp;
	text-align: center;
	min-height: 40dp;
	focus: auto;
	tab-index: auto;
}
button.primary {
	background: #6366F1;
	color: #F5F5FA;
}
button.primary:hover {
	background: #818CF8;
}
button.build {
	background: #1A1A26;
	color: #F5F5FA;
}
button.build:hover {
	background: #22222E;
}
button.build.active {
	background: #14532D;
	color: #6EE7B7;
}
button.build.active:hover {
	background: #166534;
}
button.quit {
	background: transparent;
	color: #F87171;
	margin-top: 4dp;
	margin-bottom: 0;
}
button.quit:hover {
	color: #FCA5A5;
}
</style>
</head>
<body>
	<div id="shell">
		<h1 id="title">ARACDIA</h1>
		<p id="subtitle">Menu du jeu</p>
		<div class="rule"></div>
		<button id="btn_resume" class="primary" data-action="resume">Reprendre</button>
		<button id="btn_build" class="build" data-action="build_toggle">Mode construction</button>
		<div class="rule"></div>
		<button id="btn_quit" class="quit" data-action="quit_game">Quitter le jeu</button>
	</div>
</body>
</rml>
)RML";

const char *const kPanelDocument = R"RML(
<rml>
<head>
<style>
body {
	font-family: Arimo;
	width: 100%;
	height: 100%;
	margin: 0;
	display: flex;
	align-items: center;
	justify-content: center;
}
#panel {
	position: relative;
	width: 420dp;
	padding: 24dp;
	background: rgba(14, 14, 20, 0.92);
	border: 1dp rgba(99, 102, 241, 0.45);
	border-radius: 12dp;
	color: #E0E7FF;
	text-align: center;
	display: block;
}
h1 {
	display: block;
	font-size: 22dp;
	margin: 0 0 8dp 0;
}
p {
	display: block;
	color: #A8A8B8;
	font-size: 13dp;
	margin: 0 0 12dp 0;
}
button {
	display: block;
	width: 100%;
	box-sizing: border-box;
	margin-top: 8dp;
	padding: 8dp 16dp;
	background: #6366F1;
	color: #F5F5FA;
	border-radius: 8dp;
	cursor: pointer;
	min-height: 36dp;
	focus: auto;
	tab-index: auto;
}
button:hover {
	background: #818CF8;
}
</style>
</head>
<body>
	<div id="panel">
		<h1 id="title">Aracdia</h1>
		<p id="message"></p>
		<button id="btn_primary" data-action="close">Fermer</button>
	</div>
</body>
</rml>
)RML";

const char *const kSpikeDocument = R"RML(
<rml>
<head>
<style>
body {
	font-family: Arimo;
	width: 100%;
	height: 100%;
}
#panel {
	position: absolute;
	left: 50%;
	top: 50%;
	transform: translate(-50%, -50%);
	width: 420dp;
	padding: 24dp;
	background: rgba(14, 14, 20, 0.92);
	border: 1dp rgba(99, 102, 241, 0.45);
	border-radius: 12dp;
	color: #E0E7FF;
	text-align: center;
}
h1 {
	font-size: 22dp;
	margin-bottom: 8dp;
}
p {
	color: #A8A8B8;
	font-size: 13dp;
	margin-bottom: 6dp;
}
small {
	color: #6366F1;
	font-size: 11dp;
}
button {
	margin-top: 12dp;
	padding: 8dp 16dp;
	background: #6366F1;
	color: #F5F5FA;
	border-radius: 8dp;
	cursor: pointer;
}
button:hover {
	background: #818CF8;
}
</style>
</head>
<body>
	<div id="panel">
		<h1>Aracdia UI</h1>
		<p>RmlUi natif — Phase C.1</p>
		<small>Ctrl+Shift+U ouvre · Ctrl+Shift+O sous-écran · Échap ferme</small>
		<p>F8 / F9 si touches fonction activées (Fn+F8 sur Mac)</p>
	</div>
</body>
</rml>
)RML";

const char *const kOverlayDocument = R"RML(
<rml>
<head>
<style>
body {
	font-family: Arimo;
	width: 100%;
	height: 100%;
}
#panel {
	position: absolute;
	left: 50%;
	top: 50%;
	transform: translate(-50%, -50%);
	width: 320dp;
	padding: 20dp;
	background: rgba(20, 20, 28, 0.96);
	border: 1dp rgba(129, 140, 248, 0.55);
	border-radius: 10dp;
	color: #E0E7FF;
	text-align: center;
}
h2 {
	font-size: 18dp;
	margin-bottom: 8dp;
}
p {
	color: #A8A8B8;
	font-size: 12dp;
}
</style>
</head>
<body>
	<div id="panel">
		<h2>Sous-écran</h2>
		<p>Deuxième niveau de la pile UI. Échap ferme celui-ci en premier.</p>
	</div>
</body>
</rml>
)RML";

bool load_default_font()
{
	std::string font_path = g_settings->get("font_path");
	if (font_path.empty())
		font_path = porting::getDataPath("fonts" DIR_DELIM "Arimo-Regular.ttf");
	else if (!fs::IsPathAbsolute(font_path))
		font_path = porting::path_share + DIR_DELIM + font_path;

	if (!fs::PathExists(font_path)) {
		warningstream << "Aracdia RmlUi: font not found at " << font_path << std::endl;
		return false;
	}

	if (!Rml::LoadFontFace(font_path)) {
		warningstream << "Aracdia RmlUi: failed to load font " << font_path << std::endl;
		return false;
	}
	return true;
}

} // namespace

AracdiaRmlUiManager &AracdiaRmlUiManager::get()
{
	static AracdiaRmlUiManager instance;
	return instance;
}

const char *AracdiaRmlUiManager::documentSourceForId(const char *id) const
{
	if (!id)
		return nullptr;
	if (strcmp(id, kSpikeScreenId) == 0)
		return kSpikeDocument;
	if (strcmp(id, kOverlayScreenId) == 0)
		return kOverlayDocument;
	if (strcmp(id, kPanelScreenId) == 0)
		return kPanelDocument;
	if (strcmp(id, kMenuScreenId) == 0)
		return kMenuDocument;
	return nullptr;
}

void AracdiaRmlUiManager::setActionCallback(ActionCallback callback)
{
	m_action_callback = std::move(callback);
}

void AracdiaRmlUiManager::attachActionListener(Rml::ElementDocument *document)
{
	if (!document)
		return;

	document->AddEventListener(Rml::EventId::Mousedown, &g_ui_action_listener, false);

	static const char *const kActionButtonIds[] = {
		"btn_resume", "btn_build", "btn_quit", "btn_primary",
	};
	for (const char *id : kActionButtonIds) {
		if (Rml::Element *button = document->GetElementById(id))
			button->AddEventListener(Rml::EventId::Mousedown, &g_ui_action_listener, false);
	}
}

void AracdiaRmlUiManager::applyJsonState(Rml::ElementDocument *document,
		const std::string &screen_id, const std::string &json_state)
{
	if (!document || json_state.empty())
		return;

	Json::Value state;
	Json::CharReaderBuilder builder;
	std::string errmsg;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if (!reader->parse(json_state.data(), json_state.data() + json_state.size(),
			&state, &errmsg)) {
		warningstream << "Aracdia RmlUi: failed to parse UI state JSON: "
		              << errmsg << std::endl;
		return;
	}

	if (screen_id == kMenuScreenId)
		applyMenuState(document, state);
	else
		applyPanelState(document, state);
}

Rml::ElementDocument *AracdiaRmlUiManager::findDocument(const std::string &id)
{
	for (Screen &screen : m_stack) {
		if (screen.id == id)
			return screen.document;
	}
	return nullptr;
}

void AracdiaRmlUiManager::openFromServer(const std::string &screen,
		const std::string &json_state)
{
	if (screen.empty())
		return;

	if (hasScreen(screen.c_str())) {
		updateFromServer(screen, json_state);
		return;
	}

	if (!pushScreen(screen.c_str()))
		return;

	syncMousePosition();

	if (Rml::ElementDocument *document = findDocument(screen))
		applyJsonState(document, screen, json_state);
}

void AracdiaRmlUiManager::updateFromServer(const std::string &screen,
		const std::string &json_state)
{
	if (Rml::ElementDocument *document = findDocument(screen))
		applyJsonState(document, screen, json_state);
}

void AracdiaRmlUiManager::closeFromServer(const std::string &screen)
{
	if (screen.empty()) {
		closeAll();
		return;
	}

	for (auto it = m_stack.begin(); it != m_stack.end();) {
		if (it->id == screen) {
			if (it->document)
				it->document->Close();
			it = m_stack.erase(it);
		} else {
			++it;
		}
	}
}

void AracdiaRmlUiManager::closeScreen(const std::string &id)
{
	closeFromServer(id);
}

void AracdiaRmlUiManager::dispatchUiAction(const std::string &action,
		const std::string &payload_json)
{
	if (m_stack.empty() || !m_action_callback)
		return;

	const std::string &screen = m_stack.back().id;
	m_action_callback(screen, action, payload_json);
}

void AracdiaRmlUiManager::ensureReady()
{
	if (m_initialized || m_init_failed)
		return;
	auto *driver = RenderingEngine::get_video_driver();
	if (driver)
		init(driver);
}

void AracdiaRmlUiManager::init(video::IVideoDriver *driver)
{
	if (m_initialized || m_init_failed || !driver)
		return;

	m_width = driver->getScreenSize().Width;
	m_height = driver->getScreenSize().Height;

	const bool try_gl3 = prefer_gl3_backend(driver);
	if (try_gl3) {
		Rml::String gl_message;
		if (!RmlGL3::Initialize(&gl_message)) {
			warningstream << "Aracdia RmlUi: OpenGL loader failed: " << gl_message
			              << " — falling back to GL2 renderer" << std::endl;
		} else {
			infostream << "Aracdia RmlUi: " << gl_message << std::endl;
			g_renderer_gl3 = std::make_unique<RenderInterface_GL3>();
			if (*g_renderer_gl3) {
				g_backend = RmlBackend::GL3;
				Rml::SetRenderInterface(g_renderer_gl3.get());
			} else {
				warningstream << "Aracdia RmlUi: GL3 renderer construction failed"
				              << " — falling back to GL2 renderer" << std::endl;
				g_renderer_gl3.reset();
				RmlGL3::Shutdown();
			}
		}
	}

	if (!RendererAccess::ready()) {
		g_renderer_gl2 = std::make_unique<RenderInterface_GL2>();
		g_backend = RmlBackend::GL2;
		Rml::SetRenderInterface(g_renderer_gl2.get());
		infostream << "Aracdia RmlUi: using GL2 renderer ("
		           << driver->getName() << ")" << std::endl;
	}

	Rml::SetSystemInterface(&g_system_interface);

	if (!Rml::Initialise()) {
		errorstream << "Aracdia RmlUi: Rml::Initialise failed" << std::endl;
		reset_renderer_state();
		if (try_gl3)
			RmlGL3::Shutdown();
		m_init_failed = true;
		return;
	}

	if (!load_default_font()) {
		warningstream << "Aracdia RmlUi: continuing without default font" << std::endl;
	}

	g_context = Rml::CreateContext("aracdia_main",
			Rml::Vector2i(m_width, m_height));
	if (!g_context) {
		errorstream << "Aracdia RmlUi: failed to create context" << std::endl;
		Rml::Shutdown();
		reset_renderer_state();
		if (try_gl3)
			RmlGL3::Shutdown();
		m_init_failed = true;
		return;
	}

	syncUiDensity();

	m_initialized = true;
	infostream << "Aracdia RmlUi: manager ready (" << m_width << "x" << m_height
	           << ", dp=" << uiDensityRatio() << ")" << std::endl;
}

void AracdiaRmlUiManager::shutdown()
{
	if (!m_initialized)
		return;

	closeAll();

	if (g_context) {
		Rml::RemoveContext("aracdia_main");
		g_context = nullptr;
	}

	Rml::Shutdown();
	const bool had_gl3 = g_backend == RmlBackend::GL3;
	reset_renderer_state();
	if (had_gl3)
		RmlGL3::Shutdown();

	m_initialized = false;
	m_init_failed = false;
}

void AracdiaRmlUiManager::onResize(int width, int height)
{
	m_width = width;
	m_height = height;
	if (g_context) {
		g_context->SetDimensions(Rml::Vector2i(width, height));
		syncUiDensity();
	}
	if (RendererAccess::ready())
		RendererAccess::setViewport(width, height);
}

void AracdiaRmlUiManager::syncViewport(video::IVideoDriver *driver)
{
	if (!driver)
		return;

	const auto size = driver->getScreenSize();
	if (size.Width != (u32)m_width || size.Height != (u32)m_height)
		onResize(size.Width, size.Height);
	else if (g_context)
		syncUiDensity();
}

void AracdiaRmlUiManager::syncMousePosition()
{
	if (!g_context)
		return;
	IrrlichtDevice *device = RenderingEngine::get_raw_device();
	if (!device)
		return;
	gui::ICursorControl *cursor = device->getCursorControl();
	if (!cursor)
		return;
	cursor->setRelativeMode(false);
	cursor->setVisible(true);
	const core::position2d<s32> pos = cursor->getPosition();
	g_context->ProcessMouseMove(pos.X, pos.Y, 0);
}

bool AracdiaRmlUiManager::hasScreen(const char *id) const
{
	if (!id)
		return false;
	for (const Screen &screen : m_stack) {
		if (screen.id == id)
			return true;
	}
	return false;
}

bool AracdiaRmlUiManager::pushScreen(const char *id)
{
	ensureReady();
	if (!m_initialized || !g_context || !id)
		return false;

	const char *source = documentSourceForId(id);
	if (!source)
		return false;

	std::string path = std::string("aracdia/") + id + ".rml";
	Rml::ElementDocument *document = g_context->LoadDocumentFromMemory(source, path.c_str());
	if (!document) {
		errorstream << "Aracdia RmlUi: failed to load screen \"" << id << "\"" << std::endl;
		return false;
	}

	document->Show();
	attachActionListener(document);

	m_stack.push_back({id, document});
	infostream << "Aracdia RmlUi: pushed screen \"" << id << "\" (depth "
	           << m_stack.size() << ")" << std::endl;
	return true;
}

void AracdiaRmlUiManager::closeTop()
{
	if (m_stack.empty())
		return;

	Screen top = m_stack.back();
	m_stack.pop_back();

	if (top.document)
		top.document->Close();

	infostream << "Aracdia RmlUi: closed screen \"" << top.id << "\" (depth "
	           << m_stack.size() << ")" << std::endl;
}

void AracdiaRmlUiManager::closeAll()
{
	while (!m_stack.empty())
		closeTop();
}

void AracdiaRmlUiManager::toggleDevSpike()
{
	ensureReady();
	if (hasScreen(kSpikeScreenId)) {
		closeAll();
		return;
	}
	if (!pushScreen(kSpikeScreenId))
		warningstream << "Aracdia RmlUi: dev panel unavailable (init or load failed)"
		              << std::endl;
}

bool AracdiaRmlUiManager::processEvent(const SEvent &event)
{
	if (!m_initialized || !g_context || m_stack.empty())
		return false;

	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			closeTop();
			return true;
		}
	}

	return AracdiaRmlInput::feedEvent(g_context, event);
}

void AracdiaRmlUiManager::update(float dtime)
{
	(void)dtime;
	if (!m_initialized || !g_context || m_stack.empty())
		return;

	g_context->Update();
}

void AracdiaRmlUiManager::render()
{
	if (!m_initialized || !g_context || !RendererAccess::ready() || m_stack.empty())
		return;

	RendererAccess::setViewport(m_width, m_height);
	RendererAccess::beginFrame();
	g_context->Render();
	RendererAccess::endFrame();
}

#endif // ENABLE_ARACDIA_RMLUI
