# Hardware Audio I/O Integration Design

**Date:** 2026-04-30
**Project:** tft_touch_s3 (ESP32-S3, ESP-IDF, LVGL 9.3)
**Scope:** Integrate INMP441 mic, MAX98357A speaker, physical button, RGB LED into the existing voice-assistant avatar UI. Audio pipeline references xiaozhi-esp32 architecture.

---

## 1. Hardware Modules

| Module | Role | Interface |
|--------|------|-----------|
| INMP441 | I2S microphone | I2S_NUM_0 RX |
| MAX98357A | I2S Class-D amplifier + speaker | I2S_NUM_1 TX |
| Physical button | Wake trigger | GPIO (active-low, internal pull-up) |
| RGB LED (WS2812) | Status indicator | GPIO → RMT (led_strip) |

---

### Default GPIO Allocation

The assistant hardware defaults must not reuse the TFT/touch pins already defined in `Kconfig.projbuild`.
The table below is the default wiring target for this design. Board-specific overrides are allowed through
Kconfig, but the defaults must remain conflict-free.

| Function | GPIO | Existing / New | Notes |
|----------|------|----------------|-------|
| TFT SCLK | 18 | Existing | SPI2 shared bus |
| TFT MOSI | 17 | Existing | SPI2 shared bus |
| Touch MISO | 21 | Existing | XPT2046 data out |
| LCD DC | 5 | Existing | Reserved for ST7789 |
| LCD RST | 3 | Existing | Reserved for ST7789 |
| LCD CS | 4 | Existing | Reserved for ST7789 |
| LCD backlight | 2 | Existing | LEDC PWM |
| Touch CS | 15 | Existing | XPT2046 chip select |
| Assistant button | 38 | New | Active-low, internal pull-up |
| RGB LED | 48 | New | WS2812 via RMT; board-specific if onboard RGB differs |
| INMP441 BCLK | 8 | New | I2S RX |
| INMP441 WS | 9 | New | I2S RX |
| INMP441 DIN | 10 | New | I2S RX data input |
| MAX98357A BCLK | 11 | New | I2S TX |
| MAX98357A LRCLK | 12 | New | I2S TX |
| MAX98357A DOUT | 13 | New | I2S TX data output |
| MAX98357A SD_MODE | 14 | New | High=enable, low=mute |

Avoid GPIO0 and other boot strapping pins for new defaults. Avoid GPIO19/20 if USB-JTAG/USB-CDC is used
for flashing or logs on the target board.

---

## 2. Overall Architecture

Three new directories under `main/`:

```
main/
├── audio/
│   ├── audio_io.c/.h        I2S mic + speaker init and blocking read/write
│   └── audio_service.c/.h   Three audio FreeRTOS tasks: input, codec, output
├── assistant/
│   ├── assistant_state.c/.h  Mutex-protected shared state (mirrors app_net_state pattern)
│   ├── assistant_proto.c/.h  WebSocket v1 JSON message helpers
│   └── assistant.c/.h        WebSocket connection + main state machine task
└── hal/
    ├── btn.c/.h              esp-iot-button wrapper, single-click callback
    └── led_status.c/.h       Three-state RGB LED via led_strip RMT driver
```

### Data Flow

```
INMP441
  └─ I2S_NUM_0 RX → audio_input_task(P5) → encode_queue(2)
                                               └→ opus_codec_task(P3) → send_queue(40)
                                                                           └→ assistant_task(P5) → WebSocket → xiaozhi.me

xiaozhi.me → WebSocket → assistant_task → decode_queue(40)
                                              └→ opus_codec_task(P3) → playback_queue(2)
                                                                           └→ audio_output_task(P4) → I2S_NUM_1 TX → MAX98357A
```

UI `ui_page_ai.c` LVGL timer polls `assistant_state` and drives the existing avatar animation — no mock state machine.

---

## 3. HAL Layer

### `hal/btn.c/.h`

- Component: `espressif/button ~4.1.5`
- Single-click → calls `assistant_start_listening()`
- Long-press callback slot reserved for future use
- GPIO configured via `CONFIG_ASSISTANT_BTN_GPIO` (Kconfig)

