
#define ROWS_COUNT 2
#define COLS_COUNT 3
#define LAYER_COUNT 2

/* ---------- 引脚定义 ---------- */
#define BUTTON1_PIN   17
#define ENC_A         32
#define ENC_B         33

#define HALL1_ADC_CHANNEL 11   // P1.1
#define HALL2_ADC_CHANNEL 14   // P1.4
#define HALL3_ADC_CHANNEL 15   // P1.5

/* ================== ADC 阈值 ================== */
#define HALL_PRESS_THRESHOLD    245
#define HALL_RELEASE_THRESHOLD  225

/* ================== RGB 配置 ================== */
#define RGB_LED_COUNT     3         // 2x3 = 6 个灯
#define RGB_BRIGHTNESS    255       // 默认亮度 0-255

// pretend we are BN003 to let via know we are compatible
#define USB_VID 0x4249
#define USB_PID 0x4287
