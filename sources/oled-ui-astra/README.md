# oled-ui-astra

基于 STM32F103CBTx 的 128×64 OLED 动效 UI 框架 —— **Astra UI**。

[Releases](https://github.com/dcfsswindy/oled-ui-astra/releases) · [Wiki](https://github.com/dcfsswindy/oled-ui-astra/wiki) · [Video](https://www.bilibili.com/video/BV16x421S7qc)

---

## 硬件

| 项目 | 规格 |
| --- | --- |
| MCU | STM32F103CBTx (Cortex-M3, 128KB Flash, 20KB RAM) |
| 显示 | 128×64 OLED，SPI 接口，u8g2 驱动 |
| 输入 | 2 按键（KEY_0 确认，KEY_1 返回/导航） |

## 编译

### 前提

安装 **STM32CubeIDE 2.0+**，其内置了所有所需工具（arm-none-eabi-gcc、CMake、Ninja），无需额外安装。

### 在 VS Code 中编译（推荐）

1. 安装插件：[CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)、[C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
2. 用 VS Code 打开本仓库根目录
3. `Ctrl+Shift+P` → **CMake: Configure**（首次使用）
4. 按 `F7` 编译

`.vscode/` 目录已配置好指向 STM32CubeIDE 内置工具链，无需手动选择 Kit。

### 命令行编译

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja
```

编译产物位于 `build/`：`astra.elf` / `astra.hex` / `astra.bin`。

## 烧录

通过 ST-Link 连接板子（SWD 接口），然后：

### VS Code 一键烧录

`Ctrl+Shift+P` → **Tasks: Run Task** → **Flash (ST-Link)**

或先编译再烧录：**Build & Flash**

### 命令行烧录

```bash
STM32_Programmer_CLI.exe -c port=SWD -w build/astra.hex -v -rst
```

> `STM32_Programmer_CLI.exe` 位于 STM32CubeIDE 安装目录下的 `plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.*/tools/bin/`

---

powered by astra UI.