```c
void btn_init(gpio_num_t gpio);
void btn_set_click_cb(void (*cb)(void));
void btn_set_long_press_cb(void (*cb)(void), uint32_t hold_ms);
```

### `hal/led_status.c/.h`

- Component: `espressif/led_strip ~3.0.2`, WS2812 GRB via RMT
- GPIO configured via `CONFIG_ASSISTANT_LED_GPIO` (Kconfig)

| Status | Color | Behavior |
|--------|-------|----------|
| `LED_IDLE` | Blue (0, 0, 32) | Steady |
| `LED_WORKING` | Green (0, 32, 0) | Breathe 500ms |
| `LED_SPEAKING` | Green (0, 20, 0) | Steady |
| `LED_ERROR` | Red (32, 0, 0) | Fast blink 200ms |

```c
typedef enum { LED_IDLE, LED_WORKING, LED_SPEAKING, LED_ERROR } led_status_t;
void led_status_init(gpio_num_t gpio);
void led_status_set(led_status_t status);
```

---

## 4. Audio I/O Layer `audio/audio_io.c/.h`

Two independent I2S channels. All GPIOs via Kconfig.

### INMP441 — I2S_NUM_0 RX

```c
i2s_chan_config_t: I2S_NUM_0, MASTER, auto_clear=true
i2s_std_config_t: sample_rate=16000, 16-bit, Philips, STEREO
gpio: BCLK=CONFIG_ASSISTANT_MIC_BCLK, WS=CONFIG_ASSISTANT_MIC_WS,
      DIN=CONFIG_ASSISTANT_MIC_DIN, MCLK=UNUSED
```

Driver reads stereo frames (2×int16 per sample). `audio_io_read(buf, n)` fills `n` **stereo pairs** = `2n` int16 values. `audio_input_task` always calls `audio_io_read(stereo_buf, 960)` (1920 int16 = 3840 bytes), then extracts the left channel into a 960-sample mono `audio_pcm_frame_t` before enqueuing:

```c
for (int i = 0; i < 960; i++) frame->samples[i] = stereo_buf[i * 2]; // left channel
```

### MAX98357A — I2S_NUM_1 TX

```c
i2s_chan_config_t: I2S_NUM_1, MASTER, auto_clear=true
i2s_std_config_t: sample_rate=24000 (updated after server hello), 16-bit, Philips, STEREO
gpio: BCLK=CONFIG_ASSISTANT_SPK_BCLK, LRCLK=CONFIG_ASSISTANT_SPK_LRCLK,
      DOUT=CONFIG_ASSISTANT_SPK_DOUT, MCLK=UNUSED
SD_MODE: CONFIG_ASSISTANT_SPK_SD_MODE — high=enable, low=mute
```

### Interface

```c
esp_err_t audio_io_init(void);
// Reads `stereo_pairs` stereo pairs with a bounded timeout; buf must hold
// stereo_pairs*2 int16 values. Returns <=0 on timeout/error so stop_input can
// quiesce the input task without waiting on an indefinite I2S read.
int       audio_io_read(int16_t *buf, int stereo_pairs);
void      audio_io_write(const int16_t *buf, int samples);
// Disables TX channel, reconfigures clock, re-enables TX channel. Must not be
// called while audio_output_task is writing — caller must quiesce output first.
esp_err_t audio_io_set_output_sample_rate(uint32_t hz);
// Controls MAX98357A SD_MODE pin (high=enable, low=mute).
void      audio_io_enable_output(bool en);
```

---

## 5. Audio Service Layer `audio/audio_service.c/.h`

### Tasks

| Task | Priority | Stack | Role |
|------|----------|-------|------|
| `audio_input_task` | 5 | 6 KB | Read 960 mono samples/frame from mic → encode_queue |
| `opus_codec_task` | 3 | 24 KB | Encode (encode_queue→send_queue) + Decode (decode_queue→playback_queue) |
| `audio_output_task` | 4 | 4 KB | Pop playback_queue → audio_io_write |

**Priority rationale:** LVGL task runs at P2. Codec at P3 ensures audio processing is not starved by display repaints. `audio_input_task` at P5 matches `assistant_task` — the I2S DMA interrupt wakes it; it does not spin. Both stay well below ESP-IDF WiFi/system tasks (P20+).

