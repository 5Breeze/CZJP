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

void press_qmk_key(__data uint8_t row, __xdata uint8_t col,
                   __xdata uint8_t layer, __xdata uint8_t press) {
  __data uint16_t keycode = dynamic_keymap_get_keycode(layer, row, col);
  __data uint8_t application = (keycode >> 8) & 0x00FF;
  __data uint8_t code = keycode & 0x00FF;

  if (application >= 0x01 &&
      application <= 0x1F) {
    if (application & 0x10) {
      for (__data uint8_t i = 0; i < 4; i++) {
        if (application & (1 << i)) {
          if (press) {
            Keyboard_quantum_modifier_press(1 << (4 + i));
          } else {
            Keyboard_quantum_modifier_release(1 << (4 + i));
          }
        }
      }
    } else {
      for (__data uint8_t i = 0; i < 4; i++) {
        if (application & (1 << i)) {
          if (press) {
            Keyboard_quantum_modifier_press(1 << (0 + i));
          } else {
            Keyboard_quantum_modifier_release(1 << (0 + i));
          }
        }
      }
    }
  }

  if (press) {
    Keyboard_quantum_regular_press(keycode & 0x00FF);
  } else {
    Keyboard_quantum_regular_release(keycode & 0x00FF);
  }
}