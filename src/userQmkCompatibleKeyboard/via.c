// clang-format off
#include "via.h"
#include "USBhandler.h"
#include <Arduino.h>
#include "../../keyboardConfig.h"
#include "USBHIDKeyboardMouse.h"
#include "rgbLight.h"
// clang-format on

enum {
  ID_GET_PROTOCOL_VERSION = 0x01,
  ID_GET_KEYBOARD_VALUE,
  ID_SET_KEYBOARD_VALUE,
  ID_KEYMAP_GET_KEYCODE,
  ID_KEYMAP_SET_KEYCODE,
  ID_KEYMAP_RESET,
  ID_LIGHTING_SET_VALUE,
  ID_LIGHTING_GET_VALUE,
  ID_LIGHTING_SAVE,
  ID_EEPROM_RESET,
  ID_BOOTLOADER_JUMP,
  ID_MACRO_GET_COUNT,
  ID_MACRO_GET_BUFFER_SIZE,
  ID_MACRO_GET_BUFFER,
  ID_MACRO_SET_BUFFER,
  ID_MACRO_RESET, // 0x10
  ID_KEYMAP_GET_LAYER_COUNT,
  ID_KEYMAP_GET_BUFFER,
  ID_KEYMAP_SET_BUFFER,
  ID_UNHANDLED = 0xFF,
};

// VIA lighting channels
enum {
  CHANNEL_BACKLIGHT = 0,
  CHANNEL_UNDERGLOW = 1,
  CHANNEL_RGBMATRIX = 2
};

// VIA lighting values
enum {
  LIGHTING_BRIGHTNESS = 1,
  LIGHTING_EFFECT = 2,
  LIGHTING_EFFECT_SPEED = 3,
  LIGHTING_COLOR = 4
};

volatile __xdata uint8_t viaCmdReceived = 0;

void raw_hid_send() { USB_EP2_send(); }

void raw_hid_receive(void) { viaCmdReceived = 1; }

uint16_t dynamic_keymap_get_keycode(__data uint8_t layer, __xdata uint8_t row,
                                    uint8_t col) {
  __data uint8_t addr =
      ((layer * ROWS_COUNT * COLS_COUNT) + row * COLS_COUNT + col) * 2;
  return eeprom_read_byte(addr) << 8 | eeprom_read_byte(addr + 1);
}

