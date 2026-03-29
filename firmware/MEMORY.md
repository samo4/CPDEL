# sousim2 Memory Map — Executive Overview

> Generated March 29, 2026 · ESP-IDF v6.0 · ESP32-S2

```powershell
.\esp.ps1 size
.\esp.ps1 size-components
```

## Flash

> 87% full (191 KB free of 1,472 KB app partition)

### Flash Code top consumers

| Library                                | KB  | %     |
| -------------------------------------- | --- | ----- |
| Wi-Fi (net80211 + pp + wpa_supplicant) | 225 | 23.2% |
| LVGL render engine                     | 178 | 18.8% |
| mbedTLS / TFPSACrypto                  | 103 | 10.6% |
| lwIP                                   | 99  | 10.4% |
| Modbus                                 | 25  | 2.6%  |
| App (libmain.a)                        | 21  | 2.2%  |

### Flash Data top consumers

| Library        | KB  | Notes                                    |
| -------------- | --- | ---------------------------------------- |
| `libesp_stdio` | 108 | printf/scanf format tables — pure rodata |
| LVGL           | 40  | Font bitmaps, style constants            |
| Wi-Fi stack    | 13  |                                          |

---

## DIRAM

> ⚠️ 99.1% full (1,579 bytes remaining)

Total capacity: 172,032 bytes · Used: 170,453 bytes · **No margin for new globals.**

### IRAM code (.text, 92 KB) — functions that must execute from RAM

| Library                | KB  | Why in IRAM                        |
| ---------------------- | --- | ---------------------------------- |
| Wi-Fi MAC (`libpp.a`)  | 19  | Timing-critical PHY/MAC            |
| SPI Flash driver       | 9   | Must run from RAM during flash ops |
| Wi-Fi PHY (`libphy.a`) | 9   |                                    |
| MSPI + GPSPI drivers   | 7.5 |                                    |
| Heap manager           | 7   |                                    |
| Wi-Fi `net80211`       | 5   |                                    |
| FreeRTOS               | 3.7 |                                    |

### BSS — zero-inited statics (57 KB)

| Library                | Bytes  | % of BSS |
| ---------------------- | ------ | -------- |
| LVGL static pool       | 37,988 | 65%      |
| Wi-Fi `net80211` state | 8,641  | 15%      |
| lwIP sockets/buffers   | 3,721  | 6%       |
| `libpp` (MAC state)    | 1,551  | 3%       |
| App (`libmain.a`)      | 1,399  | 2%       |
| FreeRTOS               | 684    | 1%       |

> The 37,988 bytes for LVGL includes the 36,864-byte static pool (`CONFIG_LV_MEM_SIZE_KILOBYTES=36`)
> plus ~1,124 bytes of other LVGL statics.

### Data — inited statics (16 KB)

SPI flash driver (5 KB), Wi-Fi MAC (3 KB), FreeRTOS (1.5 KB), misc HAL.

---

## Key Takeaways

1. **DIRAM** Only 1,579 bytes of static headroom remain. The runtime FreeRTOS heap (task stacks, dynamic allocations) draws from this plus any unclaimed IRAM regions. The LVGL task stack alone is 8 KB (HWM: 5,716 bytes used).

2. **LVGL pool owns 65% of all BSS.**

3. **Wi-Fi is a heavy IRAM tenant.**  
   net80211 + pp + phy together use ~33 KB of IRAM — more than FreeRTOS + heap manager combined. Fixed cost.

4. **`libesp_stdio` is a hidden flash cost.**  
   111 KB total, of which 110 KB is pure rodata (printf/scanf format tables). Not easily reclaimed without fully replacing printf.

5. **Flash has comfortable room.**  
   191 KB free in the app partition. SPIFFS, OTA image, or web assets can grow without pressure.

---

## Configuration Reference

```
CONFIG_LV_MEM_SIZE_KILOBYTES=36   # esp/sdkconfig + sdkconfig.defaults
# CONFIG_LV_MEM_CUSTOM is not set  # use fixed BSS pool, not system malloc
```

LVGL task: stack = 8192 bytes, priority = 5, HWM = 5,716 bytes (observed).
