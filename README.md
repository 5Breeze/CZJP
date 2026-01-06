## Overview
CZJP is a custom firmware/hardware qmk configuration project based on Arduino example for QMK keyboard devices, featuring WS2812 RGB LED control, USB HID keyboard/mouse emulation, ADC-based hall effect sensor input, and VIA-compatible USB identification.

## Core Components & Key Files

### 1. WS2812 RGB LED Control
Optimized low-level control for WS2812 addressable LEDs on CH552G, with strict timing compliance for reliable LED data transmission.

### 2. USB HID Keyboard/Mouse Emulation
Handles USB endpoint 1 communication for HID report transmission (keyboard, mouse, consumer/system control).

#### Features
- Supports multiple HID report types (keyboard, mouse, consumer control, system control).
- Timeout protection for USB endpoint write operations (50000 iterations × 5μs = 250ms timeout).
- Dynamic report length configuration based on report ID.

### 3. Hardware Configuration
Centralized pin, sensor, RGB, and USB configuration for the input device.

#### Key Configuration Parameters
```c
#define ROWS_COUNT 2
#define COLS_COUNT 3
#define LAYER_COUNT 2

/* ---------- Pin Definitions ---------- */
#define BUTTON1_PIN   17
#define ENC_A         32
#define ENC_B         33

#define HALL1_ADC_CHANNEL 11   // P1.1
#define HALL2_ADC_CHANNEL 14   // P1.4
#define HALL3_ADC_CHANNEL 15   // P1.5

/* ================== ADC Thresholds ================== */
#define HALL_PRESS_THRESHOLD    245
#define HALL_RELEASE_THRESHOLD  225

/* ================== RGB Configuration ================== */
#define RGB_LED_COUNT     3         // 2x3 = 6 LEDs total
#define RGB_BRIGHTNESS    255       // Default brightness (0-255)

// VIA compatibility: emulate BN003 device
#define USB_VID 0x4249
#define USB_PID 0x4287
```

## Key Hardware & Functional Features
### 1. Input Sensing
- **Hall Effect Sensors**: 3 ADC channels (P1.1/P1.4/P1.5) with configurable press/release thresholds for contactless key detection.
- **Mechanical Buttons/Encoders**: Dedicated pins for physical buttons (BUTTON1_PIN) and rotary encoders (ENC_A/ENC_B).

### 2. RGB Lighting
- WS2812 addressable LED control with 8051 assembly-optimized timing.
- Configurable LED count and brightness (0-255 range).

### 3. USB HID & Compatibility
- Multi-type HID report support (keyboard, mouse, consumer/system control).
- VIA-compatible USB VID/PID (emulates BN003) for keyboard customization via VIA software.

## Compatibility
- Target MCU: CH552G 8051-based microcontrollers (minimum 4 MHz clock for WS2812 support).
- USB: HID-compliant for keyboard/mouse emulation; VIA-compatible for configuration.
- Peripherals: WS2812 RGB LEDs, hall effect sensors, mechanical buttons/rotary encoders.

## Usage Notes
- **ADC Thresholds**: Adjust `HALL_PRESS_THRESHOLD`/`HALL_RELEASE_THRESHOLD` to match hardware sensitivity.
- **HALL ADC**: Ensure the HALL's VCC is 3.3V.
