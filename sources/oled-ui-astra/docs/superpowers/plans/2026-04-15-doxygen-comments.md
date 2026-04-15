# Doxygen Comment & Indentation Standardization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite all comments in `astra/` and `hal/hal_dreamCore/` (excluding u8g2 third-party library) to Doxygen English style, and normalize indentation to consistent 2-space style.

**Architecture:** Each file is an independent transformation unit — read the file, rewrite comments and indentation, write back. No logic changes allowed. Header files (`.h`) are processed first (Batch 1, parallel), then implementation files (`.cpp`) (Batch 2, parallel).

**Tech Stack:** C++14, Doxygen comment style (`/** */`, `///< `, `@file`, `@brief`, `@param`, `@return`), 2-space indentation.

---

## Doxygen Style Rules (apply to every file)

1. **File header** — every file must start with:
   ```cpp
   /**
    * @file   filename.h
    * @brief  One-sentence description of the file's purpose.
    * @author Fir
    * @date   YYYY-MM-DD
    */
   ```
   Preserve the original `@date` from the existing `// Created by Fir on` header. If no date is present, use the file's original creation date comment or leave as `N/A`.

2. **Class / struct** — immediately above the declaration:
   ```cpp
   /**
    * @brief One-sentence description.
    *
    * Optional longer description paragraph.
    */
   class Foo { ... };
   ```

3. **Public method / function declaration** — above the declaration in the `.h`:
   ```cpp
   /**
    * @brief  What this function does.
    * @param  paramName  Description of parameter.
    * @return Description of return value. Omit tag if void.
    */
   void doSomething(int paramName);
   ```
   - Omit `@return` for `void` functions.
   - For overrides, add `@note Overrides HAL::methodName.` (or equivalent parent).

4. **Member variables** — use trailing `///<` Doxygen inline style:
   ```cpp
   float x;    ///< Current X position in pixels.
   float xTrg; ///< Target X position for animation.
   ```

5. **Private / internal logic comments** — use plain `//` single-line comments. Do **not** Doxygen-ify every implementation detail. Keep concise English.

6. **Remove** all Chinese comments. Replace with equivalent English.

7. **Remove** ASCII-art decorations (e.g., Chinese New Year banners) that are not meaningful documentation.

8. **Indentation** — 2 spaces per level. Fix any lines that use 4-space or tab indentation. Each level of nesting = 2 spaces; continuation lines inside an expression may align with the opening parenthesis.

9. **Do not change** any code logic, variable names, includes, or preprocessor directives.

---

## Batch 1 — Header Files (`.h`) — run all tasks in parallel

### Task 1: `Core/Src/hal/hal.h`

**Files:**
- Modify: `Core/Src/hal/hal.h`

- [ ] Read the full file and understand its content.
- [ ] Rewrite: add `@file` header, `@brief` for each class/struct, `@brief`/`@param`/`@return` for each public method, `///<` for member variables, plain `//` for internal logic.
- [ ] Replace all Chinese comments with English equivalents.
- [ ] Verify indentation is 2-space throughout.
- [ ] Write the modified file back (no logic changes).

---

### Task 2: `Core/Src/hal/hal_dreamCore/hal_dreamCore.h`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/hal_dreamCore.h`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Add `@brief` to the `HALDreamCore` class.
- [ ] Add `@brief`/`@param`/`@return` to each public override (drawing, key, beep, screen, canvas methods).
- [ ] Add `///<` to all member variables (e.g., `canvasBuffer`).
- [ ] Replace all Chinese comments with English.
- [ ] Fix any indentation inconsistencies.
- [ ] Write back.

---

### Task 3: `Core/Src/hal/hal_dreamCore/memory_pool.h`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/memory_pool.h`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Add `@brief` to all functions and structs.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 4: `Core/Src/astra/config/config.h`

**Files:**
- Modify: `Core/Src/astra/config/config.h`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Add `@brief` to the `UIConfig` struct and `getUIConfig()` function.
- [ ] Add `///<` to every config field (speeds, margins, fonts, etc.).
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 5: `Core/Src/astra/ui/launcher.h`

**Files:**
- Modify: `Core/Src/astra/ui/launcher.h`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Add `@brief` to the `Launcher` class.
- [ ] Add `@brief`/`@param`/`@return` to all public methods (`update`, `open`, `close`, etc.).
- [ ] Add `///<` to member variables (camera, selector, currentMenu pointer, etc.).
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 6: `Core/Src/astra/ui/item/menu/menu.h`

**Files:**
- Modify: `Core/Src/astra/ui/item/menu/menu.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to `Menu`, `List`, `Tile` classes.
- [ ] Add `@brief`/`@param`/`@return` to `addItem`, `render`, `update`, and navigation methods.
- [ ] Add `///<` to member variables (position, children vector, etc.).
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 7: `Core/Src/astra/ui/item/widget/widget.h`

**Files:**
- Modify: `Core/Src/astra/ui/item/widget/widget.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to `Widget`, `CheckBox`, `PopUp`, `Slider` classes.
- [ ] Add `@brief`/`@param`/`@return` to all public methods.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 8: `Core/Src/astra/ui/item/item.h`

**Files:**
- Modify: `Core/Src/astra/ui/item/item.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to `Item` base class and all methods.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 9: `Core/Src/astra/ui/item/camera/camera.h`

