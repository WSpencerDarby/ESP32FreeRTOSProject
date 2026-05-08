# ESP32 FreeRTOS Scheduler Timing Demo

A multi-task FreeRTOS firmware for the **Adafruit Feather ESP32 V2** that demonstrates real-time task scheduling, hardware I/O, IMU-based acceleration detection, and live system observability through an HTTP dashboard and UDP logging.

---

## Features

- **7 concurrent FreeRTOS tasks** pinned across both ESP32 cores with distinct priorities and periods
- **MPU6050 DMP** for orientation and linear acceleration measurement
- **Two-tier acceleration detection** a low-frequency polling task notifies a high-priority task when movement crosses a threshold
- **Live HTTP dashboard** at `/` auto-refreshes every 5 seconds with per-task CPU%, execution time, jitter, and deadline misses
- **JSON REST API** at `/api/status` machine-readable snapshot of all task metrics, queryable by scripts or external tools
- **UDP log streaming** all log messages broadcast over UDP for real-time monitoring from a PC
- **LED patterns** 5 modes cycled by a button (off, sequential, alternating, wave, all blink)
- **Brightness control** potentiometer sets PWM LED brightness and speaker volume, with change logging
- **Morse code** SOS transmitted on a dedicated LED and buzzer
- **NTP time display** current date/time printed to Serial with timezone selection

---

## Hardware Requirements

| Component | Quantity |
|-----------|----------|
| Adafruit Feather ESP32 V2 | 1 |
| MPU6050 IMU (I2C) | 1 |
| LEDs | 5 (4 pattern + 1 Morse) |
| Push button | 1 |
| Potentiometer | 1 |
| Speaker / passive buzzer | 2 (1 brightness, 1 Morse) |
| Resistors (220Ω for LEDs) | 5 |

---

## Pin Configuration

| Signal | GPIO |
|--------|------|
| LED 1 | 12 |
| LED 2 | 13 |
| LED 3 | 15 |
| LED 4 | 27 |
| Button | 26 |
| PWM LED (brightness) | 25 |
| Potentiometer (ADC) | 34 |
| Speaker (brightness) | 32 |
| Morse LED | 33 |
| Morse Buzzer | 14 |
| I2C SCL (MPU6050) | 20 |
| I2C SDA (MPU6050) | 22 |

---

## Task Overview

| Task | Core | Priority | Period | Description |
|------|------|----------|--------|-------------|
| `LEDPattern` | 0 | 2 | 10 ms | Button debounce + 5 LED patterns |
| `BrightnessControl` | 1 | 2 | 50 ms | ADC pot → PWM LED + speaker |
| `MorseCode` | 0 | 1 | event | SOS on LED and buzzer |
| `DateTime` | 1 | 1 | 10 s | NTP fetch + Serial time print |
| `LowAccelHandle` | 0 | 1 | 500 ms | Polls MPU6050, notifies high-accel task |
| `HighAccelHandle` | 0 | 3 | 50 ms | Wakes on notification, reads DMP FIFO |
| `SchedulerLogger` | 1 | 1 | 5 s | Broadcasts task stats over UDP |

---

## Project Structure

```
ESP32FreeRTOSProject/
├── src/
│   ├── main.cpp                  # Setup, task creation, system init
│   ├── app_logger.cpp            # Mutex-protected Serial + UDP logger
│   ├── http_status_server.cpp    # HTTP dashboard + JSON API + stats logger task
│   ├── task_metrics.cpp          # Per-task timing accumulators (jitter, deadline misses)
│   ├── config/
│   │   └── config.h              # All pins, periods, priorities, Wi-Fi credentials
│   └── tasks/
│       └── task_definitions.cpp  # All FreeRTOS task implementations
├── include/
│   ├── app_logger.h
│   ├── http_status_server.h
│   ├── task_definitions.h
│   └── task_metrics.h
├── tools/
│   └── listen_udp_logs.py        # PC-side UDP log receiver
└── platformio.ini
```

---

## Setup & Configuration

### 1. Configure Wi-Fi credentials

Open `src/config/config.h` and update:

```c
#define WIFI_SSID     "your-network-name"
#define WIFI_PASSWORD "your-password"
```

### 2. (Optional) Adjust task parameters

All periods, priorities, stack sizes, and thresholds are defined in `src/config/config.h`. No other files need to be changed for tuning.

---

## Build & Flash

This project uses [PlatformIO](https://platformio.org/). Install the PlatformIO IDE extension for VS Code, then:

**Compile only (no board needed):**
```powershell
pio run
```

**Compile and flash to the board:**
```powershell
pio run --target upload
```

**Open Serial Monitor:**
```powershell
pio device monitor
```

Monitor baud rate is `115200`.

---

## HTTP Dashboard

Once the board is connected to Wi-Fi, the IP address is printed to Serial:

```
ESP32 IP address: 192.168.1.42
HTTP status server: http://192.168.1.42/
JSON API:          http://192.168.1.42/api/status
```

Open either URL in a browser on the same network.

### HTML Dashboard (`/`)
Shows system info (IP, uptime, heap) and a per-task table with CPU usage, execution time stats, jitter, and deadline misses. **Auto-refreshes every 5 seconds.**

### JSON API (`/api/status`)
Returns a JSON object with the same data in a machine-readable format. Useful for scripting, monitoring tools, or integrations.

Example response:
```json
{
  "ip": "192.168.1.42",
  "uptime_s": 137,
  "free_heap": 203456,
  "task_count": 9,
  "tasks": [
    {
      "name": "LedPattern",
      "period_ms": 10,
      "samples": 1370,
      "deadline_misses": 0,
      "cpu_pct": 0.4,
      "state": "blocked",
      "exec_us": { "last": 45, "avg": 42, "max": 89 },
      "jitter_us": { "last": 12, "avg_abs": 9, "max_abs": 31 }
    }
  ]
}
```

---

## UDP Log Monitoring

The firmware broadcasts log messages over UDP to `255.255.255.255:4210`. Use the included Python script on your PC to receive them.

**Basic usage (terminal output):**
```bash
python tools/listen_udp_logs.py
```

**Save to a file as JSON Lines:**
```bash
python tools/listen_udp_logs.py --format jsonl --output logs.jsonl
```

**Save as CSV:**
```bash
python tools/listen_udp_logs.py --format csv --output logs.csv
```

Log format on the wire:
```
<millis>    <LEVEL>    <TAG>    <message>
```

Notable log tags:

| Tag | Source | Description |
|-----|--------|-------------|
| `SCHED_STATS` | SchedulerLogger | Full per-task timing snapshot every 5 s |
| `BRIGHTNESS` | BrightnessControlTask | Logs when pot value changes significantly |
| `ACCEL` | Accel tasks | Logs when acceleration crosses the threshold |

---

## Libraries

| Library | Source |
|---------|--------|
| I2Cdevlib-Core | `jrowberg/I2Cdevlib-Core` |
| I2Cdevlib-MPU6050 | `jrowberg/I2Cdevlib-MPU6050` |
| ESP-IDF HTTP Server | Built into ESP32 Arduino framework |
| FreeRTOS | Built into ESP32 Arduino framework |