void via_process(void) {
  if (viaCmdReceived == 0) {
    return;
  }
  viaCmdReceived = 0;

  memcpy(Ep2Buffer + 64, Ep2Buffer, 32);

  //Serial0_print("VIA CMD:");
  //Serial0_println(Ep2Buffer[0]);
  
  switch (Ep2Buffer[0]) {
  case ID_GET_PROTOCOL_VERSION:
    Ep2Buffer[64 + 1] = 1;
    Ep2Buffer[64 + 2] = 1;
    break;
    
  case ID_GET_KEYBOARD_VALUE:
    for (uint8_t i = 2; i < 32; i++) {
      Ep2Buffer[64 + i] = 0;
    }
    break;
    
  case ID_SET_KEYBOARD_VALUE:
    break;
    
  case ID_KEYMAP_GET_KEYCODE: {
    __data uint16_t keycode =
        dynamic_keymap_get_keycode(Ep2Buffer[1], Ep2Buffer[2], Ep2Buffer[3]);
    Ep2Buffer[64 + 4] = (keycode >> 8) & 0xFF;
    Ep2Buffer[64 + 5] = keycode & 0xFF;
  } break;
  
  case ID_KEYMAP_SET_KEYCODE: {
    __data uint8_t layer = Ep2Buffer[1];
    __data uint8_t row = Ep2Buffer[2];
    __data uint8_t col = Ep2Buffer[3];
    __data uint8_t addr =
        ((layer * ROWS_COUNT * COLS_COUNT) + row * COLS_COUNT + col) * 2;

    eeprom_write_byte(addr, Ep2Buffer[4]);
    eeprom_write_byte(addr + 1, Ep2Buffer[5]);
  } break;
  
  case ID_KEYMAP_RESET:
    break;
    
  case ID_LIGHTING_SET_VALUE: {
    __data uint8_t channel = Ep2Buffer[1];  // 通道
    __data uint8_t type = Ep2Buffer[2];     // 类型
    __data uint8_t value = Ep2Buffer[3];    // 值
    __data uint8_t value2 = Ep2Buffer[4];   // 额外值（如需要）
    
    // Serial0_print("Light CH:");
    // Serial0_print(channel);
    // Serial0_print(" Type:");
    // Serial0_print(type);
    // Serial0_print(" Val:");
    // Serial0_println(value);
    
    // 只处理 underglow channel (channel 1)
    if (channel == CHANNEL_RGBMATRIX) {
      switch (type) {
        case LIGHTING_BRIGHTNESS:
          rgb_state.val = value;
          // Serial0_print("Set brightness:");
          // Serial0_println(rgb_state.val);
          rgb_update_display();
          break;
          
        case LIGHTING_EFFECT:
          // Effect 0 = OFF, Effect 1 = Solid Color
          if (value == 0) {
            rgb_state.enabled = 0;
            //Serial0_println("RGB OFF");
          } else {
            rgb_state.enabled = 1;
            //Serial0_println("RGB ON");
          }
          rgb_update_display();
          break;
          
        case LIGHTING_EFFECT_SPEED:
          // 不需要速度控制
          break;
          
        case LIGHTING_COLOR:
          rgb_state.hue = value;
          rgb_state.sat = value2;
          // Serial0_print("Set hue:");
          // Serial0_println(rgb_state.hue);
          // Serial0_print("Set sat:");
          // Serial0_println(rgb_state.sat);
          rgb_update_display();
          break;
      }
    }
  } break;

  case ID_LIGHTING_GET_VALUE: {
    __data uint8_t channel = Ep2Buffer[1];
    __data uint8_t type = Ep2Buffer[2];
    
    // 只处理 underglow channel
    if (channel == CHANNEL_RGBMATRIX) {
      switch (type) {
        case LIGHTING_BRIGHTNESS:
          Ep2Buffer[64 + 3] = rgb_state.val;
          break;
          
        case LIGHTING_EFFECT:
          // 0 = OFF, 1 = Solid Color
          Ep2Buffer[64 + 3] = rgb_state.enabled ? 1 : 0;
          break;
          
        case LIGHTING_EFFECT_SPEED:
          Ep2Buffer[64 + 3] = 0;
          break;
          
        case LIGHTING_COLOR:
          Ep2Buffer[64 + 3] = rgb_state.hue;
          Ep2Buffer[64 + 4] = rgb_state.sat;
          break;
      }
    }
  } break;

  case ID_LIGHTING_SAVE:{
    uint8_t addrn = ((2 * ROWS_COUNT * COLS_COUNT) + ROWS_COUNT * COLS_COUNT + COLS_COUNT) * 2 + 8;
        eeprom_write_byte(addrn, rgb_state.hue);
        eeprom_write_byte(addrn + 1, rgb_state.sat);
        eeprom_write_byte(addrn + 2, rgb_state.val);
        eeprom_write_byte(addrn + 3, rgb_state.enabled);
  }
    break;
    
  case ID_EEPROM_RESET:
    break;
    
  case ID_BOOTLOADER_JUMP:
    break;
    
  case ID_MACRO_GET_COUNT:
    Ep2Buffer[64 + 1] = 0;
    break;
    
  case ID_MACRO_GET_BUFFER_SIZE:
    Ep2Buffer[64 + 2] = 0;
    Ep2Buffer[64 + 3] = 0;
    break;
    
  case ID_MACRO_GET_BUFFER:
    break;
    
  case ID_MACRO_SET_BUFFER:
    break;
    
  case ID_MACRO_RESET:
    break;

  case ID_KEYMAP_GET_LAYER_COUNT:
    Ep2Buffer[64 + 1] = LAYER_COUNT;
    break;
    
  case ID_KEYMAP_GET_BUFFER: {
    __data uint16_t offset =
        (Ep2Buffer[1] << 8) | Ep2Buffer[2];
    __data uint8_t size = Ep2Buffer[3];
    for (uint8_t i = 0; i < size; i++) {
      Ep2Buffer[64 + 4 + i] = eeprom_read_byte(offset + i);
    }
  } break;
  
  case ID_KEYMAP_SET_BUFFER: {
    __data uint16_t offset =
        (Ep2Buffer[1] << 8) | Ep2Buffer[2];
    __data uint8_t size = Ep2Buffer[3];
    for (uint8_t i = 0; i < size; i++) {
      eeprom_write_byte(offset + i, Ep2Buffer[4 + i]);
    }
  } break;
  
  default:
    Ep2Buffer[64 + 0] = ID_UNHANDLED;
    break;
  }
  
  raw_hid_send();
}

