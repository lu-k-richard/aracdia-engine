# Aracdia engine changes (LGPL)

Modifications to the Luanti fork for the Aracdia commercial game.

## Native UI — RmlUi (Phase C.0–C.1)

- **CMake**: `ENABLE_ARACDIA_RMLUI` (default ON) fetches [RmlUi 6.0](https://github.com/mikke89/RmlUi) via FetchContent.
- **Client overlay**: `src/client/aracdia/rmlui_manager.cpp` — GL3 renderer, screen stack.
- **Render backends**: GL3 when the video driver is OpenGL 3+; automatic **GL2 fallback** on macOS legacy OpenGL 2.1 (fixes `GL3 renderer construction failed` spam).
- **Render hook**: `src/client/render/plain.cpp` — draws after Irrlicht GUI.
- **Dev shortcuts**: Ctrl+Shift+U toggle spike, Ctrl+Shift+O push overlay, Esc close top screen. F8/F9 also work when function keys are enabled (Fn+F8 on Mac laptops).
- **Protocol 54**: `TOCLIENT_ARACDIA_UI_{OPEN,UPDATE,CLOSE}`, `TOSERVER_ARACDIA_UI_ACTION`
- **Lua (server)**: `core.open_aracdia_ui`, `core.close_aracdia_ui`, `core.register_on_aracdia_ui_action`
- **Toggle off**: `cmake -DENABLE_ARACDIA_RMLUI=OFF ..`

## Pause menu (v0.2.0)

- `TOSERVER_PAUSE_MENU` / `TOCLIENT_SHOW_NATIVE_PAUSE_MENU` (protocol 53)
- `core.register_on_pause_menu` Lua hook

See monorepo `docs/ARACDIA_NATIVE_UI.md` for the full roadmap.