### Queues (FreeRTOS QueueHandle_t)

All queues pass **pointers** to heap-allocated buffers, not values. The producer allocates with `heap_caps_malloc`, the consumer frees after use. This avoids copying large PCM frames inside the queue and keeps queue item size to `sizeof(void*)`.

| Queue | Slots | Item type | Contents |
|-------|-------|-----------|----------|
| encode_queue | 2 | `audio_pcm_frame_t *` | 960 mono int16 samples + timestamp; producer: `audio_input_task`; consumer frees |
| send_queue | 40 | `audio_opus_pkt_t *` | Opus payload bytes + byte count + timestamp; producer: `opus_codec_task`; consumer frees |
| decode_queue | 40 | `audio_opus_pkt_t *` | Same struct; producer: `assistant_task`; consumer frees |
| playback_queue | 2 | `audio_pcm_frame_t *` | Decoded PCM + sample count; producer: `opus_codec_task`; consumer frees |

### Queue Ownership And Backpressure

Every queue item is heap-owned by the queue after a successful `xQueueSend`.
The consumer must free it after processing. If `xQueueSend` fails, the producer
still owns the item and must free or retry it before returning.

| Queue | Send timeout | Full-queue policy | Free responsibility |
|-------|--------------|-------------------|---------------------|
| encode_queue | 0 ms | Drop newest PCM frame; never block I2S input | `audio_input_task` frees dropped frame |
| send_queue | 20 ms | Drop oldest queued Opus packet, then enqueue newest once | Helper frees evicted packet; codec frees newest if retry fails |
| decode_queue | 20 ms | Drop oldest queued Opus packet, then enqueue newest once | Helper frees evicted packet; assistant frees newest if retry fails |
| playback_queue | 50 ms | Drop oldest decoded PCM frame, then enqueue newest once | Helper frees evicted frame; codec frees newest if retry fails |

Log each drop path with a rate-limited warning and increment per-queue drop
counters for later diagnostics. Do not block indefinitely in any audio queue
operation; bounded latency is more important than perfect retention for this
phase.

```c
// n_samples is 960 for encoder input; for decoder output it equals
// (server_sample_rate * frame_duration_ms / 1000), e.g. 1440 for 24kHz/60ms.
typedef struct { uint32_t timestamp_ms; size_t n_samples; int16_t samples[]; } audio_pcm_frame_t;
// Allocated as: heap_caps_malloc(sizeof(audio_pcm_frame_t) + n_samples * sizeof(int16_t), MALLOC_CAP_DEFAULT)

typedef struct { uint32_t timestamp_ms; size_t len; uint8_t data[]; } audio_opus_pkt_t;
// Allocated as: heap_caps_malloc(sizeof(audio_opus_pkt_t) + len, MALLOC_CAP_DEFAULT)
```

### Opus Parameters (identical to xiaozhi)

```
Encoder: 16 kHz, mono, 60 ms frames (960 samples), complexity=0, DTX+VBR, auto bitrate
Decoder: sample_rate and frame_duration set from server hello response
```

### Interface

```c
esp_err_t audio_service_init(void);
void      audio_service_start_input(void);
void      audio_service_stop_input(void);
// Copies `opus` immediately into a newly allocated audio_opus_pkt_t before
// enqueueing. The caller may pass a transient WebSocket callback buffer.
// If allocation or enqueue fails, push_decode frees any owned buffer before
// returning; it never enqueues the caller's pointer directly.
void      audio_service_push_decode(const uint8_t *opus, size_t len, uint32_t timestamp_ms);
// Blocking dequeue with 100 ms timeout. Returns true and fills buf/len/timestamp_ms
// on success; returns false on timeout (no frame available). Max Opus packet size
// is 1275 bytes (RFC 6716 §3.4); buf must be at least AUDIO_OPUS_PKT_MAX_BYTES.
// The LISTENING loop in assistant_task calls this with a short timeout so it can
// also check for incoming WebSocket events between send attempts.
#define AUDIO_OPUS_PKT_MAX_BYTES 1275
bool      audio_service_pop_send(uint8_t *buf, size_t *len, uint32_t *timestamp_ms);

// Pause/resume the output path (used around sample-rate reconfiguration).
// pause_output drains playback_queue and blocks the output task; times out after
// 200 ms and returns ESP_ERR_TIMEOUT if the task is stuck (caller should abort).
esp_err_t audio_service_pause_output(void);
void      audio_service_resume_output(void);

// Set decoder params. Must be called while output is paused (between pause/resume).
// Closes the existing Opus decoder and opens a new one for the new rate/duration.
void      audio_service_set_decode_params(uint32_t sample_rate, int frame_ms);

// Free all pending items from the named queues. Used on session boundaries so
// old audio cannot leak into the next conversation.
void      audio_service_flush_input_path(void);   // encode_queue + send_queue
void      audio_service_flush_output_path(void);  // decode_queue + playback_queue
void      audio_service_flush_all(void);
```

