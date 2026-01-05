#include "rgbLight.h"
#include "../../keyboardConfig.h"
#include "WS2812.h"
#include <Arduino.h>

/* RGB 缓冲区 - WS2812 需要 GRB 格式 */
__xdata uint8_t rgb_buffer[RGB_LED_COUNT * 3];

/* RGB 状态 */
rgb_state_t rgb_state;

/* ==================== WS2812 显示 ==================== */

void rgb_show(void) {
    // 使用 WS2812 库发送数据到 P3.5
    // 注意：RGB_LED_COUNT 在你的配置中是 6，但实际只有 3 个灯珠
    // 只发送前 3 个灯珠的数据
    neopixel_show_P3_4(rgb_buffer, 9);
}

/* ==================== HSV 转 RGB ==================== */

void hsv_to_rgb_impl(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region, remainder, p, q, t;
    
    if (s == 0) {
        *r = v;
        *g = v;
        *b = v;
        return;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            *r = v; *g = t; *b = p;
            break;
        case 1:
            *r = q; *g = v; *b = p;
            break;
        case 2:
            *r = p; *g = v; *b = t;
            break;
        case 3:
            *r = p; *g = q; *b = v;
            break;
        case 4:
            *r = t; *g = p; *b = v;
            break;
        default:
            *r = v; *g = p; *b = q;
            break;
    }
}

rgb_color_t hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v) {
    rgb_color_t rgb;
    hsv_to_rgb_impl(h, s, v, &rgb.r, &rgb.g, &rgb.b);
    return rgb;
}

/* ==================== RGB 控制函数 ==================== */

void rgb_init(void) {
    uint8_t i;
    
    // 初始化 RGB 状态
    rgb_state.hue = 0;        // 红色
    rgb_state.sat = 255;      // 全饱和度
    rgb_state.val = RGB_BRIGHTNESS;
    rgb_state.enabled = 1;    // 默认开启
    

    uint8_t addrn = ((2 * ROWS_COUNT * COLS_COUNT) + ROWS_COUNT * COLS_COUNT + COLS_COUNT) * 2 + 8;
    rgb_state.hue = eeprom_read_byte(addrn + 0);
    rgb_state.sat = eeprom_read_byte(addrn + 1);
    rgb_state.val = eeprom_read_byte(addrn + 2);
    rgb_state.enabled = eeprom_read_byte(addrn + 3);

    // 清空缓冲区
    for (i = 0; i < RGB_LED_COUNT * 3; i++) {
        rgb_buffer[i] = 0;
    }
    
    // 初始化默认颜色并显示一次
    rgb_update_display();

    rgb_update_display();
}

void rgb_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= 3) return;  // 只有 3 个灯珠
    // 使用库提供的宏设置 GRB 格式的像素
    set_pixel_for_GRB_LED(rgb_buffer, index, r, g, b);
}

void rgb_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t i;
    // 只设置前 3 个灯珠
    for (i = 0; i < 3; i++) {
        set_pixel_for_GRB_LED(rgb_buffer, i, r, g, b);
    }
}

/* ==================== VIA 接口函数 ==================== */

void rgb_matrix_sethsv_noeeprom(uint8_t h, uint8_t s, uint8_t v) {
    rgb_state.hue = h;
    rgb_state.sat = s;
    rgb_state.val = v;
    
    // 设置后立即更新显示
    rgb_update_display();
}

void rgb_matrix_toggle_noeeprom(void) {
    rgb_state.enabled = !rgb_state.enabled;
    
    // 立即更新显示
    rgb_update_display();
}

/* 更新显示 - 只在需要时调用 */
void rgb_update_display(void) {
    uint8_t r, g, b;
    
    if (!rgb_state.enabled) {
        // 关闭时设置为全黑
        rgb_set_color_all(0, 0, 0);
    } else {
        // 开启时根据 HSV 设置颜色
        hsv_to_rgb_impl(rgb_state.hue, rgb_state.sat, rgb_state.val, &r, &g, &b);
        rgb_set_color_all(r, g, b);
    }
    
    // 立即显示
    rgb_show();
}