**Files:**
- Modify: `Core/Src/astra/ui/item/camera/camera.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to `Camera` class and all public methods.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 10: `Core/Src/astra/ui/item/selector/selector.h`

**Files:**
- Modify: `Core/Src/astra/ui/item/selector/selector.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to `Selector` class and all public methods.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 11: `Core/Src/astra/ui/item/plugin/plugin.h`

**Files:**
- Modify: `Core/Src/astra/ui/item/plugin/plugin.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to `Plugin` class and all public methods.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 12: `Core/Src/astra/astra_logo.h`

**Files:**
- Modify: `Core/Src/astra/astra_logo.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to any declared functions/arrays.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 13: `Core/Src/astra/astra_rocket.h`

**Files:**
- Modify: `Core/Src/astra/astra_rocket.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to declared functions.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 14: `Core/Src/astra/app/astra_app.h`

**Files:**
- Modify: `Core/Src/astra/app/astra_app.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to class and public methods.
- [ ] Add `///<` to member variables.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 15: `Core/Src/astra/app/astra_app_i.h`

**Files:**
- Modify: `Core/Src/astra/app/astra_app_i.h`

- [ ] Read the full file.
- [ ] Add `@file` header.
- [ ] Add `@brief` to any declared entities.
- [ ] Replace Chinese comments with English.
- [ ] Fix indentation.
- [ ] Write back.

---

## Batch 2 — Implementation Files (`.cpp`) — run all tasks in parallel after Batch 1 completes

### Task 16: `Core/Src/hal/hal.cpp`

**Files:**
- Modify: `Core/Src/hal/hal.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block (brief only, no method docs — those live in `.h`).
- [ ] Convert any Chinese block/inline comments to plain English `//` comments.
- [ ] Remove ASCII-art decorations if present.
- [ ] Fix indentation to 2-space.
- [ ] Write back.

---

### Task 17: `Core/Src/hal/hal_dreamCore/memory_pool.cpp`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/memory_pool.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 18: `Core/Src/hal/hal_dreamCore/components/hal_stm32.cpp`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/components/hal_stm32.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 19: `Core/Src/hal/hal_dreamCore/components/oled/hal_oled.cpp`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/components/oled/hal_oled.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 20: `Core/Src/hal/hal_dreamCore/components/hal_buzzer.cpp`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/components/hal_buzzer.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 21: `Core/Src/hal/hal_dreamCore/components/hal_key.cpp`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/components/hal_key.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 22: `Core/Src/hal/hal_dreamCore/components/hal_config.cpp`

**Files:**
- Modify: `Core/Src/hal/hal_dreamCore/components/hal_config.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 23: `Core/Src/astra/ui/launcher.cpp`

**Files:**
- Modify: `Core/Src/astra/ui/launcher.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese inline/block comments to English `//` comments.
- [ ] Remove ASCII-art decorations.
- [ ] Fix indentation (pay attention to the frame loop and navigation state machine).
- [ ] Write back.

---

### Task 24: `Core/Src/astra/ui/item/menu/menu.cpp`

**Files:**
- Modify: `Core/Src/astra/ui/item/menu/menu.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments (including any ASCII-art banner) to English `//` comments.
- [ ] Remove ASCII-art decorations.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 25: `Core/Src/astra/ui/item/widget/widget.cpp`

**Files:**
- Modify: `Core/Src/astra/ui/item/widget/widget.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 26: `Core/Src/astra/ui/item/camera/camera.cpp`

**Files:**
- Modify: `Core/Src/astra/ui/item/camera/camera.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 27: `Core/Src/astra/ui/item/selector/selector.cpp`

**Files:**
- Modify: `Core/Src/astra/ui/item/selector/selector.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 28: `Core/Src/astra/ui/item/plugin/plugin.cpp`

**Files:**
- Modify: `Core/Src/astra/ui/item/plugin/plugin.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 29: `Core/Src/astra/astra_rocket.cpp`

**Files:**
- Modify: `Core/Src/astra/astra_rocket.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 30: `Core/Src/astra/astra_logo.cpp`

**Files:**
- Modify: `Core/Src/astra/astra_logo.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

### Task 31: `Core/Src/astra/app/astra_app.cpp`

**Files:**
- Modify: `Core/Src/astra/app/astra_app.cpp`

- [ ] Read the full file.
- [ ] Add `@file` header block.
- [ ] Convert Chinese comments to English `//` comments.
- [ ] Fix indentation.
- [ ] Write back.

---

## Final Task: Commit

- [ ] Stage all modified files:
  ```bash
  git add Core/Src/astra/ Core/Src/hal/hal.h Core/Src/hal/hal.cpp Core/Src/hal/hal_dreamCore/
  ```
- [ ] Commit from repo root (`D:/code/proj/edge-ai-lab`):
  ```bash
  git commit -m "[oled-ui-astra] docs: standardize comments to Doxygen English style"
  ```
