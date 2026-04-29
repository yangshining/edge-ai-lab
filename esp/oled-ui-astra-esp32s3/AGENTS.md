# oled-ui-astra-esp32s3 Agent Notes

## Project

This directory (`D:\code\proj\edge-ai-lab\esp\oled-ui-astra-esp32s3`) is the ESP32-S3 ESP-IDF port of the STM32 `oled-ui-astra` OLED UI demo.

- `main/`: ESP-IDF entry point. `app_main()` starts the Astra UI task.
- `components/astra/`: platform-neutral Astra UI/application code copied from the STM32 reference.
- `components/hal_esp32s3/`: ESP32-S3 HAL implementation, including GPIO, SPI, key input, u8g2, and the copied u8g2 sources.
- `sdkconfig.defaults`: committed default ESP-IDF configuration.
- `sdkconfig`: generated local ESP-IDF configuration; do not commit unless explicitly requested.

The STM32 reference implementation lives at:

`D:\code\proj\edge-ai-lab\sources\oled-ui-astra`

## Build

Use the conda `esp` environment with the local ESP-IDF checkout:

```powershell
conda activate esp
. D:\code\proj\esp-idf\export.ps1
idf.py build
```

Expected target is `esp32s3`. Successful builds generate:

`build\oled-ui-astra-esp32s3.bin`

If `build` was generated from another project path, run:

```powershell
idf.py fullclean
idf.py build
```

The current `build` directory was refreshed for this project path on 2026-04-29.

For board testing:

```powershell
idf.py -p COMx flash monitor
```

Replace `COMx` with the actual serial port.

## Development Guidance

- Keep `components/astra/` close to the STM32 reference unless ESP-IDF compiler/runtime constraints require changes.
- Keep ESP32-S3 specific behavior in `components/hal_esp32s3/`.
- The u8g2 C files are third-party style C sources; avoid broad formatting or warning cleanup in that directory.
- The OLED is SSD1306 128x64 over SPI. Current pin definitions are in `components/hal_esp32s3/hal_esp32s3.h`.
- `HALEspCore` uses ESP-IDF GPIO/SPI drivers and u8g2 callbacks; preserve explicit `ESP_ERROR_CHECK` calls for hardware initialization and SPI transfers.
- The UI loop should yield to FreeRTOS to avoid watchdog problems.
- The launcher throttles `HAL::keyScan()` to a 10 ms cadence; keep this aligned with the HAL long-press counter.
- Widget rows are activated by long-pressing KEY1. CheckBox toggles immediately; PopUp and Slider enter edit mode and exit on long-press KEY0.

## Git

Commit messages for this repository should include the project directory prefix:

```text
[oled-ui-astra-esp32s3] short imperative summary
```

Examples:

```text
[oled-ui-astra-esp32s3] complete ESP32-S3 OLED UI port build
[oled-ui-astra-esp32s3] update HAL pin configuration
```

Do not include unrelated changes from other projects, such as `D:\code\proj\edge-ai-lab\sources\oled-ui-astra`, unless the task explicitly asks for them.