**Decode-params reconfiguration protocol** (called from `assistant_task` upon receiving server hello):

```
1. audio_service_pause_output()          // drain playback_queue, block output task
2. audio_io_set_output_sample_rate(hz)  // teardown + reconfigure I2S TX
3. audio_service_set_decode_params(hz, frame_ms)  // close old decoder, open new one
4. audio_service_resume_output()         // unblock output task
5. audio_io_enable_output(true)         // unmute SD_MODE
```

This sequence is only needed when the server-provided sample rate differs from the 24 kHz default. If the server hello confirms 24 kHz / 60 ms, steps 1–4 are skipped.

### Session Cleanup Protocol

All terminal paths must explicitly stop producers, unblock consumers, and free
queued buffers.

- On `TTS start`: call `audio_service_stop_input()`, then
  `audio_service_flush_input_path()` before entering `ASSISTANT_SPEAKING`.
  This prevents microphone packets captured before speech playback from being
  sent after the server has moved to TTS.
- On `TTS stop`: wait up to 500 ms for `playback_queue` to drain, then mute
  `SD_MODE` if closing the WebSocket. If the wait times out, call
  `audio_service_flush_output_path()` and continue cleanup.
- On local abort, server disconnect, network error, or `ASSISTANT_ERROR`: call
  `audio_service_stop_input()`, `audio_service_pause_output()`,
  `audio_service_flush_all()`, `audio_io_enable_output(false)`, close the
  WebSocket, then set the final assistant state.
- `audio_service_stop_input()` sets an input-enabled flag to false and relies
  on bounded `audio_io_read()` timeouts to let `audio_input_task` leave the read
  loop. It must not delete the task while it may be inside the I2S driver.

---

## 6. Assistant Layer `assistant/`

### `assistant_state.c/.h`

Mutex-protected, polled by UI LVGL timer. Pattern mirrors `app_net_state`.

```c
typedef enum {
    ASSISTANT_IDLE, ASSISTANT_CONNECTING, ASSISTANT_LISTENING,
    ASSISTANT_UPLOADING, ASSISTANT_THINKING, ASSISTANT_SPEAKING, ASSISTANT_ERROR,
} assistant_status_t;

typedef struct {
    assistant_status_t status;
    char session_id[64];
    char last_reply[128];
    esp_err_t last_error;
} assistant_state_t;

void assistant_state_init(void);
void assistant_state_get(assistant_state_t *out);
void assistant_state_set_status(assistant_status_t s);
```

### `assistant_proto.c/.h`

Target protocol for this design: XiaoZhi WebSocket protocol version 1 against
`CONFIG_ASSISTANT_WS_URL` (`wss://api.xiaozhi.me/xiaozhi/v1/` by default).
Control messages are JSON text frames. Audio messages are raw Opus WebSocket
binary frames. Do not prepend a Binary Protocol 2 header on this v1 path.

Binary Protocol 2 is intentionally out of scope for this phase. If a future
server endpoint requires `Protocol-Version: 2`, add a separate config option
and update the hello message, request headers, and audio framing together.