// clang-format off
#include "via.h"
#include "USBhandler.h"
#include <Arduino.h>
#include "../../keyboardConfig.h"
#include "USBHIDKeyboardMouse.h"
#include "rgbLight.h"
// clang-format on

// ... (保留原有的 VIA 协议处理代码，enum、via_process 等函数) ...

void press_qmk_key(__data uint8_t row, __xdata uint8_t col,
                   __xdata uint8_t layer, __xdata uint8_t press) {
  __data uint16_t keycode = dynamic_keymap_get_keycode(layer, row, col);
  __data uint8_t high_byte = (keycode >> 8) & 0x00FF;
  __data uint8_t low_byte = keycode & 0x00FF;

  // ==================== Mouse Keys ====================
  // QK_MOUSE_CURSOR_UP to QK_MOUSE_WHEEL_RIGHT (0x00CD-0x00DC)
  if (high_byte == 0x00 && low_byte >= 0xCD && low_byte <= 0xDC) {
    switch (low_byte) {
      case 0xCD: // MS_UP
        if (press) Mouse_move(0, -8);
        break;
      case 0xCE: // MS_DOWN
        if (press) Mouse_move(0, 8);
        break;
      case 0xCF: // MS_LEFT
        if (press) Mouse_move(-8, 0);
        break;
      case 0xD0: // MS_RIGHT
        if (press) Mouse_move(8, 0);
        break;
      case 0xD1: // MS_BTN1
        if (press) Mouse_press(1);
        else Mouse_release(1);
        break;
      case 0xD2: // MS_BTN2
        if (press) Mouse_press(2);
        else Mouse_release(2);
        break;
      case 0xD3: // MS_BTN3
        if (press) Mouse_press(4);
        else Mouse_release(4);
        break;
      case 0xD4: // MS_BTN4
        if (press) Mouse_press(8);
        else Mouse_release(8);
        break;
      case 0xD5: // MS_BTN5
        if (press) Mouse_press(16);
        else Mouse_release(16);
        break;
      case 0xD9: // MS_WHLU (Wheel up)
        if (press) Mouse_scroll(1);
        break;
      case 0xDA: // MS_WHLD (Wheel down)
        if (press) Mouse_scroll(-1);
        break;
    }
    return;
  }

  // ==================== Modifier + Key Combos ====================
  // QK_MODS range (0x0100-0x1FFF)
  // Format: 0x0MMM KKKK where MMM = modifiers, KKKK = keycode
  if (keycode >= 0x0100 && keycode <= 0x1FFF) {
    __data uint8_t mods = (keycode >> 8) & 0x1F;
    __data uint8_t key = keycode & 0xFF;
    
    if (press) {
      // Press modifiers
      if (mods & 0x01) Keyboard_quantum_modifier_press(0x01); // L Ctrl
      if (mods & 0x02) Keyboard_quantum_modifier_press(0x02); // L Shift
      if (mods & 0x04) Keyboard_quantum_modifier_press(0x04); // L Alt
      if (mods & 0x08) Keyboard_quantum_modifier_press(0x08); // L GUI
      if (mods & 0x10) Keyboard_quantum_modifier_press(0x10); // R Ctrl
      
      // Press key
      Keyboard_quantum_regular_press(key);
    } else {
      // Release key
      Keyboard_quantum_regular_release(key);
      
      // Release modifiers
      if (mods & 0x01) Keyboard_quantum_modifier_release(0x01);
      if (mods & 0x02) Keyboard_quantum_modifier_release(0x02);
      if (mods & 0x04) Keyboard_quantum_modifier_release(0x04);
      if (mods & 0x08) Keyboard_quantum_modifier_release(0x08);
      if (mods & 0x10) Keyboard_quantum_modifier_release(0x10);
    }
    return;
  }

  // ==================== System Control Keys ====================
  // KC_SYSTEM_POWER/SLEEP/WAKE (0x00A5-0x00A7)
  if (high_byte == 0x00 && low_byte >= 0xA5 && low_byte <= 0xA7) {
    __data uint8_t system_code = low_byte - 0xA4; // 1=Power, 2=Sleep, 3=Wake
    if (press) {
      System_press(system_code);
    } else {
      System_release();
    }
    return;
  }

  // ==================== Consumer Control Keys ====================
  // Media keys (0x00A8-0x00C2)
  if (high_byte == 0x00 && low_byte >= 0xA8 && low_byte <= 0xC2) {
    if (press) {
      __data uint16_t consumer_code = 0;
      
      switch (low_byte) {
        case 0xA8: consumer_code = 0x00E2; break; // KC_AUDIO_MUTE
        case 0xA9: consumer_code = 0x00E9; break; // KC_AUDIO_VOL_UP
        case 0xAA: consumer_code = 0x00EA; break; // KC_AUDIO_VOL_DOWN
        case 0xAB: consumer_code = 0x00B5; break; // KC_MEDIA_NEXT_TRACK
        case 0xAC: consumer_code = 0x00B6; break; // KC_MEDIA_PREV_TRACK
        case 0xAD: consumer_code = 0x00B7; break; // KC_MEDIA_STOP
        case 0xAE: consumer_code = 0x00CD; break; // KC_MEDIA_PLAY_PAUSE
        case 0xAF: consumer_code = 0x0183; break; // KC_MEDIA_SELECT
        case 0xB0: consumer_code = 0x00B8; break; // KC_MEDIA_EJECT
        case 0xB1: consumer_code = 0x018A; break; // KC_MAIL
        case 0xB2: consumer_code = 0x0192; break; // KC_CALCULATOR
        case 0xB3: consumer_code = 0x0194; break; // KC_MY_COMPUTER
        case 0xB4: consumer_code = 0x0221; break; // KC_WWW_SEARCH
        case 0xB5: consumer_code = 0x0223; break; // KC_WWW_HOME
        case 0xB6: consumer_code = 0x0224; break; // KC_WWW_BACK
        case 0xB7: consumer_code = 0x0225; break; // KC_WWW_FORWARD
        case 0xB8: consumer_code = 0x0226; break; // KC_WWW_STOP
        case 0xB9: consumer_code = 0x0227; break; // KC_WWW_REFRESH
        case 0xBA: consumer_code = 0x022A; break; // KC_WWW_FAVORITES
        case 0xBB: consumer_code = 0x00B3; break; // KC_MEDIA_FAST_FORWARD
        case 0xBC: consumer_code = 0x00B4; break; // KC_MEDIA_REWIND
        case 0xBD: consumer_code = 0x006F; break; // KC_BRIGHTNESS_UP
        case 0xBE: consumer_code = 0x0070; break; // KC_BRIGHTNESS_DOWN
        case 0xBF: consumer_code = 0x019F; break; // KC_CONTROL_PANEL
        case 0xC0: consumer_code = 0x01CB; break; // KC_ASSISTANT
        case 0xC1: consumer_code = 0x029F; break; // KC_MISSION_CONTROL
        case 0xC2: consumer_code = 0x02A0; break; // KC_LAUNCHPAD
        default:
          return;
      }
      
      Consumer_press(consumer_code);
    } else {
      Consumer_release();
    }
    return;
  }

  // ==================== Basic Keys ====================
  // Standard keyboard keys (0x0000-0x00FF)
  if (high_byte == 0x00 && low_byte <= 0xE7) {
    // Modifier keys (0x00E0-0x00E7)
    if (low_byte >= 0xE0 && low_byte <= 0xE7) {
      __data uint8_t mod_bit = 1 << (low_byte - 0xE0);
      if (press) {
        Keyboard_quantum_modifier_press(mod_bit);
      } else {
        Keyboard_quantum_modifier_release(mod_bit);
      }
      return;
    }
    
    // Regular keys (0x0004-0x00DF)
    if (press) {
      Keyboard_quantum_regular_press(low_byte);
    } else {
      Keyboard_quantum_regular_release(low_byte);
    }
    return;
  }

  // ==================== Transparent Key ====================
  if (keycode == 0x0001) { // KC_TRANSPARENT
    // Do nothing - fall through to lower layer
    return;
  }
  
  // ==================== No Operation ====================
  if (keycode == 0x0000) { // KC_NO
    // Do nothing
    return;
  }
}