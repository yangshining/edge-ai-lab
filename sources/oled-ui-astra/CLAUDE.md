# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository context

This project lives at `sources/oled-ui-astra/` inside the monorepo rooted at `D:/code/proj/edge-ai-lab`. Git commands must be run from the repo root (`D:/code/proj/edge-ai-lab`). Commit messages use the prefix `[oled-ui-astra] type: description`.

## Build

All toolchain binaries come from STM32CubeIDE 2.0 plugins — no separate install needed. VS Code `.vscode/settings.json` already sets `PATH` to point at them.

**VS Code:** `F7` (CMake: build)

**Command line:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug   # or Release
ninja -C build
```

Build outputs: `build/astra.elf` (debug), `build/astra.hex`, `build/astra.bin`.

**Flash:** `Ctrl+Shift+P` → *Tasks: Run Task* → **Flash (ST-Link)**, or **Build & Flash**.

**Debug:** `F5` via Cortex-Debug + OpenOCD (requires CMSIS-DAP probe). Config in `.vscode/launch.json`.

There are no automated tests — this is bare-metal firmware.

## Architecture

### Layer stack (bottom → top)

```
STM32 HAL (Drivers/)
    ↓
HALDreamCore  (hal/hal_dreamCore/)   ← concrete HAL for this board
    ↓
HAL           (hal/hal.h)            ← abstract interface, singleton
    ↓
Astra UI      (astra/ui/)            ← renderer, layout engine
    ↓
astra_rocket  (astra/astra_rocket.cpp) ← wires everything together
    ↓
main.c        → astraCoreInit() → astraCoreStart()
```

### HAL singleton

`HAL` is a static singleton injected at startup:
```cpp
HAL::inject(new HALDreamCore);   // astra_rocket.cpp
```
All drawing and I/O calls go through `HAL::*` static methods (`HAL::drawBox`, `HAL::getKey`, etc). To port to another board, subclass `HAL` and inject the new implementation.

### UI tree

`Menu` is the base node for the page tree. The two concrete layout types are:

- **`List`** — vertical scrolling list with optional sidebar progress bar
- **`Tile`** — horizontal icon grid with animated arrow/dotted-line foreground

Menu items are added via `addItem(Menu*)` or `addItem(Menu*, Widget*)`. The `Widget*` attaches an interactive control to that list entry. Available widgets: `CheckBox`, `PopUp`, `Slider`.

### Launcher

`Launcher` is the frame loop driver. It owns a `Camera` (virtual viewport offset), a `Selector` (animated highlight box), and a pointer to the current `Menu`. `Launcher::update()` is called every frame (blocking loop in `astraCoreStart`). Navigation — open/close sub-menus, widget interaction — is handled inside `update()`.

### Animation system

All motion uses exponential decay via `Animation::move(float *pos, float target, float speed)`. Speed is 0–100; higher is faster. Each animated element stores a current value and a target (`xTrg`, `yTrg`, etc); `move()` steps the current value toward the target each frame.

Global animation and layout parameters (speeds, margins, fonts) live in `astra::config` (`astra/config/config.h`). The singleton is `astra::getUIConfig()`.

### Memory

The MCU has 20 KB RAM. A custom 13 KB pool (`hal_dreamCore/memory_pool`) provides `myMalloc`/`myFree`. The STL containers (`std::vector`, `std::string`) inside the UI layer rely on this pool via the overridden `new`/`delete`.

Flash usage is already at ~91% — adding large assets or new STL usage may overflow.

## Key files

| File | Purpose |
|------|---------|
| `Core/Src/astra/astra_rocket.cpp` | Entry point: builds the menu tree, injects HAL, starts the loop |
| `Core/Src/astra/config/config.h` | All UI animation speeds and layout constants |
| `Core/Src/hal/hal.h` | HAL abstract interface — the contract between UI and hardware |
| `Core/Src/hal/hal_dreamCore/hal_dreamCore.h` | STM32F103 concrete HAL |
| `Core/Src/astra/ui/launcher.cpp` | Frame loop, navigation state machine |
| `Core/Src/astra/ui/item/menu/menu.h` | `Menu`, `List`, `Tile` definitions |
| `Core/Src/astra/ui/item/widget/widget.h` | `CheckBox`, `PopUp`, `Slider` definitions |
| `debugCfg.cfg` | OpenOCD config (CMSIS-DAP, STM32F103, 128 KB flash) |