```c
void              proto_make_hello(char *buf, size_t size);
void              proto_make_listen_start(char *buf, size_t size, const char *session_id);
void              proto_make_listen_stop(char *buf, size_t size, const char *session_id);
void              proto_make_abort(char *buf, size_t size, const char *session_id);
server_msg_type_t proto_parse_server_msg(const char *json,
                                          char *session_id_out, char *text_out, size_t text_size);
```

### `assistant.c/.h` — State Machine Task (P5, 8 KB)

`assistant_init()` initializes state and queues but does **not** start the state-machine task. The task is created on the first `assistant_start_listening()` call (or re-created after ERROR reset). This ensures the task cannot fire before `ui_main_init()` has completed and the LVGL page is live.

Init order in `main.c`:
```c
assistant_state_init();   // mutex + state struct only
audio_io_init();
audio_service_init();
btn_init(...);
btn_set_click_cb(assistant_start_listening);
led_status_init(...);
assistant_init();         // queues only, no task yet
// --- LVGL + UI init ---
lcd_touch_init();
ui_main_init();           // LVGL task starts, ui_page_ai_init() runs
// assistant_task created lazily on first button press
```

```
IDLE
  └─ assistant_start_listening() signal
       → ws connect, send hello JSON
CONNECTING
  └─ server hello received → save session_id, update decode params
       → audio_service_start_input(), send listen_start JSON
LISTENING
  ├─ loop: audio_service_pop_send() → ws send raw Opus binary frame
  └─ server STT stop event → audio_service_stop_input(), send listen_stop JSON
THINKING
  └─ server TTS start event → SPEAKING
SPEAKING
  ├─ server binary frame → audio_service_push_decode()
  └─ server TTS end event → IDLE, close WebSocket
ERROR (any stage)
  └─ set ASSISTANT_ERROR, led_status_set(LED_ERROR)
  └─ task exits; next assistant_start_listening() recreates it
     Minimum retry interval: 3 s enforced in assistant_start_listening()
     (reject call if last attempt was < 3 s ago — prevents server hammering)
```

On every state transition: `assistant_state_set_status()` + `led_status_set()`.

WebSocket URL and token via Kconfig:
```
CONFIG_ASSISTANT_WS_URL  = "wss://api.xiaozhi.me/xiaozhi/v1/"
CONFIG_ASSISTANT_WS_TOKEN = ""
```

WebSocket request headers:

| Header | Value source | Notes |
|--------|--------------|-------|
| `Authorization` | `Bearer ${CONFIG_ASSISTANT_WS_TOKEN}` | Required when token is configured; fail fast with `ESP_ERR_INVALID_STATE` if empty and auth is required |
| `Protocol-Version` | Literal `"1"` | Matches the v1 raw-Opus WebSocket path used by this design |
| `Device-Id` | STA MAC from `esp_read_mac(..., ESP_MAC_WIFI_STA)` | Format as lowercase colon-separated MAC |
| `Client-Id` | UUID stored in NVS namespace `assistant`, key `client_id` | Generate once on first boot, persist across OTA and normal restarts |

Use `esp_websocket_client_config_t.headers` (or the ESP-IDF version's
equivalent header setter) to pass these during the HTTP upgrade. The hello JSON
still carries `version: 1`, `transport: "websocket"`, and 16 kHz mono Opus
audio parameters.

---

## 7. UI Integration `ui/ui_page_ai.c`

Replace mock state machine timer with `assistant_state` polling. Map to existing `ai_demo_state_t`:

```c
static const ai_demo_state_t kStateMap[] = {
    [ASSISTANT_IDLE]       = AI_DEMO_STATE_IDLE,
    [ASSISTANT_CONNECTING] = AI_DEMO_STATE_IDLE,
    [ASSISTANT_LISTENING]  = AI_DEMO_STATE_LISTENING,
    [ASSISTANT_UPLOADING]  = AI_DEMO_STATE_UPLOADING,
    [ASSISTANT_THINKING]   = AI_DEMO_STATE_THINKING,
    [ASSISTANT_SPEAKING]   = AI_DEMO_STATE_SPEAKING,
    [ASSISTANT_ERROR]      = AI_DEMO_STATE_ERROR,
};
```

`ui_ai_update_result()` signature unchanged; label set to `state.last_reply`.

---

## 8. Kconfig Additions (`Kconfig.projbuild`)

