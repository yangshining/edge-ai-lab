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

## 2. Overall Architecture

Three new directories under `main/`:

```
main/
├── audio/
│   ├── audio_io.c/.h        I2S mic + speaker init and blocking read/write
│   └── audio_service.c/.h   Four FreeRTOS tasks: input, codec, output, (send handled by assistant)
├── assistant/
│   ├── assistant_state.c/.h  Mutex-protected shared state (mirrors app_net_state pattern)
│   ├── assistant_proto.c/.h  Binary Protocol 2 framing + JSON message helpers
│   └── assistant.c/.h        WebSocket connection + main state machine task
└── hal/
    ├── btn.c/.h              esp-iot-button wrapper, single-click callback
    └── led_status.c/.h       Three-state RGB LED via led_strip RMT driver
```

### Data Flow

```
INMP441
  └─ I2S_NUM_0 RX → audio_input_task(P8) → encode_queue(2)
                                               └→ opus_codec_task(P2) → send_queue(40)
                                                                           └→ assistant_task(P5) → WebSocket → xiaozhi.me

xiaozhi.me → WebSocket → assistant_task → decode_queue(40)
                                              └→ opus_codec_task → playback_queue(2)
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

Driver reads stereo frames; left channel used for mono Opus encode.

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
int       audio_io_read(int16_t *buf, int samples);
void      audio_io_write(const int16_t *buf, int samples);
void      audio_io_set_output_sample_rate(uint32_t hz);
void      audio_io_enable_output(bool en);
```

---

## 5. Audio Service Layer `audio/audio_service.c/.h`

### Tasks

| Task | Priority | Stack | Role |
|------|----------|-------|------|
| `audio_input_task` | 8 | 6 KB | Read 960 samples/frame from mic → encode_queue |
| `opus_codec_task` | 2 | 24 KB | Encode (encode_queue→send_queue) + Decode (decode_queue→playback_queue) |
| `audio_output_task` | 4 | 4 KB | Pop playback_queue → audio_io_write |

### Queues (FreeRTOS QueueHandle_t)

| Queue | Slots | Contents |
|-------|-------|----------|
| encode_queue | 2 | Raw PCM frames (960×int16) |
| send_queue | 40 | Opus packets + timestamp |
| decode_queue | 40 | Opus packets + timestamp |
| playback_queue | 2 | Decoded PCM frames |

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
void      audio_service_push_decode(const uint8_t *opus, size_t len, uint32_t timestamp_ms);
bool      audio_service_pop_send(uint8_t *buf, size_t *len, uint32_t *timestamp_ms);
void      audio_service_set_decode_params(uint32_t sample_rate, int frame_ms);
```

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

Stateless helpers for Binary Protocol 2 (network byte order, 14-byte header) and JSON control messages.

```c
// Binary Protocol 2 header
// [uint16 version=2][uint16 type=0(audio)/1(json)][uint32 reserved][uint32 timestamp][uint32 payload_size][payload...]

size_t            proto_pack_audio(uint8_t *out, size_t out_size,
                                   const uint8_t *opus, size_t opus_len, uint32_t ts_ms);
void              proto_make_hello(char *buf, size_t size);
void              proto_make_listen_start(char *buf, size_t size, const char *session_id);
void              proto_make_listen_stop(char *buf, size_t size, const char *session_id);
void              proto_make_abort(char *buf, size_t size, const char *session_id);
server_msg_type_t proto_parse_server_msg(const char *json,
                                          char *session_id_out, char *text_out, size_t text_size);
```

### `assistant.c/.h` — State Machine Task (P5, 8 KB)

```
IDLE
  └─ assistant_start_listening() signal
       → ws connect, send hello JSON
CONNECTING
  └─ server hello received → save session_id, update decode params
       → audio_service_start_input(), send listen_start JSON
LISTENING
  ├─ loop: audio_service_pop_send() → proto_pack_audio() → ws send binary
  └─ server STT stop event → audio_service_stop_input(), send listen_stop JSON
THINKING
  └─ server TTS start event → SPEAKING
SPEAKING
  ├─ server binary frame → audio_service_push_decode()
  └─ server TTS end event → IDLE, close WebSocket
ERROR (any stage)
  └─ set ASSISTANT_ERROR, led_status_set(LED_ERROR)
```

On every state transition: `assistant_state_set_status()` + `led_status_set()`.

WebSocket URL and token via Kconfig:
```
CONFIG_ASSISTANT_WS_URL  = "wss://api.xiaozhi.me/xiaozhi/v1/"
CONFIG_ASSISTANT_WS_TOKEN = ""
```

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
    config ASSISTANT_BTN_GPIO       int  default 0
    config ASSISTANT_LED_GPIO       int  default 48
    config ASSISTANT_MIC_BCLK       int  default 1
    config ASSISTANT_MIC_WS         int  default 2
    config ASSISTANT_MIC_DIN        int  default 3
    config ASSISTANT_SPK_BCLK       int  default 4
    config ASSISTANT_SPK_LRCLK      int  default 5
    config ASSISTANT_SPK_DOUT       int  default 6
    config ASSISTANT_SPK_SD_MODE    int  default 7
    config ASSISTANT_WS_URL         string default "wss://api.xiaozhi.me/xiaozhi/v1/"
    config ASSISTANT_WS_TOKEN       string default ""
endmenu
```

---

## 9. New Component Dependencies (`main/idf_component.yml`)

```yaml
espressif/esp_audio_codec: "~2.4.1"   # Opus encoder + decoder
espressif/button: "~4.1.5"            # Button debounce
espressif/led_strip: "~3.0.2"         # WS2812 via RMT
```

`esp_websocket_client` is part of ESP-IDF — no additional declaration needed.

---

## 10. `main.c` Additions

Add after `app_prov_init()`, before `lcd_touch_init()`:

```c
assistant_state_init();
audio_io_init();
audio_service_init();
btn_init(CONFIG_ASSISTANT_BTN_GPIO);
btn_set_click_cb(assistant_start_listening);
led_status_init(CONFIG_ASSISTANT_LED_GPIO);
assistant_init();
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
| `main/CMakeLists.txt` | Add new source files |
| `main/idf_component.yml` | Add 3 dependencies |
| `Kconfig.projbuild` | Add Assistant Hardware menu |
