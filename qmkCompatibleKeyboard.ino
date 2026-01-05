/*
  HID Keyboard + Encoder example
  QMK/VIA compatible
  CH552 / CH55xduino
*/

#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif


#define NUM_LEDS 3
#define COLOR_PER_LEDS 3
#define NUM_BYTES (NUM_LEDS * COLOR_PER_LEDS)


#define LONG_PRESS_TIME 500  // 长按阈值，毫秒，可自行调整

bool button1Prev = false;
bool button1Pressed = false;
unsigned long button1PressTime = 0;

#if NUM_BYTES > 255
#error "NUM_BYTES can not be larger than 255."
#endif

#include "src/WS2812.h"
#include "src/userQmkCompatibleKeyboard/USBHIDKeyboardMouse.h"
#include "src/userQmkCompatibleKeyboard/rgbLight.h"
#include "keyboardConfig.h"

/* ================== 状态变量 ================== */

bool hall1Pressed = false;
bool hall2Pressed = false;
bool hall3Pressed = false;

/* ---------- 全局变量 ---------- */
unsigned long previousKeyScanMillis = 0;
uint8_t layerInUse = 0;

/* ================== ADC 读取函数 ================== */

uint16_t readHallADC(uint8_t channel) {
  return analogRead(channel);;
}

/* ---------- 编码器状态机 ---------- */
int8_t encoder_read(void)
{
    static uint8_t last = 0;
    uint8_t current = 0;

    if (digitalRead(ENC_A)) current |= 0x01;
    if (digitalRead(ENC_B)) current |= 0x02;

    int8_t dir = 0;

    // 顺时针
    if ((last == 0b00 && current == 0b01) ||
        (last == 0b01 && current == 0b11) ||
        (last == 0b11 && current == 0b10) ||
        (last == 0b10 && current == 0b00)) {
        dir = 1;
    }
    // 逆时针
    else if ((last == 0b00 && current == 0b10) ||
             (last == 0b10 && current == 0b11) ||
             (last == 0b11 && current == 0b01) ||
             (last == 0b01 && current == 0b00)) {
        dir = -1;
    }

    last = current;
    return dir;
}

/* ---------- 初始化 ---------- */
void setup()
{
    USBInit();

    /* EEPROM 默认 keymap 初始化 */
    {
        __data uint8_t allConfigFF = 1;
        __data uint8_t dataLength = ROWS_COUNT * COLS_COUNT * LAYER_COUNT * 2;

        for (__data uint8_t i = 0; i < dataLength; i++) {
            if (eeprom_read_byte(i) != 0xFF) {
                allConfigFF = 0;
                break;
            }
        }

        if (allConfigFF) {
            // Layer0: Ctrl-C / Ctrl-V / Tab
            // Layer1: Cmd-C  / Cmd-V  / Tab
            const uint8_t defaultKeymap[] = {
    // Row 0
    0x00, 0x04,   // A
    0x00, 0x05,   // B
    0x00, 0x06,   // C

    // Row 1
    0x00, 0x07,   // D
    0x00, 0x08,   // E
    0x00, 0x09,   // F
        // Row 0
    0x00, 0x0A,   // A
    0x00, 0x0B,   // B
    0x00, 0x0C,   // C

    // Row 1
    0x00, 0x0D,   // D
    0x00, 0x0E,   // E
    0x00, 0x0F,   // F
            };
            for (__data uint8_t i = 0; i < dataLength; i++) {
                eeprom_write_byte(i, defaultKeymap[i]);
            }
        }
    }

    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    /* RGB 初始化 */
    rgb_init();
    //Serial0_begin(9600); 
    //Serial0_println("Hello");
}

/* ---------- 主循环 ---------- */
void loop()
{
    via_process();
    if ((signed int)(millis() - previousKeyScanMillis) < 5) {
        return;
    }
    previousKeyScanMillis = millis();

    // /* OS 自动选择 layer */
    // uint8_t osDetected = detected_host_os();
    // if (osDetected == OS_LINUX || osDetected == OS_WINDOWS) {
    //     layerInUse = 0;
    // } else {
    //     layerInUse = 1;
    // }

/* ---------- 普通按键 ---------- */
bool button1 = !digitalRead(BUTTON1_PIN);

/* 按下瞬间 */
if (button1 && !button1Prev) {
    button1PressTime = millis();
    button1Pressed = true;
}

/* 松开瞬间 */
if (!button1 && button1Prev && button1Pressed) {
    unsigned long pressDuration = millis() - button1PressTime;

    if (pressDuration < LONG_PRESS_TIME) {
        /* 短按：正常触发 QMK 键 */
        press_qmk_key(0, 1, layerInUse, true);
        press_qmk_key(0, 1, layerInUse, false);
    } else {
        /* 长按：切换层 */
        layerInUse = !layerInUse;
    }

    button1Pressed = false;
}

button1Prev = button1;

    /* ---------- 编码器 ---------- */
    static uint8_t step = 0;
    int8_t enc = encoder_read();

    if (enc != 0) {
        step++;
        if (step >= 4) {   // 每一格只触发一次
            step = 0;

            if (enc > 0) {
                // 顺时针 → key(0,1)
                press_qmk_key(0, 0, layerInUse, true);
                press_qmk_key(0, 0, layerInUse, false);
            } else {
                // 逆时针 → key(0,2)
                press_qmk_key(0, 2, layerInUse, true);
                press_qmk_key(0, 2, layerInUse, false);
            }
        }
    }


      /* -------- Hall 1 -------- */
    uint16_t hall1 = readHallADC(HALL1_ADC_CHANNEL);
    if (!hall1Pressed && hall1 > HALL_PRESS_THRESHOLD) {
      hall1Pressed = true;
      press_qmk_key(1, 2, layerInUse, true);
    } else if (hall1Pressed && hall1 < HALL_RELEASE_THRESHOLD) {
      hall1Pressed = false;
      press_qmk_key(1, 2, layerInUse, false);
    }

    /* -------- Hall 2 -------- */
    uint16_t hall2 = readHallADC(HALL2_ADC_CHANNEL);
    if (!hall2Pressed && hall2 > HALL_PRESS_THRESHOLD) {
      hall2Pressed = true;
      press_qmk_key(1, 0, layerInUse, true);
    } else if (hall2Pressed && hall2 < HALL_RELEASE_THRESHOLD) {
      hall2Pressed = false;
      press_qmk_key(1, 0, layerInUse, false);
    }

    /* -------- Hall 3 -------- */
    uint16_t hall3 = readHallADC(HALL3_ADC_CHANNEL);
    if (!hall3Pressed && hall3 > HALL_PRESS_THRESHOLD) {
      hall3Pressed = true;
      press_qmk_key(1, 1, layerInUse, true);
    } else if (hall3Pressed && hall3 < HALL_RELEASE_THRESHOLD) {
      hall3Pressed = false;
      press_qmk_key(1, 1, layerInUse, false);
    }
}
