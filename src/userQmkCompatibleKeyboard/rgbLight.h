#ifndef __RGB_LIGHT_H__
#define __RGB_LIGHT_H__

#include <stdint.h>

/* RGB 颜色结构 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

/* RGB 状态 */
typedef struct {
    uint8_t hue;           // 色相 0-255
    uint8_t sat;           // 饱和度 0-255
    uint8_t val;           // 亮度 0-255
    uint8_t enabled;       // 开关
} rgb_state_t;

extern rgb_state_t rgb_state;

/* 基础函数 */
void rgb_init(void);
void rgb_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void rgb_set_color_all(uint8_t r, uint8_t g, uint8_t b);
void rgb_show(void);

/* HSV 转 RGB */
rgb_color_t hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v);

/* VIA 控制函数 */
void rgb_matrix_sethsv_noeeprom(uint8_t h, uint8_t s, uint8_t v);
void rgb_matrix_toggle_noeeprom(void);

/* 刷新显示（只在颜色改变时调用） */
void rgb_update_display(void);

#endif // __RGB_LIGHT_H__