```
menu "Assistant Hardware"
    # WARNING: GPIO0 is the ESP32-S3 boot-mode strapping pin — do NOT use it.
    # Defaults must match the allocation table above and avoid TFT/touch pins.
    config ASSISTANT_BTN_GPIO       int  default 38
    config ASSISTANT_LED_GPIO       int  default 48
    config ASSISTANT_MIC_BCLK       int  default 8
    config ASSISTANT_MIC_WS         int  default 9
    config ASSISTANT_MIC_DIN        int  default 10
    config ASSISTANT_SPK_BCLK       int  default 11
    config ASSISTANT_SPK_LRCLK      int  default 12
    config ASSISTANT_SPK_DOUT       int  default 13
    config ASSISTANT_SPK_SD_MODE    int  default 14
    config ASSISTANT_WS_URL         string default "wss://api.xiaozhi.me/xiaozhi/v1/"
    config ASSISTANT_WS_TOKEN       string default ""
endmenu
```

---

## 9. New Component Dependencies (`main/idf_component.yml`)

```yaml
espressif/esp_audio_codec: "~2.4.1"   # Opus encoder + decoder (provides esp_opus_enc / esp_opus_dec API)
espressif/button: "~4.1.5"            # Button debounce
espressif/led_strip: "~3.0.2"         # WS2812 via RMT
espressif/esp_websocket_client: "~1.1.0"
```

`esp_audio_codec` is the correct component — it is the IDF component registry package that provides `esp_opus_enc_open`, `esp_opus_enc_process`, `esp_opus_dec_open`, `esp_opus_dec_decode` via the `esp_audio_enc.h` / `esp_audio_dec.h` headers, as used in xiaozhi-esp32 `audio_service.cc`. It is distinct from the older `esp-idf-lib` Opus port.

The implementation declares `espressif/esp_websocket_client` in `main/idf_component.yml` and requires `esp_websocket_client` from `main/CMakeLists.txt`.

---

## 10. `main.c` Additions

See init order in Section 6 (`assistant.c/.h`). Summary of additions:

```c
// After app_prov_init(), before lcd_touch_init():
assistant_state_init();
audio_io_init();
audio_service_init();
btn_init(CONFIG_ASSISTANT_BTN_GPIO);
btn_set_click_cb(assistant_start_listening);
led_status_init(CONFIG_ASSISTANT_LED_GPIO);
assistant_init();   // queues only — task created lazily

// Existing lcd_touch_init() and ui_main_init() remain in place after the above.
```

---

## 11. Files Changed / Created

| File | Change |
|------|--------|
| `main/hal/btn.c/.h` | New |
| `main/hal/led_status.c/.h` | New |
| `main/audio/audio_io.c/.h` | New |
| `main/audio/audio_service.c/.h` | New |
| `main/assistant/assistant_state.c/.h` | New |
| `main/assistant/assistant_proto.c/.h` | New |
| `main/assistant/assistant.c/.h` | New |
| `main/ui/ui_page_ai.c` | Replace mock state machine with assistant_state polling |
| `main/main.c` | Add 7 init calls |
| `main/CMakeLists.txt` | Add new source files (see below) |
| `main/idf_component.yml` | Add audio, button, LED, and WebSocket dependencies |
| `Kconfig.projbuild` | Add Assistant Hardware menu |

### `main/CMakeLists.txt` additions

Add to the existing `idf_component_register` call:

```cmake
# New source files
set(ASSISTANT_SRCS
    "hal/btn.c"
    "hal/led_status.c"
    "audio/audio_io.c"
    "audio/audio_service.c"
    "assistant/assistant_state.c"
    "assistant/assistant_proto.c"
    "assistant/assistant.c"
)

idf_component_register(
    SRCS
        # ... existing sources ...
        ${ASSISTANT_SRCS}
    INCLUDE_DIRS
        "."
        "ui"
        "connectivity"
        "hal"           # new
        "audio"         # new
        "assistant"     # new
    PRIV_REQUIRES
        # ... existing ...
        esp_driver_i2s
        esp_websocket_client
        espressif__cjson
        led_strip
        button
        esp_audio_codec
)
